
/*****************************************************************************/
/*                                                                           */
/*  THE KHE HIGH SCHOOL TIMETABLING ENGINE                                   */
/*  COPYRIGHT (C) 2010 Jeffrey H. Kingston                                   */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either Version 3, or (at your option)      */
/*  any later version.                                                       */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA   */
/*                                                                           */
/*  FILE:         khe_archive.c                                              */
/*  DESCRIPTION:  An archive holding instances and solution groups           */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#include <stdarg.h>
#define MAX_ERROR_STRING 200
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_INDEXED_INSTANCE - an instance with its index in archive (private)   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_indexed_instance_rec {
  KHE_INSTANCE			instance;
  int				index;
} *KHE_INDEXED_INSTANCE;

typedef HA_ARRAY(KHE_INDEXED_INSTANCE) ARRAY_KHE_INDEXED_INSTANCE;
typedef HN_TABLE(KHE_INDEXED_INSTANCE) TABLE_KHE_INDEXED_INSTANCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE - one archive of instances and solution groups               */
/*                                                                           */
/*****************************************************************************/

struct khe_archive_rec {
  HA_ARENA			arena;			/* arena for archive */
  HA_ARENA_SET			arena_set;		/* recycled archives */
  void				*back;			/* back pointer      */
  char				*id;			/* optional Id       */
  /* KHE_ARCHIVE_METADATA	meta_data; */		/* optional MetaData */
  char				*meta_name;		/* metadata name     */
  char				*meta_contributor;	/* " contributor     */
  char				*meta_date;		/* " date            */
  char				*meta_description;	/* " description     */
  char				*meta_remarks;		/* " remarks         */
  KHE_MODEL			model;			/* high school etc.  */
  ARRAY_KHE_INDEXED_INSTANCE	instance_array;		/* instance array    */
  TABLE_KHE_INDEXED_INSTANCE	instance_table;		/* instance table    */
  ARRAY_KHE_SOLN_GROUP		soln_group_array;	/* solution groups   */
  TABLE_KHE_SOLN_GROUP		soln_group_table;	/* solution groups   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_READ_STATE - info to pass around during incremental file reads       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_read_state_rec {
  HA_ARENA_SET		arena_set;
  KHE_ARCHIVE		archive;
  KHE_SOLN_GROUP	soln_group;
  bool			audit_and_fix;
  bool			resource_type_partitions;
  bool			infer_resource_partitions;
  bool			allow_invalid_solns;
  KHE_SOLN_TYPE		soln_type;
  KHE_ARCHIVE_FN	archive_begin_fn;
  KHE_ARCHIVE_FN	archive_end_fn;
  KHE_SOLN_GROUP_FN	soln_group_begin_fn;
  KHE_SOLN_GROUP_FN	soln_group_end_fn;
  KHE_SOLN_FN		soln_fn;
  void			*impl;
} *KHE_READ_STATE;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "version and version banner" (nothing to do with archives)     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheVersionNumber(void)                                             */
/*                                                                           */
/*  Return the version number of the version of KHE compiled with whatever   */
/*  executable is calling this function, e.g. "2.1".                         */
/*                                                                           */
/*****************************************************************************/

char *KheVersionNumber(void)
{
  return KHE_VERSION_NUMBER;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheVersionBanner(void)                                             */
/*                                                                           */
/*  Return the version banner of the version of KHE compiled with whatever   */
/*  executable is calling this function, e.g. "KHE Version 2.1 (May 2018)".  */
/*                                                                           */
/*****************************************************************************/

char *KheVersionBanner(void)
{
  return KHE_VERSION_BANNER;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INDEXED_INSTANCE KheIndexedInstanceMake(KHE_INSTANCE ins, int index) */
/*                                                                           */
/*  Make a new indexed instance object with these attributes.                */
/*                                                                           */
/*****************************************************************************/

static KHE_INDEXED_INSTANCE KheIndexedInstanceMake(KHE_INSTANCE ins, int index,
  HA_ARENA a)
{
  KHE_INDEXED_INSTANCE res;
  HaMake(res, a);
  res->instance = ins;
  res->index = index;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIndexedInstanceDelete(KHE_INDEXED_INSTANCE ii)                   */
/*                                                                           */
/*  Delete ii.                                                               */
/*                                                                           */
/*****************************************************************************/

/* *** don't delete now that there are arenas
static void KheIndexedInstanceDelete(KHE_INDEXED_INSTANCE ii)
{
  MFree(ii);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE KheArchiveMake(char *id, KHE_MODEL model)                    */
/*                                                                           */
/*  Make an initially empty archive, in its own arena.                       */
/*                                                                           */
/*****************************************************************************/

KHE_ARCHIVE KheArchiveMake(char *id, KHE_MODEL model, HA_ARENA_SET as)
{
  KHE_ARCHIVE res;  HA_ARENA a;
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->arena_set = as;
  res->back = NULL;
  res->id = HnStringCopy(id, a);
  res->meta_name = NULL;
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_description = NULL;
  res->meta_remarks = NULL;
  /* res->meta_data = md; */
  res->model = model;
  HaArrayInit(res->instance_array, a);
  HnTableInit(res->instance_table, a);
  HaArrayInit(res->soln_group_array, a);
  HnTableInit(res->soln_group_table, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveDelete(KHE_ARCHIVE archive)                               */
/*                                                                           */
/*  Delete archive along with its instances, solution groups, and solutions. */
/*                                                                           */
/*****************************************************************************/

void KheArchiveDelete(KHE_ARCHIVE archive)
{
  KheArchiveClear(archive);
  HaArenaDelete(archive->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveClear(KHE_ARCHIVE archive)                                */
/*                                                                           */
/*  Clear archive's instances and solution groups, leaving it empty except   */
/*  for its metadata.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheArchiveClear(KHE_ARCHIVE archive)
{
  KHE_INSTANCE ins;

  /* delete archive's solution groups */
  while( HaArrayCount(archive->soln_group_array) > 0 )
    KheSolnGroupDelete(HaArrayFirst(archive->soln_group_array), true);

  /* delete archive's instances, either completely or just from archive */
  while( HaArrayCount(archive->instance_array) > 0 )
  {
    ins = HaArrayFirst(archive->instance_array)->instance;
    if( KheInstanceArchiveCount(ins) == 1 )
      KheInstanceDelete(ins);
    else
      KheArchiveDeleteInstance(archive, ins);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveMeld(KHE_ARCHIVE dst_archive, KHE_ARCHIVE src_archive)    */
/*                                                                           */
/*  Meld the instances, solution groups, and solutions of src_archive        */
/*  into dst_archive.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheArchiveMeld(KHE_ARCHIVE dst_archive, KHE_ARCHIVE src_archive)
{
  int i, j;  KHE_INSTANCE ins;  KHE_SOLN_GROUP src_sg, dst_sg;  char *id;
  KHE_SOLN soln;

  /* check that the models are equal */
  HnAssert(dst_archive->model == src_archive->model,
    "KheArchiveMeld: dst_archive and src_archive have different models");

  /* meld the instances */
  for( i = 0;  i < KheArchiveInstanceCount(src_archive);  i++ )
  {
    ins = KheArchiveInstance(src_archive, i);
    if( !KheArchiveAddInstance(dst_archive, ins) )
      HnAbort("KheArchiveMeld:  cannot add instance \"%s\" of src_archive "
	"to dst_archive", KheInstanceId(ins));
  }

  /* meld the soln groups */
  for( i = 0;  i < KheArchiveSolnGroupCount(src_archive);  i++ )
  {
    /* get src_sg, a soln group from src_archive */
    src_sg = KheArchiveSolnGroup(src_archive, i);

    /* get (or make) dst_sg, a soln group from dst_archive */
    id = KheSolnGroupId(src_sg);
    if( !KheArchiveRetrieveSolnGroup(dst_archive, id, &dst_sg) )
    {
      if( !KheSolnGroupMake(dst_archive, id, &dst_sg) )
	HnAbort("KheArchiveMeld internal error");
    }

    /* copy the solutions of src_sg into dst_sg */
    for( j = 0;  j < KheSolnGroupSolnCount(src_sg);  j++ )
    {
      soln = KheSolnGroupSoln(src_sg, j);
      KheSolnGroupAddSoln(dst_sg, KheSolnCopy(soln, dst_archive->arena_set));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheArchiveId(KHE_ARCHIVE archive)                                  */
/*                                                                           */
/*  Return the Id of archive, possibly NULL.                                 */
/*                                                                           */
/*****************************************************************************/

char *KheArchiveId(KHE_ARCHIVE archive)
{
  return archive->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveSetBack(KHE_ARCHIVE archive, void *back)                  */
/*                                                                           */
/*  Set the back pointer of archive.                                         */
/*                                                                           */
/*****************************************************************************/

void KheArchiveSetBack(KHE_ARCHIVE archive, void *back)
{
  archive->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheArchiveBack(KHE_ARCHIVE archive)                                */
/*                                                                           */
/*  Retrieve the back pointer of archive.                                    */
/*                                                                           */
/*****************************************************************************/

void *KheArchiveBack(KHE_ARCHIVE archive)
{
  return archive->back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveSetMetaData(KHE_ARCHIVE archive, char *name,              */
/*    char *contributor, char *date, char *description, char *remarks)       */
/*                                                                           */
/*  Set the metadata fields to copies of these values.                       */
/*                                                                           */
/*****************************************************************************/

void KheArchiveSetMetaData(KHE_ARCHIVE archive, char *name,
  char *contributor, char *date, char *description, char *remarks)
{
  archive->meta_name = HnStringCopy(name, archive->arena);
  archive->meta_contributor = HnStringCopy(contributor, archive->arena);
  archive->meta_date = HnStringCopy(date, archive->arena);
  archive->meta_description = HnStringCopy(description, archive->arena);
  archive->meta_remarks = HnStringCopy(remarks, archive->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveMetaData(KHE_ARCHIVE archive, char **name,                */
/*    char **contributor, char **date, char **description, char **remarks)   */
/*                                                                           */
/*  Return the metadata fields.                                              */
/*                                                                           */
/*****************************************************************************/

void KheArchiveMetaData(KHE_ARCHIVE archive, char **name,
  char **contributor, char **date, char **description, char **remarks)
{
  *name = archive->meta_name;
  *contributor = archive->meta_contributor;
  *date = archive->meta_date;
  *description = archive->meta_description;
  *remarks = archive->meta_remarks;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheArchiveMetaDataText(KHE_ARCHIVE archive)                        */
/*                                                                           */
/*  Return the archive metadata as a paragraph of English text.              */
/*                                                                           */
/*****************************************************************************/

char *KheArchiveMetaDataText(KHE_ARCHIVE archive)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  char *s;

  /* handle the case of no metadata */
  if( HnStringIsWhiteSpaceOnly(archive->meta_name) &&
      HnStringIsWhiteSpaceOnly(archive->meta_contributor) &&
      HnStringIsWhiteSpaceOnly(archive->meta_date) &&
      HnStringIsWhiteSpaceOnly(archive->meta_description) &&
      HnStringIsWhiteSpaceOnly(archive->meta_remarks) )
    return "This archive has no metadata.";

  /* name */
  a = archive->arena;
  HnStringBegin(anc, a);
  if( HnStringIsWhiteSpaceOnly(archive->meta_name) )
    HnStringAdd(&anc, "This archive has no name.\n");
  else
    HnStringAdd(&anc, "This archive's name is %s.\n",
      HnStringCopyStripped(archive->meta_name, a));

  /* contributor and date */
  if( !HnStringIsWhiteSpaceOnly(archive->meta_contributor) )
  {
    if( !HnStringIsWhiteSpaceOnly(archive->meta_date) )
    {
      /* contributor and date */
      HnStringAdd(&anc, "It was contributed by %s on %s.\n",
        HnStringCopyStripped(archive->meta_contributor, a),
        HnStringCopyStripped(archive->meta_date, a));
    }
    else
    {
      /* contributor only */
      HnStringAdd(&anc, "It was contributed by %s.\n",
        HnStringCopyStripped(archive->meta_contributor, a));
    }
  }
  else
  {
    if( !HnStringIsWhiteSpaceOnly(archive->meta_date) )
    {
      /* date only */
      HnStringAdd(&anc, "  It was contributed on %s.\n",
        HnStringCopyStripped(archive->meta_date, a));
    }
  }

  /* description */
  if( !HnStringIsWhiteSpaceOnly(archive->meta_description) )
  {
    s = HnStringCopyStripped(archive->meta_description, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "%s\n", s);
    else
      HnStringAdd(&anc, "%s.\n", s);
  }

  /* remarks */
  if( !HnStringIsWhiteSpaceOnly(archive->meta_remarks) )
  {
    s = HnStringCopyStripped(archive->meta_remarks, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "%s\n", s);
    else
      HnStringAdd(&anc, "%s.\n", s);
  }

  /* all done */
  return HnStringEnd(anc);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE_METADATA KheArchiveMetaData(KHE_ARCHIVE archive)             */
/*                                                                           */
/*  Return the MetaData attribute of archive.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_ARCHIVE_METADATA KheArchiveMetaData(KHE_ARCHIVE archive)
{
  return archive->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveSetMetaData(KHE_ARCHIVE archive, KHE_ARCHIVE_METADATA md) */
/*                                                                           */
/*  Set the metadata attribute of archive to md.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheArchiveSetMetaData(KHE_ARCHIVE archive, KHE_ARCHIVE_METADATA md)
{
  archive->meta_data = md;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MODEL KheArchiveModel(KHE_ARCHIVE archive)                           */
/*                                                                           */
/*  Return the model of archive.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_MODEL KheArchiveModel(KHE_ARCHIVE archive)
{
  return archive->model;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheArchiveArenaBegin(KHE_ARCHIVE archive, bool large)           */
/*                                                                           */
/*  Return a fresh arena.                                                    */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheArchiveArenaBegin(KHE_ARCHIVE archive)
{
  return HaArenaMake(archive->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveArenaEnd(KHE_ARCHIVE archive, HA_ARENA a)                 */
/*                                                                           */
/*  Recycle a through archive's arena set.                                   */
/*                                                                           */
/*****************************************************************************/

void KheArchiveArenaEnd(KHE_ARCHIVE archive, HA_ARENA a)
{
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "instances"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveAddInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)        */
/*                                                                           */
/*  Add ins to archive and return true, or it that is not possible because   */
/*  the models disagree or the instance Id is already in use, return false.  */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveAddInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)
{
  KHE_INDEXED_INSTANCE ii;  char *id;  int pos;

  /* make sure models agree */
  if( KheInstanceModel(ins) != KheArchiveModel(archive) )
    return false;

  /* if the instance has an id, make sure it is not already in use */
  id = KheInstanceId(ins);
  if( id != NULL && HnTableContains(archive->instance_table, id, pos) )
    return false;

  /* all good, so go ahead and add the instance */
  ii = KheIndexedInstanceMake(ins, HaArrayCount(archive->instance_array),
    archive->arena);
  HaArrayAddLast(archive->instance_array, ii);
  if( id != NULL )
    HnTableAdd(archive->instance_table, id, ii);
  KheInstanceAddArchive(ins, archive);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveDeleteInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)     */
/*                                                                           */
/*  Delete ins from archive.  Also delete any solutions for ins.             */
/*                                                                           */
/*****************************************************************************/

void KheArchiveDeleteInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins)
{
  int pos, i;  char *id;  KHE_SOLN_GROUP sg;
  KHE_INDEXED_INSTANCE ii, ii2;

  if( DEBUG5 )
    fprintf(stderr, "[ KheArchiveDeleteInstance(%s, %s)\n",
      KheArchiveId(archive), KheInstanceId(ins));

  /* find ins's indexed instance, either by retrieving by id or searching; */
  /* also delete the indexed instance from archive's table */
  ii = NULL;
  id = KheInstanceId(ins);
  if( id != NULL )
  {
    if( !HnTableRetrieve(archive->instance_table, id, ii, pos) )
      HnAbort("KheArchiveDeleteInstance: instance %s not in archive", id);
    HnTableDelete(archive->instance_table, pos);
  }
  else
  {
    HaArrayForEach(archive->instance_array, ii, i)
      if( ii->instance == ins )
	break;
    if( i >= HaArrayCount(archive->instance_array) )
      HnAbort("KheArchiveDeleteInstance: instance not in archive");
  }

  /* remove ii from archive's array and change indexes of following entries */
  ii2 = HaArray(archive->instance_array, ii->index);
  HnAssert(ii2 == ii && ii->instance == ins,
    "KheArchiveDeleteInstance internal error");
  for( i = ii->index + 1;  i < HaArrayCount(archive->instance_array);  i++ )
  {
    ii2 = HaArray(archive->instance_array, i);
    HaArrayPut(archive->instance_array, i - 1, ii2);
    ii2->index = i - 1;
  }
  HaArrayDeleteLast(archive->instance_array);

  /* delete archive from ins */
  KheInstanceDeleteArchive(ins, archive);

  /* delete solutions for ins from archive's solution groups */
  HaArrayForEach(archive->soln_group_array, sg, i)
    KheSolnGroupDeleteInstance(sg, ii->instance, ii->index);

  /* delete ii */
  /* KheIndexedInstanceDelete(ii); */

  /* delete the link between archive and ins */
  /* ***
  HnAssert(MArrayContains(archive->instance_array, ins, &pos),
    "KheArchiveDeleteInstance: ins not in archive");
  MArrayRemove(archive->instance_array, pos);
  KheInstanceDeleteArchive(ins, archive);
  if( KheInstanceId(ins) != NULL )
  {
    if( !MTableContains(archive->instance_table, KheInstanceId(ins), &pos) )
      HnAbort("KheArchiveDeleteInstance: internal error");
    MTableDelete(archive->instance_table, pos);
  }
  *** */

  /* delete solutions for ins in archive */
  /* ***
  HaArrayForEach(archive->soln_group_array, sg, i)
    KheSolnGroupDeleteSolnsForInstance(sg, ins);
  *** */
  if( DEBUG5 )
    fprintf(stderr, "] KheArchiveDeleteInstance returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheArchiveInstanceCount(KHE_ARCHIVE archive)                         */
/*                                                                           */
/*  Return the number of instances in archive.                               */
/*                                                                           */
/*****************************************************************************/

int KheArchiveInstanceCount(KHE_ARCHIVE archive)
{
  return HaArrayCount(archive->instance_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheArchiveInstance(KHE_ARCHIVE archive, int i)              */
/*                                                                           */
/*  Return the i'th instance of archive.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheArchiveInstance(KHE_ARCHIVE archive, int i)
{
  return HaArray(archive->instance_array, i)->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveRetrieveInstance(KHE_ARCHIVE archive, char *id,           */
/*    KHE_INSTANCE *ins)                                                     */
/*                                                                           */
/*  Retrieve an instance with the given id from archive.                     */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveRetrieveInstance(KHE_ARCHIVE archive, char *id,
  KHE_INSTANCE *ins, int *index)
{
  int pos;  KHE_INDEXED_INSTANCE ii;
  if( HnTableRetrieve(archive->instance_table, id, ii, pos) )
    return *ins = ii->instance, *index = ii->index, true;
  else
    return *ins = NULL, *index = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveFindInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins,       */
/*    KHE_INDEXED_INSTANCE *ii)                                              */
/*                                                                           */
/*  If archive contains ins, return true with *ii set to its indexed         */
/*  instance object.  Otherwise return false, with *ii set to NULL.          */
/*                                                                           */
/*****************************************************************************/

static bool KheArchiveFindInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins,
  KHE_INDEXED_INSTANCE *ii)
{
  char *id;  KHE_INDEXED_INSTANCE ii2;  int i;
  id = KheInstanceId(ins);
  if( id != NULL )
  {
    if( HnTableRetrieve(archive->instance_table, id, ii2, i) )
      return *ii = ii2, true;
  }
  else
  {
    HaArrayForEach(archive->instance_array, ii2, i)
      if( ii2->instance == ins )
	return *ii = ii2, true;
  }
  return *ii = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveInstanceIndex(KHE_ARCHIVE archive, KHE_INSTANCE ins,      */
/*    int *index)                                                            */
/*                                                                           */
/*  Return the index of instance in archive.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveContainsInstance(KHE_ARCHIVE archive, KHE_INSTANCE ins,
  int *index)
{
  KHE_INDEXED_INSTANCE ii;
  if( KheArchiveFindInstance(archive, ins, &ii) )
    return *index = ii->index, true;
  else
    return *index = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solution groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveAddSolnGroup(KHE_ARCHIVE archive,                         */
/*    KHE_SOLN_GROUP soln_group)                                             */
/*                                                                           */
/*  Add soln_group to archive, assuming it is safe (not duplicate Id).       */
/*                                                                           */
/*****************************************************************************/

void KheArchiveAddSolnGroup(KHE_ARCHIVE archive, KHE_SOLN_GROUP soln_group)
{
  char *id;
  HaArrayAddLast(archive->soln_group_array, soln_group);
  id = KheSolnGroupId(soln_group);
  if( id != NULL )
    HnTableAdd(archive->soln_group_table, id, soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveDeleteSolnGroup(KHE_ARCHIVE archive,                      */
/*    KHE_SOLN_GROUP soln_group)                                             */
/*                                                                           */
/*  Delete soln_group from archive.  It must be present.                     */
/*                                                                           */
/*****************************************************************************/

void KheArchiveDeleteSolnGroup(KHE_ARCHIVE archive, KHE_SOLN_GROUP soln_group)
{
  char *id;  int pos;  KHE_SOLN_GROUP sg;
  if( !HaArrayContains(archive->soln_group_array, soln_group, &pos) )
    HnAbort("KheArchiveDeleteSolnGroup internal error 1");
  HaArrayDeleteAndShift(archive->soln_group_array, pos);
  id = KheSolnGroupId(soln_group);
  if( id != NULL )
  {
    if( !HnTableRetrieve(archive->soln_group_table, id, sg, pos) ||
	sg != soln_group )
      HnAbort("KheArchiveDeleteSolnGroup internal error 2");
    HnTableDelete(archive->soln_group_table, pos);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheArchiveSolnGroupCount(KHE_ARCHIVE archive)                        */
/*                                                                           */
/*  Return the number of solution groups in archive.                         */
/*                                                                           */
/*****************************************************************************/

int KheArchiveSolnGroupCount(KHE_ARCHIVE archive)
{
  return HaArrayCount(archive->soln_group_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP KheArchiveSolnGroup(KHE_ARCHIVE archive, int i)           */
/*                                                                           */
/*  Return the i'th solution group of archive.                               */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_GROUP KheArchiveSolnGroup(KHE_ARCHIVE archive, int i)
{
  return HaArray(archive->soln_group_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveRetrieveSolnGroup(KHE_ARCHIVE archive, char *id,          */
/*    KHE_SOLN_GROUP *soln_group)                                            */
/*                                                                           */
/*  Retrieve a solution group with the given id from archive.                */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveRetrieveSolnGroup(KHE_ARCHIVE archive, char *id,
  KHE_SOLN_GROUP *soln_group)
{
  int pos;
  return HnTableRetrieve(archive->soln_group_table, id, *soln_group, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading archives"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveMetaDataMakeFromKml(KML_ELT md_elt,                       */
/*    KHE_ARCHIVE archive, KML_ERROR *ke)                                    */
/*                                                                           */
/*  Make an archive metadata object based on md_elt and add it to            */
/*  archive.                                                                 */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveMetaDataMakeFromKml(KML_ELT md_elt, KHE_ARCHIVE archive,
  KML_ERROR *ke)
{
  if( !KmlCheck(md_elt,": $Name $Contributor $Date $Description +$Remarks",ke) )
    return false;
  KheArchiveSetMetaData(archive,
    KmlText(KmlChild(md_elt, 0)),
    KmlText(KmlChild(md_elt, 1)),
    KmlText(KmlChild(md_elt, 2)),
    KmlText(KmlChild(md_elt, 3)),
    KmlChildCount(md_elt) == 5 ? KmlText(KmlChild(md_elt, 4)) : NULL);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveEltToArchive(KML_ELT archive_elt, KHE_ARCHIVE *archive,   */
/*    KML_ERROR *ke, HA_ARENA a)                                             */
/*                                                                           */
/*  Convert archive_elt into *archive.                                       */
/*                                                                           */
/*  Arena a is used only for error messages.                                 */
/*                                                                           */
/*  Implementation note.  Because this function is called only by            */
/*  segmented reads, there are no instances and no solution groups           */
/*  under archive_elt when it is called.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheArchiveEltToArchive(KML_ELT archive_elt, HA_ARENA_SET as,
  KHE_ARCHIVE *archive, KML_ERROR *ke, HA_ARENA a)
{
  char *id;  KML_ELT metadata_elt;  KHE_ARCHIVE res;  KHE_MODEL model;

  /* fail if archive_elt has problems */
  if( strcmp(KmlLabel(archive_elt), "HighSchoolTimetableArchive") == 0 )
    model = KHE_MODEL_HIGH_SCHOOL_TIMETABLE;
  else if( strcmp(KmlLabel(archive_elt), "EmployeeScheduleArchive") == 0 )
    model = KHE_MODEL_EMPLOYEE_SCHEDULE;
  else
  {
    *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
      "file does not begin with <HighSchoolTimetableArchive>"
      " or <EmployeeScheduleArchive>");
    return *archive = NULL, false;
  }
  /* NB Instances opening tag will have been read by this time */
  if( !KmlCheck(archive_elt, "+Id : +MetaData +Instances +SolutionGroups", ke) )
    return *archive = NULL, false;

  /* create archive with optional id and optional metadata */
  id = KmlAttributeCount(archive_elt) == 0 ? NULL :
    KmlAttributeValue(archive_elt, 0);
  res = KheArchiveMake(id, model, as);
  if( KmlContainsChild(archive_elt, "MetaData", &metadata_elt) &&
      !KheArchiveMetaDataMakeFromKml(metadata_elt, res, ke) )
  {
    HnAssert(*ke != NULL, "KheArchiveEltToArchive internal error 1");
    return *archive = NULL, false;
  }

  /* all good */
  return *archive = res, *ke = NULL, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveEltCheckArchive(KML_ELT archive_elt,                      */
/*    KHE_ARCHIVE archive, KML_ERROR *ke, HA_ARENA a)                        */
/*                                                                           */
/*  Like KheArchiveEltToArchive except that it checks archive_elt against    */
/*  an existing archive rather than converting it into an archive.           */
/*                                                                           */
/*  Arena a is used only for error messages.                                 */
/*                                                                           */
/*  Implementation note.  Because this function is called only by            */
/*  segmented reads, there are no instances and no solution groups           */
/*  under archive_elt when it is called.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheArchiveEltCheckArchive(KML_ELT archive_elt,
  KHE_ARCHIVE archive, KML_ERROR *ke, HA_ARENA a)
{
  /* char *id; */  KHE_MODEL model;

  /* fail if archive_elt has problems */
  if( strcmp(KmlLabel(archive_elt), "HighSchoolTimetableArchive") == 0 )
    model = KHE_MODEL_HIGH_SCHOOL_TIMETABLE;
  else if( strcmp(KmlLabel(archive_elt), "EmployeeScheduleArchive") == 0 )
    model = KHE_MODEL_EMPLOYEE_SCHEDULE;
  else
  {
    *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
      "when loading, file does not begin with <HighSchoolTimetableArchive>"
      " or <EmployeeScheduleArchive>");
    return false;
  }
  /* NB Instances opening tag will have been read by this time */
  if( !KmlCheck(archive_elt, "+Id : +MetaData +Instances +SolutionGroups", ke) )
    return false;

  /* check id */
  /* *** I've decided not to check the Id
  if( KmlAttributeCount(archive_elt) == 0 )
  {
    ** no Id present in file, archive must not have one either **
    if( KheArchiveId(archive) != NULL )
    {
      *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
	"when loading, file has no archive Id but archive has Id \"%s\"",
	KheArchiveId(archive));
      return false;
    }
  }
  else
  {
    ** Id present in file, archive must have the same one **
    id = KmlAttributeValue(archive_elt, 0);
    if( KheArchiveId(archive) == NULL )
    {
      *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
	"when loading, file has archive Id \"%s\" but archive has none", id);
      return false;
    }
    else if( strcmp(id, KheArchiveId(archive)) != 0 )
    {
      *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
	"when loading, file has archive Id \"%s\" but archive has Id \"%s\"",
	id, KheArchiveId(archive));
      return false;
    }
  }
  *** */

  /* check model */
  if( model != KheArchiveModel(archive) )
  {
    if( model == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
      *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
	"when loading, file begins with <HighSchoolTimetableArchive>"
	" but archive begins with <EmployeeScheduleArchive>");
    else
      *ke = KmlErrorMake(a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
	"when loading, file begins with <EmployeeScheduleArchive>"
	" but archive begins with <HighSchoolTimetableArchive>");
    return false;
  }

  /* all good */
  return *ke = NULL, true;
}


/* *** old version that includes instances and solution groups
static bool KheArchiveEltToArchive(KML_ELT archive_elt, KHE_ARCHIVE *archive,
  bool audit_and_fix, bool infer_resource_partitions, ** bool with_monitors, **
  bool allow_invalid_solns, KML_ERROR *ke, HA_ARENA a)
{
  char *id;  KML_ELT metadata_elt, instances_elt, instance_elt;
  KML_ELT soln_groups_elt, soln_group_elt;  KHE_ARCHIVE res;  int i;
  KHE_INSTANCE ins;  KHE_MODEL model;

  ** fail if archive_elt has problems **
  *archive = NULL;
  if( strcmp(KmlLabel(archive_elt), "HighSchoolTimetableArchive") == 0 )
    model = KHE_MODEL_HIGH_SCHOOL_TIMETABLE;
  else if( strcmp(KmlLabel(archive_elt), "EmployeeScheduleArchive") == 0 )
    model = KHE_MODEL_EMPLOYEE_SCHEDULE;
  else
    return KmlError(ke, a, KmlLineNum(archive_elt), KmlColNum(archive_elt),
      "file does not begin with <HighSchoolTimetableArchive>"
      " or <EmployeeScheduleArchive>");
  if( !KmlCheck(archive_elt, "+Id : +MetaData +Instances +SolutionGroups", ke) )
    return false;

  ** create archive with optional id and optional metadata **
  id = KmlAttributeCount(archive_elt) == 0 ? NULL :
    KmlAttributeValue(archive_elt, 0);
  res = KheArchiveMake(id, NULL, model);
  if( KmlContainsChild(archive_elt, "MetaData", &metadata_elt) &&
      !KheArchiveMetaDataMakeFromKml(metadata_elt, res, ke) )
  {
    HnAssert(*ke != NULL, "KheArchiveEltToArc hive internal error 1");
    return false;
  }

  ** build and add instances **
  if( KmlContainsChild(archive_elt, "Instances", &instances_elt) )
  {
    if( !KmlCheck(instances_elt, ": *Instance", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(instances_elt);  i++ )
    {
      instance_elt = KmlChild(instances_elt, i);
      if( !KheInstanceM akeFromKml(instance_elt, model, audit_and_fix,
	    infer_resource_partitions, &ins, ke) )
      {
	HnAssert(*ke != NULL, "KheArchiveE ltToArchive internal error 2");
	return false;
      }
      if( !KheArchiveAddInstance(res, ins) )
	return KmlError(ke, a,KmlLineNum(instance_elt), KmlColNum(instance_elt),
	  "instance Id \"%s\" used previously", KheInstanceId(ins));
    }
  }

  ** build and add solution groups **
  if( KmlContainsChild(archive_elt, "SolutionGroups", &soln_groups_elt) )
  {
    if( !KmlCheck(soln_groups_elt, ": *SolutionGroup", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(soln_groups_elt);  i++ )
    {
      soln_group_elt = KmlChild(soln_groups_elt, i);
      if( !KheSolnGroupMakeFromKml(soln_group_elt, res,
	    ** with_monitors, ** allow_invalid_solns, ke) )
      {
	HnAssert(*ke != NULL, "KheArchiveEltToA rchive internal error 3");
	return false;
      }
    }
  }
  *archive = res;
  *ke = NULL;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveRead(FILE *fp, HA_ARENA_SET as, KHE_ARCHIVE *archive,     */
/*    KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,      */
/*    bool infer_resource_partitions,                                        */
/*    bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp)      */
/*                                                                           */
/*  Read *archive from fp.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheArchiveRead(FILE *fp, HA_ARENA_SET as, KHE_ARCHIVE *archive,
  KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,
  bool infer_resource_partitions, bool allow_invalid_solns,
  KHE_SOLN_TYPE soln_type, FILE *echo_fp)
{
  return KheArchiveReadIncremental(fp, as, archive, ke, audit_and_fix,
    resource_type_partitions, infer_resource_partitions, allow_invalid_solns,
    soln_type, echo_fp, NULL, NULL, NULL, NULL, NULL, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveLoad(KHE_ARCHIVE archive, FILE *fp,                       */
/*    KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,      */
/*    bool infer_resource_partitions, bool allow_invalid_solns,              */
/*    KHE_SOLN_TYPE soln_type, FILE *echo_fp)                                */
/*                                                                           */
/*  Like KheArchiveRead except done into an existing archive.                */
/*                                                                           */
/*****************************************************************************/
static bool KheArchiveLoadIncremental(KHE_ARCHIVE archive, FILE *fp,
  KML_ERROR *ke, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,
  KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,
  KHE_SOLN_GROUP_FN soln_group_begin_fn,
  KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl);

bool KheArchiveLoad(KHE_ARCHIVE archive, FILE *fp,
  KML_ERROR *ke, bool audit_and_fix, bool resource_type_partitions,
  bool infer_resource_partitions, bool allow_invalid_solns,
  KHE_SOLN_TYPE soln_type, FILE *echo_fp)
{
  return KheArchiveLoadIncremental(archive, fp, ke, audit_and_fix,
    resource_type_partitions, infer_resource_partitions, allow_invalid_solns,
    soln_type, echo_fp, NULL, NULL, NULL, NULL, NULL, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KmlEltHasRootLabel(KML_ELT elt)                                     */
/*                                                                           */
/*  Return true if elt has the appropriate label for a root element:         */
/*  <HighSchoolTimetableArchive> or <EmployeeScheduleArchive>.               */
/*                                                                           */
/*****************************************************************************/

/* *** unusued
static bool KmlEltHasRootLabel(KML_ELT elt)
{
  return strcmp(KmlLabel(elt), "HighSchoolTimetableArchive") == 0 ||
    strcmp(KmlLabel(elt), "EmployeeScheduleArchive") == 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCheckSolnGroupContext(KML_ELT sg, KML_READ_INFO ri)              */
/*                                                                           */
/*  Check that sg, whose label is <SolutionGroup>, has occurred in the       */
/*  correct context.                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCheckSolnGroupContext(KML_ELT sg, KML_READ_INFO ri)
{
  KML_ELT sgs, hsta;  HA_ARENA a;
  sgs = KmlParent(sg);
  a = KmlReadArena(ri);
  if( sgs == NULL || strcmp(KmlLabel(sgs), "SolutionGroups") != 0 )
    KmlReadFail(ri, KmlErrorMake(a, KmlLineNum(sg), KmlColNum(sg),
      "<SolutionGroup> does not lie within <SolutionGroups>"));
  hsta = KmlParent(sgs);
  if( hsta == NULL || !KmlEltHasRootLabel(hsta) )
    KmlReadFail(ri, KmlErrorMake(a, KmlLineNum(sgs), KmlColNum(sgs),
      "<SolutionGroups> does not lie within <HighSchoolTimetableArchive>"
      " or <EmployeeScheduleArchive>"));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCheckSolnContext(KML_ELT s, KML_READ_INFO ri)                    */
/*                                                                           */
/*  Check that s, whose label is <Solution>, has occurred in the correct     */
/*  context.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCheckSolnContext(KML_ELT s, KML_READ_INFO ri)
{
  KML_ELT sg;
  sg = KmlParent(s);
  if( sg == NULL || strcmp(KmlLabel(sg), "SolutionGroup") != 0 )
    KmlReadFail(ri, KmlErrorMake(KmlReadArena(ri), KmlLineNum(s), KmlColNum(s),
      "<Solution> does not lie within <SolutionGroup>"));
  KheCheckSolnGroupContext(sg, ri);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCheckInstanceContext(KML_ELT ins, KML_READ_INFO ri)              */
/*                                                                           */
/*  Check that ins, whose label is <Instance>, has occurred in the correct   */
/*  context.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheCheckInstanceContext(KML_ELT ins, KML_READ_INFO ri)
{
  KML_ELT inss, hsta;
  inss = KmlParent(ins);
  if( inss == NULL || strcmp(KmlLabel(inss), "Instances") != 0 )
    KmlReadFail(ri, KmlErrorMake(KmlReadArena(ri), KmlLineNum(ins),
      KmlColNum(ins), "<Instance> does not lie within <Instances>"));
  hsta = KmlParent(inss);
  if( hsta == NULL || !KmlEltHasRootLabel(hsta) )
    KmlReadFail(ri, KmlErrorMake(KmlReadArena(ri), KmlLineNum(inss),
      KmlColNum(inss), "<Instances> does not lie within "
      "<HighSchoolTimetableArchive> or <EmployeeScheduleArchive> "
      "(it lies within %s)", hsta == NULL ? "nothing" : KmlLabel(hsta)));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheReadBeginArchive(KHE_READ_STATE krs, KHE_ARCHIVE archive)        */
/*                                                                           */
/*  Function to call when beginning an archive.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheReadBeginArchive(KHE_READ_STATE krs, KHE_ARCHIVE archive)
{
  HnAssert(krs->archive == NULL, "KheReadBeginArchive internal error 1");
  krs->archive = archive;
  if( krs->archive_begin_fn != NULL )
    krs->archive_begin_fn(krs->archive, krs->impl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheReadEndArchive(KHE_READ_STATE krs)                               */
/*                                                                           */
/*  Function to call when ending an archive.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheReadEndArchive(KHE_READ_STATE krs)
{
  HnAssert(krs->archive != NULL, "KheReadEndArchive internal error 1");
  if( krs->archive_end_fn != NULL )
    krs->archive_end_fn(krs->archive, krs->impl);
  krs->archive = NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheReadInArchive(KHE_READ_STATE krs)                                */
/*                                                                           */
/*  Return true if krs is currently within an archive.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheReadInArchive(KHE_READ_STATE krs)
{
  return krs->archive != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheReadBeginSolnGroup(KHE_READ_STATE krs, KHE_SOLN_GROUP soln_group)*/
/*                                                                           */
/*  Function to call when beginning a soln group.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheReadBeginSolnGroup(KHE_READ_STATE krs, KHE_SOLN_GROUP soln_group)
{
  HnAssert(krs->archive != NULL, "KheReadBeginSolnGroup internal error 1");
  HnAssert(krs->soln_group == NULL, "KheReadBeginSolnGroup internal error 2");
  krs->soln_group = soln_group;
  if( krs->soln_group_begin_fn != NULL )
    krs->soln_group_begin_fn(krs->soln_group, krs->impl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheReadEndSolnGroup(KHE_READ_STATE krs, KHE_SOLN_GROUP soln_group)  */
/*                                                                           */
/*  Function to call when ending a soln group.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheReadEndSolnGroup(KHE_READ_STATE krs, KHE_SOLN_GROUP soln_group)
{
  HnAssert(krs->archive != NULL, "KheReadEndSolnGroup internal error 1");
  HnAssert(krs->soln_group != NULL, "KheReadEndSolnGroup internal error 2");
  HnAssert(krs->soln_group==soln_group,"KheReadEndSolnGroup internal error 3");
  if( krs->soln_group_end_fn != NULL )
    krs->soln_group_end_fn(krs->soln_group, krs->impl);
  krs->soln_group = NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheReadInSolnGroup(KHE_READ_STATE krs)                              */
/*                                                                           */
/*  Return true if krs is currently within a solution group.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheReadInSolnGroup(KHE_READ_STATE krs)
{
  return krs->soln_group != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheReadEltFn(KML_ELT elt, KML_READ_INFO ri)                         */
/*                                                                           */
/*  Element function called during incremental reading.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheReadEltFn(KML_ELT elt, KML_READ_INFO ri)
{
  KML_ERROR ke;  int count;  KHE_SOLN soln;  KHE_INSTANCE ins;
  KHE_ARCHIVE archive;  KHE_SOLN_GROUP soln_group;
  KHE_READ_STATE krs = (KHE_READ_STATE) KmlReadImpl(ri);
  if( DEBUG2 )
    fprintf(stderr, "[ KheReadEltFn(%s, %d)\n",
      KmlLabel(elt), KmlReadCurrDepth(ri));

  ** after completing the read of an instance **
  ke = NULL;
  if( strcmp(KmlLabel(elt), "Instance") == 0 )
  {
    KheCheckInstanceContext(elt, ri);
    if( !KheReadInArchive(krs) )
    {
      ** this must be the very first instance **
      if( !KheArchiveEltToArch ive(KmlParent(KmlParent(elt)), &archive,
	    krs->audit_and_fix, krs->infer_resource_partitions,
	    ** krs->with_monitors, ** krs->allow_invalid_solns, &ke,
	    KmlReadArena(ri)) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 1");
	KmlReadFail(ri, ke);
      }
      KheReadBeginArchive(krs, archive);
      count = KheArchiveSolnGroupCount(krs->archive);
      HnAssert(count == 0, "KheReadEltFn internal error 2");
      count = KheArchiveInstanceCount(krs->archive);
      HnAssert(count == 1, "KheReadEltFn internal error 3");
      if( DEBUG2 )
      {
	ins = KheArchiveInstance(krs->archive, 0);
	fprintf(stderr, "  adding initial instance %s to archive\n",
	  KheInstanceId(ins));
      }
    }
    else
    {
      ** this must be at least the second instance in this archive **
      if( !KheInstanceMak eFromKml(elt, KheArchiveModel(krs->archive),
	    krs->audit_and_fix, krs->infer_resource_partitions, &ins, &ke) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 4");
        KmlReadFail(ri, ke);
      }
      if( !KheArchiveAddInstance(krs->archive, ins) )
      {
	KmlError(&ke, KmlReadArena(ri), KmlLineNum(elt), KmlColNum(elt),
	  "instance Id \"%s\" used previously", KheInstanceId(ins));
	KmlReadFail(ri, ke);
      }
      count = KheArchiveInstanceCount(krs->archive);
      HnAssert(count >= 2, "KheReadEltFn internal error 5");
      if( DEBUG2 )
      {
	ins = KheArchiveInstance(krs->archive, count - 1);
	fprintf(stderr, "  adding instance %s to archive\n",
	  KheInstanceId(ins));
      }
    }
    KmlDeleteChild(KmlParent(elt), elt);
    KmlFree(elt);  ** stil l to do here **
  }

  ** after completing the read of a solution **
  else if( strcmp(KmlLabel(elt), "Solution") == 0 )
  {
    KheCheckSolnContext(elt, ri);
    if( !KheReadInArchive(krs) )
    {
      ** this must be the very first solution **
      if( !KheArchiveEltToAr chive(KmlParent(KmlParent(KmlParent(elt))),
	    &archive, krs->audit_and_fix, krs->infer_resource_partitions,
	    ** krs->with_monitors, ** krs->allow_invalid_solns, &ke,
	    KmlReadArena(ri)) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 6");
	KmlReadFail(ri, ke);
      }
      KheReadBeginArchive(krs, archive);
      count = KheArchiveSolnGroupCount(krs->archive);
      HnAssert(count == 1, "KheReadEltFn internal error 7");
      soln_group = KheArchiveSolnGroup(krs->archive, 0);
      HnAssert(KheSolnGroupSolnCount(soln_group) == 1,
	"KheReadEltFn internal error 2");
      KheReadBeginSolnGroup(krs, soln_group);
      if( DEBUG2 )
      {
	soln = KheSolnGroupSoln(krs->soln_group, 0);
	fprintf(stderr, "  adding initial solution (to %s) to soln group %s\n",
	  KheInstanceId(KheSolnInstance(soln)),KheSolnGroupId(krs->soln_group));
      }
    }
    else if( !KheReadInSolnGroup(krs) )
    {
      ** this must be the first solution in this solution group **
      if( !KheSolnGroupMakeFromKml(KmlParent(elt), krs->archive,
	    ** krs->with_monitors, ** krs->allow_invalid_solns, &ke) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 8");
	KmlReadFail(ri, ke);
      }
      count = KheArchiveSolnGroupCount(krs->archive);
      HnAssert(count >= 1, "KheReadEltFn internal error 9");
      soln_group = KheArchiveSolnGroup(krs->archive, count - 1);
      HnAssert(KheSolnGroupSolnCount(soln_group) == 1,
	"KheReadEltFn internal error 10");
      KheReadBeginSolnGroup(krs, soln_group);
      if( DEBUG2 )
      {
	soln = KheSolnGroupSoln(krs->soln_group, 0);
	fprintf(stderr, "  adding first solution (to %s) to soln group %s\n",
	  KheInstanceId(KheSolnInstance(soln)),KheSolnGroupId(krs->soln_group));
      }
    }
    else
    {
      ** this must be at least the second solution in this solution group **
      if( !KheSolnMakeFromKml(elt, krs->archive, ** krs->with_monitors, **
	    krs->allow_invalid_solns, &soln, &ke) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 11");
        KmlReadFail(ri, ke);
      }
      KheSolnGroupAddSoln(krs->soln_group, soln);
      if( DEBUG2 )
	fprintf(stderr, "  adding solution to soln group %s\n",
          KheSolnGroupId(krs->soln_group));
    }
    KmlDeleteChild(KmlParent(elt), elt);
    KmlFree(elt);
    if( krs->soln_fn != NULL )
    {
      count = KheSolnGroupSolnCount(krs->soln_group);
      HnAssert(count >= 1, "KheReadEltFn internal error 11a");
      soln = KheSolnGroupSoln(krs->soln_group, count - 1);
      if( DEBUG2 && krs->soln_fn != NULL )
	fprintf(stderr, "  calling soln_fn on soln to %s\n",
	  KheInstanceId(KheSolnInstance(soln)));
      krs->soln_fn(soln, krs->impl);
    }
  }

  ** after completing the read of a solution group **
  else if( strcmp(KmlLabel(elt), "SolutionGroup") == 0 )
  {
    KheCheckSolnGroupContext(elt, ri);
    if( !KheReadInArchive(krs) )
    {
      ** this must be the very first solution group, and it must contain **
      ** no solutions, which is unlikely but legal **
      if( !KheArchiveEltToA rchive(KmlParent(KmlParent(elt)), &archive,
	    krs->audit_and_fix, krs->infer_resource_partitions,
	    ** krs->with_monitors, ** krs->allow_invalid_solns, &ke,
	    KmlReadArena(ri)) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 12");
	KmlReadFail(ri, ke);
      }
      KheReadBeginArchive(krs, archive);
      count = KheArchiveSolnGroupCount(krs->archive);
      HnAssert(count == 1, "KheReadEltFn internal error 13");
      soln_group = KheArchiveSolnGroup(krs->archive, 0);
      KheReadBeginSolnGroup(krs, soln_group);
      KheReadEndSolnGroup(krs, soln_group);
      if( DEBUG2 )
	fprintf(stderr, "  adding empty first solution group to archive\n");
    }
    else if( !KheReadInSolnGroup(krs) )
    {
      ** this must be a solution group after the first, and it must **
      ** contain no solutions **
      if( !KheSolnGroupMakeFromKml(elt, krs->archive,
	    ** krs->with_monitors, ** krs->allow_invalid_solns, &ke) )
      {
	HnAssert(ke != NULL, "KheReadEltFn internal error 14");
	KmlReadFail(ri, ke);
      }
      count = KheArchiveSolnGroupCount(krs->archive);
      soln_group = KheArchiveSolnGroup(krs->archive, count - 1);
      KheReadBeginSolnGroup(krs, soln_group);
      KheReadEndSolnGroup(krs, soln_group);
      if( DEBUG2 )
	fprintf(stderr, "  adding empty solution group to archive\n");
    }
    else
    {
      ** must have seen at least one solution, which will have done **
      ** all the real work.  So just end this solution group now    **
      if( DEBUG2 )
	fprintf(stderr, "  ending soln group %s\n",
          KheSolnGroupId(krs->soln_group));
      KheReadEndSolnGroup(krs, krs->soln_group);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheReadEltFn returning\n");
}
*** */

static void KheArchiveSegmentBeginFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;  KML_ERROR ke;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  if( krs->archive != NULL )
  {
    /* loading into an existing archive */
    if( !KheArchiveEltCheckArchive(KmlSegmentRoot(ks), krs->archive,
      &ke, KmlReaderArena(kr)) )
    {
      /* KmlSegmentRoot(ks) not compatible with existing archive */
      HnAssert(ke != NULL, "KheArchiveSegmentBeginFn internal error 2");
      KmlReaderFail(kr, ke);
    }
  }
  else
  {
    /* creating a new archive */
    if( !KheArchiveEltToArchive(KmlSegmentRoot(ks), krs->arena_set,
	&krs->archive, &ke, KmlReaderArena(kr)) )
    {
      /* can't convert KmlSegmentRoot(ks) into an archive */
      HnAssert(ke != NULL, "KheArchiveSegmentBeginFn internal error 2");
      KmlReaderFail(kr, ke);
    }
  }
  if( krs->archive_begin_fn != NULL )
    krs->archive_begin_fn(krs->archive, krs->impl);
}

static void KheArchiveSegmentEndFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  if( krs->archive_end_fn != NULL )
    krs->archive_end_fn(krs->archive, krs->impl);
}

static void KheInstanceSegmentFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;  KML_ERROR ke;  KHE_INSTANCE ins;
  KML_ELT ins_elt;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  ins_elt = KmlSegmentRoot(ks);
  if( !KheInstanceMakeFromKml(ins_elt, KheArchiveModel(krs->archive),
	krs->arena_set, krs->audit_and_fix, krs->resource_type_partitions,
	krs->infer_resource_partitions, &ins, &ke) )
  {
    HnAssert(ke != NULL, "KheArchiveEltToArchive internal error 2");
    KmlReaderFail(kr, ke);
  }
  if( !KheArchiveAddInstance(krs->archive, ins) )
  {
    ke = KmlErrorMake(KmlReaderArena(kr), KmlLineNum(ins_elt),
      KmlColNum(ins_elt), "instance Id \"%s\" used previously",
      KheInstanceId(ins));
    KmlReaderFail(kr, ke);
  }
}

static void KheSolnGroupSegmentBeginFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;  KML_ERROR ke;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  if( !KheSolnGroupMakeFromKml(KmlSegmentRoot(ks), krs->archive,
	krs->allow_invalid_solns, krs->soln_type, krs->arena_set,
	&krs->soln_group, &ke, KmlReaderArena(kr)) )
  {
    HnAssert(ke != NULL, "KheSolnGroupSegmentBeginFn internal error 1");
    KmlReaderFail(kr, ke);
  }
  if( krs->soln_group_begin_fn != NULL )
    krs->soln_group_begin_fn(krs->soln_group, krs->impl);
}

static void KheSolnGroupSegmentEndFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  HnAssert(krs->soln_group != NULL, "KheSolnGroupSegmentEndFn internal error");
  if( krs->soln_group_end_fn != NULL )
    krs->soln_group_end_fn(krs->soln_group, krs->impl);
  krs->soln_group = NULL;
}

static void KheSolnSegmentFn(KML_SEGMENT ks)
{
  KML_READER kr;  KHE_READ_STATE krs;  KML_ERROR ke;  KHE_SOLN soln;
  KML_ELT soln_elt;
  kr = KmlSegmentReader(ks);
  krs = (KHE_READ_STATE) KmlReaderImpl(kr);
  HnAssert(krs->soln_group != NULL, "KheSolnSegmentFn internal error 1");
  soln_elt = KmlSegmentRoot(ks);
  if( !KheSolnMakeFromKml(soln_elt, krs->archive, krs->allow_invalid_solns,
      krs->soln_type, krs->arena_set, &soln, &ke, KmlReaderArena(kr)) )
  {
    HnAssert(ke != NULL, "KheSolnSegmentFn internal error 2");
    KmlReaderFail(kr, ke);
  }
  KheSolnGroupAddSoln(krs->soln_group, soln);
  if( krs->soln_fn != NULL )
    krs->soln_fn(soln, krs->impl);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveReadIncremental(FILE *fp, HA_ARENA_SET as,                */
/*    KHE_ARCHIVE *archive, KML_ERROR *ke, bool audit_and_fix,               */
/*    bool resource_type_partitions, bool infer_resource_partitions,         */
/*    bool allow_invalid_solns, FILE *echo_fp,                               */
/*    KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,        */
/*    KHE_SOLN_GROUP_FN soln_group_begin_fn,                                 */
/*    KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl)  */
/*                                                                           */
/*  Like KheArchiveRead but also calls the callback functions as needed.     */
/*                                                                           */
/*  Implementation note.  A callback to KheReadEltFn above occurs as each    */
/*  category of depth at most 3 is completed.  Here is the grammar:          */
/*                                                                           */
/*    HighSchoolTimetableArchive +Id                                         */
/*       +MetaData                                                           */
/*       +Instances                                                          */
/*          *Instance                                                        */
/*       +SolutionGroups                                                     */
/*          *SolutionGroup Id                                                */
/*             MetaData                                                      */
/*             *Solution                                                     */
/*                                                                           */
/*  HighSchoolTimetableArchive may be replaced by EmployeeScheduleArchive.   */
/*                                                                           */
/*****************************************************************************/
#define archive_str "HighSchoolTimetableArchive:EmployeeScheduleArchive"
#define instance_str "Instances/Instance"
#define soln_group_str "SolutionGroups/SolutionGroup"
#define soln_str "Solution"

bool KheArchiveReadIncremental(FILE *fp, HA_ARENA_SET as,
  KHE_ARCHIVE *archive, KML_ERROR *ke, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,
  KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,
  KHE_SOLN_GROUP_FN soln_group_begin_fn,
  KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl)
{
  struct khe_read_state_rec krs_rec;  bool res;  KML_READER kr;  HA_ARENA a;
  if( DEBUG3 )
    fprintf(stderr, "[ KheArchiveReadIncremental\n");

  /* initialize incremental read data */
  krs_rec.arena_set = as;
  krs_rec.archive = NULL;
  krs_rec.soln_group = NULL;
  krs_rec.audit_and_fix = audit_and_fix;
  krs_rec.resource_type_partitions = resource_type_partitions;
  krs_rec.infer_resource_partitions = infer_resource_partitions;
  krs_rec.allow_invalid_solns = allow_invalid_solns;
  krs_rec.soln_type = soln_type;
  krs_rec.archive_begin_fn = archive_begin_fn;
  krs_rec.archive_end_fn = archive_end_fn;
  krs_rec.soln_group_begin_fn = soln_group_begin_fn;
  krs_rec.soln_group_end_fn = soln_group_end_fn;
  krs_rec.soln_fn = soln_fn;
  krs_rec.impl = impl;

  /* set up for reading incrementally */
  a = HaArenaMake(as);
  kr = KmlReaderMake((void *) &krs_rec, as, a);
  KmlReaderDeclareSegmentBegin(kr, archive_str, &KheArchiveSegmentBeginFn);
  KmlReaderDeclareSegment(kr, instance_str, &KheInstanceSegmentFn);
  KmlReaderDeclareSegmentBegin(kr, soln_group_str, &KheSolnGroupSegmentBeginFn);
  KmlReaderDeclareSegment(kr, soln_str, &KheSolnSegmentFn);
  KmlReaderDeclareSegmentEnd(kr, &KheSolnGroupSegmentEndFn);
  KmlReaderDeclareSegmentEnd(kr, &KheArchiveSegmentEndFn);

  /* read fp incrementally, calling back after each high-level thing */
  *ke = NULL;
  if( KmlReaderReadFileSegmented(kr, fp, echo_fp, ke) )
  {
    /* the incremental read worked */
    *archive = krs_rec.archive;
    HnAssert(*ke == NULL, "KheArchiveReadIncremental internal error 1");
    res = true;
  }
  else
  {
    /* the incremental read did not work */
    *archive = NULL;
    HnAssert(*ke != NULL, "KheArchiveReadIncremental internal error 2");
    res = false;
  }
  if( DEBUG3 )
    fprintf(stderr, "KheArchiveReadIncremental returning %s\n",
      res ? "true" : "false");
  if( DEBUG3 )
    fprintf(stderr, "] KheArchiveReadIncremental returning\n");
  HaArenaDelete(a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static bool KheArchiveLoadIncremental(KHE_ARCHIVE archive, FILE *fp,
  KML_ERROR *ke, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp,
  KHE_ARCHIVE_FN archive_begin_fn, KHE_ARCHIVE_FN archive_end_fn,
  KHE_SOLN_GROUP_FN soln_group_begin_fn,
  KHE_SOLN_GROUP_FN soln_group_end_fn, KHE_SOLN_FN soln_fn, void *impl)
{
  struct khe_read_state_rec krs_rec;  bool res;  KML_READER kr;  HA_ARENA a;
  if( DEBUG3 )
    fprintf(stderr, "[ KheArchiveLoadIncremental\n");

  /* initialize incremental read data */
  krs_rec.arena_set = archive->arena_set;
  krs_rec.archive = archive;
  krs_rec.soln_group = NULL;
  krs_rec.audit_and_fix = audit_and_fix;
  krs_rec.resource_type_partitions = resource_type_partitions;
  krs_rec.infer_resource_partitions = infer_resource_partitions;
  krs_rec.allow_invalid_solns = allow_invalid_solns;
  krs_rec.soln_type = soln_type;
  krs_rec.archive_begin_fn = archive_begin_fn;
  krs_rec.archive_end_fn = archive_end_fn;
  krs_rec.soln_group_begin_fn = soln_group_begin_fn;
  krs_rec.soln_group_end_fn = soln_group_end_fn;
  krs_rec.soln_fn = soln_fn;
  krs_rec.impl = impl;

  /* set up for reading incrementally */
  a = HaArenaMake(archive->arena_set);
  kr = KmlReaderMake((void *) &krs_rec, archive->arena_set, a);
  KmlReaderDeclareSegmentBegin(kr, archive_str, &KheArchiveSegmentBeginFn);
  KmlReaderDeclareSegment(kr, instance_str, &KheInstanceSegmentFn);
  KmlReaderDeclareSegmentBegin(kr, soln_group_str, &KheSolnGroupSegmentBeginFn);
  KmlReaderDeclareSegment(kr, soln_str, &KheSolnSegmentFn);
  KmlReaderDeclareSegmentEnd(kr, &KheSolnGroupSegmentEndFn);
  KmlReaderDeclareSegmentEnd(kr, &KheArchiveSegmentEndFn);

  /* read fp incrementally, calling back after each high-level thing */
  *ke = NULL;
  if( KmlReaderReadFileSegmented(kr, fp, echo_fp, ke) )
  {
    /* the incremental read worked */
    HnAssert(*ke == NULL, "KheArchiveLoadIncremental internal error 1");
    res = true;
  }
  else
  {
    /* the incremental read did not work */
    HnAssert(*ke != NULL, "KheArchiveLoadIncremental internal error 2");
    res = false;
  }
  if( DEBUG3 )
    fprintf(stderr, "KheArchiveLoadIncremental returning %s\n",
      res ? "true" : "false");
  if( DEBUG3 )
    fprintf(stderr, "] KheArchiveLoadIncremental returning\n");
  HaArenaDelete(a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading via the command line"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void ArchiveExcludeInstances(KHE_ARCHIVE archive, char *ids)             */
/*                                                                           */
/*  Exclude instances with the given names from archive.                     */
/*                                                                           */
/*****************************************************************************/

static void ArchiveExcludeInstances(KHE_ARCHIVE archive, char *ids)
{
  char *id;  KHE_INSTANCE ins;  int junk;
  for( id = strtok(ids, ",");  id != NULL;  id = strtok(NULL, ",") )
    if( KheArchiveRetrieveInstance(archive, id, &ins, &junk) )
      KheArchiveDeleteInstance(archive, ins);
    else
      HnFatal("khe: no instance \"%s\" in archive", id);
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveIncludeInstances(KHE_ARCHIVE archive, char *ids)             */
/*                                                                           */
/*  Include only instances with the given names from archive.                */
/*                                                                           */
/*****************************************************************************/

static void ArchiveIncludeInstances(KHE_ARCHIVE archive, char *ids)
{
  KHE_INSTANCE ins;  ARRAY_KHE_INSTANCE included_instances;  char *id;
  int i, junk;

  /* make sure all the named instances are present; build included_instances */
  HaArrayInit(included_instances, archive->arena);
  for( id = strtok(ids, ",");  id != NULL;  id = strtok(NULL, ",") )
    if( KheArchiveRetrieveInstance(archive, id, &ins, &junk) )
      HaArrayAddLast(included_instances, ins);
    else
      HnFatal("khe: no instance \"%s\" in archive", id);

  /* delete all instances not in included_instances */
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    ins = KheArchiveInstance(archive, i);
    if( !HaArrayContains(included_instances, ins, &junk) )
    {
      KheArchiveDeleteInstance(archive, ins);
      i--;
    }
  }
  HaArrayFree(included_instances);
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveIncludeFirstInstances(KHE_ARCHIVE archive, char *count_str)  */
/*                                                                           */
/*  Include only the first count_str instances.                              */
/*                                                                           */
/*****************************************************************************/

static void ArchiveIncludeFirstInstances(KHE_ARCHIVE archive, char *count_str)
{
  KHE_INSTANCE ins;  int count;

  /* make sure all the named instances are present; build included_instances */
  if( sscanf(count_str, "%d", &count) != 1 || count < 0 )
    HnFatal("khe: invalid -i value \"%s\"", count_str);

  /* delete all instances beyond count */
  while( KheArchiveInstanceCount(archive) > count )
  {
    ins = KheArchiveInstance(archive, count);
    KheArchiveDeleteInstance(archive, ins);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveExcludeSolutionGroups(KHE_ARCHIVE archive, char *ids)        */
/*                                                                           */
/*  Exclude solution groups with the given names from archive.               */
/*                                                                           */
/*****************************************************************************/

static void ArchiveExcludeSolutionGroups(KHE_ARCHIVE archive, char *ids)
{
  char *id;  KHE_SOLN_GROUP soln_group;
  if( ids[0] == '\0' )
  {
    /* special case, means delete all solution groups */
    while( KheArchiveSolnGroupCount(archive) > 0 )
      KheSolnGroupDelete(KheArchiveSolnGroup(archive, 0), true);
  }
  else
  {
    /* ordinary case, delete each solution group named by the option */
    for( id = strtok(ids, ",");  id != NULL;  id = strtok(NULL, ",") )
      if( KheArchiveRetrieveSolnGroup(archive, id, &soln_group) )
	KheSolnGroupDelete(soln_group, true);
      else
	HnFatal("khe: no solution group \"%s\" in archive", id);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void ArchiveIncludeSolutionGroups(KHE_ARCHIVE archive, char *ids)        */
/*                                                                           */
/*  Include only solution groups with the given names from archive.          */
/*                                                                           */
/*****************************************************************************/

static void ArchiveIncludeSolutionGroups(KHE_ARCHIVE archive, char *ids)
{
  KHE_SOLN_GROUP soln_group;  ARRAY_KHE_SOLN_GROUP included_soln_groups;
  char *id;  int i, junk;

  /* ensure all the named soln groups are present; build included_soln_groups */
  HaArrayInit(included_soln_groups, archive->arena);
  for( id = strtok(ids, ",");  id != NULL;  id = strtok(NULL, ",") )
    if( KheArchiveRetrieveSolnGroup(archive, id, &soln_group) )
      HaArrayAddLast(included_soln_groups, soln_group);
    else
      HnFatal("khe: no solution group \"%s\" in archive", id);

  /* delete all soln groups not in included_soln_groups */
  for( i = 0;  i < KheArchiveSolnGroupCount(archive);  i++ )
  {
    soln_group = KheArchiveSolnGroup(archive, i);
    if( !HaArrayContains(included_soln_groups, soln_group, &junk) )
    {
      KheSolnGroupDelete(soln_group, true);
      i--;
    }
  }
  HaArrayFree(included_soln_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE KheArchiveReadFromCommandLine(int argc, char *argv[],        */
/*    int *pos, HA_ARENA_SET as, bool audit_and_fix,                         */
/*    bool resource_type_partitions, bool infer_resource_partitions,         */
/*    bool allow_invalid_solns, FILE *echo_fp)                               */
/*                                                                           */
/*  Read an archive, returning only if successful.   The archive name is     */
/*  at argv[*pos], and following options cause it to be updated.             */
/*                                                                           */
/*****************************************************************************/

KHE_ARCHIVE KheArchiveReadFromCommandLine(int argc, char *argv[],
  int *pos, HA_ARENA_SET as, bool audit_and_fix,
  bool resource_type_partitions, bool infer_resource_partitions,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, FILE *echo_fp)
{
  FILE *fp;  KHE_ARCHIVE res;  KML_ERROR ke;
  bool seen_x, seen_i, seen_n, seen_X, seen_I;

  /* open the archive file for reading */
  if( *pos >= argc )
    HnFatal("%s: archive name missing from command line", argv[0]);
  if( strcmp(argv[*pos], "-") == 0 )
    fp = stdin;
  else
  {
    fp = fopen(argv[*pos], "r");
    if( fp == NULL )
      HnFatal("%s: cannot open file \"%s\" for reading", argv[0], argv[*pos]);
  }

  /* read the archive file */
  if( !KheArchiveRead(fp, as, &res, &ke, audit_and_fix,
        resource_type_partitions, infer_resource_partitions,
	allow_invalid_solns, soln_type, echo_fp) )
    HnFatal("%s:%d:%d: %s\n", argv[*pos], KmlErrorLineNum(ke),
      KmlErrorColNum(ke), KmlErrorString(ke));
  if( fp != stdin )
    fclose(fp);

  /* process zero or more following -x, -i, -X, and -I options */
  seen_x = seen_i = seen_n = seen_X = seen_I = false;
  for( (*pos)++;  *pos < argc && argv[*pos][0] == '-';  (*pos)++ )
    switch( argv[*pos][1] )
    {
      case 'x':

	if( seen_x )
	  HnFatal("%s: -x option used twice", argv[0]);
	if( seen_i )
	  HnFatal("%s: -x and -i used together", argv[0]);
	if( seen_n )
	  HnFatal("%s: -x and -n used together", argv[0]);
	seen_x = true;
	ArchiveExcludeInstances(res, &argv[*pos][2]);
	break;

      case 'i':

	if( seen_i )
	  HnFatal("%s: -i option used twice", argv[0]);
	if( seen_x )
	  HnFatal("%s: -i and -x used together", argv[0]);
	if( seen_n )
	  HnFatal("%s: -i and -n used together", argv[0]);
	seen_i = true;
	ArchiveIncludeInstances(res, &argv[*pos][2]);
	break;

      case 'n':

	if( seen_n )
	  HnFatal("%s: -n option used twice", argv[0]);
	if( seen_i )
	  HnFatal("%s: -i and -n used together", argv[0]);
	if( seen_x )
	  HnFatal("%s: -n and -x used together", argv[0]);
	seen_n = true;
	ArchiveIncludeFirstInstances(res, &argv[*pos][2]);
	break;

      case 'X':

	if( seen_X )
	  HnFatal("%s: -X option used twice", argv[0]);
	if( seen_I )
	  HnFatal("%s: -X and -I used together", argv[0]);
	seen_X = true;
	ArchiveExcludeSolutionGroups(res, &argv[*pos][2]);
	break;

      case 'I':

	if( seen_I )
          HnFatal("%s: -I option used twice", argv[0]);
	if( seen_X )
          HnFatal("%s: -I and -X used together", argv[0]);
	seen_I = true;
	ArchiveIncludeSolutionGroups(res, &argv[*pos][2]);
	break;

      default:

	/* stop at first unrecognized option */
	return res;
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "writing archives"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheArchiveInitialTag(KHE_ARCHIVE archive)                          */
/*                                                                           */
/*  Return the initial tag needed for a file containing archive.             */
/*                                                                           */
/*****************************************************************************/

static char *KheArchiveInitialTag(KHE_ARCHIVE archive)
{
  switch( KheArchiveModel(archive) )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE: return "HighSchoolTimetableArchive";
    case KHE_MODEL_EMPLOYEE_SCHEDULE:	  return "EmployeeScheduleArchive";

    default:

      HnAbort("KheArchiveInitialTag internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveMetaDataWrite(KHE_ARCHIVE_METADATA md, KML_FILE kf)       */
/*                                                                           */
/*  Write instance metadata to xml, if there is any to write.                */
/*                                                                           */
/*****************************************************************************/

static bool Has(char *val)
{
  return val != NULL && *val != '\0';
}

static char *Val(char *val, char *alt)
{
  return Has(val) ? val : alt;
}

void KheArchiveMetaDataWrite(KHE_ARCHIVE archive, KML_FILE kf)
{
  if( Has(archive->meta_name) || Has(archive->meta_contributor) ||
      Has(archive->meta_date) || Has(archive->meta_description) ||
      Has(archive->meta_remarks) )
  {
    KmlBegin(kf, "MetaData");
    KmlEltPlainText(kf, "Name", Val(archive->meta_name, "No name"));
    KmlEltPlainText(kf, "Contributor",
      Val(archive->meta_contributor, "No contributor"));
    KmlEltPlainText(kf, "Date", Val(archive->meta_date, "No date"));
    KmlEltPlainText(kf, "Description",
      Val(archive->meta_description, "No description"));
    if( Has(archive->meta_remarks) )
      KmlEltPlainText(kf, "Remarks", archive->meta_remarks);
    KmlEnd(kf, "MetaData");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheArchiveWriteHeaderAndInstances(KHE_ARCHIVE archive, KML_FILE kf) */
/*                                                                           */
/*  Write the header and instances of archive to kf.                         */
/*                                                                           */
/*****************************************************************************/

static void KheArchiveWriteHeaderAndInstances(KHE_ARCHIVE archive, KML_FILE kf)
{
  int i;  KHE_INDEXED_INSTANCE ii;

  /* header with optional Id, followed by optional metadata */
  KmlBegin(kf, KheArchiveInitialTag(archive));
  if( archive->id != NULL )
    KmlAttribute(kf, "Id", archive->id);
  KheArchiveMetaDataWrite(archive, kf);

  /* instances */
  if( HaArrayCount(archive->instance_array) > 0 )
  {
    KmlBegin(kf, "Instances");
    HaArrayForEach(archive->instance_array, ii, i)
      KheInstanceWrite(ii->instance, kf);
    KmlEnd(kf, "Instances");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveWrite(KHE_ARCHIVE archive, bool with_reports, FILE *fp)   */
/*                                                                           */
/*  Write archive to fp, which must be open for writing.  Include reports    */
/*  in the written archive if with_reports is true.                          */
/*                                                                           */
/*  Implementation note.  We use the archive's arena to hold the KML         */
/*  file object (added by JeffK 11/25, suggested by Ortiz).  That's OK,      */
/*  because the file object is not used after this function returns.         */
/*                                                                           */
/*****************************************************************************/

void KheArchiveWrite(KHE_ARCHIVE archive, bool with_reports, FILE *fp)
{
  KML_FILE kf;  int i;  KHE_SOLN_GROUP soln_group;

  /* header and instances */
  if( DEBUG3 )
    fprintf(stderr, "[ KheArchiveWrite\n");
  kf = KmlMakeFile(fp, 0, 2, archive->arena);
  KheArchiveWriteHeaderAndInstances(archive, kf);

  /* soln groups */
  if( HaArrayCount(archive->soln_group_array) > 0 )
  {
    KmlBegin(kf, "SolutionGroups");
    HaArrayForEach(archive->soln_group_array, soln_group, i)
      KheSolnGroupWrite(soln_group, with_reports, kf);
    KmlEnd(kf, "SolutionGroups");
  }

  /* close header and exit */
  KmlEnd(kf, KheArchiveInitialTag(archive));
  if( DEBUG3 )
    fprintf(stderr, "] KheArchiveWrite returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveWriteSolnGroup(KHE_ARCHIVE archive,                       */
/*    KHE_SOLN_GROUP soln_group, bool with_reports, FILE *fp)                */
/*                                                                           */
/*  Write archive, but only include one of its solution groups:  soln_group. */
/*                                                                           */
/*****************************************************************************/

void KheArchiveWriteSolnGroup(KHE_ARCHIVE archive,
  KHE_SOLN_GROUP soln_group, bool with_reports, FILE *fp)
{
  KML_FILE kf;

  /* header and instances */
  HnAssert(KheSolnGroupArchive(soln_group) == archive,
    "KheArchiveWriteSolnGroup:  soln_group not in archive");
  kf = KmlMakeFile(fp, 0, 2, archive->arena);
  KheArchiveWriteHeaderAndInstances(archive, kf);

  /* soln group */
  KmlBegin(kf, "SolutionGroups");
  KheSolnGroupWrite(soln_group, with_reports, kf);
  KmlEnd(kf, "SolutionGroups");

  /* close header and exit */
  KmlEnd(kf, KheArchiveInitialTag(archive));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveWriteWithoutSolnGroups(KHE_ARCHIVE archive, FILE *fp)     */
/*                                                                           */
/*  Write archive, omitting its solution groups.                             */
/*                                                                           */
/*****************************************************************************/

void KheArchiveWriteWithoutSolnGroups(KHE_ARCHIVE archive, FILE *fp)
{
  KML_FILE kf;

  /* header and instances */
  kf = KmlMakeFile(fp, 0, 2, archive->arena);
  KheArchiveWriteHeaderAndInstances(archive, kf);

  /* close header and exit */
  KmlEnd(kf, KheArchiveInitialTag(archive));
}

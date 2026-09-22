
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XESTT CONVERTER                            */
/*  COPYRIGHT (C) 2016, Jeffrey H. Kingston                                  */
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
/*  FILE:         nrc_archive.c                                              */
/*  DESCRIPTION:  An archive holding instances and solution groups           */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#include <stdarg.h>
#define MAX_ERROR_STRING 200
#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_ARCHIVE - one archive of problems and solution groups                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SOLN_GROUP) ARRAY_NRC_SOLN_GROUP;
typedef HN_TABLE(NRC_SOLN_GROUP) TABLE_NRC_SOLN_GROUP;

typedef HA_ARRAY(NRC_INSTANCE) ARRAY_NRC_INSTANCE;
typedef HN_TABLE(NRC_INSTANCE) TABLE_NRC_INSTANCE;

struct nrc_archive_rec {
  HA_ARENA			arena;			/* arena             */
  HA_ARENA_SET			arena_set;		/* arena set         */
  char				*id;			/* optional Id       */
  /* NRC_ARCHIVE_METADATA	meta_data; */		/* optional MetaData */
  char				*meta_name;		/* metadata name     */
  char				*meta_contributor;	/* " contributor     */
  char				*meta_date;		/* " date            */
  char				*meta_description;	/* " description     */
  char				*meta_remarks;		/* " remarks         */
  ARRAY_NRC_INSTANCE		instance_array;		/* instance array    */
  TABLE_NRC_INSTANCE		instance_table;		/* instance table    */
  ARRAY_NRC_SOLN_GROUP		soln_group_array;	/* solution groups   */
  TABLE_NRC_SOLN_GROUP		soln_group_table;	/* solution groups   */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *NrcVersionNumber(void)                                             */
/*                                                                           */
/*  Return the version number.                                               */
/*                                                                           */
/*****************************************************************************/

char *NrcVersionNumber(void)
{
  return KheVersionNumber();
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcVersionBanner(void)                                             */
/*                                                                           */
/*  Return the version banner.                                               */
/*                                                                           */
/*****************************************************************************/

char *NrcVersionBanner(void)
{
  return KheVersionBanner();
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_ARCHIVE NrcArchiveMake(char *id, HA_ARENA_SET as)                    */
/*                                                                           */
/*  Make an initially empty archive.                                         */
/*                                                                           */
/*****************************************************************************/

NRC_ARCHIVE NrcArchiveMake(char *id, HA_ARENA_SET as)
{
  NRC_ARCHIVE res;  HA_ARENA a;
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->arena_set = as;
  res->id = HnStringCopy(id, a);
  res->meta_name = NULL;
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_description = NULL;
  res->meta_remarks = NULL;
  /* res->meta_data = md; */
  HaArrayInit(res->instance_array, a);
  HnTableInit(res->instance_table, a);
  HaArrayInit(res->soln_group_array, a);
  HnTableInit(res->soln_group_table, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA NrcArchiveArenaBegin(NRC_ARCHIVE archive)                       */
/*                                                                           */
/*  Get a fresh or recycled arena.                                           */
/*                                                                           */
/*****************************************************************************/

HA_ARENA NrcArchiveArenaBegin(NRC_ARCHIVE archive)
{
  return HaArenaMake(archive->arena_set);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcArchiveArenaEnd(NRC_ARCHIVE archive, HA_ARENA a)                 */
/*                                                                           */
/*  Recycle arena a.                                                         */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveArenaEnd(NRC_ARCHIVE archive, HA_ARENA a)
{
  HaArenaDelete(a);
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcArchiveId(NRC_ARCHIVE archive)                                  */
/*                                                                           */
/*  Return the Id of archive, possibly NULL.                                 */
/*                                                                           */
/*****************************************************************************/

char *NrcArchiveId(NRC_ARCHIVE archive)
{
  return archive->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcArchiveSetMetaData(NRC_ARCHIVE archive, char *name,              */
/*    char *contributor, char *date, char *description, char *remarks)       */
/*                                                                           */
/*  Set the metadata fields to copies of these values.                       */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveSetMetaData(NRC_ARCHIVE archive, char *name,
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
/*  void NrcArchiveMetaData(NRC_ARCHIVE archive, char **name,                */
/*    char **contributor, char **date, char **description, char **remarks)   */
/*                                                                           */
/*  Return the metadata fields.                                              */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveMetaData(NRC_ARCHIVE archive, char **name,
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
/*  NRC_ARCHIVE_METADATA NrcArchiveMetaData(NRC_ARCHIVE archive)             */
/*                                                                           */
/*  Return the MetaData attribute of archive.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_ARCHIVE_METADATA NrcArchiveMetaData(NRC_ARCHIVE archive)
{
  return archive->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "instances"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcArchiveAddInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins)        */
/*                                                                           */
/*  Add ins to archive, assuming it is safe to do so (not duplicate Id).     */
/*                                                                           */
/*****************************************************************************/

bool NrcArchiveAddInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins)
{
  int pos;
  if( NrcInstanceId(ins) != NULL &&
      HnTableContains(archive->instance_table, NrcInstanceId(ins), pos) )
    return false;
  HnTableAdd(archive->instance_table, NrcInstanceId(ins), ins);
  NrcInstanceAddArchive(ins, archive);
  HaArrayAddLast(archive->instance_array, ins);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcArchiveDeleteInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins)     */
/*                                                                           */
/*  Delete ins from archive.  Also delete any solutions for ins.             */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveDeleteInstance(NRC_ARCHIVE archive, NRC_INSTANCE ins)
{
  int pos, i;  NRC_SOLN_GROUP sg;

  /* delete the link between archive and ins */
  HnAssert(HaArrayContains(archive->instance_array, ins, &pos),
    "NrcArchiveDeleteInstance: ins not in archive");
  HaArrayDeleteAndShift(archive->instance_array, pos);
  NrcInstanceDeleteArchive(ins, archive);
  if( NrcInstanceId(ins) != NULL )
  {
    if( !HnTableContains(archive->instance_table, NrcInstanceId(ins), pos) )
      HnAssert(false, "NrcArchiveDeleteInstance: internal error");
    HnTableDelete(archive->instance_table, pos);
  }

  /* delete solutions for ins in archive */
  HaArrayForEach(archive->soln_group_array, sg, i)
    NrcSolnGroupDeleteSolnsForInstance(sg, ins);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcArchiveInstanceCount(NRC_ARCHIVE archive)                         */
/*                                                                           */
/*  Return the number of instances in archive.                               */
/*                                                                           */
/*****************************************************************************/

int NrcArchiveInstanceCount(NRC_ARCHIVE archive)
{
  return HaArrayCount(archive->instance_array);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcArchiveInstance(NRC_ARCHIVE archive, int i)              */
/*                                                                           */
/*  Return the i'th instance of archive.                                     */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcArchiveInstance(NRC_ARCHIVE archive, int i)
{
  return HaArray(archive->instance_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcArchiveRetrieveInstance(NRC_ARCHIVE archive, char *id,           */
/*    NRC_INSTANCE *ins)                                                     */
/*                                                                           */
/*  Retrieve an instance with the given id from archive.                     */
/*                                                                           */
/*****************************************************************************/

bool NrcArchiveRetrieveInstance(NRC_ARCHIVE archive, char *id,
  NRC_INSTANCE *ins)
{
  int pos;
  return HnTableRetrieve(archive->instance_table, id, *ins, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Sumbodule "solution groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcArchiveAddSolnGroup(NRC_ARCHIVE archive,                         */
/*    NRC_SOLN_GROUP soln_group)                                             */
/*                                                                           */
/*  Add soln_group to archive, assuming it is safe (not duplicate Id).       */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveAddSolnGroup(NRC_ARCHIVE archive, NRC_SOLN_GROUP soln_group)
{
  HaArrayAddLast(archive->soln_group_array, soln_group);
  if( NrcSolnGroupId(soln_group) != NULL )
    HnTableAdd(archive->soln_group_table, NrcSolnGroupId(soln_group),
      soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcArchiveSolnGroupCount(NRC_ARCHIVE archive)                        */
/*                                                                           */
/*  Return the number of solution groups in archive.                         */
/*                                                                           */
/*****************************************************************************/

int NrcArchiveSolnGroupCount(NRC_ARCHIVE archive)
{
  return HaArrayCount(archive->soln_group_array);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP NrcArchiveSolnGroup(NRC_ARCHIVE archive, int i)           */
/*                                                                           */
/*  Return the i'th solution group of archive.                               */
/*                                                                           */
/*****************************************************************************/

NRC_SOLN_GROUP NrcArchiveSolnGroup(NRC_ARCHIVE archive, int i)
{
  return HaArray(archive->soln_group_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcArchiveRetrieveSolnGroup(NRC_ARCHIVE archive, char *id,          */
/*    NRC_SOLN_GROUP *soln_group)                                            */
/*                                                                           */
/*  Retrieve a solution group with the given id from archive.                */
/*                                                                           */
/*****************************************************************************/

bool NrcArchiveRetrieveSolnGroup(NRC_ARCHIVE archive, char *id,
  NRC_SOLN_GROUP *soln_group)
{
  int pos;
  return HnTableRetrieve(archive->soln_group_table, id, *soln_group, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "writing archives"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE NrcArchiveConvert(NRC_ARCHIVE archive)                       */
/*                                                                           */
/*  Convert archive, an NRC archive, into a KHE archive.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_ARCHIVE NrcArchiveConvert(NRC_ARCHIVE archive)
{
  KHE_ARCHIVE res;  NRC_INSTANCE ins;  int i;  NRC_SOLN_GROUP soln_group;
  if( DEBUG1 )
    fprintf(stderr, "[ NrcArchiveConvert(archive)\n");
  /* ***
  kmd = KheArchiveMetaDataMake(NrcArchiveMetaDataName(nmd),
    NrcArchiveMetaDataContributor(nmd), NrcArchiveMetaDataDate(nmd),
    NrcArchiveMetaDataDescription(nmd), NrcArchiveMetaDataRemarks(nmd));
  *** */
  res = KheArchiveMake(NrcArchiveId(archive), KHE_MODEL_EMPLOYEE_SCHEDULE,NULL);
  KheArchiveSetMetaData(res, archive->meta_name, archive->meta_contributor,
    archive->meta_date, archive->meta_description, archive->meta_remarks);
  HaArrayForEach(archive->instance_array, ins, i)
  {
    if( DEBUG1 )
      fprintf(stderr, "  calling NrcInstanceConvert(%s)\n",
	NrcInstanceId(ins));
    if( !KheArchiveAddInstance(res, NrcInstanceConvert(ins)) )
      HnAssert(false, "NrcArchiveConvert: failed to add instance");
  }
  HaArrayForEach(archive->soln_group_array, soln_group, i)
  {
    if( DEBUG1 )
      fprintf(stderr, "  calling NrcSolnGroupConvert(%s)\n",
	NrcSolnGroupId(soln_group));
    NrcSolnGroupConvert(soln_group, res);
  }
  if( DEBUG1 )
    fprintf(stderr, "] NrcArchiveConvert returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcArchiveWrite(NRC_ARCHIVE archive, bool with_reports, FILE *fp)   */
/*                                                                           */
/*  Write archive to fp.                                                     */
/*                                                                           */
/*****************************************************************************/

void NrcArchiveWrite(NRC_ARCHIVE archive, bool with_reports, FILE *fp)
{
  KheArchiveWrite(NrcArchiveConvert(archive), with_reports, fp);
}


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
/*  FILE:         nrc_soln_group.c                                           */
/*  DESCRIPTION:  One group of solutions within an archive                   */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP - one group of solutions within an archive                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SOLN) ARRAY_NRC_SOLN;

struct nrc_soln_group_rec {
  HA_ARENA			arena;			/* arena             */
  NRC_ARCHIVE			archive;		/* enclosing archive */
  char				*id;			/* Id                */
  char				*meta_contributor;	/* metadata contr.   */
  char				*meta_date;		/* " date            */
  char				*meta_description;	/* " description     */
  char				*meta_publication;	/* " publication     */
  char				*meta_remarks;		/* " remarks         */
  /* NRC_SOLN_GROUP_METADATA	meta_data; */		/* MetaData          */
  ARRAY_NRC_SOLN		solutions;		/* the solutions     */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcSolnGroupMake(NRC_ARCHIVE archive, char *id,                     */
/*    NRC_SOLN_GROUP *soln_group)                                            */
/*                                                                           */
/*  Make an initially empty solution group with these attributes.            */
/*                                                                           */
/*****************************************************************************/

bool NrcSolnGroupMake(NRC_ARCHIVE archive, char *id, NRC_SOLN_GROUP *soln_group)
{
  NRC_SOLN_GROUP res;  HA_ARENA a;

  /* make sure archive does not already contain this id  */
  if( id != NULL && archive != NULL &&
      NrcArchiveRetrieveSolnGroup(archive, id, &res) )
  {
    *soln_group = NULL;
    return false;
  }

  /* OK, so go ahead and make the instance */
  a = NrcArchiveArenaBegin(archive);
  HaMake(res, a);
  res->arena = a;
  res->archive = archive;
  res->id = HnStringCopy(id, a);
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_description = NULL;
  res->meta_publication = NULL;
  res->meta_remarks = NULL;
  /* res->meta_data = md; */
  HaArrayInit(res->solutions, a);
  if( archive != NULL )
    NrcArchiveAddSolnGroup(archive, res);
  *soln_group = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_ARCHIVE NrcSolnGroupArchive(NRC_SOLN_GROUP soln_group)               */
/*                                                                           */
/*  Return the archive containing soln_group.                                */
/*                                                                           */
/*****************************************************************************/

NRC_ARCHIVE NrcSolnGroupArchive(NRC_SOLN_GROUP soln_group)
{
  return soln_group->archive;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcSolnGroupId(NRC_SOLN_GROUP soln_group)                          */
/*                                                                           */
/*  Return the Id attribute of soln_group.                                   */
/*                                                                           */
/*****************************************************************************/

char *NrcSolnGroupId(NRC_SOLN_GROUP soln_group)
{
  return soln_group->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupSetMetaData(NRC_SOLN_GROUP soln_group,                  */
/*    char *contributor, char *date, char *description, char *publication,   */
/*    char *remarks)                                                         */
/*                                                                           */
/*  Set the metadata fields of soln_group.                                   */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupSetMetaData(NRC_SOLN_GROUP soln_group,
  char *contributor, char *date, char *description, char *publication,
  char *remarks)
{
  soln_group->meta_contributor = HnStringCopy(contributor, soln_group->arena);
  soln_group->meta_date = HnStringCopy(date, soln_group->arena);
  soln_group->meta_description = HnStringCopy(description, soln_group->arena);
  soln_group->meta_publication = HnStringCopy(publication, soln_group->arena);
  soln_group->meta_remarks = HnStringCopy(remarks, soln_group->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group,                     */
/*    char **contributor, char **date, char **description,                   */
/*    char **publication, char **remarks)                                    */
/*                                                                           */
/*  Return the metadata fields of soln_group.                                */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group,
  char **contributor, char **date, char **description, char **publication,
  char **remarks)
{
  *contributor = soln_group->meta_contributor;
  *date = soln_group->meta_date;
  *description = soln_group->meta_description;
  *publication = soln_group->meta_publication;
  *remarks = soln_group->meta_remarks;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP_METADATA NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group)  */
/*                                                                           */
/*  Return the MetaData attribute of soln_group.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_SOLN_GROUP_METADATA NrcSolnGroupMetaData(NRC_SOLN_GROUP soln_group)
{
  return soln_group->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solutions"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupAddSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln)       */
/*                                                                           */
/*  Add soln to soln_group.                                                  */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupAddSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln)
{
  NRC_ARCHIVE archive;  NRC_INSTANCE ins;  int i;

  /* ensure soln's instance lies in soln_group's archive */
  archive = NrcSolnGroupArchive(soln_group);
  for( i = 0;  i < NrcArchiveInstanceCount(archive);  i++ )
  {
    ins = NrcArchiveInstance(archive, i);
    if( ins == NrcSolnInstance(soln) )
      break;
  }
  HnAssert(i < NrcArchiveInstanceCount(archive),
    "NrcSolnGroupAddSoln: soln's instance is not in soln_group's archive");

  /* do the adding */
  HaArrayAddLast(soln_group->solutions, soln);
  NrcSolnAddSolnGroup(soln, soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupDeleteSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln)    */
/*                                                                           */
/*  Delete soln from soln_group.                                             */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupDeleteSoln(NRC_SOLN_GROUP soln_group, NRC_SOLN soln)
{
  int pos;
  if( !HaArrayContains(soln_group->solutions, soln, &pos) )
    HnAssert(false, "NrcSolnGroupDeleteSoln: soln not present");
  HaArrayDeleteAndShift(soln_group->solutions, pos);
  NrcSolnDeleteSolnGroup(soln, soln_group);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcSolnGroupSolnCount(NRC_SOLN_GROUP soln_group)                     */
/*                                                                           */
/*  Return the number of solutions in soln_group.                            */
/*                                                                           */
/*****************************************************************************/

int NrcSolnGroupSolnCount(NRC_SOLN_GROUP soln_group)
{
  return HaArrayCount(soln_group->solutions);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN NrcSolnGroupSoln(NRC_SOLN_GROUP soln_group, int i)              */
/*                                                                           */
/*  Return the i'th solution of soln_group.                                  */
/*                                                                           */
/*****************************************************************************/

NRC_SOLN NrcSolnGroupSoln(NRC_SOLN_GROUP soln_group, int i)
{
  return HaArray(soln_group->solutions, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupDeleteSolnsForInstance(NRC_SOLN_GROUP soln_group,          */
/*    NRC_INSTANCE ins)                                                      */
/*                                                                           */
/*  Delete from soln_group all solutions for ins.                            */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupDeleteSolnsForInstance(NRC_SOLN_GROUP soln_group,
  NRC_INSTANCE ins)
{
  NRC_SOLN soln;  int i;
  HaArrayForEach(soln_group->solutions, soln, i)
    if( NrcSolnInstance(soln) == ins )
    {
      NrcSolnGroupDeleteSoln(soln_group, soln);
      i--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion to KHE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcSolnGroupConvert(NRC_SOLN_GROUP soln_group, KHE_ARCHIVE archive) */
/*                                                                           */
/*  Convert soln_group to KHE and add the result to archive.                 */
/*                                                                           */
/*****************************************************************************/

void NrcSolnGroupConvert(NRC_SOLN_GROUP soln_group, KHE_ARCHIVE archive)
{
  KHE_SOLN_GROUP ksg;  NRC_SOLN soln;  int i;

  if( DEBUG1 )
    fprintf(stderr, "[ NrcSolnGroupConvert(%s, archive)\n",
      NrcSolnGroupId(soln_group));
  if( !KheSolnGroupMake(archive, soln_group->id, &ksg) )
    HnAssert(false, "NrcSolnGroupConvert internal error: KheSolnGroupMake");
  KheSolnGroupSetMetaData(ksg, soln_group->meta_contributor,
      soln_group->meta_date, soln_group->meta_description,
      soln_group->meta_publication, soln_group->meta_remarks);
  HaArrayForEach(soln_group->solutions, soln, i)
  {
    if( DEBUG1 )
      fprintf(stderr, "  converting %s solution %d (to %s):\n",
	NrcSolnGroupId(soln_group), i, NrcInstanceId(NrcSolnInstance(soln)));
    KheSolnGroupAddSoln(ksg, NrcSolnConvert(soln));
  }
  if( DEBUG1 )
    fprintf(stderr, "] NrcSolnGroupConvert\n");
}

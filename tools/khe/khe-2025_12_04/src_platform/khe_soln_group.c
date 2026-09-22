
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
/*  FILE:         khe_soln_group.c                                           */
/*  DESCRIPTION:  One group of solutions within an archive                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP - one group of solutions within an archive                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_SOLN_SET) ARRAY_KHE_SOLN_SET;
/* typedef HA_ARRAY(KHE_SOLN_WRITE_ONLY) ARRAY_KHE_SOLN_WRITE_ONLY; */

struct khe_soln_group_rec {
  HA_ARENA			arena;			/* arena for sg only */
  void				*back;			/* back pointer      */
  KHE_ARCHIVE			archive;		/* enclosing archive */
  char				*id;			/* Id                */
  char				*meta_contributor;	/* metadata contr.   */
  char				*meta_date;		/* " date            */
  char				*meta_description;	/* " description     */
  char				*meta_publication;	/* " publication     */
  char				*meta_remarks;		/* " remarks         */
  /* KHE_SOLN_GROUP_METADATA	meta_data; */		/* MetaData          */
  ARRAY_KHE_SOLN		solutions;		/* the solutions     */
  ARRAY_KHE_SOLN_SET		soln_sets;		/* one per instance  */
  /* ARRAY_KHE_SOLN_WRITE_ONLY	write_only_solns; */	/* write-only solns  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnGroupMake(KHE_ARCHIVE archive, char *id,                     */
/*     KHE_SOLN_GROUP *soln_group)                                           */
/*                                                                           */
/*  Make an initially empty solution group with these attributes.            */
/*                                                                           */
/*****************************************************************************/

bool KheSolnGroupMake(KHE_ARCHIVE archive, char *id,
  KHE_SOLN_GROUP *soln_group)
{
  KHE_SOLN_GROUP res;  HA_ARENA a;

  /* make sure archive does not already contain this id  */
  HnAssert(archive != NULL, "KheSolnGroupMake:  archive == NULL");
  if( id != NULL && KheArchiveRetrieveSolnGroup(archive, id, &res) )
  {
    *soln_group = NULL;
    return false;
  }

  /* OK, so go ahead and make the soln group, in its own arena */
  a = KheArchiveArenaBegin(archive);
  HaMake(res, a);
  res->arena = a;
  res->back = NULL;
  res->archive = archive;
  res->id = HnStringCopy(id, a);
  res->meta_contributor = NULL;
  res->meta_date = NULL;
  res->meta_description = NULL;
  res->meta_publication = NULL;
  res->meta_remarks = NULL;
  /* res->meta_data = md; */
  HaArrayInit(res->solutions, a);
  HaArrayInit(res->soln_sets, a);
  /* HaArrayInit(res->write_only_solns, a); */
  if( archive != NULL )
    KheArchiveAddSolnGroup(archive, res);
  *soln_group = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupDelete(KHE_SOLN_GROUP soln_group, bool delete_solns)    */
/*                                                                           */
/*  Delete soln_group.  If delete_solns is true, also delete its solutions.  */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupDelete(KHE_SOLN_GROUP soln_group, bool delete_solns)
{
  KHE_SOLN soln;

  /* remove soln_group's solutions from soln_group; optionally delete them */
  while( HaArrayCount(soln_group->solutions) > 0 )
  {
    soln = HaArrayFirst(soln_group->solutions);
    KheSolnGroupDeleteSoln(soln_group, soln);
    if( delete_solns )
      KheSolnDelete(soln);
  }

  /* remove soln_group from its archive */
  KheArchiveDeleteSolnGroup(soln_group->archive, soln_group);

  /* free memory */
  KheArchiveArenaEnd(soln_group->archive, soln_group->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupSetBack(KHE_SOLN_GROUP soln_group, void *back)          */
/*                                                                           */
/*  Set the back pointer of soln_group                                       */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupSetBack(KHE_SOLN_GROUP soln_group, void *back)
{
  soln_group->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheSolnGroupBack(KHE_SOLN_GROUP soln_group)                        */
/*                                                                           */
/*  Return the back pointer of soln_group.                                   */
/*                                                                           */
/*****************************************************************************/

void *KheSolnGroupBack(KHE_SOLN_GROUP soln_group)
{
  return soln_group->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ARCHIVE KheSolnGroupArchive(KHE_SOLN_GROUP soln_group)               */
/*                                                                           */
/*  Return the archive containing soln_group.                                */
/*                                                                           */
/*****************************************************************************/

KHE_ARCHIVE KheSolnGroupArchive(KHE_SOLN_GROUP soln_group)
{
  return soln_group->archive;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSolnGroupId(KHE_SOLN_GROUP soln_group)                          */
/*                                                                           */
/*  Return the Id attribute of soln_group.                                   */
/*                                                                           */
/*****************************************************************************/

char *KheSolnGroupId(KHE_SOLN_GROUP soln_group)
{
  return soln_group->id;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupSetMetaData(KHE_SOLN_GROUP soln_group,                  */
/*    char *contributor, char *date, char *description, char *publication,   */
/*    char *remarks)                                                         */
/*                                                                           */
/*  Set the metadata fields of soln_group.                                   */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupSetMetaData(KHE_SOLN_GROUP soln_group,
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
/*  void KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group,                     */
/*    char **contributor, char **date, char **description,                   */
/*    char **publication, char **remarks)                                    */
/*                                                                           */
/*  Return the metadata fields of soln_group.                                */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group,
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
/*  char *KheSolnGroupMetaDataText(KHE_SOLN_GROUP soln_group)                */
/*                                                                           */
/*  Return the soln_group metadata as a paragraph of English text.           */
/*                                                                           */
/*****************************************************************************/

char *KheSolnGroupMetaDataText(KHE_SOLN_GROUP soln_group)
{
  HA_ARRAY_NCHAR anc;  HA_ARENA a;  char *s;

  /* handle the case of no metadata */
  if( HnStringIsWhiteSpaceOnly(soln_group->meta_contributor) &&
      HnStringIsWhiteSpaceOnly(soln_group->meta_date) &&
      HnStringIsWhiteSpaceOnly(soln_group->meta_description) &&
      HnStringIsWhiteSpaceOnly(soln_group->meta_publication) &&
      HnStringIsWhiteSpaceOnly(soln_group->meta_remarks) )
    return "This solution group has no metadata.";

  /* description */
  a = soln_group->arena;
  HnStringBegin(anc, a);
  if( HnStringIsWhiteSpaceOnly(soln_group->meta_description) )
    HnStringAdd(&anc, "This solution group has no description.\n");
  else
    HnStringAdd(&anc, "%s.\n",
      HnStringCopyStripped(soln_group->meta_description, a));

  /* contributor and date */
  if( !HnStringIsWhiteSpaceOnly(soln_group->meta_contributor) )
  {
    if( !HnStringIsWhiteSpaceOnly(soln_group->meta_date) )
    {
      /* contributor and date */
      HnStringAdd(&anc, "Contributed by %s on %s.\n",
        HnStringCopyStripped(soln_group->meta_contributor, a),
        HnStringCopyStripped(soln_group->meta_date, a));
    }
    else
    {
      /* contributor only */
      HnStringAdd(&anc, "Contributed by %s.\n",
        HnStringCopyStripped(soln_group->meta_contributor, a));
    }
  }
  else
  {
    if( !HnStringIsWhiteSpaceOnly(soln_group->meta_date) )
    {
      /* date only */
      HnStringAdd(&anc, "  Contributed on %s.\n",
        HnStringCopyStripped(soln_group->meta_date, a));
    }
  }

  /* publication */
  if( !HnStringIsWhiteSpaceOnly(soln_group->meta_publication) )
  {
    s = HnStringCopyStripped(soln_group->meta_publication, a);
    if( s[strlen(s) - 1] == '.' )
      HnStringAdd(&anc, "%s\n", s);
    else
      HnStringAdd(&anc, "%s.\n", s);
  }

  /* remarks */
  if( !HnStringIsWhiteSpaceOnly(soln_group->meta_remarks) )
  {
    s = HnStringCopyStripped(soln_group->meta_remarks, a);
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
/*  KHE_SOLN_GROUP_METADATA KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group)  */
/*                                                                           */
/*  Return the MetaData attribute of soln_group.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_SOLN_GROUP_METADATA KheSolnGroupMetaData(KHE_SOLN_GROUP soln_group)
{
  return soln_group->meta_data;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupSetMetaData(KHE_SOLN_GROUP soln_group,                  */
/*    KHE_SOLN_GROUP_METADATA md)                                            */
/*                                                                           */
/*  Set the metadata attribute of soln_group to md.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnGroupSetMetaData(KHE_SOLN_GROUP soln_group,
  KHE_SOLN_GROUP_METADATA md)
{
  soln_group->meta_data = md;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solutions"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupAddSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)       */
/*                                                                           */
/*  Add soln to soln_group.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupAddSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)
{
  KHE_INSTANCE ins;  int index;  KHE_SOLN_SET ss;

  /* ensure soln's instance lies in soln_group's archive, and get its index */
  ins = KheSolnInstance(soln);
  if( !KheArchiveContainsInstance(soln_group->archive, ins, &index) )
    HnAbort("KheSolnGroupAddSoln: soln's instance not in soln_group's archive");

  /* add soln to soln_group->solutions */
  HaArrayAddLast(soln_group->solutions, soln);
  KheSolnAddSolnGroup(soln, soln_group);

  /* add soln to soln_group->soln_sets */
  while( index >= HaArrayCount(soln_group->soln_sets) )
  {
    ss = KheSolnSetMake(soln_group->arena);
    HaArrayAddLast(soln_group->soln_sets, ss);
  }
  ss = HaArray(soln_group->soln_sets, index);
  KheSolnSetAddSoln(ss, soln);
}


/* *** old version from before instance indexes
void KheSolnGroupAddSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)
{
  KHE_ARCHIVE archive;  KHE_INSTANCE ins;  int i;

  ** ensure soln's instance lies in soln_group's archive **
  archive = KheSolnGroupArchive(soln_group);
  for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
  {
    ins = KheArchiveInstance(archive, i);
    if( ins == KheSolnInstance(soln) )
      break;
  }
  HnAssert(i < KheArchiveInstanceCount(archive),
    "KheSolnGroupAddSoln: soln's instance is not in soln_group's archive");

  ** do the adding **
  HaArrayAddLast(soln_group->solutions, soln);
  KheSolnAddSolnGroup(soln, soln_group);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupAddSolnWriteOnly(KHE_SOLN_GROUP soln_group,             */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Add soln to soln_group in write-only form.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSolnGroupAddSolnWriteOnly(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)
{
  HaArrayAddLast(soln_group->write_only_solns,
    KheSolnWriteOnlyFromSoln(soln, soln_group->arena));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupDeleteSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)    */
/*                                                                           */
/*  Delete soln from soln_group.                                             */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupDeleteSoln(KHE_SOLN_GROUP soln_group, KHE_SOLN soln)
{
  int pos, index;  KHE_INSTANCE ins;  KHE_SOLN_SET ss;

  /* ensure soln's instance lies in soln_group's archive, and get its index */
  ins = KheSolnInstance(soln);
  if( !KheArchiveContainsInstance(soln_group->archive, ins, &index) )
    HnAbort("KheSolnGroupDeleteSoln: soln's instance not in soln_group's"
      " archive");

  /* delete soln from soln_group->solutions */
  if( !HaArrayContains(soln_group->solutions, soln, &pos) )
    HnAbort("KheSolnGroupDeleteSoln: soln not present");
  HaArrayDeleteAndShift(soln_group->solutions, pos);
  KheSolnDeleteSolnGroup(soln, soln_group);

  /* delete soln from soln_group->soln_sets */
  HnAssert(index < HaArrayCount(soln_group->soln_sets),
    "KheSolnGroupDeleteSoln: internal error 1");
  ss = HaArray(soln_group->soln_sets, index);
  HnAssert(KheSolnSetContainsSoln(ss, soln, &pos),
    "KheSolnGroupDeleteSoln: internal error 2");
  KheSolnSetDeleteSoln(ss, soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupDeleteInstance(KHE_SOLN_GROUP soln_group,               */
/*    KHE_INSTANCE ins, int index)                                           */
/*                                                                           */
/*  Delete from soln_group all solutions for ins, whose index in             */
/*  soln_group's archive is index.                                           */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupDeleteInstance(KHE_SOLN_GROUP soln_group,
  KHE_INSTANCE ins, int index)
{
  KHE_SOLN soln;  int i;  /* KHE_SOLN_SET ss; */

  /* delete ins's solutions from soln_group->solutions */
  HaArrayForEach(soln_group->solutions, soln, i)
    if( KheSolnInstance(soln) == ins )
    {
      HaArrayDeleteAndShift(soln_group->solutions, i);
      i--;
      KheSolnDeleteSolnGroup(soln, soln_group);
    }

  /* delete ins's solutions from soln_group->soln_sets */
  if( index < HaArrayCount(soln_group->soln_sets) )
  {
    /* ss = HaArray(soln_group->soln_sets, index); */
    /* KheSolnSetDelete(ss); */
    HaArrayDeleteAndShift(soln_group->soln_sets, index);
  }
}


/* *** obsolete earlier version
void KheSolnGroupDeleteSolnsForInstance(KHE_SOLN_GROUP soln_group,
  KHE_INSTANCE ins)
{
  KHE_SOLN soln;  int i;
  HaArrayForEach(soln_group->solutions, soln, i)
    if( KheSolnInstance(soln) == ins )
    {
      KheSolnGroupDeleteSoln(soln_group, soln);
      i--;
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSolnGroupSolnCount(KHE_SOLN_GROUP soln_group)                     */
/*                                                                           */
/*  Return the number of solutions in soln_group.                            */
/*                                                                           */
/*****************************************************************************/

int KheSolnGroupSolnCount(KHE_SOLN_GROUP soln_group)
{
  return HaArrayCount(soln_group->solutions);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheSolnGroupSoln(KHE_SOLN_GROUP soln_group, int i)              */
/*                                                                           */
/*  Return the i'th solution of soln_group.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheSolnGroupSoln(KHE_SOLN_GROUP soln_group, int i)
{
  return HaArray(soln_group->solutions, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_SET KheSolnGroupInstanceSolnSet(KHE_SOLN_GROUP soln_group,      */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Return a solution set holding the solutions of ins in soln_group.        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_SET KheSolnGroupInstanceSolnSet(KHE_SOLN_GROUP soln_group,
  KHE_INSTANCE ins)
{
  int index;
  if( !KheArchiveContainsInstance(soln_group->archive, ins, &index) )
    HnAbort("KheSolnGroupInstanceSolnSet: ins not in soln_group's archive");
  return KheSolnGroupInstanceSolnSetByIndex(soln_group, index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_SET KheSolnGroupInstanceSolnSetByIndex(                         */
/*    KHE_SOLN_GROUP soln_group, int index)                                  */
/*                                                                           */
/*  Return the solution set for the index'th instance of soln_group's        */
/*  archive.                                                                 */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN_SET KheSolnGroupInstanceSolnSetByIndex(
  KHE_SOLN_GROUP soln_group, int index)
{
  KHE_SOLN_SET ss;

  /* check that index is in range */
  HnAssert(index >= 0 && index < KheArchiveInstanceCount(soln_group->archive),
    "KheSolnGroupInstanceSolnSetByIndex:  index (%d) out of range (0 - %d)",
    index, KheArchiveInstanceCount(soln_group->archive) - 1);

  /* make sure there is a solution set for this index, possibly empty */
  while( index >= HaArrayCount(soln_group->soln_sets) )
  {
    ss = KheSolnSetMake(soln_group->arena);
    HaArrayAddLast(soln_group->soln_sets, ss);
  }

  /* return the index'th soln set */
  return HaArray(soln_group->soln_sets, index);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnGroupMetaDataMakeFromKml(KML_ELT md_elt,                     */
/*    KHE_SOLN_GROUP soln_group, KML_ERROR *ke)                              */
/*                                                                           */
/*  Add solution group metadata based on md_elt to soln_group.               */
/*                                                                           */
/*****************************************************************************/

bool KheSolnGroupMetaDataMakeFromKml(KML_ELT md_elt,
  KHE_SOLN_GROUP soln_group, KML_ERROR *ke)
{
  KML_ELT elt;
  if( !KmlCheck(md_elt,
      ": $Contributor $Date $Description +$Publication +$Remarks", ke) )
    return false;
  KheSolnGroupSetMetaData(soln_group,
    KmlText(KmlChild(md_elt, 0)),
    KmlText(KmlChild(md_elt, 1)),
    KmlText(KmlChild(md_elt, 2)),
    KmlContainsChild(md_elt, "Publication", &elt) ? KmlText(elt) : NULL,
    KmlContainsChild(md_elt, "Remarks", &elt) ? KmlText(elt) : NULL);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnGroupMakeFromKml(KML_ELT soln_group_elt, KHE_ARCHIVE archive,*/
/*    bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,    */
/*    KHE_SOLN_GROUP *sg, KML_ERROR *ke, HA_ARENA a)                         */
/*                                                                           */
/*  Make a solution group based on soln_group_elt and add it to archive.     */
/*  This will included adding solutions if there are any, but in reality     */
/*  incremental file reading causes this function to be called before        */
/*  solution elements are added.                                             */
/*                                                                           */
/*  Parameter a is used to hold *ke, if it needs to be created.              */
/*                                                                           */
/*****************************************************************************/

bool KheSolnGroupMakeFromKml(KML_ELT soln_group_elt, KHE_ARCHIVE archive,
  bool allow_invalid_solns, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,
  KHE_SOLN_GROUP *sg, KML_ERROR *ke, HA_ARENA a)
{
  KHE_SOLN_GROUP res;  int i;  KML_ELT soln_elt;  char *id;  KHE_SOLN soln;

  /* check soln_group_elt and make res */
  if( !KmlCheck(soln_group_elt, "Id : MetaData *Solution", ke) )
    return *sg = NULL, false;
  id = KmlAttributeValue(soln_group_elt, 0);
  if( !KheSolnGroupMake(archive, id, &res) )
  {
    *ke = KmlErrorMake(a, KmlLineNum(soln_group_elt), KmlColNum(soln_group_elt),
      "<SolutionGroup> Id \"%s\" used previously", id);
    return *sg = NULL, false;
  }

  /* add metadata and solutions */
  if( !KheSolnGroupMetaDataMakeFromKml(KmlChild(soln_group_elt, 0), res, ke) )
    return *sg = NULL, false;
  for( i = 1;  i < KmlChildCount(soln_group_elt);  i++ )
  {
    soln_elt = KmlChild(soln_group_elt, i);
    if( !KheSolnMakeFromKml(soln_elt, archive, allow_invalid_solns,
	  soln_type, as, &soln, ke, a) )
      return *sg = NULL, false;
    KheSolnGroupAddSoln(res, soln);
  }
  return *sg = res, true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupMetaDataWrite(KHE_SOLN_GROUP soln_group, KML_FILE kf)   */
/*                                                                           */
/*  Write soln_group metadata to xml.                                        */
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


void KheSolnGroupMetaDataWrite(KHE_SOLN_GROUP soln_group, KML_FILE kf)
{
  if( Has(soln_group->meta_contributor) || Has(soln_group->meta_date) ||
      Has(soln_group->meta_description) || Has(soln_group->meta_publication) ||
      Has(soln_group->meta_remarks) )
  {
    KmlBegin(kf, "MetaData");
    KmlEltPlainText(kf, "Contributor",
      Val(soln_group->meta_contributor, "No contributor"));
    KmlEltPlainText(kf, "Date",
      Val(soln_group->meta_date, "No date"));
    KmlEltPlainText(kf, "Description",
      Val(soln_group->meta_description, "No description"));
    if( Has(soln_group->meta_publication) )
      KmlEltPlainText(kf, "Publication", soln_group->meta_publication);
    if( Has(soln_group->meta_remarks) )
      KmlEltPlainText(kf, "Remarks", soln_group->meta_remarks);
    KmlEnd(kf, "MetaData");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnGroupWrite(KHE_SOLN_GROUP soln_group, bool with_reports,     */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write soln_group to kf, with reports if with_reports is true.            */
/*                                                                           */
/*****************************************************************************/

void KheSolnGroupWrite(KHE_SOLN_GROUP soln_group, bool with_reports,
  KML_FILE kf)
{
  KHE_SOLN soln;  /* KHE_SOLN_WRITE_ONLY swo; */  int i;

  /* make sure safe to write */
  HnAssert(soln_group->id != NULL,
    "KheArchiveWrite: Id missing in SolutionGroup");
  /* ***
  HnAssert(!with_reports || HaArrayCount(soln_group->write_only_solns) == 0,
    "cannot write soln group %s with reports (it has write-only solutions)",
    soln_group->id);
  *** */

  /* write ordinary solutions followed by write-only solutions */
  KmlBegin(kf, "SolutionGroup");
  KmlAttribute(kf, "Id", soln_group->id);
  /* *** NULL metadata is allowed now
  HnAssert(soln_group->meta_data != NULL,
    "KheArchiveWrite: MetaData missing in SolutionGroup");
  *** */
  KheSolnGroupMetaDataWrite(soln_group, kf);
  HaArrayForEach(soln_group->solutions, soln, i)
    KheSolnWrite(soln, with_reports, kf);
  /* ***
  HaArrayForEach(soln_group->write_only_solns, swo, i)
    KheSolnWriteOnlyWrite(swo, kf);
  *** */
  KmlEnd(kf, "SolutionGroup");
}

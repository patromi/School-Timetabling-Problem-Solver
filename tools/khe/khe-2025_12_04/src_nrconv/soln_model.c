
/*****************************************************************************/
/*                                                                           */
/*  THE NRCONV NURSE ROSTERING TO XHSTT CONVERTER                            */
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
/*  FILE:         soln_model.c                                               */
/*  MODULE:       Main module, handles reading and operation dispatch        */
/*                                                                           */
/*****************************************************************************/
#define _POSIX_SOURCE
#include "externs.h"


/*****************************************************************************/
/*                                                                           */
/*  formats - maps soln source format names to conversion functions          */
/*                                                                           */
/*****************************************************************************/

typedef void (*CVT_SOLN_FN)(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as);

static struct {
  char			*name;
  CVT_SOLN_FN		cvt_soln_fn;
} formats[] =
{
  {"coi-soln.xml", &COIConvertSoln},
  {"inrc1-soln.xml", &INRC1ConvertSoln},
  {"inrc2-soln.xml", &INRC2ConvertSoln},
  {"inrc2-soln-omer.txt", &INRC2ConvertSolnOmer},
  {"inrc2-soln-schaerf.txt", &INRC2ConvertSolnSchaerf},
  {"cq14-soln.xml", &CQ14ConvertSoln},
  {NULL, NULL}
};


/*****************************************************************************/
/*                                                                           */
/*  linkages - maps soln group function names to soln group functions        */
/*                                                                           */
/*****************************************************************************/

typedef NRC_SOLN_GROUP (*SOLN_GROUP_FN)(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime);

static NRC_SOLN_GROUP FirstSolnGroupFn(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime);

static struct {
  char			*name;
  SOLN_GROUP_FN		soln_group_fn;
} linkages[] =
{
  {"first", &FirstSolnGroupFn},
  {"cq14", &CQ14SolnGroupFn},
  {NULL, NULL}
};


/*****************************************************************************/
/*                                                                           */
/*  SOLN_GROUP_MODEL - one soln group model                                  */
/*                                                                           */
/*****************************************************************************/

/* *** example:
SolnGroup: GOAL
Contributor: The GOAL team
Date:
Description:
Publication: http://www.goal.ufop.br/nrp/
Remarks: converted from INRC1 format by NRConv
*** */

typedef struct soln_group_model_rec {
  char			*name;
  char			*contributor;
  char			*date;
  char			*description;
  char			*publication;
  char			*remarks;
} *SOLN_GROUP_MODEL;

typedef HA_ARRAY(SOLN_GROUP_MODEL) ARRAY_SOLN_GROUP_MODEL;


/*****************************************************************************/
/*                                                                           */
/*  SOLN_MODEL - one solution model                                          */
/*                                                                           */
/*****************************************************************************/

/* *** example:
SolnSourceFormat: inrc1-soln
LinkageToInstance: internal
LinkageToSolnGroup: first
Keep: best
SolnGroup: GOAL
Contributor: The GOAL team
Date:
Description:
Publication: http://www.goal.ufop.br/nrp/
Remarks: converted from INRC1 format by NRConv
*** */

typedef enum {
  SOLN_MODEL_KEEP_ALL,
  SOLN_MODEL_KEEP_BEST
} SOLN_MODEL_KEEP;

struct soln_model_rec {
  CVT_SOLN_FN			cvt_soln_fn;
  bool				external_linkage_to_instance;
  SOLN_GROUP_FN			soln_group_fn;
  SOLN_MODEL_KEEP		keep;
  ARRAY_SOLN_GROUP_MODEL	soln_group_models;
};

/* keeps - maps keep strings to keep values */
static struct {
  char			*name;
  SOLN_MODEL_KEEP	keep;
} keeps[] =
{
  {"all", SOLN_MODEL_KEEP_ALL},
  {"best", SOLN_MODEL_KEEP_BEST},
  {NULL, 0}
};



/*****************************************************************************/
/*                                                                           */
/*  Soln group functions                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void CheckField(char *str1, char *str2, char *tag)                       */
/*                                                                           */
/*  If str1 and str2 differ, exit with a fatal error quoting tag.            */
/*                                                                           */
/*****************************************************************************/

static void CheckField(char *str1, char *str2, char *tag)
{
  /* normalize empty strings to NULL */
  if( str1 != NULL && *str1 == '\0' ) str1 = NULL;
  if( str2 != NULL && *str2 == '\0' ) str2 = NULL;

  /* do the comparison, with NULL strings a special case */
  if( ((str1 == NULL) != (str2 == NULL)) ||
      (str1 != NULL && strcmp(str1, str2) != 0) )
    FatalError(NULL, -1, "inconsistent %s metadata (%s vs %s)",
      str1 == NULL ? "(null)" : str1, str2 == NULL ? "(null)" : str2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool GetSolnGroupModel(FILE *fp, char *file_name, NRC_ARCHIVE archive,   */
/*    SOLN_GROUP_MODEL *res)                                                 */
/*                                                                           */
/*  Read a soln_group model from fp, or return NULL if not present.          */
/*                                                                           */
/*****************************************************************************/

static bool GetSolnGroupModel(FILE *fp, char *file_name, NRC_ARCHIVE archive,
  SOLN_GROUP_MODEL *res, HA_ARENA a)
{
  SOLN_GROUP_MODEL sgm;  /* NRC_SOLN_GROUP_METADATA md; */
  NRC_SOLN_GROUP soln_group;
  char *name, *contributor, *date, *description, *publication, *remarks;
  if( ReadOptionalModelLine(fp, file_name, "SolnGroup", &name, a) )
  {
    /* make the soln group model */
    HaMake(sgm, a);
    sgm->name = name;
    sgm->contributor = ReadModelLine(fp, file_name, "Contributor", a);
    sgm->date = ReadModelLine(fp, file_name, "Date", a);
    sgm->description = ReadModelLine(fp, file_name, "Description", a);
    sgm->publication = ReadModelLine(fp, file_name, "Publication", a);
    sgm->remarks = ReadModelLine(fp, file_name, "Remarks", a);
    *res = sgm;

    /* ensure that it is in archive */
    if( NrcArchiveRetrieveSolnGroup(archive, name, &soln_group) )
    {
      /* already in archive; check consistency */
      NrcSolnGroupMetaData(soln_group, &contributor, &date, &description,
	&publication, &remarks);
      CheckField(contributor, sgm->contributor, "contributor");
      CheckField(date, sgm->date, "date");
      CheckField(description, sgm->description, "description");
      CheckField(publication, sgm->publication, "publication");
      CheckField(remarks, sgm->remarks, "remarks");
      /* ***
      md = NrcSolnGroupMetaData(soln_group);
      CheckField(NrcSolnGroupMetaDataContributor(md), sgm->contributor,
	"contributor");
      CheckField(NrcSolnGroupMetaDataDate(md), sgm->date,
	"date");
      CheckField(NrcSolnGroupMetaDataDescription(md), sgm->description,
        "description");
      CheckField(NrcSolnGroupMetaDataPublication(md), sgm->publication,
        "publication");
      CheckField(NrcSolnGroupMetaDataRemarks(md), sgm->remarks,
        "remarks");
      *** */
    }
    else
    {
      /* add a soln group based on sgm to archive */
      NrcSolnGroupMake(archive, sgm->name, &soln_group);
      NrcSolnGroupSetMetaData(soln_group, sgm->contributor, sgm->date,
	sgm->description, sgm->publication, sgm->remarks);
      /* ***
      md = NrcSolnGroupMetaDataMake(sgm->contributor,
        sgm->date, sgm->description,
	*sgm->publication == '\0' ? NULL : sgm->publication,
	*sgm->remarks == '\0' ? NULL : sgm->remarks);
      NrcSolnGroupMake(archive, sgm->name, md, &soln_group);
      *** */
    }
    return true;
  }
  else
  {
    *res = NULL;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  SOLN_MODEL SolnModelMake(char *file_name, NRC_ARCHIVE archive)           */
/*                                                                           */
/*  Get soln model from file_name.  Ensure that its soln groups are in       */
/*  archive.                                                                 */
/*                                                                           */
/*****************************************************************************/

SOLN_MODEL SolnModelMake(char *file_name, NRC_ARCHIVE archive, HA_ARENA a)
{
  FILE *fp;  SOLN_MODEL res;  char *str;  SOLN_GROUP_MODEL sgm;  int i;

  /* open file and fail if can't */
  fp = fopen(file_name, "r");
  if( fp == NULL )
    FatalError(file_name, -1, "cannot open model file");

  /* make res and get NRConvSolnModelName */
  HaMake(res, a);

  /* get SolnSourceFormat */
  str = ReadModelLine(fp, file_name, "SolnSourceFormat", a);
  for( i = 0;  formats[i].name != NULL;  i++ )
    if( strcmp(formats[i].name, str) == 0 )
      break;
  if( formats[i].name == NULL )
    FatalError(file_name, -1, "unknown SolnSourceFormat %s", str);
  res->cvt_soln_fn = formats[i].cvt_soln_fn;

  /* get LinkageToInstance */
  str = ReadModelLine(fp, file_name, "LinkageToInstance", a);
  if( strcmp(str, "internal") == 0 )
    res->external_linkage_to_instance = false;
  else if( strcmp(str, "external") == 0 )
    res->external_linkage_to_instance = true;
  else
    FatalError(file_name, -1, "unknown value %s of LinkageToInstance");

  /* get LinkageToSolnGroup */
  str = ReadModelLine(fp, file_name, "LinkageToSolnGroup", a);
  for( i = 0;  linkages[i].name != NULL;  i++ )
    if( strcmp(linkages[i].name, str) == 0 )
      break;
  if( linkages[i].name == NULL )
    FatalError(file_name, -1, "unknown LinkageToSolnGroup %s", str);
  res->soln_group_fn = linkages[i].soln_group_fn;

  /* get Keep */
  str = ReadModelLine(fp, file_name, "Keep", a);
  for( i = 0;  keeps[i].name != NULL;  i++ )
    if( strcmp(keeps[i].name, str) == 0 )
      break;
  if( keeps[i].name == NULL )
    FatalError(file_name, -1, "unknown Keep %s", str);
  res->keep = keeps[i].keep;

  /* get soln group models and return */
  HaArrayInit(res->soln_group_models, a);
  while( GetSolnGroupModel(fp, file_name, archive, &sgm, a) )
    HaArrayAddLast(res->soln_group_models, sgm);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnModelConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,   */
/*    NRC_INSTANCE ins, NRC_ARCHIVE archive)                                 */
/*                                                                           */
/*  Convert a solution which uses soln_model and is stored in                */
/*  soln_file_name into an NRC soln and add it to a solution group of        */
/*  archive that the solution chooses for itself.  If this model uses        */
/*  extenal linkage to instances, ins must be non-NULL.                      */
/*                                                                           */
/*****************************************************************************/

void SolnModelConvertSoln(SOLN_MODEL soln_model, char *soln_file_name,
  NRC_INSTANCE ins, NRC_ARCHIVE archive, HA_ARENA_SET as)
{
  /* make sure instance linkage is correct */
  if( ins == NULL && soln_model->external_linkage_to_instance )
    FatalError(NULL, -1, "solution model requires instance in -s flag");
  if( ins != NULL && !soln_model->external_linkage_to_instance )
    FatalError(NULL, -1, "solution model requires no instance in -s flag");

  /* do the conversion */
  soln_model->cvt_soln_fn(soln_model, soln_file_name, ins, archive, as);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP SolnModelSolnGroup(SOLN_MODEL soln_model,                 */
/*    NRC_ARCHIVE archive, char *algorithm, float runtime)                   */
/*                                                                           */
/*  Return the appropriate soln group of archive for a soln with these       */
/*  attributes.                                                              */
/*                                                                           */
/*****************************************************************************/

static NRC_SOLN_GROUP SolnModelSolnGroup(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime)
{
  return soln_model->soln_group_fn(soln_model, archive, algorithm, runtime);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SOLN_GROUP FirstSolnGroupFn(SOLN_MODEL soln_model,                   */
/*    NRC_ARCHIVE archive, char *algorithm, float runtime)                   */
/*                                                                           */
/*  Return the first soln group named in soln_model.                         */
/*                                                                           */
/*****************************************************************************/

NRC_SOLN_GROUP FirstSolnGroupFn(SOLN_MODEL soln_model,
  NRC_ARCHIVE archive, char *algorithm, float runtime)
{
  SOLN_GROUP_MODEL sgm;  NRC_SOLN_GROUP res;
  if( HaArrayCount(soln_model->soln_group_models) == 0 )
    FatalError(NULL, 0, "LinkageToSolnGroup first: no soln groups in model");
  sgm = HaArrayFirst(soln_model->soln_group_models);
  if( !NrcArchiveRetrieveSolnGroup(archive, sgm->name, &res) )
    HnAbort("LinkageToSolnGroup first: soln group %s not found", sgm->name);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int SolnReportedCost(NRC_SOLN soln)                                      */
/*                                                                           */
/*  Return the reported cost of soln, or 0 if none.                          */
/*                                                                           */
/*****************************************************************************/

static int SolnReportedCost(NRC_SOLN soln)
{
  char *str;  int res;
  str = NrcSolnDescription(soln);
  if( str != NULL && sscanf(str, "Reported cost %d", &res) == 1 )
    return res;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnModelAddSoln(SOLN_MODEL soln_model, NRC_ARCHIVE archive,        */
/*    char *algorithm, float runtime, NRC_SOLN soln)                         */
/*                                                                           */
/*  Add soln to archive.                                                     */
/*                                                                           */
/*****************************************************************************/

void SolnModelAddSoln(SOLN_MODEL soln_model, NRC_ARCHIVE archive,
  char *algorithm, float runtime, NRC_SOLN soln)
{
  NRC_SOLN_GROUP sg;  NRC_SOLN soln2;  int i;
  sg = SolnModelSolnGroup(soln_model, archive, algorithm, runtime);
  switch( soln_model->keep )
  {
    case SOLN_MODEL_KEEP_ALL:

      NrcSolnGroupAddSoln(sg, soln);
      break;

    case SOLN_MODEL_KEEP_BEST:

      /* find an existing solution for the same instance, if any */
      for( i = 0;  i < NrcSolnGroupSolnCount(sg);  i++ )
      {
	soln2 = NrcSolnGroupSoln(sg, i);
	if( NrcSolnInstance(soln2) == NrcSolnInstance(soln) )
	  break;
      }

      if( i < NrcSolnGroupSolnCount(sg) )
      {
	/* existing solution i, either replace it or not */
        soln2 = NrcSolnGroupSoln(sg, i);
	if( SolnReportedCost(soln) < SolnReportedCost(soln2) )
	{
	  NrcSolnGroupDeleteSoln(sg, soln2);
	  NrcSolnGroupAddSoln(sg, soln);
	}
      }
      else
      {
	/* no existing solution, so just add soln */
	NrcSolnGroupAddSoln(sg, soln);
      }
      break;

    default:

      HnAbort("SolnModelAddSoln internal error");
      break;
  }
}


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
/*  FILE:         ins_model.c                                                */
/*  MODULE:       Instance model                                             */
/*                                                                           */
/*****************************************************************************/
#define _POSIX_SOURCE
#include "externs.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  formats - maps instance source format names to conversion functions      */
/*                                                                           */
/*****************************************************************************/

typedef NRC_INSTANCE (*CVT_INSTANCE_FN)(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as);

static struct {
  char			*name;
  CVT_INSTANCE_FN	cvt_instance_fn;
} formats[] =
{
  /* {"bcv.xml", &BCVConvertInstance}, abandoned */
  {"coi.xml", &COIConvertInstance},
  {"inrc1.xml", &INRC1ConvertInstance},
  {"inrc2.xml", &INRC2ConvertInstance},
  {"inrc2-static.xml", &INRC2ConvertInstanceStatic},
  {"cq14.txt", &CQ14ConvertInstance},
  {NULL, NULL}
};


/*****************************************************************************/
/*                                                                           */
/*  INSTANCE_MODEL                                                           */
/*                                                                           */
/*****************************************************************************/

struct instance_model_rec {
  CVT_INSTANCE_FN	cvt_instance_fn;
  char			*contributor;
  char			*date;
  char			*country;
  char			*description;
  char			*remarks;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading model files"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool ReadOptionalModelLine(FILE *fp, char *file_name,                    */
/*    char *expected_label, char **res, HA_ARENA a)                          */
/*                                                                           */
/*  Read one line of model file fp, setting buff to its value.  The line     */
/*  is expected to begin with expected_label and a colon, and there will     */
/*  be a fatal error if not.                                                 */
/*                                                                           */
/*****************************************************************************/

static bool is_space(char ch)
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

bool ReadOptionalModelLine(FILE *fp, char *file_name,
  char *expected_label, char **res, HA_ARENA a)
{
  char line[5000];  int len;

  /* get the next non-empty line, or return false if there isn't one */
  do
  {
    /* get one line */
    if( !fgets(line, 5000, fp) )
      return false;

    /* trim any white space off the end */
    for( len = strlen(line) - 1;  len >= 0 && is_space(line[len]);  len-- );
    line[len + 1] = '\0';

    /* try again if the line is empty */
  } while (line[0] == '\0');

  /* if line does not being with expected_label, it's an error */
  if( strstr(line, expected_label) != line )
    FatalError(file_name, -1, "label %s missing or misplaced in model file",
      expected_label);

  /* if there is no following colon, that's an error */
  len = strlen(expected_label);
  if( line[len] != ':' )
    FatalError(file_name, -1, "colon missing on %s line of model file",
      expected_label);

  /* skip the colon and any following white space */
  for( len++;  line[len] == ' ' || line[len] == '\t';  len++ );

  /* copy the rest into buff and success */
  *res = HnStringCopy(&line[len], a);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *ReadModelLine(FILE *fp, char *file_name, char *expected_label,     */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Read a line of fp, like ReadOptionalModelLine except compulsory.         */
/*                                                                           */
/*****************************************************************************/

char *ReadModelLine(FILE *fp, char *file_name, char *expected_label,
  HA_ARENA a)
{
  char *res;
  if( !ReadOptionalModelLine(fp, file_name, expected_label, &res, a) )
    FatalError(file_name, -1, "missing %s line in model file",
      expected_label);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  INSTANCE_MODEL InstanceModelMake(char *file_name)                        */
/*                                                                           */
/*  Make an instance model, using values from from file_name.                */
/*                                                                           */
/*****************************************************************************/

INSTANCE_MODEL InstanceModelMake(char *file_name, HA_ARENA a)
{
  FILE *fp;  INSTANCE_MODEL res;  char *str;  int i;
  fp = fopen(file_name, "r");
  if( fp == NULL )
    FatalError(file_name, -1, "cannot open model file");
  HaMake(res, a);
  str = ReadModelLine(fp, file_name, "InstanceSourceFormat", a);
  for( i = 0;  formats[i].name != NULL;  i++ )
    if( strcmp(formats[i].name, str) == 0 )
      break;
  if( formats[i].name == NULL )
    FatalError(file_name, -1, "unknown InstanceSourceFormat %s", str);
  res->cvt_instance_fn = formats[i].cvt_instance_fn;
  res->contributor = ReadModelLine(fp, file_name, "Contributor", a);
  res->date = ReadModelLine(fp, file_name, "Date", a);
  res->country = ReadModelLine(fp, file_name, "Country", a);
  res->description = ReadModelLine(fp, file_name, "Description", a);
  res->remarks = ReadModelLine(fp, file_name, "Remarks", a);
  if( *res->remarks == '\0' )
    res->remarks = NULL;
  if( fgetc(fp) != EOF )
    FatalError(file_name, -1, "extra material after Remarks in model file");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE InstanceModelConvertInstance(INSTANCE_MODEL ins_model,      */
/*    char *instance_file_name, HA_ARENA_SET as)                             */
/*                                                                           */
/*  Read an instance which uses the ins_model model from file                */
/*  instance_file_name and convert it to an NRC instance.                    */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE InstanceModelConvertInstance(INSTANCE_MODEL ins_model,
  char *instance_file_name, HA_ARENA_SET as)
{
  NRC_INSTANCE res;
  if( DEBUG1 )
    fprintf(stderr, "[ InstanceModelConvertInstance(ins_model, %s, as)\n",
      instance_file_name);
  res = ins_model->cvt_instance_fn(ins_model, instance_file_name, as);
  if( DEBUG1 )
    fprintf(stderr, "] InstanceModelConvertInstance returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE_METADATA InstanceModelMetaData(INSTANCE_MODEL ins_model,    */
/*    char *name, char *contributor, char *date, char *country,              */
/*    char *description, char *remarks)                                      */
/*                                                                           */
/*  Return instance metadata with these values, or the values from the       */
/*  model if NULL.                                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
NRC_INSTANCE_METADATA InstanceModelMetaData(INSTANCE_MODEL ins_model,
  char *name, char *contributor, char *date, char *country, char *description,
  char *remarks)
{
  return NrcInstanceMetaDataMake(name,
    contributor != NULL ? contributor : ins_model->contributor,
    date != NULL ? date : ins_model->date,
    country != NULL ? country : ins_model->country,
    description != NULL ? description : ins_model->description,
    remarks != NULL ? remarks : ins_model->remarks);
}
*** */


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
/*  FILE:         nrc_day_set_set.c                                          */
/*  MODULE:       A set (actually a sequence) of day-sets                    */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET_SET                                                          */
/*                                                                           */
/*  A sequence of day-sets.                                                  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DAY_SET) ARRAY_NRC_DAY_SET;

struct nrc_day_set_set_rec {
  NRC_INSTANCE		instance;
  char			*short_name;
  char			*long_name;
  ARRAY_NRC_DAY_SET	day_sets;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET_SET NrcDaySetSetMake(NRC_INSTANCE ins, char *short_name,     */
/*    char *long_name)                                                       */
/*                                                                           */
/*  Make a new, empty day-set set with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET_SET NrcDaySetSetMake(NRC_INSTANCE ins, char *short_name,
  char *long_name)
{
  NRC_DAY_SET_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->short_name = HnStringCopy(short_name, a);
  res->long_name = HnStringCopy(long_name, a);
  HaArrayInit(res->day_sets, a);
  NrcInstanceAddDaySetSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDaySetSetAddDaySet(NRC_DAY_SET_SET dss, NRC_DAY_SET ds)          */
/*                                                                           */
/*  Add ds to dss.                                                           */
/*                                                                           */
/*****************************************************************************/

void NrcDaySetSetAddDaySet(NRC_DAY_SET_SET dss, NRC_DAY_SET ds)
{
  HaArrayAddLast(dss->day_sets, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDaySetSetInstance(NRC_DAY_SET_SET dss)                   */
/*                                                                           */
/*  Return the instance attribute of dss.                                    */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDaySetSetInstance(NRC_DAY_SET_SET dss)
{
  return dss->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDaySetSetShortName(NRC_DAY_SET_SET dss)                         */
/*                                                                           */
/*  Return the short_name attribute of dss.                                  */
/*                                                                           */
/*****************************************************************************/

char *NrcDaySetSetShortName(NRC_DAY_SET_SET dss)
{
  return dss->short_name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDaySetSetLongName(NRC_DAY_SET_SET dss)                          */
/*                                                                           */
/*  Return the long_name attribute of dss.                                   */
/*                                                                           */
/*****************************************************************************/

char *NrcDaySetSetLongName(NRC_DAY_SET_SET dss)
{
  return dss->long_name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDaySetSetDaySetCount(NRC_DAY_SET_SET dss)                         */
/*                                                                           */
/*  Return the number of day-sets in dss.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcDaySetSetDaySetCount(NRC_DAY_SET_SET dss)
{
  return HaArrayCount(dss->day_sets);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySetSetDaySet(NRC_DAY_SET_SET dss, int i)               */
/*                                                                           */
/*  Return the i'th day-set of dss.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySetSetDaySet(NRC_DAY_SET_SET dss, int i)
{
  return HaArray(dss->day_sets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetSetRetrieveDaySetShort(NRC_DAY_SET_SET dss,                */
/*    char *short_name, NRC_DAY_SET *ds)                                     */
/*                                                                           */
/*  Retrieve the day-set of dss with this short_name.                        */
/*                                                                           */
/*****************************************************************************/

bool NrcDaySetSetRetrieveDaySetShort(NRC_DAY_SET_SET dss,
  char *short_name, NRC_DAY_SET *ds)
{
  NRC_DAY_SET ds2;  int i;
  HaArrayForEach(dss->day_sets, ds2, i)
    if( strcmp(NrcDaySetShortName(ds2), short_name) == 0 )
    {
      *ds = ds2;
      return true;
    }
  *ds = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetSetRetrieveDaySetLong(NRC_DAY_SET_SET dss,                 */
/*    char *long_name, NRC_DAY_SET *ds)                                      */
/*                                                                           */
/*  Retrieve the day-set of dss with this long_name.                         */
/*                                                                           */
/*****************************************************************************/

bool NrcDaySetSetRetrieveDaySetLong(NRC_DAY_SET_SET dss,
  char *long_name, NRC_DAY_SET *ds)
{
  NRC_DAY_SET ds2;  int i;
  HaArrayForEach(dss->day_sets, ds2, i)
    if( strcmp(NrcDaySetLongName(ds2), long_name) == 0 )
    {
      *ds = ds2;
      return true;
    }
  *ds = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDaySetSetDebug(NRC_DAY_SET_SET dss, int indent, FILE *fp)        */
/*                                                                           */
/*  Debug print of dss onto fp with the given indent.                        */
/*                                                                           */
/*****************************************************************************/

void NrcDaySetSetDebug(NRC_DAY_SET_SET dss, int indent, FILE *fp)
{
  NRC_DAY_SET ds;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Day-set set(%s, %s):\n", indent, "", dss->short_name,
      dss->long_name);
    HaArrayForEach(dss->day_sets, ds, i)
      NrcDaySetDebug(ds, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "{");
    HaArrayForEach(dss->day_sets, ds, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      NrcDaySetDebug(ds, -1, fp);
    }
    fprintf(fp, "}");
  }
}

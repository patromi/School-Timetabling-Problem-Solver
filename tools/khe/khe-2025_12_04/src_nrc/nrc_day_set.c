
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
/*  FILE:         nrc_day_set.c                                              */
/*  MODULE:       A set (actually a sequence) of days                        */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#if USE_MKTIME
#include <time.h>
#endif

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET                                                              */
/*                                                                           */
/*  A sequence of days.                                                      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_DAY) ARRAY_NRC_DAY;

struct nrc_day_set_rec {
  NRC_INSTANCE		instance;
  char			*short_name;
  char			*long_name;
  ARRAY_NRC_DAY		days;
  NRC_SHIFT_SET		starting_ss;
  NRC_SHIFT_SET		all_ss;
  NRC_SHIFT_SET_SET	days_sss;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "internals" (private)                                          */
/*                                                                           */
/*****************************************************************************/
#if USE_MKTIME

/*****************************************************************************/
/*                                                                           */
/*  bool TmFromYMD(char *ymd, struct tm *tp, char **err_str)                 */
/*                                                                           */
/*  Convert a string in the format YYYY-MM-DD into a tm struct.              */
/*                                                                           */
/*  If successful, return true, otherwise set *err_str to an error           */
/*  message and return false.                                                */
/*                                                                           */
/*****************************************************************************/

static bool TmFromYMD(char *ymd, struct tm *tp, char **err_str)
{
  int year, month, day;  time_t res;

  /* parse and check the ymd string */
  if( sscanf(ymd, "%d-%d-%d", &year, &month, &day) != 3 )
  {
    *err_str = "wrong format";
    return false;
  }
  if( year < 1900 )
  {
    *err_str = "year too small (minimum is 1900)";
    return false;
  }
  if( month < 1 )
  {
    *err_str = "month too small (minimum is 1)";
    return false;
  }
  if( month > 12 )
  {
    *err_str = "month too large (maximum is 12)";
    return false;
  }
  if( day < 1 )
  {
    *err_str = "day too small (minimum is 1)";
    return false;
  }
  if( day > 31 )
  {
    *err_str = "day too large (maximum is 31)";
    return false;
  }

  /* set *tp to the beginning of the given year, month, and day */
  tp->tm_sec = 0;
  tp->tm_min = 0;
  tp->tm_hour = 0;
  tp->tm_mday = day;
  tp->tm_mon = month - 1;
  tp->tm_year = year - 1900;
  tp->tm_wday = 0;
  tp->tm_yday = 0;
  tp->tm_isdst = 0;
  res = mktime(tp);
  if( res == (time_t) -1 )
  {
    *err_str = "value is invalid according to mktime()";
    return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int DaysInYear(int year)                                                 */
/*                                                                           */
/*  Return the number of days in year.                                       */
/*                                                                           */
/*****************************************************************************/

static int DaysInYear(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) ? 366 : 365;
}


/*****************************************************************************/
/*                                                                           */
/*  int DayIndex(struct tm *doc1, struct tm *doc2, char **err_str,           */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return the index of doc2 relative to doc1.  This will be 0 when doc2     */
/*  equals doc1, 1 when doc2 is the next doc, and so on.  Or if doc2         */
/*  precedes doc1, return -1 and set *err_str to an error message            */
/*  explaining the problem.                                                  */
/*                                                                           */
/*****************************************************************************/

static int DayIndex(struct tm *doc1, struct tm *doc2, char **err_str,
  HA_ARENA a)
{
  int res, i;

  if( doc1->tm_year > doc2->tm_year )
  {
    *err_str = HnStringMake(a, "second year %04d precedes first year %04d",
      doc1->tm_year + 1900, doc2->tm_year + 1900);
    return -1;
  }
  if( doc1->tm_year == doc2->tm_year )
  {
    if( doc1->tm_mon > doc2->tm_mon )
    {
      *err_str = HnStringMake(a, "second month %02d precedes first month %02d",
	doc1->tm_mon + 1, doc2->tm_mon + 1);
      return -1;
    }
    if( doc1->tm_mon == doc2->tm_mon )
    {
      if( doc1->tm_mon > doc2->tm_mon )
      {
	*err_str = HnStringMake(a, "second day %02d precedes first day %02d",
	  doc1->tm_mday, doc2->tm_mday);
	return -1;
      }
    }
    res = doc2->tm_yday - doc1->tm_yday;
  }
  else
  {
    res = DaysInYear(doc1->tm_year + 1900) - doc1->tm_yday;
    for( i = doc1->tm_year + 1;  i < doc2->tm_year;  i++ )
      res += DaysInYear(i + 1900);
    res += doc2->tm_yday;
  }
  return res;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySetMake(NRC_INSTANCE ins, char *short_name,            */
/*    char *long_name)                                                       */
/*                                                                           */
/*  Make a day set with these names and initially no days.                   */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySetMake(NRC_INSTANCE ins, char *short_name,
  char *long_name)
{
  NRC_DAY_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  HnAssert(short_name != NULL, "NrcDaySetMake: NULL short_name");
  HnAssert(long_name != NULL, "NrcDaySetMake: NULL long_name");
  res->instance = ins;
  res->short_name = HnStringCopy(short_name, a);
  res->long_name = HnStringCopy(long_name, a);
  HaArrayInit(res->days, a);
  res->starting_ss = NULL;
  res->all_ss = NULL;
  res->days_sss = NULL;
  NrcInstanceAddDaySet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDaySetAddDay(NRC_DAY_SET ds, NRC_DAY d)                          */
/*                                                                           */
/*  Add day d to day set ds.                                                 */
/*                                                                           */
/*****************************************************************************/

void NrcDaySetAddDay(NRC_DAY_SET ds, NRC_DAY d)
{
  HnAssert(ds->all_ss == NULL,"NrcDaySetAddDay called after NrcDaySetShiftSet");
  HnAssert(ds->starting_ss == NULL,
    "NrcDaySetAddDay called after NrcDaySetStartingShiftSet");
  HaArrayAddLast(ds->days, d);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySetMakeCycle(NRC_INSTANCE ins, int day_count,          */
/*    int first_day_index, NRC_DAY_SET_SET *days_of_week)                    */
/*                                                                           */
/*  Return a day_set object for the days of a cycle.                         */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySetMakeCycle(NRC_INSTANCE ins, int day_count,
  int first_day_index, NRC_DAY_SET_SET *days_of_week)
{
  NRC_DAY_SET res, dow;  int i, idow;  NRC_DAY day;

  /* make an initially empty day set, for holding the cycle */
  HnAssert(day_count % 7 == 0, "DaySetMakeCQCycle: day_count");
  res = NrcDaySetMake(ins, "Cycle", "Cycle");

  /* make a set of 7 empty day sets, for holding the days of the week */
  *days_of_week = NrcDaySetSetMake(ins, "DaysOfWeek", "DaysOfWeek");
  for( i = 0;  i < 7;  i++ )
  {
    dow = NrcDaySetMake(ins, NrcInstanceShortDayName(ins, i),
      NrcInstanceLongDayName(ins, i));
    NrcDaySetSetAddDaySet(*days_of_week, dow);
  }

  /* make one day for each day of the cycle */
  for( i = 0;  i < day_count;  i++ )
  {
    /* make a day for i */
    idow = (i + first_day_index) % 7;
    dow = NrcDaySetSetDaySet(*days_of_week, idow);
    day = NrcDayMake(ins, "-", i, (i / 7) + 1, idow, dow);
    NrcDaySetAddDay(res, day);
    NrcDaySetAddDay(dow, day);
  }

  /* and return the new days of cycle object */
  if( DEBUG1 )
    NrcDaySetDebug(res, 0, stderr);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySetMakeCalendarCycle(NRC_INSTANCE ins, char *start_ymd,*/
/*    char *end_ymd, NRC_DAY_SET_SET *days_of_week, char **err_str)          */
/*                                                                           */
/*  Return a day_set object for the days of the cycle, whose start and end   */
/*  days have this YMD.  These are the only days that are actually created.  */
/*  Also return a set of day sets, one day set for each day of the week.     */
/*                                                                           */
/*  If there is something wrong with start_ymd and end_ymd, set *err_str to  */
/*  a non-NULL value explaining the problem and return NULL.                 */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySetMakeCalendarCycle(NRC_INSTANCE ins, char *start_ymd,
  char *end_ymd, NRC_DAY_SET_SET *days_of_week, char **err_str)
{
#if USE_MKTIME
  NRC_DAY_SET res, dow;  int day_index, i;  NRC_DAY day;
  struct tm start_tm, end_tm, day_tm;  char *ymd;  HA_ARENA a;

  /* make an initially empty day set, for holding the cycle */
  if( DEBUG1 )
    fprintf(stderr, "[ NrcDaySetMakeCalendarCycle(ins, %s, %s, ...)\n",
      start_ymd, end_ymd);
  a = NrcInstanceArena(ins);
  res = NrcDaySetMake(ins, "Cycle", "Cycle");

  /* make a set of 7 empty day sets, for holding the days of the week */
  *days_of_week = NrcDaySetSetMake(ins, "DaysOfWeek", "DaysOfWeek");
  for( i = 0;  i < 7;  i++ )
  {
    dow = NrcDaySetMake(ins, NrcInstanceShortDayName(ins, i),
      NrcInstanceLongDayName(ins, i));
    NrcDaySetSetAddDaySet(*days_of_week, dow);
  }

  /* convert start_ymd and end_ymd to struct tm format, and make sure OK */
  if( !TmFromYMD(start_ymd, &start_tm, err_str) )
  {
    *err_str = HnStringMake(a, "in StartDate %s, %s", start_ymd, *err_str);
    return NULL;
  }
  if( !TmFromYMD(end_ymd, &end_tm, err_str) )
  {
    *err_str = HnStringMake(a, "in EndDate %s, %s", end_ymd, *err_str);
    return NULL;
  }
  day_index = DayIndex(&start_tm, &end_tm, err_str, a);
  if( day_index == -1 )
  {
    *err_str = HnStringMake(a, "comparing StartDate %s and EndDate %s, %s",
      start_ymd, end_ymd, *err_str);
    return NULL;
  }

  day_tm = start_tm;
  for( i = 0;  i <= day_index;  i++ )
  {
    /* make a day for i */
    mktime(&day_tm);
    ymd = HnStringMake(a, "%04d-%02d-%02d", day_tm.tm_year + 1900,
      day_tm.tm_mon + 1, day_tm.tm_mday);
    dow = NrcDaySetSetDaySet(*days_of_week, day_tm.tm_wday);
    day = NrcDayMake(ins, ymd, i, i / 7 + 1, day_tm.tm_wday, dow);
    NrcDaySetAddDay(res, day);
    NrcDaySetAddDay(dow, day);

    /* move to next day of the month; "man mktime" for why this works! */
    day_tm.tm_mday++;
  }

  /* and return the new days of cycle object */
  if( DEBUG1 )
  {
    NrcDaySetDebug(res, 0, stderr);
    fprintf(stderr, "] NrcDaySetMakeCalendarCycle returning\n");
  }
  if( DEBUG2 )
    fprintf(stderr, "  NrcDaySetMakeCalendarCycle: starts on %s, ends on %s\n",
      NrcDaySetLongName(NrcDayDayOfWeek(HaArrayFirst(res->days))),
      NrcDaySetLongName(NrcDayDayOfWeek(HaArrayLast(res->days))));
  return res;
#else
  HnAbort("NrcDaySetMakeCalendarCycle called when USE_MKTIME is 0");
  return NULL;  /* keep compiler happy */
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "query"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDaySetInstance(NRC_DAY_SET ds)                           */
/*                                                                           */
/*  Return the instance containing ds.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDaySetInstance(NRC_DAY_SET ds)
{
  return ds->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDaySetShortName(NRC_DAY_SET ds)                                 */
/*                                                                           */
/*  Return the short name of day_set.                                        */
/*                                                                           */
/*****************************************************************************/

char *NrcDaySetShortName(NRC_DAY_SET ds)
{
  return ds->short_name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDaySetLongName(NRC_DAY_SET ds)                                  */
/*                                                                           */
/*  Return the long name of ds.                                              */
/*                                                                           */
/*****************************************************************************/

char *NrcDaySetLongName(NRC_DAY_SET ds)
{
  return ds->long_name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDaySetDayCount(NRC_DAY_SET ds)                                    */
/*                                                                           */
/*  Return the number of days in ds.                                         */
/*                                                                           */
/*****************************************************************************/

int NrcDaySetDayCount(NRC_DAY_SET ds)
{
  return HaArrayCount(ds->days);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcDaySetDay(NRC_DAY_SET ds, int i)                              */
/*                                                                           */
/*  Return the i'th day of ds.                                               */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcDaySetDay(NRC_DAY_SET ds, int i)
{
  HnAssert(i >= 0 && i < HaArrayCount(ds->days),
    "DaySetDay: i (%d) out of range (0..%d)", i, HaArrayCount(ds->days));
  return HaArray(ds->days, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDayNameNormalize(char *ymd, char buff[50])                       */
/*                                                                           */
/*  Normalize ymd into buff, and return true if successful.                  */
/*                                                                           */
/*****************************************************************************/

static bool NrcDayNameNormalize(char *ymd, char buff[50])
{
  int year, month, day;
  if( sscanf(ymd, "%d-%d-%d", &year, &month, &day) != 3 )
    return false;
  sprintf(buff, "%04d-%02d-%02d", year, month, day);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetContainsDay(NRC_DAY_SET ds, NRC_DAY d)                     */
/*                                                                           */
/*  Return true if ds contains d.                                            */
/*                                                                           */
/*****************************************************************************/

bool NrcDaySetContainsDay(NRC_DAY_SET ds, NRC_DAY d)
{
  int pos;
  return HaArrayContains(ds->days, d, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetRetrieveDay(NRC_DAY_SET ds, char *ymd, NRC_DAY *d)         */
/*                                                                           */
/*  Retrieve the day with this ymd, or return false if none.                 */
/*                                                                           */
/*****************************************************************************/

bool NrcDaySetRetrieveDay(NRC_DAY_SET ds, char *ymd, NRC_DAY *d)
{
  NRC_DAY d2;  int i;  char buff[50];

  /* normalize ymd */
  if( !NrcDayNameNormalize(ymd, buff) )
    return false;

  /* retrieve ymd, or rather buff */
  HaArrayForEach(ds->days, d2, i)
    if( strcmp(buff, NrcDayYMD(d2)) == 0 )
    {
      *d = d2;
      return true;
    }
  *d = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDaySetsOverlap(NRC_DAY_SET ds1, NRC_DAY_SET ds2)                 */
/*                                                                           */
/*  Return true when ds1 and ds2 have a non-empty intersection.              */
/*                                                                           */
/*****************************************************************************/

bool NrcDaySetsOverlap(NRC_DAY_SET ds1, NRC_DAY_SET ds2)
{
  NRC_DAY d;  int i;
  HaArrayForEach(ds1->days, d, i)
    if( NrcDaySetContainsDay(ds2, d) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcNameDifference(char *name1, char *name2, HA_ARENA a)            */
/*                                                                           */
/*  Return "name1-name2".                                                    */
/*                                                                           */
/*****************************************************************************/

static char *NrcNameDifference(char *name1, char *name2, HA_ARENA a)
{
  return HnStringMake(a, "%s-%s", name1 == NULL ? "(null)" : name1,
    name2 == NULL ? "(null)" : name2);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySetDifference(NRC_DAY_SET ds1, NRC_DAY_SET ds2)        */
/*                                                                           */
/*  Return the set difference ds1 - ds2.                                     */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySetDifference(NRC_DAY_SET ds1, NRC_DAY_SET ds2)
{
  NRC_DAY_SET res;  NRC_DAY d;  int i;  char *short_name, *long_name;
  HA_ARENA a = NrcInstanceArena(ds1->instance);
  short_name = NrcNameDifference(ds1->short_name, ds2->short_name, a);
  long_name = NrcNameDifference(ds1->long_name, ds2->long_name, a);
  res = NrcDaySetMake(ds1->instance, short_name, long_name);
  HaArrayForEach(ds1->days, d, i)
    if( !NrcDaySetContainsDay(ds2, d) )
      NrcDaySetAddDay(res, d);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDaySetShiftSet(NRC_DAY_SET ds)                          */
/*                                                                           */
/*  Return the shift-set containing all this shifts of the days of ds.       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDaySetShiftSet(NRC_DAY_SET ds)
{
  int i, j;  NRC_DAY d;  NRC_SHIFT_SET ss;
  if( ds->all_ss == NULL )
  {
    ds->all_ss = NrcShiftSetMake(ds->instance);
    HaArrayForEach(ds->days, d, i)
    {
      ss = NrcDayShiftSet(d);
      for( j = 0;  j < NrcShiftSetShiftCount(ss);  j++ )
        NrcShiftSetAddShift(ds->all_ss, NrcShiftSetShift(ss, j));
    }
  }
  return ds->all_ss;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDaySetStartingShiftSet(NRC_DAY_SET ds)                  */
/*                                                                           */
/*  Return the starting shift-set of ds.                                     */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDaySetStartingShiftSet(NRC_DAY_SET ds)
{
  int i;  NRC_DAY d;  NRC_SHIFT_SET ss;
  if( ds->starting_ss == NULL )
  {
    ds->starting_ss = NrcShiftSetMake(ds->instance);
    HaArrayForEach(ds->days, d, i)
    {
      ss = NrcDayShiftSet(d);
      HnAssert(NrcShiftSetShiftCount(ss) != 0,
	"NrcDaySetStartingShiftSet: the shift set of the %d'th day is empty",i);
      NrcShiftSetAddShift(ds->starting_ss, NrcShiftSetShift(ss, 0));
    }
  }
  return ds->starting_ss;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcDaySetShiftSetSet(NRC_DAY_SET ds)                   */
/*                                                                           */
/*  Return the shift-set set with one shift-set for each day of ds.          */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcDaySetShiftSetSet(NRC_DAY_SET ds)
{
  int i;  NRC_DAY d;  NRC_SHIFT_SET ss;
  if( ds->days_sss == NULL )
  {
    ds->days_sss = NrcShiftSetSetMake(ds->instance);
    HaArrayForEach(ds->days, d, i)
    {
      ss = NrcDayShiftSet(d);
      NrcShiftSetSetAddShiftSet(ds->days_sss, ss);
    }
  }
  return ds->days_sss;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDaySetShiftSetFromShiftType(NRC_DAY_SET ds,             */
/*    NRC_SHIFT_TYPE st)                                                     */
/*                                                                           */
/*  Return the set of all shifts on the days of ds with shift type st.       */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDaySetShiftSetFromShiftType(NRC_DAY_SET ds, NRC_SHIFT_TYPE st)
{
  NRC_SHIFT_SET res;  NRC_DAY d;  int i;
  if( HaArrayCount(ds->days) == 1 )
    res = NrcShiftSingletonShiftSet(
      NrcDayShiftFromShiftType(HaArrayFirst(ds->days), st));
  else
  {
    res = NrcShiftSetMake(ds->instance);
    HaArrayForEach(ds->days, d, i)
      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDaySetShiftSetFromShiftTypeSet(NRC_DAY_SET ds,          */
/*    NRC_SHIFT_TYPE_SET sts)                                                */
/*                                                                           */
/*  Return the set of all shifts on the days of ds with types from sts.      */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDaySetShiftSetFromShiftTypeSet(NRC_DAY_SET ds,
  NRC_SHIFT_TYPE_SET sts)
{
  NRC_SHIFT_SET res;  NRC_DAY d;  int i;
  if( HaArrayCount(ds->days) == 1 )
    res = NrcDayShiftSetFromShiftTypeSet(HaArrayFirst(ds->days), sts);
  else
  {
    res = NrcShiftSetMake(ds->instance);
    HaArrayForEach(ds->days, d, i)
      NrcShiftSetAddShiftSet(res, NrcDayShiftSetFromShiftTypeSet(d, sts));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void DaySetDebug(DAY_SET day_set, int indent, FILE *fp)                  */
/*                                                                           */
/*  Generate a debug print of ds onto fp with the given indent.              */
/*                                                                           */
/*****************************************************************************/

void NrcDaySetDebug(NRC_DAY_SET ds, int indent, FILE *fp)
{
  NRC_DAY d;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ DaySet(short_name %s, long_name %s, %d days)\n", indent,
      "", ds->short_name, ds->long_name, HaArrayCount(ds->days));
    HaArrayForEach(ds->days, d, i)
      NrcDayDebug(d, indent + 4, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "{");
    HaArrayForEach(ds->days, d, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      NrcDayDebug(d, -1, fp);
    }
    fprintf(fp, "}");
  }
}

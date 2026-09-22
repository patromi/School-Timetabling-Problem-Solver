
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
/*  FILE:         day.c                                                      */
/*  MODULE:       One day of the cycle                                       */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#include <time.h>

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_INTERVAL_SHIFT_SET - a (time_interval, shift_set) pair          */
/*                                                                           */
/*****************************************************************************/

typedef struct nrc_time_interval_shift_set_rec {
  NRC_TIME_INTERVAL	time_interval;
  NRC_SHIFT_SET		shift_set;
} *NRC_TIME_INTERVAL_SHIFT_SET;

typedef HA_ARRAY(NRC_TIME_INTERVAL_SHIFT_SET) ARRAY_NRC_TIME_INTERVAL_SHIFT_SET;


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY                                                                  */
/*                                                                           */
/*  One day of the cycle, represented in source files by YYYY-MM-DD string.  */
/*                                                                           */
/*****************************************************************************/

struct nrc_day_rec {
  NRC_INSTANCE		instance;		/* enclosing instance      */
  char			*ymd;			/* "YYYY-MM-DD"            */
  char			*short_name;		/* day name eg "2Tue"      */
  char			*long_name;		/* day name eg "2Tuesday"  */
  int			index_in_cycle; 	/* 0 is the start doc      */
  int			week_in_cycle;		/* week number             */
  int			index_in_week;	 	/* index in week           */
  NRC_DAY_SET		day_of_week;		/* day of the week         */
  NRC_DAY_SET		singleton_ds;		/* singleton day set       */
  NRC_SHIFT_SET		shift_set;		/* shifts on this day      */
  NRC_SHIFT_SET_SET	shift_set_set;		/* shifts on this day      */
  ARRAY_NRC_TIME_INTERVAL_SHIFT_SET time_interval_shift_sets;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcDayMake(NRC_INSTANCE ins, char *ymd, int index_in_cycle,      */
/*    int week_in_cycle, NRC_DAY_SET day_of_week)                            */
/*                                                                           */
/*  Make a new DAY object.                                                   */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcDayMake(NRC_INSTANCE ins, char *ymd, int index_in_cycle,
  int week_in_cycle, int index_in_week, NRC_DAY_SET day_of_week)
{
  NRC_DAY res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->ymd = ymd;
  res->short_name = HnStringMake(a, "%d%s", week_in_cycle,
    NrcDaySetShortName(day_of_week));
  res->long_name = HnStringMake(a, "%d%s", week_in_cycle,
    NrcDaySetLongName(day_of_week));
  res->index_in_cycle = index_in_cycle;
  res->week_in_cycle = week_in_cycle;
  res->index_in_week = index_in_week;
  res->day_of_week = day_of_week;
  res->singleton_ds = NULL;
  res->shift_set = NrcShiftSetMakeInternal(ins, KHE_TIME_GROUP_KIND_DAY);
  res->shift_set_set = NULL;   /* created on demand */
  HaArrayInit(res->time_interval_shift_sets, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDayAddShift(NRC_DAY d, NRC_SHIFT s)                              */
/*                                                                           */
/*  Add s to day.                                                            */
/*                                                                           */
/*****************************************************************************/

void NrcDayAddShift(NRC_DAY d, NRC_SHIFT s)
{
  NrcShiftSetAddShift(d->shift_set, s);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcDayInstance(NRC_DAY d)                                   */
/*                                                                           */
/*  Return the instance attribute of d.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcDayInstance(NRC_DAY d)
{
  return d->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDayYMD(NRC_DAY d)                                               */
/*                                                                           */
/*  Return the YMD string of day d.                                          */
/*                                                                           */
/*****************************************************************************/

char *NrcDayYMD(NRC_DAY d)
{
  return d->ymd;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDayShortName(NRC_DAY d)                                         */
/*                                                                           */
/*  Return the short name of day d:  "2Tue" or whatever.                     */
/*                                                                           */
/*****************************************************************************/

char *NrcDayShortName(NRC_DAY d)
{
  return d->short_name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcDayLongName(NRC_DAY d)                                          */
/*                                                                           */
/*  Return the long name of day d:  "2Tuesday" or whatever.                  */
/*                                                                           */
/*****************************************************************************/

char *NrcDayLongName(NRC_DAY d)
{
  return d->long_name;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDayIndexInCycle(NRC_DAY d)                                        */
/*                                                                           */
/*  Return the index of day d in the cycle.                                  */
/*                                                                           */
/*****************************************************************************/

int NrcDayIndexInCycle(NRC_DAY d)
{
  return d->index_in_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDayWeekInCycle(NRC_DAY d)                                         */
/*                                                                           */
/*  Return the week number of day d in the cycle.                            */
/*                                                                           */
/*****************************************************************************/

int NrcDayWeekInCycle(NRC_DAY d)
{
  return d->week_in_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDayIndexInWeek(NRC_DAY d)                                         */
/*                                                                           */
/*  Return the index of day d in the week (0 to 6).                          */
/*                                                                           */
/*****************************************************************************/

int NrcDayIndexInWeek(NRC_DAY d)
{
  return d->index_in_week;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcDayPrev(NRC_DAY d)                                            */
/*                                                                           */
/*  Return the day preceding d in the cycle, or NULL if d is the first day.  */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcDayPrev(NRC_DAY d)
{
  return (d->index_in_cycle == 0 ?
    NULL : NrcInstanceCycleDay(d->instance, d->index_in_cycle - 1));
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY NrcDayNext(NRC_DAY d)                                            */
/*                                                                           */
/*  Return the day following d in the cycle, or NULL if d is the last day.   */
/*                                                                           */
/*****************************************************************************/

NRC_DAY NrcDayNext(NRC_DAY d)
{
  return (d->index_in_cycle == NrcInstanceCycleDayCount(d->instance) - 1 ?
    NULL : NrcInstanceCycleDay(d->instance, d->index_in_cycle + 1));
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDayDayOfWeek(NRC_DAY d)                                   */
/*                                                                           */
/*  Return the day of the week of day d.                                     */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDayDayOfWeek(NRC_DAY d)
{
  return d->day_of_week;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_DAY_SET NrcDaySingletonDaySet(NRC_DAY d)                             */
/*                                                                           */
/*  Return a day set containing just d.                                      */
/*                                                                           */
/*****************************************************************************/

NRC_DAY_SET NrcDaySingletonDaySet(NRC_DAY d)
{
  if( d->singleton_ds == NULL )
  {
    d->singleton_ds = NrcDaySetMake(d->instance, d->short_name, d->long_name);
    NrcDaySetAddDay(d->singleton_ds, d);
  }
  return d->singleton_ds;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDayShiftSet(NRC_DAY d)                                  */
/*                                                                           */
/*  Return a shift-set holding the shifts of day d.                          */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDayShiftSet(NRC_DAY d)
{
  return d->shift_set;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcDayShiftCount(NRC_DAY d)                                          */
/*                                                                           */
/*  Return the number of shifts on day d.                                    */
/*                                                                           */
/*****************************************************************************/

int NrcDayShiftCount(NRC_DAY d)
{
  return NrcShiftSetShiftCount(d->shift_set);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT NrcDayShift(NRC_DAY d, int i)                                  */
/*                                                                           */
/*  Return the i'th shift of day d.                                          */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT NrcDayShift(NRC_DAY d, int i)
{
  return NrcShiftSetShift(d->shift_set, i);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT NrcDayShiftFromShiftType(NRC_DAY d, NRC_SHIFT_TYPE st)         */
/*                                                                           */
/*  Return the shift with this day and shift type.                           */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT NrcDayShiftFromShiftType(NRC_DAY d, NRC_SHIFT_TYPE st)
{
  return NrcShiftSetShift(d->shift_set, NrcShiftTypeIndex(st));
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcDayShiftSetFromShiftTypeSet(NRC_DAY d,                  */
/*    NRC_SHIFT_TYPE_SET sts)                                                */
/*                                                                           */
/*  Return the shifts with this day and these shift types.                   */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcDayShiftSetFromShiftTypeSet(NRC_DAY d, NRC_SHIFT_TYPE_SET sts)
{
  NRC_SHIFT_SET res;  NRC_SHIFT_TYPE st;  int i;
  res = NrcShiftSetMake(d->instance);
  for( i = 0;  i < NrcShiftTypeSetShiftTypeCount(sts);  i++ )
  {
    st = NrcShiftTypeSetShiftType(sts, i);
    NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d, st));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET_SET NrcDayShiftSetSet(NRC_DAY d)                           */
/*                                                                           */
/*  Return a shift-set set containing one shift-set for each shift on        */
/*  day d, holding that shift.                                               */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET_SET NrcDayShiftSetSet(NRC_DAY d)
{
  int i;  NRC_SHIFT s;
  if( d->shift_set_set == NULL )
  {
    d->shift_set_set = NrcShiftSetSetMake(d->instance);
    for( i = 0;  i < NrcShiftSetShiftCount(d->shift_set);  i++ )
    {
      s = NrcShiftSetShift(d->shift_set, i);
      NrcShiftSetSetAddShiftSet(d->shift_set_set, NrcShiftSingletonShiftSet(s));
    }
  }
  return d->shift_set_set;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDayAddTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti,      */
/*    NRC_SHIFT_SET ss)                                                      */
/*                                                                           */
/*  Add (ti, ss) to d's cache of these pairs.                                */
/*                                                                           */
/*****************************************************************************/

void NrcDayAddTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti,
  NRC_SHIFT_SET ss)
{
  NRC_TIME_INTERVAL_SHIFT_SET tiss;  HA_ARENA a;
  a = NrcInstanceArena(d->instance);
  HaMake(tiss, a);
  tiss->time_interval = ti;
  tiss->shift_set = ss;
  HaArrayAddLast(d->time_interval_shift_sets, tiss);
  if( DEBUG1 )
    fprintf(stderr, "  NrcDayAddTimeIntervalShiftSet(%s, %s, ss) count %d\n",
      NrcDayShortName(d), NrcTimeIntervalShow(ti, a),
      HaArrayCount(d->time_interval_shift_sets));
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDayRetrieveTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti, */
/*    NRC_SHIFT_SET *ss)                                                     */
/*                                                                           */
/*  Retrieve a (ti, ss) with the given ti from d's cache.                    */
/*                                                                           */
/*****************************************************************************/

bool NrcDayRetrieveTimeIntervalShiftSet(NRC_DAY d, NRC_TIME_INTERVAL ti,
  NRC_SHIFT_SET *ss)
{
  NRC_TIME_INTERVAL_SHIFT_SET tiss;  int i;
  HaArrayForEach(d->time_interval_shift_sets, tiss, i)
    if( NrcTimeIntervalEqual(tiss->time_interval, ti) )
    {
      if( DEBUG1 )
      {
	HA_ARENA a = NrcInstanceArena(d->instance);
	fprintf(stderr, "  NrcDayRetrieveTimeIntervalShiftSet(%s, %s) true\n",
	  NrcDayShortName(d), NrcTimeIntervalShow(ti, a));
      }
      return *ss = tiss->shift_set, true;
    }
  if( DEBUG1 )
  {
    HA_ARENA a = NrcInstanceArena(d->instance);
    fprintf(stderr, "  NrcDayRetrieveTimeIntervalShiftSet(%s, %s) false\n",
      NrcDayShortName(d), NrcTimeIntervalShow(ti, a));
  }
  return *ss = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcDayDebug(NRC_DAY d, int indent, FILE *fp)                        */
/*                                                                           */
/*  Debug print of day d onto fp with the given indent.                      */
/*                                                                           */
/*****************************************************************************/

void NrcDayDebug(NRC_DAY d, int indent, FILE *fp)
{
  if( indent < 0 )
    fprintf(fp, "%s", d->short_name);
  else
    fprintf(fp,"%*sDay %s (ymd %s, week %d, day %s, %s)\n",
      indent, "", d->short_name, d->ymd, d->week_in_cycle,
      NrcDaySetShortName(d->day_of_week), d->long_name);
}

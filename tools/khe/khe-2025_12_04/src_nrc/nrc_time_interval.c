
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
/*  FILE:         nrc_time_interval.c                                        */
/*  MODULE:       A time interval relative to one day                        */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"
#define MIDNIGHT (24 * 60 * 60)

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_INTERVAL                                                        */
/*                                                                           */
/*  A time interval.                                                         */
/*                                                                           */
/*****************************************************************************/

struct nrc_time_interval_rec {
  int		start_secs;
  int		end_secs;
};


/*****************************************************************************/
/*                                                                           */
/*  bool NrcHMSToSecs(char *hms, int *res)                                   */
/*                                                                           */
/*  Convert hms to secs.                                                     */
/*                                                                           */
/*****************************************************************************/

bool NrcHMSToSecs(char *hms, int *res)
{
  int hrs, mins, secs;
  if( sscanf(hms, "%d:%d:%d", &hrs, &mins, &secs) == 3 )
    return *res = hrs * 60 * 60 + mins * 60 + secs, true;
  else if( sscanf(hms, "%d:%d", &hrs, &mins) == 2 )
    return *res = hrs * 60 * 60 + mins * 60, true;
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcSecsToHMS(int secs)                                             */
/*                                                                           */
/*  Convert secs to HMS.                                                     */
/*                                                                           */
/*****************************************************************************/

char *NrcSecsToHMS(int secs, HA_ARENA a)
{
  int hrs, mins;
  hrs = secs / (60 * 60);
  secs = secs % (60 * 60);
  mins = secs / 60;
  secs = secs % 60;
  return HnStringMake(a, "%02d:%02d:%02d", hrs, mins, secs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeIntervalLegal(int start_secs, int end_secs)                  */
/*                                                                           */
/*  Return true if (start_secs, end_secs) is a legal time interval.          */
/*                                                                           */
/*****************************************************************************/

static bool NrcTimeIntervalLegal(int start_secs, int end_secs)
{
  return 0 <= start_secs && start_secs < MIDNIGHT &&
         0 < end_secs && end_secs <= MIDNIGHT;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_TIME_INTERVAL NrcTimeIntervalMake(int start_secs, int end_secs)      */
/*                                                                           */
/*  Make a time interval with this start and end time.                       */
/*                                                                           */
/*****************************************************************************/

NRC_TIME_INTERVAL NrcTimeIntervalMake(int start_secs, int end_secs,
  NRC_INSTANCE ins)
{
  NRC_TIME_INTERVAL res;  HA_ARENA a;
  HnAssert(0 <= start_secs && start_secs < MIDNIGHT,
    "NrcTimeIntervalMake: start_secs (%d) out of range (0 .. %d)",
    start_secs, MIDNIGHT - 1);
  HnAssert(0 < end_secs && end_secs <= MIDNIGHT,
    "NrcTimeIntervalMake: end_secs (%d) out of range (%d .. %d)",
    end_secs, 1, MIDNIGHT);
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->start_secs = start_secs;
  res->end_secs = end_secs;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeIntervalMakeFromHMS(char *start_hms, char *end_hms,          */
/*    NRC_TIME_INTERVAL *res, NRC_INSTANCE ins)                              */
/*                                                                           */
/*  Make time interval *res from start_hms and end_hms, returning true       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

bool NrcTimeIntervalMakeFromHMS(char *start_hms, char *end_hms,
  NRC_TIME_INTERVAL *res, NRC_INSTANCE ins)
{
  int start_secs, end_secs;
  if( NrcHMSToSecs(start_hms, &start_secs) && NrcHMSToSecs(end_hms, &end_secs) )
  {
    if( end_secs == 0 )  end_secs = MIDNIGHT;
    if( NrcTimeIntervalLegal(start_secs, end_secs) )
      return *res = NrcTimeIntervalMake(start_secs, end_secs, ins), true;
    else
      return *res = NULL, false;
  }
  else
    return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcTimeIntervalStartSecs(NRC_TIME_INTERVAL ti)                       */
/*                                                                           */
/*  Return the start time of ti.                                             */
/*                                                                           */
/*****************************************************************************/

int NrcTimeIntervalStartSecs(NRC_TIME_INTERVAL ti)
{
  return ti->start_secs;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcTimeIntervalEndSecs(NRC_TIME_INTERVAL ti)                         */
/*                                                                           */
/*  Return the end time of ti.                                               */
/*                                                                           */
/*****************************************************************************/

int NrcTimeIntervalEndSecs(NRC_TIME_INTERVAL ti)
{
  return ti->end_secs;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeIntervalEqual(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2)  */
/*                                                                           */
/*  Return true when ti and t2 are equal (have equal endpoints).             */
/*                                                                           */
/*****************************************************************************/

bool NrcTimeIntervalEqual(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2)
{
  return ti1->start_secs == ti2->start_secs && ti1->end_secs == ti2->end_secs;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcDisjoint(int s1, int e1, int s2, int e2)                         */
/*                                                                           */
/*  Return true when open intervals (s1, e1) and (s2, e2) are disjoint.      */
/*                                                                           */
/*****************************************************************************/

static bool NrcDisjoint(int s1, int e1, int s2, int e2)
{
  return s1 >= e2 || s2 >= e1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeIntervalDisjoint(NRC_TIME_INTERVAL ti1,                      */
/*    NRC_TIME_INTERVAL ti2)                                                 */
/*                                                                           */
/*  Return true if ti1 and ti2 are disjoint (when taken as open intervals).  */
/*                                                                           */
/*****************************************************************************/

bool NrcTimeIntervalDisjoint(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2)
{
  bool res;
  if( ti1->start_secs <= ti1->end_secs )
  {
    /* ti1 is an ordinary interval: (start, end) */
    if( ti2->start_secs <= ti2->end_secs )
    {
      /* ti2 is an ordinary interval: (start, end) */
      res = NrcDisjoint(ti1->start_secs, ti1->end_secs,
	ti2->start_secs, ti2->end_secs);
    }
    else
    {
      /* ti2 is two intervals: (start, midnight) and (midnight, end) */
      res = NrcDisjoint(ti1->start_secs, ti1->end_secs,ti2->start_secs,MIDNIGHT)
       	&& NrcDisjoint(ti1->start_secs, ti1->end_secs, 0, ti2->end_secs);
    }
  }
  else
  {
    /* ti1 is two intervals: (start, midnight) and (midnight, end) */
    if( ti2->start_secs <= ti2->end_secs )
    {
      /* ti2 is an ordinary interval: (start, end) */
      res = NrcDisjoint(ti1->start_secs, MIDNIGHT,ti2->start_secs,ti2->end_secs)
       	&& NrcDisjoint(0, ti1->end_secs, ti2->start_secs, ti2->end_secs);
    }
    else
    {
      /* ti2 is two intervals: (start, midnight) and (midnight, end) */
      res = NrcDisjoint(ti1->start_secs, MIDNIGHT, ti2->start_secs, MIDNIGHT)
       	&& NrcDisjoint(ti1->start_secs, MIDNIGHT, 0, ti2->end_secs) &&
           NrcDisjoint(0, ti1->end_secs, ti2->start_secs, MIDNIGHT) &&
           NrcDisjoint(0, ti1->end_secs, 0, ti2->end_secs);
    }
  }
  /* *** no arena to print this into now!
  if( DEBUG1 )
    fprintf(stderr, "  NrcTimeIntervalDisjoint(%s, %s) = %s\n",
      NrcTimeIntervalShow(ti1), NrcTimeIntervalShow(ti2),
      res ? "true" : "false");
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcSubset(int s1, int e1, int s2, int e2)                           */
/*                                                                           */
/*  Return true when open interval (s1, e1) is a subset of (s2, e2).         */
/*                                                                           */
/*****************************************************************************/

static bool NrcSubset(int s1, int e1, int s2, int e2)
{
  return s1 >= s2 && e1 <= e2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeIntervalSubset(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2) */
/*                                                                           */
/*  Return true when ti1 is a subset of ti2.                                 */
/*                                                                           */
/*****************************************************************************/

bool NrcTimeIntervalSubset(NRC_TIME_INTERVAL ti1, NRC_TIME_INTERVAL ti2)
{
  bool res;
  if( ti1->start_secs <= ti1->end_secs )
  {
    /* ti1 is an ordinary interval: (start, end) */
    if( ti2->start_secs <= ti2->end_secs )
    {
      /* ti2 is an ordinary interval: (start, end) */
      res = NrcSubset(ti1->start_secs, ti1->end_secs,
	ti2->start_secs, ti2->end_secs);
    }
    else
    {
      /* ti2 is two intervals: (start, midnight) and (midnight, end) */
      res = NrcSubset(ti1->start_secs, ti1->end_secs, ti2->start_secs, MIDNIGHT)
       	|| NrcSubset(ti1->start_secs, ti1->end_secs, 0, ti2->end_secs);
    }
  }
  else
  {
    /* ti1 is two intervals: (start, midnight) and (midnight, end) */
    if( ti2->start_secs <= ti2->end_secs )
    {
      /* ti2 is an ordinary interval: (start, end) */
      /* actually this will always return false */
      res = NrcSubset(ti1->start_secs, MIDNIGHT, ti2->start_secs, ti2->end_secs)
       	&& NrcSubset(0, ti1->end_secs, ti2->start_secs, ti2->end_secs);
    }
    else
    {
      /* ti2 is two intervals: (start, midnight) and (midnight, end) */
      res = NrcSubset(ti1->start_secs, MIDNIGHT, ti2->start_secs, MIDNIGHT)
       	&& NrcSubset(0, ti1->end_secs, 0, ti2->end_secs);
    }
  }
  /* *** no arena to print this into now!
  if( DEBUG1 )
    fprintf(stderr, "  NrcTimeIntervalSubset(%s, %s) = %s\n",
      NrcTimeIntervalShow(ti1), NrcTimeIntervalShow(ti2),
      res ? "true" : "false");
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcTimeIntervalStandardize(NRC_TIME_INTERVAL ti, int offset,        */
/*    int *res_start_secs, int *res_end_secs)                                */
/*                                                                           */
/*  Get ti into standard form, assuming its day begins offset seconds after  */
/*  the starting day.  Put the result in (*res_start_secs, *res_end_secs).   */
/*                                                                           */
/*****************************************************************************/
#define ONE_DAY (24 * 60 * 60)

static void NrcTimeIntervalStandardize(NRC_TIME_INTERVAL ti, int offset,
  int *res_start_secs, int *res_end_secs)
{
    *res_start_secs = ti->start_secs + offset;
    *res_end_secs = ti->end_secs + offset;
    if( *res_end_secs < *res_start_secs )
      *res_end_secs += ONE_DAY;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcTimeIntervalShiftSet(NRC_TIME_INTERVAL ti, NRC_DAY d)   */
/*                                                                           */
/*  Return the set of shifts that are running during time interval ti,       */
/*  which is here taken to start on day d and end either on that day or the  */
/*  next day.  The result could include shifts that begin on the day before  */
/*  d (if any), on d itself, and on the day after d (if any).                */
/*                                                                           */
/*  Implementation note 1.  The result is cached in d, so the first step is  */
/*  to try to retrieve it from there.                                        */
/*                                                                           */
/*  Implementation note 2.  The code structure here is "for each of the      */
/*  three possible days, for each shift type on that day, if the shift of    */
/*  that type on that day overlaps ti, add it to the result".  Intervals     */
/*  are converted to a standard form, with start and end points measured     */
/*  from the start of the day before d, whether or not that day exists.      */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcTimeIntervalShiftSet(NRC_TIME_INTERVAL ti, NRC_DAY d)
{
  NRC_SHIFT_SET res;
  int di, i, j, ti_start, ti_end, st_ti_start, st_ti_end, offs;  NRC_DAY d2;
  NRC_INSTANCE ins;  NRC_TIME_INTERVAL st_ti;  NRC_SHIFT_TYPE st;
  if( !NrcDayRetrieveTimeIntervalShiftSet(d, ti, &res) )
  {
    ins = NrcDayInstance(d);
    if( DEBUG2 )
    {
      HA_ARENA a = NrcInstanceArena(ins);
      fprintf(stderr, "[ NrcTimeIntervalShiftSet(%s, %s)\n",
	NrcTimeIntervalShow(ti, a), NrcDayShortName(d));
    }
    NrcTimeIntervalStandardize(ti, ONE_DAY, &ti_start, &ti_end);
    res = NrcShiftSetMake(ins);
    di = NrcDayIndexInCycle(d);
    for( i = di - 1, offs = 0;  i <= di + 1;  i++, offs += ONE_DAY )
    {
      if( i >= 0 && i < NrcInstanceCycleDayCount(ins) )
      {
	/* d is a possible day */
	d2 = NrcInstanceCycleDay(ins, i);
	for( j = 0;  j < NrcInstanceShiftTypeCount(ins);  j++ )
	{
	  st = NrcInstanceShiftType(ins, j);
	  st_ti = NrcShiftTypeTimeInterval(st);
	  if( st_ti != NULL )
	  {
            NrcTimeIntervalStandardize(st_ti, offs, &st_ti_start, &st_ti_end);
	    if( !NrcDisjoint(ti_start, ti_end, st_ti_start, st_ti_end) )
	      NrcShiftSetAddShift(res, NrcDayShiftFromShiftType(d2, st));
	  }
	}
      }
    }
    if( DEBUG2 )
    {
      NrcShiftSetDebug(res, 2, stderr);
      fprintf(stderr, "] NrcTimeIntervalShiftSet returning\n");
    }
    NrcDayAddTimeIntervalShiftSet(d, ti, res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcTimeIntervalShow(NRC_TIME_INTERVAL ti, HA_ARENA a)              */
/*                                                                           */
/*  Return a string representation of ti in heap memory.                     */
/*                                                                           */
/*****************************************************************************/

char *NrcTimeIntervalShow(NRC_TIME_INTERVAL ti, HA_ARENA a)
{
  return HnStringMake(a, "%s--%s", NrcSecsToHMS(ti->start_secs, a),
    NrcSecsToHMS(ti->end_secs, a));
}

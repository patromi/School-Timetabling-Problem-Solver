
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
/*  FILE:         nrc_shift_set.c                                            */
/*  MODULE:       A shift-set                                                */
/*                                                                           */
/*****************************************************************************/
#include "nrc_interns.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET                                                            */
/*                                                                           */
/*  A shift-set                                                              */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(NRC_SHIFT) ARRAY_NRC_SHIFT;

struct nrc_shift_set_rec {
  NRC_INSTANCE			instance;
  KHE_TIME_GROUP_KIND		kind;
  ARRAY_NRC_SHIFT		shifts;
  KHE_TIME_GROUP		time_group;
  KHE_EVENT_GROUP		event_group;
};


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftSetMakeInternal(NRC_INSTANCE ins,                  */
/*    KHE_TIME_GROUP_KIND kind)                                              */
/*                                                                           */
/*  Make a new shift-set with the given kind and initially no shifts.        */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftSetMakeInternal(NRC_INSTANCE ins,
  KHE_TIME_GROUP_KIND kind)
{
  NRC_SHIFT_SET res;  HA_ARENA a;
  a = NrcInstanceArena(ins);
  HaMake(res, a);
  res->instance = ins;
  res->kind = kind;
  HaArrayInit(res->shifts, a);
  res->time_group = NULL;  /* added as required */
  res->event_group = NULL;  /* added as required */
  NrcInstanceAddShiftSet(ins, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftSetMakeShifted(NRC_SHIFT_SET ss, int delta,                 */
/*    NRC_SHIFT_SET *res)                                                    */
/*                                                                           */
/*  If ss can be shifted by delta, return true and set *res to the           */
/*  shifted shift set.  Else return false.                                   */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftSetMakeShifted(NRC_SHIFT_SET ss, int delta, NRC_SHIFT_SET *res)
{
  int min_index, max_index, index, i, count;  NRC_SHIFT s;  NRC_INSTANCE ins;

  /* empty shift set can be shifted, and a delta of 0 is always OK */
  if( HaArrayCount(ss->shifts) == 0 || delta == 0 )
    return *res = ss, true;

  /* find min_index and max_index */
  min_index = max_index = NrcShiftIndex(HaArrayFirst(ss->shifts));
  for( i = 1;  i < HaArrayCount(ss->shifts);  i++ )
  {
    s = HaArray(ss->shifts, i);
    index = NrcShiftIndex(s);
    if( index < min_index )  min_index = index;
    if( index > max_index )  max_index = index;
  }

  /* return false if delta would go off either end */
  ins = ss->instance;
  count = NrcInstanceShiftCount(ins);
  if( min_index + delta < 0 || max_index + delta >= count )
    return *res = NULL, false;

  /* make and return the shifted shift set */
  *res = NrcShiftSetMake(ss->instance);
  HaArrayForEach(ss->shifts, s, i)
    NrcShiftSetAddShift(*res, NrcInstanceShift(ins, NrcShiftIndex(s) + delta));
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT_SET NrcShiftSetMake(NRC_INSTANCE ins)                          */
/*                                                                           */
/*  Make a new shift-set, initialy with no shifts.                           */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT_SET NrcShiftSetMake(NRC_INSTANCE ins)
{
  return NrcShiftSetMakeInternal(ins, KHE_TIME_GROUP_KIND_ORDINARY);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetAddShift(NRC_SHIFT_SET ss, NRC_SHIFT s)                  */
/*                                                                           */
/*  Add s to ss.                                                             */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetAddShift(NRC_SHIFT_SET ss, NRC_SHIFT s)
{
  HaArrayAddLast(ss->shifts, s);
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetAddShiftSet(NRC_SHIFT_SET ss, NRC_SHIFT_SET ss2)         */
/*                                                                           */
/*  Add the shifts of ss2 to ss.                                             */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetAddShiftSet(NRC_SHIFT_SET ss, NRC_SHIFT_SET ss2)
{
  NRC_SHIFT s;  int i;
  HaArrayForEach(ss2->shifts, s, i)
    HaArrayAddLast(ss->shifts, s);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_INSTANCE NrcShiftSetInstance(NRC_SHIFT_SET ss)                       */
/*                                                                           */
/*  Return the instance containing ss.                                       */
/*                                                                           */
/*****************************************************************************/

NRC_INSTANCE NrcShiftSetInstance(NRC_SHIFT_SET ss)
{
  return ss->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftSetShiftCount(NRC_SHIFT_SET ss)                              */
/*                                                                           */
/*  Return the number of shifts in ss.                                       */
/*                                                                           */
/*****************************************************************************/

int NrcShiftSetShiftCount(NRC_SHIFT_SET ss)
{
  return HaArrayCount(ss->shifts);
}


/*****************************************************************************/
/*                                                                           */
/*  NRC_SHIFT NrcShiftSetShift(NRC_SHIFT_SET ss, int i)                      */
/*                                                                           */
/*  Return the i'th shift of ss.                                             */
/*                                                                           */
/*****************************************************************************/

NRC_SHIFT NrcShiftSetShift(NRC_SHIFT_SET ss, int i)
{
  return HaArray(ss->shifts, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftSetContainsShift(NRC_SHIFT_SET ss, NRC_SHIFT s)             */
/*                                                                           */
/*  Return true when ss contains s.                                          */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftSetContainsShift(NRC_SHIFT_SET ss, NRC_SHIFT s)
{
  NRC_SHIFT s2;  int i;
  HaArrayForEach(ss->shifts, s2, i)
    if( s2 == s )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "uniformity"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftSetUniform(NRC_SHIFT_SET ss, int *offset)                   */
/*                                                                           */
/*  Return true if ss is uniform, that is, if its shifts have the same       */
/*  offsets.                                                                 */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftSetUniform(NRC_SHIFT_SET ss, int *offset)
{
  NRC_SHIFT s1, s2;  int i;
  HnAssert(HaArrayCount(ss->shifts) >= 2, "NrcShiftSetUniform: internal error");
  s1 = HaArray(ss->shifts, 0);
  s2 = HaArray(ss->shifts, 1);
  *offset = NrcShiftIndex(s2) - NrcShiftIndex(s1);
  for( i = 2;  i < HaArrayCount(ss->shifts);  i++ )
  {
    s1 = HaArray(ss->shifts, i-1);
    s2 = HaArray(ss->shifts, i);
    if( *offset != NrcShiftIndex(s2) - NrcShiftIndex(s1) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftSetsUniform(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2,           */
/*    int *offset)                                                           */
/*                                                                           */
/*  Return true if ss1 and ss2 are uniform and set *offset to the offset.    */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftSetsUniform(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2, int *offset)
{
  NRC_SHIFT s1, s2;  int i;

  /* must be the same non-zero number of shifts in each shift set */
  *offset = 0;
  if( HaArrayCount(ss1->shifts) != HaArrayCount(ss2->shifts) ||
      HaArrayCount(ss1->shifts) == 0 )
    return false;

  /* get the offset between the first shifts in each set */
  s1 = HaArrayFirst(ss1->shifts);
  s2 = HaArrayFirst(ss2->shifts);
  *offset = NrcShiftIndex(s2) - NrcShiftIndex(s1);

  /* make sure it's the same offset throughout */
  for( i = 1;  i < HaArrayCount(ss1->shifts);  i++ )
  {
    s1 = HaArray(ss1->shifts, i);
    s2 = HaArray(ss2->shifts, i);
    if( NrcShiftIndex(s2) - NrcShiftIndex(s1) != *offset )
      return false;
  }

  /* shift sets are uniform */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcShiftSetsEqual(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2)             */
/*                                                                           */
/*  Return true if ss1 and ss2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

bool NrcShiftSetsEqual(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2)
{
  NRC_SHIFT s1, s2;  int i;

  /* must be the same number of shifts in each shift set */
  if( HaArrayCount(ss1->shifts) != HaArrayCount(ss2->shifts) )
    return false;

  /* make sure the shifts are equal throughout */
  for( i = 0;  i < HaArrayCount(ss1->shifts);  i++ )
  {
    s1 = HaArray(ss1->shifts, i);
    s2 = HaArray(ss2->shifts, i);
    if( s1 != s2 )
      return false;
  }

  /* shift sets are equal */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  TIMESET - a set of times with a simple name                              */
/*                                                                           */
/*****************************************************************************/

typedef struct timeset_rec {
  int		first_week;
  int		last_week;
  int		first_dow;
  int		last_dow;
  HA_ARRAY_INT	intervals;
} *TIMESET;

typedef HA_ARRAY(TIMESET) ARRAY_TIMESET;


/*****************************************************************************/
/*                                                                           */
/*  void NrcAnalyseShift(NRC_SHIFT s, int *wk, int *dow, int *tod)           */
/*                                                                           */
/*  Analyse time_id into its week, day of week, and time of day elements.    */
/*                                                                           */
/*****************************************************************************/

static void NrcAnalyseShift(NRC_SHIFT s, int *wk, int *dow, int *tod)
{
  *wk = NrcDayWeekInCycle(NrcShiftDay(s));
  *dow = NrcDayIndexInWeek(NrcShiftDay(s));
  *tod = NrcShiftTypeIndex(NrcShiftType(s)) + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  TIMESET NrcTimeSetMake(int week, int dow, int first_time_on_dow,         */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a timeset with these attributes.                                    */
/*                                                                           */
/*****************************************************************************/

static TIMESET NrcTimeSetMake(int week, int dow, int first_time_on_dow,
  HA_ARENA a)
{
  TIMESET res;
  HnAssert(week >= 1, "NrcTimeSetMake: week (%d) out of range", week);
  HnAssert(0 <= dow && dow < 7, "NrcTimeSetMake: dow (%d) out of range", dow);
  HnAssert(first_time_on_dow >= 0,
    "NrcTimeSetMake: first_time_on_dow (%d) out of range", first_time_on_dow);
  HaMake(res, a);
  res->first_week = res->last_week = week;
  res->first_dow = res->last_dow = dow;
  HaArrayInit(res->intervals, a);
  HaArrayAddLast(res->intervals, first_time_on_dow);
  HaArrayAddLast(res->intervals, first_time_on_dow);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcTimeSetAddId(TIMESET ts, int times_per_dow, ARRAY_CHAR *ac)      */
/*                                                                           */
/*  Add a suitable name for ts to *ac.                                       */
/*                                                                           */
/*****************************************************************************/

static void NrcTimeSetAddId(NRC_INSTANCE ins, TIMESET ts, HA_ARRAY_NCHAR *ac)
{
  int i, ti_start, ti_end;

  /* add weeks */
  if( ts->first_week == ts->last_week )
    HnStringAdd(ac, "%d", ts->first_week);
  else
    HnStringAdd(ac, "%d-%d", ts->first_week, ts->last_week);

  /* add dows */
  if( ts->first_dow == ts->last_dow )
    HnStringAdd(ac, "%s", NrcInstanceShortDayName(ins, ts->first_dow));
  else
    HnStringAdd(ac, "%s-%s",
      NrcInstanceShortDayName(ins, ts->first_dow),
      NrcInstanceShortDayName(ins, ts->last_dow));

  /* add dow intervals */
  if( HaArrayCount(ts->intervals) == 2 &&
    HaArrayFirst(ts->intervals) == 1 &&
    HaArray(ts->intervals, 1) == NrcInstanceShiftTypeCount(ins) )
  {
    /* add nothing; this means the whole dow */
  }
  else
  {
    /* add the intervals, separated by commas */
    for( i = 0;  i < HaArrayCount(ts->intervals);  i += 2 )
    {
      if( i > 0 )
	HnStringAdd(ac, "%s", ",");
      ti_start = HaArray(ts->intervals, i);
      ti_end = HaArray(ts->intervals, i + 1);
      if( ti_start == ti_end )
	HnStringAdd(ac, "%d", ti_start);
      else
	HnStringAdd(ac, "%d-%d", ti_start, ti_end);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcTimeSetsId(ARRAY_TIMESET *timesets, int times_per_day)          */
/*                                                                           */
/*  Return a suitable name for this array of timesets.                       */
/*                                                                           */
/*****************************************************************************/

static char *NrcTimeSetsId(NRC_INSTANCE ins, ARRAY_TIMESET *timesets)
{
  TIMESET ts;  int i;  HA_ARRAY_NCHAR ac;
  HnStringBegin(ac, NrcInstanceArena(ins));
  HaArrayForEach(*timesets, ts, i)
  {
    if( i > 0 )
      HnStringAdd(&ac, "%s", ":");
    NrcTimeSetAddId(ins, ts, &ac);
  }
  return HnStringEnd(ac);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeSetsMergableOnDay(TIMESET ts1, TIMESET ts2)                  */
/*                                                                           */
/*  Return true if ts1 and ts2 are mergeable on the day of week fields.      */
/*                                                                           */
/*****************************************************************************/

static bool NrcTimeSetsMergableOnDay(TIMESET ts1, TIMESET ts2)
{
  int i, x;
  if( ts1->first_week != ts2->first_week || ts1->last_week != ts2->last_week )
    return false;
  if( (ts1->last_dow + 1) % 7 != ts2->first_dow )
    return false;
  if( HaArrayCount(ts1->intervals) != HaArrayCount(ts2->intervals) )
    return false;
  HaArrayForEach(ts1->intervals, x, i)
    if( HaArray(ts2->intervals, i) != x )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool NrcTimeSetsMergableOnWeek(TIMESET ts1, TIMESET ts2)                 */
/*                                                                           */
/*  Return true if ts1 and ts2 are mergeable on the week fields.             */
/*                                                                           */
/*****************************************************************************/

static bool NrcTimeSetsMergableOnWeek(TIMESET ts1, TIMESET ts2)
{
  int i, x;
  if( ts1->last_week + 1 != ts2->first_week )
    return false;
  if( ts1->first_dow != ts2->first_dow || ts1->last_dow != ts2->last_dow )
    return false;
  if( HaArrayCount(ts1->intervals) != HaArrayCount(ts2->intervals) )
    return false;
  HaArrayForEach(ts1->intervals, x, i)
    if( HaArray(ts2->intervals, i) != x )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *NrcShiftSetNameMake(NRC_SHIFT_SET ss)                              */
/*                                                                           */
/*  Return a KHE name for ss based on its content.                           */
/*                                                                           */
/*****************************************************************************/

static char *NrcShiftSetNameMake(NRC_SHIFT_SET ss)
{
  ARRAY_TIMESET timesets;  TIMESET ts, ts2;  int i, wk, dow, tod;
  NRC_SHIFT s;  char *res;  HA_ARENA a;

  if( DEBUG1 )
    fprintf(stderr, "[ NrcShiftSetNameMake(ss with %d shifts)\n",
      HaArrayCount(ss->shifts));

  /* if ss is empty, return a special name for that */
  if( HaArrayCount(ss->shifts) == 0 )
    res = "TG:Empty";
  else
  {
    /* sort shifts into increasing time order; make sure each appears once */
    HaArraySortUnique(ss->shifts, &NrcShiftCmp);

    /* convert the array of shifts into an array of timesets */
    a = NrcInstanceArena(ss->instance);
    HaArrayInit(timesets, a);
    ts = NULL;
    HaArrayForEach(ss->shifts, s, i)
    {
      NrcAnalyseShift(s, &wk, &dow, &tod);
      if( DEBUG1 )
	fprintf(stderr, "  s(wk = %d, dow = %d, tod = %d): ", wk, dow, tod);
      if( ts == NULL || ts->first_week != wk || ts->first_dow != dow )
      {
	if( DEBUG1 )
	  fprintf(stderr, "new ts (%s)\n", ts == NULL ? "first" :
            ts->first_week != wk ? "new week" : "new dow");
	ts = NrcTimeSetMake(wk, dow, tod, a);
	HaArrayAddLast(timesets, ts);
      }
      else if( HaArrayLast(ts->intervals) + 1 == tod )
      {
	if( DEBUG1 )
	  fprintf(stderr, "extend interval\n");
	HaArrayLast(ts->intervals)++;
      }
      else
      {
	if( DEBUG1 )
	  fprintf(stderr, "new interval\n");
	HaArrayAddLast(ts->intervals, tod);
	HaArrayAddLast(ts->intervals, tod);
      }
    }

    /* merge adjacent timesets on day where possible */
    for( i = 0;  i < HaArrayCount(timesets) - 1;  i++ )
    {
      ts = HaArray(timesets, i);
      ts2 = HaArray(timesets, i + 1);
      if( NrcTimeSetsMergableOnDay(ts, ts2) )
      {
	ts->last_dow = ts2->last_dow;
	HaArrayDeleteAndShift(timesets, i+1);
	i--;
      }
    }

    /* merge adjacent timesets on week where possible */
    for( i = 0;  i < HaArrayCount(timesets) - 1;  i++ )
    {
      ts = HaArray(timesets, i);
      ts2 = HaArray(timesets, i + 1);
      if( NrcTimeSetsMergableOnWeek(ts, ts2) )
      {
	ts->last_week = ts2->last_week;
	HaArrayDeleteAndShift(timesets, i+1);
	i--;
      }
    }

    /* return the name for the (hopefully reduced) array of timesets */
    res = NrcTimeSetsId(ss->instance, &timesets);
  }
  if( DEBUG1 )
    fprintf(stderr, "] NrcShiftSetNameMake returning \"%s\"\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP NrcShiftSetTimeGroup(NRC_SHIFT_SET ss, KHE_INSTANCE ins)  */
/*                                                                           */
/*  Return a time group corresponding to ss.  Make up its name.              */
/*                                                                           */
/*****************************************************************************/

static char *NrcTimeGroupKind(KHE_TIME_GROUP_KIND kind)
{
  switch( kind )
  {
    case KHE_TIME_GROUP_KIND_DAY:	return "day";
    case KHE_TIME_GROUP_KIND_WEEK:	return "week";
    case KHE_TIME_GROUP_KIND_ORDINARY:	return "ordinary";
    default:				return "?";
  }
}

KHE_TIME_GROUP NrcShiftSetTimeGroup(NRC_SHIFT_SET ss, KHE_INSTANCE ins)
{
  NRC_SHIFT s;  int i;  char *id;  HA_ARENA a;
  if( ss->time_group == NULL )
  {
    id = NrcShiftSetNameMake(ss);
    if( DEBUG2 )
      fprintf(stderr, "  [ NrcShiftSetTimeGroup(%s, %s)\n", id,
	KheInstanceId(ins));
    if( !KheInstanceRetrieveTimeGroup(ins, id, &ss->time_group) )
    {
      if( DEBUG2 )
	fprintf(stderr, "    making %s time group\n",
          NrcTimeGroupKind(ss->kind));
      if( !KheTimeGroupMake(ins, ss->kind, id, id, &ss->time_group) )
	HnAbort("NrcShiftSetTimeGroup:  KHE name %s used twice", id);
      HaArrayForEach(ss->shifts, s, i)
	KheTimeGroupAddTime(ss->time_group, NrcShiftTime(s));
    }
    else if( ss->kind != KHE_TIME_GROUP_KIND_ORDINARY )
    {
      a = NrcInstanceArena(NrcShiftSetInstance(ss));
      id = HnStringMake(a, "%s-%s", id,
	ss->kind == KHE_TIME_GROUP_KIND_DAY ? "Dy" : "Wk");
      if( DEBUG2 )
	fprintf(stderr, "    making %s time group\n",
          NrcTimeGroupKind(ss->kind));
      if( !KheTimeGroupMake(ins, ss->kind, id, id, &ss->time_group) )
	HnAbort("NrcShiftSetTimeGroup:  KHE name %s used twice", id);
      HaArrayForEach(ss->shifts, s, i)
	KheTimeGroupAddTime(ss->time_group, NrcShiftTime(s));
    }
    if( DEBUG2 )
      fprintf(stderr, "  ] NrcShiftSetTimeGroup\n");
  }
  return ss->time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP NrcShiftSetEventGroup(NRC_SHIFT_SET ss, char *name,      */
/*    KHE_INSTANCE ins)                                                      */
/*                                                                           */
/*  Return an event group corresponding to ss.  Use name in its name.        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP NrcShiftSetEventGroup(NRC_SHIFT_SET ss, char *name,
  KHE_INSTANCE ins)
{
  NRC_SHIFT s;  int i;
  if( ss->event_group == NULL )
  {
    if( !KheEventGroupMake(ins, KHE_EVENT_GROUP_KIND_ORDINARY,
	  name, name, &ss->event_group) )
      HnAbort("NrcShiftSetEventGroup internal error");
    HaArrayForEach(ss->shifts, s, i)
      KheEventGroupAddEvent(ss->event_group, NrcShiftEvent(s));
  }
  return ss->event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetSortShifts(NRC_SHIFT_SET ss)                             */
/*                                                                           */
/*  Sort the shifts of ss into increasing order.                             */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetSortShifts(NRC_SHIFT_SET ss)
{
  HaArraySort(ss->shifts, &NrcShiftCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftSetTypedCmp(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2)            */
/*                                                                           */
/*  Assuming that the shifts are sorted in both, compare them.               */
/*                                                                           */
/*  This function also accepts NULL shift-sets, comparing equal when         */
/*  both are NULL, or both are non-NULL and equal.                           */
/*                                                                           */
/*****************************************************************************/

int NrcShiftSetTypedCmp(NRC_SHIFT_SET ss1, NRC_SHIFT_SET ss2)
{
  int cmp, i;  NRC_SHIFT s1, s2;

  /* handle the cases where one or both of ss1 and ss2 are NULL */
  if( ss1 == NULL )
    return ss2 == NULL ? 0 : 1;
  if( ss2 == NULL )
    return -1;

  /* handle the non-NULL cases */
  cmp = HaArrayCount(ss1->shifts) - HaArrayCount(ss2->shifts);
  if( cmp != 0 )  return cmp;
  for( i = 0;  i < HaArrayCount(ss1->shifts);  i++ )
  {
    s1 = HaArray(ss1->shifts, i);
    s2 = HaArray(ss2->shifts, i);
    cmp = NrcShiftTypedCmp(s1, s2);
    if( cmp != 0 )  return cmp;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int NrcShiftSetCmp(const void *p1, const void *p2)                       */
/*                                                                           */
/*  Comparison function for sorting shift-sets.                              */
/*                                                                           */
/*****************************************************************************/

int NrcShiftSetCmp(const void *p1, const void *p2)
{
  NRC_SHIFT_SET ss1 = * (NRC_SHIFT_SET *) p1;
  NRC_SHIFT_SET ss2 = * (NRC_SHIFT_SET *) p2;
  return NrcShiftSetTypedCmp(ss1, ss2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetAndPolarityDebug(NRC_SHIFT_SET ss, NRC_POLARITY po,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ss onto fp.                                               */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetAndPolarityDebug(NRC_SHIFT_SET ss, NRC_POLARITY po,
  int indent, FILE *fp)
{
  int i;  NRC_SHIFT s;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[", indent, "");
    HaArrayForEach(ss->shifts, s, i)
    {
      if( i > 0 )
      {
	if( i % 8 == 0 )
	  fprintf(fp, ",\n%*s ", indent, "");
	else
	  fprintf(fp, ", ");
      }
      NrcShiftDebug(s, -1, fp);
    }
    fprintf(fp, "]%s\n", po == NRC_NEGATIVE ? "*" : "");
  }
  else
    fprintf(fp, "[%d shifts]%s", HaArrayCount(ss->shifts),
      po == NRC_NEGATIVE ? "*" : "");
}


/*****************************************************************************/
/*                                                                           */
/*  void NrcShiftSetDebug(NRC_SHIFT_SET ss, int indent, FILE *fp)            */
/*                                                                           */
/*  Debug print of ss onto fp.                                               */
/*                                                                           */
/*****************************************************************************/

void NrcShiftSetDebug(NRC_SHIFT_SET ss, int indent, FILE *fp)
{
  NrcShiftSetAndPolarityDebug(ss, NRC_POSITIVE, indent, fp);
}

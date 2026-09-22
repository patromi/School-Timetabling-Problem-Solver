
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
/*  FILE:         khe_limit_active_intervals_monitor.c                       */
/*  DESCRIPTION:  A limit active intervals monitor                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG_CHECK 0
#define DEBUG10 0	/* cases of *last_index = -1 */


/*****************************************************************************/
/*                                                                           */
/*  KHE_POINT                                                                */
/*                                                                           */
/*  A point is an index into the sequence of time groups being monitored.    */
/*  It is defined by two attributes:  the monitor and an integer index.      */
/*                                                                           */
/*  A point may be real or virtual.  It is real when its index points to a   */
/*  real time group (when 0 <= index < m->time_group_info_count)             */
/*  and virtual otherwise.  A virtual point may index the history_before     */
/*  range, in which case its state is active when the monitor is attached,   */
/*  or it may index the history_after range, in which case its state is      */
/*  open when the monitor is attached.                                       */
/*                                                                           */
/*****************************************************************************/

/* we don't define this type, we just pass its two attributes around */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RANGE                                                                */
/*                                                                           */
/*  A range is a sequence of adjacent points.  It is usually non-empty,      */
/*  although we sometimes speak of the history_before and history_after      */
/*  ranges, which can be empty.  It is defined by three attributes: the      */
/*  monitor, a first index, and a last index.  It includes its endpoints.    */
/*                                                                           */
/*****************************************************************************/

/* we don't define this type, we just pass its three attributes around */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_INFO                                                      */
/*                                                                           */
/*  A time group info object, called a time group in the implementation      */
/*  chapter of the User's Guide.  It contains everything we need to know     */
/*  about the time group at one real point, notably its state:               */
/*                                                                           */
/*    KHE_INACTIVE                                                           */
/*      The time group is not active, or not attached (because the           */
/*      monitor is not attached).  It does not lie in any interval.          */
/*                                                                           */
/*    KHE_ACTIVE                                                             */
/*      The time group is active.  It lies in some interval.                 */
/*                                                                           */
/*    KHE_OPEN                                                               */
/*      The time group is attached, at or after the sweep index, and         */
/*      not busy, and KheLimitActiveIntervalsMonitorSetNotBusyState has      */
/*      not been called to say that its state should be considered to be     */
/*      either active or inactive in this case.  It lies in some interval.   */
/*                                                                           */
/*  When checking for violations of minimum limits, open time groups         */
/*  count as active, because they could be active, and this gives the        */
/*  smallest (most conservative) estimate of cost.  When checking for        */
/*  violations of maximum limits, open time groups count as inactive,        */
/*  because they could be inactive, and again this gives the smallest        */
/*  (most conservative) estimate of cost.                                    */
/*                                                                           */
/*  A time group may be virtual:  logically present, but without any         */
/*  time group info object.  Virtual time groups have no time group or       */
/*  polarity.  When the monitor is attached, virtual time groups in the      */
/*  history_before range lie in history_before_interval and have state       */
/*  KHE_ACTIVE, while time groups in the history_after range lie in          */
/*  history_after_interval and have state KHE_OPEN.  When the monitor        */
/*  is not attached, like other time groups virtual ones lie in no           */
/*  interval and have state KHE_INACTIVE.                                    */
/*                                                                           */
/*  It is often better to represent a time group by a point than by a time   */
/*  group info object.  This hides the distinction between real and virtual  */
/*  time groups from those parts of the implementation that do not need to   */
/*  know about it, and makes it easy to access adjacent points.              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_INACTIVE,
  KHE_ACTIVE,
  KHE_OPEN
} KHE_TIME_GROUP_STATE;

typedef struct khe_ao_interval_rec *KHE_AO_INTERVAL;

typedef struct khe_time_group_info_rec {
  KHE_MONITORED_TIME_GROUP	monitored_time_group;	/* the time group    */
  KHE_POLARITY			polarity;	/* polarity of time group    */
  KHE_TIME_GROUP_STATE		state;		/* state of time group       */
  KHE_TIME_GROUP_STATE		not_busy_state;	/* from SetNotBusy           */
  KHE_AO_INTERVAL		interval;	/* when active or open       */
  KHE_COST			tilt_cost;	/* depends on time group     */
} KHE_TIME_GROUP_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AO_INTERVAL - one interval of active or open time groups             */
/*                                                                           */
/*  An ao-interval, usually called simply an interval in this file,          */
/*  represents a range in which every time group is either active or         */
/*  open, and the time groups just beyond the range either do not exist      */
/*  or are inactive.  Unlike a range, which could be empty, an interval      */
/*  is always non-empty, so that first_index <= last_index.                  */
/*                                                                           */
/*  The last_active_index attribute is the index of the last active point    */
/*  in the initial a-interval, if any, otherwise it is first_index - 1.      */
/*                                                                           */
/*  An interval does not know and does not care whether the points in its    */
/*  range are real or virtual.                                               */
/*                                                                           */
/*  The deviation is the amount by which the interval's length falls short   */
/*  of the minimum limit, or its active length exceeds the maxiumum limit,   */
/*  or is 0 if neither case applies (it is impossible to have both), and     */
/*  the cost is the cost corresponding to this deviation.                    */
/*                                                                           */
/*  The least important thing about an interval is that it is always in      */
/*  one of three states:                                                     */
/*                                                                           */
/*    KHE_FREE                                                               */
/*      The interval is not in use; it is on the free list.                  */
/*                                                                           */
/*    KHE_GOOD                                                               */
/*      The interval is in use, and is violating no limits.                  */
/*                                                                           */
/*    KHE_BAD                                                                */
/*      The interval is in use, and it is violating some limit.              */
/*                                                                           */
/*  Corresponding to these three states there are three lists of intervals.  */
/*  Every interval lies in exactly one of these lists.  An interval's index  */
/*  in its current list is held in index_in_list.                            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_FREE = 0,
  KHE_GOOD = 1,
  KHE_BAD = 2
} KHE_AO_INTERVAL_STATE;

struct khe_ao_interval_rec {
  KHE_AO_INTERVAL_STATE	state;			/* free, good, or bad        */
  int			index_in_list;		/* index in state list       */
  int			first_index;		/* start of range            */
  int			last_index;		/* end of range              */
  int			last_active_index;	/* initial a-interval        */
  int			deviation;		/* deviation, if violating   */
  int			at_max_limit_count;	/* 1 if at max limit         */
  KHE_COST		cost;			/* cost of deviation         */
  KHE_AO_INTERVAL	copy;			/* temporary, for copying    */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR - monitors active intervals           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_AO_INTERVAL) ARRAY_KHE_AO_INTERVAL;

struct khe_limit_active_intervals_monitor_rec {
  INHERIT_MONITOR(unushed, offset)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT constraint;	/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  int				history_before;		/* history: ai       */
  int				history_after;		/* history: ci       */
  int				history;		/* history: xi       */
  int				at_max_limit_count;	/* at max count      */
  KHE_TIME			sweep_time;		/* sweep time        */
  int				sweep_index;		/* sweep index       */
  ARRAY_KHE_AO_INTERVAL		intervals[3];		/* lists of ints.    */
  KHE_AO_INTERVAL		history_before_interval;/* when attached     */
  KHE_AO_INTERVAL		history_after_interval;	/* when attached     */
  KHE_COST			history_adjustment;	/* prev monitor cost */
  KHE_TIME			first_time;		/* time range        */
  KHE_TIME			last_time;		/* time range        */
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR copy;		/* used when copying */
  bool				tilt;			/* tilt wanted       */
  int				time_group_info_count;	/* length of tgi     */
  KHE_TIME_GROUP_INFO		time_group_info[1];	/* extends!          */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "points and ranges"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePointIsReal(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)     */
/*                                                                           */
/*  Return true if point is real.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KhePointIsReal(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)
{
  return 0 <= index && index < m->time_group_info_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_STATE KheRealPointState(                                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)                       */
/*                                                                           */
/*  Return the state of this real point.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_STATE KheRealPointState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)
{
  return m->time_group_info[index].state;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_STATE KhePointState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, */
/*    int index)                                                             */
/*                                                                           */
/*  Return the state of this real or virtual point.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_STATE KhePointState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  if( index < 0 )
    return m->attached ? KHE_ACTIVE : KHE_INACTIVE;
  else if( index < m->time_group_info_count )
    return KheRealPointState(m, index);
  else
    return m->attached ? KHE_OPEN : KHE_INACTIVE;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_STATE KheRealAttachedPointNewState(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index, int busy_count)       */
/*                                                                           */
/*  Assuming that (m, index) is a real and attached point, work out          */
/*  its new state given this value of busy_count.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_STATE KheRealAttachedPointNewState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index, int busy_count)
{
  KHE_POLARITY po;
  if( busy_count == 0 && index >= m->sweep_index )
    return m->time_group_info[index].not_busy_state;
  else
  {
    po = m->time_group_info[index].polarity;
    if( (po == KHE_NEGATIVE) == (busy_count == 0) )
      return KHE_ACTIVE;
    else
      return KHE_INACTIVE;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointSetState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,          */
/*    int index, KHE_TIME_GROUP_STATE new_state)                             */
/*                                                                           */
/*  Set the state of the real point at index.                                */
/*                                                                           */
/*****************************************************************************/
static char *KheTimeGroupStateShow(KHE_TIME_GROUP_STATE state);

static void KheRealPointSetState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index, KHE_TIME_GROUP_STATE new_state)
{
  HnAssert(KhePointIsReal(m, index), "KheRealPointSetState internal error: "
    "index %d out of range 0 .. %d", index, m->time_group_info_count - 1);
  m->time_group_info[index].state = new_state;
  if( DEBUG5 )
    fprintf(stderr, "KheRealPointSetState(%s, %d, %s) assigned %s\n",
      KheLimitActiveIntervalsMonitorId(m), index,
      KheTimeGroupStateShow(new_state),
      KheTimeGroupStateShow(m->time_group_info[index].state));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KhePointTiltCost(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,          */
/*    int index)                                                             */
/*                                                                           */
/*  Return the tilt cost of this real or virtual point.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KhePointTiltCost(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  if( m->time_group_info_count == 0 )
    return KheCost(0, 0);
  if( index < 0 )
    index = 0;
  else if( index >= m->time_group_info_count )
    index = m->time_group_info_count - 1;
  return m->time_group_info[index].tilt_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AO_INTERVAL KheRealPointInterval(                                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)                       */
/*                                                                           */
/*  Return the interval of the real time group at index.  This will be       */
/*  NULL if the time group is inactive.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_AO_INTERVAL KheRealPointInterval(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)
{
  return m->time_group_info[index].interval;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP KheRealPointMonitoredTimeGroup(                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index, KHE_POLARITY *po)     */
/*                                                                           */
/*  Return the monitored time group at this real point, and also set *po.    */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITORED_TIME_GROUP KheRealPointMonitoredTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index, KHE_POLARITY *po)
{
  KHE_TIME_GROUP_INFO mi;
  HnAssert(index >= 0 && index < m->time_group_info_count,
    "KheRealPointMonitoredTimeGroup: index (%d) out of range (0 .. %d)",
    index, m->time_group_info_count - 1);
  mi = m->time_group_info[index];
  *po = mi.polarity;
  return mi.monitored_time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AO_INTERVAL KhePointInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,   */
/*    int index)                                                             */
/*                                                                           */
/*  Return the interval of the real or virtual time group at index.  This    */
/*  will be NULL if the time group is inactive.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_AO_INTERVAL KhePointInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  KHE_AO_INTERVAL res;

  /* get the interval, possibly a history interval */
  if( index < 0 )
    res = m->history_before_interval;
  else if( index < m->time_group_info_count )
    res = KheRealPointInterval(m, index);
  else
    res = m->history_after_interval;

  /* check consistency and return */
  if( res != NULL )
  {
    HnAssert(res->first_index <= index, "KhePointInterval internal error 2");
    HnAssert(index <= res->last_index,  "KhePointInterval internal error 3");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointSetInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,       */
/*    int index, KHE_AO_INTERVAL ai)                                         */
/*                                                                           */
/*  Set the interval at the real point with this index to ai.                */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointSetInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index, KHE_AO_INTERVAL ai)
{
  m->time_group_info[index].interval = ai;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointCheck(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)  */
/*                                                                           */
/*  Check that the real point at this index is consistent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointCheck(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  KHE_TIME_GROUP_INFO mi;  int busy_count;  KHE_TIME_GROUP_STATE ns;
  KHE_AO_INTERVAL ai;  KHE_BUSY_AND_IDLE busy_and_idle;
  mi = m->time_group_info[index];
  if( !m->attached )
    HnAssert(mi.state == KHE_INACTIVE, "KheRealPointCheck(%s, %d) error 1",
      KheLimitActiveIntervalsMonitorId(m), index);
  else
  {
    busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mi.monitored_time_group);
    busy_count = busy_and_idle->busy_count;
    ns = KheRealAttachedPointNewState(m, index, busy_count);
    HnAssert(mi.state == ns, "KheRealPointCheck(%s, %d) error 2: recorded %s, "
      "calculated %s:\n  (polarity %s, busy_count %d, sweep_index %d)",
      KheLimitActiveIntervalsMonitorId(m), index,
      KheTimeGroupStateShow(mi.state), KheTimeGroupStateShow(ns),
      mi.polarity == KHE_POSITIVE ? "positive" : "negative", busy_count,
      m->sweep_index);
    ai = mi.interval;
    if( mi.state == KHE_INACTIVE )
      HnAssert(ai == NULL, "KheRealPointCheck(%s, %d) error 3",
	KheLimitActiveIntervalsMonitorId(m), index);
    else
    {
      HnAssert(ai != NULL, "KheRealPointCheck(%s, %d) error 4", index);
      HnAssert(ai->first_index <= index && index <= ai->last_index,
	"KheRealPointCheck(%s, %d) error 5",
	KheLimitActiveIntervalsMonitorId(m), index);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRangeSetInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,           */
/*    int first_index, int last_index, KHE_AO_INTERVAL ai)                   */
/*                                                                           */
/*  Inform the points in range first_index .. last_index (including virtual  */
/*  points) that they belong to interval ai.  The time groups at these       */
/*  points will be active or open, although that is not checked here.        */
/*                                                                           */
/*****************************************************************************/

static void KheRangeSetInterval(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int first_index, int last_index, KHE_AO_INTERVAL ai)
{
  int i;

  /* set the interval in the history_before range */
  if( first_index < 0 )
  {
    m->history_before_interval = ai;
    first_index = 0;
  }

  /* set the interval in the history_after range */
  if( last_index >= m->time_group_info_count )
  {
    m->history_after_interval = ai;
    last_index = m->time_group_info_count - 1;
  }

  /* set the interval in the real range */
  for( i = first_index;  i <= last_index;  i++ )
    KheRealPointSetInterval(m, i, ai);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeGroupStateShow(KHE_TIME_GROUP_STATE state)                  */
/*                                                                           */
/*  Return a string value showing state.                                     */
/*                                                                           */
/*****************************************************************************/

static char *KheTimeGroupStateShow(KHE_TIME_GROUP_STATE state)
{
  switch( state )
  {
    case KHE_INACTIVE:	return "inactive";
    case KHE_ACTIVE:	return "active";
    case KHE_OPEN:	return "open";
    default:		return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "intervals - miscellaneous updates and queries"                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalRemoveFromList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,     */
/*    KHE_AO_INTERVAL ai)                                                    */
/*                                                                           */
/*  Assuming that ai is on some list, remove it from that list.              */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalRemoveFromList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai)
{
  KHE_AO_INTERVAL_STATE old_state;  KHE_AO_INTERVAL ai2;  int i;
  old_state = ai->state;
  if( old_state == KHE_BAD )
  {
    /* preserve the order of the remaining intervals */
    for( i = ai->index_in_list+1; i < HaArrayCount(m->intervals[KHE_BAD]); i++ )
    {
      ai2 = HaArray(m->intervals[KHE_BAD], i);
      HaArrayPut(m->intervals[KHE_BAD], i - 1, ai2);
      ai2->index_in_list = i - 1;
    }
    HaArrayDeleteLast(m->intervals[KHE_BAD]);
  }
  else
  {
    /* no need to preserve the order of the intervals */
    if( ai == HaArrayLast(m->intervals[old_state]) )
      HaArrayDeleteLast(m->intervals[old_state]);
    else
    {
      ai2 = HaArrayLastAndDelete(m->intervals[old_state]);
      HaArrayPut(m->intervals[old_state], ai->index_in_list, ai2);
      ai2->index_in_list = ai->index_in_list;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalAddToList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,          */
/*    KHE_AO_INTERVAL ai, KHE_AO_INTERVAL_STATE new_state)                   */
/*                                                                           */
/*  Change the state of ai and add it to the appropriate list.               */
/*  If new_state is KHE_BAD, keep the list sorted by first_index.            */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalAddToList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai, KHE_AO_INTERVAL_STATE new_state)
{
  int i;  KHE_AO_INTERVAL ai2;
  ai->state = new_state;
  if( new_state == KHE_BAD )
  {
    /* add at the point which keeps the list sorted by first_index */
    HaArrayAddLast(m->intervals[KHE_BAD], NULL);  /* make a space */
    for( i = HaArrayCount(m->intervals[KHE_BAD]) - 1;  i > 0;  i-- )
    {
      ai2 = HaArray(m->intervals[KHE_BAD], i - 1);
      if( ai2->first_index < ai->first_index )
	break;
      HaArrayPut(m->intervals[KHE_BAD], i, ai2);
      ai2->index_in_list = i;
    }
    HaArrayPut(m->intervals[KHE_BAD], i, ai);
    ai->index_in_list = i;
  }
  else
  {
    /* add at end */
    ai->index_in_list = HaArrayCount(m->intervals[new_state]);
    HaArrayAddLast(m->intervals[new_state], ai);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePointId(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,        */
/*    char buff[])                                                           */
/*                                                                           */
/*  Return an identifying Id for this point.  If scratch memory is needed,   */
/*  use buff.                                                                */
/*                                                                           */
/*****************************************************************************/

static char *KhePointId(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,
  char buff[])
{
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  if( KhePointIsReal(m, index) )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(m, index, &po);
    if( KheTimeGroupId(tg) != NULL )
      return KheTimeGroupId(tg);
  }
  else if( index < 0 )
    sprintf(buff, "(%d)", index);
  else
    sprintf(buff, "%d", index);
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheIntervalShow(KHE_AO_INTERVAL ai,                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, char buff[])                     */
/*                                                                           */
/*  Debug print of ai using buff[] as scratch memory.                        */
/*                                                                           */
/*****************************************************************************/

static char *KheIntervalShow(KHE_AO_INTERVAL ai,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, char buff[])
{
  char buff1[50], buff2[50], buff3[50], buffa[50], buffb[50];
  if( ai->last_active_index != ai->last_index )
    sprintf(buffa, "[%s-%s|%s", KhePointId(m, ai->first_index, buff1),
      KhePointId(m, ai->last_active_index, buff2),
      KhePointId(m, ai->last_index, buff3));
  else if( ai->first_index != ai->last_index )
    sprintf(buffa, "[%s-%s", KhePointId(m, ai->first_index, buff1),
      KhePointId(m, ai->last_index, buff2));
  else
    sprintf(buffa, "[%s", KhePointId(m, ai->first_index, buff1));
  if( ai->deviation != 0 )
    sprintf(buffb, " dev %d]", ai->deviation);
  else
    sprintf(buffb, "]");
  sprintf(buff, "%s%s", buffa, buffb);
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheIntervalStateShow(KHE_AO_INTERVAL_STATE state)                  */
/*                                                                           */
/*  Debug print of state.                                                    */
/*                                                                           */
/*****************************************************************************/

static char *KheIntervalStateShow(KHE_AO_INTERVAL_STATE state)
{
  switch( state )
  {
    case KHE_FREE:	return "free";
    case KHE_GOOD:	return "good";
    case KHE_BAD:	return "bad";
    default:		return "?";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalMoveToList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,         */
/*    KHE_AO_INTERVAL ai, KHE_AO_INTERVAL_STATE new_state)                   */
/*                                                                           */
/*  Change the state of ai from whatever it is now to new_state, including   */
/*  removing it from its old list and adding it to its new list.             */
/*                                                                           */
/*****************************************************************************/

void KheIntervalMoveToList(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai, KHE_AO_INTERVAL_STATE new_state)
{
  char buff[200];
  if( DEBUG8 )
    fprintf(stderr, "  KheIntervalMoveToList(m, %s, %s -> %s)\n",
      KheIntervalShow(ai, m, buff), KheIntervalStateShow(ai->state),
      KheIntervalStateShow(new_state));
  KheIntervalRemoveFromList(m, ai);
  KheIntervalAddToList(m, ai, new_state);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AO_INTERVAL KheIntervalCopyPhase1(KHE_AO_INTERVAL ai, HA_ARENA a)    */
/*                                                                           */
/*  Carry out phase 1 of copying ai.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_AO_INTERVAL KheIntervalCopyPhase1(KHE_AO_INTERVAL ai, HA_ARENA a)
{
  KHE_AO_INTERVAL copy;
  if( ai->copy == NULL )
  {
    HaMake(copy, a);
    /* MMakeCount(copy, 31); */
    ai->copy = copy;
    copy->state = ai->state;
    copy->index_in_list = ai->index_in_list;
    copy->first_index = ai->first_index;
    copy->last_index = ai->last_index;
    copy->last_active_index = ai->last_active_index;
    copy->deviation = ai->deviation;
    copy->at_max_limit_count = ai->at_max_limit_count;
    copy->cost = ai->cost;
    copy->copy = NULL;
  }
  return ai->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCopyPhase2(KHE_AO_INTERVAL ai)                           */
/*                                                                           */
/*  Carry out phase 2 of copying ai.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalCopyPhase2(KHE_AO_INTERVAL ai)
{
  ai->copy = NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalFree(KHE_AO_INTERVAL ai)                                 */
/*                                                                           */
/*  Free ai.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheIntervalFree(KHE_AO_INTERVAL ai)
{
  MFree(ai);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalLength(KHE_AO_INTERVAL ai)                                */
/*                                                                           */
/*  Return the length of ai.                                                 */
/*                                                                           */
/*****************************************************************************/

static int KheIntervalLength(KHE_AO_INTERVAL ai)
{
  return ai->last_index - ai->first_index + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalActiveLength(KHE_AO_INTERVAL ai)                          */
/*                                                                           */
/*  Return the length of the initial active interval of ai, or 0 if none.    */
/*                                                                           */
/*****************************************************************************/

static int KheIntervalActiveLength(KHE_AO_INTERVAL ai)
{
  return ai->last_active_index - ai->first_index + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalDev(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,                 */
/*    KHE_AO_INTERVAL ai, int *at_max_limit_count)                           */
/*                                                                           */
/*  Return the deviation contributed by interval ai, not forgetting that     */
/*  intervals that begin at or after the sweep index attract no cost.        */
/*                                                                           */
/*  Also set *at_max_limit_count when the interval is one step away from     */
/*  violating a maximum limit.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheIntervalDev(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai, int *at_max_limit_count)
{
  int len;

  if( ai->first_index < m->sweep_index )
  {
    /* check for initial a-interval length over the limit */
    len = KheIntervalActiveLength(ai);
    if( len > m->maximum )
    {
      *at_max_limit_count = 0;
      return len - m->maximum;
    }

    /* check for ao-interval length under the limit */
    len = KheIntervalLength(ai);
    if( len < m->minimum )
    {
      *at_max_limit_count = 0;
      return m->minimum - len;
    }

    /* neither over the maximum nor under the minimum */
    *at_max_limit_count = (len == m->maximum ? 1 : 0);
    return 0;
  }
  else
  {
    /* past the sweep index, so no deviation for this interval */
    *at_max_limit_count = 0;
    return 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalGetLastActiveIndex(KHE_AO_INTERVAL ai,                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int from_index)                  */
/*                                                                           */
/*  Scan ai to determine its last active index.  Time groups in the range    */
/*  ai->first_index ... from_index - 1 are already known to be active, so    */
/*  start checking from from_index.                                          */
/*                                                                           */
/*****************************************************************************/

static int KheIntervalGetLastActiveIndex(KHE_AO_INTERVAL ai,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int from_index)
{
  int i;
  for( i = from_index;  i <= ai->last_index;  i++ )
    if( KhePointState(m, i) != KHE_ACTIVE )
      break;
  return i - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorDevToCostWithTilt(                                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int dev, int tilt_index)         */
/*                                                                           */
/*  Like KheMonitorDevToCost, except it subtracts a tilt when m->tilt.       */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheMonitorDevToCostWithTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int dev, int tilt_index)
{
  KHE_COST res;
  if( dev == 0 || m->combined_weight == 0 )
    return 0;
  else switch( m->cost_function )
  {
    case KHE_STEP_COST_FUNCTION:

        res = m->combined_weight;
	break;

    case KHE_LINEAR_COST_FUNCTION:

        res = dev * m->combined_weight;
	break;

    case KHE_QUADRATIC_COST_FUNCTION:

	res = dev * dev * m->combined_weight;
	break;

    default:

	HnAbort("KheMonitorDevToCostWithTilt internal error");
	res = 0;  /* keep compiler happy */
  }
  if( m->tilt )
    return res - KhePointTiltCost(m, tilt_index);
  else
    return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalCheck(KHE_AO_INTERVAL ai,                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Check interval.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalCheck(KHE_AO_INTERVAL ai,
  KHE_AO_INTERVAL_STATE expected_state, KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_TIME_GROUP_STATE state;  int i, at_max_limit_count, dev;  char buff[100];
  HnAssert(ai->state == expected_state,
    "KheIntervalCheck(%s): %s interval state = %s, expected = %s",
    KheLimitActiveIntervalsMonitorId(m), KheIntervalShow(ai, m, buff),
    KheIntervalStateShow(ai->state), KheIntervalStateShow(expected_state));
  HnAssert(ai->first_index >= - m->history, "KheIntervalCheck 1");
  HnAssert(ai->last_index < m->time_group_info_count + m->history_after,
    "KheIntervalCheck 2");
  HnAssert(ai->first_index <= ai->last_index, "KheIntervalCheck 3");
  HnAssert(ai->last_active_index >= ai->first_index - 1, "KheIntervalCheck 4");
  HnAssert(ai->last_active_index <= ai->last_index,
    "KheIntervalCheck(%s): %s but last_active_index = %d",
    KheLimitActiveIntervalsMonitorId(m), KheIntervalShow(ai, m, buff),
    ai->last_active_index);
  dev = KheIntervalDev(m, ai, &at_max_limit_count);
  HnAssert(ai->deviation == dev,
    "KheIntervalCheck(%s): %s ai->deviation = %d, calculated = %d",
    KheLimitActiveIntervalsMonitorId(m), KheIntervalShow(ai, m, buff),
    ai->deviation, dev);
  HnAssert(ai->at_max_limit_count == at_max_limit_count,
    "KheIntervalCheck(%s): %s ai->at_max_limit_count = %d, calculated = %d",
    KheLimitActiveIntervalsMonitorId(m), KheIntervalShow(ai, m, buff),
    ai->at_max_limit_count, at_max_limit_count);
  HnAssert(ai->cost == KheMonitorDevToCostWithTilt(m, ai->deviation,
    ai->first_index), "KheIntervalCheck 7");

  for( i = ai->first_index;  i <= ai->last_active_index;  i++ )
  {
    state = KhePointState(m, i);
    HnAssert(state == KHE_ACTIVE, "KheIntervalCheck 8");
  }
  for( i = ai->first_index;  i <= ai->last_index;  i++ )
  {
    state = KhePointState(m, i);
    HnAssert(state == KHE_ACTIVE || state == KHE_OPEN,
      "KheIntervalCheck(%s): %s state at %d = %s",
      KheLimitActiveIntervalsMonitorId(m), KheIntervalShow(ai, m, buff),
      i, KheTimeGroupStateShow(state));
    HnAssert(KhePointInterval(m, i) == ai, "KheIntervalCheck 10");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "intervals - adding, adjusting, and deleting"                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIntervalAdd(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,            */
/*    int first_index, int last_index, int last_active_index,                */
/*    KHE_AO_INTERVAL *ai)                                                   */
/*                                                                           */
/*  Add a new interval with the given attributes, update everything          */
/*  accordingly, and return the change (it can only be an increase) in       */
/*  cost caused by the new interval.  The corresponding time_group_info      */
/*  entries will point to the new interval.                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIntervalAdd(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int first_index, int last_index, int last_active_index, KHE_AO_INTERVAL *ai)
{
  KHE_AO_INTERVAL res;

  /* get or make the object */
  if( HaArrayCount(m->intervals[KHE_FREE]) > 0 )
    res = HaArrayLastAndDelete(m->intervals[KHE_FREE]);
  else
    HaMake(res, KheSolnArena(m->soln));
    /* MMakeCount(res, 32); */
  *ai = res;
  if( DEBUG4 )
    fprintf(stderr, "KheIntervalAdd(%s, %d, %d, %d, -)\n",
      KheLimitActiveIntervalsMonitorId(m), first_index, last_index,
      last_active_index);

  /* initialize it and add it to the appropriate intervals list */
  HnAssert(first_index <= last_index, "KheIntervalAdd internal error");
  res->first_index = first_index;
  res->last_index = last_index;
  res->last_active_index = last_active_index;
  res->deviation = KheIntervalDev(m, res, &res->at_max_limit_count);
  m->at_max_limit_count += res->at_max_limit_count;
  res->cost = KheMonitorDevToCostWithTilt(m, res->deviation, res->first_index);
  res->copy = NULL;
  KheIntervalAddToList(m, res, res->deviation == 0 ? KHE_GOOD : KHE_BAD);

  /* make the corresponding time_group_info entries point to res, and return */
  KheRangeSetInterval(m, first_index, last_index, res);
  return res->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIntervalAdjust(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,         */
/*    KHE_AO_INTERVAL ai, int new_first_index, int new_last_index,           */
/*    int new_last_active_index)                                             */
/*                                                                           */
/*  Adjust the endpoints and last_active_index attribute of ai, update       */
/*  everything accordingly, and return the change in cost.                   */
/*                                                                           */
/*  This function does not ensure that the corresponding time_group_info     */
/*  entries point to the right intervals.  That is done separately.          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIntervalAdjust(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai, int new_first_index, int new_last_index,
  int new_last_active_index)
{
  KHE_COST old_cost;  int old_at_max_limit_count;
  old_cost = ai->cost;
  old_at_max_limit_count = ai->at_max_limit_count;
  ai->first_index = new_first_index;
  ai->last_index = new_last_index;
  ai->last_active_index = new_last_active_index;
  ai->deviation = KheIntervalDev(m, ai, &ai->at_max_limit_count);
  m->at_max_limit_count += (ai->at_max_limit_count - old_at_max_limit_count);
  ai->cost = KheMonitorDevToCostWithTilt(m, ai->deviation, ai->first_index);
  if( ai->cost != old_cost )
  {
    if( old_cost == 0 )
    {
      if( DEBUG8 )
	fprintf(stderr,
	  "  calling KheIntervalMoveToList from KheIntervalAdjust (a)\n");
      KheIntervalMoveToList(m, ai, KHE_BAD);
    }
    else if( ai->cost == 0 )
    {
      if( DEBUG8 )
	fprintf(stderr,
	  "  calling KheIntervalMoveToList from KheIntervalAdjust (b)\n");
      KheIntervalMoveToList(m, ai, KHE_GOOD);
    }
  }
  return ai->cost - old_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheIntervalDelete(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,         */
/*    KHE_AO_INTERVAL ai)                                                    */
/*                                                                           */
/*  Delete ai, returning the resulting change in cost (always non-positive). */
/*                                                                           */
/*  This function does not ensure that the corresponding time_group_info     */
/*  entries point to the right intervals.  That is done separately.          */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheIntervalDelete(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai)
{
  if( DEBUG8 )
    fprintf(stderr, "  calling KheIntervalMoveToList from KheIntervalDelete\n");
  KheIntervalMoveToList(m, ai, KHE_FREE);
  return - ai->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalSweepIndexHasChanged(                                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_AO_INTERVAL ai)              */
/*                                                                           */
/*  The sweep index has changed in a way that might affect the cost of ai.   */
/*  Update everything including reporting cost.                              */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalSweepIndexHasChanged(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_AO_INTERVAL ai)
{
  KHE_COST old_cost;  int new_dev, old_at_max_limit_count;
  old_at_max_limit_count = ai->at_max_limit_count;
  new_dev = KheIntervalDev(m, ai, &ai->at_max_limit_count);
  m->at_max_limit_count += (ai->at_max_limit_count - old_at_max_limit_count);
  if( new_dev != ai->deviation )
  {
    old_cost = ai->cost;
    ai->deviation = new_dev;
    ai->cost = KheMonitorDevToCostWithTilt(m, ai->deviation, ai->first_index);
    if( ai->cost != old_cost )
    {
      if( old_cost == 0 )
      {
	if( DEBUG8 )
	  fprintf(stderr, "  calling KheIntervalMoveToList from "
	    "KheIntervalSweepIndexHasChanged (a)\n");
	KheIntervalMoveToList(m, ai, KHE_BAD);
      }
      else if( ai->cost == 0 )
      {
	if( DEBUG8 )
	  fprintf(stderr, "  calling KheIntervalMoveToList from "
	    "KheIntervalSweepIndexHasChanged (b)\n");
	KheIntervalMoveToList(m, ai, KHE_GOOD);
      }
      KheMonitorChangeCost((KHE_MONITOR) m, m->cost + ai->cost - old_cost);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "real point state changes"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointActiveToOpen(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,      */
/*    int index)                                                             */
/*                                                                           */
/*  Change the state at this real point from active to open, updating        */
/*  everything accordingly.  No intervals change, but the last active        */
/*  index of the point's interval may decrease.                              */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointActiveToOpen(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRealPointActiveToOpen(m, %d)\n", index);
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* update the time group state */
  KheRealPointSetState(m, index, KHE_OPEN);

  /* update the enclosing interval, which must exist */
  ai = KheRealPointInterval(m, index);
  if( index <= ai->last_active_index )
  {
    delta_cost = KheIntervalAdjust(m, ai, ai->first_index, ai->last_index,
      index - 1);
    if( delta_cost != 0 )
      KheMonitorChangeCost((KHE_MONITOR) m, m->cost + delta_cost);
  }
  if( DEBUG1 )
  {
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheRealPointActiveToOpen returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointOpenToActive(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,      */
/*    int index)                                                             */
/*                                                                           */
/*  Change the state at this real point from open to active, updating        */
/*  everything accordingly.  No intervals change, but the last active        */
/*  index of the point's interval may increase.                              */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointOpenToActive(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRealPointOpenToActive(m, %d)\n", index);
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* update the time group */
  KheRealPointSetState(m, index, KHE_ACTIVE);

  /* update the enclosing interval, which must exist */
  ai = KheRealPointInterval(m, index);
  if( index == ai->last_active_index + 1 )
  {
    delta_cost = KheIntervalAdjust(m, ai, ai->first_index, ai->last_index,
      KheIntervalGetLastActiveIndex(ai, m, index + 1));
    if( delta_cost != 0 )
      KheMonitorChangeCost((KHE_MONITOR) m, m->cost + delta_cost);
  }
  if( DEBUG1 )
  {
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheRealPointOpenToActive returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointActiveOrOpenToInactive(                                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)                       */
/*                                                                           */
/*  Change the state at this real point from active or open to inactive,     */
/*  updating everything accordingly.  Intervals will change.                 */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointActiveOrOpenToInactive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)
{
  KHE_AO_INTERVAL ai, ai2;  int init_last_index, lai1, lai2;
  KHE_COST delta_cost;  KHE_TIME_GROUP_STATE prev_state;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRealPointActiveOrOpenToInactive(m, %d)\n", index);
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* update the time group */
  prev_state = KheRealPointState(m, index);
  KheRealPointSetState(m, index, KHE_INACTIVE);

  /* update the encosing interval, which must exist; it needs to be split */
  ai = KheRealPointInterval(m, index);
  KheRealPointSetInterval(m, index, NULL);
  if( index == ai->first_index )
  {
    if( index == ai->last_index )
    {
      /* delete ai */
      delta_cost = KheIntervalDelete(m, ai);
    }
    else
    {
      /* reduce the length of ai at the left end by one */
      lai1 = (prev_state == KHE_ACTIVE ? ai->last_active_index :
	KheIntervalGetLastActiveIndex(ai, m, index + 1));
      delta_cost = KheIntervalAdjust(m, ai, index + 1, ai->last_index, lai1);
    }
  }
  else
  {
    if( index == ai->last_index )
    {
      /* reduce the length of ai at the right end by one */
      lai1 = (ai->last_active_index == ai->last_index ?
	ai->last_active_index - 1 : ai->last_active_index);
      delta_cost = KheIntervalAdjust(m, ai, ai->first_index, index - 1, lai1);
    }
    else
    {
      /* reduce ai and make a new interval for what follows index */
      if( ai->last_active_index == ai->last_index )
      {
	lai1 = index - 1;
	lai2 = ai->last_index;
      }
      else
      {
	lai1 = min(index - 1, ai->last_active_index);
	lai2 = KheIntervalGetLastActiveIndex(ai, m, index + 1);
      }
      init_last_index = ai->last_index;
      delta_cost = KheIntervalAdjust(m, ai, ai->first_index, index - 1, lai1)
	+ KheIntervalAdd(m, index + 1, init_last_index, lai2, &ai2);
    }
  }

  /* if the cost has changed, record it and update the cost */
  if( delta_cost != 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, m->cost + delta_cost);
  if( DEBUG1 )
  {
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheRealPointActiveOrOpenToInactive returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointInactiveToActiveOrOpen(                                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,                       */
/*    KHE_TIME_GROUP_STATE new_state)                                        */
/*                                                                           */
/*  Change the state at this real point from inactive to active or open,     */
/*  updating everything accordingly.  Intervals will change.                 */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointInactiveToActiveOrOpen(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,
  KHE_TIME_GROUP_STATE new_state)
{
  KHE_AO_INTERVAL lai, rai;  int xx;  KHE_COST delta_cost;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheRealPointInactiveToActiveOrOpen(m, %d, %s)\n", index,
      KheTimeGroupStateShow(new_state));
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* change the state at index */
  KheRealPointSetState(m, index, new_state);

  /* get the intervals of the adjacent points (NULL if inactive or off end) */
  /* even though index is real, index - 1 and index + 1 could be virtual,   */
  /* or even beyond the virtual range if there is no history.  But the code */
  /* still works, because KhePointInterval returns NULL in those cases */
  lai = KhePointInterval(m, index - 1);
  rai = KhePointInterval(m, index + 1);

  /* adjust the intervals and determine delta_cost, the change in cost */
  if( lai == NULL )
  {
    if( rai == NULL )
    {
      /* make a new interval containing just index */
      xx = (new_state == KHE_ACTIVE ? index : index - 1);
      delta_cost = KheIntervalAdd(m, index, index, xx, &rai);
      KheRealPointSetInterval(m, index, rai);
    }
    else
    {
      /* increase the length of rai at the left end by one */
      xx = (new_state == KHE_OPEN ? index - 1 : rai->last_active_index);
      delta_cost = KheIntervalAdjust(m, rai, index, rai->last_index, xx);
      KheRealPointSetInterval(m, index, rai);
    }
  }
  else
  {
    if( rai == NULL )
    {
      /* increase the length of lai at the right end by one */
      if( new_state == KHE_ACTIVE && lai->last_active_index == lai->last_index )
	xx = lai->last_active_index + 1;
      else
	xx = lai->last_active_index;
      delta_cost = KheIntervalAdjust(m, lai, lai->first_index, index, xx);
      KheRealPointSetInterval(m, index, lai);
    }
    else
    {
      /* extend lai to where rai ends, and delete rai */
      if( new_state == KHE_ACTIVE && lai->last_active_index == lai->last_index )
	xx = rai->last_active_index;
      else
	xx = lai->last_active_index;
      delta_cost = KheIntervalAdjust(m, lai, lai->first_index, rai->last_index,
	xx) + KheIntervalDelete(m, rai);
      KheRangeSetInterval(m, index, lai->last_index, lai);
    }
  }

  /* if the cost has changed, update it */
  if( delta_cost != 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, m->cost + delta_cost);
  if( DEBUG1 )
  {
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointChangeState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,       */
/*    int index, KHE_TIME_GROUP_STATE new_state)                             */
/*                                                                           */
/*  Change the state of the real time group at index to new_state.           */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointChangeState(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int index, KHE_TIME_GROUP_STATE new_state)
{
  if( DEBUG6 )
    fprintf(stderr, "[ KheRealPointChangeState(%s, %d, %s)\n",
      KheLimitActiveIntervalsMonitorId(m), index,
      KheTimeGroupStateShow(new_state));
  switch( KheRealPointState(m, index) )
  {
    case KHE_INACTIVE:

      switch( new_state )
      {
	case KHE_INACTIVE:

	  /* nothing to do here */
	  break;

	case KHE_ACTIVE:
	case KHE_OPEN:

	  KheRealPointInactiveToActiveOrOpen(m, index, new_state);
	  break;
      }
      break;

    case KHE_ACTIVE:

      switch( new_state )
      {
	case KHE_INACTIVE:

	  KheRealPointActiveOrOpenToInactive(m, index);
	  break;

	case KHE_ACTIVE:

	  /* nothing to do here */
	  break;

	case KHE_OPEN:

	  KheRealPointActiveToOpen(m, index);
	  break;
      }
      break;

    case KHE_OPEN:

      switch( new_state )
      {
	case KHE_INACTIVE:

	  KheRealPointActiveOrOpenToInactive(m, index);
	  break;

	case KHE_ACTIVE:

	  KheRealPointOpenToActive(m, index);
	  break;

	case KHE_OPEN:

	  /* nothing to do here */
	  break;
      }
      break;
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheRealPointChangeState returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "virtual range state changes"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheHistoryBeforeRangeInactiveToActive(                          */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Convert the history_before range from inactive to active, and return     */
/*  the resulting change in cost.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheHistoryBeforeRangeInactiveToActive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;
  HnAssert(m->history_before_interval == NULL,
    "KheHistoryBeforeRangeInactiveToActive internal error");
  if( m->history > 0 )
  {
    /* get the interval just to the right of the history before range */
    ai = KhePointInterval(m, 0);
    if( ai == NULL )
    {
      /* make a new interval for the history range */
      delta_cost = KheIntervalAdd(m, - m->history, -1, -1, &ai);
    }
    else
    {
      /* extend rai to the left to cover the history range */
      delta_cost = KheIntervalAdjust(m, ai, - m->history,
	ai->last_index, ai->last_active_index);
    }
    m->history_before_interval = ai;
  }
  else
    delta_cost = 0;
  return delta_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheHistoryBeforeRangeActiveToInactive(                          */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Convert the history_before range from active to inactive, and return     */
/*  the resulting change in cost.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheHistoryBeforeRangeActiveToInactive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;
  if( m->history > 0 )
  {
    HnAssert(m->history_before_interval != NULL,
      "KheHistoryBeforeRangeActiveToInactive internal error");
    ai = m->history_before_interval;
    if( ai->last_index >= 0 )
    {
      /* adjust rai to remove the history range */
      delta_cost = KheIntervalAdjust(m, ai, 0, ai->last_index,
	ai->last_active_index);
    }
    else
    {
      /* delete rai */
      delta_cost = KheIntervalDelete(m, ai);
    }
    m->history_before_interval = NULL;
  }
  else
    delta_cost = 0;
  return delta_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheHistoryAfterRangeInactiveToOpen(                             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Convert the history_after range from inactive to open, and return        */
/*  the resulting change in cost.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheHistoryAfterRangeInactiveToOpen(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;  int tg_count;
  HnAssert(m->history_after_interval == NULL,
    "KheHistoryAfterRangeInactiveToOpen internal error");
  if( DEBUG1 )
    fprintf(stderr, "[ KheHistoryAfterRangeInactiveToOpen(%s)\n",
      KheLimitActiveIntervalsMonitorId(m));
  if( m->history_after > 0 )
  {
    /* get the interval just to the left of the history_after range */
    tg_count = m->time_group_info_count;
    ai = KhePointInterval(m, tg_count - 1);
    if( ai == NULL )
    {
      /* make a new interval for the history_after range */
      delta_cost = KheIntervalAdd(m, tg_count, tg_count + m->history_after - 1,
	tg_count - 1, &ai);
    }
    else
    {
      /* extend ai to the right to cover the history_after range */
      delta_cost = KheIntervalAdjust(m, ai, ai->first_index,
	tg_count + m->history_after - 1, ai->last_active_index);
    }
    m->history_after_interval = ai;
  }
  else
    delta_cost = 0;
  if( DEBUG1 )
    fprintf(stderr, "] KheHistoryAfterRangeInactiveToOpen returning\n");
  return delta_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheHistoryAfterRangeOpenToInactive(                             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Convert the history_after range from open to inactive, and return        */
/*  the resulting change in cost.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheHistoryAfterRangeOpenToInactive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_AO_INTERVAL ai;  KHE_COST delta_cost;  int tg_count;
  if( m->history_after > 0 )
  {
    HnAssert(m->history_after_interval != NULL,
      "KheHistoryAfterRangeOpenToInactive internal error");
    ai = m->history_after_interval;
    tg_count = m->time_group_info_count;
    if( ai->first_index < tg_count )
    {
      /* adjust ai to remove the history range */
      delta_cost = KheIntervalAdjust(m, ai, ai->first_index,
	tg_count - 1, ai->last_active_index);
    }
    else
    {
      /* delete ai */
      delta_cost = KheIntervalDelete(m, ai);
    }
    m->history_after_interval = NULL;
  }
  else
    delta_cost = 0;
  return delta_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDev(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int known_active,       */
/*    int possible_active)                                                   */
/*                                                                           */
/*  Return the deviation of an active interval with at least known_active    */
/*  and at most known_active + possible_active active time groups.           */
/*                                                                           */
/*****************************************************************************/

static int KheDev(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int known_active,
  int possible_active)
{
  int res;
  res = known_active - m->maximum;
  if( res > 0 )
    return res;
  res = m->minimum - known_active - possible_active;
  if( res > 0 )
    return res;
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTiltCost(KHE_TIME_GROUP tg)                                       */
/*                                                                           */
/*  Return the tilt cost of tg.  This is just the index of the first         */
/*  time of tg, or 0 if tg is empty.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheTiltCost(KHE_TIME_GROUP tg)
{
  int index;
  if( KheTimeGroupTimeCount(tg) == 0 )
    index = 0;
  else
    index = KheTimeIndex(KheTimeGroupTime(tg, 0));
  return KheCost(0, index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR KheLimitActiveIntervalsMonitorMake(   */
/*    KHE_RESOURCE_IN_SOLN rs, int offset,                                   */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Make a new limit active intervals monitor for (rs, offset), monitoring c.*/
/*                                                                           */
/*****************************************************************************/
static void KheLimitActiveIntervalsMonitorCheck(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m);

KHE_LIMIT_ACTIVE_INTERVALS_MONITOR KheLimitActiveIntervalsMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR res;   KHE_SOLN soln;  KHE_RESOURCE r;
  int dev, i, count;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TIME_GROUP_INFO mi;  HA_ARENA a;  KHE_INSTANCE ins;
  r = KheResourceInSolnResource(rs);
  soln = KheResourceInSolnSoln(rs);
  ins = KheSolnInstance(soln);
  a = KheSolnArena(soln);
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  res = HaAlloc(a, sizeof(struct khe_limit_active_intervals_monitor_rec)
    + (count - 1) * sizeof(KHE_TIME_GROUP_INFO));
  /* HaMake(res, a); */
  /* MMakeCount(res, 33); */
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->resource_in_soln = rs;
  res->constraint = c;
  res->offset = offset;
  res->minimum = KheLimitActiveIntervalsConstraintMinimum(c);
  res->maximum = KheLimitActiveIntervalsConstraintMaximum(c);
  res->history_before = KheLimitActiveIntervalsConstraintHistoryBefore(c);
  res->history_after = KheLimitActiveIntervalsConstraintHistoryAfter(c);
  res->history = KheLimitActiveIntervalsConstraintHistory(c, r);
  res->at_max_limit_count = 0;
  if( KheInstanceTimeCount(ins) > 0 )
    res->sweep_time = KheInstanceTime(ins, KheInstanceTimeCount(ins) - 1);
  else
    res->sweep_time = NULL;
  res->sweep_index = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  rtm = KheResourceInSolnTimetableMonitor(rs);
  res->tilt = false;
  res->time_group_info_count = res->sweep_index;
  for( i = 0;  i < res->time_group_info_count;  i++ )
  {
    tg = KheLimitActiveIntervalsConstraintTimeGroup(c, i, offset, &mi.polarity);
    mi.monitored_time_group =
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg);
    mi.state = KHE_INACTIVE;
    mi.not_busy_state = KHE_OPEN;
    mi.interval = NULL;
    mi.tilt_cost = KheTiltCost(tg);
    res->time_group_info[i] = mi;
  }
  HaArrayInit(res->intervals[KHE_FREE], a);
  HaArrayInit(res->intervals[KHE_GOOD], a);
  HaArrayInit(res->intervals[KHE_BAD], a);
  res->history_before_interval = NULL;
  res->history_after_interval = NULL;
  dev = res->history == 0 ? 0 : KheDev(res, res->history,
    res->history_after + res->sweep_index);
  res->history_adjustment = KheMonitorDevToCost((KHE_MONITOR) res, dev);
  /* res->frame = KheFrameMakeNull(); */
  res->first_time = NULL;
  res->last_time = NULL;
  res->copy = NULL;
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  KheLimitActiveIntervalsMonitorCheck(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR                                       */
/*    KheLimitActiveIntervalsMonitorCopyPhase1(                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, HA_ARENA a)                      */
/*                                                                           */
/*  Carry out Phase 1 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_ACTIVE_INTERVALS_MONITOR KheLimitActiveIntervalsMonitorCopyPhase1(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR copy;  int i;  KHE_TIME_GROUP_INFO mi, mi2;
  KHE_AO_INTERVAL ai;
  if( m->copy == NULL )
  {
    copy = HaAlloc(a, sizeof(struct khe_limit_active_intervals_monitor_rec)
      + (m->time_group_info_count - 1) * sizeof(KHE_TIME_GROUP_INFO));
    /* MMakeCount(copy, 33); */
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->resource_in_soln =
      KheResourceInSolnCopyPhase1(m->resource_in_soln, a);
    copy->constraint = m->constraint;
    copy->offset = m->offset;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->history_before = m->history_before;
    copy->history_after = m->history_after;
    copy->history = m->history;
    copy->at_max_limit_count = m->at_max_limit_count;
    copy->sweep_index = m->sweep_index;
    copy->sweep_time = m->sweep_time;
    copy->tilt = m->tilt;
    copy->time_group_info_count = m->time_group_info_count;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mi = m->time_group_info[i];
      mi2.monitored_time_group =
	KheMonitoredTimeGroupCopyPhase1(mi.monitored_time_group, a);
      mi2.polarity = mi.polarity;
      mi2.state = mi.state;
      mi2.not_busy_state = mi.not_busy_state;
      mi2.interval = (mi.interval == NULL ? NULL :
        KheIntervalCopyPhase1(mi.interval, a));
      copy->time_group_info[i] = mi2;
    }
    HaArrayInit(copy->intervals[KHE_FREE], a);
    HaArrayInit(copy->intervals[KHE_GOOD], a);
    HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
      HaArrayAddLast(copy->intervals[KHE_GOOD], KheIntervalCopyPhase1(ai, a));
    HaArrayInit(copy->intervals[KHE_BAD], a);
    HaArrayForEach(m->intervals[KHE_BAD], ai, i)
      HaArrayAddLast(copy->intervals[KHE_BAD], KheIntervalCopyPhase1(ai, a));
    copy->history_before_interval = (m->history_before_interval == NULL ?
      NULL : KheIntervalCopyPhase1(m->history_before_interval, a));
    copy->history_after_interval = (m->history_after_interval == NULL ?
      NULL : KheIntervalCopyPhase1(m->history_after_interval, a));
    copy->history_adjustment = m->history_adjustment;
    /* copy->frame = KheFrameMakeNull(); */
    copy->first_time = m->first_time;
    copy->last_time = m->last_time;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorCopyPhase2(                           */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorCopyPhase2(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_TIME_GROUP_INFO mi;  KHE_AO_INTERVAL ai;
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mi = m->time_group_info[i];
      KheMonitoredTimeGroupCopyPhase2(mi.monitored_time_group);
      if( mi.interval != NULL )
	KheIntervalCopyPhase2(mi.interval);
    }
    HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
      KheIntervalCopyPhase2(ai);
    HaArrayForEach(m->intervals[KHE_BAD], ai, i)
      KheIntervalCopyPhase2(ai);
    if( m->history_before_interval != NULL )
      KheIntervalCopyPhase2(m->history_before_interval);
    if( m->history_after_interval != NULL )
      KheIntervalCopyPhase2(m->history_after_interval);
    KheLimitActiveIntervalsMonitorCheck(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorDelete(                               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitActiveIntervalsMonitorDelete(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_AO_INTERVAL ai;  int i;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheLimitActiveIntervalsMonitorDelete(m)\n");
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }
  KheLimitActiveIntervalsMonitorCheck(m);
  if( !KheFrameIsNull(m->frame) )
    KheFrameDelete(m->frame);
  if( m->attached )
    KheLimitActiveIntervalsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheResourceInSolnDeleteMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
    KheIntervalFree(ai);
  HaArrayForEach(m->intervals[KHE_BAD], ai, i)
    KheIntervalFree(ai);
  ** *** dodgy but the problem seems to be time group monitors going first
  HnAssert(HaArrayCount(m->intervals[KHE_GOOD]) == 0,
    "KheLimitActiveIntervalsMonitorDelete internal error 1");
  HnAssert(HaArrayCount(m->intervals[KHE_BAD]) == 0,
    "KheLimitActiveIntervalsMonitorDelete internal error 2");
  *** **
  HaArrayForEach(m->intervals[KHE_FREE], ai, i)
    KheIntervalFree(ai);
  MFree(m);
  if( DEBUG2 )
    fprintf(stderr, "] KheLimitActiveIntervalsMonitorDelete returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheLimitActiveIntervalsMonitorResourceInSoln(       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheLimitActiveIntervalsMonitorResourceInSoln(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->resource_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT                                    */
/*    KheLimitActiveIntervalsMonitorConstraint(                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the contraint that m is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT KheLimitActiveIntervalsMonitorConstraint(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitActiveIntervalsMonitorResource(                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the resource that m is monitoring.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitActiveIntervalsMonitorResource(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorMinimum(                               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the minimum limit from the constraint.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorMinimum(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorMaximum(                               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the maximum limit from the constraint.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorMaximum(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorHistoryBefore(                         */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the history value stored in m.                                    */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->history_before;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorHistoryAfter(                          */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the history value stored in m.                                    */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->history_after;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorHistory(                               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the history  stored in m.                                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->history;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorOffset(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the offset that m is monitoring.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorOffset(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorCheck(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Check m for consistency.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheLimitActiveIntervalsMonitorCheck(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_AO_INTERVAL ai;
  if( DEBUG_CHECK )
  {
    for( i = 0;  i < m->time_group_info_count;  i++ )
      KheRealPointCheck(m, i);
    HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
      KheIntervalCheck(ai, KHE_GOOD, m);
    HaArrayForEach(m->intervals[KHE_BAD], ai, i)
      KheIntervalCheck(ai, KHE_BAD, m);
    /* check total cost - I never got around to doing this */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reporting state during the solve"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorTimeGroupCount(                        */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the number of time groups of m.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorTimeGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->time_group_info_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitActiveIntervalsMonitorTimeGroup(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_POLARITY *po)         */
/*                                                                           */
/*  Return the i'th time group of m, and its polarity.                       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitActiveIntervalsMonitorTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_POLARITY *po)
{
  KHE_MONITORED_TIME_GROUP mtg;
  mtg = KheRealPointMonitoredTimeGroup(m, i, po);
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorRange(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,                                  */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLimitActiveIntervalsMonitorSweepTimeRange
void KheLimitActiveIntervalsMonitorRange(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mtg = m->time_group_info[i].monitored_time_group;
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      count = KheTimeGroupTimeCount(tg);
      if( count > 0 )
      {
	index = KheTimeIndex(KheTimeGroupTime(tg, 0));
	if( index < first_index )
          first_index = index;
	index = KheTimeIndex(KheTimeGroupTime(tg, count - 1));
	if( index > last_index )
          last_index = index;
      }
    }
    if( first_index < INT_MAX )
    {
      ins = KheSolnInstance(m->soln);
      m->first_time = KheInstanceTime(ins, first_index);
      m->last_time = KheInstanceTime(ins, last_index);
    }
  }
  *first_time = m->first_time;
  *last_time = m->last_time;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorTimeGroupIsActive(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_TIME_GROUP *tg,       */
/*    KHE_POLARITY *po, int *busy_count)                                     */
/*                                                                           */
/*  Set *tg to the i'th time group, *po to its polarity, *busy_count to its  */
/*  number of busy times, and return true when it is active; that is, when   */
/*                                                                           */
/*    (*po == KHE_NEGATIVE) == (*busy_count == 0)                            */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsMonitorTimeGroupIsActive(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, KHE_TIME_GROUP *tg,
  KHE_POLARITY *po, int *busy_count)
{
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  mtg = KheRealPointMonitoredTimeGroup(m, i, po);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  *tg = KheMonitoredTimeGroupTimeGroup(mtg);
  *busy_count = busy_and_idle->busy_count;
  return (*po == KHE_NEGATIVE) == (*busy_count == 0);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorDefectiveIntervalCount(                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the number of defective intervals of m.                           */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorDefectiveIntervalCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return HaArrayCount(m->intervals[KHE_BAD]);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorDefectiveInterval(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, int *history,             */
/*    int *first_index, int *last_index, int *history_after, bool *too_long) */
/*                                                                           */
/*  Return the key attributes of the i'th defective interval of m:           */
/*                                                                           */
/*    *history_before    If the interval includes the first time group,      */
/*                       the part of its length from before the first        */
/*                       time group, otherwise 0.                            */
/*                                                                           */
/*    *first_index       The index of the first time group in the interval,  */
/*                       not including virtual time groups from before the   */
/*                       interval began.                                     */
/*                                                                           */
/*    *last_index        The index of the last time group in the interval,   */
/*                       not including virtual time groups from after the    */
/*                       interval ends.                                      */
/*                                                                           */
/*                       This value can be negative, indicating that the     */
/*                       defective interval lies entirely within history.    */
/*                                                                           */
/*    *history_after     If the interval includes the last time group,       */
/*                       the possible extra length from after the last       */
/*                       time group (to be included in the length only       */
/*                       when comparing with a minimum limit), else 0.       */
/*                                                                           */
/*    *too_long          Since this is a defective interval, it must be      */
/*                       either too long or too short.  This parameter is    */
/*                       true if it is too long.                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorDefectiveInterval(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, int *history_before,
  int *first_index, int *last_index, int *history_after, bool *too_long)
{
  KHE_AO_INTERVAL ai;  int actual_last_index;

  /* get the i'th defective interval */
  ai = HaArray(m->intervals[KHE_BAD], i);

  /* set *first index, increased to 0 if required, and set */
  /* *history_before to whatever is lost by increasing *first_index */
  *first_index = max(ai->first_index, 0);
  *history_before = *first_index - ai->first_index;

  /* set actual_last_index and *too_long depending on too long or too short */
  if( KheIntervalActiveLength(ai) > m->maximum )
  {
    actual_last_index = ai->last_active_index;
    *too_long = true;
  }
  else if( KheIntervalLength(ai) < m->minimum )
  {
    actual_last_index = ai->last_index;
    *too_long = false;
  }
  else
  {
    HnAbort("KheLimitActiveIntervalsMonitorDefectiveInterval internal error");
    actual_last_index = ai->last_index;	/* keep compiler happy */
    *too_long = false;  		/* keep compiler happy */
  }

  /* set *last_index, decreased to m->time_group_info_count - 1 if reqd */
  /* and set *history_after to whatever is lost by decreasing *last_index */
  *last_index = min(actual_last_index, m->time_group_info_count - 1);
  if( DEBUG10 && *last_index < 0 )
  {
    char buff[100];
    fprintf(stderr,
      "[ KheLimitActiveIntervalsMonitorDefectiveInterval(%s: %s, %d)\n",
      KheInstanceId(KheSolnInstance(m->soln)), KheMonitorId((KHE_MONITOR)m), i);
    fprintf(stderr, "  %s\n", KheIntervalShow(ai, m, buff));
    fprintf(stderr, "  *history_before = %d, *first_index = %d,\n",
      *history_before, *first_index);
    fprintf(stderr, "  *last_index = %d, *history_after = %d,\n",
      *last_index, *history_after);
    fprintf(stderr, "]\n");
    HnAbort("KheLimitActiveIntervalsMonitorDefectiveInterval aborting now");
  }
  *history_after = actual_last_index - *last_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorAtMaxLimitCount(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the number of non-defective intervals whose length equals         */
/*  the maximum limit.                                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorAtMaxLimitCount(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  /* ***
  return m->at_max_limit_count;
  *** */
  KHE_AO_INTERVAL ai;  int i, res;
  res = 0;
  HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
    if( ai->first_index < m->sweep_index &&
	KheIntervalActiveLength(ai) == m->maximum )
      res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*  The definitions of sweep time and sweep index from cluster busy times    */
/*  monitors apply here as well.                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorTimeRange(                            */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME *first_time,            */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Find the time range of m.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsMonitorTimeRange(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mtg = m->time_group_info[i].monitored_time_group;
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      count = KheTimeGroupTimeCount(tg);
      if( count > 0 )
      {
	index = KheTimeIndex(KheTimeGroupTime(tg, 0));
	if( index < first_index )
          first_index = index;
	index = KheTimeIndex(KheTimeGroupTime(tg, count - 1));
	if( index > last_index )
          last_index = index;
      }
    }
    if( first_index < INT_MAX )
    {
      ins = KheSolnInstance(m->soln);
      m->first_time = KheInstanceTime(ins, first_index);
      m->last_time = KheInstanceTime(ins, last_index);
    }
  }
  *first_time = m->first_time;
  *last_time = m->last_time;
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorFindSweepIndex(                        */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME sweep_time)             */
/*                                                                           */
/*  Find the sweep index corresponding to sweep_time.                        */
/*                                                                           */
/*  When this function is called, m->sweep_time and m->sweep_index are       */
/*  well-defined, and m->sweep_index is correct for m->sweep_time.  This     */
/*  fact is used in this function to speed up the search for the new index.  */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveIntervalsMonitorFindSweepIndex(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME sweep_time)
{
  int i;  KHE_TIME_GROUP tg;  KHE_TIME t;
  KHE_MONITORED_TIME_GROUP mtg;  KHE_POLARITY po;
  if( sweep_time == NULL )
  {
    /* nothing is being monitored except history */
    return 0;
  }
  else
  {
    /* find i, the index to start the search from, either existing or 0 */
    if( m->sweep_time != NULL &&
        KheTimeIndex(m->sweep_time) <= KheTimeIndex(sweep_time) )
      i = m->sweep_index;
    else
      i = 0;

    /* search for the index from i */
    for( ;  i < m->time_group_info_count;  i++ )
    {
      mtg = KheRealPointMonitoredTimeGroup(m, i, &po);
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      if( KheTimeGroupTimeCount(tg) > 0 )
      {
	t = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
	if( KheTimeIndex(t) > KheTimeIndex(sweep_time) )
	  break;
      }
    }

    /* all done */
    return i;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointSweepIndexHasChanged(                                   */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)                       */
/*                                                                           */
/*  There is a new sweep index in m->sweep_index, m is attached, and this    */
/*  could affect the state of the real point at index, and also of the       */
/*  interval enclosing that point.  Update everything.                       */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointSweepIndexHasChanged(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index)
{
  int busy;  KHE_TIME_GROUP_STATE new_state;  KHE_BUSY_AND_IDLE busy_and_idle;
  KHE_AO_INTERVAL ai;  KHE_MONITORED_TIME_GROUP mtg;  KHE_POLARITY po;
  mtg = KheRealPointMonitoredTimeGroup(m, index, &po);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  busy = busy_and_idle->busy_count;
  new_state = KheRealAttachedPointNewState(m, index, busy);
  if( DEBUG7 )
    fprintf(stderr, "KheRealPointSweepIndexHasChanged(%s, %d) %s -> %s\n",
      KheLimitActiveIntervalsMonitorId(m), index,
      KheTimeGroupStateShow(KheRealPointState(m, index)),
      KheTimeGroupStateShow(new_state));
  KheRealPointChangeState(m, index, new_state);
  ai = KheRealPointInterval(m, index);
  if( ai != NULL )
    KheIntervalSweepIndexHasChanged(m, ai);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorSetSweepIndex(                        */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int sweep_index)                 */
/*                                                                           */
/*  Set the sweep index of m to sweep_index, including calling               */
/*  KheRealPointSweepIndexHasChanged for all time groups affected.           */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorSetSweepIndex(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int sweep_index)
{
  int i, old_sweep_index;
  if( DEBUG3 )
    fprintf(stderr, "   KheLimitActiveIntervalsMonitorSetSweepIndex(%s, %d)\n",
      KheMonitorId((KHE_MONITOR) m), sweep_index);
  KheLimitActiveIntervalsMonitorCheck(m);
 HnAssert(0 <= sweep_index && sweep_index <= m->time_group_info_count,
    "KheLimitActiveIntervalsMonitorSetSweepIndex: sweep index (%d) out of"
    " range (0 .. %d)", sweep_index, m->time_group_info_count);
  old_sweep_index = m->sweep_index;
  m->sweep_index = sweep_index;
  if( m->attached )
  {
    if( sweep_index < old_sweep_index )
    {
      /* points from sweep_index to old_sweep_index - 1 might change state */
      for( i = old_sweep_index - 1;  i >= sweep_index;  i-- )
	KheRealPointSweepIndexHasChanged(m, i);
    }
    else
    {
      /* points from old_sweep_index to sweep_index - 1 might change state */
      for( i = old_sweep_index;  i < sweep_index;  i++ )
	KheRealPointSweepIndexHasChanged(m, i);
    }
  }
  KheLimitActiveIntervalsMonitorCheck(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorSetSweepTime(                         */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME t)                      */
/*                                                                           */
/*  Set the sweep index of m to the smallest index such that the time        */
/*  group at that index contains a time that is later than t.  If there      */
/*  is no such time group, set the sweep index to the number of time         */
/*  groups (this cuts off nothing).                                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorSetSweepTime(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME t)
{
  int new_sweep_index;
  new_sweep_index = KheLimitActiveIntervalsMonitorFindSweepIndex(m, t);
  KheLimitActiveIntervalsMonitorSetSweepIndex(m, new_sweep_index);
  m->sweep_time = t;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheLimitActiveIntervalsMonitorInitialSweepTime(                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the smallest time t such that cutting off at t is not the same    */
/*  as cutting off at index 0.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TIME KheLimitActiveIntervalsMonitorInitialSweepTime(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_INSTANCE ins;  KHE_TIME_GROUP tg;  KHE_MONITORED_TIME_GROUP mtg;
  KHE_POLARITY po;

  if( m->time_group_info_count == 0 )
  {
    ** no time groups, so no time can produce a different sweep **
    return NULL;
  }
  else
  {
    mtg = KheRealPointMonitoredTimeGroup(m, 0, &po);
    tg = KheMonitoredTimeGroupTimeGroup(mtg);
    if( KheTimeGroupTimeCount(tg) == 0 )
    {
      ** even cutting off at the first time will be different **
      ins = KheSolnInstance(m->soln);
      if( KheInstanceTimeCount(ins) == 0 )
	return NULL;
      else
	return KheInstanceTime(ins, 0);
    }
    else
    {
      ** cutting off at the last time of tg will be different **
      return KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorSweepTimeRange(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME *first_time,            */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Find the sweep time range of m.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsMonitorSweepTimeRange(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  return KheLimitActiveIntervalsMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorSetNotBusyState(                      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, bool active)              */
/*                                                                           */
/*  Inform m that time group i should be considered active (if active is     */
/*  true) or inactive (if active is false) when it is not busy and it lies   */
/*  beyond the sweep index.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorSetNotBusyState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i, bool active)
{
  KHE_TIME_GROUP_STATE new_state;  int busy_count;  KHE_POLARITY po;
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  KheLimitActiveIntervalsMonitorCheck(m);
  m->time_group_info[i].not_busy_state = (active ? KHE_ACTIVE : KHE_INACTIVE);
  if( m->attached )
  {
    mtg = KheRealPointMonitoredTimeGroup(m, i, &po);
    busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
    busy_count = busy_and_idle->busy_count;
    new_state = KheRealAttachedPointNewState(m, i, busy_count);
    KheRealPointChangeState(m, i, new_state);
  }
  KheLimitActiveIntervalsMonitorCheck(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorClearNotBusyState(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i)                           */
/*                                                                           */
/*  Undo the corresponding KheLimitActiveIntervalsMonitorSetNotBusyState.    */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorClearNotBusyState(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int i)
{
  KHE_TIME_GROUP_STATE new_state;  int busy_count;  KHE_POLARITY po;
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  KheLimitActiveIntervalsMonitorCheck(m);
  m->time_group_info[i].not_busy_state = KHE_OPEN;
  if( m->attached )
  {
    mtg = KheRealPointMonitoredTimeGroup(m, i, &po);
    busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
    busy_count = busy_and_idle->busy_count;
    new_state = KheRealAttachedPointNewState(m, i, busy_count);
    KheRealPointChangeState(m, i, new_state);
  }
  KheLimitActiveIntervalsMonitorCheck(m);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tilt"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorSetTilt(                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Set the tilt flag.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorSetTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  if( !m->tilt )
  {
    if( m->attached )
    {
      KheLimitActiveIntervalsMonitorDetachFromSoln(m);
      m->tilt = true;
      KheLimitActiveIntervalsMonitorAttachToSoln(m);
    }
    else
      m->tilt = true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorClearTilt(                            */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Clear the tilt flag.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorClearTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  if( m->tilt )
  {
    if( m->attached )
    {
      KheLimitActiveIntervalsMonitorDetachFromSoln(m);
      m->tilt = false;
      KheLimitActiveIntervalsMonitorAttachToSoln(m);
    }
    else
      m->tilt = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorTilt(                                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the tilt flag.                                                    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsMonitorTilt(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  return m->tilt;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "frame"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheLimitActiveIntervalsMonitorFrame(                           */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the frame corresponding to m.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_FRAME KheLimitActiveIntervalsMonitorFrame(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;  KHE_FRAME_MAKE fm;
  KHE_SOLN soln;
  if( KheFrameIsNull(m->frame) )
  {
    soln = KheMonitorSoln((KHE_MONITOR) m);
    if( DEBUG9 )
      fprintf(stderr,
      "  calling KheFrameMa keBegin from KheLimitActiveIntervalsMonitorFrame\n");
    fm = KheFrameM akeBegin(soln);
    for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(m);  i++ )
    {
      tg = KheLimitActiveIntervalsMonitorTimeGroup(m, i, &po);
      KheFrameMakeAddTimeGroup(fm, tg, po);
    }
    m->frame = KheFrameMakeEnd(fm, false);
  }
  return m->frame;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When m is unlinked, there are no active intervals except the history.    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorAttachToSoln(                         */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorAttachToSoln(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_COST delta_cost;  int i;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheLimitActiveIntervalsMonitorAttachToSoln(m)\n");
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* check that current state is as expected when unattached */
  KheLimitActiveIntervalsMonitorCheck(m);
  HnAssert(m->cost == 0,
    "KheLimitActiveIntervalsMonitorAttachToSoln internal error 1");
  HnAssert(HaArrayCount(m->intervals[KHE_GOOD]) == 0,
    "KheLimitActiveIntervalsMonitorAttachToSoln internal error 2");
  HnAssert(HaArrayCount(m->intervals[KHE_BAD]) == 0,
    "KheLimitActiveIntervalsMonitorAttachToSoln internal error 3");

  /* mark m attached */
  m->attached = true;

  /* update history intervals; include history adjustment in delta_cost */
  delta_cost = KheHistoryBeforeRangeInactiveToActive(m) +
    KheHistoryAfterRangeInactiveToOpen(m) - m->history_adjustment;
  if( delta_cost != 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, delta_cost);

  /* attach each monitored time group; it will call back with a busy count */
  for( i = 0;  i < m->time_group_info_count;  i++ )
    KheMonitoredTimeGroupAttachMonitor(
      m->time_group_info[i].monitored_time_group, (KHE_MONITOR) m, i);
  KheLimitActiveIntervalsMonitorCheck(m);

  if( DEBUG2 )
  {
    fprintf(stderr, "  at end:\n");
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheLimitActiveIntervalsMonitorAttachToSoln(m)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorDetachFromSoln(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorDetachFromSoln(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  KHE_COST delta_cost;  int i;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheLimitActiveIntervalsMonitorDetachFromSoln(m)\n");
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
  }

  /* detach each monitored time group; it will call back to change state */
  KheLimitActiveIntervalsMonitorCheck(m);
  for( i = 0;  i < m->time_group_info_count;  i++ )
    KheMonitoredTimeGroupDetachMonitor(
      m->time_group_info[i].monitored_time_group, (KHE_MONITOR) m, i);

  /* update history intervals; include history adjustment in delta_cost */
  delta_cost = KheHistoryBeforeRangeActiveToInactive(m) +
    KheHistoryAfterRangeOpenToInactive(m) + m->history_adjustment;
  if( delta_cost != 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, m->cost + delta_cost);

  /* mark m unattached */
  m->attached = false;

  /* check that current state is as expected when unattached */
  HnAssert(m->cost == 0,
    "KheLimitActiveIntervalsMonitorDetachFromSoln internal error 1");
  HnAssert(HaArrayCount(m->intervals[KHE_GOOD]) == 0,
    "KheLimitActiveIntervalsMonitorDetachFromSoln internal error 2");
  HnAssert(HaArrayCount(m->intervals[KHE_BAD]) == 0,
    "KheLimitActiveIntervalsMonitorDetachFromSoln internal error 3");
  KheLimitActiveIntervalsMonitorCheck(m);

  if( DEBUG2 )
  {
    fprintf(stderr, "  at end:\n");
    KheLimitActiveIntervalsMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheLimitActiveIntervalsMonitorDetachFromSoln(m)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorAddBusyAndIdle(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,                       */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add a busy value for one monitored time group.                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorAddBusyAndIdle(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  KHE_TIME_GROUP_STATE new_state;
  /* not safe to check here KheLimitActiveIntervalsMonitorCheck(m); */
  HnAssert(m->attached,
    "KheLimitActiveIntervalsMonitorAddBusyAndIdle internal error 1");
  new_state = KheRealAttachedPointNewState(m, index, busy_and_idle->busy_count);
  KheRealPointChangeState(m, index, new_state);
  /* not safe here either KheLimitActiveIntervalsMonitorCheck(m); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorDeleteBusyAndIdle(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,                       */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Remove the busy value of one monitored time group.                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorDeleteBusyAndIdle(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  /* *** actually this flag will still be set when this is called
  HnAssert(!m->attached,
    "KheLimitActiveIntervalsMonitorDeleteBusyAndIdle internal error 1");
  *** */
  /* not safe here KheLimitActiveIntervalsMonitorCheck(m); */
  KheRealPointChangeState(m, index, KHE_INACTIVE);
  /* not safe here KheLimitActiveIntervalsMonitorCheck(m); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorChangeBusyAndIdle(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,                       */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change the value of one monitored time group.                            */
/*                                                                           */
/*  Implementation note.  This code is correct when old_busy_count and       */
/*  new_busy_count are equal, as is possible, so no special case is made.    */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorChangeBusyAndIdle(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle,
  KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  KHE_TIME_GROUP_STATE new_state;
  /* wrong to check here KheLimitActiveIntervalsMonitorCheck(m); */
  HnAssert(m->attached,
    "KheLimitActiveIntervalsMonitorChangeBusyAndIdle internal error 1");
  new_state = KheRealAttachedPointNewState(m, index,
    new_busy_and_idle->busy_count);
  KheRealPointChangeState(m, index, new_state);
  KheLimitActiveIntervalsMonitorCheck(m);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsMonitorDeviation(                             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsMonitorDeviation(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i, res;  KHE_AO_INTERVAL ai;
  res = 0;
  HaArrayForEach(m->intervals[KHE_BAD], ai, i)
    res += ai->deviation;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheActiveTimeGroupName(KHE_TIME_GROUP tg)                          */
/*                                                                           */
/*  Return an adjusted form of the name of tg.                               */
/*                                                                           */
/*****************************************************************************/

static char *KheActiveTimeGroupName(KHE_TIME_GROUP tg)
{
  if( KheTimeGroupTimeCount(tg) == 1 )
    return KheTimeName(KheTimeGroupTime(tg, 0));
  else if( KheTimeGroupId(tg) != NULL )
    return KheTimeGroupId(tg);
  else
    return "?";
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddActiveTimeGroups(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,        */
/*    KHE_AO_INTERVAL ai, int last_index, HA_ARRAY_NCHAR *ac, char sign,     */
/*    bool *first)                                                           */
/*                                                                           */
/*  Add string representations of the active time groups of ai to *ac.       */
/*                                                                           */
/*****************************************************************************/

static void KheAddActiveTimeGroups(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_AO_INTERVAL ai, int last_index, HA_ARRAY_NCHAR *ac, char sign, bool *first)
{
  int i;  KHE_POLARITY po;  KHE_MONITORED_TIME_GROUP mtg;
  char *name;  KHE_TIME_GROUP tg;
  for( i = ai->first_index;  i <= last_index;  i++ )
  {
    if( !*first )
      HnStringAdd(ac, "  %c  ", sign);
    if( i < 0 )
    {
      HnStringAdd(ac, "History %d", - ai->first_index);
      i = -1;
    }
    else if( i >= m->time_group_info_count )
    {
      HnStringAdd(ac, "History %d",
	last_index - m->time_group_info_count + 1);
      i = last_index;
    }
    else
    {
      mtg = KheRealPointMonitoredTimeGroup(m, i, &po);
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      name = KheActiveTimeGroupName(tg);
      HnStringAdd(ac, "%s%s", po == KHE_NEGATIVE ? "*" : "", name);
    }
    *first = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitActiveIntervalsMonitorDeviationDescription(                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitActiveIntervalsMonitorDeviationDescription(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i;  KHE_AO_INTERVAL ai;  bool first;  HA_ARENA a;
  if( HaArrayCount(m->intervals[KHE_BAD]) == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HaArrayForEach(m->intervals[KHE_BAD], ai, i)
    {
      if( i > 0 )
	HnStringAdd(&ac, "; ");
      if( KheIntervalLength(ai) < m->minimum )
      {
	HnStringAdd(&ac, "%d", m->minimum);
	first = false;
	KheAddActiveTimeGroups(m, ai, ai->last_index, &ac, '-', &first);
      }
      else if( KheIntervalActiveLength(ai) > m->maximum )
      {
	first = true;
	KheAddActiveTimeGroups(m, ai, ai->last_active_index, &ac, '+', &first);
	HnStringAdd(&ac, "  -  %d", m->maximum);
      }
      else
	HnAbort("KheLimitActiveIntervalsMonitorDeviationDescription "
	  "internal error");
    }
    if( m->history_adjustment > 0 )
      HnStringAdd(&ac, " (adjust %.5f)",KheCostShow(m->history_adjustment));
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitActiveIntervalsMonitorPointOfApplication(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitActiveIntervalsMonitorPointOfApplication(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  char *name;  HA_ARENA a;
  name = KheResourceName(KheLimitActiveIntervalsMonitorResource(m));
  if( KheLimitActiveIntervalsConstraintAppliesToTimeGroup(m->constraint)!=NULL )
  {
    a = KheSolnArena(m->soln);
    return HnStringMake(a, "%s offset %d", name, m->offset);
  }
  else
    return name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitActiveIntervalsMonitorId(                                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)                                  */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitActiveIntervalsMonitorId(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheLimitActiveIntervalsMonitorResource(m));
    a = KheSolnArena(m->soln);
    if( m->offset == 0 )
      m->id = HnStringMake(a, "%s/%s", constraint_id, resource_id);
    else
      m->id = HnStringMake(a, "%s/%s/%d", constraint_id, resource_id,m->offset);
  }
  return m->id;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsMonitorDebug(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, int verbosity, int indent,       */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsMonitorDebug(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  KHE_AO_INTERVAL ai;  int i;  char buff[100];
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, "%s", " LAIM");
    if( m->sweep_index < m->time_group_info_count )
      fprintf(fp, " (sweep %d)", m->sweep_index);
    if( m->minimum == 0 )
      fprintf(fp, " (max %d)", m->maximum);
    else if( m->maximum >= m->time_group_info_count )
      fprintf(fp, " (min %d)", m->minimum);
    else
      fprintf(fp, " (min %d, max %d)", m->minimum, m->maximum);
    HaArrayForEach(m->intervals[KHE_GOOD], ai, i)
      fprintf(fp, "%s", KheIntervalShow(ai, m, buff));
    if( HaArrayCount(m->intervals[KHE_BAD]) > 0 )
      fprintf(fp, "%s", "$");
    HaArrayForEach(m->intervals[KHE_BAD], ai, i)
      fprintf(fp, "%s", KheIntervalShow(ai, m, buff));
    if( m->history_adjustment > 0 )
      fprintf(fp, ", adjust %.5f", KheCostShow(m->history_adjustment));
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

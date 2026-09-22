
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
/*  FILE:         khe_cluster_busy_times_monitor.c                           */
/*  DESCRIPTION:  A cluster busy times monitor                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0


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
/*      monitor is not attached).                                            */
/*                                                                           */
/*    KHE_ACTIVE                                                             */
/*      The time group is active.  It contributes a value of 1 to the        */
/*      active_group_count attribute.                                        */
/*                                                                           */
/*    KHE_OPEN                                                               */
/*      The time group is attached, at or after the sweep index, and         */
/*      not busy, so its state is not clearly either active or inactive.     */
/*      It contributes a value of 1 to the open_group_count attribute.       */
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
/*  history_before range have state KHE_ACTIVE, while time groups in the     */
/*  history_after range have state KHE_OPEN.  When the monitor is not        */
/*  attached, like other time groups virtual ones have state KHE_INACTIVE.   */
/*                                                                           */
/*  When implementing the limit active intervals monitor, it is              */
/*  convenient to represent a time group by a pair (m, index), where m       */
/*  is the monitor and index is an index in the sequence of time             */
/*  groups.  This is convenient because a virtual time group (before         */
/*  or after) can use this same representation.  Although the need is        */
/*  not as definite here, for consistency with limit active intervals        */
/*  monitors we represent a time group by the pair (m, index).               */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_INACTIVE,
  KHE_ACTIVE,
  KHE_OPEN
} KHE_TIME_GROUP_STATE;

typedef struct khe_time_group_info_rec {
  KHE_MONITORED_TIME_GROUP	monitored_time_group;	/* the time group    */
  KHE_POLARITY			polarity;	/* polarity of time group    */
  KHE_TIME_GROUP_STATE		state;		/* state of time group       */
  KHE_TIME_GROUP_STATE		not_busy_state;	/* from SetNotBusy           */
} KHE_TIME_GROUP_INFO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_MONITOR - monitors busy times                     */
/*                                                                           */
/*  I've deleted history_before, history_after, and history to save space.   */
/*  There is no significant hit to running time, as I've verified by         */
/*  looking at all the places where these attributes are used.               */
/*                                                                           */
/*****************************************************************************/

struct khe_cluster_busy_times_monitor_rec {
  INHERIT_MONITOR(allow_zero, deviation)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT constraint;		/* monitoring this   */
  int				offset;			/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  /* int			multiplier; */		/* multiplier        */
  KHE_TIME                      sweep_time;	        /* sweep time        */
  int				sweep_index;		/* sweep index       */
  int				active_group_count;	/* active groups     */
  int				open_group_count;	/* open groups       */
  int				time_group_info_count;	/* len of tgi        */
  KHE_COST			history_adjustment;	/* prev inst cost    */
  KHE_TIME			first_time;		/* first time        */
  KHE_TIME			last_time;		/* last time         */
  KHE_CLUSTER_BUSY_TIMES_MONITOR copy;			/* used when copying */
  KHE_TIME_GROUP_INFO		time_group_info[1];	/* extends!          */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "real points"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePointIsReal(KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index)         */
/*                                                                           */
/*  Return true if point is real.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KhePointIsReal(KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index)
{
  return 0 <= index && index < m->time_group_info_count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_STATE KheRealPointState(                                  */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index)                           */
/*                                                                           */
/*  Return the state of this real point.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TIME_GROUP_STATE KheRealPointState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index)
{
  return m->time_group_info[index].state;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_STATE KheRealPointNewState(                               */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index, int busy_count,           */
/*    int sweep_index)                                                       */
/*                                                                           */
/*  Assuming that (m, index) is a real and attached point, work out          */
/*  its new state given these values of busy_count and sweep_index.          */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_STATE KheRealPointNewState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index, int busy_count,
  int sweep_index)
{
  KHE_POLARITY po;
  if( busy_count == 0 && index >= sweep_index )
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
/*  void KheRealPointSetState(KHE_CLUSTER_BUSY_TIMES_MONITOR m,              */
/*    int index, KHE_TIME_GROUP_STATE new_state)                             */
/*                                                                           */
/*  Set the state of the real point at index.                                */
/*                                                                           */
/*****************************************************************************/
/* ***
static char *KheTimeGroupStateShow(KHE_TIME_GROUP_STATE state);

static void KheRealPointSetState(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int index, KHE_TIME_GROUP_STATE new_state)
{
  HnAssert(KhePointIsReal(m, index), "KheRealPointSetState internal error: "
    "index %d out of range 0 .. %d", index, m->time_group_info_count - 1);
  m->time_group_info[index].state = new_state;
  if( DEBUG5 )
    fprintf(stderr, "KheRealPointSetState(%s, %d, %s) assigned %s\n",
      KheClusterBusyTimesMonitorId(m), index,
      KheTimeGroupStateShow(new_state),
      KheTimeGroupStateShow(KheRealPointState(m, index)));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointChangeState(KHE_CLUSTER_BUSY_TIMES_MONITOR m,           */
/*    int index, KHE_TIME_GROUP_STATE new_state)                             */
/*                                                                           */
/*  Change the state of the real point at index to new_state, and flush.     */
/*                                                                           */
/*****************************************************************************/
static void Flush(KHE_CLUSTER_BUSY_TIMES_MONITOR m, bool force);
static char *KheTimeGroupStateShow(KHE_TIME_GROUP_STATE state);

static void KheRealPointChangeState(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int index, KHE_TIME_GROUP_STATE new_state)
{
  KHE_TIME_GROUP_STATE prev_state;

  if( DEBUG6 )
    fprintf(stderr, "[ KheRealPointChangeState(%s, %d, %s -> %s)\n",
      KheClusterBusyTimesMonitorId(m), index,
      KheTimeGroupStateShow(m->time_group_info[index].state),
      KheTimeGroupStateShow(new_state));

  /* save the old state and change the state */
  prev_state = m->time_group_info[index].state;
  m->time_group_info[index].state = new_state;

  /* update active_group_count and open_group_count and flush */
  switch( prev_state )
  {
    case KHE_INACTIVE:

      switch( new_state )
      {
	case KHE_INACTIVE:

	  /* nothing to do here */
	  break;

	case KHE_ACTIVE:

          m->active_group_count++;
	  Flush(m, false);
	  break;

	case KHE_OPEN:

          m->open_group_count++;
	  Flush(m, false);
	  break;
      }
      break;

    case KHE_ACTIVE:

      switch( new_state )
      {
	case KHE_INACTIVE:

          m->active_group_count--;
	  Flush(m, false);
	  break;

	case KHE_ACTIVE:

	  /* nothing to do here */
	  break;

	case KHE_OPEN:

          m->active_group_count--;
          m->open_group_count++;
	  Flush(m, false);
	  break;
      }
      break;

    case KHE_OPEN:

      switch( new_state )
      {
	case KHE_INACTIVE:

          m->open_group_count--;
	  Flush(m, false);
	  break;

	case KHE_ACTIVE:

          m->open_group_count--;
          m->active_group_count++;
	  Flush(m, false);
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
/*  void KheRealPointUpdateState(KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i)    */
/*                                                                           */
/*  Something has changed at index i.  Update its state.                     */
/*                                                                           */
/*****************************************************************************/

static void KheRealPointUpdateState(KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i)
{
  int busy_count;  KHE_TIME_GROUP_STATE new_state;
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  mtg = m->time_group_info[i].monitored_time_group;
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  busy_count = busy_and_idle->busy_count;
  new_state = KheRealPointNewState(m, i, busy_count, m->sweep_index);
  KheRealPointChangeState(m, i, new_state);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRealPointCheck(KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index)      */
/*                                                                           */
/*  Check that the real point at this index is consistent.                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused, but worth keeping just in case
static void KheRealPointCheck(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int index)
{
  KHE_TIME_GROUP_INFO mi;  int busy_count;  KHE_TIME_GROUP_STATE ns;
  mi = MArrayGet(m->time_group_info, index);
  if( !m->attached )
    HnAssert(mi.state == KHE_INACTIVE, "KheRealPointCheck(%s, %d) error 1",
      KheClusterBusyTimesMonitorId(m), index);
  else
  {
    busy_count = KheMonitoredTimeGroupBusyCount(mi.monitored_time_group);
    ns = KheRealPointNewState(m, index, busy_count, m->sweep_index);
    HnAssert(mi.state == ns, "KheRealPointCheck(%s, %d) error 2: recorded %s, "
      "calculated %s:\n  (polarity %s, busy_count %d, sweep_index %d)",
      KheClusterBusyTimesMonitorId(m), index,
      KheTimeGroupStateShow(mi.state), KheTimeGroupStateShow(ns),
      mi.polarity == KHE_POSITIVE ? "positive" : "negative", busy_count,
      m->sweep_index);
  }
}
*** */


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
/*  Submodule "deviation and cost"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheClusterDev(KHE_CLUSTER_BUSY_TIMES_MONITOR m,                      */
/*    int active_group_count, int open_group_count)                          */
/*                                                                           */
/*  Return the deviation of m, given this number of active and open time     */
/*  groups.                                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheClusterDev(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int active_group_count, int open_group_count)
{
  if( m->allow_zero && active_group_count == 0 )
    return 0;
  else if( active_group_count > m->maximum )
    return active_group_count - m->maximum;
  else if( active_group_count + open_group_count < m->minimum )
    return m->minimum - (active_group_count + open_group_count);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void Flush(KHE_CLUSTER_BUSY_TIMES_MONITOR m, bool force)                 */
/*                                                                           */
/*  Assuming that m is attached and that m->active_group_count and           */
/*  m->open_group_count are up to date but that m->deviation may not be,     */
/*  bring m->deviation and m->cost up to date and inform parents of any      */
/*  change in cost.                                                          */
/*                                                                           */
/*  If force is true, change the cost even if the deviation has not          */
/*  changed.  This is used only when the multiplier has changed.             */
/*                                                                           */
/*****************************************************************************/

static void Flush(KHE_CLUSTER_BUSY_TIMES_MONITOR m, bool force)
{
  int new_dev;  KHE_COST new_cost;
  new_dev = KheClusterDev(m, m->active_group_count, m->open_group_count);
  if( new_dev != m->deviation || force )
  {
    m->deviation = new_dev;
    new_cost = (KheMonitorDevToCost((KHE_MONITOR) m, new_dev)
      - m->history_adjustment) /* * m->multiplier */;
    if( DEBUG8 )
      fprintf(stderr, "  %s from %.5f --(active %d, open %d)--> %.5f\n",
	KheMonitorId((KHE_MONITOR) m), KheCostShow(m->cost),
          m->active_group_count, m->open_group_count, KheCostShow(new_cost));
    KheMonitorChangeCost((KHE_MONITOR) m, new_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMtgInfoCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Comparison function for sorting time groups by increasing index          */
/*  of last (latest) time.                                                   */
/*                                                                           */
/*****************************************************************************/

static int KheClusterMtgInfoCmp(const void *t1, const void *t2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_TIME time1, time2;
  KHE_TIME_GROUP_INFO mi1 = * (KHE_TIME_GROUP_INFO *) t1;
  KHE_TIME_GROUP_INFO mi2 = * (KHE_TIME_GROUP_INFO *) t2;
  tg1 = KheMonitoredTimeGroupTimeGroup(mi1.monitored_time_group);
  tg2 = KheMonitoredTimeGroupTimeGroup(mi2.monitored_time_group);
  if( KheTimeGroupTimeCount(tg1) == 0 )
    return -1;
  else if( KheTimeGroupTimeCount(tg2) == 0 )
    return 1;
  else
  {
    time1 = KheTimeGroupTime(tg1, KheTimeGroupTimeCount(tg1) - 1);
    time2 = KheTimeGroupTime(tg2, KheTimeGroupTimeCount(tg2) - 1);
    return KheTimeIndex(time1) - KheTimeIndex(time2);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheClusterBusyTimesMonitorGetHistoryAdjustment(                 */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Find the history adjustment for m (in the previous instance).            */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheClusterBusyTimesMonitorGetHistoryAdjustment(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  int history_before, history_after, history;  KHE_RESOURCE r;
  history_before = KheClusterBusyTimesConstraintHistoryBefore(m->constraint);
  history_after = KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
  r = KheClusterBusyTimesMonitorResource(m);
  history = KheClusterBusyTimesConstraintHistory(m->constraint, r);
  if( history_before <= 0 )
    return 0;
  else
    return KheMonitorDevToCost((KHE_MONITOR) m,
      KheClusterDev(m, history, history_after + m->sweep_index));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterBusyTimesMonitorMake(           */
/*    KHE_RESOURCE_IN_SOLN rs, int offset,                                   */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Make a new cluster busy times monitor for (rs, offset), monitoring c.    */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterBusyTimesMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, int offset, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR res;   KHE_SOLN soln;  /* KHE_RESOURCE r; */
  int i, count;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;
  KHE_TIME_GROUP_INFO mi;  HA_ARENA a;  KHE_INSTANCE ins;

  count = KheClusterBusyTimesConstraintTimeGroupCount(c);
  soln = KheResourceInSolnSoln(rs);
  ins = KheSolnInstance(soln);
  a = KheSolnArena(soln);
  res = HaAlloc(a, sizeof(struct khe_cluster_busy_times_monitor_rec)
    + (count - 1) * sizeof(KHE_TIME_GROUP_INFO));
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->allow_zero = KheClusterBusyTimesConstraintAllowZero(c);
  res->deviation = 0;
  res->resource_in_soln = rs;
  res->constraint = c;
  res->offset = offset;
  res->minimum = KheClusterBusyTimesConstraintMinimum(c);
  res->maximum = KheClusterBusyTimesConstraintMaximum(c);
  /* res->multiplier = 1; */
  if( KheInstanceTimeCount(ins) > 0 )
    res->sweep_time = KheInstanceTime(ins, KheInstanceTimeCount(ins) - 1);
  else
    res->sweep_time = NULL;
  res->sweep_index = KheClusterBusyTimesConstraintTimeGroupCount(c);
  res->active_group_count = 0;
  res->open_group_count = 0;
  res->history_adjustment = KheClusterBusyTimesMonitorGetHistoryAdjustment(res);
  res->first_time = NULL;
  res->last_time = NULL;
  res->copy = NULL;
  res->time_group_info_count = res->sweep_index;
  rtm = KheResourceInSolnTimetableMonitor(rs);
  for( i = 0;  i < res->time_group_info_count;  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(c, i, offset, &mi.polarity);
    mi.monitored_time_group =
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg);
    mi.state = KHE_INACTIVE;
    mi.not_busy_state = KHE_OPEN;
    res->time_group_info[i] = mi;
  }
  qsort(res->time_group_info, res->time_group_info_count,
    sizeof(KHE_TIME_GROUP_INFO), &KheClusterMtgInfoCmp);
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  if( DEBUG8 )
    fprintf(stderr, "  %s after making, cost is %.5f\n",
      KheMonitorId((KHE_MONITOR) res), KheCostShow(res->cost));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterBusyTimesMonitorCopyPhase1(     */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, HA_ARENA a)                          */
/*                                                                           */
/*  Carry out Phase 1 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterBusyTimesMonitorCopyPhase1(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR copy;  int i;  KHE_TIME_GROUP_INFO mi, mi2;
  if( m->copy == NULL )
  {
    copy = HaAlloc(a, sizeof(struct khe_cluster_busy_times_monitor_rec)
      + (m->time_group_info_count - 1) * sizeof(KHE_TIME_GROUP_INFO));
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->allow_zero = m->allow_zero;
    copy->deviation = m->deviation;
    copy->resource_in_soln = KheResourceInSolnCopyPhase1(m->resource_in_soln,a);
    copy->constraint = m->constraint;
    copy->offset = m->offset;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    /* copy->multiplier = m->multiplier; */
    copy->sweep_time = m->sweep_time;
    copy->sweep_index = m->sweep_index;
    copy->active_group_count = m->active_group_count;
    copy->open_group_count = m->open_group_count;
    copy->history_adjustment = m->history_adjustment;
    copy->first_time = m->first_time;
    copy->last_time = m->last_time;
    copy->copy = NULL;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mi = m->time_group_info[i];
      mi2.monitored_time_group =
	KheMonitoredTimeGroupCopyPhase1(mi.monitored_time_group, a);
      mi2.polarity = mi.polarity;
      mi2.state = mi.state;
      mi2.not_busy_state = mi.not_busy_state;
      copy->time_group_info[i] = mi2;
    }
    if( DEBUG9 )
    {
      fprintf(stderr, "KheClusterBusyTimesMonitorCopyPhase1(%p) original: ",
	(void *) m);
      KheClusterBusyTimesMonitorDebug(m, 2, 0, stderr);
    }
  }
  if( DEBUG9 )
  {
    fprintf(stderr, "KheClusterBusyTimesMonitorCopyPhase1(%p) copy %p\n",
      (void *) m, (void *) m->copy);
    /* *** can't do this, it might not be ready
    KheClusterBusyTimesMonitorDebug(m->copy, 2, 0, stderr);
    *** */
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorCopyPhase2(                               */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorCopyPhase2(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  int i;  KHE_TIME_GROUP_INFO mi;
  if( m->copy != NULL )
  {
    if( DEBUG9 )
    {
      fprintf(stderr, "KheClusterBusyTimesMonitorCopyPhase2(%p): ",
	(void *) m);
      KheClusterBusyTimesMonitorDebug(m, 2, 0, stderr);
    }
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      mi = m->time_group_info[i];
      KheMonitoredTimeGroupCopyPhase2(mi.monitored_time_group);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheClusterBusyTimesMonitorResourceInSoln(           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** correct but unused
KHE_RESOURCE_IN_SOLN KheClusterBusyTimesMonitorResourceInSoln(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->resource_in_soln;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterBusyTimesMonitorConstraint(  */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the contraint that m is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterBusyTimesMonitorConstraint(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheClusterBusyTimesMonitorResource(                         */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the resource that m is monitoring.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheClusterBusyTimesMonitorResource(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorHistoryBefore(                             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the history before value (ai) stored in m.                        */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorHistoryBefore(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return KheClusterBusyTimesConstraintHistoryBefore(m->constraint);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorHistoryAfter(                              */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the history after value (ci) stored in m.                         */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorHistoryAfter(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorHistory(KHE_CLUSTER_BUSY_TIMES_MONITOR m)  */
/*                                                                           */
/*  Return the history value (xi) stored in m.                               */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorHistory(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_RESOURCE r;
  r = KheClusterBusyTimesMonitorResource(m);
  return KheClusterBusyTimesConstraintHistory(m->constraint, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorOffset(KHE_CLUSTER_BUSY_TIMES_MONITOR m)   */
/*                                                                           */
/*  Return the offset that m is monitoring.                                  */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorOffset(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorTimeGroupCount(                            */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the number of time groups being monitored by m.                   */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorTimeGroupCount(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->time_group_info_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheClusterBusyTimesMonitorTimeGroup(                      */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_POLARITY *po)             */
/*                                                                           */
/*  Return the i'th time group being monitored by m, and its polarity.       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheClusterBusyTimesMonitorTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_POLARITY *po)
{
  KHE_TIME_GROUP_INFO mi;
  mi = m->time_group_info[i];
  *po = mi.polarity;
  return KheMonitoredTimeGroupTimeGroup(mi.monitored_time_group);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorActiveTimeGroupCount(                     */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int *active_group_count,             */
/*    int *open_group_count, int *minimum, int *maximum, bool *allow_zero)   */
/*                                                                           */
/*  Set *active_group_count to the current number of active time groups,     */
/*  *open_group_count to the current number of open time groups, and         */
/*  *minimum, *maximum, and *allow_zero to the Minimum, Maximum, and         */
/*  AllowZero attributes of the corresponding constraint.                    */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorActiveTimeGroupCount(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int *active_group_count,
  int *open_group_count, int *minimum, int *maximum, bool *allow_zero)
{
  *active_group_count = m->active_group_count;
  *open_group_count = m->open_group_count;
  *minimum = m->minimum;
  *maximum = m->maximum;
  *allow_zero = m->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorRange(KHE_CLUSTER_BUSY_TIMES_MONITOR m,   */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.  Both return values could be NULL.           */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheClusterBusyTimesMonitorTimeSweepRange
void KheClusterBusyTimesMonitorRange(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      tg = KheMonitoredTimeGroupTimeGroup(
	m->time_group_info[i].monitored_time_group);
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
/*  bool KheClusterBusyTimesMonitorTimeGroupIsActive(                        */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,           */
/*    KHE_POLARITY *po, int *busy_count)                                     */
/*                                                                           */
/*  Similar to KheClusterBusyTimesMonitorTimeGroupState, only returning      */
/*  true when tg is active.                                                  */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesMonitorTimeGroupIsActive(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,
  KHE_POLARITY *po, int *busy_count)
{
  KHE_TIME_GROUP_INFO mi;  KHE_BUSY_AND_IDLE busy_and_idle;
  mi = m->time_group_info[i];
  *tg = KheMonitoredTimeGroupTimeGroup(mi.monitored_time_group);
  *po = mi.polarity;
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mi.monitored_time_group);
  *busy_count = busy_and_idle->busy_count;
  return (*po == KHE_NEGATIVE) == (*busy_count == 0);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorAtMaxLimitCount(                           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return true if m is not violating but is at the maximum limit.           */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorAtMaxLimitCount(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->deviation == 0 && m->active_group_count == m->maximum ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When m is unattached, all time groups are considered to be inactive,     */
/*  the deviation is 0, and the cost is 0.                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorAttachToSoln(                             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorAttachToSoln(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_TIME_GROUP_INFO mi;  int i;  KHE_RESOURCE r;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheClusterBusyTimesMonitorAttachToSoln(m)\n");
    KheClusterBusyTimesMonitorDebug(m, 2, 2, stderr);
  }

  /* check that current state is as expected when unattached */
  HnAssert(m->cost == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 1");
  HnAssert(m->deviation == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 2");
  HnAssert(m->active_group_count == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 3");
  HnAssert(m->open_group_count == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 4");

  /* move from unattached state to unlinked state */
  m->attached = true;
  r = KheClusterBusyTimesMonitorResource(m);
  m->active_group_count +=
    KheClusterBusyTimesConstraintHistory(m->constraint, r);
  m->open_group_count +=
    KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
  Flush(m, false);

  /* attach each monitored time group; it will call back with a busy count */
  for( i = 0;  i < m->time_group_info_count;  i++ )
  {
    mi = m->time_group_info[i];
    KheMonitoredTimeGroupAttachMonitor(mi.monitored_time_group,
      (KHE_MONITOR) m, i);
  }

  if( DEBUG2 )
  {
    fprintf(stderr, "  at end:\n");
    KheClusterBusyTimesMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheClusterBusyTimesMonitorAttachToSoln(m)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorDetachFromSoln(                           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorDetachFromSoln(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_TIME_GROUP_INFO mi;  int i;  KHE_RESOURCE r;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheClusterBusyTimesMonitorDetachFromSoln(m)\n");
    KheClusterBusyTimesMonitorDebug(m, 2, 2, stderr);
  }

  /* detach each monitored time group; it will call back to change state */
  for( i = 0;  i < m->time_group_info_count;  i++ )
  {
    mi = m->time_group_info[i];
    KheMonitoredTimeGroupDetachMonitor(mi.monitored_time_group,
      (KHE_MONITOR) m, i);
  }

  /* move from unlinked state to unattached state */
  r = KheClusterBusyTimesMonitorResource(m);
  m->active_group_count -=
    KheClusterBusyTimesConstraintHistory(m->constraint, r);
  m->open_group_count -=
    KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
  HnAssert(m->active_group_count == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 3");
  HnAssert(m->open_group_count == 0,
    "KheClusterBusyTimesMonitorAttachToSoln internal error 4");
  m->deviation = 0;
  if( m->cost != 0 )
  {
    KheMonitorChangeCost((KHE_MONITOR) m, 0);
    m->cost = 0;
  }
  m->attached = false;

  if( DEBUG2 )
  {
    fprintf(stderr, "  at end:\n");
    KheClusterBusyTimesMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "] KheClusterBusyTimesMonitorDetachFromSoln(m)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*  The sweep time is as defined in the Guide:  the largest time where       */
/*  tasks are assigned or being assigned, or NULL if there is no such        */
/*  time (because we are at the start of the time sweep).                    */
/*                                                                           */
/*  The sweep index is the index of the first time group that contains at    */
/*  least one time that is strictly larger than the sweep time.  This time   */
/*  group thus contains at least one open time, and so it is considered to   */
/*  be open, as are all following time groups.                               */
/*                                                                           */
/*  If there is no such time group, then all times in all time groups are    */
/*  less than or equal to the sweep time, so tasks are assigned or being     */
/*  assigned everywhere and the sweep index is one greater than the index    */
/*  of the last time group, that is, it is the number of time groups.        */
/*                                                                           */
/*  If the sweep time is NULL, all times are considered to be greater than   */
/*  the sweep time, including the times of the first time group, so the      */
/*  sweep index is 0.                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesMonitorTimeRange(                                */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m,                                      */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m, or false if empty.                           */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesMonitorTimeRange(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    for( i = 0;  i < m->time_group_info_count;  i++ )
    {
      tg = KheMonitoredTimeGroupTimeGroup(
	m->time_group_info[i].monitored_time_group);
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
/*  int KheClusterBusyTimesMonitorFindSweepIndex(                            */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_TIME sweep_time)                 */
/*                                                                           */
/*  Find the sweep index corresponding to sweep_time.                        */
/*                                                                           */
/*  When this function is called, m->sweep_time and m->sweep_index are       */
/*  well-defined, and m->sweep_index is correct for m->sweep_time.  This     */
/*  fact is used in this function to speed up the search for the new index.  */
/*                                                                           */
/*****************************************************************************/

static int KheClusterBusyTimesMonitorFindSweepIndex(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_TIME sweep_time)
{
  int i;  KHE_TIME_GROUP tg;  KHE_TIME t;  KHE_MONITORED_TIME_GROUP mtg;
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
      mtg = m->time_group_info[i].monitored_time_group;
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
/*  void KheClusterBusyTimesMonitorSetSweepIndex(                            */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int sweep_index)                     */
/*                                                                           */
/*  Set the sweep index of m to sweep_index, including calling               */
/*  KheRealPointUpdateState for all time groups affected.                    */
/*                                                                           */
/*****************************************************************************/

static void KheClusterBusyTimesMonitorSetSweepIndex(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int sweep_index)
{
  int i, old_sweep_index;
  if( DEBUG3 )
    fprintf(stderr, "   KheClusterBusyTimesMonitorSetSweepIndex(%s, %d)\n",
      KheMonitorId((KHE_MONITOR) m), sweep_index);
  HnAssert(0 <= sweep_index && sweep_index <= m->time_group_info_count,
    "KheClusterBusyTimesMonitorSetSweepIndex: sweep index (%d) out of"
    " range (0 .. %d)", sweep_index, m->time_group_info_count);
  old_sweep_index = m->sweep_index;
  m->sweep_index = sweep_index;
  if( m->attached )
  {
    if( sweep_index < old_sweep_index )
    {
      /* points from sweep_index to old_sweep_index - 1 might change state */
      for( i = old_sweep_index - 1;  i >= sweep_index;  i-- )
	KheRealPointUpdateState(m, i);
    }
    else
    {
      /* points from old_sweep_index to sweep_index - 1 might change state */
      for( i = old_sweep_index;  i < sweep_index;  i++ )
	KheRealPointUpdateState(m, i);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorSetSweepTime(                             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_TIME t)                          */
/*                                                                           */
/*  Set the sweep time of m to t.  Also update the sweep index.              */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorSetSweepTime(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_TIME t)
{
  int new_sweep_index;
  new_sweep_index = KheClusterBusyTimesMonitorFindSweepIndex(m, t);
  KheClusterBusyTimesMonitorSetSweepIndex(m, new_sweep_index);
  m->sweep_time = t;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheClusterBusyTimesMonitorInitialSweepTime(                     */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return the first time t such that cutting off at t is not the same as    */
/*  cutting off at index 0, or NULL if there is no such time.                */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TIME KheClusterBusyTimesMonitorInitialSweepTime(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KHE_INSTANCE ins;  KHE_TIME_GROUP_INFO mi;  KHE_TIME_GROUP tg;

  if( m->time_group_info_count == 0 )
  {
    ** no time groups, so no time can produce a different sweep **
    return NULL;
  }
  else
  {
    mi = m->time_group_info[0];
    tg = KheMonitoredTimeGroupTimeGroup(mi.monitored_time_group);
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
/*  bool KheClusterBusyTimesMonitorSweepTimeRange(                           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m,                                      */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the sweep time range of m, or false if empty.                     */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesMonitorSweepTimeRange(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheClusterBusyTimesMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorSetNotBusyState(                          */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, bool active)                  */
/*                                                                           */
/*  Inform m that time group i should be considered active (if active is     */
/*  true) or inactive (if active is false) when it is not busy and it lies   */
/*  beyond the sweep index.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorSetNotBusyState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i, bool active)
{
  m->time_group_info[i].not_busy_state = (active ? KHE_ACTIVE : KHE_INACTIVE);
  if( m->attached )
    KheRealPointUpdateState(m, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorClearNotBusyState(                        */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i)                               */
/*                                                                           */
/*  Undo the corresponding KheClusterBusyTimesMonitorSetNotBusyState.        */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorClearNotBusyState(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int i)
{
  m->time_group_info[i].not_busy_state = KHE_OPEN;
  if( m->attached )
    KheRealPointUpdateState(m, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorAddBusyAndIdle(                           */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,                           */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add a busy value for one monitored time group.                           */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorAddBusyAndIdle(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  KHE_TIME_GROUP_STATE new_state;
  HnAssert(m->attached,
    "KheClusterBusyTimesMonitorAddBusyAndIdle internal error 1");
  new_state = KheRealPointNewState(m, index,
    busy_and_idle->busy_count, m->sweep_index);
  KheRealPointChangeState(m, index, new_state);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorDeleteBusyAndIdle(                        */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,                           */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Remove the busy value of one monitored time group.                       */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorDeleteBusyAndIdle(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  HnAssert(m->attached,
    "KheClusterBusyTimesMonitorDeleteBusyAndIdle internal error 1");
  KheRealPointChangeState(m, index, KHE_INACTIVE);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorChangeBusyAndIdle(                        */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,                           */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change the value of one monitored time group.                            */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorChangeBusyAndIdle(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle,
  KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  KHE_TIME_GROUP_STATE new_state;
  HnAssert(m->attached,
    "KheClusterBusyTimesMonitorChangeBusyAndIdle internal error 1");
  new_state = KheRealPointNewState(m, index,
    new_busy_and_idle->busy_count, m->sweep_index);
  KheRealPointChangeState(m, index, new_state);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorDeviation(KHE_CLUSTER_BUSY_TIMES_MONITOR m)*/
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorDeviation(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->deviation;
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
/*  void KheAddActiveTimeGroups(KHE_CLUSTER_BUSY_TIMES_MONITOR m,            */
/*    HA_ARRAY_NCHAR *ac, char sign, bool *not_first)                        */
/*                                                                           */
/*  Add string representations of the active time groups of m to *ac.        */
/*                                                                           */
/*****************************************************************************/

static void KheAddActiveTimeGroups(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  HA_ARRAY_NCHAR *ac, char sign, bool *not_first)
{
  int i, busy;  char buff[10];  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
  {
    if( KheClusterBusyTimesMonitorTimeGroupIsActive(m, i, &tg, &po, &busy) )
    {
      if( *not_first )
	sprintf(buff, "  %c  ", sign);
      else
	sprintf(buff, "%s", "");
      HnStringAdd(ac, "%s%s%s", buff, po == KHE_NEGATIVE ? "*" : "",
	KheActiveTimeGroupName(tg));
      *not_first = true;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheClusterBusyTimesMonitorDeviationDescription(                    */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheClusterBusyTimesMonitorDeviationDescription(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int total, history, history_after;
  bool not_first;  HA_ARENA a;  KHE_RESOURCE r;
  r = KheClusterBusyTimesMonitorResource(m);
  history_after = KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
  history = KheClusterBusyTimesConstraintHistory(m->constraint, r);
  total = m->active_group_count + history;
  if( m->allow_zero && total == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    if( total + history_after < m->minimum )
    {
      /* m->minimum - history - m->active_group_count - history_after */
      HnStringAdd(&ac, "%d", m->minimum);
      if( history > 0 )
	HnStringAdd(&ac, "  -  History %d", history);
      not_first = true;
      KheAddActiveTimeGroups(m, &ac, '-', &not_first);
      if( history_after > 0 )
	HnStringAdd(&ac, "  -  History %d", history_after);
    }
    else if( total > m->maximum )
    {
      /* history + m->active_group_count - m->maximum */
      not_first = false;
      if( history > 0 )
      {
	HnStringAdd(&ac, "History %d", history);
	not_first = true;
      }
      KheAddActiveTimeGroups(m, &ac, '+', &not_first);
      HnStringAdd(&ac, "  -  %d", m->maximum);
    }
    else
      HnStringAdd(&ac, "0");
    if( m->history_adjustment > 0 )
      HnStringAdd(&ac, " (adjust %.5f)",KheCostShow(m->history_adjustment));
    /* ***
    if( m->multiplier != 1 )
      HnStringAdd(&ac, " (multiplier %d)", m->multiplier);
    *** */
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheClusterBusyTimesMonitorPointOfApplication(                      */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheClusterBusyTimesMonitorPointOfApplication(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  char *name;  HA_ARENA a;
  name = KheResourceName(KheClusterBusyTimesMonitorResource(m));
  if( KheClusterBusyTimesConstraintAppliesToTimeGroup(m->constraint) != NULL )
  {
    a = KheSolnArena(m->soln);
    return HnStringMake(a, "%s offset %d", name, m->offset);
  }
  else
    return name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheClusterBusyTimesMonitorId(KHE_CLUSTER_BUSY_TIMES_MONITOR m)     */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheClusterBusyTimesMonitorId(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;  KHE_TIME_GROUP tg;
  KHE_MONITORED_TIME_GROUP mtg;  KHE_TIME t;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheClusterBusyTimesMonitorResource(m));
    a = KheSolnArena(m->soln);
    if( m->offset == 0 )
      m->id = HnStringMake(a, "%s/%s", constraint_id, resource_id);
    else if( m->time_group_info_count > 0 )
    {
      mtg = m->time_group_info[0].monitored_time_group;
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      if( KheTimeGroupTimeCount(tg) > 0 )
      {
	t = KheTimeGroupTime(tg, 0);
	m->id = HnStringMake(a, "%s/%s/%s", constraint_id, resource_id,
	  KheTimeId(t));
      }
      else
	m->id = HnStringMake(a, "%s/%s/%d", constraint_id, resource_id,
	  m->offset);
    }
    else
      m->id = HnStringMake(a, "%s/%s/%d", constraint_id, resource_id,
	m->offset);
  }
  return m->id;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "multiplier"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorSetMultiplier(                            */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)                             */
/*                                                                           */
/*  Set m's multiplier to val.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheClusterBusyTimesMonitorSetMultiplier(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)
{
  HnAssert(val >= 0, "KheClusterBusyTimesMonitorSetMultiplier: val < 0");
  if( DEBUG7 )
    fprintf(stderr, "  KheClusterBusyTimesMonitorSetMultiplier(%s, %d -> %d)\n",
      KheMonitorId((KHE_MONITOR) m), m->multiplier, val);
  m->multiplier = val;
  if( m->attached )
    Flush(m, true);
}
*** */


/*****************************************************************************/
/*                                                                           */
/* int KheClusterBusyTimesMonitorMultiplier(KHE_CLUSTER_BUSY_TIMES_MONITOR m)*/
/*                                                                           */
/*  Return m's multiplier.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheClusterBusyTimesMonitorMultiplier(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->multiplier;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "maximum and minimum"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorMaximum(KHE_CLUSTER_BUSY_TIMES_MONITOR m)  */
/*                                                                           */
/*  Return the maximum limit of m.                                           */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorMaximum(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesMonitorMinimum(KHE_CLUSTER_BUSY_TIMES_MONITOR m)  */
/*                                                                           */
/*  Return the minimum limit of m.                                           */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesMonitorMinimum(KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  return m->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorSetMinimum(                               */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)                             */
/*                                                                           */
/*  Set m's minimum limit to val.                                            */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorSetMinimum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)
{
  HnAssert(val >= 0, "KheClusterBusyTimesMonitorSetMinimum: val < 0");
  if( DEBUG7 )
    fprintf(stderr, "  KheClusterBusyTimesMonitorSetMinimum(%s, %d -> %d)\n",
      KheMonitorId((KHE_MONITOR) m), m->minimum, val);
  m->minimum = val;
  if( m->attached )
    Flush(m, true);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorResetMinimum(                             */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m)                                      */
/*                                                                           */
/*  Reset m's minimum limit to its original value.                           */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorResetMinimum(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  KheClusterBusyTimesMonitorSetMinimum(m,
    KheClusterBusyTimesConstraintMinimum(m->constraint));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesMonitorDebug(KHE_CLUSTER_BUSY_TIMES_MONITOR m,   */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesMonitorDebug(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  int history, history_after;  KHE_RESOURCE r;
  r = KheClusterBusyTimesMonitorResource(m);
  history_after = KheClusterBusyTimesConstraintHistoryAfter(m->constraint);
  history = KheClusterBusyTimesConstraintHistory(m->constraint, r);
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " %s", "CBTM ");
    if( m->sweep_index < m->time_group_info_count )
      fprintf(fp, "sweep %d, ", m->sweep_index);
    if( m->minimum > 0 )
      fprintf(fp, "min %d, ", m->minimum);
    if( m->maximum < m->time_group_info_count )
      fprintf(fp, "max %d, ", m->maximum);
    /* ***
    if( m->multiplier != 1 )
      fprintf(fp, "mult %d, ", m->multiplier);
    *** */
    if( m->allow_zero )
      fprintf(fp, "allow_zero, ");
    if( history > 0 )
      fprintf(fp, "history %d, ", history);
    if( history_after > 0 )
      fprintf(fp, "history_after %d, ", history_after);
    fprintf(fp, "active_count %d", m->active_group_count);
    if( m->open_group_count > 0 )
      fprintf(fp, ", open_count %d", m->open_group_count);
    if( m->history_adjustment > 0 )
      fprintf(fp, ", adjust %.5f", KheCostShow(m->history_adjustment));
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

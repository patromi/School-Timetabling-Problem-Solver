
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
/*  FILE:         khe_avoid_unavailable_times_monitor.c                      */
/*  DESCRIPTION:  An avoid unavailable times monitor                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR                                      */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_unavailable_times_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT constraint;	/* monitoring this   */
  KHE_MONITORED_TIME_GROUP	monitored_time_group;	/* monitored domain  */
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR copy;		/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorSetLowerBound(                       */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  If m's lower bound is greater than 0, set m->lower_bound to its value.   */
/*                                                                           */
/*****************************************************************************/

static void KheAvoidUnavailableTimesMonitorSetLowerBound(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  KHE_RESOURCE r;  KHE_TIME_GROUP tg;  /* KHE_CONSTRAINT mc; */
  int events_durn, avoid_durn, d;

  /* if r does not have a highly weighted avoid clashes constraint, quit */
  r = KheAvoidUnavailableTimesMonitorResource(m);
  /* mc = (KHE_CONSTRAINT) m->constraint; */
  if( !KheResourceHasAvoidClashesConstraint(r, m->combined_weight))
    return;  /* no highly weighted avoid clashes constraint */
  /* ***
  if( !KheResourceHasAvoidClashesConstraint(r,K heConstraintCombinedWeight(mc)))
    return;
  *** */

  /* find the duration of highly weighted preassigned events */
  events_durn = KheResourcePreassignedEventsDuration(r, m->combined_weight);
    /* Khe ConstraintCombinedWeight(mc)); */

  /* find the number of times to be avoided */
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(m->constraint);
  avoid_durn = KheTimeGroupTimeCount(tg);

  /* Let d be the number of deviations, and set lower_bound if needed */
  d = events_durn + avoid_durn - KheInstanceTimeCount(KheSolnInstance(m->soln));
  if( d > 0 )
    m->lower_bound = KheMonitorDevToCost((KHE_MONITOR) m, d);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR KheAvoidUnavailableTimesMonitorMake( */
/*    KHE_RESOURCE_IN_SOLN rs, KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)     */
/*                                                                           */
/*  Make a new avoid unavailable times monitor object with these attributes. */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_UNAVAILABLE_TIMES_MONITOR KheAvoidUnavailableTimesMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->deviation = 0;
  res->resource_in_soln = rs;
  res->constraint = c;
  res->monitored_time_group = KheResourceTimetableMonitorAddMonitoredTimeGroup(
    KheResourceInSolnTimetableMonitor(rs),
    KheAvoidUnavailableTimesConstraintUnavailableTimes(c));
  res->copy = NULL;
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  /* KheGroupMonitorAddMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) res); */
  KheAvoidUnavailableTimesMonitorSetLowerBound(res);
  if( DEBUG1 && res->lower_bound > 0 )
  {
    fprintf(stderr, "KheAvoidUnavailableTimesMonitorMake: ");
    KheMonitorDebug((KHE_MONITOR) res, 1, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR                                      */
/*    KheAvoidUnavailableTimesMonitorCopyPhase1(                             */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, HA_ARENA a)                     */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_UNAVAILABLE_TIMES_MONITOR KheAvoidUnavailableTimesMonitorCopyPhase1(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->deviation = m->deviation;
    copy->resource_in_soln =
      KheResourceInSolnCopyPhase1(m->resource_in_soln, a);
    copy->monitored_time_group =
      KheMonitoredTimeGroupCopyPhase1(m->monitored_time_group, a);
    copy->constraint = m->constraint;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorCopyPhase2(                          */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorCopyPhase2(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
    KheMonitoredTimeGroupCopyPhase2(m->monitored_time_group);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorDelete(                              */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidUnavailableTimesMonitorDelete(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  if( m->attached )
    KheAvoidUnavailableTimesMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheResourceInSolnDeleteMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT                                   */
/*    KheAvoidUnavailableTimesMonitorConstraint(                             */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return the constraint monitored by m.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT
  KheAvoidUnavailableTimesMonitorConstraint(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheAvoidUnavailableTimesMonitorResource(                    */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return the resource monitored by m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheAvoidUnavailableTimesMonitorResource(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0, because without a timetable to monitor      */
/*  there can be no usage of unavailable times.                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorAttachToSoln(                        */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorAttachToSoln(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  m->attached = true;
  /* KheResourceInSolnAttachMonitor(m->resource_in_soln, (KHE_MONITOR) m); */
  KheMonitoredTimeGroupAttachMonitor(m->monitored_time_group, (KHE_MONITOR)m,0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorDetachFromSoln(                      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorDetachFromSoln(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  /* KheResourceInSolnDetachMonitor(m->resource_in_soln, (KHE_MONITOR) m); */
  KheMonitoredTimeGroupDetachMonitor(m->monitored_time_group, (KHE_MONITOR)m,0);
  HnAssert(m->deviation == 0,
    "KheAvoidUnavailableTimesMonitorDetachFromSoln internal error");
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesMonitorTimeRange(                           */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m,                                 */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidUnavailableTimesMonitorTimeRange(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c;  KHE_TIME_GROUP tg;
  c = KheAvoidUnavailableTimesMonitorConstraint(m);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(c);
  if( KheTimeGroupTimeCount(tg) == 0 )
    return *first_time = *last_time = NULL, false;
  else
    return *first_time = KheTimeGroupTime(tg, 0),
      *last_time = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1), true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorSetSweepTime(                        */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m,                                 */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*  Adding a sweep time does not change the cost of m, assuming that         */
/*  times beyond the sweep time are unassigned.                              */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorSetSweepTime(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesMonitorSweepTimeRange(                      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m,                                 */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*  Adding a sweep time does not change the cost of m, assuming that         */
/*  times beyond the sweep time are unassigned.                              */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidUnavailableTimesMonitorSweepTimeRange(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return *first_time = *last_time = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorAddBusyAndIdle(                      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,                      */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Receive a report of a new lot of busy times, and pass it on.  Idle       */
/*  times are not used by this monitor.                                      */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorAddBusyAndIdle(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  HnAssert(m->deviation == 0,
    "KheAvoidUnavailableTimesMonitorAddBusyAndIdle internal error");
  m->deviation = busy_and_idle->busy_count;
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorDeleteBusyAndIdle(                   */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,                      */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Receive a report of a lot of busy times being deleted, and pass it on.   */
/*  Idle times are not used by this monitor.                                 */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorDeleteBusyAndIdle(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  HnAssert(m->cost == KheMonitorDevToCost((KHE_MONITOR) m,
    busy_and_idle->busy_count),
    "KheAvoidUnavailableTimesMonitorDeleteBusyAndIdle internal error");
  m->deviation = 0;
  KheMonitorChangeCost((KHE_MONITOR) m, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesMonitorChangeBusyAndIdle(                   */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,                      */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Receive a report of a change in the number of busy and idle times,       */
/*  and pass it on.  Idle times are not used by this monitor.                */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorChangeBusyAndIdle(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle, KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  HnAssert(old_busy_and_idle->busy_count == m->deviation,
    "KheAvoidUnavailableTimesMonitorChangeBusyAndIdle internal error");
  if( old_busy_and_idle->busy_count != new_busy_and_idle->busy_count )
  {
    m->deviation = new_busy_and_idle->busy_count;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesMonitorDeviation(                            */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesMonitorDeviation(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidUnavailableTimesMonitorDeviationDescription(               */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidUnavailableTimesMonitorDeviationDescription(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TIME t;  char *name;  KHE_TIME_GROUP tg;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d: ", m->deviation);
    rtm = KheResourceInSolnTimetableMonitor(m->resource_in_soln);
    if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
      KheMonitorAttachToSoln((KHE_MONITOR) rtm);
    tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(m->constraint);
    count = 0;
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      if( KheResourceTimetableMonitorTimeTaskCount(rtm, t) >= 1 )
      {
	if( count > 0 )
          HnStringAdd(&ac, "; ");
	name = KheTimeName(t);
	HnStringAdd(&ac, name == NULL ? "?" : name);
	count++;
      }
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidUnavailableTimesMonitorPointOfApplication(                 */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidUnavailableTimesMonitorPointOfApplication(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  return KheResourceName(KheAvoidUnavailableTimesMonitorResource(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidUnavailableTimesMonitorId(                                 */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)                                 */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidUnavailableTimesMonitorId(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheAvoidUnavailableTimesMonitorResource(m));
    a = KheSolnArena(m->soln);
    m->id = HnStringMake(a,"%s/%s", constraint_id, resource_id);
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
/*  void KheAvoidUnavailableTimesMonitorDebug(                               */
/*    KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of m onto fp with the given indent.                          */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesMonitorDebug(
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " %s", "AUTM");
    /* KheResourceInSolnDebug(m->resource_in_soln, 1, -1, fp); */
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

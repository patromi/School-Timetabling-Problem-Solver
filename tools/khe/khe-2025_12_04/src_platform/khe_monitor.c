
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
/*  FILE:         khe_monitor.c                                              */
/*  DESCRIPTION:  A monitor (abstract supertype)                             */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR - monitors one point of application of one constraint        */
/*                                                                           */
/*****************************************************************************/

struct khe_monitor_rec {
  INHERIT_MONITOR(unused1, unused2)
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorCheck(KHE_MONITOR m)                                      */
/*                                                                           */
/*  Check and invariant of m.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorCheck(KHE_MONITOR m)
{
  int i;  KHE_MONITOR defect;
  if( DEBUG2 )
  {
    HnAssert(m->cost >= 0, "KheMonitorCheck: negative cost");
    if( m->parent != NULL )
      HnAssert((m->defect_index == -1) == (m->cost == 0),
	"KheMonitorCheck: defect index problem 1: (prnt, index %d, cost %.5f)",
	m->defect_index, KheCostShow(m->cost));
    else
      HnAssert(m->defect_index == -1,
	"KheMonitorCheck: defect index problem 2: (no prnt, index %d)",
	m->defect_index);
    ** can't call KheGroupMonitorDefectCount in this function! ***
    if( m->cost > 0 && m->parent != NULL )
    {
      for( i = 0;  i < KheGroupMonitorDefectCount(m->parent);  i++ )
      {
	defect = KheGroupMonitorDefect(m->parent, i);
	if( defect == m )
	{
	  HnAssert(i == m->defect_index,
	    "KheMonitorCheck: defect index problem 2");
	  break;
	}
      }
      HnAssert(i < KheGroupMonitorDefectCount(m->parent),
        "KheMonitorCheck: defect index problem 2");
    }
    *** **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetBack(KHE_MONITOR m, void *back)                        */
/*                                                                           */
/*  Set the back pointer of m.                                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSetBack(KHE_MONITOR m, void *back)
{
  m->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheMonitorBack(KHE_MONITOR m)                                      */
/*                                                                           */
/*  Return the back pointer of m.                                            */
/*                                                                           */
/*****************************************************************************/

void *KheMonitorBack(KHE_MONITOR m)
{
  return m->back;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "visit numbers"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetVisitNum(KHE_MONITOR m, int num)                       */
/*                                                                           */
/*  Set the visit number of m.                                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSetVisitNum(KHE_MONITOR m, int num)
{
  m->visit_num = num;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorVisitNum(KHE_MONITOR m)                                    */
/*                                                                           */
/*  Return the visit number of m.                                            */
/*                                                                           */
/*****************************************************************************/

int KheMonitorVisitNum(KHE_MONITOR m)
{
  return m->visit_num;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorVisited(KHE_MONITOR m, int slack)                         */
/*                                                                           */
/*  Return true if m has been visited recently.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorVisited(KHE_MONITOR m, int slack)
{
  return KheSolnGlobalVisitNum(m->soln) - m->visit_num <= slack;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorVisit(KHE_MONITOR m)                                      */
/*                                                                           */
/*  Visit m.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheMonitorVisit(KHE_MONITOR m)
{
  m->visit_num = KheSolnGlobalVisitNum(m->soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorUnVisit(KHE_MONITOR m)                                    */
/*                                                                           */
/*  Unvisit m.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorUnVisit(KHE_MONITOR m)
{
  m->visit_num = KheSolnGlobalVisitNum(m->soln) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other queries"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheMonitorSoln(KHE_MONITOR m)                                   */
/*                                                                           */
/*  Return the solution containing m.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheMonitorSoln(KHE_MONITOR m)
{
  return m->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorCost(KHE_MONITOR m)                                   */
/*                                                                           */
/*  Return the cost of m.                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMonitorCost(KHE_MONITOR m)
{
  if( m->tag == KHE_GROUP_MONITOR_TAG ||
      m->tag == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
      m->tag == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    KheSolnMatchingUpdate(KheMonitorSoln(m));
  return m->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorLowerBound(KHE_MONITOR m)                             */
/*                                                                           */
/*  Return a lower bound on m's cost, usually 0.                             */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMonitorLowerBound(KHE_MONITOR m)
{
  return m->lower_bound;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST_FUNCTION KheMonitorCostFunction(KHE_MONITOR m)                  */
/*                                                                           */
/*  Return m's cost function.                                                */
/*                                                                           */
/*****************************************************************************/

KHE_COST_FUNCTION KheMonitorCostFunction(KHE_MONITOR m)
{
  return m->cost_function;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorCombinedWeight(KHE_MONITOR m)                         */
/*                                                                           */
/*  Return the combined weight of m.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMonitorCombinedWeight(KHE_MONITOR m)
{
  return m->combined_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetCombinedWeight(KHE_MONITOR m, KHE_COST combined_weight)*/
/*                                                                           */
/*  Set the combined weight of m to combined_weight.                         */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSetCombinedWeight(KHE_MONITOR m, KHE_COST combined_weight)
{
  HnAssert(combined_weight >= 0,
    "KheMonitorSetCombinedWeight: combined_weight is negative");
  if( m->combined_weight != combined_weight )
  {
    if( KheMonitorAttachedToSoln(m) )
    {
      KheMonitorDetachFromSoln(m);
      m->combined_weight = combined_weight;
      KheMonitorAttachToSoln(m);
    }
    else
      m->combined_weight = combined_weight;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorResetCombinedWeight(KHE_MONITOR m)                        */
/*                                                                           */
/*  Reset the combined weight of m to its initial value.  If m does          */
/*  not have a constraint to get the initial value from, do nothing.         */
/*                                                                           */
/*****************************************************************************/

void KheMonitorResetCombinedWeight(KHE_MONITOR m)
{
  KHE_CONSTRAINT c;
  c = KheMonitorConstraint(m);
  if( c != NULL )
    KheMonitorSetCombinedWeight(m, KheConstraintCombinedWeight(c));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMonitorDevToCost(KHE_MONITOR m, int dev)                     */
/*                                                                           */
/*  Convert deviation dev into a cost.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheMonitorDevToCost(KHE_MONITOR m, int dev)
{
  switch( m->cost_function )
  {
    case KHE_STEP_COST_FUNCTION:

        return dev > 0 ? m->combined_weight : 0;

    case KHE_LINEAR_COST_FUNCTION:

        return dev * m->combined_weight;

    case KHE_QUADRATIC_COST_FUNCTION:

	return dev * dev * m->combined_weight;

    default:

	HnAbort("KheMonitorDevToCost internal error");
	return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_TAG KheMonitorTag(KHE_MONITOR m)                             */
/*                                                                           */
/*  Return the tag attribute of m.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR_TAG KheMonitorTag(KHE_MONITOR m)
{
  return m->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorAttachedToSoln(KHE_MONITOR m)                             */
/*                                                                           */
/*  Return true if m is currently attached.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorAttachedToSoln(KHE_MONITOR m)
{
  /* *** allowing this now
  HnAssert(m->tag != KHE_GROUP_MONITOR_TAG,
    "KheMonitorAttachedToSoln: m is a group monitor");
  *** */
  return m->attached;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorPathCount(KHE_MONITOR lower_m, KHE_MONITOR higher_m)       */
/*                                                                           */
/*  Return the number of distinct paths from lower_m to higher_m.            */
/*                                                                           */
/*****************************************************************************/

int KheMonitorPathCount(KHE_MONITOR lower_m, KHE_MONITOR higher_m)
{
  KHE_MONITOR_LINK link;  int i, res;
  if( lower_m == higher_m )
    res = 1;
  else
  {
    res = 0;
    HaArrayForEach(lower_m->parent_links, link, i)
      res += KheMonitorPathCount((KHE_MONITOR) link->parent, higher_m);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEnsureOfficialCost(KHE_SOLN soln)                            */
/*                                                                           */
/*  Ensure that KheSolnCost(soln) is the official cost, by ensuring that     */
/*  all constraint monitors are both attached to the solution and reporting  */
/*  their cost to the solution, directly or indirectly, and that all         */
/*  non-constraint non-group monitors are detached from the solution.        */
/*                                                                           */
/*****************************************************************************/

void KheSolnEnsureOfficialCost(KHE_SOLN soln)
{
  int i;  KHE_MONITOR m;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    switch( m->tag )
    {
      case KHE_PREFER_RESOURCES_MONITOR_TAG:

	if( KheMonitorConstraint(m) == NULL )
	{
	  /* if redundancy monitor, ensure detached */
	  if( KheMonitorAttachedToSoln(m) )
	    KheMonitorDetachFromSoln(m);
	  break;
	}
	/* *** NB NO BREAK *** */

      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
      case KHE_ASSIGN_TIME_MONITOR_TAG:
      case KHE_SPLIT_EVENTS_MONITOR_TAG:
      case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
      case KHE_PREFER_TIMES_MONITOR_TAG:
      case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
      case KHE_SPREAD_EVENTS_MONITOR_TAG:
      case KHE_LINK_EVENTS_MONITOR_TAG:
      case KHE_ORDER_EVENTS_MONITOR_TAG:
      case KHE_AVOID_CLASHES_MONITOR_TAG:
      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
      case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	/* ensure the combined weight is reset */
	KheMonitorResetCombinedWeight(m);

	/* ensure attached to soln */
	if( !KheMonitorAttachedToSoln(m) )
	  KheMonitorAttachToSoln(m);

	/* ensure linked to soln exactly once */
        if( KheMonitorPathCount(m, (KHE_MONITOR) soln) != 1 )
	{
	  /* remove all parents then add soln as parent */
	  while( KheMonitorParentMonitorCount(m) > 0 )
	    KheGroupMonitorDeleteChildMonitor(KheMonitorParentMonitor(m, 0), m);
	  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, m);
	}

	/* make sure ceilings are lifted and minimums are reset */
	if( m->tag == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
	  KheLimitBusyTimesMonitorSetCeiling(
	    (KHE_LIMIT_BUSY_TIMES_MONITOR) m, INT_MAX);
	else if( m->tag == KHE_LIMIT_WORKLOAD_MONITOR_TAG )
	  KheLimitWorkloadMonitorSetCeiling(
	    (KHE_LIMIT_WORKLOAD_MONITOR) m, INT_MAX);
	else if( m->tag == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
	{
          cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  /* KheClusterBusyTimesMonitorSetMultiplier(cbtm, 1); */
	  KheClusterBusyTimesMonitorResetMinimum(cbtm);
	}
	break;

      case KHE_EVENT_TIMETABLE_MONITOR_TAG:
      case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
      /* case KHE_TIMETABLE_MONITOR_TAG: */
      case KHE_GROUP_MONITOR_TAG:

	/* do nothing with these ones */
	break;

      case KHE_ORDINARY_DEMAND_MONITOR_TAG:
      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
      case KHE_EVENNESS_MONITOR_TAG:

	/* ensure detached from soln */
	if( KheMonitorAttachedToSoln(m) )
	  KheMonitorDetachFromSoln(m);
	break;

      default:

	HnAbort("KheSolnEnsureOfficialCost internal error");
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorTimeRange(KHE_MONITOR m,                                  */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  If m is influenced by what is happening at certain times, set            */
/*  *first_time to the first of these times, and *last_time to the           */
/*  last of these times, and return true.  Otherwise set *first_time         */
/*  and *last_time to NULL and return false.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorTimeRange(KHE_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorTimeRange(
	(KHE_ASSIGN_RESOURCE_MONITOR) m, first_time, last_time);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorTimeRange(
	(KHE_ASSIGN_TIME_MONITOR) m, first_time, last_time);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorTimeRange(
	(KHE_SPLIT_EVENTS_MONITOR) m, first_time, last_time);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorTimeRange(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, first_time, last_time);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorTimeRange(
	(KHE_PREFER_RESOURCES_MONITOR) m, first_time, last_time);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorTimeRange(
	(KHE_PREFER_TIMES_MONITOR) m, first_time, last_time);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorTimeRange(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, first_time, last_time);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorTimeRange(
	(KHE_SPREAD_EVENTS_MONITOR) m, first_time, last_time);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorTimeRange(
	(KHE_LINK_EVENTS_MONITOR) m, first_time, last_time);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorTimeRange(
	(KHE_ORDER_EVENTS_MONITOR) m, first_time, last_time);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorTimeRange(
	(KHE_AVOID_CLASHES_MONITOR) m, first_time, last_time);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorTimeRange(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, first_time, last_time);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorTimeRange(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, first_time, last_time);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorTimeRange(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, first_time, last_time);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorTimeRange(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, first_time, last_time);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorTimeRange(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, first_time, last_time);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorTimeRange(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, first_time, last_time);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KheLimitResourcesMonitorTimeRange(
	(KHE_LIMIT_RESOURCES_MONITOR) m, first_time, last_time);

    default:

      return *first_time = *last_time = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetSweepTime(KHE_MONITOR m, KHE_TIME t)                   */
/*                                                                           */
/*  Set the cutoff time of m to t, or do nothing if m is not open to         */
/*  handling cutoff times.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSetSweepTime(KHE_MONITOR m, KHE_TIME t)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorSetSweepTime(
	(KHE_ASSIGN_RESOURCE_MONITOR) m, t);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorSetSweepTime(
	(KHE_ASSIGN_TIME_MONITOR) m, t);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorSetSweepTime(
	(KHE_SPLIT_EVENTS_MONITOR) m, t);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorSetSweepTime(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, t);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorSetSweepTime(
	(KHE_PREFER_RESOURCES_MONITOR) m, t);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorSetSweepTime(
	(KHE_PREFER_TIMES_MONITOR) m, t);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorSetSweepTime(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, t);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorSetSweepTime(
	(KHE_SPREAD_EVENTS_MONITOR) m, t);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorSetSweepTime(
	(KHE_LINK_EVENTS_MONITOR) m, t);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorSetSweepTime(
	(KHE_ORDER_EVENTS_MONITOR) m, t);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorSetSweepTime(
	(KHE_AVOID_CLASHES_MONITOR) m, t);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorSetSweepTime(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, t);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorSetSweepTime(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, t);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorSetSweepTime(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, t);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorSetSweepTime(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, t);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorSetSweepTime(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, t);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorSetSweepTime(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, t);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorSetSweepTime(
	(KHE_LIMIT_RESOURCES_MONITOR) m, t);
      break;

    default:

      /* do nothing, this monitor not receptive to cutoff times */
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorSweepTimeRange(KHE_MONITOR m,                             */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  If m is influenced by sweep times, do what KheMonitorTimeRange does.     */
/*  Otherwise set *first_time and *last_time to NULL and return false.       */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorSweepTimeRange(KHE_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  switch( m->tag )
  {
      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	return KheAssignResourceMonitorSweepTimeRange(
	  (KHE_ASSIGN_RESOURCE_MONITOR) m, first_time, last_time);

      case KHE_ASSIGN_TIME_MONITOR_TAG:

	return KheAssignTimeMonitorSweepTimeRange(
	  (KHE_ASSIGN_TIME_MONITOR) m, first_time, last_time);

      case KHE_SPLIT_EVENTS_MONITOR_TAG:

	return KheSplitEventsMonitorSweepTimeRange(
	  (KHE_SPLIT_EVENTS_MONITOR) m, first_time, last_time);

      case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

	return KheDistributeSplitEventsMonitorSweepTimeRange(
	  (KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, first_time, last_time);

      case KHE_PREFER_RESOURCES_MONITOR_TAG:

	return KhePreferResourcesMonitorSweepTimeRange(
	  (KHE_PREFER_RESOURCES_MONITOR) m, first_time, last_time);

      case KHE_PREFER_TIMES_MONITOR_TAG:

	return KhePreferTimesMonitorSweepTimeRange(
	  (KHE_PREFER_TIMES_MONITOR) m, first_time, last_time);

      case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

	return KheAvoidSplitAssignmentsMonitorSweepTimeRange(
	  (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, first_time, last_time);

      case KHE_SPREAD_EVENTS_MONITOR_TAG:

	return KheSpreadEventsMonitorSweepTimeRange(
	  (KHE_SPREAD_EVENTS_MONITOR) m, first_time, last_time);

      case KHE_LINK_EVENTS_MONITOR_TAG:

	return KheLinkEventsMonitorSweepTimeRange(
	  (KHE_LINK_EVENTS_MONITOR) m, first_time, last_time);

      case KHE_ORDER_EVENTS_MONITOR_TAG:

	return KheOrderEventsMonitorSweepTimeRange(
	  (KHE_ORDER_EVENTS_MONITOR) m, first_time, last_time);

      case KHE_AVOID_CLASHES_MONITOR_TAG:

	return KheAvoidClashesMonitorSweepTimeRange(
	  (KHE_AVOID_CLASHES_MONITOR) m, first_time, last_time);

      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	return KheAvoidUnavailableTimesMonitorSweepTimeRange(
	  (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, first_time, last_time);

      case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

	return KheLimitIdleTimesMonitorSweepTimeRange(
	  (KHE_LIMIT_IDLE_TIMES_MONITOR) m, first_time, last_time);

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	return KheClusterBusyTimesMonitorSweepTimeRange(
	  (KHE_CLUSTER_BUSY_TIMES_MONITOR) m, first_time, last_time);

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	return KheLimitBusyTimesMonitorSweepTimeRange(
	  (KHE_LIMIT_BUSY_TIMES_MONITOR) m, first_time, last_time);

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	return KheLimitWorkloadMonitorSweepTimeRange(
	  (KHE_LIMIT_WORKLOAD_MONITOR) m, first_time, last_time);

      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	return KheLimitActiveIntervalsMonitorSweepTimeRange(
	  (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, first_time, last_time);

      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	return KheLimitResourcesMonitorSweepTimeRange(
	  (KHE_LIMIT_RESOURCES_MONITOR) m, first_time, last_time);

      default:

	return *first_time = *last_time = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorChangeCost(KHE_MONITOR m, KHE_COST new_cost)              */
/*                                                                           */
/*  Change the cost of m from m->cost to new_cost, including updating        */
/*  all ancestors and handling tracing.                                      */
/*                                                                           */
/*****************************************************************************/

void KheMonitorChangeCost(KHE_MONITOR m, KHE_COST new_cost)
{
  KHE_MONITOR_LINK link;  int i;  KHE_COST old_cost;
  HnAssert(new_cost >= 0, "KheMonitorChangeCost internal error");
  if( new_cost != m->cost )
  {
    /* KheMonitorCheck(m); */
    old_cost = m->cost;
    m->cost = new_cost;
    HaArrayForEach(m->parent_links, link, i)
      KheGroupMonitorChangeCost(link->parent, m, link, old_cost, new_cost);
    /* KheMonitorCheck(m); */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_LINK KheMonitorLinkCopyPhase1(KHE_MONITOR_LINK link,         */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of the copying of link.                                */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR_LINK KheMonitorLinkCopyPhase1(KHE_MONITOR_LINK link, HA_ARENA a)
{
  KHE_MONITOR_LINK copy;
  if( link->copy == NULL )
  {
    HaMake(copy, a);
    link->copy = copy;
    copy->parent = KheGroupMonitorCopyPhase1(link->parent, a);
    copy->child = KheMonitorCopyPhase1(link->child, a);
    copy->parent_index = link->parent_index;
    copy->parent_defects_index = link->parent_defects_index;
    copy->child_index = link->child_index;
    copy->copy = NULL;
  }
  return link->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorLinkCopyPhase2(KHE_MONITOR_LINK link)                     */
/*                                                                           */
/*  Carry out Phase 2 of the copying of link.                                */
/*                                                                           */
/*****************************************************************************/

void KheMonitorLinkCopyPhase2(KHE_MONITOR_LINK link)
{
  if( link->copy != NULL )
  {
    link->copy = NULL;
    KheGroupMonitorCopyPhase2(link->parent);
    KheMonitorCopyPhase2(link->child);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorInitCommonFields(KHE_MONITOR m, KHE_SOLN soln,            */
/*    KHE_MONITOR_TAG tag)                                                   */
/*                                                                           */
/*  Initialize the common fields of m, except for parent_links.              */
/*                                                                           */
/*****************************************************************************/

void KheMonitorInitCommonFields(KHE_MONITOR m, KHE_SOLN soln,
  KHE_MONITOR_TAG tag, KHE_COST_FUNCTION cost_function,
  KHE_COST combined_weight)
{
  m->soln = soln;
  m->tag = tag;
  m->cost_function = cost_function;
  m->attached = false;
  KheSolnAddMonitor(soln, m);  /* will set m->soln_index */
  m->visit_num = 0;
  m->back = NULL;
  m->id = NULL;
  /* HaArrayInit(res->parent_links, a); */
  /* m->trace_cost = 0; */
  m->cost = 0;
  m->lower_bound = 0;
  m->combined_weight = combined_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorCopyCommonFieldsPhase1(KHE_MONITOR copy,                  */
/*    KHE_MONITOR orig, HA_ARENA a)                                          */
/*                                                                           */
/*  Carry out the common fields part of Phase 1 of copying monitor orig.     */
/*                                                                           */
/*****************************************************************************/

void KheMonitorCopyCommonFieldsPhase1(KHE_MONITOR copy,
  KHE_MONITOR orig, HA_ARENA a)
{
  KHE_MONITOR_LINK link;  int i;
  copy->soln = KheSolnCopyPhase1(orig->soln);
  copy->tag = orig->tag;
  copy->attached = orig->attached;
  copy->soln_index = orig->soln_index;
  copy->visit_num = orig->visit_num;
  copy->back = orig->back;
  copy->id = NULL;  /* will be regenerated later if required */
  HaArrayInit(copy->parent_links, a);
  HaArrayForEach(orig->parent_links, link, i)
    HaArrayAddLast(copy->parent_links, KheMonitorLinkCopyPhase1(link, a));
  copy->cost = orig->cost;
  copy->lower_bound = orig->lower_bound;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorCopyCommonFieldsPhase2(KHE_MONITOR orig)                  */
/*                                                                           */
/*  Carry out the common fields part of Phase 2 of copying monitor orig.     */
/*                                                                           */
/*****************************************************************************/

void KheMonitorCopyCommonFieldsPhase2(KHE_MONITOR orig)
{
  KHE_MONITOR_LINK link;  int i;
  KheSolnCopyPhase2(orig->soln);
  HaArrayForEach(orig->parent_links, link, i)
    KheMonitorLinkCopyPhase2(link);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorSolnIndex(KHE_MONITOR m)                                   */
/*                                                                           */
/*  Return the index of m in soln.                                           */
/*                                                                           */
/*****************************************************************************/

int KheMonitorSolnIndex(KHE_MONITOR m)
{
  return m->soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetSolnIndex(KHE_MONITOR m, int soln_index)               */
/*                                                                           */
/*  Set the index of m in soln to soln_index.                                */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSetSolnIndex(KHE_MONITOR m, int soln_index)
{
  m->soln_index = soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheMonitorConstraint(KHE_MONITOR m)                       */
/*                                                                           */
/*  Return the constraint that m monitors one point of application of,       */
/*  or NULL if none.                                                         */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheMonitorConstraint(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheAssignResourceMonitorConstraint(
	(KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheAssignTimeMonitorConstraint(
	(KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheSplitEventsMonitorConstraint(
	(KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheDistributeSplitEventsMonitorConstraint(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KhePreferResourcesMonitorConstraint(
	(KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KhePreferTimesMonitorConstraint(
	(KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheAvoidSplitAssignmentsMonitorConstraint(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheSpreadEventsMonitorConstraint(
	(KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLinkEventsMonitorConstraint(
	(KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheOrderEventsMonitorConstraint(
	(KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheAvoidClashesMonitorConstraint(
	(KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheAvoidUnavailableTimesMonitorConstraint(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLimitIdleTimesMonitorConstraint(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheClusterBusyTimesMonitorConstraint(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLimitBusyTimesMonitorConstraint(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLimitWorkloadMonitorConstraint(
	(KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLimitActiveIntervalsMonitorConstraint(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return (KHE_CONSTRAINT) KheLimitResourcesMonitorConstraint(
	(KHE_LIMIT_RESOURCES_MONITOR) m);

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:

      /* these types do not monitor a constraint, so we return NULL */
      return NULL;

    default:

      HnAbort("KheMonitorConstraint: invalid monitor type tag");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorAppliesToName(KHE_MONITOR m)                             */
/*                                                                           */
/*  Return the name of the point of application of m, or NULL if none.       */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorAppliesToName(KHE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  KHE_EVENT e, e2;  int egi;  KHE_EVENT_GROUP eg;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;  KHE_RESOURCE r;  HA_ARENA a;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;   /* ARRAY_CHAR ac; */
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      er =KheAssignResourceMonitorEventResource((KHE_ASSIGN_RESOURCE_MONITOR)m);
      return KheEventName(KheEventResourceEvent(er));

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      e = KheAssignTimeMonitorEvent((KHE_ASSIGN_TIME_MONITOR) m);
      return KheEventName(e);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      e = KheSplitEventsMonitorEvent((KHE_SPLIT_EVENTS_MONITOR) m);
      return KheEventName(e);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      e = KheDistributeSplitEventsMonitorEvent(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      return KheEventName(e);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      er = KhePreferResourcesMonitorEventResource(
	(KHE_PREFER_RESOURCES_MONITOR) m);
      return KheEventName(KheEventResourceEvent(er));

    case KHE_PREFER_TIMES_MONITOR_TAG:

      e = KhePreferTimesMonitorEvent((KHE_PREFER_TIMES_MONITOR) m);
      return KheEventName(e);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      asac = KheAvoidSplitAssignmentsMonitorConstraint(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      eg = KheAvoidSplitAssignmentsConstraintEventGroup(asac, egi);
      return KheEventGroupName(eg);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      eg = KheSpreadEventsMonitorEventGroup((KHE_SPREAD_EVENTS_MONITOR) m);
      return KheEventGroupName(eg);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      eg = KheLinkEventsMonitorEventGroup((KHE_LINK_EVENTS_MONITOR) m);
      return KheEventGroupName(eg);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      a = KheSolnArena(m->soln);
      e = KheOrderEventsMonitorFirstEvent((KHE_ORDER_EVENTS_MONITOR) m);
      e2 = KheOrderEventsMonitorSecondEvent((KHE_ORDER_EVENTS_MONITOR) m);
      return HnStringMake(a, "%s,%s", KheEventName(e), KheEventName(e2));
      /* ***
      MStringInit(ac);
      e = KheOrderEventsMonitorFirstEvent((KHE_ORDER_EVENTS_MONITOR) m);
      MStringAddString(ac, KheEventName(e));
      MStringAddString(ac, ",");
      e = KheOrderEventsMonitorSecondEvent((KHE_ORDER_EVENTS_MONITOR) m);
      MStringAddString(ac, KheEventName(e));
      return MStringVal(ac);
      *** */

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      r = KheAvoidClashesMonitorResource((KHE_AVOID_CLASHES_MONITOR) m);
      return KheResourceName(r);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      r = KheAvoidUnavailableTimesMonitorResource(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      return KheResourceName(r);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      r = KheLimitIdleTimesMonitorResource((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      return KheResourceName(r);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      r = KheClusterBusyTimesMonitorResource((KHE_CLUSTER_BUSY_TIMES_MONITOR)m);
      return KheResourceName(r);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      r = KheLimitBusyTimesMonitorResource((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      return KheResourceName(r);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      r = KheLimitWorkloadMonitorResource((KHE_LIMIT_WORKLOAD_MONITOR) m);
      return KheResourceName(r);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      r = KheLimitActiveIntervalsMonitorResource(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      return KheResourceName(r);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      egi = KheLimitResourcesMonitorEventGroupIndex(
	(KHE_LIMIT_RESOURCES_MONITOR) m);
      lrc = KheLimitResourcesMonitorConstraint(
	(KHE_LIMIT_RESOURCES_MONITOR) m);
      eg = KheLimitResourcesConstraintEventGroup(lrc, egi);
      return eg == NULL ? "-" : KheEventGroupName(eg);

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:

      /* these types do not monitor a constraint, so we return NULL */
      return NULL;

    default:

      HnAbort("KheMonitorAppliesToName: invalid monitor type tag");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorPointOfApplication(KHE_MONITOR m)                        */
/*                                                                           */
/*  Return a precise description of the point of application of m, or NULL   */
/*  if none.                                                                 */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorPointOfApplication(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorPointOfApplication(
	(KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorPointOfApplication(
	(KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorPointOfApplication(
	(KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorPointOfApplication(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorPointOfApplication(
	(KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorPointOfApplication(
	(KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorPointOfApplication(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorPointOfApplication(
	(KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorPointOfApplication(
	(KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorPointOfApplication(
	(KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorPointOfApplication(
	(KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorPointOfApplication(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorPointOfApplication(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorPointOfApplication(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorPointOfApplication(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorPointOfApplication(
	(KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorPointOfApplication(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KheLimitResourcesMonitorPointOfApplication(
	(KHE_LIMIT_RESOURCES_MONITOR) m);

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:

      /* these types do not monitor a constraint, so we return NULL */
      return NULL;

    default:

      HnAbort("KheMonitorPointOfApplication: invalid monitor type tag");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorId(KHE_MONITOR m)                                        */
/*                                                                           */
/*  Return a precise description of the point of application of m, or NULL   */
/*  if none.                                                                 */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorId(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorId((KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorId((KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorId((KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorId(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorId((KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorId((KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorId(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorId((KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorId((KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorId((KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorId((KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorId(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorId((KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorId((KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorId((KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorId((KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorId(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KheLimitResourcesMonitorId((KHE_LIMIT_RESOURCES_MONITOR) m);

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      return "<event_timetable_monitor>";

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      return "<resource_timetable_monitor>";

    case KHE_GROUP_MONITOR_TAG:

      return "<group_monitor>";

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return "<ordinary_demand_monitor>";

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return "<workload_demand_monitor>";

    case KHE_EVENNESS_MONITOR_TAG:

      return "<evenness_monitor>";

    default:

      HnAbort("KheMonitorId: invalid monitor type tag");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheMonitorCopyPhase1(KHE_MONITOR m, HA_ARENA a)              */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheMonitorCopyPhase1(KHE_MONITOR m, HA_ARENA a)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return (KHE_MONITOR) KheAssignResourceMonitorCopyPhase1(
	(KHE_ASSIGN_RESOURCE_MONITOR) m, a);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return (KHE_MONITOR) KheAssignTimeMonitorCopyPhase1(
	(KHE_ASSIGN_TIME_MONITOR) m, a);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheSplitEventsMonitorCopyPhase1(
	(KHE_SPLIT_EVENTS_MONITOR) m, a);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheDistributeSplitEventsMonitorCopyPhase1(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, a);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return (KHE_MONITOR) KhePreferResourcesMonitorCopyPhase1(
	(KHE_PREFER_RESOURCES_MONITOR) m, a);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return (KHE_MONITOR) KhePreferTimesMonitorCopyPhase1(
	(KHE_PREFER_TIMES_MONITOR) m, a);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheAvoidSplitAssignmentsMonitorCopyPhase1(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, a);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheSpreadEventsMonitorCopyPhase1(
	(KHE_SPREAD_EVENTS_MONITOR) m, a);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheLinkEventsMonitorCopyPhase1(
	(KHE_LINK_EVENTS_MONITOR) m, a);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return (KHE_MONITOR) KheOrderEventsMonitorCopyPhase1(
	(KHE_ORDER_EVENTS_MONITOR) m, a);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return (KHE_MONITOR) KheAvoidClashesMonitorCopyPhase1(
	(KHE_AVOID_CLASHES_MONITOR) m, a);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return (KHE_MONITOR) KheAvoidUnavailableTimesMonitorCopyPhase1(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, a);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return (KHE_MONITOR) KheLimitIdleTimesMonitorCopyPhase1(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, a);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return (KHE_MONITOR) KheClusterBusyTimesMonitorCopyPhase1(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, a);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return (KHE_MONITOR) KheLimitBusyTimesMonitorCopyPhase1(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, a);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return (KHE_MONITOR) KheLimitWorkloadMonitorCopyPhase1(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, a);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return (KHE_MONITOR) KheLimitActiveIntervalsMonitorCopyPhase1(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, a);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return (KHE_MONITOR) KheLimitResourcesMonitorCopyPhase1(
	(KHE_LIMIT_RESOURCES_MONITOR) m, a);

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      return (KHE_MONITOR) KheEventTimetableMonitorCopyPhase1(
	(KHE_EVENT_TIMETABLE_MONITOR) m, a);

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      return (KHE_MONITOR) KheResourceTimetableMonitorCopyPhase1(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m, a);

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      return (KHE_MONITOR) KheTimetableMonitorCopyPhase1(
	(KHE_TIMETABLE_MONITOR) m, a);
    *** */

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return (KHE_MONITOR) KheOrdinaryDemandMonitorCopyPhase1(
	(KHE_ORDINARY_DEMAND_MONITOR) m, a);

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return (KHE_MONITOR) KheWorkloadDemandMonitorCopyPhase1(
	(KHE_WORKLOAD_DEMAND_MONITOR) m, a);

    case KHE_EVENNESS_MONITOR_TAG:

      return (KHE_MONITOR) KheEvennessMonitorCopyPhase1(
	(KHE_EVENNESS_MONITOR) m, a);

    case KHE_GROUP_MONITOR_TAG:

      return (KHE_MONITOR) KheGroupMonitorCopyPhase1(
	(KHE_GROUP_MONITOR) m, a);

    default:

      HnAbort("KheMonitorCopyPhase1: invalid monitor type tag");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorCopyPhase2(KHE_MONITOR m)                                 */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheMonitorCopyPhase2(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorCopyPhase2((KHE_ASSIGN_RESOURCE_MONITOR) m);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorCopyPhase2((KHE_ASSIGN_TIME_MONITOR) m);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorCopyPhase2((KHE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorCopyPhase2(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorCopyPhase2((KHE_PREFER_RESOURCES_MONITOR) m);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorCopyPhase2((KHE_PREFER_TIMES_MONITOR) m);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorCopyPhase2(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorCopyPhase2((KHE_SPREAD_EVENTS_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorCopyPhase2((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorCopyPhase2((KHE_ORDER_EVENTS_MONITOR) m);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorCopyPhase2((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorCopyPhase2(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorCopyPhase2((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorCopyPhase2((KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorCopyPhase2((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorCopyPhase2((KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorCopyPhase2(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorCopyPhase2((KHE_LIMIT_RESOURCES_MONITOR) m);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorCopyPhase2((KHE_EVENT_TIMETABLE_MONITOR) m);
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorCopyPhase2((KHE_RESOURCE_TIMETABLE_MONITOR) m);
      break;

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorCopyPhase2((KHE_TIMETABLE_MONITOR) m);
      break;
    *** */

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorCopyPhase2((KHE_ORDINARY_DEMAND_MONITOR) m);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorCopyPhase2((KHE_WORKLOAD_DEMAND_MONITOR) m);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorCopyPhase2((KHE_EVENNESS_MONITOR) m);
      break;

    case KHE_GROUP_MONITOR_TAG:

      KheGroupMonitorCopyPhase2((KHE_GROUP_MONITOR) m);
      break;

    default:

      HnAbort("KheMonitorCopyPhase2: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDelete(KHE_MONITOR m)                                     */
/*                                                                           */
/*  Delete m.  It must be detached.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorDelete(KHE_MONITOR m)
{
  HnAssert(!m->attached, "KheMonitorDelete internal error");
  if( m->id != NULL )
    MFree(m->id);
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorDelete((KHE_ASSIGN_RESOURCE_MONITOR) m);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorDelete((KHE_ASSIGN_TIME_MONITOR) m);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorDelete((KHE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorDelete(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorDelete((KHE_PREFER_RESOURCES_MONITOR) m);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorDelete((KHE_PREFER_TIMES_MONITOR) m);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorDelete(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorDelete((KHE_SPREAD_EVENTS_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorDelete((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorDelete((KHE_ORDER_EVENTS_MONITOR) m);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorDelete((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorDelete(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorDelete((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorDelete((KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorDelete((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorDelete((KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorDelete(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorDelete((KHE_LIMIT_RESOURCES_MONITOR) m);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorDoDelete((KHE_EVENT_TIMETABLE_MONITOR) m);
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorDelete((KHE_RESOURCE_TIMETABLE_MONITOR) m);
      break;

    ** ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorDelete((KHE_TIMETABLE_MONITOR) m);
      break;
    *** **

    case KHE_GROUP_MONITOR_TAG:

      KheGroupMonitorDelete((KHE_GROUP_MONITOR) m);
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDem andMonitorDelete((KHE_ORDINARY_DEMAND_MONITOR) m);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDem andMonitorDelete((KHE_WORKLOAD_DEMAND_MONITOR) m);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorDelete((KHE_EVENNESS_MONITOR) m);
      break;

    default:

      HnAbort("KheMonitorDelete: invalid monitor type tag");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parent monitors"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddParentLink(KHE_MONITOR m, KHE_MONITOR_LINK link)       */
/*                                                                           */
/*  Add link as a parent link to m; also set link's parent index.            */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAddParentLink(KHE_MONITOR m, KHE_MONITOR_LINK link)
{
  link->child_index = HaArrayCount(m->parent_links);
  HaArrayAddLast(m->parent_links, link);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteParentLink(KHE_MONITOR m, KHE_MONITOR_LINK link)    */
/*                                                                           */
/*  Delete link from m's parent links.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteParentLink(KHE_MONITOR m, KHE_MONITOR_LINK link)
{
  KHE_MONITOR_LINK link2;
  HnAssert(link->child_index != -1,
    "KheMonitorDeleteParentLink internal error 1");
  HnAssert(HaArray(m->parent_links, link->child_index) == link,
    "KheMonitorDeleteParentLink internal error 2");
  link2 = HaArrayLastAndDelete(m->parent_links);
  if( link2 != link )
  {
    HaArrayPut(m->parent_links, link->child_index, link2);
    link2->child_index = link->child_index;
  }
  link->child_index = -1;  /* actually link will be deleted shortly */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorHasParentLink(KHE_MONITOR m, KHE_GROUP_MONITOR gm,        */
/*    KHE_MONITOR_LINK *link)                                                */
/*                                                                           */
/*  Return true if gm is a parent of m, and set *link if so.                 */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorHasParentLink(KHE_MONITOR m, KHE_GROUP_MONITOR gm,
  KHE_MONITOR_LINK *link)
{
  int i;
  HaArrayForEach(m->parent_links, *link, i)
    if( (*link)->parent == gm )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorParentMonitorCount(KHE_MONITOR m)                          */
/*                                                                           */
/*  Return the number of parent monitors of m.                               */
/*                                                                           */
/*****************************************************************************/

int KheMonitorParentMonitorCount(KHE_MONITOR m)
{
  return HaArrayCount(m->parent_links);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheMonitorParentMonitor(KHE_MONITOR m, int i)          */
/*                                                                           */
/*  Return the ith parent monitor of m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheMonitorParentMonitor(KHE_MONITOR m, int i)
{
  KHE_MONITOR_LINK link;
  link = HaArray(m->parent_links, i);
  return link->parent;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorDescendant(KHE_MONITOR m1, KHE_MONITOR m2)                */
/*                                                                           */
/*  Return true if m1 is a descendant of m2.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorDescendant(KHE_MONITOR m1, KHE_MONITOR m2)
{
  return KheMonitorPathCount(m1, m2) >= 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorLinkTypedCmp(KHE_MONITOR_LINK link1,                       */
/*    KHE_MONITOR_LINK link2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of monitor links so       */
/*  that their parent monitors are in soln_index order.                      */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorLinkParentTypedCmp(KHE_MONITOR_LINK link1,
  KHE_MONITOR_LINK link2)
{
  KHE_MONITOR m1 = (KHE_MONITOR) link1->parent;
  KHE_MONITOR m2 = (KHE_MONITOR) link2->parent;
  return m1->soln_index - m2->soln_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorLinkParentCmp(const void *t1, const void *t2)              */
/*                                                                           */
/*  Untyped comparison function for sorting an array of monitor links so     */
/*  that their parent monitors are in soln_index order.                      */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorLinkParentCmp(const void *t1, const void *t2)
{
  KHE_MONITOR_LINK link1 = * (KHE_MONITOR_LINK *) t1;
  KHE_MONITOR_LINK link2 = * (KHE_MONITOR_LINK *) t2;
  return KheMonitorLinkParentTypedCmp(link1, link2);
}

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorParentMonitorsSort(KHE_MONITOR m)                         */
/*                                                                           */
/*  Sort the parents of m so that the usual traversal will visit them        */
/*  in increasing soln_index order.                                          */
/*                                                                           */
/*  Implementation note.  The indexes of the parent links in m's list        */
/*  of parent links need to be (and are) updated after the sort.             */
/*                                                                           */
/*****************************************************************************/

void KheMonitorParentMonitorsSort(KHE_MONITOR m)
{
  KHE_MONITOR_LINK parent_link;  int i;
  HaArraySort(m->parent_links, &KheMonitorLinkParentCmp);
  HaArrayForEach(m->parent_links, parent_link, i)
    parent_link->child_index = i;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteAllParentMonitors(KHE_MONITOR m)                    */
/*                                                                           */
/*  Disconnect m from all its parent monitors.                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteAllParentMonitors(KHE_MONITOR m)
{
  KHE_MONITOR_LINK link;
  while( HaArrayCount(m->parent_links) > 0 )
  {
    link = HaArrayLast(m->parent_links);
    KheGroupMonitorDeleteChildMonitor(link->parent, m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorParentIndex(KHE_MONITOR m)                                 */
/*                                                                           */
/*  Return the parent_index attribute of m.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheMonitorParentIndex(KHE_MONITOR m)
{
  return m->parent_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetParentMonitorAndIndex(KHE_MONITOR m,                   */
/*    KHE_GROUP_MONITOR parent, int parent_index)                            */
/*                                                                           */
/*  Set the parent and parent_index attributes of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorSetParentMonitorAndIndex(KHE_MONITOR m,
  KHE_GROUP_MONITOR parent, int parent_index)
{
  m->parent = parent;
  m->parent_index = parent_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDefectIndex(KHE_MONITOR m)                                 */
/*                                                                           */
/*  Return m's index in its parent monitor's list of defects.                */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheMonitorDefectIndex(KHE_MONITOR m)
{
  return m->defect_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSetDefectIndex(KHE_MONITOR m, int index)                  */
/*                                                                           */
/*  Set m's defect index.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorSetDefectIndex(KHE_MONITOR m, int index)
{
  m->defect_index = index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAttachToSoln(KHE_MONITOR m)                               */
/*                                                                           */
/*  Attach m to soln.  Make sure first that it is detached.                  */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAttachToSoln(KHE_MONITOR m)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheMonitorAttachToSoln(m %p %s) attached %d\n",
      (void *) m, KheMonitorTagShow(m->tag), (int) m->attached);
  HnAssert(!m->attached,
    "KheMonitorAttachToSoln: %s monitor %s already attached",
    KheMonitorTagShow(m->tag), KheMonitorId(m));
  HnAssert(m->cost == 0 || m->tag == KHE_GROUP_MONITOR_TAG,
    "KheMonitorAttachToSoln internal error");
  /* ***
  if( m->parent != NULL )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  prnt %p\n", (void *) m->parent);
      fprintf(stderr, "  prnt %s attached %d\n",
	KheMonitorTagShow(KheMonitorTag((KHE_MONITOR) m->parent)),
	(int) ((KHE_MONITOR) m->parent)->attached);
    }
    if( !KheMonitorAttached((KHE_MONITOR) m->parent) )
      KheMonitorAttach((KHE_MONITOR) m->parent);
  }
  *** */
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorAttachToSoln((KHE_ASSIGN_RESOURCE_MONITOR) m);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorAttachToSoln((KHE_ASSIGN_TIME_MONITOR) m);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorAttachToSoln((KHE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorAttachToSoln(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorAttachToSoln((KHE_PREFER_RESOURCES_MONITOR) m);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorAttachToSoln((KHE_PREFER_TIMES_MONITOR) m);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorAttachToSoln(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorAttachToSoln((KHE_SPREAD_EVENTS_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorAttachToSoln((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorAttachToSoln((KHE_ORDER_EVENTS_MONITOR) m);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorAttachToSoln((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorAttachToSoln(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorAttachToSoln((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorAttachToSoln(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorAttachToSoln((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorAttachToSoln((KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorAttachToSoln(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorAttachToSoln((KHE_LIMIT_RESOURCES_MONITOR) m);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorAttachToSoln((KHE_EVENT_TIMETABLE_MONITOR) m);
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorAttachToSoln(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m);
      break;

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorAttachToSoln((KHE_TIMETABLE_MONITOR) m);
      break;
    *** */

    case KHE_GROUP_MONITOR_TAG:

      KheGroupMonitorAttachToSoln((KHE_GROUP_MONITOR) m);
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorAttachToSoln((KHE_ORDINARY_DEMAND_MONITOR) m);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorAttachToSoln((KHE_WORKLOAD_DEMAND_MONITOR) m);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorAttachToSoln((KHE_EVENNESS_MONITOR) m);
      break;

    default:

      HnAbort("KheMonitorAttachToSoln: invalid monitor type tag");
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheMonitorAttachToSoln(m %p) attached %d returning\n",
      (void *) m, (int) m->attached);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDetachFromSoln(KHE_MONITOR m)                             */
/*                                                                           */
/*  Detach m.  Check first that it is currently attached.                    */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDetachFromSoln(KHE_MONITOR m)
{
  HnAssert(m->attached, "KheMonitorDetachFromSoln: monitor already detached");
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorDetachFromSoln((KHE_ASSIGN_RESOURCE_MONITOR) m);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorDetachFromSoln((KHE_ASSIGN_TIME_MONITOR) m);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorDetachFromSoln((KHE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorDetachFromSoln(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorDetachFromSoln((KHE_PREFER_RESOURCES_MONITOR) m);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorDetachFromSoln((KHE_PREFER_TIMES_MONITOR) m);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorDetachFromSoln(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorDetachFromSoln((KHE_SPREAD_EVENTS_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorDetachFromSoln((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorDetachFromSoln((KHE_ORDER_EVENTS_MONITOR) m);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorDetachFromSoln((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorDetachFromSoln(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorDetachFromSoln((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorDetachFromSoln(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorDetachFromSoln((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorDetachFromSoln((KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorDetachFromSoln(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorDetachFromSoln((KHE_LIMIT_RESOURCES_MONITOR) m);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorDetachFromSoln((KHE_EVENT_TIMETABLE_MONITOR) m);
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorDetachFromSoln(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m);
      break;

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorDetachFromSoln((KHE_TIMETABLE_MONITOR) m);
      break;
    *** */

    case KHE_GROUP_MONITOR_TAG:

      KheGroupMonitorDetachFromSoln((KHE_GROUP_MONITOR) m);
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorDetachFromSoln((KHE_ORDINARY_DEMAND_MONITOR) m);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorDetachFromSoln((KHE_WORKLOAD_DEMAND_MONITOR) m);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorDetachFromSoln((KHE_EVENNESS_MONITOR) m);
      break;

    default:

      HnAbort("KheMonitorDetachFromSoln: invalid monitor type tag");
  }
  HnAssert(m->cost == 0 || m->tag == KHE_GROUP_MONITOR_TAG,
    "KheMonitorDetachFromSoln (%s) internal error (final cost %.5f)",
    KheMonitorTagShow(m->tag), KheCostShow(m->cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAttachCheck(KHE_MONITOR m)                                */
/*                                                                           */
/*  Check whether m needs to be attached to the soln, and make sure it is    */
/*  attached or detached as appropriate.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorAttachCheck(KHE_MONITOR m)
{
  if( DEBUG1 )
    fprintf(stderr, "[ KheMonitorAttachCheck(m %p %s) attached %d\n",
      (void *) m, KheMonitorTagShow(m->tag), (int) m->attached);
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorAttachCheck((KHE_ASSIGN_RESOURCE_MONITOR) m);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorAttachCheck((KHE_ASSIGN_TIME_MONITOR) m);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorAttachCheck((KHE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorAttachCheck(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorAttachCheck((KHE_PREFER_RESOURCES_MONITOR) m);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorAttachCheck((KHE_PREFER_TIMES_MONITOR) m);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorAttachCheck(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorAttachCheck((KHE_SPREAD_EVENTS_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorAttachCheck((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorAttachCheck((KHE_ORDER_EVENTS_MONITOR) m);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorAttachCheck((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorAttachCheck(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorAttachCheck((KHE_LIMIT_IDLE_TIMES_MONITOR) m);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorAttachCheck((KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorAttachCheck((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorAttachCheck((KHE_LIMIT_WORKLOAD_MONITOR) m);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorAttachCheck(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
      break;

    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorAttachCheck((KHE_TIMETABLE_MONITOR) m);
      break;

    case KHE_GROUP_MONITOR_TAG:

      HnAbort("KheMonitorAttachCheck: m is a group monitor");
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorAttachCheck((KHE_ORDINARY_DEMAND_MONITOR) m);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorAttachCheck((KHE_WORKLOAD_DEMAND_MONITOR) m);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorAttachCheck((KHE_EVENNESS_MONITOR) m);
      break;

    default:

      HnAbort("KheMonitorAttachCheck: invalid monitor type tag");
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheMonitorAttachCheck(m %p) attached %d returning\n",
      (void *) m, (int) m->attached);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "calls emanating from KHE_EVENT_IN_SOLN objects"               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddMeet(KHE_MONITOR m, KHE_MEET meet)                     */
/*                                                                           */
/*  Inform m that meet has been added to its purview.                        */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAddMeet(KHE_MONITOR m, KHE_MEET meet)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorAddMeet((KHE_ASSIGN_TIME_MONITOR) m, meet);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorAddMeet((KHE_PREFER_TIMES_MONITOR) m, meet);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorAddMeet((KHE_SPLIT_EVENTS_MONITOR) m, meet);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorAddMeet(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, meet);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorAddMeet((KHE_EVENT_TIMETABLE_MONITOR) m, meet);
      break;

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorAddMeet((KHE_TIMETABLE_MONITOR) m, meet);
      break;
    *** */

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorAddMeet((KHE_SPREAD_EVENTS_MONITOR) m, meet);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorAddMeet((KHE_ORDER_EVENTS_MONITOR) m, meet);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAddMeet: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteMeet(KHE_MONITOR m, KHE_MEET meet)                  */
/*                                                                           */
/*  Inform m that meet has been deleted.                                     */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteMeet(KHE_MONITOR m, KHE_MEET meet)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorDeleteMeet((KHE_ASSIGN_TIME_MONITOR) m, meet);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorDeleteMeet((KHE_PREFER_TIMES_MONITOR) m, meet);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorDeleteMeet((KHE_SPLIT_EVENTS_MONITOR) m, meet);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorDeleteMeet(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, meet);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorDeleteMeet((KHE_EVENT_TIMETABLE_MONITOR) m, meet);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorDeleteMeet((KHE_SPREAD_EVENTS_MONITOR) m, meet);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorDeleteMeet((KHE_ORDER_EVENTS_MONITOR) m, meet);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorDeleteMeet: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSplitMeet(KHE_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)  */
/*                                                                           */
/*  Inform m that a split into meet1 and meet2 has occurred.                 */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSplitMeet(KHE_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorSplitMeet((KHE_ASSIGN_TIME_MONITOR) m, meet1, meet2);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorSplitMeet((KHE_PREFER_TIMES_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorSplitMeet((KHE_SPLIT_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorSplitMeet(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, meet1, meet2);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorSplitMeet((KHE_EVENT_TIMETABLE_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorSplitMeet((KHE_SPREAD_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorSplitMeet((KHE_ORDER_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorSplitMeet: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorMergeMeet(KHE_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)  */
/*                                                                           */
/*  Inform m that a merge of meet1 and meet2 is occurring.                   */
/*                                                                           */
/*****************************************************************************/

void KheMonitorMergeMeet(KHE_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorMergeMeet((KHE_ASSIGN_TIME_MONITOR) m, meet1, meet2);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorMergeMeet((KHE_PREFER_TIMES_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorMergeMeet((KHE_SPLIT_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorMergeMeet(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, meet1, meet2);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorMergeMeet((KHE_EVENT_TIMETABLE_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorMergeMeet((KHE_SPREAD_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorMergeMeet((KHE_ORDER_EVENTS_MONITOR) m,
	meet1, meet2);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorMergeMeet: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAssignTime(KHE_MONITOR m, KHE_MEET meet,                  */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform m that a time assignment of meet is occurring.                    */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAssignTime(KHE_MONITOR m, KHE_MEET meet,
  int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorAssignTime((KHE_ASSIGN_TIME_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorAssignTime((KHE_PREFER_TIMES_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      if( DEBUG5 )
	fprintf(stderr, "  KheMonitorAssignTime(m, %s, %d) calling "
	  "  KheEventTimetableMonitorAssignTime\n", KheMeetId(meet),
          assigned_time_index);
      KheEventTimetableMonitorAssignTime((KHE_EVENT_TIMETABLE_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorAssignTime((KHE_SPREAD_EVENTS_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorAssignTime((KHE_ORDER_EVENTS_MONITOR) m,
	meet, assigned_time_index);
      break;

    /* these two don't need this call and we optimize by omitting them */
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAssignTime: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorUnAssignTime(KHE_MONITOR m, KHE_MEET meet,                */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform m that a time unassignment of meet is occurring.                  */
/*                                                                           */
/*****************************************************************************/

void KheMonitorUnAssignTime(KHE_MONITOR m, KHE_MEET meet,
  int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorUnAssignTime((KHE_ASSIGN_TIME_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorUnAssignTime((KHE_PREFER_TIMES_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorUnAssignTime((KHE_EVENT_TIMETABLE_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorUnAssignTime((KHE_SPREAD_EVENTS_MONITOR) m,
	meet, assigned_time_index);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorUnAssignTime((KHE_ORDER_EVENTS_MONITOR) m,
	meet, assigned_time_index);
      break;

    /* these two don't need this call and we optimize by omitting them */
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorUnAssignTime: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "calls emanating from KHE_EVENT_RESOURCE_IN_SOLN objects"      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddTask(KHE_MONITOR m, KHE_TASK task)                     */
/*                                                                           */
/*  Monitor the effect of adding task.                                       */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAddTask(KHE_MONITOR m, KHE_TASK task)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorAddTask(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorAddTask(
        (KHE_PREFER_RESOURCES_MONITOR) m, task);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorAddTask(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorAddTask((KHE_LIMIT_RESOURCES_MONITOR) m, task);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAddTask: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteTask(KHE_MONITOR m, KHE_TASK task)                  */
/*                                                                           */
/*  Monitor the effect of deleting task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteTask(KHE_MONITOR m, KHE_TASK task)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorDeleteTask(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorDeleteTask(
        (KHE_PREFER_RESOURCES_MONITOR) m, task);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorDeleteTask(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorDeleteTask((KHE_LIMIT_RESOURCES_MONITOR) m, task);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorDeleteTask: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorSplitTask(KHE_MONITOR m, KHE_TASK task1, KHE_TASK task2)  */
/*                                                                           */
/*  Let m know that a task has just split into task1 and task2.              */
/*  Either both tasks are assigned, or they aren't.  This                    */
/*  call emanates from KHE_RESOURCE_IN_SOLN as well.                         */
/*                                                                           */
/*****************************************************************************/

void KheMonitorSplitTask(KHE_MONITOR m, KHE_TASK task1, KHE_TASK task2)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorSplitTask(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task1, task2);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorSplitTask(
        (KHE_PREFER_RESOURCES_MONITOR) m, task1, task2);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorSplitTask(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task1, task2);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorSplitTask((KHE_LIMIT_RESOURCES_MONITOR) m,
	task1, task2);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      /* called but nothing to do in this case */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorSplitTask((KHE_RESOURCE_TIMETABLE_MONITOR) m,
	task1, task2);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorSplitTask: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorMergeTask(KHE_MONITOR m, KHE_TASK task1, KHE_TASK task2)  */
/*                                                                           */
/*  Let m know that task1 and task2 are just about to be merged.             */
/*                                                                           */
/*****************************************************************************/

void KheMonitorMergeTask(KHE_MONITOR m, KHE_TASK task1, KHE_TASK task2)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorMergeTask(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task1, task2);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorMergeTask(
        (KHE_PREFER_RESOURCES_MONITOR) m, task1, task2);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorMergeTask(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task1, task2);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorMergeTask((KHE_LIMIT_RESOURCES_MONITOR) m,
	task1, task2);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      /* called but nothing to do in this case */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorMergeTask((KHE_RESOURCE_TIMETABLE_MONITOR) m,
	task1, task2);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorMergeTask: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAssignResource(KHE_MONITOR m, KHE_TASK task,              */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Let m know that task has just been assigned resource r.                  */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAssignResource(KHE_MONITOR m, KHE_TASK task, KHE_RESOURCE r)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorAssignResource(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task, r);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorAssignResource(
        (KHE_PREFER_RESOURCES_MONITOR) m, task, r);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorAssignResource(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task, r);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorAssignResource((KHE_LIMIT_RESOURCES_MONITOR) m,
	task, r);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      HnAbort("KheMonitorAssignResource: not accepting limit workload");
      /* ***
      KheLimitWorkloadMonitorAssignResource(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, task, r);
      *** */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorAssignResource(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m, task, r);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAssignResource: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorUnAssignResource(KHE_MONITOR m, KHE_TASK task,            */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Let m know that task has just been unassigned resource r.                */
/*                                                                           */
/*****************************************************************************/

void KheMonitorUnAssignResource(KHE_MONITOR m, KHE_TASK task,
  KHE_RESOURCE r)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorUnAssignResource(
        (KHE_ASSIGN_RESOURCE_MONITOR) m, task, r);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorUnAssignResource(
        (KHE_PREFER_RESOURCES_MONITOR) m, task, r);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorUnAssignResource(
        (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, task, r);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorUnAssignResource((KHE_LIMIT_RESOURCES_MONITOR) m,
	task, r);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      HnAbort("KheMonitorUnAssignResource: not accepting limit workload");
      /* ***
      KheLimitWorkloadMonitorUnAssignResource(
        (KHE_LIMIT_WORKLOAD_MONITOR) m, task, r);
      *** */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorUnAssignResource(
        (KHE_RESOURCE_TIMETABLE_MONITOR) m, task, r);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorUnAssignResource: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "calls emanating from KHE_RESOURCE_IN_SOLN objects"            */
/*                                                                           */
/*  KheMonitorSplitTask, KheMonitorMergeTask,                                */
/*  KheMonitorAssignResource and KheMonitorUnAssignResource also emanate     */
/*  from KHE_EVENT_RESOURCE_IN_SOLN objects, and appear above.               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorTaskAssignTime(KHE_MONITOR m, KHE_TASK task,              */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Let m know that task's meet has been assigned this time.                 */
/*                                                                           */
/*****************************************************************************/

void KheMonitorTaskAssignTime(KHE_MONITOR m, KHE_TASK task,
  int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      /* will be called but there is nothing to do */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorTaskAssignTime(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m, task, assigned_time_index);
      break;

    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorTaskAssignTime: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorTaskUnAssignTime(KHE_MONITOR m, KHE_TASK task,            */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Let m know that task's meet has been unassigned this time.               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorTaskUnAssignTime(KHE_MONITOR m, KHE_TASK task,
  int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      /* will be called but there is nothing to do */
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorTaskUnAssignTime(
	(KHE_RESOURCE_TIMETABLE_MONITOR) m, task, assigned_time_index);
      break;

    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorTaskUnAssignTime: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "calls emanating from KHE_TIMETABLE_MONITOR objects"           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAssignNonClash(KHE_MONITOR m, int assigned_time_index)    */
/*                                                                           */
/*  This time is changing from unassigned to assigned in the timetable.      */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorAssignNonClash(KHE_MONITOR m, int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorAssignNonClash(
	(KHE_LINK_EVENTS_MONITOR) m, assigned_time_index);
      break;


    case KHE_TIME_GROUP_MONITOR_TAG:

      KheTimeGroupMonitorAssignNonClash(
	(KHE_TIME_GROUP_MONITOR) m, assigned_time_index);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAssignNonClash: invalid monitor type tag");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorUnAssignNonClash(KHE_MONITOR m, int assigned_time_index)  */
/*                                                                           */
/*  This time is changing from assigned to unassigned in the timetable.      */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorUnAssignNonClash(KHE_MONITOR m, int assigned_time_index)
{
  switch( m->tag )
  {
    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorUnAssignNonClash(
	(KHE_LINK_EVENTS_MONITOR) m, assigned_time_index);
      break;

    case KHE_TIME_GROUP_MONITOR_TAG:

      KheTimeGroupMonitorUnAssignNonClash(
	(KHE_TIME_GROUP_MONITOR) m, assigned_time_index);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorUnAssignNonClash: invalid monitor type tag");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFlush(KHE_MONITOR m)                                      */
/*                                                                           */
/*  Flush m.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorFlush(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorFlush((KHE_AVOID_CLASHES_MONITOR) m);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorFlush((KHE_LINK_EVENTS_MONITOR) m);
      break;

    case KHE_TIME_GROUP_MONITOR_TAG:

      KheTimeGroupMonitorFlush((KHE_TIME_GROUP_MONITOR) m);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorFlush: invalid monitor type tag");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "calls emanating from KHE_TIME_GROUP_MONITOR objects"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddBusyAndIdle(KHE_MONITOR m, int index,                  */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add a link coming in to m with these values.                             */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAddBusyAndIdle(KHE_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  switch( m->tag )
  {
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorAddBusyAndIdle(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorAddBusyAndIdle(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorAddBusyAndIdle(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorAddBusyAndIdle(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorAddBusyAndIdle(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorAddBusyAndIdle(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorAddBusyAndIdle: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteBusyAndIdle(KHE_MONITOR m, int index,               */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Remove a link coming in to m with these values.                          */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteBusyAndIdle(KHE_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  switch( m->tag )
  {
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorDeleteBusyAndIdle(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorDeleteBusyAndIdle(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorDeleteBusyAndIdle(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorDeleteBusyAndIdle(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorDeleteBusyAndIdle(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, index, busy_and_idle);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorDeleteBusyAndIdle(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, index, busy_and_idle);
      break;


    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorDeleteBusyAndIdle: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorChangeBusyAndIdle(KHE_MONITOR m, int index,               */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change a link coming in to m from these old to new values.               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorChangeBusyAndIdle(KHE_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle, KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  switch( m->tag )
  {
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorChangeBusyAndIdle(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorChangeBusyAndIdle(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorChangeBusyAndIdle(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorChangeBusyAndIdle(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorChangeBusyAndIdle(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorChangeBusyAndIdle(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, index,
	old_busy_and_idle, new_busy_and_idle);
      break;


    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_LIMIT_RESOURCES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    case KHE_EVENNESS_MONITOR_TAG:
    default:

      HnAbort("KheMonitorChangeBusyAndIdle: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "operations on demand monitors"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheMonitorFirstCompetitor(KHE_MONITOR m)                     */
/*                                                                           */
/*  Return the first competitor of demand monitor m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MONITOR KheMonitorFirstCompetitor(KHE_MONITOR m)
{
  KHE_MATCHING sm;
  HnAssert(m->tag == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
    m->tag == KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    "KheMonitorFirstCompetitor: m is not a demand monitor");
  HnAssert(m->attached, "KheMonitorFirstCompetitor: m is not attached");
  sm = KheSolnMatching(m->soln);
  HnAssert(sm != NULL, "KheMonitorFirstCompetitor internal error: no matching");
  return (KHE_MONITOR) KheMatchingFirstCompetitor(sm,
    (KHE_MATCHING_DEMAND_NODE) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheMonitorNextCompetitor(KHE_MONITOR m)                      */
/*                                                                           */
/*  Return the next competitor of demand monitor m.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MONITOR KheMonitorNextCompetitor(KHE_MONITOR m)
{
  KHE_MATCHING sm;
  HnAssert(m->tag == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
    m->tag == KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    "KheMonitorNextCompetitor: m is not a demand monitor");
  HnAssert(m->attached, "KheMonitorNextCompetitor: m is not attached");
  sm = KheSolnMatching(m->soln);
  HnAssert(sm != NULL, "KheMonitorNextCompetitor internal error: no matching");
  return (KHE_MONITOR) KheMatchingNextCompetitor(sm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations and deviation parts"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDeviation(KHE_MONITOR m)                                   */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheMonitorDeviation(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorDeviation(
	(KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorDeviation(
	(KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorDeviation(
	(KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorDeviation(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorDeviation(
	(KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorDeviation(
	(KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorDeviation(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorDeviation(
	(KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorDeviation(
	(KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorDeviation(
	(KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorDeviation(
	(KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorDeviation(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorDeviation(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorDeviation(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorDeviation(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorDeviation(
	(KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorDeviation(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KheLimitResourcesMonitorDeviation((KHE_LIMIT_RESOURCES_MONITOR) m);

    /* deviation not really defined for these ones */
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:

      return 0;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return KheOrdinaryDemandMonitorDeviation(
	(KHE_ORDINARY_DEMAND_MONITOR) m);

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return KheWorkloadDemandMonitorDeviation(
	(KHE_WORKLOAD_DEMAND_MONITOR) m);

    case KHE_EVENNESS_MONITOR_TAG:

      return KheEvennessMonitorDeviation(
	(KHE_EVENNESS_MONITOR) m);

    default:

      HnAbort("KheMonitorDeviation: invalid monitor type tag");
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorDeviationDescription(KHE_MONITOR m)                      */
/*                                                                           */
/*  Return a description of how the deviation of m was calculated.           */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorDeviationDescription(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorDeviationDescription(
	(KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorDeviationDescription(
	(KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorDeviationDescription(
	(KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorDeviationDescription(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorDeviationDescription(
	(KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorDeviationDescription(
	(KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorDeviationDescription(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorDeviationDescription(
	(KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorDeviationDescription(
	(KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorDeviationDescription(
	(KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorDeviationDescription(
	(KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorDeviationDescription(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorDeviationDescription(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorDeviationDescription(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorDeviationDescription(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorDeviationDescription(
	(KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorDeviationDescription(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KheLimitResourcesMonitorDeviationDescription(
	(KHE_LIMIT_RESOURCES_MONITOR) m);

    /* not legal for these ones */
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_GROUP_MONITOR_TAG:

      HnAbort("KheMonitorDeviationDescription: not legal for this monitor");
      return "";  /* keep compiler happy */

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return KheOrdinaryDemandMonitorDeviationDescription(
	(KHE_ORDINARY_DEMAND_MONITOR) m);

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return KheWorkloadDemandMonitorDeviationDescription(
	(KHE_WORKLOAD_DEMAND_MONITOR) m);

    case KHE_EVENNESS_MONITOR_TAG:

      return KheEvennessMonitorDeviationDescription(
	(KHE_EVENNESS_MONITOR) m);

    default:

      HnAbort("KheMonitorDeviationDescription: invalid monitor tag");
      return "";  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDeviationPartCount(KHE_MONITOR m)                          */
/*                                                                           */
/*  Return the number of deviation parts of m.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheMonitorDeviationPartCount(KHE_MONITOR m)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorDeviationPartCount(
	(KHE_ASSIGN_RESOURCE_MONITOR) m);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorDeviationPartCount(
	(KHE_ASSIGN_TIME_MONITOR) m);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorDeviationPartCount(
	(KHE_SPLIT_EVENTS_MONITOR) m);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorDeviationPartCount(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorDeviationPartCount(
	(KHE_PREFER_RESOURCES_MONITOR) m);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorDeviationPartCount(
	(KHE_PREFER_TIMES_MONITOR) m);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorDeviationPartCount(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorDeviationPartCount(
	(KHE_SPREAD_EVENTS_MONITOR) m);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorDeviationPartCount(
	(KHE_LINK_EVENTS_MONITOR) m);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorDeviationPartCount(
	(KHE_ORDER_EVENTS_MONITOR) m);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorDeviationPartCount(
	(KHE_AVOID_CLASHES_MONITOR) m);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorDeviationPartCount(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorDeviationPartCount(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorDeviationPartCount(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorDeviationPartCount(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorDeviationPartCount(
	(KHE_LIMIT_WORKLOAD_MONITOR) m);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorDeviationPartCount(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);

    case KHE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:

      ** deviations not really defined for these ones **
      return 0;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return KheOrdinaryDemandMonitorDeviationPartCount(
	(KHE_ORDINARY_DEMAND_MONITOR) m);

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return KheWorkloadDemandMonitorDeviationPartCount(
	(KHE_WORKLOAD_DEMAND_MONITOR) m);

    case KHE_EVENNESS_MONITOR_TAG:

      return KheEvennessMonitorDeviationPartCount(
	(KHE_EVENNESS_MONITOR) m);

    default:

      HnAbort("KheMonitorDeviationPartCount: invalid monitor type tag");
      return 0;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeviationPart(KHE_MONITOR m, int i,                       */
/*    int *value, char **description)                                        */
/*                                                                           */
/*  Set *value to the value and *description to the description of the       */
/*  i'th deviation part of m.  The description could be NULL.                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheMonitorDeviationPart(KHE_MONITOR m, int i,
  int *value, char **description)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorDeviationPart(
	(KHE_ASSIGN_RESOURCE_MONITOR) m, i, value, description);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorDeviationPart(
	(KHE_ASSIGN_TIME_MONITOR) m, i, value, description);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorDeviationPart(
	(KHE_SPLIT_EVENTS_MONITOR) m, i, value, description);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorDeviationPart(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, i, value, description);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorDeviationPart(
	(KHE_PREFER_RESOURCES_MONITOR) m, i, value, description);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorDeviationPart(
	(KHE_PREFER_TIMES_MONITOR) m, i, value, description);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorDeviationPart(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, i, value, description);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorDeviationPart(
	(KHE_SPREAD_EVENTS_MONITOR) m, i, value, description);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorDeviationPart(
	(KHE_LINK_EVENTS_MONITOR) m, i, value, description);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorDeviationPart(
	(KHE_ORDER_EVENTS_MONITOR) m, i, value, description);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorDeviationPart(
	(KHE_AVOID_CLASHES_MONITOR) m, i, value, description);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorDeviationPart(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, i, value, description);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorDeviationPart(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, i, value, description);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorDeviationPart(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, i, value, description);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorDeviationPart(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, i, value, description);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorDeviationPart(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, i, value, description);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorDeviationPart(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, i, value, description);
      break;

    ** no legal value of i for these ones **
    case KHE_TIMETABLE_MONITOR_TAG:
    case KHE_GROUP_MONITOR_TAG:

      HnAbort("KheMonitorDeviationPart: no parts for this monitor");
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorDeviationPart(
	(KHE_ORDINARY_DEMAND_MONITOR) m, i, value, description);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorDeviationPart(
	(KHE_WORKLOAD_DEMAND_MONITOR) m, i, value, description);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorDeviationPart(
	(KHE_EVENNESS_MONITOR) m, i, value, description);
      break;

    default:

      HnAbort("KheMonitorDeviation: invalid monitor type tag");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorDeviationDescription(KHE_MONITOR m, int i)               */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheMonitorDeviationDescription(KHE_MONITOR m, int i)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KheAssignResourceMonitorDeviationDescription(
	(KHE_ASSIGN_RESOURCE_MONITOR) m, i);

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KheAssignTimeMonitorDeviationDescription(
	(KHE_ASSIGN_TIME_MONITOR) m, i);

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorDeviationDescription(
	(KHE_SPLIT_EVENTS_MONITOR) m, i);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorDeviationDescription(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, i);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KhePreferResourcesMonitorDeviationDescription(
	(KHE_PREFER_RESOURCES_MONITOR) m, i);

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KhePreferTimesMonitorDeviationDescription(
	(KHE_PREFER_TIMES_MONITOR) m, i);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KheAvoidSplitAssignmentsMonitorDeviationDescription(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, i);

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KheSpreadEventsMonitorDeviationDescription(
	(KHE_SPREAD_EVENTS_MONITOR) m, i);

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KheLinkEventsMonitorDeviationDescription(
	(KHE_LINK_EVENTS_MONITOR) m, i);

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KheOrderEventsMonitorDeviationDescription(
	(KHE_ORDER_EVENTS_MONITOR) m, i);

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KheAvoidClashesMonitorDeviationDescription(
	(KHE_AVOID_CLASHES_MONITOR) m, i);

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KheAvoidUnavailableTimesMonitorDeviationDescription(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, i);

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KheLimitIdleTimesMonitorDeviationDescription(
	(KHE_LIMIT_IDLE_TIMES_MONITOR) m, i);

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KheClusterBusyTimesMonitorDeviationDescription(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m, i);

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KheLimitBusyTimesMonitorDeviationDescription(
	(KHE_LIMIT_BUSY_TIMES_MONITOR) m, i);

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KheLimitWorkloadMonitorDeviationDescription(
	(KHE_LIMIT_WORKLOAD_MONITOR) m, i);

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KheLimitActiveIntervalsMonitorDeviationDescription(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, i);

    case KHE_TIMETABLE_MONITOR_TAG:

      return KheTimetableMonitorDeviationDescription(
	(KHE_TIMETABLE_MONITOR) m, i);

    case KHE_GROUP_MONITOR_TAG:

      return KheGroupMonitorDeviationDescription(
	(KHE_GROUP_MONITOR) m, i);

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return KheOrdinaryDemandMonitorDeviationDescription(
	(KHE_ORDINARY_DEMAND_MONITOR) m, i);

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return KheWorkloadDemandMonitorDeviationDescription(
	(KHE_WORKLOAD_DEMAND_MONITOR) m, i);

    case KHE_EVENNESS_MONITOR_TAG:

      return KheEvennessMonitorDeviationDescription(
	(KHE_EVENNESS_MONITOR) m, i);

    default:

      HnAbort("KheMonitorDeviationDescription: invalid monitor type tag");
      return NULL;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDebug(KHE_MONITOR m, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDebug(KHE_MONITOR m, int verbosity, int indent, FILE *fp)
{
  switch( m->tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      KheAssignResourceMonitorDebug((KHE_ASSIGN_RESOURCE_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      KheAssignTimeMonitorDebug((KHE_ASSIGN_TIME_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      KheSplitEventsMonitorDebug((KHE_SPLIT_EVENTS_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      KheDistributeSplitEventsMonitorDebug(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) m, verbosity, indent, fp);
      break;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      KhePreferResourcesMonitorDebug((KHE_PREFER_RESOURCES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      KhePreferTimesMonitorDebug((KHE_PREFER_TIMES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      KheAvoidSplitAssignmentsMonitorDebug(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m, verbosity, indent, fp);
      break;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      KheSpreadEventsMonitorDebug((KHE_SPREAD_EVENTS_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      KheLinkEventsMonitorDebug((KHE_LINK_EVENTS_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      KheOrderEventsMonitorDebug((KHE_ORDER_EVENTS_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      KheAvoidClashesMonitorDebug((KHE_AVOID_CLASHES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      KheAvoidUnavailableTimesMonitorDebug(
	(KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m, verbosity, indent, fp);
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      KheLimitIdleTimesMonitorDebug((KHE_LIMIT_IDLE_TIMES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      KheClusterBusyTimesMonitorDebug((KHE_CLUSTER_BUSY_TIMES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      KheLimitBusyTimesMonitorDebug((KHE_LIMIT_BUSY_TIMES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      KheLimitWorkloadMonitorDebug((KHE_LIMIT_WORKLOAD_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      KheLimitActiveIntervalsMonitorDebug(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m, verbosity, indent, fp);
      break;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      KheLimitResourcesMonitorDebug((KHE_LIMIT_RESOURCES_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      KheEventTimetableMonitorDebug((KHE_EVENT_TIMETABLE_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      KheResourceTimetableMonitorDebug((KHE_RESOURCE_TIMETABLE_MONITOR) m,
	verbosity, indent, fp);
      break;

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      KheTimetableMonitorDebug((KHE_TIMETABLE_MONITOR) m, verbosity,indent,fp);
      break;
    *** */

    case KHE_GROUP_MONITOR_TAG:

      KheGroupMonitorDebug((KHE_GROUP_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      KheOrdinaryDemandMonitorDebug((KHE_ORDINARY_DEMAND_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      KheWorkloadDemandMonitorDebug((KHE_WORKLOAD_DEMAND_MONITOR) m,
	verbosity, indent, fp);
      break;

    case KHE_EVENNESS_MONITOR_TAG:

      KheEvennessMonitorDebug((KHE_EVENNESS_MONITOR) m,
	verbosity, indent, fp);
      break;

    default:

      HnAbort("KheMonitorDebug: invalid monitor type tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorTagShow(KHE_MONITOR_TAG tag)                             */
/*                                                                           */
/*  Return a string version of tag.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorTagShow(KHE_MONITOR_TAG tag)
{
  switch( tag )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return "AssignResourceMonitor";

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return "AssignTimeMonitor";

    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return "SplitEventsMonitor";

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return "DistributeSplitEventsMonitor";

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return "PreferResourcesMonitor";

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return "PreferTimesMonitor";

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return "AvoidSplitAssignmentsMonitor";

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return "SpreadEventsMonitor";

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return "LinkEventsMonitor";

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return "OrderEventsMonitor";

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return "AvoidClashesMonitor";

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return "AvoidUnavailableTimesMonitor";

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return "LimitIdleTimesMonitor";

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return "ClusterBusyTimesMonitor";

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return "LimitBusyTimesMonitor";

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return "LimitWorkloadMonitor";

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return "LimitActiveIntervalsMonitor";

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return "LimitResourcesMonitor";

    case KHE_EVENT_TIMETABLE_MONITOR_TAG:

      return "EventTimetableMonitor";

    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:

      return "ResourceTimetableMonitor";

    /* ***
    case KHE_TIMETABLE_MONITOR_TAG:

      return "TimetableMonitor";
    *** */

    case KHE_GROUP_MONITOR_TAG:

      return "GroupMonitor";

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return "OrdinaryDemandMonitor";

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return "WorkloadDemandMonitor";

    case KHE_EVENNESS_MONITOR_TAG:

      return "EvennessMonitor";

    default:

      HnAbort("KheMonitorTagShow: invalid monitor type tag %d", tag);
      return "??";  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMonitorLabel(KHE_MONITOR m)                                     */
/*                                                                           */
/*  Return an identifying label for m:  its tag if it is not a group         */
/*  monitor, and its subtag label if it is.                                  */
/*                                                                           */
/*****************************************************************************/

char *KheMonitorLabel(KHE_MONITOR m)
{
  if( m->tag == KHE_GROUP_MONITOR_TAG )
    return KheGroupMonitorSubTagLabel((KHE_GROUP_MONITOR) m);
  else
    return KheMonitorTagShow(KheMonitorTag(m));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorIsLinkedToSoln(KHE_MONITOR m)                             */
/*                                                                           */
/*  Return true if m is linked, directly or indirectly, to its soln.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMonitorIsLinkedToSoln(KHE_MONITOR m)
{
  KHE_SOLN soln;
  soln = m->soln;
  while( m->parent != NULL )
    m = (KHE_MONITOR) m->parent;
  return m == (KHE_MONITOR) soln;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDebugWithTagBegin(KHE_MONITOR m, char *tag, int indent,   */
/*    FILE *fp)                                                              */
/*  void KheMonitorDebugBegin(KHE_MONITOR m, int indent, FILE *fp)           */
/*                                                                           */
/*  Begin a debug print of m onto fp.   KheMonitorDebugWithTagBegin allows   */
/*  the caller to specify the tag at the start; KheMonitorDebugBegin uses    */
/*  a string value of the monitor's tag to supply this tag.                  */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDebugWithTagBegin(KHE_MONITOR m, char *tag, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ %c%d %05d %-25s %11.5f",
    KheMonitorTag(m) == KHE_GROUP_MONITOR_TAG ?
    'G' : KheMonitorAttachedToSoln(m) ? 'A' : 'D',
    KheMonitorPathCount(m, (KHE_MONITOR) m->soln), m->soln_index,
    /* KheMonitorId(m), */ tag, KheCostShow(m->cost));
  if( m->lower_bound != 0 )
    fprintf(fp, " (lb %.5f)", KheCostShow(m->lower_bound));
}

void KheMonitorDebugBegin(KHE_MONITOR m, int indent, FILE *fp)
{
  KheMonitorDebugWithTagBegin(m, KheMonitorId(m), indent, fp);
  /* KheMonitorDebugWithTagBegin(m, KheMonitorTagShow(m->tag), indent, fp); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDebugEnd(KHE_MONITOR m, bool single_line,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  End a debug print of m onto fp.  If single_line is true, the entire      */
/*  debug has been on a single line, so the concluding ] can go on that      */
/*  same line.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDebugEnd(KHE_MONITOR m, bool single_line,
  int indent, FILE *fp)
{
  if( indent < 0 )
    fprintf(fp, " ]");
  else if ( single_line )
    fprintf(fp, " ]\n");
  else
    fprintf(fp, "%*s]\n", indent, "");
}

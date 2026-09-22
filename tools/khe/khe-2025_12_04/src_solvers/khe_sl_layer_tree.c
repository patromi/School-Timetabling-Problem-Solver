
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
/*  FILE:         khe_sl_layer_tree.c                                        */
/*  DESCRIPTION:  KheSplitLinkAndLayer() algorithm                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_sl_layer_tree.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddConstraintJobs(KHE_SOLN soln,                            */
/*    ARRAY_KHE_SPLIT_JOB *split_jobs)                                       */
/*                                                                           */
/*  Add to *split_jobs those jobs of ins that are derived from constraints.  */
/*                                                                           */
/*****************************************************************************/

static void KheSplitAddConstraintJobs(KHE_SOLN soln,
  ARRAY_KHE_SPLIT_JOB *split_jobs, HA_ARENA a)
{
  KHE_INSTANCE ins;  KHE_CONSTRAINT c;  int i;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    switch( KheConstraintTag(c) )
    {
      case KHE_SPLIT_EVENTS_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheSplitSplitJobMake((KHE_SPLIT_EVENTS_CONSTRAINT) c, a));
	break;

      case KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheDistributeSplitJobMake(
	    (KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT) c, a));
	break;

      case KHE_PREFER_TIMES_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheDomainSplitJobMake((KHE_PREFER_TIMES_CONSTRAINT) c, a));
	break;

      case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheSpreadSplitJobMake((KHE_SPREAD_EVENTS_CONSTRAINT) c, a));
	break;

      case KHE_LINK_EVENTS_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheLinkSplitJobMake((KHE_LINK_EVENTS_CONSTRAINT) c, a));
	break;

      case KHE_ORDER_EVENTS_CONSTRAINT_TAG:

	/* still to do here */
	break;

      case KHE_AVOID_CLASHES_CONSTRAINT_TAG:

	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheAvoidClashesSplitJobMake((KHE_AVOID_CLASHES_CONSTRAINT) c, a));
	break;

      case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:
      case KHE_ASSIGN_TIME_CONSTRAINT_TAG:
      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:
      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:
      case KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG:
      case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:
      case KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG:
      case KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG:

	/* nothing to do for these constraints */
	break;

      default:

	HnAbort("KheSplitAddConstraintJobs given unknown constraint type (%d)",
          KheConstraintTag(c));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddPreExistingSplitJobs(KHE_SOLN soln,                      */
/*    ARRAY_KHE_SPLIT_JOB *split_jobs, HA_ARENA a)                           */
/*                                                                           */
/*  Add pre-existing split jobs to *split_jobs.                              */
/*                                                                           */
/*****************************************************************************/

static void KheSplitAddPreExistingSplitJobs(KHE_SOLN soln,
  ARRAY_KHE_SPLIT_JOB *split_jobs, HA_ARENA a)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, j, durn;  KHE_PARTITION p;
  KHE_MEET meet;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    durn = KheEventDuration(e);
    if( KheEventMeetCount(soln, e) != 1 ||
	KheMeetDuration(KheEventMeet(soln, e, 0)) != durn )
    {
      p = KhePartitionMake(a);
      for( j = 0;  i < KheEventMeetCount(soln, e);  j++ )
      {
	meet = KheEventMeet(soln, e, j);
	KhePartitionAdd(p, KheMeetDuration(meet));
      }
      HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB) KhePackSplitJobMake(e, p, a));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddPreassignedSplitJobs(KHE_SOLN soln,                      */
/*    ARRAY_KHE_SPLIT_JOB *split_jobs)                                       */
/*                                                                           */
/*  Add jobs to *split_jobs installing preassigned times.                    */
/*                                                                           */
/*****************************************************************************/

static void KheSplitAddPreassignedSplitJobs(KHE_SOLN soln,
  ARRAY_KHE_SPLIT_JOB *split_jobs, HA_ARENA a)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  int i;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) != NULL )
      HaArrayAddLast(*split_jobs,
	(KHE_SPLIT_JOB) KhePreassignedSplitJobMake(e, a));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddLayerJobs(KHE_SOLN soln,                                 */
/*    ARRAY_KHE_SPLIT_JOB *split_jobs)                                       */
/*                                                                           */
/*  Add one job to *split_jobs for each pre-existing layer.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSplitAddLayerJobs(KHE_SOLN soln,
  ARRAY_KHE_SPLIT_JOB *split_jobs)
{
  int i;  KHE_LAYER layer;
  for( i = 0;  i < KheSolnLayerCount(soln);  i++ )
  {
    layer = KheSolnLayer(soln, i);
    HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB) KheLayerSplitJobMake(layer));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitAddAssignedSplitJobs(KHE_SOLN soln,                         */
/*    ARRAY_KHE_SPLIT_JOB *split_jobs)                                       */
/*                                                                           */
/*  Add jobs for installing existing assignments.                            */
/*                                                                           */
/*****************************************************************************/

static void KheSplitAddAssignedSplitJobs(KHE_SOLN soln,
  ARRAY_KHE_SPLIT_JOB *split_jobs, HA_ARENA a)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, j;  KHE_MEET meet, meet2;
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
    {
      meet = KheEventMeet(soln, e, j);
      meet2 = KheMeetAsst(meet);
      if( meet2 != NULL && KheMeetEvent(meet2) != NULL )
      {
	HaArrayAddLast(*split_jobs, (KHE_SPLIT_JOB)
	  KheAssignedSplitJobMake(e, KheMeetEvent(meet2), a));
	break;
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventHomogenizeAssignments(KHE_SOLN soln, KHE_EVENT e)      */
/*                                                                           */
/*  Homogenize the assignments of e, so that they are all either to          */
/*  meets of the same event, or to cycle meets.  Here e is not preassigned.  */
/*                                                                           */
/*****************************************************************************/

static void KheSplitEventHomogenizeAssignments(KHE_SOLN soln, KHE_EVENT e)
{
  int i;  KHE_MEET meet, first_asst;
  first_asst = NULL;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetAsst(meet) != NULL )
    {
      if( first_asst == NULL )
	first_asst = KheMeetAsst(meet);
      else if( KheMeetIsCycleMeet(first_asst) )
      {
	if( !KheMeetIsCycleMeet(KheMeetAsst(meet)) )
	{
	  /* first_asst is a cycle meet, but this isn't, so deassign */
	  if( DEBUG1 )
	    fprintf(stderr, "  unassigning meet of %s (not cycle)\n",
	      KheEventId(e) != NULL ? KheEventId(e) : "-");
	  KheMeetAssignUnFix(meet);
	  KheMeetUnAssign(meet);  /* meet known to be not preassigned */
	}
      }
      else if( KheMeetEvent(first_asst) != NULL )
      {
	if( KheMeetEvent(KheMeetAsst(meet)) !=
	    KheMeetEvent(first_asst) )
	{
	  /* first_asst is to event, but this asst isn't, so deassign */
	  if( DEBUG1 )
	    fprintf(stderr, "  unassigning meet of %s (not event)\n",
	      KheEventId(e) != NULL ? KheEventId(e) : "-");
	  KheMeetAssignUnFix(meet);
	  KheMeetUnAssign(meet);  /* meet known to be not preassigned */
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitLayerIsFullyAssigned(KHE_LAYER layer)                       */
/*                                                                           */
/*  Return true if every meet of layer is assigned.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheSplitLayerIsFullyAssigned(KHE_LAYER layer)
{
  int i;  KHE_MEET meet;
  for( i = 0;  i < KheLayerMeetCount(layer);  i++ )
  {
    meet = KheLayerMeet(layer, i);
    if( KheMeetAsst(meet) == NULL )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMeetsLieInNode(KHE_SOLN soln, KHE_EVENT e)                  */
/*                                                                           */
/*  Return true if the meets of e all lie in the same node.                  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheEventMeetsLieInNode(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_MEET meet;  int i;  KHE_NODE node;
  HnAssert(KheEventMeetCount(soln, e) >= 1,
    "KheEventMeetsLieInNode internal error");
  node = KheMeetNode(KheEventMeet(soln, e, 0));
  if( node == NULL )
    return false;
  for( i = 1;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetNode(meet) != node )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventSolEventsLieOutsideNodes(KHE_SOLN soln, KHE_EVENT e)        */
/*                                                                           */
/*  Return true if the meets of e all lie outside nodes.                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheEventSolEventsLieOutsideNodes(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_MEET meet;  int i;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetNode(meet) != NULL )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMeetsAssignedTo(KHE_SOLN soln, KHE_EVENT e,                 */
/*    KHE_EVENT leader_event)                                                */
/*                                                                           */
/*  Return true if all the meets of e are assigned to meets                  */
/*  of leader_event of the same duration as themselves.                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheEventMeetsAssignedTo(KHE_SOLN soln, KHE_EVENT e,
  KHE_EVENT leader_event)
{
  KHE_MEET meet;  int i;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KheMeetAsst(meet) == NULL ||
	KheMeetEvent(KheMeetAsst(meet)) != leader_event ||
	KheMeetDuration(meet) != KheMeetDuration(KheMeetAsst(meet)) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventMeetsAssignedDistinctly(KHE_SOLN soln, KHE_EVENT e)         */
/*                                                                           */
/*  Return true if the meets of e are assigned to distinct meets.            */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheEventMeetsAssignedDistinctly(KHE_SOLN soln, KHE_EVENT e)
{
  KHE_MEET meet, meet2;  int i, j;
  for( i = 1;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    for( j = 0;  j < i;  j++ )
    {
      meet2 = KheEventMeet(soln, e, j);
      if( KheMeetAsst(meet) == KheMeetAsst(meet2) )
	return false;
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerTreeLinkEventsMonitorMustHaveZeroCost(KHE_SOLN soln,        */
/*    KHE_EVENT e, KHE_LINK_EVENTS_MONITOR m)                                */
/*                                                                           */
/*  Return true if m must have zero cost, taking e as the leader event.      */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitorAttach Check
static bool KheLayerTreeLinkEventsMonitorMustHaveZeroCost(KHE_SOLN soln,
  KHE_EVENT e, KHE_LINK_EVENTS_MONITOR m)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e2;  int i;

  ** return false if e's meets don't lie in a common node **
  if( !KheEventMeetsLieInNode(soln, e) )
    return false;

  ** check the other events of the monitor's event group **
  eg = KheLinkEventsMonitorEventGroup(m);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e2 = KheEventGroupEvent(eg, i);
    if( e2 != e )
    {
      ** return false if e2's meets lie in nodes **
      if( !KheEventSolEventsLieOutsideNodes(soln, e2) )
	return false;

      ** return false if e2's meets are not assigned to soln **
      ** events of e of the same duration as themselves **
      if( !KheEventMeetsAssignedTo(soln, e2, e) )
	return false;

      ** return false if e2's meets are not assigned distinctly **
      if( !KheEventMeetsAssignedDistinctly(soln, e2) )
	return false;
    }
  }

  ** all tests passed **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerTreePreferTimesMonitorMustHaveZeroCost(KHE_SOLN soln,       */
/*    KHE_EVENT e, KHE_PREFER_TIMES_MONITOR m)                               */
/*                                                                           */
/*  Return true if this monitor must have cost 0, given the domains of the   */
/*  meets of e.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitorAttach Check
static bool KheLayerTreePreferTimesMonitorMustHaveZeroCost(KHE_SOLN soln,
  KHE_EVENT e, KHE_PREFER_TIMES_MONITOR m)
{
  KHE_MEET meet;  KHE_TIME_GROUP tg;  int i;
  KHE_PREFER_TIMES_CONSTRAINT c;
  c = KhePreferTimesMonitorConstraint(m);
  tg = KhePreferTimesConstraintDomain(c);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    if( KhePreferTimesConstraintDuration(c) == KHE_ANY_DURATION ||
	KhePreferTimesConstraintDuration(c) == KheMeetDuration(meet) )
    {
      ** m is applicable to meet **
      if( !KheTimeGroupSubset(KheMeetCurrentDomain(meet), tg) )
	return false;
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerTreePreferResourcesMonitorMustHaveZeroCost(KHE_SOLN soln,   */
/*    KHE_EVENT_RESOURCE er, KHE_PREFER_RESOURCES_MONITOR m)                 */
/*                                                                           */
/*  Return true if m must have cost 0, given the domains of the soln         */
/*  resources it is monitoring.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** replace by KheMonitorAttach Check (out of place in layer trees anyway)
static bool KheLayerTreePreferResourcesMonitorMustHaveZeroCost(KHE_SOLN soln,
  KHE_EVENT_RESOURCE er, KHE_PREFER_RESOURCES_MONITOR m)
{
  int i;  KHE_JOB job;  KHE_RESOURCE_GROUP rg;
  rg =
    KhePreferResource sConstraintDomain(KhePreferResourcesMonitorConstraint(m));
  for( i = 0;  i < KheEventResourceJobCount(soln, er);  i++ )
  {
    job = KheEventResourceJob(soln, er, i);
    if( !KheResourceGroupSubset(KheJobDomain(job), rg) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTreeCheckMonitors(KHE_SOLN soln,                            */
/*    bool check_prefer_times_monitors, bool check_split_events_monitors,    */
/*    bool check_link_events_monitors)                                       */
/*                                                                           */
/*  Detach monitors, if requested and appropriate.                           */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static void KheLayerTreeCheckMonitors(KHE_SOLN soln,
  bool check_prefer_times_monitors, bool check_split_events_monitors,
  bool check_link_events_monitors)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  KHE_MONITOR m;
  int i, j;
  ins = KheSolnInstance(soln);
  if( DEBUG3 )
    fprintf(stderr, "[ KheLayerTreeCheckMonitors(soln, %s, %s, %s):\n",
      check_prefer_times_monitors ? "true" : "false",
      check_split_events_monitors ? "true" : "false",
      check_link_events_monitors ? "true" : "false");
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    ** check event monitors as requested **
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheEventMonitorCount(soln, e);  j++ )
    {
      m = KheEventMonitor(soln, e, j);
      if( KheMonitorAttachedToSoln(m) )
	switch( KheMonitorTag(m) )
	{
	  case KHE_PREFER_TIMES_MONITOR_TAG:

	    if( check_prefer_times_monitors )
	      KheMonitorAttach Check(m);
	    break;


	  case KHE_SPLIT_EVENTS_MONITOR_TAG:
	  case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

	    if( check_split_events_monitors )
	      KheMonitorAttach Check(m);
	    break;


	  case KHE_LINK_EVENTS_MONITOR_TAG:

	    if( check_link_events_monitors )
	      KheMonitorAttach Check(m);
	    break;

	  default:

	    ** ignore other monitors **
	    break;
	}
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "] KheLayerTreeCheckMonitors returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTreeAddPreassignedResources(KHE_MEET meet,                  */
/*    ARRAY_KHE_RESOURCE *ra)                                                */
/*                                                                           */
/*  Add to *ra the preassigned resources of meet and the meets assigned      */
/*  (directly or indirectly) to meet.                                        */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

static void KheLayerTreeAddPreassignedResources(KHE_MEET meet,
  ARRAY_KHE_RESOURCE *ra)
{
  KHE_EVENT e;  int i;  KHE_MEET child_meet;  KHE_RESOURCE r;

  /* do it for meet itself */
  e = KheMeetEvent(meet);
  if( e != NULL )
    for( i = 0;  i < KheEventResourceCount(e);  i++ )
    {
      r = KheEventResourcePreassignedResource(KheEventResource(e, i));
      if( r != NULL )
	HaArrayAddLast(*ra, r);
    }

  /* do it for the child meets of meet */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    KheLayerTreeAddPreassignedResources(child_meet, ra);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLayerTreeResourceCmp(const void *t1, const void *t2)              */
/*                                                                           */
/*  Comparison function for sorting an array of resources by index.          */
/*                                                                           */
/*****************************************************************************/

static int KheLayerTreeResourceCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE r1 = * (KHE_RESOURCE *) t1;
  KHE_RESOURCE r2 = * (KHE_RESOURCE *) t2;
  return KheResourceInstanceIndex(r1) - KheResourceInstanceIndex(r2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerTreeResourceArraysEqual(ARRAY_KHE_RESOURCE *ra1,            */
/*    ARRAY_KHE_RESOURCE *ra2)                                               */
/*                                                                           */
/*  Return true if ra1 and ra2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheLayerTreeResourceArraysEqual(ARRAY_KHE_RESOURCE *ra1,
  ARRAY_KHE_RESOURCE *ra2)
{
  int i;  KHE_RESOURCE r1, r2;
  if( HaArrayCount(*ra1) != HaArrayCount(*ra2) )
    return false;
  for( i = 0;  i < HaArrayCount(*ra1);  i++ )
  {
    r1 = HaArray(*ra1, i);
    r2 = HaArray(*ra2, i);
    if( r1 != r2 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTreeMergeEventNodes(KHE_SOLN soln, KHE_EVENT e1,            */
/*    KHE_EVENT e2, ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)        */
/*                                                                           */
/*  Merge the nodes of soln containing the meets of e1 and e2, if suitable.  */
/*  Parameters ra1 and ra2 are scratch arrays of resources.                  */
/*                                                                           */
/*****************************************************************************/

static void KheLayerTreeMergeEventNodes(KHE_SOLN soln, KHE_EVENT e1,
  KHE_EVENT e2, ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)
{
  KHE_NODE node1, node2;  KHE_MEET meet1, meet2;  int junk;
  meet1 = KheMeetFirstMovable(KheEventMeet(soln, e1, 0), &junk);
  meet2 = KheMeetFirstMovable(KheEventMeet(soln, e2, 0), &junk);
  if( meet1 != NULL && meet2 != NULL )
  {
    node1 = KheMeetNode(meet1);
    node2 = KheMeetNode(meet2);
    if( node1 != NULL && node2 != NULL && node1 != node2 )
    {
      HaArrayClear(*ra1);
      KheLayerTreeAddPreassignedResources(meet1, ra1);
      HaArraySortUnique(*ra1, &KheLayerTreeResourceCmp);
      HaArrayClear(*ra2);
      KheLayerTreeAddPreassignedResources(meet2, ra2);
      HaArraySortUnique(*ra2, &KheLayerTreeResourceCmp);
      if( HaArrayCount(*ra1) >= 1 && KheLayerTreeResourceArraysEqual(ra1, ra2) )
      {
	if( DEBUG4 )
	{
	  fprintf(stderr, "  KheLayerTreeMergeEventNodes merging nodes:\n");
	  KheNodeDebug(node1, 2, 4, stderr);
	  KheNodeDebug(node2, 2, 4, stderr);
	}
	KheNodeMerge(node1, node2, &node1);
      }
    }
  }
}


/* *** old version not fussy enough about preassigned resources
static void KheLayerTreeMergeEventNodes(KHE_SOLN soln, KHE_EVENT e1,
  KHE_EVENT e2)
{
  KHE_RESOURCE r;  KHE_NODE node1, node2;
  node1 = KheMeetNode(KheEventMeet(soln, e1, 0));
  node2 = KheMeetNode(KheEventMeet(soln, e2, 0));
  if( node1 != NULL && node2 != NULL && node1 != node2 &&
      KheEventSharePreassignedResource(e1, e2, &r) &&
      KheEventMergeable(e1, e2, 1) )
    KheNodeMerge(node1, node2, &node1);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTreeMergeAvoidSplitNodes(KHE_SOLN soln,                     */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c,                              */
/*    ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)                      */
/*                                                                           */
/*  Merge nodes shared by the events of c, where possible.                   */
/*  Parameters ra1 and ra2 are scratch arrays of resources.                  */
/*                                                                           */
/*****************************************************************************/
#define KheAvoidSplitAssignmentsConstraintERCount \
  KheAvoidSplitAssignmentsConstraintEventResourceCount

static void KheLayerTreeMergeAvoidSplitNodes(KHE_SOLN soln,
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c,
  ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)
{
  int i, j, k;  KHE_EVENT_RESOURCE er1;  KHE_EVENT_RESOURCE er2;
  for( i = 0;  i < KheAvoidSplitAssignmentsConstraintEventGroupCount(c);  i++ )
  {
    for( j = 0;  j < KheAvoidSplitAssignmentsConstraintERCount(c, i);  j++ )
    {
      er1 = KheAvoidSplitAssignmentsConstraintEventResource(c, i, j);
      for( k = j+1;  k < KheAvoidSplitAssignmentsConstraintERCount(c, i);  k++ )
      {
        er2 = KheAvoidSplitAssignmentsConstraintEventResource(c, i, k);
	KheLayerTreeMergeEventNodes(soln, KheEventResourceEvent(er1),
	  KheEventResourceEvent(er2), ra1, ra2);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTreeMergeSpreadEventsNodes(KHE_SOLN soln,                   */
/*    KHE_SPREAD_EVENTS_CONSTRAINT c,                                        */
/*    ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)                      */
/*                                                                           */
/*  Merge nodes shared by the events of c, where possible.                   */
/*  Parameters ra1 and ra2 are scratch arrays of resources.                  */
/*                                                                           */
/*****************************************************************************/

static void KheLayerTreeMergeSpreadEventsNodes(KHE_SOLN soln,
  KHE_SPREAD_EVENTS_CONSTRAINT c,
  ARRAY_KHE_RESOURCE *ra1, ARRAY_KHE_RESOURCE *ra2)
{
  KHE_EVENT_GROUP eg;  int i, j, k;  KHE_EVENT e1, e2;
  for( i = 0;  i < KheSpreadEventsConstraintEventGroupCount(c);  i++ )
  {
    eg = KheSpreadEventsConstraintEventGroup(c, i);
    for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
    {
      e1 = KheEventGroupEvent(eg, j);
      for( k = j + 1;  k < KheEventGroupEventCount(eg);  k++ )
      {
	e2 = KheEventGroupEvent(eg, k);
	KheLayerTreeMergeEventNodes(soln, e1, e2, ra1, ra2);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_NODE KheLayerTreeMake(KHE_SOLN soln)                                 */
/*                                                                           */
/*  Build a layer tree for soln, using a for scratch memory.                 */
/*                                                                           */
/*****************************************************************************/

KHE_NODE KheLayerTreeMake(KHE_SOLN soln)
{
  KHE_INSTANCE ins;  KHE_SPLIT_FOREST sf;  KHE_SPLIT_JOB sj, sj2;
  int i, j, max_duration;  KHE_MEET meet, junk;  /* KHE_LAYER layer; */
  ARRAY_KHE_SPLIT_JOB split_jobs;  KHE_NODE res;  KHE_CONSTRAINT c;
  ARRAY_KHE_RESOURCE ra1, ra2;  HA_ARENA a;  KHE_EVENT e;
  KHE_TIMER st = NULL;
  /* KHE_STATS_TIMER st = NULL; */
  /* struct timeval tv; */

  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */

  ins = KheSolnInstance(soln);
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheLayerTreeMake(soln to %s)\n",
      KheInstanceId(ins) != NULL ? KheInstanceId(ins) : "-");
    st = KheTimerMake("layer_tree", KHE_NO_TIME, a);
    /* st = KheStatsTimerMake(a); */
    /* KheTestTimeOfDayBegin(&tv); */
  }

  /* check precondition: there may be no pre-existing nodes */
  HnAssert(KheSolnNodeCount(soln) == 0,"KheLayerTreeMake: nodes already exist");

  /* ensure that all events are assigned to at most one distinct thing */
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    if( KheEventPreassignedTime(e) == NULL )
      KheSplitEventHomogenizeAssignments(soln, e);
  }

  /* ensure that all meets have durations within the cycle size */
  max_duration = KheInstanceTimeCount(ins);
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    if( KheMeetDuration(meet) > max_duration )
      KheMeetSplit(meet, max_duration, false, &meet, &junk);
      /* junk goes on the end and gets examined later in the loop */
  }

  /* make and sort the split jobs */
  if( DEBUG1 )
    fprintf(stderr, "  before creating jobs, run time %.2fs\n",
      KheTimerElapsedTime(st));
  HaArrayInit(split_jobs, a);
  KheSplitAddConstraintJobs(soln, &split_jobs, a);
  KheSplitAddPreExistingSplitJobs(soln, &split_jobs, a);
  KheSplitAddPreassignedSplitJobs(soln, &split_jobs, a);
  /* KheSplitAddLayerJobs(soln, &split_jobs); */
  KheSplitAddAssignedSplitJobs(soln, &split_jobs, a);
  if( DEBUG2 )
  {
    fprintf(stderr, "[ split jobs before sorting (%d jobs):\n",
      HaArrayCount(split_jobs));
    HaArrayForEach(split_jobs, sj, i)
      KheSplitJobDebug(sj, 2, stderr, a);
    fprintf(stderr, "]\n");
  }
  if( DEBUG1 )
    fprintf(stderr, "  before sorting jobs, run time %.2fs\n",
      KheTimerElapsedTime(st));
  HaArraySort(split_jobs, &KheSplitJobDecreasingPriorityCmp);
  if( DEBUG1 )
    fprintf(stderr, "  after sorting jobs, run time %.2fs\n",
      KheTimerElapsedTime(st));
  if( DEBUG2 )
  {
    fprintf(stderr, "[ split jobs after sorting (%d jobs):\n",
      HaArrayCount(split_jobs));
    HaArrayForEach(split_jobs, sj, i)
      KheSplitJobDebug(sj, 2, stderr, a);
    fprintf(stderr, "]\n");
  }

  /* make a forest and try the jobs on it */
  sf = KheSplitForestMake(soln, a);
  for( i = 0;  i < HaArrayCount(split_jobs);  i = j )
  {
    sj = HaArray(split_jobs, i);
    if( DEBUG1 )
    {
      fprintf(stderr, "  [ Job %d:\n", i);
      KheSplitJobDebug(sj, 4, stderr, a);
    }
    for( j = i + 1;  j < HaArrayCount(split_jobs);  j++ )
    {
      sj2 = HaArray(split_jobs, j);
      if( KheSplitJobTag(sj2) != KheSplitJobTag(sj) ||
	  KheSplitJobPriority(sj2) != KheSplitJobPriority(sj) )
	break;
    }
    KheSplitJobTry(&split_jobs, i, j, sf, a);
    if( DEBUG1 )
      fprintf(stderr, "  ] after Job %d, run time %.2fs\n", i,
	KheTimerElapsedTime(st));
  }
  /* ***
  if( DEBUG1 )
    KheSplitForestDebug(sf, 2, stderr);
  *** */

  /* choose partitions, split events into meets, and build tree */
  res = KheSplitForestFinalize(sf, a);
  if( DEBUG1 )
    fprintf(stderr, "  after finalizing, run time %.2fs\n",
      KheTimerElapsedTime(st));

  /* free the split jobs, the array containing them, and the forest */
  /* ***
  HaArrayForEach(split_jobs, sj, i)
    KheSplitJobFree(sj);
  MArrayFree(split_jobs);
  KheSplitForestFree(sf);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "  after freeing, run time %.2fs\n",
      KheTimerElapsedTime(st));

  /* remove redundant layers */
  /* ***
  for( i = 0;  i < KheSolnLayerCount(soln);  i++ )
  {
    layer = KheSolnLayer(soln, i);
    if( KheSplitLayerIsFullyAssigned(layer) )
    {
      KheLayerDelete(layer);
      i--;
    }
  }
  *** */
  /* ***
  for( i = 0;  i < KheSolnLayerCount(soln);  i++ )
  {
    layer = KheSolnLayer(soln, i);
    if( KheLayerIsRedundant(layer) )
    {
      KheLayerDelete(layer);
      i--;
    }
  }
  *** */

  /* ensure that every unassigned meet lies in at least one layer */
  /* ***
  for( i = 0;  i < KheMeetCount(soln);  i++ )
  {
    meet = KheMeet(soln, i);
    if( KheMeetAsst(meet) == NULL && KheMeetLayerCount(meet) == 0 )
    {
      layer = KheLayerMake(soln, NULL);
      if( !KheLayerAddMeet(layer, meet) )
	HnAbort("KheLayerTreeMake internal error");
    }
  }
  *** */

  /* merge nodes which share spread events or avoid split asst consts */
  HaArrayInit(ra1, a);
  HaArrayInit(ra2, a);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    switch( KheConstraintTag(c) )
    {
      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	KheLayerTreeMergeAvoidSplitNodes(soln,
	  (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, &ra1, &ra2);
	break;

      case KHE_SPREAD_EVENTS_CONSTRAINT_TAG:

	KheLayerTreeMergeSpreadEventsNodes(soln,
	  (KHE_SPREAD_EVENTS_CONSTRAINT) c, &ra1, &ra2);
	break;

      default:

	break;
    }
  }
  /* ***
  MArrayFree(ra1);
  MArrayFree(ra2);
  *** */

  /* check monitors as requested */
  /* ***
  if( check_prefer_times_monitors || check_split_events_monitors ||
      check_link_events_monitors )
    KheLayerTreeCheckMonitors(soln, check_prefer_times_monitors,
      check_split_events_monitors, check_link_events_monitors);
  *** */

  if( DEBUG1 )
  {
    /* ***
    fprintf(stderr, "  final layers:\n");
    for( i = 0;  i < KheSolnLayerCount(soln);  i++ )
    {
      layer = KheSolnLayer(soln, i);
      KheLayerDebug(layer, 2, 2, stderr);
    }
    *** */
    fprintf(stderr, "  final tree:\n");
    KheNodeDebug(res, 2, 2, stderr);
    fprintf(stderr, "] KheLayerTreeMake (%d nodes, run time %.2fs)\n",
      KheSolnNodeCount(soln), KheTimerElapsedTime(st));
  }
  KheSolnArenaEnd(soln, a);
  /* HaArenaDe lete(a); */
  return res;
}

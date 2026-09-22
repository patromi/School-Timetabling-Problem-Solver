
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
/*  FILE:         khe_ss_grouping.c                                          */
/*  DESCRIPTION:  Grouping helper functions                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"


/*****************************************************************************/
/*                                                                           */
/*  char *KheSubTagLabel(KHE_SUBTAG_STANDARD_TYPE sub_tag)                   */
/*                                                                           */
/*  Return the standard label for this sub-tag.                              */
/*                                                                           */
/*****************************************************************************/

char *KheSubTagLabel(KHE_SUBTAG_STANDARD_TYPE sub_tag)
{
  switch( sub_tag )
  {
    case KHE_SUBTAG_SPLIT_EVENTS:	return "SplitEventsGroupMonitor";
    case KHE_SUBTAG_DISTRIBUTE_SPLIT_EVENTS:
				    return "DistributeSplitEventsGroupMonitor";
    case KHE_SUBTAG_ASSIGN_TIME:	return "AssignTimeGroupMonitor";
    case KHE_SUBTAG_PREFER_TIMES:	return "PreferTimesGroupMonitor";
    case KHE_SUBTAG_SPREAD_EVENTS:	return "SpreadEventsGroupMonitor";
    case KHE_SUBTAG_LINK_EVENTS:	return "LinkEventsGroupMonitor";
    case KHE_SUBTAG_ORDER_EVENTS:	return "OrderEventsGroupMonitor";
    case KHE_SUBTAG_ASSIGN_RESOURCE:	return "AssignResourceGroupMonitor";
    case KHE_SUBTAG_PREFER_RESOURCES:	return "PreferResourcesGroupMonitor";
    case KHE_SUBTAG_AVOID_SPLIT_ASSIGNMENTS:
				    return "AvoidSplitAssignmentsGroupMonitor";
    case KHE_SUBTAG_AVOID_CLASHES:	return "AvoidClashesGroupMonitor";
    case KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES:
				    return "AvoidUnavailableTimesGroupMonitor";
    case KHE_SUBTAG_LIMIT_IDLE_TIMES:	return "LimitIdleTimesGroupMonitor";
    case KHE_SUBTAG_CLUSTER_BUSY_TIMES:	return "ClusterBusyTimesGroupMonitor";
    case KHE_SUBTAG_LIMIT_BUSY_TIMES:	return "LimitBusyTimesGroupMonitor";
    case KHE_SUBTAG_LIMIT_WORKLOAD:	return "LimitWorkloadGroupMonitor";
    case KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS:
				    return "LimitActiveIntervalsGroupMonitor";
    case KHE_SUBTAG_LIMIT_RESOURCES:	return "LimitResourcesGroupMonitor";
    case KHE_SUBTAG_ORDINARY_DEMAND:	return "OrdinaryDemandGroupMonitor";
    case KHE_SUBTAG_WORKLOAD_DEMAND:	return "WorkloadDemandGroupMonitor";
    case KHE_SUBTAG_KEMPE_DEMAND:	return "KempeDemandGroupMonitor";
    case KHE_SUBTAG_NODE_TIME_REPAIR:	return "NodeTimeRepairGroupMonitor";
    case KHE_SUBTAG_LAYER_TIME_REPAIR:	return "LayerTimeRepairGroupMonitor";
    /* case KHE_SUBTAG_TASKING:		return "TaskingGroupMonitor"; */
    case KHE_SUBTAG_RESOURCE_REPAIR:	return "ResourceRepairGroupMonitor";
    case KHE_SUBTAG_ALL_DEMAND:		return "AllDemandGroupMonitor";

  default:

    HnAbort("KheSubTagLabel: sub_tag out of range");
    return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SUBTAG_STANDARD_TYPE KheSubTagFromTag(KHE_MONITOR_TAG tag)           */
/*                                                                           */
/*  Return the subtag corresponding to tag.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_SUBTAG_STANDARD_TYPE KheSubTagFromTag(KHE_MONITOR_TAG tag)
{
  switch( tag )
  {
    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KHE_SUBTAG_SPLIT_EVENTS;

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KHE_SUBTAG_DISTRIBUTE_SPLIT_EVENTS;

    case KHE_ASSIGN_TIME_MONITOR_TAG:

      return KHE_SUBTAG_ASSIGN_TIME;

    case KHE_PREFER_TIMES_MONITOR_TAG:

      return KHE_SUBTAG_PREFER_TIMES;

    case KHE_SPREAD_EVENTS_MONITOR_TAG:

      return KHE_SUBTAG_SPREAD_EVENTS;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      return KHE_SUBTAG_LINK_EVENTS;

    case KHE_ORDER_EVENTS_MONITOR_TAG:

      return KHE_SUBTAG_ORDER_EVENTS;

    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      return KHE_SUBTAG_ASSIGN_RESOURCE;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      return KHE_SUBTAG_PREFER_RESOURCES;

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      return KHE_SUBTAG_AVOID_SPLIT_ASSIGNMENTS;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      return KHE_SUBTAG_AVOID_CLASHES;

    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

      return KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

      return KHE_SUBTAG_LIMIT_IDLE_TIMES;

    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

      return KHE_SUBTAG_CLUSTER_BUSY_TIMES;

    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

      return KHE_SUBTAG_LIMIT_BUSY_TIMES;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      return KHE_SUBTAG_LIMIT_WORKLOAD;

    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

      return KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      return KHE_SUBTAG_LIMIT_RESOURCES;

    case KHE_ORDINARY_DEMAND_MONITOR_TAG:

      return KHE_SUBTAG_ORDINARY_DEMAND;

    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

      return KHE_SUBTAG_ORDINARY_DEMAND;

    default:

      HnAbort("KheSubTagFromTag: unknown tag %d", tag);
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorHasParent(KHE_MONITOR m, int sub_tag,                     */
/*    KHE_GROUP_MONITOR *res_gm)                                             */
/*                                                                           */
/*  If m has a parent group monitor with the given sub-tag, then return      */
/*  true and set *gm to one such parent; otherwise return false.             */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorHasParent(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR *res_gm)
{
  int i;  KHE_GROUP_MONITOR prnt_gm;
  for( i = 0;  i < KheMonitorParentMonitorCount(m);  i++ )
  {
    prnt_gm = KheMonitorParentMonitor(m, i);
    if( KheGroupMonitorSubTag(prnt_gm) == sub_tag )
    {
      *res_gm = prnt_gm;
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddSelfOrParent(KHE_MONITOR m, int sub_tag,               */
/*    KHE_GROUP_MONITOR gm)                                                  */
/*                                                                           */
/*  Ensure that m is a decendant of gm, via a parent whose sub-tag           */
/*  is sub_tag if m has one, or else directly.                               */
/*                                                                           */
/*****************************************************************************/

void KheMonitorAddSelfOrParent(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR gm)
{
  KHE_GROUP_MONITOR sub_tag_gm;
  if( KheMonitorHasParent(m, sub_tag, &sub_tag_gm) )
  {
    /* m has a sub_tag group monitor parent, so ensure it is linked */
    if( !KheMonitorDescendant((KHE_MONITOR) sub_tag_gm, (KHE_MONITOR) gm) )
      KheGroupMonitorAddChildMonitor(gm, (KHE_MONITOR) sub_tag_gm);
  }
  else
  {
    /* m has no sub_tag group monitor proper ancestor, so ensure m is linked */
    if( !KheMonitorDescendant(m, (KHE_MONITOR) gm) )
      KheGroupMonitorAddChildMonitor(gm, m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorDeleteAllParentsRecursive(KHE_MONITOR m)                  */
/*                                                                           */
/*  Remove m from all its parents.  Any parents left childless by this       */
/*  are removed too, possibly recursively, except not the solution object.   */
/*                                                                           */
/*****************************************************************************/

void KheMonitorDeleteAllParentsRecursive(KHE_MONITOR m)
{
  KHE_GROUP_MONITOR gm;
  while( KheMonitorParentMonitorCount(m) > 0 )
  {
    gm = KheMonitorParentMonitor(m, 0);
    KheGroupMonitorDeleteChildMonitor(gm, m);
    if( KheGroupMonitorChildMonitorCount(gm) == 0 &&
	gm != (KHE_GROUP_MONITOR) KheMonitorSoln(m) )
    {
      KheMonitorDeleteAllParentsRecursive((KHE_MONITOR) gm);
      KheGroupMonitorDelete(gm);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorHasProperAncestor(KHE_MONITOR m, int sub_tag,             */
/*    KHE_GROUP_MONITOR *gm)                                                 */
/*                                                                           */
/*  If m has a proper ancestor group monitor with the given sub-tag, then    */
/*  return true and set *gm to one such ancestor; otherwise return false.    */
/*                                                                           */
/*****************************************************************************/

/* *** using KheMonitorHasParent now
bool KheMonitorHasProperAncestor(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR *res_gm)
{
  int i;  KHE_GROUP_MONITOR prnt_gm;
  for( i = 0;  i < KheMonitorParentMonitorCount(m);  i++ )
  {
    prnt_gm = KheMonitorParentMonitor(m, i);
    if( KheGroupMonitorSubTag(prnt_gm) == sub_tag )
    {
      *res_gm = prnt_gm;
      return true;
    }
    if( KheMonitorHasProperAncestor((KHE_MONITOR) prnt_gm, sub_tag, res_gm) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorAddSelfOrAncestor(KHE_MONITOR m, int sub_tag,             */
/*    KHE_GROUP_MONITOR gm)                                                  */
/*                                                                           */
/*  Ensure that m is a decendant of gm, via a proper ancestor whose sub-tag  */
/*  is sub_tag if m has one, or else directly.                               */
/*                                                                           */
/*****************************************************************************/

/* *** using KheMonitorAddSelfOrParent instead now
void KheMonitorAddSelfOrAncestor(KHE_MONITOR m, int sub_tag,
  KHE_GROUP_MONITOR gm)
{
  KHE_GROUP_MONITOR sub_tag_gm;
  if( KheMonitorHasProperAncestor(m, sub_tag, &sub_tag_gm) )
  {
    ** m has a sub_tag group monitor proper ancestor, so ensure it is linked **
    if( !KheMonitorDescendant((KHE_MONITOR) sub_tag_gm, (KHE_MONITOR) gm) )
      KheGroupMonitorAddChildMonitor(gm, (KHE_MONITOR) sub_tag_gm);
  }
  else
  {
    ** m has no sub_tag group monitor proper ancestor, so ensure m is linked **
    if( !KheMonitorDescendant(m, (KHE_MONITOR) gm) )
      KheGroupMonitorAddChildMonitor(gm, m);
  }
}
*** */

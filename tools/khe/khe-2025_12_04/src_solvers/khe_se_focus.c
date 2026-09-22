
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
/*  FILE:         khe_se_secondary.c                                         */
/*  DESCRIPTION:  Secondary groupings for ejection chains                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG9 0


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAddResourceMonitors(KHE_GROUP_MONITOR gm,            */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Add r's resource monitors for to gm, or their parents if grouped.        */
/*                                                                           */
/*  Implementation note.  Function KheMonitorAddSelfOrParent ensures that    */
/*  no monitor is added to gm twice.  It does so by checking whether the     */
/*  candidate monitor has gm as an ancestor, which is much faster than       */
/*  checking whether gm has the candidate monitor as a child, since gm       */
/*  may have many children.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorAddResourceMonitors(KHE_GROUP_MONITOR gm,
  KHE_RESOURCE r)
{
  int j;  KHE_MONITOR m;  KHE_SOLN soln;
  soln = KheMonitorSoln((KHE_MONITOR) gm);
  for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
  {
    m = KheSolnResourceMonitor(soln, r, j);
    switch( KheMonitorTag(m) )
    {
      case KHE_AVOID_CLASHES_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_AVOID_CLASHES, gm);
	break;

      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES, gm);
	break;

      case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_IDLE_TIMES, gm);
	break;

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_CLUSTER_BUSY_TIMES, gm);
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_BUSY_TIMES, gm);
	break;

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_WORKLOAD, gm);
	break;

      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS, gm);
	break;

      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:

	/* not sure about this; not in the Guide */
	KheMonitorAddSelfOrParent(m, KHE_SUBTAG_WORKLOAD_DEMAND, gm);
	break;

      default:

	/* not interested */
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupMonitorAddAllResourceMonitors(KHE_GROUP_MONITOR gm)         */
/*                                                                           */
/*  Add all resource monitors to gm.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheGroupMonitorAddAllResourceMonitors(KHE_GROUP_MONITOR gm)
{
  KHE_SOLN soln;  KHE_INSTANCE ins;  KHE_RESOURCE r;  int i;
  soln = KheMonitorSoln((KHE_MONITOR) gm);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheGroupMonitorAddResourceMonitors(gm, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMeetDoGroupMonitorMake(KHE_MEET meet, KHE_GROUP_MONITOR gm)      */
/*                                                                           */
/*  Add the monitors needed for time repair that monitor meet and its        */
/*  tasks, either directly or via primary group monitors.                    */
/*                                                                           */
/*****************************************************************************/

static void KheMeetDoGroupMonitorMake(KHE_MEET meet, KHE_GROUP_MONITOR gm)
{
  KHE_SOLN soln;  KHE_EVENT e;  int i, j;  KHE_TASK task;  KHE_MONITOR m;
  KHE_MEET child_meet;

  /* assign time, prefer times, spread events, and order events monitors */
  soln = KheMeetSoln(meet);
  e = KheMeetEvent(meet);
  if( e != NULL )
    for( i = 0;  i < KheSolnEventMonitorCount(soln, e);  i++ )
    {
      m = KheSolnEventMonitor(soln, e, i);
      switch( KheMonitorTag(m) )
      {
	case KHE_ASSIGN_TIME_MONITOR_TAG:

	  KheMonitorAddSelfOrParent(m, KHE_SUBTAG_ASSIGN_TIME, gm);
	  break;

	case KHE_PREFER_TIMES_MONITOR_TAG:

	  KheMonitorAddSelfOrParent(m, KHE_SUBTAG_PREFER_TIMES, gm);
	  break;

	case KHE_SPREAD_EVENTS_MONITOR_TAG:

	  KheMonitorAddSelfOrParent(m, KHE_SUBTAG_SPREAD_EVENTS, gm);
	  break;

	case KHE_ORDER_EVENTS_MONITOR_TAG:

	  KheMonitorAddSelfOrParent(m, KHE_SUBTAG_ORDER_EVENTS, gm);
	  break;

	default:

	  /* not interested */
	  break;
      }
    }

  /* ordinary demand monitors */
  for( i = 0;  i < KheMeetTaskCount(meet);  i++ )
  {
    task = KheMeetTask(meet, i);
    for( j = 0;  j < KheTaskDemandMonitorCount(task);  j++ )
    {
      m = (KHE_MONITOR) KheTaskDemandMonitor(task, j);
      KheMonitorAddSelfOrParent(m, KHE_SUBTAG_ORDINARY_DEMAND, gm);
    }
  }

  /* do the same for all meets fixed-assigned to meet that are not in a node */
  for( i = 0;  i < KheMeetAssignedToCount(meet);  i++ )
  {
    child_meet = KheMeetAssignedTo(meet, i);
    if( KheMeetNode(child_meet) == NULL && KheMeetAssignIsFixed(child_meet) )
      KheMeetDoGroupMonitorMake(child_meet, gm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheNodeDoGroupMonitorMake(KHE_NODE node, KHE_GROUP_MONITOR gm)      */
/*                                                                           */
/*  Add the event monitors (or primary groupings of them) of the events      */
/*  of node and its descendants to gm.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheNodeDoGroupMonitorMake(KHE_NODE node, KHE_GROUP_MONITOR gm)
{
  int i;  KHE_NODE child_node;  KHE_MEET meet;  /* KHE_SOLN soln; */

  /* do it for the meets of node */
  /* soln = KheNodeSoln(node); */
  for( i = 0;  i < KheNodeMeetCount(node);  i++ )
  {
    meet = KheNodeMeet(node, i);
    KheMeetDoGroupMonitorMake(meet, gm);
  }

  /* do it for the descendants of node */
  for( i = 0;  i < KheNodeChildCount(node);  i++ )
  {
    child_node = KheNodeChild(node, i);
    KheNodeDoGroupMonitorMake(child_node, gm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheNodeTimeRepairStartGroupMonitorMake(KHE_NODE node)  */
/*                                                                           */
/*  Make a node group monitor with the appropriate children.                 */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheNodeTimeRepairStartGroupMonitorMake(KHE_NODE node)
{
  KHE_GROUP_MONITOR res;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheNodeTimeRepairStartGroupMonitorMake(");
    KheNodeDebug(node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  res = KheGroupMonitorMake(KheNodeSoln(node), KHE_SUBTAG_NODE_TIME_REPAIR,
    KheSubTagLabel(KHE_SUBTAG_NODE_TIME_REPAIR));
  KheNodeDoGroupMonitorMake(node, res);
  KheGroupMonitorAddAllResourceMonitors(res);
  if( DEBUG2 )
  {
    KheMonitorDebug((KHE_MONITOR) res, 2, 2, stderr);
    fprintf(stderr, "] KheNodeTimeRepairStartGroupMonitorMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLayerTimeRepairAddLayer(KHE_GROUP_MONITOR gm, KHE_LAYER layer)   */
/*                                                                           */
/*  Add monitors for layer to gm.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheLayerTimeRepairAddLayer(KHE_GROUP_MONITOR gm, KHE_LAYER layer)
{
  int i;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    KheNodeDoGroupMonitorMake(KheLayerChildNode(layer, i), gm);
  for( i = 0;  i < KheLayerResourceCount(layer);  i++ )
    KheGroupMonitorAddResourceMonitors(gm, KheLayerResource(layer, i));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheLayerTimeRepairStartGroupMonitorMake(               */
/*    KHE_LAYER layer)                                                       */
/*                                                                           */
/*  Make a layer start group monitor with the appropriate children.          */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheLayerTimeRepairStartGroupMonitorMake(KHE_LAYER layer)
{
  KHE_GROUP_MONITOR res;
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheLayerTimeRepairStartGroupMonitorMake(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  res = KheGroupMonitorMake(KheLayerSoln(layer), KHE_SUBTAG_LAYER_TIME_REPAIR,
    KheSubTagLabel(KHE_SUBTAG_LAYER_TIME_REPAIR));
  KheLayerTimeRepairAddLayer(res, layer);
  if( DEBUG3 )
  {
    KheMonitorDebug((KHE_MONITOR) res, 2, 2, stderr);
    fprintf(stderr, "] KheLayerTimeRepairStartGroupMonitorMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheLayerTimeRepairLongStartGroupMonitorMake(           */
/*    KHE_LAYER layer)                                                       */
/*                                                                           */
/*  Make a layer continue group monitor with the appropriate children.       */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheLayerTimeRepairLongStartGroupMonitorMake(KHE_LAYER layer)
{
  KHE_GROUP_MONITOR res;  KHE_NODE parent_node;  int layer_index, i;
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheLayerTimeRepairLongStartGroupMonitorMake(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  res = KheGroupMonitorMake(KheLayerSoln(layer), KHE_SUBTAG_LAYER_TIME_REPAIR,
    KheSubTagLabel(KHE_SUBTAG_LAYER_TIME_REPAIR));
  parent_node = KheLayerParentNode(layer);
  layer_index = KheLayerParentNodeIndex(layer);
  for( i = 0;  i <= layer_index;  i++ )
  {
    layer = KheNodeChildLayer(parent_node, i);
    KheLayerTimeRepairAddLayer(res, layer);
  }
  if( DEBUG3 )
  {
    KheMonitorDebug((KHE_MONITOR) res, 2, 2, stderr);
    fprintf(stderr, "] KheLayerTimeRepairLongStartGroupMonitorMake returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheTaskingStartGroupMonitorMake(KHE_TASKING tasking)   */
/*                                                                           */
/*  Make a tasking group monitor with the appropriate children.              */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheStartGroupMonitorMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  KHE_RESOURCE r;  KHE_INSTANCE ins;  KHE_GROUP_MONITOR gm;
  KHE_TASK task;  KHE_EVENT_RESOURCE er;  int i, j;  KHE_MONITOR m;
  if( DEBUG4 )
    fprintf(stderr, "[ KheTaskingStartGroupMonitorMake(tasking)\n");

  /* event resource monitors */
  /* ***
  gm = KheGroupMonitorMake(soln, KHE_SUBTAG_TASKING,
    KheSubTagLabel(KHE_SUBTAG_TASKING));
  *** */
  gm = KheGroupMonitorMake(soln, KHE_SUBTAG_RESOURCE_REPAIR,
    KheSubTagLabel(KHE_SUBTAG_RESOURCE_REPAIR));
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    er = KheTaskEventResource(task);
    if( er != NULL && (rt == NULL || rt == KheEventResourceResourceType(er)) )
      for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
      {
	m = KheSolnEventResourceMonitor(soln, er, j);
	switch( KheMonitorTag(m) )
	{
	  case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_ASSIGN_RESOURCE, gm);
	    break;

	  case KHE_PREFER_RESOURCES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_PREFER_RESOURCES, gm);
	    break;

	  case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

	    if( !KheMonitorDescendant(m, (KHE_MONITOR) gm) )
	      KheGroupMonitorAddChildMonitor(gm, m);
	    break;

	  case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	    if( !KheMonitorDescendant(m, (KHE_MONITOR) gm) )
	      KheGroupMonitorAddChildMonitor(gm, m);
	    break;

	  default:

	    /* not interested */
	    break;
	}
      }
  }

  /* resource monitors */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    if( rt == NULL || KheResourceResourceType(r) == rt )
      for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
      {
	m = KheSolnResourceMonitor(soln, r, j);
	switch( KheMonitorTag(m) )
	{
	  case KHE_AVOID_CLASHES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_AVOID_CLASHES, gm);
	    break;

	  case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES,gm);
	    break;

	  case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_IDLE_TIMES, gm);
	    break;

	  case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_CLUSTER_BUSY_TIMES, gm);
	    break;

	  case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_BUSY_TIMES, gm);
	    break;

	  case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_WORKLOAD, gm);
	    break;

	  case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	    KheMonitorAddSelfOrParent(m, KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS, gm);
	    break;

	  default:

	    /* not interested */
	    break;
	}
      }
  }
  if( DEBUG4 )
  {
    KheMonitorDebug((KHE_MONITOR) gm, 2, 2, stderr);
    fprintf(stderr, "] KheTaskingStartGroupMonitorMake returning\n");
  }
  return gm;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheGroupEventMonitors(KHE_SOLN soln,                   */
/*    KHE_MONITOR_TAG tag, KHE_SUBTAG_STANDARD_TYPE sub_tag)                 */
/*                                                                           */
/*  Group event monitors with the given tag.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheGroupEventMonitors(KHE_SOLN soln,
  KHE_MONITOR_TAG tag, KHE_SUBTAG_STANDARD_TYPE sub_tag)
{
  KHE_INSTANCE ins;  KHE_EVENT e;  int i, j;  KHE_MONITOR m;
  KHE_GROUP_MONITOR res;
  if( DEBUG5 )
    fprintf(stderr, "[ KheGroupEventMonitors(soln, %s)\n",
      KheMonitorTagShow(tag));
  res = KheGroupMonitorMake(soln, sub_tag, KheSubTagLabel(sub_tag));
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventCount(ins);  i++ )
  {
    e = KheInstanceEvent(ins, i);
    for( j = 0;  j < KheSolnEventMonitorCount(soln, e);  j++ )
    {
      m = KheSolnEventMonitor(soln, e, j);
      if( KheMonitorTag(m) == tag )
	KheMonitorAddSelfOrParent(m, sub_tag, res);
    }
  }
  if( DEBUG5 )
  {
    KheMonitorDebug((KHE_MONITOR) res, 2, 2, stderr);
    fprintf(stderr, "] KheGroupEventMonitors returning\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP_MONITOR KheAllDemandGroupMonitorMake(KHE_SOLN soln)            */
/*                                                                           */
/*  Make a group monitor with all demand monitors for its children.          */
/*                                                                           */
/*****************************************************************************/

KHE_GROUP_MONITOR KheAllDemandGroupMonitorMake(KHE_SOLN soln)
{
  KHE_RESOURCE r;  KHE_INSTANCE ins;  KHE_GROUP_MONITOR gm;
  KHE_MONITOR m;  int i, j, k;  KHE_MEET meet;  KHE_TASK task;

  /* ordinary demand nodes */
  gm = KheGroupMonitorMake(soln, KHE_SUBTAG_ALL_DEMAND,
    KheSubTagLabel(KHE_SUBTAG_ALL_DEMAND));
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
    {
      task = KheMeetTask(meet, j);
      for( k = 0;  k < KheTaskDemandMonitorCount(task);  k++ )
      {
	m = (KHE_MONITOR) KheTaskDemandMonitor(task, k);
	KheGroupMonitorAddChildMonitor(gm, m);
      }
    }
  }

  /* workload demand nodes */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
	KheGroupMonitorAddChildMonitor(gm, m);
    }
  }
  return gm;
}

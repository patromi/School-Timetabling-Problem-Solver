
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
/*  FILE:         khe_st_basic.c                                             */
/*  DESCRIPTION:  Basic time solvers                                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "simple assign times"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAllowAsst(KHE_MEET meet, KHE_MEET target_meet)                   */
/*                                                                           */
/*  Return true if meet is allowed to be assigned to target_meet, because    */
/*  none of meet's fellows are assigned to it already.                       */
/*                                                                           */
/*****************************************************************************/

static bool KheAllowAsst(KHE_MEET meet, KHE_MEET target_meet)
{
  KHE_EVENT e;  KHE_SOLN soln;  int i;
  e = KheMeetEvent(meet);
  if( e == NULL )
    return true;
  soln = KheMeetSoln(meet);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    if( KheMeetAsst(KheEventMeet(soln, e, i)) == target_meet )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMakeBestAsst(KHE_MEET meet, KHE_NODE parent_node)                */
/*                                                                           */
/*  If meet can be assigned to any of the meets of parent_node at any        */
/*  offset, make the best of these assignments and return true.  Otherwise   */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheMakeBestAsst(KHE_MEET meet, KHE_NODE parent_node)
{
  KHE_COST cost, best_cost;  int i, offset, offset_max, best_offset;
  KHE_SOLN soln;  KHE_MEET parent_meet, best_parent_meet;
  if( DEBUG1 )
  {
    fprintf(stderr, "  [ KheMakeBestAsst(");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, " (durn %d), Node %d)\n",
      KheMeetDuration(meet), KheNodeSolnIndex(parent_node));
  }
  HnAssert(KheMeetAsst(meet) == NULL,
    "KheSimpleAssignTimes internal error 1");
  soln = KheNodeSoln(parent_node);
  best_parent_meet = NULL;
  best_cost = 0;
  best_offset = 0;
  for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
  {
    parent_meet = KheNodeMeet(parent_node, i);
    if( DEBUG1 )
    {
      fprintf(stderr, "    trying parent_meet ");
      KheMeetDebug(parent_meet, 1, -1, stderr);
      fprintf(stderr, " (durn %d)\n", KheMeetDuration(parent_meet));
    }
    if( KheAllowAsst(meet, parent_meet) )
    {
      offset_max = KheMeetDuration(parent_meet) - KheMeetDuration(meet);
      for( offset = 0;  offset <= offset_max;  offset++ )
      {
	if( DEBUG1 )
	  fprintf(stderr, "      trying offset %d\n", offset);
	if( KheMeetAssign(meet, parent_meet, offset) )
	{
	  cost = KheSolnCost(soln);
	  if( DEBUG1 )
	  {
	    fprintf(stderr, "        assigned ");
	    KheMeetDebug(meet, 1, -1, stderr);
	    fprintf(stderr, " to ");
	    KheMeetDebug(parent_meet, 1, -1, stderr);
	    fprintf(stderr, "+%d cost %.5f%s\n", offset, KheCostShow(cost),
	      best_parent_meet == NULL || cost < best_cost ? " (best)" : "");
	    /* ***
	    KheMonitorDebug((KHE_MONITOR) KheSolnMatchingMonitor(soln),
	      2, 4, stderr);
	    *** */
	  }
	  if( best_parent_meet == NULL || cost < best_cost )
	  {
	    best_parent_meet = parent_meet;
	    best_offset = offset;
	    best_cost = cost;
	  }
	  KheMeetUnAssign(meet);  /* OK: experimental asst now being removed */
	}
      }
    }
  }
  if( best_parent_meet != NULL )
  {
    if( !KheMeetAssign(meet, best_parent_meet, best_offset) )
      HnAbort("KheSimpleAssignTimes internal error 2");
    if( DEBUG1 )
    {
      fprintf(stderr, "    finally assigned ");
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, " to ");
      KheMeetDebug(best_parent_meet, 1, -1, stderr);
      fprintf(stderr, "+%d\n", best_offset);
      fprintf(stderr, "  ] KheMakeBestAsst returning true\n");
    }
    return true;
  }
  else
  {
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeSimpleAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options) */
/*                                                                           */
/*  Assign times to the meets of the child nodes of parent_node, using a     */
/*  simple algorithm.                                                        */
/*                                                                           */
/*****************************************************************************/

bool KheNodeSimpleAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  int i, j, durn, max_durn;  KHE_NODE child_node;
  KHE_MEET meet;  bool res;
  if( DEBUG1 )
    fprintf(stderr, "[ KheNodeSimpleAssignTimes(Node %d)\n",
      KheNodeSolnIndex(parent_node));

  /* ensure everything is unassigned */
  KheNodeUnAssignTimes(parent_node, options);

  /* find the maximum duration */
  max_durn = 0;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetDuration(meet) > max_durn )
	max_durn = KheMeetDuration(meet);
    }
  }

  /* assign each meet in decreasing duration order */
  res = true;
  for( durn = max_durn;  durn > 0;  durn-- )
    for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
    {
      child_node = KheNodeChild(parent_node, i);
      for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
      {
	meet = KheNodeMeet(child_node, j);
	if( KheMeetDuration(meet) == durn )
	  res &= KheMakeBestAsst(meet, parent_node);
      }
    }
  if( DEBUG1 )
    fprintf(stderr, "] KheNodeSimpleAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerSimpleAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)     */
/*                                                                           */
/*  Assign times to the meets of the nodes of layer, using a simple          */
/*  algorithm.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KheLayerSimpleAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  int i, j, durn, max_durn;  KHE_NODE parent_node, child_node;
  KHE_MEET meet;  bool res;
  if( DEBUG1 )
    fprintf(stderr, "[ KheLayerSimpleAssignTimes(layer)\n");

  /* ensure everything is unassigned */
  parent_node = KheLayerParentNode(layer);
  KheLayerUnAssignTimes(layer, options);

  /* find the maximum duration */
  max_durn = 0;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetDuration(meet) > max_durn )
	max_durn = KheMeetDuration(meet);
    }
  }

  /* assign each meet in decreasing duration order */
  res = true;
  for( durn = max_durn;  durn > 0;  durn-- )
    for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    {
      child_node = KheLayerChildNode(layer, i);
      for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
      {
	meet = KheNodeMeet(child_node, j);
	if( KheMeetDuration(meet) == durn )
	  res &= KheMakeBestAsst(meet, parent_node);
      }
    }
  if( DEBUG1 )
    fprintf(stderr, "] KheLayerSimpleAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "miscellaneous"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeRecursiveAssignTimes(KHE_NODE parent_node,                   */
/*    KHE_NODE_TIME_SOLVER solver, KHE_OPTIONS options)                      */
/*                                                                           */
/*  Apply solver to the descendants of parent_node in postorder.             */
/*                                                                           */
/*****************************************************************************/

bool KheNodeRecursiveAssignTimes(KHE_NODE parent_node,
  KHE_NODE_TIME_SOLVER solver, KHE_OPTIONS options)
{
  int i;  bool res;  KHE_NODE child_node;
  res = true;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    res &= KheNodeRecursiveAssignTimes(child_node, solver, options);
  }
  res &= solver(parent_node, options);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeUnAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)     */
/*                                                                           */
/*  Unassign all the meets in all the child nodes of parent_node.            */
/*                                                                           */
/*****************************************************************************/

bool KheNodeUnAssignTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) != NULL )
	if( !KheMeetUnAssign(meet) )  /* in KheNodeUnAssignTimes, which is OK */
	  return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerUnAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)         */
/*                                                                           */
/*  Unassign all the meets in all the nodes of layer.                        */
/*                                                                           */
/*****************************************************************************/

bool KheLayerUnAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) != NULL )
	if( !KheMeetUnAssign(meet) )  /* in KheLayerUnAssignTimes, unused */
	  return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeAllChildMeetsAssigned(KHE_NODE parent_node)                  */
/*                                                                           */
/*  Return true if all meets in all child nodes of parent_node are assigned. */
/*                                                                           */
/*****************************************************************************/

bool KheNodeAllChildMeetsAssigned(KHE_NODE parent_node)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;
  for( i = 0;  i < KheNodeChildCount(parent_node);  i++ )
  {
    child_node = KheNodeChild(parent_node, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) == NULL )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerAllChildMeetsAssigned(KHE_LAYER layer)                      */
/*                                                                           */
/*  Return true if all meets in all child nodes of layer are assigned.       */
/*                                                                           */
/*****************************************************************************/

bool KheLayerAllChildMeetsAssigned(KHE_LAYER layer)
{
  int i, j;  KHE_NODE child_node;  KHE_MEET meet;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) == NULL )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preassigned meet assignment"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheNodePreassignedAssignTimes(KHE_NODE root_node,                   */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assuming that root_node is the overall root node, search its child       */
/*  nodes for unassigned preassigned meets, and assign them.                 */
/*                                                                           */
/*****************************************************************************/

bool KheNodePreassignedAssignTimes(KHE_NODE root_node,
  KHE_OPTIONS options)
{
  bool res;  int j, k;  KHE_NODE child_node;  KHE_MEET meet;  KHE_TIME t;
  if( DEBUG2 )
    fprintf(stderr, "[ KheNodePreassignedAssignTimes(Node %d)\n",
      KheNodeSolnIndex(root_node));
  res = true;
  for( j = 0;  j < KheNodeChildCount(root_node);  j++ )
  {
    child_node = KheNodeChild(root_node, j);
    for( k = 0;  k < KheNodeMeetCount(child_node);  k++ )
    {
      meet = KheNodeMeet(child_node, k);
      if( KheMeetAsst(meet) == NULL &&
	  KheTimeGroupTimeCount(KheMeetDomain(meet)) == 1 )
	  /* KheMeetIsAssignedPreassigned(meet, false, &t) ) */
      {
	t = KheTimeGroupTime(KheMeetDomain(meet), 0);
	if( DEBUG2 )
	{
	  fprintf(stderr, "  calling KheMeetAssignTime(");
	  KheMeetDebug(meet, 1, -1, stderr);
	  fprintf(stderr, ", %s)\n", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	}
	if( KheMeetAssignIsFixed(meet) )
	  KheMeetAssignUnFix(meet);
	if( !KheMeetAssignTime(meet, t) )
	  HnAbort("KheNodePreassignedAssignTimes failed to assign meet");
	res = false;  /* will never happen */
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheNodePreassignedAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLayerPreassignedAssignTimes(KHE_LAYER layer,                     */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  As for KheNodePreassignedAssignTimes, except search only the child       */
/*  nodes of the i'th segment of root_node.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheLayerPreassignedAssignTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  bool res;  int i, j;  KHE_NODE child_node;  KHE_MEET meet;  KHE_TIME t;
  if( DEBUG2 )
    fprintf(stderr, "[ KheLayerPreassignedAssignTimes(layer)\n");
  res = true;
  for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
  {
    child_node = KheLayerChildNode(layer, i);
    for( j = 0;  j < KheNodeMeetCount(child_node);  j++ )
    {
      meet = KheNodeMeet(child_node, j);
      if( KheMeetAsst(meet) == NULL &&
	  KheTimeGroupTimeCount(KheMeetDomain(meet)) == 1 )
	  /* KheMeetIsAssignedPreassigned(meet, false, &t) ) */
      {
	t = KheTimeGroupTime(KheMeetDomain(meet), 0);
	if( DEBUG2 )
	{
	  fprintf(stderr, "  calling KheMeetAssignTime(");
	  KheMeetDebug(meet, 1, -1, stderr);
	  fprintf(stderr, ", %s)\n", KheTimeId(t) == NULL ? "-" : KheTimeId(t));
	}
	if( !KheMeetAssignTime(meet, t) )
	  res = false;
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheLayerPreassignedAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnTryMeetUnAssignments(KHE_SOLN soln)                          */
/*                                                                           */
/*  Try unassigning each meet of soln, to see if that improves the cost.     */
/*  Return true if any unassignments were kept.                              */
/*                                                                           */
/*****************************************************************************/

bool KheSolnTryMeetUnAssignments(KHE_SOLN soln)
{
  KHE_MEET meet, target_meet;  int i, offset;  KHE_COST cost;  bool res;
  /* KHE_TIME t;  KHE_EVENT e; */
  if( DEBUG5 )
    fprintf(stderr, "[ KheSolnTryMeetUnAssignments(soln)\n");
  res = false;
  for( i = 0;  i < KheSolnMeetCount(soln);  i++ )
  {
    meet = KheSolnMeet(soln, i);
    /* e = KheMeetEvent(meet); */
    target_meet = KheMeetAsst(meet);
    offset = KheMeetAsstOffset(meet);
    cost = KheSolnCost(soln);
    if( target_meet != NULL && !KheMeetIsAssignedPreassigned(meet, NULL) &&
	KheMeetUnAssign(meet) )   /* meet not preassigned */
    {
      if( KheSolnCost(soln) < cost )
      {
	res = true;
	if( DEBUG5 )
	{
	  fprintf(stderr, "  %.5f -> %.5f for unassigning meet ",
	    KheCostShow(cost), KheCostShow(KheSolnCost(soln)));
	  KheMeetDebug(meet, 1, 0, stderr);
	}
      }
      else
	KheMeetAssign(meet, target_meet, offset);
    }
  }
  if( DEBUG5 )
    fprintf(stderr, "] KheSolnTryMeetUnAssignments returning %s\n",
      res ? "true" : "false");
  return res;
}

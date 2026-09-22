
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
/*  FILE:         khe_sr_first_resource.c                                    */
/*  DESCRIPTION:  Most-constrained-first resource assignment solver          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_priqueue.h"
#include <limits.h>

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_NODE - a node representing one task group                 */
/*  KHE_RESOURCE_NODE - a node representing one resource.                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_group_node_rec *KHE_TASK_GROUP_NODE;
typedef HA_ARRAY(KHE_TASK_GROUP_NODE) ARRAY_KHE_TASK_GROUP_NODE;

typedef struct khe_resource_node_rec *KHE_RESOURCE_NODE;
typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;

struct khe_task_group_node_rec {
  int				priqueue_index;		/* for priqueue      */
  KHE_TASK_GROUP		task_group;		/* tasks to do       */
  int				unassigned_tasks;	/* no unassigned     */
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;		/* avail resources   */
};

struct khe_resource_node_rec {
  KHE_RESOURCE			resource;		/* the resource      */
  ARRAY_KHE_TASK_GROUP_NODE	task_group_nodes;	/* suitable tasks    */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task group nodes" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_NODE KheTaskGroupNodeMake(KHE_TASK_GROUP task_group,      */
/*    int unassigned_tasks)                                                  */
/*                                                                           */
/*  Create a new task set node with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_GROUP_NODE KheTaskGroupNodeMake(KHE_TASK_GROUP task_group,
  int unassigned_tasks, HA_ARENA a)
{
  KHE_TASK_GROUP_NODE res;
  HaMake(res, a);
  res->priqueue_index = 0;  /* actually undefined */
  res->task_group = task_group;
  res->unassigned_tasks = unassigned_tasks;
  HaArrayInit(res->resource_nodes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeDelete(KHE_TASK_GROUP_NODE tgn)                     */
/*                                                                           */
/*  Delete tgn.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskGroupNodeDelete(KHE_TASK_GROUP_NODE tgn)
{
  MArrayFree(tgn->resource_nodes);
  MFree(tgn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int64_t KheTaskGroupNodeKey(void *tgn)                                   */
/*                                                                           */
/*  PriQueue callback function which returns the priority of tgn.            */
/*                                                                           */
/*****************************************************************************/

static int64_t KheTaskGroupNodeKey(void *tgn)
{
  return HaArrayCount(((KHE_TASK_GROUP_NODE) tgn)->resource_nodes) -
    ((KHE_TASK_GROUP_NODE) tgn)->unassigned_tasks;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupNodeIndex(void *tgn)                                     */
/*                                                                           */
/*  PriQueue callback function which returns the index of tgn.               */
/*                                                                           */
/*****************************************************************************/

static int KheTaskGroupNodeIndex(void *tgn)
{
  return ((KHE_TASK_GROUP_NODE) tgn)->priqueue_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeSetIndex(void *tgn, int index)                      */
/*                                                                           */
/*  PriQueue callback function which sets the index of tgn.                  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupNodeSetIndex(void *tgn, int index)
{
  ((KHE_TASK_GROUP_NODE) tgn)->priqueue_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeDebug(KHE_TASK_GROUP_NODE tgn, int verbosity,       */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of tgn onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupNodeDebug(KHE_TASK_GROUP_NODE tgn, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ TaskGroupNode %ld (%d resources, %d unassigned tasks)",
      KheTaskGroupNodeKey(tgn), HaArrayCount(tgn->resource_nodes),
      tgn->unassigned_tasks);
    if( indent >= 0 )
    {
      fprintf(fp, ":\n");
      KheTaskGroupDebug(tgn->task_group, 2, 4, stderr);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, " ]");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource nodes" (private)                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r)                    */
/*                                                                           */
/*  Make a resource node for r.                                              */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r, HA_ARENA a)
{
  KHE_RESOURCE_NODE res;
  HaMake(res, a);
  res->resource = r;
  HaArrayInit(res->task_group_nodes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDelete(KHE_RESOURCE_NODE rn)                         */
/*                                                                           */
/*  Delete rn.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceNodeDelete(KHE_RESOURCE_NODE rn)
{
  MArrayFree(rn->task_group_nodes);
  MFree(rn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAddEdge(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn)           */
/*                                                                           */
/*  Add an edge from tgn to rn.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheAddEdge(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn)
{
  HaArrayAddLast(tgn->resource_nodes, rn);
  HaArrayAddLast(rn->task_group_nodes, tgn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDeleteEdge(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn)        */
/*                                                                           */
/*  Delete the edge from tgn to rn.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheDeleteEdge(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn)
{
  int pos;
  if( !HaArrayContains(tgn->resource_nodes, rn, &pos) )
    HnAbort("KheDeleteEdge internal error 1");
  HaArrayDeleteAndShift(tgn->resource_nodes, pos);
  if( !HaArrayContains(rn->task_group_nodes, tgn, &pos) )
    HnAbort("KheDeleteEdge internal error 2");
  HaArrayDeleteAndShift(rn->task_group_nodes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEdgeAssignable(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn,    */
/*    KHE_SOLN soln, KHE_COST *cost)                                         */
/*                                                                           */
/*  If rn can be assigned to tgn without increasing the solution cost or     */
/*  the number of unassignable tixels in soln's matching, return true with   */
/*  *cost set to the solution cost after the assignment (but don't leave     */
/*  the assignment there).  Otherwise return false.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheEdgeAssignable(KHE_TASK_GROUP_NODE tgn, KHE_RESOURCE_NODE rn,
  KHE_SOLN soln, KHE_COST *cost)
{
  int unmatched_before, unmatched_after;  bool res;  KHE_COST cost_before;
  KheSolnMatchingMarkBegin(soln);
  cost_before = KheSolnCost(soln);
  unmatched_before = KheSolnMatchingDefectCount(soln);
  if( KheTaskGroupAssign(tgn->task_group, rn->resource) )
  {
    *cost = KheSolnCost(soln);
    unmatched_after = KheSolnMatchingDefectCount(soln);
    KheTaskGroupUnAssign(tgn->task_group, rn->resource);
    res = unmatched_after <= unmatched_before && *cost < cost_before;
  }
  else
    res = false;
  KheSolnMatchingMarkEnd(soln, true);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskWantsResource(KHE_TASK task, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Return true if task wants r, because one of its descendants is derived   */
/*  from an event resource which has a task which is already assigned r.     */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskWantsResource(KHE_TASK task, KHE_RESOURCE r)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;

  /* see whether task itself wants r */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
      if( KheTaskAsstResource(KheEventResourceTask(soln, er, i)) == r )
	return true;

  /* see whether any tasks assigned to task want r */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    if( KheTaskWantsResource(KheTaskAssignedTo(task, i), r) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "most constraint first resource assignment"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMostConstrainedFirstAssignResources(KHE_TASKING tasking,         */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign resources to the tasks of tasking, most constrained first.        */
/*                                                                           */
/*****************************************************************************/

bool KheMostConstrainedFirstAssignResources(KHE_TASKING tasking,
  KHE_OPTIONS options)
{
  ARRAY_KHE_TASK_GROUP_NODE task_group_nodes;  KHE_TASK_GROUP_NODE tgn, tgn2;
  ARRAY_KHE_RESOURCE_NODE resource_nodes, best_resource_nodes;
  KHE_RESOURCE_NODE rn;  KHE_PRIQUEUE task_queue;  KHE_SOLN soln;
  KHE_INSTANCE ins;  int i, j, index, unassigned_tasks;
  KHE_COST best_cost, cost;  HA_ARENA a;
  KHE_TASK_GROUP task_group;  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;
  KHE_TASK task;  KHE_TASK_GROUPS task_groups;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheMostConstrainedFirstAssignResources(tasking):\n");
    KheTaskingDebug(tasking, 2, 2, stderr);
  }

  /* make task groups */
  soln = KheTaskingSoln(tasking);
  a = KheSolnArenaBegin(soln, false);
  /* a = HaArena Make(); */
  ins = KheSolnInstance(soln);
  task_groups = KheTaskGroupsMakeFromTasking(tasking);

  /* build one task set node for each incompletely assigned task group */
  HaArrayInit(task_group_nodes, a);
  for( i = 0;  i < KheTaskGroupsTaskGroupCount(task_groups);  i++ )
  {
    task_group = KheTaskGroupsTaskGroup(task_groups, i);
    unassigned_tasks = KheTaskGroupUnassignedTaskCount(task_group);
    if( unassigned_tasks > 0 )
    {
      tgn = KheTaskGroupNodeMake(task_group, unassigned_tasks, a);
      HaArrayAddLast(task_group_nodes, tgn);
    }
  }

  /* build one resource node for each resource in any domain, plus edges */
  HaArrayInit(resource_nodes, a);
  HaArrayFill(resource_nodes, KheInstanceResourceCount(ins), NULL);
  HaArrayForEach(task_group_nodes, tgn, i)
  {
    rg = KheTaskGroupDomain(tgn->task_group);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      index = KheResourceInstanceIndex(r);
      if( HaArray(resource_nodes, index) == NULL )
	HaArrayPut(resource_nodes, index, KheResourceNodeMake(r, a));
      rn = HaArray(resource_nodes, index);
      if( KheEdgeAssignable(tgn, rn, soln, &cost) )
	KheAddEdge(tgn, rn);
    }
  }

  /* create and populate the priority queue */
  task_queue = KhePriQueueMake(&KheTaskGroupNodeKey, &KheTaskGroupNodeIndex,
    &KheTaskGroupNodeSetIndex, a);
  HaArrayForEach(task_group_nodes, tgn, i)
    KhePriQueueInsert(task_queue, tgn);

  /* do the assigning */
  HaArrayInit(best_resource_nodes, a);
  while( !KhePriQueueEmpty(task_queue) )
  {
    /* find the task set node that needs assigning next */
    tgn = (KHE_TASK_GROUP_NODE) KhePriQueueDeleteMin(task_queue);
    if( DEBUG1 )
      KheTaskGroupNodeDebug(tgn, 2, 2, stderr);

    /* find the best resources to assign to it */
    HaArrayClear(best_resource_nodes);
    best_cost = KheCostMax;
    HaArrayForEach(tgn->resource_nodes, rn, i)
      if( !KheEdgeAssignable(tgn, rn, soln, &cost) )
      {
	/* rn has gone bad, remove it permanently */
        KheDeleteEdge(tgn, rn);
	i--;
      }
      else if( cost <= best_cost )
      {
	/* rn is one of the best available */
	if( cost < best_cost )
	{
	  best_cost = cost;
	  HaArrayClear(best_resource_nodes);
	}
	HaArrayAddLast(best_resource_nodes, rn);
      }

    /* if there is a suitable resource, assign one (else do nothing) */
    if( HaArrayCount(best_resource_nodes) > 0 )
    {
      /* find task, an unassigned task of tgn */
      task = NULL;  /* keep compiler happy */
      for( i = 0;  i < KheTaskGroupTaskCount(tgn->task_group);  i++ )
      {
	task = KheTaskGroupTask(tgn->task_group, i);
	if( KheTaskAsst(task) == NULL )
	  break;
      }
      HnAssert(i < KheTaskGroupTaskCount(tgn->task_group),
        "KheMostConstrainedFirstAssignResources internal error 1");

      /* find rn, the best of best_resource_nodes to assign to task */
      HaArrayForEach(best_resource_nodes, rn, i)
	if( KheTaskWantsResource(task, rn->resource) )
	  break;
      if( i >= HaArrayCount(best_resource_nodes) )
	rn = HaArrayFirst(best_resource_nodes);

      /* assign rn's resource to task */
      if( DEBUG1 )
      {
	fprintf(stderr, "    assigning ");
	KheResourceDebug(rn->resource, 1, -1, stderr);
	fprintf(stderr, " to ");
	KheTaskDebug(task, 1, 0, stderr);
      }
      if( !KheTaskAssignResource(task, rn->resource) )
	HnAbort("KheMostConstrainedFirstAssignResources internal err 2");
      KheDeleteEdge(tgn, rn);

      /* if tgn still has unassigned tasks, reinsert, else delete all edges */
      tgn->unassigned_tasks--;
      if( tgn->unassigned_tasks > 0 )
	KhePriQueueInsert(task_queue, tgn);
      else
      {
	while( HaArrayCount(tgn->resource_nodes) > 0 )
          KheDeleteEdge(tgn, HaArrayFirst(tgn->resource_nodes));
      }

      /* update rn's availability over all its edges */
      HaArrayForEach(rn->task_group_nodes, tgn2, i)
      {
	HnAssert(tgn2 != tgn,
	  "KheMostConstrainedFirstAssignResources internal error 3");
	if( !KheEdgeAssignable(tgn2, rn, soln, &cost) )
	{
	  KheDeleteEdge(tgn2, rn);
	  KhePriQueueNotifyKeyChange(task_queue, tgn2);
	  i--;
	}
      }
    }
  }

  /* free memory */
  KheSolnArenaEnd(soln, a);
  /* KhePriQueueDelete(task_queue); */
  /* HaArenaDel te(a); */
  /* ***
  MArrayFree(best_resource_nodes);
  HaArrayForEach(task_group_nodes, tgn, i)
    KheTaskGroupNodeDelete(tgn);
  MArrayFree(task_group_nodes);
  HaArrayForEach(resource_nodes, rn, i)
    if( rn != NULL )
      KheResourceNodeDelete(rn);
  *** */

  if( DEBUG1 )
    fprintf(stderr, "] KheMostConstrainedFirstAssignResources returning\n");
  return true;
}

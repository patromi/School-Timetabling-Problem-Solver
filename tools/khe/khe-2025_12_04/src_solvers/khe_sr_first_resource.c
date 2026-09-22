
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
/*  KHE_MTASK_NODE - a node representing one mtask                           */
/*  KHE_RESOURCE_NODE - a node representing one resource                     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_node_rec *KHE_MTASK_NODE;
typedef HA_ARRAY(KHE_MTASK_NODE) ARRAY_KHE_MTASK_NODE;

typedef struct khe_resource_node_rec *KHE_RESOURCE_NODE;
typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;

struct khe_mtask_node_rec {
  int				priqueue_index;		/* for priqueue      */
  KHE_MTASK			mtask;			/* tasks to do       */
  int				unassigned_tasks;	/* no. unassigned    */
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;		/* avail resources   */
};

struct khe_resource_node_rec {
  KHE_RESOURCE			resource;		/* the resource      */
  ARRAY_KHE_MTASK_NODE		mtask_nodes;		/* suitable mtasks   */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "mtask nodes" (private)                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_NODE KheMTaskNodeMake(KHE_MTASK mtask,                         */
/*    int unassigned_tasks, HA_ARENA a)                                      */
/*                                                                           */
/*  Create a new task set node with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_NODE KheMTaskNodeMake(KHE_MTASK mtask,
  int unassigned_tasks, HA_ARENA a)
{
  KHE_MTASK_NODE res;
  HaMake(res, a);
  res->priqueue_index = -1;  /* undefined here */
  res->mtask = mtask;
  res->unassigned_tasks = unassigned_tasks;
  HaArrayInit(res->resource_nodes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeDelete(KHE_MTASK_NODE mtn)                              */
/*                                                                           */
/*  Delete mtn.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMTaskNodeDelete(KHE_MTASK_NODE mtn)
{
  MArrayFree(mtn->resource_nodes);
  MFree(mtn);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int64_t KheMTaskNodeKey(void *mtn)                                       */
/*                                                                           */
/*  PriQueue callback function which returns the priority of mtn.            */
/*                                                                           */
/*****************************************************************************/

static int64_t KheMTaskNodeKey(void *mtn)
{
  KHE_MTASK_NODE mt_node = (KHE_MTASK_NODE) mtn;
  return HaArrayCount(mt_node->resource_nodes) - mt_node->unassigned_tasks;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskNodeIndex(void *mtn)                                         */
/*                                                                           */
/*  PriQueue callback function which returns the index of mtn.               */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskNodeIndex(void *mtn)
{
  return ((KHE_MTASK_NODE) mtn)->priqueue_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeSetIndex(void *mtn, int index)                          */
/*                                                                           */
/*  PriQueue callback function which sets the index of mtn.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskNodeSetIndex(void *mtn, int index)
{
  ((KHE_MTASK_NODE) mtn)->priqueue_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeDebug(KHE_MTASK_NODE mtn, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mtn onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskNodeDebug(KHE_MTASK_NODE mtn, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ MTaskNode %ld (%d resources, %d unassigned tasks)",
      KheMTaskNodeKey(mtn), HaArrayCount(mtn->resource_nodes),
      mtn->unassigned_tasks);
    if( indent >= 0 )
    {
      fprintf(fp, ":\n");
      KheMTaskDebug(mtn->mtask, 2, 4, stderr);
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
  HaArrayInit(res->mtask_nodes, a);
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
/*  void KheAddEdge(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)                */
/*                                                                           */
/*  Add an edge from mtn to rn.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheAddEdge(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)
{
  HaArrayAddLast(mtn->resource_nodes, rn);
  HaArrayAddLast(rn->mtask_nodes, mtn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDeleteEdge(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)             */
/*                                                                           */
/*  Delete the edge from mtn to rn.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheDeleteEdge(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)
{
  int pos;
  if( !HaArrayContains(mtn->resource_nodes, rn, &pos) )
    HnAbort("KheDeleteEdge internal error 1");
  HaArrayDeleteAndShift(mtn->resource_nodes, pos);
  if( !HaArrayContains(rn->mtask_nodes, mtn, &pos) )
    HnAbort("KheDeleteEdge internal error 2");
  HaArrayDeleteAndShift(rn->mtask_nodes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEdgeAssignable(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn,         */
/*    KHE_SOLN soln, KHE_COST *cost)                                         */
/*                                                                           */
/*  If rn can be assigned to mtn without increasing the solution cost or     */
/*  the number of unassignable tixels in soln's matching, return true with   */
/*  *cost set to the solution cost after the assignment (but don't leave     */
/*  the assignment there).  Otherwise return false.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheEdgeAssignable(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn,
  KHE_SOLN soln, KHE_COST *cost)
{
  int unmatched_before, unmatched_after;  bool res;  KHE_COST cost_before;
  KheSolnMatchingMarkBegin(soln);
  cost_before = KheSolnCost(soln);
  unmatched_before = KheSolnMatchingDefectCount(soln);
  if( KheMTaskMoveResource(mtn->mtask, NULL, rn->resource, true) )
  {
    *cost = KheSolnCost(soln);
    unmatched_after = KheSolnMatchingDefectCount(soln);
    KheMTaskMoveResource(mtn->mtask, rn->resource, NULL, true);
    res = (unmatched_after <= unmatched_before && *cost < cost_before);
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

/* *** no longer used
static bool KheTaskWantsResource(KHE_TASK task, KHE_RESOURCE r)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;

  ** see whether task itself wants r **
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
      if( KheTaskAsstResource(KheEventResourceTask(soln, er, i)) == r )
	return true;

  ** see whether any tasks assigned to task want r **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    if( KheTaskWantsResource(KheTaskAssignedTo(task, i), r) )
      return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "most constrained first resource assignment"                   */
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

bool KheMostConstrainedFirstAssignResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  ARRAY_KHE_MTASK_NODE mtask_nodes;  KHE_MTASK_NODE mtn, mtn2;
  ARRAY_KHE_RESOURCE_NODE resource_nodes, best_resource_nodes;
  KHE_RESOURCE_NODE rn;  KHE_PRIQUEUE mtask_queue;
  KHE_INSTANCE ins;  int i, j, index, unassigned_tasks;
  KHE_COST best_cost, cost;  HA_ARENA a;  KHE_FRAME days_frame;
  KHE_MTASK_FINDER mtf;  KHE_MTASK mtask;
  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;
  KHE_TASK task;  /* KHE_TASK_GROUPS task_groups; */
  /* KHE_COST asst_cost, non_asst_cost; */
  /* KHE_EVENT_TIMETABLE_MONITOR etm; */
  if( DEBUG1 )
    fprintf(stderr, "[ KheMostConstrainedFirstAssignResources(soln, %s)\n",
      KheResourceTypeId(rt));

  /* make mtasks */
  a = KheSolnArenaBegin(soln);
  /* a = HaArena Make(); */
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  /* ***
  e tm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  *** */
  ins = KheSolnInstance(soln);
  mtf = KheMTaskFinderMake(soln, rt, days_frame, /* etm, */ true, a);
  /* task_groups = KheTaskGroupsMakeFromTasking(tasking); */

  /* build one mtask node for each incompletely assigned mtask */
  HaArrayInit(mtask_nodes, a);
  for( i = 0;  i < KheMTaskFinderMTaskCount(mtf);  i++ )
  {
    mtask = KheMTaskFinderMTask(mtf, i);
    unassigned_tasks = KheMTaskUnassignedTaskCount(mtask);
    if( unassigned_tasks > 0 )
    {
      mtn = KheMTaskNodeMake(mtask, unassigned_tasks, a);
      HaArrayAddLast(mtask_nodes, mtn);
    }
  }

  /* build one resource node for each resource in any domain, plus edges */
  HaArrayInit(resource_nodes, a);
  HaArrayFill(resource_nodes, KheInstanceResourceCount(ins), NULL);
  HaArrayForEach(mtask_nodes, mtn, i)
  {
    rg = KheMTaskDomain(mtn->mtask);
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      index = KheResourceInstanceIndex(r);
      if( HaArray(resource_nodes, index) == NULL )
	HaArrayPut(resource_nodes, index, KheResourceNodeMake(r, a));
      rn = HaArray(resource_nodes, index);
      if( KheEdgeAssignable(mtn, rn, soln, &cost) )
	KheAddEdge(mtn, rn);
    }
  }

  /* create and populate the priority queue */
  mtask_queue = KhePriQueueMake(&KheMTaskNodeKey, &KheMTaskNodeIndex,
    &KheMTaskNodeSetIndex, a);
  HaArrayForEach(mtask_nodes, mtn, i)
    KhePriQueueInsert(mtask_queue, mtn);

  /* do the assigning */
  HaArrayInit(best_resource_nodes, a);
  while( !KhePriQueueEmpty(mtask_queue) )
  {
    /* find the task set node that needs assigning next */
    mtn = (KHE_MTASK_NODE) KhePriQueueDeleteMin(mtask_queue);
    if( DEBUG1 )
      KheMTaskNodeDebug(mtn, 2, 2, stderr);

    /* find the best resources to assign to it */
    HaArrayClear(best_resource_nodes);
    best_cost = KheCostMax;
    HaArrayForEach(mtn->resource_nodes, rn, i)
      if( !KheEdgeAssignable(mtn, rn, soln, &cost) )
      {
	/* rn has gone bad, remove it permanently */
        KheDeleteEdge(mtn, rn);
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
      /* find one of the best_resource_nodes to assign to mtn */
      if( KheMTaskResourceAssignSuggestion(mtn->mtask, &r) )
      {
	HaArrayForEach(best_resource_nodes, rn, i)
	  if( rn->resource == r )
	    break;
	if( i < HaArrayCount(best_resource_nodes) )
	{
	  /* we have a suggestion and it is one of the best resource nodes */
	  rn = HaArray(best_resource_nodes, i);
	}
	else
	{
	  /* suggestion is not one of the best resource nodes */
	  rn = HaArrayFirst(best_resource_nodes);
	}
      }
      else
      {
	/* no suggestion, just use first resource node */
	rn = HaArrayFirst(best_resource_nodes);
      }

      /* ***
      ** find task, an unassigned task of mtn **
      task = NULL;  ** keep compiler happy **
      for( i = 0;  i < KheMTaskTaskCount(mtn->mtask);  i++ )
      {
	task = KheMTaskT ask(mtn->mtask, i, &asst_cost, &non_asst_cost);
	if( KheTaskAsst(task) == NULL )
	  break;
      }
      HnAssert(i < KheMTaskTaskCount(mtn->mtask),
        "KheMostConstrainedFirstAssignResources internal error 1");

      ** find rn, the best of best_resource_nodes to assign to task **
      HaArrayForEach(best_resource_nodes, rn, i)
	if( KheTaskWantsResource(task, rn->resource) )
	  break;
      if( i >= HaArrayCount(best_resource_nodes) )
	rn = HaArrayFirst(best_resource_nodes);
      *** */

      /* assign rn's resource to task */
      if( DEBUG1 )
      {
	fprintf(stderr, "    assigning ");
	KheResourceDebug(rn->resource, 1, -1, stderr);
	fprintf(stderr, " to ");
	KheTaskDebug(task, 1, 0, stderr);
      }
      if( !KheMTaskMoveResource(mtn->mtask, NULL, rn->resource, true) )
	HnAbort("KheMostConstrainedFirstAssignResources internal err 2");
      KheDeleteEdge(mtn, rn);

      /* if mtn still has unassigned tasks, reinsert, else delete all edges */
      mtn->unassigned_tasks--;
      if( mtn->unassigned_tasks > 0 )
	KhePriQueueInsert(mtask_queue, mtn);
      else
      {
	while( HaArrayCount(mtn->resource_nodes) > 0 )
          KheDeleteEdge(mtn, HaArrayFirst(mtn->resource_nodes));
      }

      /* update rn's availability over all its edges */
      HaArrayForEach(rn->mtask_nodes, mtn2, i)
      {
	HnAssert(mtn2 != mtn,
	  "KheMostConstrainedFirstAssignResources internal error 3");
	if( !KheEdgeAssignable(mtn2, rn, soln, &cost) )
	{
	  KheDeleteEdge(mtn2, rn);
	  KhePriQueueNotifyKeyChange(mtask_queue, mtn2);
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
  HaArrayForEach(task_group_nodes, mtn, i)
    KheMTaskNodeDelete(mtn);
  MArrayFree(task_group_nodes);
  HaArrayForEach(resource_nodes, rn, i)
    if( rn != NULL )
      KheResourceNodeDelete(rn);
  *** */

  if( DEBUG1 )
    fprintf(stderr, "] KheMostConstrainedFirstAssignResources returning\n");
  return true;
}

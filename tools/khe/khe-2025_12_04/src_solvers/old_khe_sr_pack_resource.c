
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
/*  FILE:         khe_sr_pack_resource.c                                     */
/*  DESCRIPTION:  Resource packing                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_priqueue.h"

#define DEBUG1 0
#define DEBUG2 0

typedef struct khe_task_group_node_rec *KHE_TASK_GROUP_NODE;
typedef HA_ARRAY(KHE_TASK_GROUP_NODE) ARRAY_KHE_TASK_GROUP_NODE;

typedef struct khe_resource_node_rec *KHE_RESOURCE_NODE;
typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;

typedef struct khe_pack_solver_rec *KHE_PACK_SOLVER;

typedef HA_ARRAY(KHE_RESOURCE_TYPE) ARRAY_KHE_RESOURCE_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_NODE - a node representing one task group                 */
/*                                                                           */
/*****************************************************************************/

struct khe_task_group_node_rec {
  KHE_PACK_SOLVER		pack_solver;		/* enclosing pack s. */
  KHE_TASK_GROUP		task_group;		/* tasks to do       */
  int				goodness;		/* goodness of tasks */
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;		/* avail resources   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE - a node representing one resource                     */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_node_rec {

  /* kept up to date throughout */
  KHE_PACK_SOLVER		pack_solver;		/* enclosing pack s. */
  KHE_RESOURCE			resource;		/* the resource      */
  ARRAY_KHE_TASK_GROUP_NODE	task_group_nodes;	/* suitable tasks    */
  int				demand_durn;		/* their total durn  */
  int				supply_durn;		/* available wkload  */
  int				priqueue_index;		/* index in priqueue */

  /* used only while packing the resource */
  ARRAY_KHE_TASK_GROUP_NODE	curr_task_group_nodes;	/* current asst      */
  int				curr_goodness;		/* their total good. */
  ARRAY_KHE_TASK_GROUP_NODE	best_task_group_nodes;	/* best asst         */
  int				best_goodness;		/* their total good. */
  /* KHE_COST			best_cost; */		/* cost of best asst */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_PACK_SOLVER - an object holding everything involved in one solve     */
/*                                                                           */
/*****************************************************************************/

struct khe_pack_solver_rec {
  HA_ARENA			arena;			/* holds the lot     */
  KHE_TASKING			tasking;		/* orig. tasking     */
  ARRAY_KHE_TASK_GROUP_NODE	task_group_nodes;	/* task group nodes  */
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;		/* resource nodes    */
  KHE_PRIQUEUE			priqueue;		/* resource priqueue */
};


/*****************************************************************************/
/*                                                                           */
/*  static function declarations                                             */
/*                                                                           */
/*****************************************************************************/

/* task group nodes */
static void KheTaskGroupNodeAddResourceNode(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn);
/* ***
static void KheTaskGroupNodeDeleteResourceNode(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn);
*** */
static bool KheTaskGroupNodeAssignCheck(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn);
static bool KheTaskGroupNodeAssign(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn);
static void KheTaskGroupNodeUnAssign(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn);

/* resource nodes */
static void KheResourceNodeAddTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn);
static void KheResourceNodeDeleteTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn);
static void KheResourceNodeAssignTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn);
static void KheResourceNodeUnAssignTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn);

/* pack solver */
static void KhePackSolverAddTaskGroupNode(KHE_PACK_SOLVER ps,
  KHE_TASK_GROUP_NODE tgn);
static void KhePackSolverDeleteTaskGroupNode(KHE_PACK_SOLVER ps,
  KHE_TASK_GROUP_NODE tgn);
static void KhePackSolverAddResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn);
/* ***
static void KhePackSolverDeleteResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn);
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task group nodes" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetAccumulateGoodness(KHE_MEET meet, int *goodness)             */
/*                                                                           */
/*  Accumulate the goodness of assigning a task of meet into *goodness.      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMeetAccumulateGoodness(KHE_MEET meet, int *goodness)
{
  KHE_MEET leader_meet;  int junk;
  *goodness += KheMeetDuration(meet) * 20;
  leader_meet = KheM eetLeader(meet, &junk);
  if( leader_meet != NULL && KheMeetNode(leader_meet) != NULL )
    *goodness += KheNodeDemand(KheMeetNode(leader_meet));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAccumulateGoodness(KHE_TASK task, int *goodness)             */
/*                                                                           */
/*  Accumulate the goodness of task and its followers into *goodness.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskAccumulateGoodness(KHE_TASK task, int *goodness)
{
  int i;

  ** do the job for task itself **
  if( KheTaskMeet(task) != NULL )
    KheMeetAccumulateGoodness(KheTaskMeet(task), goodness);

  ** do the job for task's followers **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    KheTaskAccumulateGoodness(KheTaskAssignedTo(task, i), goodness);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupGoodness(KHE_TASK_GROUP task_group)                      */
/*                                                                           */
/*  The goodness of assigning a task from task_group.                        */
/*                                                                           */
/*  The value chosen is the maximum, over all the tasks of the task          */
/*  group, of the demand of the first unfixed meet on the path of            */
/*  the task's meet.  This encourages tasks lying in large, complex          */
/*  meets to be assigned first, leaving tasks in small meets unassigned,     */
/*  which is desirable since those tasks are likely to be easy to repair.    */
/*                                                                           */
/*  Obsolete:                                                                */
/*  The value chosen is the common total duration of the tasks of the        */
/*  task group, minus the difference between the number of qualified         */
/*  resources and the number of tasks.                                       */
/*                                                                           */
/*  This is based on the assumption that tasks in the same task group        */
/*  are likely to be necessarily simultaneous, so that as their number       */
/*  decreases it becomes less necessary to timetable them.                   */
/*                                                                           */
/*  Obsolete:                                                                */
/*  The value chosen is the sum, over all tasks of the task group, of the    */
/*  duration * 20 plus the demand of the node containing the task's meet's   */
/*  leader meet, if any.  This emphasizes tasks whose meets will be hard to  */
/*  move later, if that is needed in order to find assignments for them.     */
/*                                                                           */
/*****************************************************************************/

static int KheTaskGroupGoodness(KHE_TASK_GROUP task_group)
{
  int i, demand, max_demand, junk;  KHE_TASK task;  KHE_MEET meet;
  max_demand = 0;
  for( i = 0;  i < KheTaskGroupTaskCount(task_group);  i++ )
  {
    task = KheTaskGroupTask(task_group, i);
    meet = KheTaskMeet(task);
    if( meet != NULL )
    {
      meet = KheMeetFirstMovable(meet, &junk);
      if( meet != NULL )
      {
	demand = KheMeetDemand(meet);
	if( demand > max_demand )
	  max_demand = demand;
      }
    }
  }
  return max_demand;

  /* *** a nice collection of alternative definitions
  return KheTaskGroupTotalDuration(task_group);
  *** */

  /* ***
  int excess, res;
  excess = KheResourceGroupResourceCount(KheTaskGroupDomain(task_group)) -
    KheTaskGroupTaskCount(task_group);
  res = 2 * KheTaskGroupTotalDuration(task_group) - excess;
  if( res < 1 )
    res = 1;
  return res;
  *** */

  /* ***
  int i, res;  KHE_TASK task;
  res = 0;
  for( i = 0;  i < KheTaskGroupTaskCount(task_group);  i++ )
  {
    task = KheTaskGroupTask(task_group, i);
    KheTaskAccumulateGoodness(task, &res);
  }
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_NODE KheTaskGroupNodeMake(KHE_PACK_SOLVER ps,             */
/*    KHE_TASK_GROUP task_group)                                             */
/*                                                                           */
/*  Make one task group node for ps, representing task_group.                */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_GROUP_NODE KheTaskGroupNodeMake(KHE_PACK_SOLVER ps,
  KHE_TASK_GROUP task_group)
{
  KHE_TASK_GROUP_NODE res;
  HaMake(res, ps->arena);
  res->pack_solver = ps;
  res->task_group = task_group;
  res->goodness = KheTaskGroupGoodness(task_group);
  HaArrayInit(res->resource_nodes, ps->arena);
  KhePackSolverAddTaskGroupNode(ps, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeAddResourceNode(KHE_TASK_GROUP_NODE tgn,            */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Add rn to tgn's list of resource nodes.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupNodeAddResourceNode(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn)
{
  HaArrayAddLast(tgn->resource_nodes, rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeDeleteResourceNode(KHE_TASK_GROUP_NODE tgn,         */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Delete rn from tgn.                                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskGroupNodeDeleteResourceNode(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn)
{
  int pos;
  if( !HaArrayContains(tgn->resource_nodes, rn, &pos) )
    HnAbort("KheTaskGroupNodeDeleteResourceNode internal error");
  HaArrayDeleteAndShift(tgn->resource_nodes, pos);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeDelete(KHE_TASK_GROUP_NODE tgn)                     */
/*                                                                           */
/*  Delete tgn.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupNodeDelete(KHE_TASK_GROUP_NODE tgn)
{
  KhePackSolverDeleteTaskGroupNode(tgn->pack_solver, tgn);
  while( HaArrayCount(tgn->resource_nodes) > 0 )
    KheResourceNodeDeleteTaskGroupNode(
      HaArrayLastAndDelete(tgn->resource_nodes), tgn);
  /* ***
  MArrayFree(tgn->resource_nodes);
  MFree(tgn);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupNodeAssign(KHE_TASK_GROUP_NODE tgn,                     */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Assign rn to tgn, but only if possible and soln cost does not increase.  */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskGroupNodeAssign(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn)
{
  KHE_SOLN soln;  int durn, unmatched_before;  KHE_RESOURCE_GROUP rg;
  KHE_COST cost_before;
  durn = KheTaskGroupTotalDuration(tgn->task_group);
  rg = KheTaskGroupDomain(tgn->task_group);
  if( durn > rn->supply_durn || !KheResourceGroupContains(rg, rn->resource) )
    return false;
  soln = KheTaskingSoln(tgn->pack_solver->tasking);
  unmatched_before = KheSolnMatchingDefectCount(soln);
  cost_before = KheSolnCost(soln);
  if( !KheTaskGroupAssign(tgn->task_group, rn->resource) )
    return false;
  else if( KheSolnMatchingDefectCount(soln) > unmatched_before ||
    KheSolnCost(soln) > cost_before )
  {
    KheTaskGroupUnAssign(tgn->task_group, rn->resource);
    return false;
  }
  KheResourceNodeAssignTaskGroupNode(rn, tgn);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupNodeAssignDebug(KHE_TASK_GROUP_NODE tgn,                */
/*    KHE_RESOURCE_NODE rn, FILE *fp)                                        */
/*                                                                           */
/*  Like KheTaskGroupNodeAssign, but also print a debug message on fp        */
/*  saying why the result is false, if it is false.                          */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currrently used
static bool KheTaskGroupNodeAssignDebug(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn, FILE *fp)
{
  KHE_SOLN soln;  int durn, unmatched_before;  KHE_RESOURCE_GROUP rg;
  KHE_COST cost_before;
  durn = KheTaskGroupTotalDuration(tgn->task_group);
  rg = KheTaskGroupDomain(tgn->task_group);
  if( durn > rn->supply_durn )
  {
    fprintf(stderr, "insufficient duration: %d > %d", durn, rn->supply_durn);
    return false;
  }
  if( !KheResourceGroupContains(rg, rn->resource) )
  {
    fprintf(stderr, "resource not in domain");
    return false;
  }
  soln = KheTaskingSoln(tgn->pack_solver->tasking);
  unmatched_before = KheSolnMatchingDefectCount(soln);
  cost_before = KheSolnCost(soln);
  if( !KheTaskGroupAssign(tgn->task_group, rn->resource) )
  {
    fprintf(stderr, "KheTaskGroupAssign returned false");
    return false;
  }
  else if( KheSolnMatchingDefectCount(soln) > unmatched_before )
  {
    fprintf(stderr, "matching (%d > %d)", KheSolnMatchingDefectCount(soln),
      unmatched_before);
    KheTaskGroupUnAssign(tgn->task_group, rn->resource);
    return false;
  }
  else if( KheSolnCost(soln) > cost_before )
  {
    fprintf(stderr, "cost (%.5f > %.5f)", KheCostShow(KheSolnCost(soln)),
      KheCostShow(cost_before));
    KheTaskGroupUnAssign(tgn->task_group, rn->resource);
    return false;
  }
  KheResourceNodeAssignTaskGroupNode(rn, tgn);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupNodeAssignCheck(KHE_TASK_GROUP_NODE tgn,                */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  If rn can be assigned to tgn, return true; do not make the assignment.   */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskGroupNodeAssignCheck(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn)
{
  if( KheTaskGroupNodeAssign(tgn, rn) )
  {
    KheTaskGroupNodeUnAssign(tgn, rn);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupNodeAssignCheckDebug(KHE_TASK_GROUP_NODE tgn,           */
/*    KHE_RESOURCE_NODE rn, FILE *fp)                                        */
/*                                                                           */
/*  Like KheTaskGroupNodeAssignCheck, but also print a message on fp         */
/*  saying why the result is false, if it is false.                          */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currently used
static bool KheTaskGroupNodeAssignCheckDebug(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn, FILE *fp)
{
  if( KheTaskGroupNodeAssignDebug(tgn, rn, fp) )
  {
    KheTaskGroupNodeUnAssign(tgn, rn);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupNodeUnAssign(KHE_TASK_GROUP_NODE tgn,                   */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Unassign rn from tgn.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupNodeUnAssign(KHE_TASK_GROUP_NODE tgn,
  KHE_RESOURCE_NODE rn)
{
  KheResourceNodeUnAssignTaskGroupNode(rn, tgn);
  KheTaskGroupUnAssign(tgn->task_group, rn->resource);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupNodeDecreasingDurationCmp(const void *t1, const void *t2)*/
/*                                                                           */
/*  Comparison function for sorting an array of task group nodes by          */
/*  decreasing total duration.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static int KheTaskGroupNodeDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_TASK_GROUP_NODE tgn1 = * (KHE_TASK_GROUP_NODE *) t1;
  KHE_TASK_GROUP_NODE tgn2 = * (KHE_TASK_GROUP_NODE *) t2;
  return KheTaskGroupDecreasingDurationCmp(tgn1->task_group, tgn2->task_group);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupNodeDecreasingGoodnessCmp(const void *t1, const void *t2)*/
/*                                                                           */
/*  Comparison function for sorting an array of task group nodes by          */
/*  decreasing total duration.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheTaskGroupNodeDecreasingGoodnessCmp(const void *t1, const void *t2)
{
  KHE_TASK_GROUP_NODE tgn1 = * (KHE_TASK_GROUP_NODE *) t1;
  KHE_TASK_GROUP_NODE tgn2 = * (KHE_TASK_GROUP_NODE *) t2;
  return tgn2->goodness - tgn1->goodness;
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
  KHE_RESOURCE_NODE rn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Task Group Node (%d unassigned tasks, %d resources, %d goodness)\n",
      indent, "", KheTaskGroupUnassignedTaskCount(tgn->task_group),
      HaArrayCount(tgn->resource_nodes), tgn->goodness);
    if( verbosity > 1 )
    {
      KheTaskGroupDebug(tgn->task_group, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  resources: ", indent, "");
      HaArrayForEach(tgn->resource_nodes, rn, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	KheResourceDebug(rn->resource, 1, -1, fp);
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource nodes" (private)                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSupplyDuration(KHE_SOLN soln, KHE_RESOURCE r)             */
/*                                                                           */
/*  Return the supply duration of r in soln:  the cycle length minus         */
/*  r's number of workload demand monitors minus the total duration of       */
/*  the tasks that r is already assigned to.                                 */
/*                                                                           */
/*****************************************************************************/

static int KheResourceSupplyDuration(KHE_SOLN soln, KHE_RESOURCE r)
{
  int i, res;  KHE_MONITOR m;  KHE_TASK task;
  res = KheInstanceTimeCount(KheSolnInstance(soln));
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
      res--;
  }
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    res -= KheTaskTotalDuration(task);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE KheResourceNodeMake(KHE_PACK_SOLVER ps,                */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Make a resource node for ps representing r.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_NODE KheResourceNodeMake(KHE_PACK_SOLVER ps,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_NODE res;

  /* kept up to date throughout */
  HaMake(res, ps->arena);
  res->pack_solver = ps;
  res->resource = r;
  HaArrayInit(res->task_group_nodes, ps->arena);
  res->demand_durn = 0;
  res->supply_durn = KheResourceSupplyDuration(KheTaskingSoln(ps->tasking), r);
  res->priqueue_index = 0;

  /* used only while packing the resource */
  HaArrayInit(res->curr_task_group_nodes, ps->arena);
  res->curr_goodness = 0;
  HaArrayInit(res->best_task_group_nodes, ps->arena);
  /* res->best_cost = 0; */
  res->best_goodness = 0;
  KhePackSolverAddResourceNode(ps, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAddTaskGroupNode(KHE_RESOURCE_NODE rn,               */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Add tgn to rn's list of assignable task group nodes.                     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn)
{
  HaArrayAddLast(rn->task_group_nodes, tgn);
  rn->demand_durn += KheTaskGroupTotalDuration(tgn->task_group);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDeleteTaskGroupNode(KHE_RESOURCE_NODE rn,            */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Delete tgn from rn's list of assignable task group nodes.                */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDeleteTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn)
{
  int pos;
  rn->demand_durn -= KheTaskGroupTotalDuration(tgn->task_group);
  if( !HaArrayContains(rn->task_group_nodes, tgn, &pos) )
    HnAbort("KheResourceNodeDeleteTaskGroupNode internal error");
  HaArrayDeleteAndShift(rn->task_group_nodes, pos);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAssignTaskGroupNode(KHE_RESOURCE_NODE rn,            */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Update rn to assign tgn to it.  This function should not be called       */
/*  directly; call KheTaskGroupNodeAssignResourceNode instead.               */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAssignTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn)
{
  HaArrayAddLast(rn->curr_task_group_nodes, tgn);
  rn->curr_goodness += tgn->goodness;
  rn->supply_durn -= KheTaskGroupTotalDuration(tgn->task_group);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeUnAssignTaskGroupNode(KHE_RESOURCE_NODE rn,          */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Update rn to unassign tgn from it.  This function should not be called   */
/*  directly; call KheTaskGroupNodeUnAssignResourceNode instead.             */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeUnAssignTaskGroupNode(KHE_RESOURCE_NODE rn,
  KHE_TASK_GROUP_NODE tgn)
{
  int i;
  rn->supply_durn += KheTaskGroupTotalDuration(tgn->task_group);
  for( i = HaArrayCount(rn->curr_task_group_nodes) - 1;  i >= 0;  i-- )
    if( HaArray(rn->curr_task_group_nodes, i) == tgn )
      break;
  HnAssert(i >= 0, "KheResourceNodeUnAssignTaskGroupNode internal error");
  HaArrayDeleteAndShift(rn->curr_task_group_nodes, i);
  rn->curr_goodness -= tgn->goodness;
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
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
  KhePackSolverDeleteResourceNode(rn->pack_solver, rn);
  while( HaArrayCount(rn->task_group_nodes) > 0 )
    KheTaskGroupNodeDeleteResourceNode(
      HaArrayLastAndDelete(rn->task_group_nodes), rn);
  ** ***
  MArrayFree(rn->task_group_nodes);
  MArrayFree(rn->curr_task_group_nodes);
  MArrayFree(rn->best_task_group_nodes);
  MFree(rn);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int64_t KheResourceNodeKey(void *entry)                                  */
/*  int KheResourceNodeIndex(void *entry)                                    */
/*  void KheResourceNodeSetIndex(void *entry, int index)                     */
/*                                                                           */
/*  Priority queue callback functions.                                       */
/*                                                                           */
/*****************************************************************************/

static int64_t KheResourceNodeKey(void *entry)
{
  KHE_RESOURCE_NODE rn = (KHE_RESOURCE_NODE) entry;
  return rn->demand_durn - rn->supply_durn;
}

static int KheResourceNodeIndex(void *entry)
{
  return ((KHE_RESOURCE_NODE) entry)->priqueue_index;
}

static void KheResourceNodeSetIndex(void *entry, int index)
{
  ((KHE_RESOURCE_NODE) entry)->priqueue_index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rn onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_GROUP_NODE tgn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp,
      "%*s[ Resource Node %s (%d task group nodes, demand %d, supply %d)",
      indent, "",
      KheResourceId(rn->resource) == NULL ? "-" : KheResourceId(rn->resource),
      HaArrayCount(rn->task_group_nodes), rn->demand_durn, rn->supply_durn);
    if( verbosity >= 3 )
    {
      fprintf(fp, "\n");
      HaArrayForEach(rn->task_group_nodes, tgn, i)
	KheTaskGroupDebug(tgn->task_group, 2, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "pack solvers" (private)                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PACK_SOLVER KhePackSolverMake(KHE_TASKING tasking, HA_ARENA a)       */
/*                                                                           */
/*  Make a pack solver containing one task group node for each incompletely  */
/*  assigned task group of tasking, and one resource node for each resource  */
/*  capable of being assigned to any of those task groups, including adding  */
/*  the appropriate links between the two.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_PACK_SOLVER KhePackSolverMake(KHE_TASKING tasking, HA_ARENA a)
{
  KHE_PACK_SOLVER res;  int i, j, k, pos;
  KHE_TASK_GROUPS task_groups;  KHE_TASK_GROUP task_group;
  KHE_TASK_GROUP_NODE tgn;  KHE_RESOURCE_NODE rn;  KHE_RESOURCE r;
  ARRAY_KHE_RESOURCE_TYPE resource_types; KHE_RESOURCE_TYPE rt;

  /* build the pack solver object proper */
  HaMake(res, a);
  res->arena = a;
  res->tasking = tasking;
  HaArrayInit(res->task_group_nodes, a);
  HaArrayInit(res->resource_nodes, a);
  res->priqueue = KhePriQueueMake(&KheResourceNodeKey, &KheResourceNodeIndex,
    &KheResourceNodeSetIndex, a);

  /* one task group node for each incompletely assigned task group of tasking */
  task_groups = KheTaskGroupsMakeFromTasking(tasking);
  for( i = 0;  i < KheTaskGroupsTaskGroupCount(task_groups);  i++ )
  {
    task_group = KheTaskGroupsTaskGroup(task_groups, i);
    if( KheTaskGroupUnassignedTaskCount(task_group) > 0 )
      KheTaskGroupNodeMake(res, task_group);
  }
  HaArraySort(res->task_group_nodes, &KheTaskGroupNodeDecreasingGoodnessCmp);

  /* find the set of all resource types within the task groups */
  HaArrayInit(resource_types, a);
  HaArrayForEach(res->task_group_nodes, tgn, i)
  {
    rt = KheResourceGroupResourceType(KheTaskGroupDomain(tgn->task_group));
    if( !HaArrayContains(resource_types, rt, &pos) )
      HaArrayAddLast(resource_types, rt);
  }

  /* one resource node for each resource of each resource type, plus edges */
  HaArrayForEach(resource_types, rt, i)
    for( j = 0;  j < KheResourceTypeResourceCount(rt);  j++ )
    {
      r = KheResourceTypeResource(rt, j);
      rn = KheResourceNodeMake(res, r);
      HaArrayForEach(res->task_group_nodes, tgn, k)
	if( KheTaskGroupNodeAssignCheck(tgn, rn) )
	{
	  KheTaskGroupNodeAddResourceNode(tgn, rn);
	  KheResourceNodeAddTaskGroupNode(rn, tgn);
	}
      KhePriQueueInsert(res->priqueue, (void *) rn);
    }
  HaArrayFree(resource_types);

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverAddTaskGroupNode(KHE_PACK_SOLVER ps,                   */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Add tgn to ps.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverAddTaskGroupNode(KHE_PACK_SOLVER ps,
  KHE_TASK_GROUP_NODE tgn)
{
  HaArrayAddLast(ps->task_group_nodes, tgn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverDeleteTaskGroupNode(KHE_PACK_SOLVER ps,                */
/*    KHE_TASK_GROUP_NODE tgn)                                               */
/*                                                                           */
/*  Delete tgn from ps.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverDeleteTaskGroupNode(KHE_PACK_SOLVER ps,
  KHE_TASK_GROUP_NODE tgn)
{
  int pos;
  if( !HaArrayContains(ps->task_group_nodes, tgn, &pos) )
    HnAbort("KhePackSolverDeleteTaskGroupNode internal error");
  HaArrayDeleteAndShift(ps->task_group_nodes, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverAddResourceNode(KHE_PACK_SOLVER ps,                    */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Add rn to ps.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverAddResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn)
{
  HaArrayAddLast(ps->resource_nodes, rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverDeleteResourceNode(KHE_PACK_SOLVER ps,                 */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Delete rn from ps.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePackSolverDeleteResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn)
{
  int pos;
  if( !HaArrayContains(ps->resource_nodes, rn, &pos) )
    HnAbort("KhePackSolverDeleteResourceNode internal error");
  HaArrayDeleteAndShift(ps->resource_nodes, pos);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverDelete(KHE_PACK_SOLVER ps)                             */
/*                                                                           */
/*  Delete ps, including its task group nodes and resource nodes.            */
/*                                                                           */
/*****************************************************************************/

/* *** deleted by deleting the arena now
static void KhePackSolverDelete(KHE_PACK_SOLVER ps)
{
  KhePriQueueDelete(ps->priqueue);
  HaAre naDelete(ps->arena);
  ** ***
  while( HaArrayCount(ps->task_group_nodes) > 0 )
    KheTaskGroupNodeDelete(HaArrayLast(ps->task_group_nodes));
  while( HaArrayCount(ps->resource_nodes) > 0 )
    KheResourceNodeDelete(HaArrayLast(ps->resource_nodes));
  MArrayFree(ps->task_group_nodes);
  MArrayFree(ps->resource_nodes);
  MFree(ps);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverDebug(KHE_PACK_SOLVER ps, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ps onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverDebug(KHE_PACK_SOLVER ps, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_GROUP_NODE tgn;  KHE_RESOURCE_NODE rn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Pack Solver (%d task group nodes, %d resource nodes)\n",
      indent, "", HaArrayCount(ps->task_group_nodes),
      HaArrayCount(ps->resource_nodes));
    HaArrayForEach(ps->task_group_nodes, tgn, i)
      KheTaskGroupNodeDebug(tgn, verbosity, indent + 2, fp);
    HaArrayForEach(ps->resource_nodes, rn, i)
      KheResourceNodeDebug(rn, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "the solver"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDoResourcePack(KHE_RESOURCE_NODE rn, int index,                  */
/*    int depth_limit, KHE_SOLN soln)                                        */
/*                                                                           */
/*  Pack task group nodes rn->task_group_nodes[index..] into rn.  If         */
/*  depth_limit > 0, explore all possibilities with a binary tree search;    */
/*  if depth_limit == 0, use a simple linear heuristic, to save time.        */
/*                                                                           */
/*****************************************************************************/

static void KheDoResourcePack(KHE_RESOURCE_NODE rn, int index,
  int depth_limit, KHE_SOLN soln)
{
  KHE_TASK_GROUP_NODE tgn;  int i;

  /* if current state is best so far, remember it */
  if( rn->curr_goodness > rn->best_goodness )
  /* if( KheSolnCost(soln) < rn->best_cost ) */
  {
    HaArrayClear(rn->best_task_group_nodes);
    HaArrayAppend(rn->best_task_group_nodes, rn->curr_task_group_nodes, i);
    /* rn->best_cost = KheSolnCost(soln); */
    rn->best_goodness = rn->curr_goodness;
  }

  /* if there is any prospect of more assignments, try one and recurse */
  if( index < HaArrayCount(rn->task_group_nodes) && rn->supply_durn > 0 )
  {
    tgn = HaArray(rn->task_group_nodes, index);
    if( depth_limit == 0 )
    {
      /* at depth limit, so use a simple linear heuristic */
      if( KheTaskGroupNodeAssign(tgn, rn) )
      {
	KheDoResourcePack(rn, index + 1, depth_limit, soln);
	KheTaskGroupNodeUnAssign(tgn, rn);
      }
      else
	KheDoResourcePack(rn, index + 1, depth_limit, soln);
    }
    else
    {
      /* not a depth limit yet, so use a binary tree search */
      if( KheTaskGroupNodeAssign(tgn, rn) )
      {
	KheDoResourcePack(rn, index + 1, depth_limit - 1, soln);
	KheTaskGroupNodeUnAssign(tgn, rn);
	KheDoResourcePack(rn, index + 1, depth_limit - 1, soln);
      }
      else
	KheDoResourcePack(rn, index + 1, depth_limit, soln);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourcePack(KHE_RESOURCE_NODE rn, int depth_limit,              */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Pack rn with a tree search with the given depth limit.                   */
/*                                                                           */
/*****************************************************************************/

static void KheResourcePack(KHE_RESOURCE_NODE rn, int depth_limit,
  KHE_SOLN soln)
{
  KHE_TASK_GROUP_NODE tgn;  int i;
  if( DEBUG2 )
    fprintf(stderr, "  [ KheResourcePack(rn, %d, soln)\n", depth_limit);

  /* sort the available task group nodes and initialize the search vars */
  /* HaArraySort(rn->task_group_nodes,&KheTaskGroupNodeDecreasingDurationCmp);*/
  HaArraySort(rn->task_group_nodes, &KheTaskGroupNodeDecreasingGoodnessCmp);
  HaArrayClear(rn->curr_task_group_nodes);
  rn->curr_goodness = 0;
  HaArrayClear(rn->best_task_group_nodes);
  rn->best_goodness = 0;
  /* rn->best_cost = KheSolnCost(soln); */

  /* do the tree search */
  KheDoResourcePack(rn, 0, 8, soln);

  /* assign the best task group nodes, and delete any completed ones */
  HaArrayForEach(rn->best_task_group_nodes, tgn, i)
  {
    if( !KheTaskGroupNodeAssign(tgn, rn) )
      HnAbort("KheResourcePack internal error");
    if( DEBUG2 )
    {
      fprintf(stderr, "    packing %s with%s: ",
	KheResourceId(rn->resource) == NULL ? "-" : KheResourceId(rn->resource),
        KheTaskGroupUnassignedTaskCount(tgn->task_group)==0 ? " complete" : "");
      KheTaskGroupDebug(tgn->task_group, 1, 0, stderr);
    }
    if( KheTaskGroupUnassignedTaskCount(tgn->task_group) == 0 )
      KheTaskGroupNodeDelete(tgn);
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "    final supply_durn: %d\n", rn->supply_durn);
    /* ***
    HaArrayForEach(rn->task_group_nodes, tgn, i)
    {
      fprintf(stderr, "    ");
      if( KheTaskGroupNodeAssignCheckDebug(tgn, rn, stderr) )
	fprintf(stderr, "still assignable");
      fprintf(stderr, ":\n");
      KheTaskGroupNodeDebug(tgn, 2, 4, stderr);
    }
    *** */
    fprintf(stderr, "  ] KheResourcePack returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePackAssignResources(KHE_TASKING tasking,                 */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign the tasks of tasking using the resource packing method.           */
/*                                                                           */
/*****************************************************************************/

bool KheResourcePackAssignResources(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KHE_PACK_SOLVER ps;  KHE_RESOURCE_NODE rn;  KHE_SOLN soln;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourcePackAssignResources(tasking):\n");

  /* make the pack solver */
  soln = KheTaskingSoln(tasking);
  a = KheSolnArenaBegin(soln, false);
  ps = KhePackSolverMake(tasking, a);
  if( DEBUG1 )
    KhePackSolverDebug(ps, 2, 2, stderr);

  /* visit resources in increasing avail_duration order */
  while( !KhePriQueueEmpty(ps->priqueue) )
  {
    rn = (KHE_RESOURCE_NODE) KhePriQueueDeleteMin(ps->priqueue);
    if( DEBUG1 )
      fprintf(stderr, "  packing %s (demand %d, supply %d):\n",
	KheResourceId(rn->resource) == NULL ? "-" : KheResourceId(rn->resource),
	rn->demand_durn, rn->supply_durn);
    KheResourcePack(rn, 4, KheTaskingSoln(tasking));
  }

  KheSolnArenaEnd(soln, a);
  /* KhePackSolverDelete(ps); */
  if( DEBUG1 )
    fprintf(stderr, "] KheResourcePackAssignResources\n");
  return true;
}

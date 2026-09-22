
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

typedef struct khe_mtask_node_rec *KHE_MTASK_NODE;
typedef HA_ARRAY(KHE_MTASK_NODE) ARRAY_KHE_MTASK_NODE;

typedef struct khe_resource_node_rec *KHE_RESOURCE_NODE;
typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;

typedef struct khe_pack_solver_rec *KHE_PACK_SOLVER;

typedef HA_ARRAY(KHE_RESOURCE_TYPE) ARRAY_KHE_RESOURCE_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_NODE - a node representing one mtask                           */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_node_rec {
  KHE_PACK_SOLVER		pack_solver;		/* enclosing pack s. */
  KHE_MTASK			mtask;			/* tasks to do       */
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
  ARRAY_KHE_MTASK_NODE		mtask_nodes;		/* suitable tasks    */
  int				demand_durn;		/* their total durn  */
  int				supply_durn;		/* available wkload  */
  int				priqueue_index;		/* index in priqueue */

  /* used only while packing the resource */
  ARRAY_KHE_MTASK_NODE		curr_mtask_nodes;	/* current asst      */
  int				curr_goodness;		/* their total good. */
  ARRAY_KHE_MTASK_NODE		best_mtask_nodes;	/* best asst         */
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
  KHE_SOLN			soln;			/* the solution      */
  KHE_RESOURCE_TYPE		resource_type;		/* resource type     */
  /* KHE_TASKING		tasking; */		/* orig. tasking     */
  ARRAY_KHE_MTASK_NODE		mtask_nodes;		/* mtask nodes       */
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;		/* resource nodes    */
  KHE_PRIQUEUE			priqueue;		/* resource priqueue */
};


/*****************************************************************************/
/*                                                                           */
/*  static function declarations                                             */
/*                                                                           */
/*****************************************************************************/

/* mtask nodes */
static void KheMTaskNodeAddResourceNode(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn);
/* ***
static void KheMTaskNodeDeleteResourceNode(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn);
*** */
static bool KheMTaskNodeAssignCheck(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn);
static bool KheMTaskNodeAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn);
static void KheMTaskNodeUnAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn);

/* resource nodes */
static void KheResourceNodeAddMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn);
static void KheResourceNodeDeleteMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn);
static void KheResourceNodeAssignMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn);
static void KheResourceNodeUnAssignMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn);

/* pack solver */
static void KhePackSolverAddMTaskNode(KHE_PACK_SOLVER ps,
  KHE_MTASK_NODE mtn);
static void KhePackSolverDeleteMTaskNode(KHE_PACK_SOLVER ps,
  KHE_MTASK_NODE mtn);
static void KhePackSolverAddResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn);
/* ***
static void KhePackSolverDeleteResourceNode(KHE_PACK_SOLVER ps,
  KHE_RESOURCE_NODE rn);
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "mtask nodes" (private)                                        */
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
/*  int KheMTaskGoodness(KHE_MTASK mt)                                       */
/*                                                                           */
/*  The goodness of assigning a task from mt.                                */
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

static int KheMTaskGoodness(KHE_MTASK mt)
{
  int i, demand, max_demand, junk;  KHE_TASK task;  KHE_MEET meet;
  KHE_COST asst_cost, non_asst_cost;
  max_demand = 0;
  for( i = 0;  i < KheMTaskTaskCount(mt);  i++ )
  {
    task = KheMTaskTask(mt, i, &non_asst_cost, &asst_cost);
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
  return KheTaskGroupTotalDuration(mt);
  *** */

  /* ***
  int excess, res;
  excess = KheResourceGroupResourceCount(KheTaskGroupDomain(mt)) -
    KheTaskGroupTaskCount(mt);
  res = 2 * KheTaskGroupTotalDuration(mt) - excess;
  if( res < 1 )
    res = 1;
  return res;
  *** */

  /* ***
  int i, res;  KHE_TASK task;
  res = 0;
  for( i = 0;  i < KheTaskGroupTaskCount(mt);  i++ )
  {
    task = KheTaskGroupTask(mt, i);
    KheTaskAccumulateGoodness(task, &res);
  }
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_NODE KheMTaskNodeMake(KHE_PACK_SOLVER ps, KHE_MTASK mt)        */
/*                                                                           */
/*  Make one mtask node for ps, representing mt.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_NODE KheMTaskNodeMake(KHE_PACK_SOLVER ps, KHE_MTASK mt)
{
  KHE_MTASK_NODE res;
  HaMake(res, ps->arena);
  res->pack_solver = ps;
  res->mtask = mt;
  res->goodness = KheMTaskGoodness(mt);
  HaArrayInit(res->resource_nodes, ps->arena);
  KhePackSolverAddMTaskNode(ps, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeAddResourceNode(KHE_MTASK_NODE mtn,                     */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Add rn to mtn's list of resource nodes.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskNodeAddResourceNode(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn)
{
  HaArrayAddLast(mtn->resource_nodes, rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeDeleteResourceNode(KHE_MTASK_NODE mtn,                  */
/*    KHE_RESOURCE_NODE rn)                                                  */
/*                                                                           */
/*  Delete rn from mtn.                                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMTaskNodeDeleteResourceNode(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn)
{
  int pos;
  if( !HaArrayContains(mtn->resource_nodes, rn, &pos) )
    HnAbort("KheMTaskNodeDeleteResourceNode internal error");
  HaArrayDeleteAndShift(mtn->resource_nodes, pos);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeDelete(KHE_MTASK_NODE mtn)                              */
/*                                                                           */
/*  Delete mtn.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskNodeDelete(KHE_MTASK_NODE mtn)
{
  KhePackSolverDeleteMTaskNode(mtn->pack_solver, mtn);
  while( HaArrayCount(mtn->resource_nodes) > 0 )
    KheResourceNodeDeleteMTaskNode(
      HaArrayLastAndDelete(mtn->resource_nodes), mtn);
  /* ***
  MArrayFree(mtn->resource_nodes);
  MFree(mtn);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNodeAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)        */
/*                                                                           */
/*  Assign rn to mtn, but only if possible and soln cost does not increase.  */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskNodeAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)
{
  KHE_SOLN soln;  int durn, unmatched_before;  KHE_RESOURCE_GROUP rg;
  KHE_COST cost_before;
  durn = KheMTaskTotalDuration(mtn->mtask);
  rg = KheMTaskDomain(mtn->mtask);
  if( durn > rn->supply_durn || !KheResourceGroupContains(rg, rn->resource) )
    return false;
  soln = mtn->pack_solver->soln;
  unmatched_before = KheSolnMatchingDefectCount(soln);
  cost_before = KheSolnCost(soln);
  if( !KheMTaskMoveResource(mtn->mtask, NULL, rn->resource, true) )
    return false;
  else if( KheSolnMatchingDefectCount(soln) > unmatched_before ||
    KheSolnCost(soln) > cost_before )
  {
    KheMTaskMoveResource(mtn->mtask, rn->resource, NULL, true);
    return false;
  }
  KheResourceNodeAssignMTaskNode(rn, mtn);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNodeAssignDebug(KHE_MTASK_NODE mtn,                         */
/*    KHE_RESOURCE_NODE rn, FILE *fp)                                        */
/*                                                                           */
/*  Like KheMTaskNodeAssign, but also print a debug message on fp saying     */
/*  why the result is false, if it is false.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currrently used
static bool KheMTaskNodeAssignDebug(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn, FILE *fp)
{
  KHE_SOLN soln;  int durn, unmatched_before;  KHE_RESOURCE_GROUP rg;
  KHE_COST cost_before;
  durn = KheTaskGroupTotalDuration(mtn->mtask);
  rg = KheTaskGroupDomain(mtn->mtask);
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
  soln = KheTaskingSoln(mtn->pack_solver->tasking);
  unmatched_before = KheSolnMatchingDefectCount(soln);
  cost_before = KheSolnCost(soln);
  if( !KheTaskGroupAssign(mtn->mtask, rn->resource) )
  {
    fprintf(stderr, "KheTaskGroupAssign returned false");
    return false;
  }
  else if( KheSolnMatchingDefectCount(soln) > unmatched_before )
  {
    fprintf(stderr, "matching (%d > %d)", KheSolnMatchingDefectCount(soln),
      unmatched_before);
    KheTaskGroupUnAssign(mtn->mtask, rn->resource);
    return false;
  }
  else if( KheSolnCost(soln) > cost_before )
  {
    fprintf(stderr, "cost (%.5f > %.5f)", KheCostShow(KheSolnCost(soln)),
      KheCostShow(cost_before));
    KheTaskGroupUnAssign(mtn->mtask, rn->resource);
    return false;
  }
  KheResourceNodeAssignMTaskNode(rn, mtn);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNodeAssignCheck(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)   */
/*                                                                           */
/*  If rn can be assigned to mtn, return true; do not make the assignment.   */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskNodeAssignCheck(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)
{
  if( KheMTaskNodeAssign(mtn, rn) )
  {
    KheMTaskNodeUnAssign(mtn, rn);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNodeAssignCheckDebug(KHE_MTASK_NODE mtn,                    */
/*    KHE_RESOURCE_NODE rn, FILE *fp)                                        */
/*                                                                           */
/*  Like KheMTaskNodeAssignCheck, but also print a message on fp saying      */
/*  why the result is false, if it is false.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not currently used
static bool KheMTaskNodeAssignCheckDebug(KHE_MTASK_NODE mtn,
  KHE_RESOURCE_NODE rn, FILE *fp)
{
  if( KheMTaskNodeAssignDebug(mtn, rn, fp) )
  {
    KheMTaskNodeUnAssign(mtn, rn);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskNodeUnAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)      */
/*                                                                           */
/*  Unassign rn from mtn.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskNodeUnAssign(KHE_MTASK_NODE mtn, KHE_RESOURCE_NODE rn)
{
  KheResourceNodeUnAssignMTaskNode(rn, mtn);
  KheMTaskMoveResource(mtn->mtask, rn->resource, NULL, true);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskNodeDecreasingDurationCmp(const void *t1, const void *t2)    */
/*                                                                           */
/*  Comparison function for sorting an array of task group nodes by          */
/*  decreasing total duration.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static int KheMTaskNodeDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_MTASK_NODE mtn1 = * (KHE_MTASK_NODE *) t1;
  KHE_MTASK_NODE mtn2 = * (KHE_MTASK_NODE *) t2;
  return KheTaskGroupDecreasingDurationCmp(mtn1->mtask, mtn2->mtask);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskNodeDecreasingGoodnessCmp(const void *t1, const void *t2)    */
/*                                                                           */
/*  Comparison function for sorting an array of task group nodes by          */
/*  decreasing total duration.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskNodeDecreasingGoodnessCmp(const void *t1, const void *t2)
{
  KHE_MTASK_NODE mtn1 = * (KHE_MTASK_NODE *) t1;
  KHE_MTASK_NODE mtn2 = * (KHE_MTASK_NODE *) t2;
  return mtn2->goodness - mtn1->goodness;
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
  KHE_RESOURCE_NODE rn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ MTask Node (%d unass tasks, %d resources, %d goodness)\n",
      indent, "", KheMTaskUnassignedTaskCount(mtn->mtask),
      HaArrayCount(mtn->resource_nodes), mtn->goodness);
    if( verbosity > 1 )
    {
      KheMTaskDebug(mtn->mtask, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  resources: ", indent, "");
      HaArrayForEach(mtn->resource_nodes, rn, i)
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
  HaArrayInit(res->mtask_nodes, ps->arena);
  res->demand_durn = 0;
  res->supply_durn = KheResourceSupplyDuration(ps->soln, r);
  res->priqueue_index = 0;

  /* used only while packing the resource */
  HaArrayInit(res->curr_mtask_nodes, ps->arena);
  res->curr_goodness = 0;
  HaArrayInit(res->best_mtask_nodes, ps->arena);
  /* res->best_cost = 0; */
  res->best_goodness = 0;
  KhePackSolverAddResourceNode(ps, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAddMTaskNode(KHE_RESOURCE_NODE rn,                   */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Add mtn to rn's list of assignable task group nodes.                     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAddMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn)
{
  HaArrayAddLast(rn->mtask_nodes, mtn);
  rn->demand_durn += KheMTaskTotalDuration(mtn->mtask);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDeleteMTaskNode(KHE_RESOURCE_NODE rn,                */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Delete mtn from rn's list of assignable task group nodes.                */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDeleteMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn)
{
  int pos;
  rn->demand_durn -= KheMTaskTotalDuration(mtn->mtask);
  if( !HaArrayContains(rn->mtask_nodes, mtn, &pos) )
    HnAbort("KheResourceNodeDeleteMTaskNode internal error");
  HaArrayDeleteAndShift(rn->mtask_nodes, pos);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeAssignMTaskNode(KHE_RESOURCE_NODE rn,                */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Update rn to assign mtn to it.  This function should not be called       */
/*  directly; call KheMTaskNodeAssignResourceNode instead.                   */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeAssignMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn)
{
  HaArrayAddLast(rn->curr_mtask_nodes, mtn);
  rn->curr_goodness += mtn->goodness;
  rn->supply_durn -= KheMTaskTotalDuration(mtn->mtask);
  if( rn->priqueue_index != 0 )
    KhePriQueueNotifyKeyChange(rn->pack_solver->priqueue, (void *) rn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeUnAssignMTaskNode(KHE_RESOURCE_NODE rn,              */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Update rn to unassign mtn from it.  This function should not be called   */
/*  directly; call KheMTaskNodeUnAssignResourceNode instead.                 */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeUnAssignMTaskNode(KHE_RESOURCE_NODE rn,
  KHE_MTASK_NODE mtn)
{
  int i;
  rn->supply_durn += KheMTaskTotalDuration(mtn->mtask);
  for( i = HaArrayCount(rn->curr_mtask_nodes) - 1;  i >= 0;  i-- )
    if( HaArray(rn->curr_mtask_nodes, i) == mtn )
      break;
  HnAssert(i >= 0, "KheResourceNodeUnAssignMTaskNode internal error");
  HaArrayDeleteAndShift(rn->curr_mtask_nodes, i);
  rn->curr_goodness -= mtn->goodness;
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
  while( HaArrayCount(rn->mtask_nodes) > 0 )
    KheMTaskNodeDeleteResourceNode(
      HaArrayLastAndDelete(rn->mtask_nodes), rn);
  ** ***
  MArrayFree(rn->mtask_nodes);
  MArrayFree(rn->curr_mtask_nodes);
  MArrayFree(rn->best_mtask_nodes);
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
  KHE_MTASK_NODE mtn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp,
      "%*s[ Resource Node %s (%d task group nodes, demand %d, supply %d)",
      indent, "",
      KheResourceId(rn->resource) == NULL ? "-" : KheResourceId(rn->resource),
      HaArrayCount(rn->mtask_nodes), rn->demand_durn, rn->supply_durn);
    if( verbosity >= 3 )
    {
      fprintf(fp, "\n");
      HaArrayForEach(rn->mtask_nodes, mtn, i)
	KheMTaskDebug(mtn->mtask, 2, indent + 2, fp);
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
/*  KHE_PACK_SOLVER KhePackSolverMake(KHE_TASKING tasking,                   */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a pack solver containing one mtask node for each incompletely       */
/*  assigned mtask of tasking, and one resource node for each resource       */
/*  capable of being assigned to any of those mtasks, including adding       */
/*  the appropriate links between the two.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_PACK_SOLVER KhePackSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_PACK_SOLVER res;  int i, j, k, pos;  KHE_FRAME days_frame;
  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;
  KHE_MTASK_NODE mtn;  KHE_RESOURCE_NODE rn;  KHE_RESOURCE r;
  ARRAY_KHE_RESOURCE_TYPE resource_types;
  /* KHE_EVENT_TIMETABLE_MONITOR etm; */

  /* build the pack solver object proper */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  HaArrayInit(res->mtask_nodes, a);
  HaArrayInit(res->resource_nodes, a);
  res->priqueue = KhePriQueueMake(&KheResourceNodeKey, &KheResourceNodeIndex,
    &KheResourceNodeSetIndex, a);

  /* one mtask node for each incompletely assigned task group of tasking */
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  /* ***
  e tm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  *** */
  mtf = KheMTaskFinderMake(soln, rt, days_frame, /* etm, */ true, a);
  /* task_groups = KheTaskGroupsMakeFromTasking(tasking); */
  for( i = 0;  i < KheMTaskFinderMTaskCount(mtf);  i++ )
  {
    mt = KheMTaskFinderMTask(mtf, i);
    if( KheMTaskUnassignedTaskCount(mt) > 0 )
      KheMTaskNodeMake(res, mt);
  }
  HaArraySort(res->mtask_nodes, &KheMTaskNodeDecreasingGoodnessCmp);

  /* find the set of all resource types within the task groups */
  HaArrayInit(resource_types, a);
  HaArrayForEach(res->mtask_nodes, mtn, i)
  {
    rt = KheResourceGroupResourceType(KheMTaskDomain(mtn->mtask));
    if( !HaArrayContains(resource_types, rt, &pos) )
      HaArrayAddLast(resource_types, rt);
  }

  /* one resource node for each resource of each resource type, plus edges */
  HaArrayForEach(resource_types, rt, i)
    for( j = 0;  j < KheResourceTypeResourceCount(rt);  j++ )
    {
      r = KheResourceTypeResource(rt, j);
      rn = KheResourceNodeMake(res, r);
      HaArrayForEach(res->mtask_nodes, mtn, k)
	if( KheMTaskNodeAssignCheck(mtn, rn) )
	{
	  KheMTaskNodeAddResourceNode(mtn, rn);
	  KheResourceNodeAddMTaskNode(rn, mtn);
	}
      KhePriQueueInsert(res->priqueue, (void *) rn);
    }
  HaArrayFree(resource_types);

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverAddMTaskNode(KHE_PACK_SOLVER ps,                       */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Add mtn to ps.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverAddMTaskNode(KHE_PACK_SOLVER ps,
  KHE_MTASK_NODE mtn)
{
  HaArrayAddLast(ps->mtask_nodes, mtn);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePackSolverDeleteMTaskNode(KHE_PACK_SOLVER ps,                    */
/*    KHE_MTASK_NODE mtn)                                                    */
/*                                                                           */
/*  Delete mtn from ps.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KhePackSolverDeleteMTaskNode(KHE_PACK_SOLVER ps,
  KHE_MTASK_NODE mtn)
{
  int pos;
  if( !HaArrayContains(ps->mtask_nodes, mtn, &pos) )
    HnAbort("KhePackSolverDeleteMTaskNode internal error");
  HaArrayDeleteAndShift(ps->mtask_nodes, pos);
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
  while( HaArrayCount(ps->mtask_nodes) > 0 )
    KheMTaskNodeDelete(HaArrayLast(ps->mtask_nodes));
  while( HaArrayCount(ps->resource_nodes) > 0 )
    KheResourceNodeDelete(HaArrayLast(ps->resource_nodes));
  MArrayFree(ps->mtask_nodes);
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
  KHE_MTASK_NODE mtn;  KHE_RESOURCE_NODE rn;  int i;
  if( verbosity >= 1 && indent >= 0 )
  {
    fprintf(fp, "%*s[ Pack Solver (%d task group nodes, %d resource nodes)\n",
      indent, "", HaArrayCount(ps->mtask_nodes),
      HaArrayCount(ps->resource_nodes));
    HaArrayForEach(ps->mtask_nodes, mtn, i)
      KheMTaskNodeDebug(mtn, verbosity, indent + 2, fp);
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
/*  Pack mtask nodes rn->mtask_nodes[index..] into rn.  If depth_limit > 0,  */
/*  explore all possibilities with binary tree search; if depth_limit == 0,  */
/*  use a simple linear heuristic, to save time.                             */
/*                                                                           */
/*****************************************************************************/

static void KheDoResourcePack(KHE_RESOURCE_NODE rn, int index,
  int depth_limit, KHE_SOLN soln)
{
  KHE_MTASK_NODE mtn;  int i;

  /* if current state is best so far, remember it */
  if( rn->curr_goodness > rn->best_goodness )
  /* if( KheSolnCost(soln) < rn->best_cost ) */
  {
    HaArrayClear(rn->best_mtask_nodes);
    HaArrayAppend(rn->best_mtask_nodes, rn->curr_mtask_nodes, i);
    /* rn->best_cost = KheSolnCost(soln); */
    rn->best_goodness = rn->curr_goodness;
  }

  /* if there is any prospect of more assignments, try one and recurse */
  if( index < HaArrayCount(rn->mtask_nodes) && rn->supply_durn > 0 )
  {
    mtn = HaArray(rn->mtask_nodes, index);
    if( depth_limit == 0 )
    {
      /* at depth limit, so use a simple linear heuristic */
      if( KheMTaskNodeAssign(mtn, rn) )
      {
	KheDoResourcePack(rn, index + 1, depth_limit, soln);
	KheMTaskNodeUnAssign(mtn, rn);
      }
      else
	KheDoResourcePack(rn, index + 1, depth_limit, soln);
    }
    else
    {
      /* not a depth limit yet, so use a binary tree search */
      if( KheMTaskNodeAssign(mtn, rn) )
      {
	KheDoResourcePack(rn, index + 1, depth_limit - 1, soln);
	KheMTaskNodeUnAssign(mtn, rn);
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
  KHE_MTASK_NODE mtn;  int i;
  if( DEBUG2 )
    fprintf(stderr, "  [ KheResourcePack(rn, %d, soln)\n", depth_limit);

  /* sort the available task group nodes and initialize the search vars */
  /* HaArraySort(rn->mtask_nodes,&KheMTaskNodeDecreasingDurationCmp);*/
  HaArraySort(rn->mtask_nodes, &KheMTaskNodeDecreasingGoodnessCmp);
  HaArrayClear(rn->curr_mtask_nodes);
  rn->curr_goodness = 0;
  HaArrayClear(rn->best_mtask_nodes);
  rn->best_goodness = 0;
  /* rn->best_cost = KheSolnCost(soln); */

  /* do the tree search */
  KheDoResourcePack(rn, 0, 8, soln);

  /* assign the best task group nodes, and delete any completed ones */
  HaArrayForEach(rn->best_mtask_nodes, mtn, i)
  {
    if( !KheMTaskNodeAssign(mtn, rn) )
      HnAbort("KheResourcePack internal error");
    if( DEBUG2 )
    {
      fprintf(stderr, "    packing %s with%s: ",
	KheResourceId(rn->resource) == NULL ? "-" : KheResourceId(rn->resource),
        KheMTaskUnassignedTaskCount(mtn->mtask)==0 ? " complete" : "");
      KheMTaskDebug(mtn->mtask, 1, 0, stderr);
    }
    if( KheMTaskUnassignedTaskCount(mtn->mtask) == 0 )
      KheMTaskNodeDelete(mtn);
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "    final supply_durn: %d\n", rn->supply_durn);
    /* ***
    HaArrayForEach(rn->mtask_nodes, mtn, i)
    {
      fprintf(stderr, "    ");
      if( KheMTaskNodeAssignCheckDebug(mtn, rn, stderr) )
	fprintf(stderr, "still assignable");
      fprintf(stderr, ":\n");
      KheMTaskNodeDebug(mtn, 2, 4, stderr);
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

bool KheResourcePackAssignResources(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  KHE_PACK_SOLVER ps;  KHE_RESOURCE_NODE rn;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourcePackAssignResources(tasking):\n");

  /* make the pack solver */
  a = KheSolnArenaBegin(soln);
  ps = KhePackSolverMake(soln, rt, options, a);
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
    KheResourcePack(rn, 4, soln);
  }

  KheSolnArenaEnd(soln, a);
  /* KhePackSolverDelete(ps); */
  if( DEBUG1 )
    fprintf(stderr, "] KheResourcePackAssignResources\n");
  return true;
}

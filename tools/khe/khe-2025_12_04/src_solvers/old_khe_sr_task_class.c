
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
/*  FILE:         khe_sr_task_class.c                                        */
/*  DESCRIPTION:  Task classes                                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST - one task with its associated costs                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_and_cost_rec {
  KHE_TASK			task;			/* a task            */
  KHE_COST			asst_cost;		/* its asst cost     */
  KHE_COST			non_asst_cost;		/* its non-asst cost */
} *KHE_TASK_AND_COST;

typedef HA_ARRAY(KHE_TASK_AND_COST) ARRAY_KHE_TASK_AND_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND - classifies monitors into one of three kinds           */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_MONITOR_MULTI_TASK,
  KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT,
  KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT
} KHE_MONITOR_KIND;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG - what similarity of atomic tasks is based on        */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_atomic_task_sig_rec {
  KHE_TIME			start_time;
  int				duration;
  float				workload_per_time;
  ARRAY_KHE_MONITOR		single_task_monitors;
  ARRAY_KHE_MONITOR		multi_task_monitors;
} *KHE_ATOMIC_TASK_SIG;

typedef HA_ARRAY(KHE_ATOMIC_TASK_SIG) ARRAY_KHE_ATOMIC_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_SIG - what similarity of general tasks is based on              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_sig_rec {
  KHE_RESOURCE_GROUP		root_domain;
  ARRAY_KHE_ATOMIC_TASK_SIG	atomic_sigs;
} *KHE_TASK_SIG;

typedef HA_ARRAY(KHE_TASK_SIG) ARRAY_KHE_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASS_TIME - what a class is doing at one time                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_class_time_rec {
  KHE_TIME			time;			/* the time          */
  float				workload_per_time;	/* workload at time  */
} *KHE_CLASS_TIME;

typedef HA_ARRAY(KHE_CLASS_TIME) ARRAY_KHE_CLASS_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS                                                           */
/*                                                                           */
/*****************************************************************************/
/* typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK; */
/* typedef HA_ARRAY(KHE_TIME) ARRAY_KHE_TIME; */

struct khe_task_class_rec {
  KHE_TASK_CLASS_SOLVER		solver;
  KHE_TASK_SIG			sig;
  ARRAY_KHE_TASK_AND_COST	tasks_and_costs;
  bool				no_overlap;
  bool				no_gaps;
  int				first_day_index;
  int				last_day_index;
  /* ARRAY_KHE_CLASS_TIME	class_times; */
  ARRAY_KHE_CLASS_TIME		class_times_by_day;
};

typedef HA_ARRAY(KHE_TASK_CLASS) ARRAY_KHE_TASK_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASSES_AT_TIME - classes beginning at a given time                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_classes_at_time_rec {
  KHE_TIME			time;			/* the time          */
  ARRAY_KHE_TASK_CLASS		classes;		/* the classes       */
} *KHE_CLASSES_AT_TIME;

typedef HA_ARRAY(KHE_CLASSES_AT_TIME) ARRAY_KHE_CLASSES_AT_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS_SOLVER                                                    */
/*                                                                           */
/*****************************************************************************/

struct khe_task_class_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;
  ARRAY_KHE_TASK_CLASS		classes;		/* the classes       */
  ARRAY_KHE_CLASSES_AT_TIME	classes_at_time;	/* classes at time   */
  ARRAY_KHE_TASK_AND_COST	free_tasks_and_costs;	/* free tasks & costs*/
  ARRAY_KHE_ATOMIC_TASK_SIG	free_atomic_task_sigs;	/* free atomic sigs  */
  ARRAY_KHE_TASK_SIG		free_task_sigs;		/* free task sigs    */
  ARRAY_KHE_CLASS_TIME		free_class_times;	/* free class times  */
  ARRAY_KHE_TASK_CLASS		free_classes;		/* free classes      */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_AND_COST"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task, KHE_COST asst_cost,  */
/*    KHE_COST non_asst_cost, KHE_TASK_CLASS_SOLVER tcs)                     */
/*                                                                           */
/*  Make and return a new task and cost object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task, KHE_COST asst_cost,
  KHE_COST non_asst_cost, KHE_TASK_CLASS_SOLVER tcs)
{
  KHE_TASK_AND_COST res;
  if( HaArrayCount(tcs->free_tasks_and_costs) > 0 )
    res = HaArrayLastAndDelete(tcs->free_tasks_and_costs);
  else
    HaMake(res, tcs->arena);
  res->task = task;
  res->asst_cost = asst_cost;
  res->non_asst_cost = non_asst_cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAndCostDelete(KHE_TASK_AND_COST tac,                         */
/*    KHE_TASK_CLASS_SOLVER tcs)                                             */
/*                                                                           */
/*  Delete tac, by appending it to the free list in tcs.                     */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheTaskAndCostDelete(KHE_TASK_AND_COST tac,
  KHE_TASK_CLASS_SOLVER tcs)
{
  HaArrayAddLast(tcs->free_tasks_and_costs, tac);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ATOMIC_TASK_SIG"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceTasksHaveSameProperRoot(KHE_EVENT_RESOURCE er,      */
/*    KHE_SOLN soln, KHE_TASK *proper_root_task)                             */
/*                                                                           */
/*  If the tasks derived from er in soln have the same proper root task,     */
/*  return true with *proper_root_task set to that proper root task.         */
/*  Otherwise return false with *proper_root_task set to NULL.               */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceTasksHaveSameProperRoot(KHE_EVENT_RESOURCE er,
  KHE_SOLN soln, KHE_TASK *proper_root_task)
{
  KHE_TASK task, prt;  int i;
  prt = KheTaskProperRoot(KheEventResourceTask(soln, er, 0));
  for( i = 1;  i < KheEventResourceTaskCount(soln, er);  i++ )
  {
    task = KheTaskProperRoot(KheEventResourceTask(soln, er, i));
    if( task != prt )
      return *proper_root_task = NULL, false;
  }
  return *proper_root_task = prt, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_SOLN soln)               */
/*                                                                           */
/*  Return true when the tasks monitored by asam have the same proper root.  */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_SOLN soln)
{
  KHE_TASK prt, task;  int i, eg_index, count;  KHE_EVENT_RESOURCE er;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;
  asac = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  eg_index = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  prt = NULL;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(asac, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(asac, eg_index, i);
    if( !KheEventResourceTasksHaveSameProperRoot(er, soln, &task) )
      return false;
    if( prt == NULL )
      prt = task;
    else if( prt != task )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorTasksHaveSameProperRoot(                    */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_SOLN soln)                        */
/*                                                                           */
/*  Return true when the tasks monitored by lrm have the same proper root.   */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorTasksHaveSameProperRoot(
  KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_SOLN soln)
{
  KHE_TASK prt, task;  int i, eg_index, count;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  prt = NULL;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    if( !KheEventResourceTasksHaveSameProperRoot(er, soln, &task) )
      return false;
    if( prt == NULL )
      prt = task;
    else if( prt != task )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorIsSingleTaskMonitor(KHE_MONITOR m)                        */
/*                                                                           */
/*  Return true if m is a single-task monitor.                               */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static bool KheMonitorIsSingleTaskMonitor(KHE_MONITOR m)
{
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_CONSTRAINT c;  KHE_TASK junk;
  soln = KheMonitorSoln(m);
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      ** true if linear cost function, or all tasks have the same proper root **
      c = KheMonito rConstraint(m);
      if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
	return true;
      arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
      er = KheAssignResourceMonitorEventResource(arm);
      return KheEventResourceTasksHaveSameProperRoot(er, soln, &junk);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      ** true if linear cost function, or all tasks have the same proper root **
      c = KheMonitor Constraint(m);
      if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
	return true;
      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      er = KhePreferResourcesMonitorEventResource(prm);
      return KheEventResourceTasksHaveSameProperRoot(er, soln, &junk);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      ** true if all tasks have the same proper root **
      asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
      return KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(asam, soln);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      ** true if all tasks have the same proper root **
      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      return KheLimitResourcesMonitorTasksHaveSameProperRoot(lrm, soln);

    default:

      HnAbort("KheMonitorIsSingleTaskMonitor: unexpected monitor tag (%s)",
	KheMonitorTagShow(KheMonitorTag(m)));
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheFi ndCost(KHE_CONSTRAINT c, int dev)                         */
/*                                                                           */
/*  Find the cost of c when the deviation is dev.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_COST KheFi ndCost(KHE_CONSTRAINT c, int dev)
{
  switch( KheConstrai ntCostFunction(c) )
  {
    case KHE_STEP_COST_FUNCTION:

      return dev > 0 ? KheConst raintCombinedWeight(c) : 0;

    case KHE_LINEAR_COST_FUNCTION:

      return KheConstrai ntCombinedWeight(c) * dev;

    case KHE_QUADRATIC_COST_FUNCTION:

      return KheConstra intCombinedWeight(c) * dev * dev;

    default:

      HnAbort("KheFindCost: unknown cost function (%d)",
	KheConstraintC ostFunction(c));
      return 0;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,                 */
/*    KHE_COST *asst_cost, KHE_COST *non_asst_cost)                          */
/*                                                                           */
/*  Return m's kind.  If m is single-task resource-independent, also add     */
/*  the cost of assignment of a task of duration durn to *asst_cost, and     */
/*  the cost of non-assignment of a task of duration durn to *non_asst_cost. */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,
  KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_RESOURCE_GROUP rg;  int count /* , min_limit, max_limit */;
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_RESOURCE er;  KHE_TASK junk;
  KHE_CONSTRAINT c;  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_SOLN soln;
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  soln = KheMonitorSoln(m);
  c = KheMonitorConstraint(m);
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
      er = KheAssignResourceMonitorEventResource(arm);
      if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
      {
	/* single-task resource-independent with non-assignment cost */
	*non_asst_cost += KheFindCost(c, durn);
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else if( KheEventResourceTasksHaveSameProperRoot(er, soln, &junk) )
      {
	/* single-task resource-independent without non-assignment cost */
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else
      {
	/* multi-task */
	return KHE_MONITOR_MULTI_TASK;
      }
      /* ***
      if( KheMonitorCostFunction(m) != KHE_LINEAR_COST_FUNCTION &&
          !KheEventResourceTasksHaveSameProperRoot(er, soln, &junk) )
      {
	** multi-task **
	return KHE_MONITOR_MULTI_TASK;
      }
      else
      {
	** single-task resource-independent with non-assignment cost **
	*non_asst_cost += KheFindCost(c, durn);
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      *** */

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      prc = KhePreferResourcesMonitorConstraint(prm);
      rg = KhePreferResourcesMonitorDomain(prm);
      rt = KheResourceGroupResourceType(rg);
      count = KheResourceGroupResourceCount(rg);
      er = KhePreferResourcesMonitorEventResource(prm);
      if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
      {
	if( count == 0 )
	{
	  /* single-task resource-independent with assignment cost */
	  *asst_cost += KheFindCost(c, durn);
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
	}
	else if( count == KheResourceTypeResourceCount(rt) )
	{
	  /* single-task resource-independent, with no cost for assignment */
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* single-task resource-dependent */
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT;
	}
      }
      else if( KheEventResourceTasksHaveSameProperRoot(er, soln, &junk) )
      {
	if( count == 0 )
	{
	  /* single-task resource-independent with assignment cost */
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
	}
	else if( count == KheResourceTypeResourceCount(rt) )
	{
	  /* single-task resource-independent, with no cost for assignment */
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* single-task resource-dependent */
	  return KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT;
	}
      }
      else
      {
	/* multi-task */
	return KHE_MONITOR_MULTI_TASK;
      }
      /* ***
      if( KheMonitorCostFunction(m) != KHE_LINEAR_COST_FUNCTION &&
          !KheEventResourceTasksHaveSameProperRoot(er, soln, &junk) )
      {
	** multi-task **
	return KHE_MONITOR_MULTI_TASK;
      }
      else if( count == 0 )
      {
	** single-task resource-independent with assignment cost **
	*asst_cost += KheFindCost(c, durn);
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else if( count == KheResourceTypeResourceCount(rt) )
      {
	** single-task resource-independent, with no cost for assignment **
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else
      {
	** single-task resource-dependent **
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT;
      }
      *** */

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
      if( !KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(asam, soln) )
      {
	/* multi-task */
	return KHE_MONITOR_MULTI_TASK;
      }
      else
      {
	/* single-task resource-independent, with no cost for assignment */
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      lrc = KheLimitResourcesMonitorConstraint(lrm);
      rg = KheLimitResourcesConstraintDomain(lrc);
      rt = KheResourceGroupResourceType(rg);
      count = KheResourceGroupResourceCount(rg);
      if( !KheLimitResourcesMonitorTasksHaveSameProperRoot(lrm, soln) )
      {
	/* multi-task */
	return KHE_MONITOR_MULTI_TASK;
      }
      else if( count == 0 )
      {
	/* single-task resource-independent, with no cost for assignment */
	/* not attempting to work out the cost here */
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else if( count == KheResourceTypeResourceCount(rt) )
      {
	/* single-task resource-independent */
	/* not attempting to work out the cost here */
	/* ***
        ** if min_limit > 0, there is a cost for non-assignment **
	min_limit = KheLimitResourcesConstraintMinimum(lrc);
	if( min_limit > 0 )
	  *non_asst_cost += KheFindCost(c, min_limit);

	** if durn > max_limit, there is a cost for assignment **
        max_limit = KheLimitResourcesConstraintMaximum(lrc);
	if( durn > max_limit )
	  *asst_cost += KheFindCost(c, durn - max_limit);

	** if durn < min_limit, there is a cost for assignment **
	if( durn < min_limit )
	  *asst_cost += KheFindCost(c, min_limit - durn);
	*** */
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT;
      }
      else
      {
	/* single-task resource-dependent */
	return KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT;
      }

    default:

      HnAbort("KheMonitorKind: unexpected monitor tag (%s)",
	KheMonitorTagShow(KheMonitorTag(m)));
      return KHE_MONITOR_MULTI_TASK;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig, KHE_MONITOR m, */
/*    int durn, KHE_COST *asst_cost, KHE_COST *non_asst_cost)                */
/*                                                                           */
/*  Add m to asig, or add its assignment cost (for a task of duration durn)  */
/*  to *asst_cost and its non-assignment cost to *non_asst_cost.             */
/*                                                                           */
/*****************************************************************************/

static void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig, KHE_MONITOR m,
  int durn, KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  switch( KheMonitorKind(m, durn, asst_cost, non_asst_cost) )
  {
    case KHE_MONITOR_MULTI_TASK:

      HaArrayAddLast(asig->multi_task_monitors, m);
      break;

    case KHE_MONITOR_SINGLE_TASK_RESOURCE_INDEPENDENT:

      /* nothing to do here; assignment costs already added */
      break;

    case KHE_MONITOR_SINGLE_TASK_RESOURCE_DEPENDENT:

      HaArrayAddLast(asig->single_task_monitors, m);
      break;

    default:

      HnAbort("KheAtomicTaskSigAddMonitor: illegal monitor kind");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMultiTaskMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)          */
/*                                                                           */
/*  Typed comparison function for sorting an array of monitors into the      */
/*  order that multi-task monitors need to be sorted into.                   */
/*                                                                           */
/*  Implementation note.  The multi-task monitors of two atomic task         */
/*  signatures need to be the same monitors for similarity, so it is         */
/*  sufficient here to sort them by their unique index numbers.              */
/*                                                                           */
/*****************************************************************************/

static int KheMultiTaskMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMultiTaskMonitorUntypedCmp(const void *t1, const void *t2)        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of monitors into the    */
/*  order that multi-task monitors need to be sorted into.                   */
/*                                                                           */
/*****************************************************************************/

static int KheMultiTaskMonitorUntypedCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheMultiTaskMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSingleTaskMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)         */
/*                                                                           */
/*  Typed comparison function for sorting an array of monitors into the      */
/*  order that resource-dependent single-task monitors need to be in.        */
/*                                                                           */
/*  Implementation note.  The resource-dependent single-task monitors of     */
/*  two atomic task signatures need to have equal attributes for similarity, */
/*  so they have to be sorted by those attributes.                           */
/*                                                                           */
/*****************************************************************************/

static int KheSingleTaskMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  KHE_PREFER_RESOURCES_CONSTRAINT prc1, prc2;  KHE_RESOURCE_GROUP rg1, rg2;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc1, lrc2;  int cmp;  KHE_CONSTRAINT c1, c2;
  KHE_PREFER_RESOURCES_MONITOR prm1, prm2;

  /* shortcut when m1 and m2 are the same monitor */
  if( m1 == m2 )
    return 0;

  /* monitor type attributes must be equal */
  cmp = KheMonitorTag(m1) - KheMonitorTag(m2);
  if( cmp != 0 ) return cmp;

  /* hardness, weight, and cost function attributes must be equal */
  c1 = KheMonitorConstraint(m1);
  c2 = KheMonitorConstraint(m2);
  cmp = KheCostCmp(KheMonitorCombinedWeight(m1), KheMonitorCombinedWeight(m2));
  if( cmp != 0 ) return cmp;
  cmp = (int) KheMonitorCostFunction(m1) - (int) KheMonitorCostFunction(m2);
  if( cmp != 0 ) return cmp;

  switch( KheMonitorTag(m1) )
  {
    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      /* preferred resources must be equal */
      prm1 = (KHE_PREFER_RESOURCES_MONITOR) m1;
      prm2 = (KHE_PREFER_RESOURCES_MONITOR) m2;
      prc1 = (KHE_PREFER_RESOURCES_CONSTRAINT) c1;
      prc2 = (KHE_PREFER_RESOURCES_CONSTRAINT) c2;
      rg1 = KhePreferResourcesMonitorDomain(prm1);
      rg2 = KhePreferResourcesMonitorDomain(prm2);
      return KheResourceGroupTypedCmp(rg1, rg2);

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      /* min limit, max limit, and preferred resources must be equal */
      lrc1 = (KHE_LIMIT_RESOURCES_CONSTRAINT) c1;
      lrc2 = (KHE_LIMIT_RESOURCES_CONSTRAINT) c2;
      cmp = KheLimitResourcesConstraintMinimum(lrc1) -
	KheLimitResourcesConstraintMinimum(lrc2);
      if( cmp != 0 ) return cmp;
      cmp = KheLimitResourcesConstraintMaximum(lrc1) -
	KheLimitResourcesConstraintMaximum(lrc2);
      if( cmp != 0 ) return cmp;
      rg1 = KheLimitResourcesConstraintDomain(lrc1);
      rg2 = KheLimitResourcesConstraintDomain(lrc2);
      return KheResourceGroupTypedCmp(rg1, rg2);

    default:

      HnAbort("KheSingleTaskMonitorTypedCmp: unexpected constraint tag (%d)",
        KheConstraintTag(c1));
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSingleTaskMonitorUntypedCmp(const void *t1, const void *t2)       */
/*                                                                           */
/*  Untyped comparison function for sorting an array of monitors into the    */
/*  order that single-task monitors need to be sorted into.                  */
/*                                                                           */
/*****************************************************************************/

static int KheSingleTaskMonitorUntypedCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheSingleTaskMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_TASK_CLASS_SOLVER tcs,      */
/*    KHE_TASK task, KHE_COST *asst_cost, KHE_COST *non_asst_cost)           */
/*                                                                           */
/*  Assuming that task lies in a meet with an assigned time, make and        */
/*  return a new atomic signature object for task.  Also add task's cost     */
/*  of assignment to *asst_cost, and of non-assignment to *non_asst_cost.    */
/*                                                                           */
/*  NB There is no KheAtomicTaskSigDelete, because KheTaskSigDelete uses     */
/*  HaArrayAppend to free all its atomic task sigs at once.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_TASK_CLASS_SOLVER tcs,
  KHE_TASK task, KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  KHE_ATOMIC_TASK_SIG res;  KHE_MEET meet;  KHE_EVENT_RESOURCE er;
  KHE_MONITOR m;  int i;

  /* get the object */
  if( HaArrayCount(tcs->free_atomic_task_sigs) > 0 )
  {
    res = HaArrayLastAndDelete(tcs->free_atomic_task_sigs);
    HaArrayClear(res->single_task_monitors);
    HaArrayClear(res->multi_task_monitors);
  }
  else
  {
    HaMake(res, tcs->arena);
    HaArrayInit(res->single_task_monitors, tcs->arena);
    HaArrayInit(res->multi_task_monitors, tcs->arena);
  }

  /* initialize start_time, duration, and workload per time */
  meet = KheTaskMeet(task);
  res->start_time = KheMeetAsstTime(meet);
  res->duration = KheMeetDuration(meet);
  res->workload_per_time = KheTaskWorkloadPerTime(task);

  /* initialize monitors */
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    for( i = 0;  i < KheSolnEventResourceMonitorCount(tcs->soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(tcs->soln, er, i);
      if( KheMonitorAttachedToSoln(m) && KheMonitorCombinedWeight(m) > 0 )
	KheAtomicTaskSigAddMonitor(res, m, res->duration, asst_cost,
	  non_asst_cost);
    }
  }

  /* sort monitors */
  HaArraySort(res->single_task_monitors, &KheSingleTaskMonitorUntypedCmp);
  HaArraySort(res->multi_task_monitors, &KheMultiTaskMonitorUntypedCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,                  */
/*    KHE_ATOMIC_TASK_SIG asig2)                                             */
/*                                                                           */
/*  Typed comparison function for sorting an array of atomic task sigs       */
/*  by increasing start time, breaking ties by increasing duration.          */
/*                                                                           */
/*****************************************************************************/

static int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,
  KHE_ATOMIC_TASK_SIG asig2)
{
  int cmp;
  cmp = KheTimeIndex(asig1->start_time) - KheTimeIndex(asig2->start_time);
  if( cmp != 0 )  return cmp;
  return asig1->duration - asig2->duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAtomicTaskSigUntypedCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Untyped comparison function for sorting an array of atomic task sigs     */
/*  by increasing start time, breaking ties by increasing duration.          */
/*                                                                           */
/*****************************************************************************/

static int KheAtomicTaskSigUntypedCmp(const void *t1, const void *t2)
{
  KHE_ATOMIC_TASK_SIG asig1 = * (KHE_ATOMIC_TASK_SIG *) t1;
  KHE_ATOMIC_TASK_SIG asig2 = * (KHE_ATOMIC_TASK_SIG *) t2;
  return KheAtomicTaskSigTypedCmp(asig1, asig2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicTaskSigSimilar(KHE_ATOMIC_TASK_SIG asig1,                  */
/*    KHE_ATOMIC_TASK_SIG asig2)                                             */
/*                                                                           */
/*  Return true if asig1 and asig2 are similar.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheAtomicTaskSigSimilar(KHE_ATOMIC_TASK_SIG asig1,
  KHE_ATOMIC_TASK_SIG asig2)
{
  int i, count;  KHE_MONITOR m1, m2;

  /* start_time, duration, and workload_per_time must be equal */
  if( asig1->start_time != asig2->start_time )
    return false;
  if( asig1->duration != asig2->duration )
    return false;
  if( asig1->workload_per_time != asig2->workload_per_time )
    return false;

  /* corresponding single-task monitors must have the same attributes */
  count = HaArrayCount(asig1->single_task_monitors);
  if( count != HaArrayCount(asig2->single_task_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->single_task_monitors, i);
    m2 = HaArray(asig2->single_task_monitors, i);
    if( KheSingleTaskMonitorTypedCmp(m1, m2) != 0 )
      return false;
  }

  /* corresponding multi-task monitors must be identical */
  count = HaArrayCount(asig1->multi_task_monitors);
  if( count != HaArrayCount(asig2->multi_task_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->multi_task_monitors, i);
    m2 = HaArray(asig2->multi_task_monitors, i);
    if( m1 != m2 )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTaskSigDebug(KHE_ATOMIC_TASK_SIG asig, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of asig onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheAtomicTaskSigDebug(KHE_ATOMIC_TASK_SIG asig, int verbosity,
  int indent, FILE *fp)
{
  KHE_MONITOR m;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ AtomicTaskSig(%s, durn %d, wkld %.2f)\n", indent, "",
      KheTimeId(asig->start_time), asig->duration, asig->workload_per_time);
    if( HaArrayCount(asig->single_task_monitors) > 0 )
    {
      fprintf(fp, "%*s  single-task monitors:\n", indent, "");
      HaArrayForEach(asig->single_task_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    if( HaArrayCount(asig->multi_task_monitors) > 0 )
    {
      fprintf(fp, "%*s  multi-task monitors:\n", indent, "");
      HaArrayForEach(asig->multi_task_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "AtomicTaskSig(%s, %d, %.2f, %d single, %d multi)",
      KheTimeId(asig->start_time), asig->duration, asig->workload_per_time,
      HaArrayCount(asig->single_task_monitors),
      HaArrayCount(asig->multi_task_monitors));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_SIG"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSigAddTaskAndDescendants(KHE_TASK_SIG sig, KHE_TASK task,    */
/*    KHE_TASK_CLASS_SOLVER tcs, KHE_COST *asst_cost,KHE_COST *non_asst_cost)*/
/*                                                                           */
/*  Add atomic signatures for task and its descendants to sig.  Also add     */
/*  the assignment and non-assignment costs for task and its descendants to  */
/*  *asst_cost and *non_asst_cost.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSigAddTaskAndDescendants(KHE_TASK_SIG sig, KHE_TASK task,
  KHE_TASK_CLASS_SOLVER tcs, KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  int i;  KHE_TASK child_task;  KHE_ATOMIC_TASK_SIG atsig;

  /* do it for task */
  if( KheTaskMeet(task) != NULL && KheTaskEventResource(task) != NULL )
  {
    atsig = KheAtomicTaskSigMake(tcs, task, asst_cost, non_asst_cost);
    HaArrayAddLast(sig->atomic_sigs, atsig);
  }

  /* do it for task's descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskSigAddTaskAndDescendants(sig, child_task, tcs, asst_cost,
      non_asst_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSigMake(KHE_TASK_CLASS_SOLVER tcs, KHE_TASK task,            */
/*    KHE_TASK_SIG *sig, KHE_COST *asst_cost, KHE_COST *non_asst_cost)       */
/*                                                                           */
/*  Assuming that task has a complete time assignment, make and return its   */
/*  signature.  Also set *asst_cost to its assignment cost, and              */
/*  *non_asst_cost to its non-assignment cost.                               */
/*                                                                           */
/*****************************************************************************/
static void KheTaskSigDelete(KHE_TASK_SIG sig, KHE_TASK_CLASS_SOLVER tcs);

static bool KheTaskSigMake(KHE_TASK_CLASS_SOLVER tcs, KHE_TASK task,
  KHE_TASK_SIG *sig, KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  KHE_TASK_SIG res;

  /* get the basic object */
  if( HaArrayCount(tcs->free_task_sigs) > 0 )
  {
    res = HaArrayLastAndDelete(tcs->free_task_sigs);
    HaArrayClear(res->atomic_sigs);
  }
  else
  {
    HaMake(res, tcs->arena);
    HaArrayInit(res->atomic_sigs, tcs->arena);
  }

  /* set its domain and atomic sigs, including sorting the atomic sigs */
  res->root_domain = KheTaskDomain(task);
  *asst_cost = *non_asst_cost = 0;
  KheTaskSigAddTaskAndDescendants(res, task, tcs, asst_cost, non_asst_cost);
  HaArraySort(res->atomic_sigs, &KheAtomicTaskSigUntypedCmp);

  /* set *sig to res and return true if array is non-empty */
  if( HaArrayCount(res->atomic_sigs) > 0 )
    return *sig = res, true;
  else
  {
    KheTaskSigDelete(res, tcs);
    return *sig = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSigDelete(KHE_TASK_SIG sig, KHE_TASK_CLASS_SOLVER tcs)       */
/*                                                                           */
/*  Delete sig, by adding it to the free list in tcs.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSigDelete(KHE_TASK_SIG sig, KHE_TASK_CLASS_SOLVER tcs)
{
  int i;
  HaArrayAppend(tcs->free_atomic_task_sigs, sig->atomic_sigs, i);
  HaArrayAddLast(tcs->free_task_sigs, sig);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSigSimilar(KHE_TASK_SIG sig1, KHE_TASK_SIG sig2)             */
/*                                                                           */
/*  Return true if sig1 and sig2 are similar.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSigSimilar(KHE_TASK_SIG sig1, KHE_TASK_SIG sig2)
{
  int i;  KHE_ATOMIC_TASK_SIG asig1, asig2;

  /* domains must be equal */
  if( !KheResourceGroupEqual(sig1->root_domain, sig2->root_domain) )
    return false;

  /* same number of atomic sigs, and they must be similar */
  if( HaArrayCount(sig1->atomic_sigs) != HaArrayCount(sig2->atomic_sigs) )
    return false;
  for( i = 0;  i < HaArrayCount(sig1->atomic_sigs);  i++ )
  {
    asig1 = HaArray(sig1->atomic_sigs, i);
    asig2 = HaArray(sig2->atomic_sigs, i);
    if( !KheAtomicTaskSigSimilar(asig1, asig2) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTaskSigStartTime(KHE_TASK_SIG sig)                           */
/*                                                                           */
/*  Return the start time of sig, taken from its first atomic sig.           */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheTaskSigStartTime(KHE_TASK_SIG sig)
{
  KHE_ATOMIC_TASK_SIG asig;
  asig = HaArrayFirst(sig->atomic_sigs);
  return asig->start_time;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSigDebug(KHE_TASK_SIG sig, int verbosity, int indent,        */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of the sig onto fp with the given verbosity and indent.      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSigDebug(KHE_TASK_SIG sig, int verbosity, int indent,
  FILE *fp)
{
  KHE_ATOMIC_TASK_SIG asig;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ TaskSig ", indent, "");
    KheResourceGroupDebug(sig->root_domain, 1, 0, fp);
    HaArrayForEach(sig->atomic_sigs, asig, i)
      KheAtomicTaskSigDebug(asig, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "TaskSig(");
    KheResourceGroupDebug(sig->root_domain, 1, -1, fp);
    fprintf(fp, ", %d atomic sigs)", HaArrayCount(sig->atomic_sigs));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CLASS_TIME"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASS_TIME KheClassTimeMake(KHE_TIME time, float workload_per_time,  */
/*    KHE_TASK_CLASS_SOLVER tcs)                                             */
/*                                                                           */
/*  Make a new class-time object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_CLASS_TIME KheClassTimeMake(KHE_TIME time, float workload_per_time,
  KHE_TASK_CLASS_SOLVER tcs)
{
  KHE_CLASS_TIME res;
  if( HaArrayCount(tcs->free_class_times) > 0 )
    res = HaArrayLastAndDelete(tcs->free_class_times);
  else
    HaMake(res, tcs->arena);
  res->time = time;
  res->workload_per_time = workload_per_time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClassTimeDelete(KHE_CLASS_TIME ct, KHE_TASK_CLASS_SOLVER tcs)    */
/*                                                                           */
/*  Delete ct by appending it to the free list in tcs.                       */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheClassTimeDelete(KHE_CLASS_TIME ct, KHE_TASK_CLASS_SOLVER tcs)
{
  HaArrayAddLast(tcs->free_class_times, ct);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_CLASS"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS KheTaskClassMake(KHE_TASK_CLASS_SOLVER tcs,               */
/*    KHE_TASK_SIG sig, KHE_TASK task, KHE_COST asst_cost,                   */
/*    KHE_COST non_asst_cost)                                                */
/*                                                                           */
/*  Make a new task class object holding just this task.  The class's        */
/*  signature is sig, the task's assignment cost is asst_cost, and the       */
/*  task's non-assignment cost is non_asst_cost.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_CLASS KheTaskClassMake(KHE_TASK_CLASS_SOLVER tcs,
  KHE_TASK_SIG sig, KHE_TASK task, KHE_COST asst_cost, KHE_COST non_asst_cost)
{
  KHE_TASK_CLASS res;  KHE_ATOMIC_TASK_SIG asig;  KHE_CLASS_TIME ct;
  int i, j, bi;  KHE_TIME busy_time;

  /* get the basic object and initialize its arrays */
  if( HaArrayCount(tcs->free_classes) > 0 )
  {
    res = HaArrayLastAndDelete(tcs->free_classes);
    HaArrayClear(res->tasks_and_costs);
    /* HaArrayClear(res->class_times); */
    HaArrayClear(res->class_times_by_day);
  }
  else
  {
    HaMake(res, tcs->arena);
    HaArrayInit(res->tasks_and_costs, tcs->arena);
    /* HaArrayInit(res->class_times, tcs->arena); */
    HaArrayInit(res->class_times_by_day, tcs->arena);
  }

  /* initialize solver and sig */
  res->solver = tcs;
  res->sig = sig;

  /* initialize tasks_and_costs */
  HaArrayAddLast(res->tasks_and_costs,
    KheTaskAndCostMake(task, asst_cost, non_asst_cost, tcs));

  /* initialize first_day_index */
  asig = HaArrayFirst(sig->atomic_sigs);
  res->first_day_index = KheFrameTimeIndex(tcs->days_frame, asig->start_time);

  /* initialize class_times_by day, and also no_overlap and last_day_index */
  res->no_overlap = true;
  HaArrayForEach(sig->atomic_sigs, asig, i)
  {
    for( j = 0;  j < asig->duration;  j++ )
    {
      busy_time = KheTimeNeighbour(asig->start_time, j);
      bi = KheFrameTimeIndex(tcs->days_frame, busy_time) - res->first_day_index;
      HaArrayFill(res->class_times_by_day, bi + 1, NULL);
      if( HaArray(res->class_times_by_day, bi) == NULL )
      {
	ct = KheClassTimeMake(busy_time, asig->workload_per_time, tcs);
	HaArrayPut(res->class_times_by_day, bi, ct);
      }
      else
	res->no_overlap = false;
    }
  }
  res->last_day_index = res->first_day_index +
    HaArrayCount(res->class_times_by_day) - 1;

  /* initialize no_gaps */
  res->no_gaps = true;
  HaArrayForEach(res->class_times_by_day, ct, bi)
    if( ct == NULL )
      res->no_gaps = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassDelete(KHE_TASK_CLASS tc, KHE_TASK_CLASS_SOLVER tcs)    */
/*                                                                           */
/*  Delete tc by appending it to the free list in tcs.                       */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheTaskClassDelete(KHE_TASK_CLASS tc, KHE_TASK_CLASS_SOLVER tcs)
{
  int i;
  HaArrayAppend(tcs->free_tasks_and_costs, tc->tasks_and_costs, i);
  HaArrayAppend(tcs->free_class_times, tc->class_times_by_day, i);
  HaArrayAddLast(tcs->free_classes, tc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheClassTaskStartTime(KHE_TASK_CLASS tc)                        */
/*                                                                           */
/*  Get the starting time of tc from its first atomic signature.             */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TIME KheClassTaskStartTime(KHE_TASK_CLASS tc)
{
  KHE_ATOMIC_TASK_SIG asig;
  asig = HaArrayFirst(tc->sig->atomic_sigs);
  return asig->start_time;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassContainsTask(KHE_TASK_CLASS tc, KHE_TASK task)          */
/*                                                                           */
/*  Return true if tc contains task.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskClassContainsTask(KHE_TASK_CLASS tc, KHE_TASK task)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEach(tc->tasks_and_costs, tac, i)
    if( tac->task == task )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassAddTask(KHE_TASK_CLASS tc, KHE_TASK task,               */
/*    KHE_COST asst_cost, KHE_COST non_asst_cost, KHE_TASK_CLASS_SOLVER tcs) */
/*                                                                           */
/*  Add task (with the given asst_cost and non_asst_cost) to tc.             */
/*                                                                           */
/*****************************************************************************/

static void KheTaskClassAddTask(KHE_TASK_CLASS tc, KHE_TASK task,
  KHE_COST asst_cost, KHE_COST non_asst_cost, KHE_TASK_CLASS_SOLVER tcs)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEachReverse(tc->tasks_and_costs, tac, i)
    if( tac->asst_cost - tac->non_asst_cost <= asst_cost - non_asst_cost )
      break;
  HaArrayAdd(tc->tasks_and_costs, i + 1,
    KheTaskAndCostMake(task, asst_cost, non_asst_cost, tcs));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CLASSES_AT_TIME"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLASSES_AT_TIME KheClassesAtTimeMake(KHE_TIME time,                  */
/*    KHE_TASK_CLASS_SOLVER tcs)                                             */
/*                                                                           */
/*  Make a new classes at time object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_CLASSES_AT_TIME KheClassesAtTimeMake(KHE_TIME time,
  KHE_TASK_CLASS_SOLVER tcs)
{
  KHE_CLASSES_AT_TIME res;
  HaMake(res, tcs->arena);
  res->time = time;
  HaArrayInit(res->classes, tcs->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task class solver functions"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS_SOLVER KheTaskClassSolverMake(KHE_SOLN soln,              */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)                */
/*                                                                           */
/*  Make a new task class solver object with these attributes.               */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_CLASS_SOLVER KheTaskClassSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame, HA_ARENA a)
{
  KHE_TASK_CLASS_SOLVER res;  KHE_INSTANCE ins;  KHE_TIME time;  int i;

  /* make the basic object */
  /* a = KheSolnArenaBegin(soln, false); */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->days_frame = days_frame;
  HaArrayInit(res->classes, a);
  HaArrayInit(res->classes_at_time, a);
  HaArrayInit(res->free_tasks_and_costs, a);
  HaArrayInit(res->free_atomic_task_sigs, a);
  HaArrayInit(res->free_task_sigs, a);
  HaArrayInit(res->free_class_times, a);
  HaArrayInit(res->free_classes, a);

  /* initialize classes_at_time */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
  {
    time = KheInstanceTime(ins, i);
    HaArrayAddLast(res->classes_at_time, KheClassesAtTimeMake(time, res));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassSolverDelete(KHE_TASK_CLASS_SOLVER tcs)                 */
/*                                                                           */
/*  Delete tcs.                                                              */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KheTaskClassSolverDelete(KHE_TASK_CLASS_SOLVER tcs)
{
  KheSolnArenaEnd(tcs->soln, tcs->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassSolverFindSig(KHE_TASK_CLASS_SOLVER tcs,                */
/*    KHE_TASK_SIG sig, KHE_TASK_CLASS *res)                                 */
/*                                                                           */
/*  If tcs contains a class with this sig, set *res to that class and        */
/*  return true.  Otherwise set *res to NULL and return false.               */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskClassSolverFindSig(KHE_TASK_CLASS_SOLVER tcs,
  KHE_TASK_SIG sig, KHE_TASK_CLASS *res)
{
  KHE_TASK_CLASS tc;  int i;  KHE_CLASSES_AT_TIME cat;

  /* get cat, an object holding all classes with signature sig */
  cat = HaArray(tcs->classes_at_time, KheTimeIndex(KheTaskSigStartTime(sig)));

  /* search cat for a class with a similar signature to sig */
  HaArrayForEach(cat->classes, tc, i)
    if( KheTaskSigSimilar(sig, tc->sig) )
      return *res = tc, true;
  return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskHasCompleteTimeAssignment(KHE_TASK task)                     */
/*                                                                           */
/*  Return true if task, and every task assigned to it directly or           */
/*  indirectly, is either not derived from a meet or else is derived         */
/*  from a meet with an assigned time.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskHasCompleteTimeAssignment(KHE_TASK task)
{
  int i;  KHE_TASK child_task;  KHE_MEET meet;

  /* check task */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAsstTime(meet) == NULL )
    return false;

  /* check descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheTaskHasCompleteTimeAssignment(child_task) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassSolverAddTask(KHE_TASK_CLASS_SOLVER tcs, KHE_TASK task) */
/*                                                                           */
/*  Add task to tcs.                                                         */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassSolverAddTask(KHE_TASK_CLASS_SOLVER tcs, KHE_TASK task,
  bool *incomplete_times)
{
  KHE_TASK_CLASS tc;  KHE_TASK_SIG sig;  KHE_COST asst_cost, non_asst_cost;
  KHE_CLASSES_AT_TIME cat;

  *incomplete_times = false;
  if( KheTaskResourceType(task) != tcs->resource_type )
  {
    /* fail (1): task has the wrong resource type */
    return false;
  }
  else if( !KheTaskIsProperRoot(task) )
  {
    /* fail (2): task is not a proper root task */
    return false;
  }
  else if( !KheTaskHasCompleteTimeAssignment(task) )
  {
    /* fail (3): task has incomplete time assignment */
    *incomplete_times = true;
    return false;
  }
  else if( !KheTaskSigMake(tcs, task, &sig, &asst_cost, &non_asst_cost) )
  {
    /* fail (4): can't make sig (no assigned times) */
    return false;
  }
  else if( !KheTaskClassSolverFindSig(tcs, sig, &tc) )
  {
    /* no class for task, make one and add it to tcs */
    tc = KheTaskClassMake(tcs, sig, task, asst_cost, non_asst_cost);
    HaArrayAddLast(tcs->classes, tc);
    cat = HaArray(tcs->classes_at_time,
      KheTimeIndex(KheTaskSigStartTime(sig)));
    HaArrayAddLast(cat->classes, tc);
    return true;
  }
  else if( KheTaskClassContainsTask(tc, task) )
  {
    /* fail (5): task is already added */
    KheTaskSigDelete(sig, tcs);
    return false;
  }
  else
  {
    /* tc exists with task not in it, so add task to tc */
    KheTaskClassAddTask(tc, task, asst_cost, non_asst_cost, tcs);
    KheTaskSigDelete(sig, tcs);
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassSolverAddAllTasks(KHE_TASK_CLASS_SOLVER tcs)            */
/*                                                                           */
/*  Call KheTaskClassSolverAddTask for each task of tcs's solution.          */
/*                                                                           */
/*****************************************************************************/

void KheTaskClassSolverAddAllTasks(KHE_TASK_CLASS_SOLVER tcs,
  bool *incomplete_times)
{
  int i;  KHE_TASK task;  bool ic;
  *incomplete_times = false;
  for( i = 0;  i < KheSolnTaskCount(tcs->soln);  i++ )
  {
    task = KheSolnTask(tcs->soln, i);
    KheTaskClassSolverAddTask(tcs, task, &ic);
    if( ic )
      *incomplete_times = true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassSolverClassCount(KHE_TASK_CLASS_SOLVER tcs)              */
/*                                                                           */
/*  Return the number of task classes in tcs.                                */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassSolverClassCount(KHE_TASK_CLASS_SOLVER tcs)
{
  return HaArrayCount(tcs->classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS KheTaskClassSolverClass(KHE_TASK_CLASS_SOLVER tcs, int i) */
/*                                                                           */
/*  Return the i'th task class of tcs.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_CLASS KheTaskClassSolverClass(KHE_TASK_CLASS_SOLVER tcs, int i)
{
  return HaArray(tcs->classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassSolverClassAtTimeCount(KHE_TASK_CLASS_SOLVER tcs,        */
/*    KHE_TIME time)                                                         */
/*                                                                           */
/*  Return the number of task classes in tcs whose start time is time.       */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassSolverClassAtTimeCount(KHE_TASK_CLASS_SOLVER tcs,
  KHE_TIME time)
{
  KHE_CLASSES_AT_TIME cat;
  cat = HaArray(tcs->classes_at_time, KheTimeIndex(time));
  return HaArrayCount(cat->classes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_CLASS KheTaskClassSolverClassAtTime(KHE_TASK_CLASS_SOLVER tcs,  */
/*    KHE_TIME time, int i)                                                  */
/*                                                                           */
/*  Return the i'th task class of tcs whose start time is time.              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_CLASS KheTaskClassSolverClassAtTime(KHE_TASK_CLASS_SOLVER tcs,
  KHE_TIME time, int i)
{
  KHE_CLASSES_AT_TIME cat;
  cat = HaArray(tcs->classes_at_time, KheTimeIndex(time));
  return HaArray(cat->classes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassSolverDebug(KHE_TASK_CLASS_SOLVER tcs, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of tcs onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheTaskClassSolverDebug(KHE_TASK_CLASS_SOLVER tcs, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_CLASS tc;  int i, j;  KHE_CLASSES_AT_TIME cat;
  fprintf(fp, "%*s[ TaskClassSolver(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(tcs->soln)),
    KheResourceTypeId(tcs->resource_type));
  HaArrayForEach(tcs->classes_at_time, cat, i)
  {
    fprintf(fp, "%*s  [ %s:\n", indent, "", KheTimeId(cat->time));
    HaArrayForEach(cat->classes, tc, j)
      KheTaskClassDebug(tc, verbosity, indent + 4, fp);
    fprintf(fp, "%*s  ]\n", indent, "");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task class public queries"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassTaskCount(KHE_TASK_CLASS tc)                             */
/*                                                                           */
/*  Return the number of tasks in tc.                                        */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassTaskCount(KHE_TASK_CLASS tc)
{
  return HaArrayCount(tc->tasks_and_costs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskClassTask(KHE_TASK_CLASS tc, int i,                      */
/*    KHE_COST *asst_cost, KHE_COST *non_asst_cost)                          */
/*                                                                           */
/*  Return the i'th task of tc.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskClassTask(KHE_TASK_CLASS tc, int i,
  KHE_COST *asst_cost, KHE_COST *non_asst_cost)
{
  KHE_TASK_AND_COST tac;
  tac = HaArray(tc->tasks_and_costs, i);
  *asst_cost = tac->asst_cost;
  *non_asst_cost = tac->non_asst_cost;
  return tac->task;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassTimeCount(KHE_TASK_CLASS tc)                             */
/*                                                                           */
/*  Return the number of busy times shared by the tasks of tc.               */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheTaskClassTimeCount(KHE_TASK_CLASS tc)
{
  return HaArrayCount(tc->class_times);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTaskClassTime(KHE_TASK_CLASS tc, int i,                      */
/*    float *workload_per_time)                                              */
/*                                                                           */
/*  Return the i'th busy time shared by the tasks of tc.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TIME KheTaskClassTime(KHE_TASK_CLASS tc, int i, float *workload_per_time)
{
  KHE_CLASS_TIME ct;
  ct = HaArray(tc->class_times, i);
  return *workload_per_time = ct->workload_per_time, ct->time;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskClassDomain(KHE_TASK_CLASS tc)                 */
/*                                                                           */
/*  Return the domain shared by the tasks of tc.                             */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskClassDomain(KHE_TASK_CLASS tc)
{
  return tc->sig->root_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassNoOverlap(KHE_TASK_CLASS tc)                            */
/*                                                                           */
/*  Return true if there are no cases where two of the shared times of       */
/*  the tasks of tc overlap with a single day of days_frame.                 */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassNoOverlap(KHE_TASK_CLASS tc)
{
  return tc->no_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassFirstDayIndex(KHE_TASK_CLASS tc)                         */
/*                                                                           */
/*  Return the day index of the first time of tc.                            */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassFirstDayIndex(KHE_TASK_CLASS tc)
{
  return tc->first_day_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassLastDayIndex(KHE_TASK_CLASS tc)                          */
/*                                                                           */
/*  Return the day index of the last time of tc.                             */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassLastDayIndex(KHE_TASK_CLASS tc)
{
  return tc->last_day_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheTaskClassDayTime(KHE_TASK_CLASS tc, int day_index,           */
/*    float *workload_per_time)                                              */
/*                                                                           */
/*  Return one time (the only one if KheTaskClassNoOverlap returns true)     */
/*  when tc is busy on day day_index.  Also return the workload per time     */
/*  then.  If tc is not busy on day_index, return NULL (0 for workload).     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheTaskClassDayTime(KHE_TASK_CLASS tc, int day_index,
  float *workload_per_time)
{
  KHE_CLASS_TIME ct;
  if( day_index < tc->first_day_index || day_index > tc->last_day_index )
    return *workload_per_time = 0, NULL;
  ct = HaArray(tc->class_times_by_day, day_index - tc->first_day_index);
  if( ct == NULL )
    return *workload_per_time = 0, NULL;
  return *workload_per_time = ct->workload_per_time, ct->time;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassNoGaps(KHE_TASK_CLASS tc)                               */
/*                                                                           */
/*  Return true when there are no gaps in the days covered by the tasks      */
/*  of tc.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassNoGaps(KHE_TASK_CLASS tc)
{
  return tc->no_gaps;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskClassUnassignedTaskCount(KHE_TASK_CLASS tc)                   */
/*                                                                           */
/*  Return the number of unassigned tasks in tc.                             */
/*                                                                           */
/*****************************************************************************/

int KheTaskClassUnassignedTaskCount(KHE_TASK_CLASS tc)
{
  int res, i;  KHE_TASK_AND_COST tac;
  res = 0;
  HaArrayForEach(tc->tasks_and_costs, tac, i)
    if( KheTaskAsstResource(tac->task) == NULL )
      res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassFindTask(KHE_TASK_CLASS tc, KHE_RESOURCE r,             */
/*    KHE_TASK *task)                                                        */
/*                                                                           */
/*  If tc contains a task assigned r (possibly NULL), return true with       */
/*  *task set to its first such task.  Otherwise return false with *task     */
/*  set to NULL.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskClassFindTask(KHE_TASK_CLASS tc, KHE_RESOURCE r,
  KHE_TASK *task)
{
  int i;  KHE_TASK_AND_COST tac;
  HaArrayForEach(tc->tasks_and_costs, tac, i)
    if( KheTaskAsstResource(tac->task) == r )
      return *task = tac->task, true;
  return *task = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassHasUnassignedTask(KHE_TASK_CLASS tc)                    */
/*                                                                           */
/*  Return true if tc contains at least one unassigned task.                 */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassHasUnassignedTask(KHE_TASK_CLASS tc)
{
  int i;  KHE_TASK_AND_COST tac;
  HaArrayForEachReverse(tc->tasks_and_costs, tac, i)
    if( KheTaskAsstResource(tac->task) == NULL )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassAssignCheck(KHE_TASK_CLASS tc, KHE_RESOURCE r)          */
/*                                                                           */
/*  Return true if r can be assigned to a task of tc.                        */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassAssignCheck(KHE_TASK_CLASS tc, KHE_RESOURCE r)
{
  KHE_TASK task;
  return KheTaskClassFindTask(tc, NULL, &task) &&
    KheTaskAssignResourceCheck(task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassAssign(KHE_TASK_CLASS tc, KHE_RESOURCE r)               */
/*                                                                           */
/*  Assign r to a task of tc, if possible.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskClassAssign(KHE_TASK_CLASS tc, KHE_RESOURCE r)
{
  KHE_TASK task;
  return KheTaskClassFindTask(tc, NULL, &task) &&
    KheTaskAssignResource(task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassUnAssign(KHE_TASK_CLASS tc, KHE_RESOURCE r)             */
/*                                                                           */
/*  Unassign r from a task of tc.                                            */
/*                                                                           */
/*****************************************************************************/

void KheTaskClassUnAssign(KHE_TASK_CLASS tc, KHE_RESOURCE r)
{
  KHE_TASK task;
  if( KheTaskClassFindTask(tc, r, &task) )
    KheTaskUnAssignResource(task);
  else
    HnAbort("KheTaskClassUnAssign: no task assigned r");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "dominance"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorResourceDominates(KHE_MONITOR m,                          */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2)                                      */
/*                                                                           */
/*  Return true if r1 dominates r2 in m.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMonitorResourceDominates(KHE_MONITOR m,
  KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_RESOURCE_GROUP rg;
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      ** always true; cost is independent of which resource is assigned **
      return true;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      ** true if r1 is preferred or r2 is not preferred **
      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      prc = KhePreferResourcesMonitorConstraint(prm);
      rg = KhePreferReso urcesConstraintDomain(prc);
      return KheResourceGroupContains(rg, r1) ||
	!KheResourceGroupContains(rg, r2);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      ** will depend, unpredictably, on the other assignments **
      return false;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      ** true if both preferred or both not; simplified, but good enough **
      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      lrc = KheLimitResourcesMonitorConstraint(lrm);
      rg = KheLimitResourcesConstraintDomain(lrc);
      return KheResourceGroupContains(rg,r1) == KheResourceGroupContains(rg,r2);

    default:

      HnAbort("KheMonitorResourceDominates: unexpected monitor tag (%d)",
        KheMonitorTag(m));
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorResourceDominatesWithTradeoff(KHE_MONITOR m, int durn,    */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)       */
/*                                                                           */
/*  Check dominance of r1 over r2 for m.                                     */
/*                                                                           */
/*  Here false is returned only when a clear answer cannot be given.  A      */
/*  true result means that r1 dominates r2, with *avail_tradeoff_cost set    */
/*  to the amount of cost available afterwards.  This could be negative.     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheMonitorResourceDominatesWithTradeoff(KHE_MONITOR m, int durn,
  KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)
{
  KHE_PREFER_RESOURCES_MONITOR prm;  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_RESOURCE_GROUP rg;  KHE_CONSTRAINT c;  ** KHE_COST new_avail; **
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      ** always true; cost is independent of which resource is assigned **
      return true;

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      ** depends on whether r1 and r2 are preferred **
      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      prc = KhePreferResourcesMonitorConstraint(prm);
      rg = KhePreferResou rcesConstraintDomain(prc);
      c = (KHE_CONSTRAINT) prc;
      if( KheResourceGroupContains(rg, r1) )
      {
        if( KheResourceGroupContains(rg, r2) )
	{
	  ** both r1 and r2 are preferred; all good as is **
	  return true;
	}
	else
	{
	  ** r1 is preferred but r2 is not; this makes more cost available **
	  *avail_tradeoff_cost += KheConst raintCombinedWeight(c) * durn;
	  return true;
	}
      }
      else
      {
        if( KheResourceGroupContains(rg, r2) )
	{
	  ** r1 is unpreferred and r2 is preferred; this consumes cost **
	  if( KheMonitorCostFunction(m) == KHE_QUADRATIC_COST_FUNCTION )
	    return false;
	  *avail_tradeoff_cost -= KheConstra intCombinedWeight(c) * durn;
	  return true;
	}
	else
	{
	  ** both r1 and r2 are unpreferred; all good as is **
	  return true;
	}
      }

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      ** will depend, unpredictably, on the other assignments **
      return false;

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      ** true if both preferred or both not; simplified, but good enough **
      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      lrc = KheLimitResourcesMonitorConstraint(lrm);
      rg = KheLimitResourcesConstraintDomain(lrc);
      return KheResourceGroupContains(rg,r1) == KheResourceGroupContains(rg,r2);

    default:

      HnAbort("KheMonitorResourceDominatesWithTradeoff: unexpected monitor "
	"tag (%d)", KheMonitorTag(m));
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicSigResourceDominates(KHE_ATOMIC_TASK_SIG asig,             */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2)                                      */
/*                                                                           */
/*  Return true if r1 dominates r2 in asig.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheAtomicSigResourceDominates(KHE_ATOMIC_TASK_SIG asig,
  KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(asig->single_task_monitors, m, i)
    if( !KheMonitorResourceDominates(m, r1, r2) )
      return false;
  HaArrayForEach(asig->multi_task_monitors, m, i)
    if( !KheMonitorResourceDominates(m, r1, r2) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicSigDominatesWithTradeoff(KHE_ATOMIC_TASK_SIG asig,         */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)       */
/*                                                                           */
/*  Like KheAtomicSigResourceDominates but with tradeoff dominance.          */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheAtomicSigDominatesWithTradeoff(KHE_ATOMIC_TASK_SIG asig,
  KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(asig->single_task_monitors, m, i)
    if( !KheMonitorResourceDominatesWithTradeoff(m, asig->duration, r1, r2,
	  avail_tradeoff_cost) )
      return false;
  HaArrayForEach(asig->multi_task_monitors, m, i)
    if( !KheMonitorResourceDominatesWithTradeoff(m, asig->duration, r1, r2,
	  avail_tradeoff_cost) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassResourceDominates(KHE_TASK_CLASS tc,                    */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2)                                      */
/*                                                                           */
/*  Return true if r1 dominates r2 in tc; that is, if assigning r1 to a      */
/*  task of tc would have equal or less cost than assigning r2 to a task     */
/*  of tc.                                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskClassResourceDominates(KHE_TASK_CLASS tc,
  KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  KHE_ATOMIC_TASK_SIG asig;  int i;
  if( tc == NULL )
    return true;
  HaArrayForEach(tc->sig->atomic_sigs, asig, i)
    if( !KheAtomicSigResourceDominates(asig, r1, r2) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskClassResourceDominates(KHE_TASK_CLASS tc,                    */
/*    KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)       */
/*                                                                           */
/*  Like KheTaskClassResourceDominates only with tradeoff dominance.         */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
bool KheTaskClassResourceDominates(KHE_TASK_CLASS tc,
  KHE_RESOURCE r1, KHE_RESOURCE r2, KHE_COST *avail_tradeoff_cost)
{
  KHE_ATOMIC_TASK_SIG asig;  int i;
  if( tc == NULL )
    return true;
  HaArrayForEach(tc->sig->atomic_sigs, asig, i)
    if( !KheAtomicSigDominatesWithTradeoff(asig, r1, r2, avail_tradeoff_cost) )
      return false;
  return *avail_tradeoff_cost >= 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassDebugHeader(KHE_TASK_CLASS tc, FILE *fp)                */
/*                                                                           */
/*  Print the header part of the debug of tc onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskClassDebugDayRange(KHE_TASK_CLASS tc, FILE *fp)
{
  KHE_TIME_GROUP tg1, tg2;
  if( tc->first_day_index != tc->last_day_index )
  {
    tg1 = KheFrameTimeGroup(tc->solver->days_frame, tc->first_day_index);
    tg2 = KheFrameTimeGroup(tc->solver->days_frame, tc->last_day_index);
    fprintf(fp, "%s-%s", KheTimeGroupId(tg1), KheTimeGroupId(tg2));
  }
  else
  {
    tg1 = KheFrameTimeGroup(tc->solver->days_frame, tc->first_day_index);
    fprintf(fp, "%s", KheTimeGroupId(tg1));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClassDebug(KHE_TASK_CLASS tc, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of tc onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheTaskClassDebug(KHE_TASK_CLASS tc, int verbosity, int indent, FILE *fp)
{
  KHE_TASK_AND_COST tac;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ TaskClass ", indent, "");
    KheTaskClassDebugDayRange(tc, fp);
    fprintf(fp, "\n");
    if( verbosity >= 2 )
      KheTaskSigDebug(tc->sig, verbosity, indent + 2, fp);
    HaArrayForEach(tc->tasks_and_costs, tac, i)
    {
      fprintf(fp, "%*s a%.5f n%.5f ", indent, "",
	KheCostShow(tac->asst_cost), KheCostShow(tac->non_asst_cost));
      KheTaskDebug(tac->task, verbosity, 0, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "TaskClass(");
    KheTaskClassDebugDayRange(tc, fp);
    HaArrayForEach(tc->tasks_and_costs, tac, i)
    {
      fprintf(fp, ", ");
      KheTaskDebug(tac->task, verbosity, -1, fp);
    }
    fprintf(fp, ")");
  }
}

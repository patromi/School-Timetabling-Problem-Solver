
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
/*  FILE:         khe_sr_mtask_finder.c                                      */
/*  DESCRIPTION:  Multi-tasks                                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Submodule "Type declarations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST - one task with its associated costs                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_and_cost_rec {
  KHE_TASK			task;			/* a task            */
  KHE_COST			non_asst_cost;		/* its non-asst cost */
  KHE_COST			asst_cost;		/* its asst cost     */
} *KHE_TASK_AND_COST;

typedef HA_ARRAY(KHE_TASK_AND_COST) ARRAY_KHE_TASK_AND_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND - classifies monitors into one of four kinds            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_MONITOR_IGNORE,
  KHE_MONITOR_INSEPARABLE,
  KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT,
  KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT
} KHE_MONITOR_KIND;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG - what similarity of atomic tasks is based on        */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_atomic_task_sig_rec {
  bool				fixed_times;
  union {
    KHE_TIME			first_time;
    KHE_MEET			encl_meet;
  } u;
  int				duration;
  float				workload_per_time;
  ARRAY_KHE_MONITOR		separable_rd_monitors;
  ARRAY_KHE_MONITOR		inseparable_monitors;
} *KHE_ATOMIC_TASK_SIG;

typedef HA_ARRAY(KHE_ATOMIC_TASK_SIG) ARRAY_KHE_ATOMIC_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ROOT_TASK_SIG - what similarity of proper root tasks is based on     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_root_task_sig_rec {
  KHE_RESOURCE_GROUP		root_domain;
  KHE_RESOURCE			root_preasst;
  bool				root_asst_is_fixed;
  int				total_duration;
  float				total_workload;
  ARRAY_KHE_ATOMIC_TASK_SIG	atomic_sigs;
} *KHE_ROOT_TASK_SIG;

typedef HA_ARRAY(KHE_ROOT_TASK_SIG) ARRAY_KHE_ROOT_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_TIME - what an mtask is doing at one time                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_time_rec {
  KHE_TIME			time;			/* the time          */
  float				workload_per_time;	/* workload at time  */
} *KHE_MTASK_TIME;

typedef HA_ARRAY(KHE_MTASK_TIME) ARRAY_KHE_MTASK_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK                                                                */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MTASK_SET) ARRAY_KHE_MTASK_SET;

struct khe_mtask_rec {
  KHE_MTASK_FINDER		mtask_finder;
  ARRAY_KHE_MTASK_SET		internal_mtask_sets;
  int				mtasks_index;
  KHE_ROOT_TASK_SIG		sig;
  ARRAY_KHE_TASK_AND_COST	tasks_and_costs;
  int				asst_count;
  bool				fixed_times;  /* not degenerate & fixed times */
  bool				fixed_times_no_overlap;
  bool				fixed_times_no_gaps;
  KHE_INTERVAL			fixed_times_interval;
  ARRAY_KHE_MTASK_TIME		fixed_times_by_day;
  KHE_TIME_SET			fixed_times_time_set;
};

typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET                                                            */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_set_rec {
  KHE_INTERVAL			interval;     /* interval covered by mtasks */
  ARRAY_KHE_MTASK		mtasks;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET - cache of mtask sets indexed by time group     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_group_mtask_set_rec {
  KHE_RESOURCE_TYPE		rt;
  KHE_TIME_GROUP		tg;
  KHE_MTASK_SET			mts;
} *KHE_TIME_GROUP_MTASK_SET;

typedef HA_ARRAY(KHE_TIME_GROUP_MTASK_SET) ARRAY_KHE_TIME_GROUP_MTASK_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET_SET - cache of mtask sets indexed by time group */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_group_mtask_set_set_rec {
  ARRAY_KHE_TIME_GROUP_MTASK_SET	mtask_sets;
} *KHE_TIME_GROUP_MTASK_SET_SET;

typedef HA_ARRAY(KHE_TIME_GROUP_MTASK_SET_SET)
  ARRAY_KHE_TIME_GROUP_MTASK_SET_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET - cache of mtask sets indexed by interval         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_interval_mtask_set_rec {
  KHE_RESOURCE_TYPE		rt;
  KHE_INTERVAL			in;
  KHE_MTASK_SET			mts;
} *KHE_INTERVAL_MTASK_SET;

typedef HA_ARRAY(KHE_INTERVAL_MTASK_SET) ARRAY_KHE_INTERVAL_MTASK_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET_SET - cache of mtask sets indexed by interval     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_interval_mtask_set_set_rec {
  ARRAY_KHE_INTERVAL_MTASK_SET	mtask_sets;
} *KHE_INTERVAL_MTASK_SET_SET;

typedef HA_ARRAY(KHE_INTERVAL_MTASK_SET_SET) ARRAY_KHE_INTERVAL_MTASK_SET_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET_CACHE - a cache holding mtask sets                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_set_cache_rec {

  /* free lists */
  ARRAY_KHE_TIME_GROUP_MTASK_SET	tg_mtask_set_free_list;
  ARRAY_KHE_TIME_GROUP_MTASK_SET_SET	tg_mtask_set_set_free_list;
  ARRAY_KHE_INTERVAL_MTASK_SET		in_mtask_set_free_list;
  ARRAY_KHE_INTERVAL_MTASK_SET_SET	in_mtask_set_set_free_list;

  /* constant attributes */
  KHE_MTASK_FINDER			mtask_finder;
  KHE_MTASK_SET				empty_mtask_set;

  /* the actual cache */
  ARRAY_KHE_TIME_GROUP_MTASK_SET_SET	mtasks_in_tg;
  ARRAY_KHE_INTERVAL_MTASK_SET_SET  	mtasks_in_in;
} *KHE_MTASK_SET_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER                                                         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

struct khe_mtask_finder_rec {

  /* free lists */
  ARRAY_KHE_TASK_AND_COST		task_and_cost_free_list;
  ARRAY_KHE_ATOMIC_TASK_SIG		atomic_task_sig_free_list;
  ARRAY_KHE_ROOT_TASK_SIG		root_task_sig_free_list;
  ARRAY_KHE_MTASK_TIME			mtask_time_free_list;
  ARRAY_KHE_MTASK			mtask_free_list;
  ARRAY_KHE_MTASK_SET			mtask_set_free_list;

  /* constant attributes */
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_FRAME			days_frame;
  bool				fixed_times;

  /* mtasks, indexed in various ways */
  ARRAY_KHE_MTASK		mtasks;			/* the mtasks        */
  ARRAY_KHE_MTASK_SET		mtasks_at_time;	       	/* mtasks at time    */
  ARRAY_KHE_MTASK_SET		mtasks_at_meet;		/* mtasks at meet    */
  KHE_MTASK_SET			mtasks_degenerate;
  ARRAY_KHE_MTASK		mtasks_by_task;		/* indexed by task   */
  /* ARRAY_KHE_TIME_GROUP_MTASK_SET_SET mtasks_in_tg; */
  /* ARRAY_KHE_INTERVAL_MTASK_SET_SET   mtasks_in_in; */
  KHE_MTASK_SET_CACHE		mts_cache;

  /* task grouping */
  KHE_TASK			group_leader_task;
  KHE_RESOURCE			group_asst;
  ARRAY_KHE_TASK		group_follower_tasks;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_AND_COST"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task,                      */
/*    KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)      */
/*                                                                           */
/*  Make and return a new task and cost object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)
{
  KHE_TASK_AND_COST res;
  if( HaArrayCount(mtf->task_and_cost_free_list) > 0 )
    res = HaArrayLastAndDelete(mtf->task_and_cost_free_list);
  else
    HaMake(res, mtf->arena);
  res->task = task;
  res->non_asst_cost = non_asst_cost;
  res->asst_cost = asst_cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAndCostDelete(KHE_TASK_AND_COST tac, KHE_MTASK_FINDER mtf)   */
/*                                                                           */
/*  Delete tac.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskAndCostDelete(KHE_TASK_AND_COST tac, KHE_MTASK_FINDER mtf)
{
  HaArrayAddLast(mtf->task_and_cost_free_list, tac);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MONITOR_KIND and monitor classification"                  */
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

/* *** no longer used
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesMonitorTaskCount(KHE_LIMIT_RESOURCES_MONITOR lrm,   */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Return the number of tasks monitored by lrm.                             */
/*                                                                           */
/*****************************************************************************/

static int KheLimitResourcesMonitorTaskCount(KHE_LIMIT_RESOURCES_MONITOR lrm,
  KHE_SOLN soln)
{
  int i, eg_index, count, res;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  res = 0;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    res += KheEventResourceTaskCount(soln, er);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KheAssignResourceConstraintClassify(KHE_CONSTRAINT c,   */
/*    int task_count, int durn, KHE_COST *non_asst_cost, KHE_COST *asst_cost)*/
/*                                                                           */
/*  Carry out KheMonitorKind for an assign resource constraint with the      */
/*  cost function and weight of c which monitors task_count tasks.           */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KheAssignResourceMonitorClassify(KHE_MONITOR m,
  int task_count, int durn, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  if( task_count == 1 || KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
  {
    /* separable resource-independent with non-assignment cost */
    *non_asst_cost += KheMonitorDevToCost(m, durn);
    return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
  }
  else
  {
    /* inseparable */
    return KHE_MONITOR_INSEPARABLE;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KhePreferResourcesConstraintClassify(KHE_CONSTRAINT c,  */
/*    KHE_RESOURCE_TYPE rt, int resource_count, int task_count, int durn,    */
/*    KHE_COST *non_asst_cost, KHE_COST *asst_cost)                          */
/*                                                                           */
/*  Carry out KheMonitorKind for a prefer resources constraint with the      */
/*  cost function and weight of c which prefers resource_count resources     */
/*  of type rt and monitors task_count tasks.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KhePreferResourcesMonitorClassify(KHE_MONITOR m,
  KHE_RESOURCE_TYPE rt, int resource_count, int task_count, int durn,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  if( resource_count == KheResourceTypeResourceCount(rt) )
  {
    /* no cost is possible, ignore this monitor */
    return KHE_MONITOR_IGNORE;
  }
  else if( task_count == 1 ||
    KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
  {
    /* separable */
    if( resource_count == 0 )
    {
      /* separable resource-independent with assignment cost */
      *asst_cost += KheMonitorDevToCost(m, durn);
      return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
    }
    else
    {
      /* separable resource-dependent */
      return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
    }
  }
  else
  {
    /* inseparable */
    return KHE_MONITOR_INSEPARABLE;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesMonitorTaskDuration(KHE_LIMIT_RESOURCES_MONITOR lrm)*/
/*                                                                           */
/*  Return the total duration of the tasks monitored by lrm.                 */
/*                                                                           */
/*****************************************************************************/

static int KheLimitResourcesMonitorTaskDuration(KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  int i, eg_index, count, res;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  res = 0;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    res += KheEventDuration(KheEventResourceEvent(er));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorMimicsAssignResourceMonitor(                */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm)                                       */
/*                                                                           */
/*  Return true if lrm mimics an assign resource monitor:  if it monitors    */
/*  every resource of its type and its minimum limit is equal to the total   */
/*  duration of the tasks it monitors.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorMimicsAssignResourceMonitor(
  KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE_TYPE rt;  int rg_count, min_limit;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  rg = KheLimitResourcesConstraintDomain(lrc);
  rg_count = KheResourceGroupResourceCount(rg);
  rt = KheResourceGroupResourceType(rg);
  min_limit = KheLimitResourcesConstraintMinimum(lrc);
  return rg_count == KheResourceTypeResourceCount(rt) &&
    min_limit == KheLimitResourcesMonitorTaskDuration(lrm);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorMimicsPreferResourcesMonitor(               */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm)                                       */
/*                                                                           */
/*  Return true if lrm mimics a prefer resources monitor:  if its maximum    */
/*  limit is 0.  It will then mimic a prefer resources monitor whose set     */
/*  of preferred resources is the complement of lrm's set of resources.      */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorMimicsPreferResourcesMonitor(
  KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  return KheLimitResourcesConstraintMaximum(lrc) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,                 */
/*    KHE_COST *non_asst_cost, KHE_COST *asst_cost)                          */
/*                                                                           */
/*  Return m's kind.  If m is separable resource-independent, also add to    */
/*  *non_asst_cost the cost of not assigning a resource to a task of the     */
/*  given durn monitored by m, and add to *asst_cost the cost of doing so.   */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_RESOURCE_GROUP rg;  int rg_count, task_count, max_limit, min_limit;
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_RESOURCE er;
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_SOLN soln;
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  soln = KheMonitorSoln(m);
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
      er = KheAssignResourceMonitorEventResource(arm);
      return KheAssignResourceMonitorClassify(m,
        KheEventResourceTaskCount(soln, er), durn, non_asst_cost, asst_cost);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      rg = KhePreferResourcesMonitorDomain(prm);
      rg_count = KheResourceGroupResourceCount(rg);
      rt = KheResourceGroupResourceType(rg);
      er = KhePreferResourcesMonitorEventResource(prm);
      return KhePreferResourcesMonitorClassify(m, rt, rg_count,
	KheEventResourceTaskCount(soln, er), durn, non_asst_cost, asst_cost);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
      if( KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(asam, soln) )
      {
	/* no cost is possible, ignore this monitor */
	return KHE_MONITOR_IGNORE;
      }
      else
      {
	/* inseparable */
	return KHE_MONITOR_INSEPARABLE;
      }

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      lrc = KheLimitResourcesMonitorConstraint(lrm);
      rg = KheLimitResourcesConstraintDomain(lrc);
      rg_count = KheResourceGroupResourceCount(rg);
      rt = KheResourceGroupResourceType(rg);
      max_limit = KheLimitResourcesConstraintMaximum(lrc);
      min_limit = KheLimitResourcesConstraintMinimum(lrc);
      task_count = KheLimitResourcesMonitorTaskCount(lrm, soln);
      /* *** what this case asserts is not true!
      if( rg_count == 0 )
      {
	** no cost is possible, ignore this monitor **
	return KHE_MONITOR_IGNORE;
      }
      else
      *** */
      if( KheLimitResourcesMonitorMimicsAssignResourceMonitor(lrm) )
      {
	/* classify as an assign resource monitor would be classifed */
	return KheAssignResourceMonitorClassify(m, task_count, durn,
	  non_asst_cost, asst_cost);
      }
      else if( KheLimitResourcesMonitorMimicsPreferResourcesMonitor(lrm) )
      {
	/* classify as a prefer resources monitor would be classifed */
	return KhePreferResourcesMonitorClassify(m, rt,
	  KheResourceTypeResourceCount(rt) - rg_count,  /* size of complement */
          task_count, durn, non_asst_cost, asst_cost);
      }
      else if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION
	  && max_limit == 0 )
      {
	/* separable */
	if( rg_count == KheResourceTypeResourceCount(rt) )
	{
	  /* separable resource-independent, with assignment cost */
	  *asst_cost += KheMonitorDevToCost(m, durn);
	  return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* separable resource-dependent */
	  return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
	}
      }
      else if( task_count == 1 )
      {
	/* separable */
	if( rg_count == KheResourceTypeResourceCount(rt) )
	{
	  /* separable resource-independent, with a messy cost calculation */
	  /* which reduces to the previous one when max_limit == 0 */
	  if( min_limit > 0 )
	    *non_asst_cost += KheMonitorDevToCost(m, min_limit);
	  if( durn < min_limit )
	    *asst_cost += KheMonitorDevToCost(m, min_limit - durn);
	  else if( durn > max_limit )
	    *asst_cost += KheMonitorDevToCost(m, durn - max_limit);
	  return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* separable resource-dependent */
	  return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
	}
      }
      else
      {
	/* inseparable */
	return KHE_MONITOR_INSEPARABLE;
      }

    default:

      HnAbort("KheMonitorKind: unexpected monitor tag (%s)",
	KheMonitorTagShow(KheMonitorTag(m)));
      return KHE_MONITOR_INSEPARABLE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ATOMIC_TASK_SIG"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig,                */
/*    KHE_MONITOR m, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Add m to asig, or add its costs to *non_asst_cost and *asst_cost.        */
/*                                                                           */
/*****************************************************************************/

static void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig,
  KHE_MONITOR m, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  switch( KheMonitorKind(m, asig->duration, non_asst_cost, asst_cost) )
  {
    case KHE_MONITOR_IGNORE:

      /* nothing to do; we're ignoring this monitor */
      break;

    case KHE_MONITOR_INSEPARABLE:

      HaArrayAddLast(asig->inseparable_monitors, m);
      break;

    case KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT:

      /* nothing to do; costs already added */
      break;

    case KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT:

      HaArrayAddLast(asig->separable_rd_monitors, m);
      break;

    default:

      HnAbort("KheAtomicTaskSigAddMonitor: illegal monitor kind");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInseparableMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)        */
/*                                                                           */
/*  Typed comparison function for sorting an array of monitors into the      */
/*  order that inseparable monitors need to be sorted into.                  */
/*                                                                           */
/*  Implementation note.  The inseparable monitors of two atomic task        */
/*  signatures need to be the same monitors for similarity, so it is         */
/*  sufficient here to sort them by their unique index numbers.              */
/*                                                                           */
/*****************************************************************************/

static int KheInseparableMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInseparableMonitorUntypedCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Untyped comparison function for sorting an array of monitors into the    */
/*  order that inseparable monitors need to be sorted into.                  */
/*                                                                           */
/*****************************************************************************/

static int KheInseparableMonitorUntypedCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheInseparableMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSeparableResourceDependentMonitorTypedCmp(KHE_MONITOR m1,         */
/*    KHE_MONITOR m2)                                                        */
/*                                                                           */
/*  Typed comparison function for sorting an array of separable resource-    */
/*  dependent monitors into the order that they monitors need to be in.      */
/*                                                                           */
/*  Implementation note.  The resource-dependent separable monitors of       */
/*  two atomic task signatures need to have equal attributes for similarity, */
/*  so they have to be sorted by those attributes.                           */
/*                                                                           */
/*****************************************************************************/

static int KheSeparableResourceDependentMonitorTypedCmp(KHE_MONITOR m1,
  KHE_MONITOR m2)
{
  KHE_RESOURCE_GROUP rg1, rg2;  KHE_PREFER_RESOURCES_MONITOR prm1, prm2;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc1, lrc2;  int cmp;
  KHE_LIMIT_RESOURCES_MONITOR lrm1, lrm2;

  /* shortcut when m1 and m2 are the same monitor */
  if( m1 == m2 )
    return 0;

  /* monitor type attributes must be equal */
  cmp = KheMonitorTag(m1) - KheMonitorTag(m2);
  if( cmp != 0 ) return cmp;

  /* hardness, weight, and cost function attributes must be equal */
  cmp = KheCostCmp(KheMonitorCombinedWeight(m1), KheMonitorCombinedWeight(m2));
  if( cmp != 0 ) return cmp;
  cmp = (int) KheMonitorCostFunction(m1) - (int) KheMonitorCostFunction(m2);
  if( cmp != 0 ) return cmp;

  switch( KheMonitorTag(m1) )
  {
    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

      /* preferred resources must be equal */
      prm1 = (KHE_PREFER_RESOURCES_MONITOR) m1;
      prm2 = (KHE_PREFER_RESOURCES_MONITOR) m2;
      rg1 = KhePreferResourcesMonitorDomain(prm1);
      rg2 = KhePreferResourcesMonitorDomain(prm2);
      return KheResourceGroupTypedCmp(rg1, rg2);

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      /* min limit, max limit, and preferred resources must be equal */
      lrm1 = (KHE_LIMIT_RESOURCES_MONITOR) m1;
      lrm2 = (KHE_LIMIT_RESOURCES_MONITOR) m2;
      lrc1 = KheLimitResourcesMonitorConstraint(lrm1);
      lrc2 = KheLimitResourcesMonitorConstraint(lrm2);
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

      HnAbort("KheSeparableResourceDependentMonitorTypedCmp internal error: "
	"unexpected constraint tag (%d)", KheMonitorTag(m1));
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSeparableResourceDependentMonitorUntypedCmp(const void *t1,       */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of separable resource-  */
/*  dependent monitors into the order that they need to be sorted into.      */
/*                                                                           */
/*****************************************************************************/

static int KheSeparableResourceDependentMonitorUntypedCmp(const void *t1,
  const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheSeparableResourceDependentMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_MTASK_FINDER mtf,           */
/*    KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Assuming that task lies in a meet, make and return a new atomic          */
/*  signature object for task.  Also add in task's costs.                    */
/*                                                                           */
/*  NB There is no KheAtomicTaskSigDelete, because KheRootTaskSigDelete      */
/*  uses HaArrayAppend to free all its atomic task sigs at once.             */
/*                                                                           */
/*****************************************************************************/

static KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_ATOMIC_TASK_SIG res;  KHE_MEET meet;  KHE_EVENT_RESOURCE er;
  KHE_MONITOR m;  int i;  KHE_TIME first_time;

  /* get the object */
  if( HaArrayCount(mtf->atomic_task_sig_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->atomic_task_sig_free_list);
    HaArrayClear(res->separable_rd_monitors);
    HaArrayClear(res->inseparable_monitors);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->separable_rd_monitors, mtf->arena);
    HaArrayInit(res->inseparable_monitors, mtf->arena);
  }

  /* initialize first_time or meet, duration, and workload per time */
  meet = KheTaskMeet(task);
  first_time = KheMeetAsstTime(meet);
  if( mtf->fixed_times && first_time != NULL )
  {
    res->fixed_times = true;
    res->u.first_time = first_time;
  }
  else
  {
    res->fixed_times = false;
    res->u.encl_meet = meet;
  }
  res->duration = KheMeetDuration(meet);
  res->workload_per_time = KheTaskWorkloadPerTime(task);

  /* initialize monitors */
  er = KheTaskEventResource(task);
  for( i = 0;  i < KheSolnEventResourceMonitorCount(mtf->soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(mtf->soln, er, i);
    if( KheMonitorAttachedToSoln(m) && KheMonitorCombinedWeight(m) > 0 )
      KheAtomicTaskSigAddMonitor(res, m, non_asst_cost, asst_cost);
  }

  /* sort monitors */
  HaArraySort(res->separable_rd_monitors,
    &KheSeparableResourceDependentMonitorUntypedCmp);
  HaArraySort(res->inseparable_monitors, &KheInseparableMonitorUntypedCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,                  */
/*    KHE_ATOMIC_TASK_SIG asig2)                                             */
/*                                                                           */
/*  Typed comparison function for sorting an array of atomic task sigs by    */
/*  increasing start time or meet.  Sigs with a start time come first.       */
/*                                                                           */
/*****************************************************************************/

static int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,
  KHE_ATOMIC_TASK_SIG asig2)
{
  int cmp;
  if( asig1->fixed_times )
  {
    if( asig2->fixed_times )
    {
      cmp = KheTimeIndex(asig1->u.first_time)-KheTimeIndex(asig2->u.first_time);
      if( cmp != 0 )  return cmp;
      return asig1->duration - asig2->duration;
    }
    else
      return -1;
  }
  else
  {
    if( asig2->fixed_times )
      return 1;
    else
      return KheMeetSolnIndex(asig1->u.encl_meet) -
	KheMeetSolnIndex(asig2->u.encl_meet);
  }
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

  /* first_time (or meet), duration, and workload_per_time must be equal */
  if( asig1->fixed_times != asig2->fixed_times )
    return false;
  if( asig1->fixed_times )
  {
    if( asig1->u.first_time != asig2->u.first_time )
      return false;
  }
  else
  {
    if( asig1->u.encl_meet != asig2->u.encl_meet )
      return false;
  }
  if( asig1->duration != asig2->duration )
    return false;
  if( asig1->workload_per_time != asig2->workload_per_time )
    return false;

  /* corresponding separable rd monitors must have the same attributes */
  count = HaArrayCount(asig1->separable_rd_monitors);
  if( count != HaArrayCount(asig2->separable_rd_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->separable_rd_monitors, i);
    m2 = HaArray(asig2->separable_rd_monitors, i);
    if( KheSeparableResourceDependentMonitorTypedCmp(m1, m2) != 0 )
      return false;
  }

  /* corresponding inseparable monitors must be identical */
  count = HaArrayCount(asig1->inseparable_monitors);
  if( count != HaArrayCount(asig2->inseparable_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->inseparable_monitors, i);
    m2 = HaArray(asig2->inseparable_monitors, i);
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
      asig->fixed_times ?  KheTimeId(asig->u.first_time) :
      KheMeetId(asig->u.encl_meet), asig->duration, asig->workload_per_time);
    if( HaArrayCount(asig->separable_rd_monitors) > 0 )
    {
      fprintf(fp, "%*s  separable rd monitors:\n", indent, "");
      HaArrayForEach(asig->separable_rd_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    if( HaArrayCount(asig->inseparable_monitors) > 0 )
    {
      fprintf(fp, "%*s  inseparable monitors:\n", indent, "");
      HaArrayForEach(asig->inseparable_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "AtomicTaskSig(%s, %d, %.2f, %d sep rd, %d insep)",
      asig->fixed_times ?  KheTimeId(asig->u.first_time) :
      KheMeetId(asig->u.encl_meet), asig->duration, asig->workload_per_time,
      HaArrayCount(asig->separable_rd_monitors),
      HaArrayCount(asig->inseparable_monitors));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ROOT_TASK_SIG"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigAddTaskAndDescendants(KHE_ROOT_TASK_SIG sig,          */
/*    KHE_TASK task, KHE_MTASK_FINDER mtf, KHE_COST *non_asst_cost,          */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Add atomic signatures for task and its descendants to sig.  Also add     */
/*  in the costs for task and its descendants.                               */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigAddTaskAndDescendants(KHE_ROOT_TASK_SIG sig,
  KHE_TASK task, KHE_MTASK_FINDER mtf, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  int i;  KHE_TASK child_task;  KHE_ATOMIC_TASK_SIG atsig;

  /* do it for task */
  if( KheTaskMeet(task) != NULL && KheTaskEventResource(task) != NULL )
  {
    atsig = KheAtomicTaskSigMake(mtf, task, non_asst_cost, asst_cost);
    HaArrayAddLast(sig->atomic_sigs, atsig);
    sig->total_duration += atsig->duration;
    sig->total_workload += atsig->duration * atsig->workload_per_time;
  }

  /* do it for task's descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheRootTaskSigAddTaskAndDescendants(sig, child_task, mtf, non_asst_cost,
      asst_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ROOT_TASK_SIG KheRootTaskSigMake(KHE_MTASK_FINDER mtf,               */
/*    KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Make and return the root task signature of task, a proper root task.     */
/*  Also set its costs.                                                      */
/*                                                                           */
/*  The result can be degenerate (no atomic signatures) if none of the       */
/*  tasks assigned to task directly or indirectly is derived from a meet.    */
/*                                                                           */
/*****************************************************************************/

static KHE_ROOT_TASK_SIG KheRootTaskSigMake(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_ROOT_TASK_SIG res;  KHE_RESOURCE r;

  /* get memory for the new object */
  if( HaArrayCount(mtf->root_task_sig_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->root_task_sig_free_list);
    HaArrayClear(res->atomic_sigs);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->atomic_sigs, mtf->arena);
  }

  /* set its domain, preasst, and is_fixed */
  res->root_domain = KheTaskDomain(task);
  res->root_preasst = (KheTaskIsPreassigned(task, &r) ? r : NULL);
  res->root_asst_is_fixed = KheTaskAssignIsFixed(task);

  /* get the atomic sigs (possibly none), and sort them */
  res->total_duration = 0;
  res->total_workload = 0.0;
  *non_asst_cost = *asst_cost = 0;
  KheRootTaskSigAddTaskAndDescendants(res, task, mtf, non_asst_cost, asst_cost);
  HaArraySort(res->atomic_sigs, &KheAtomicTaskSigUntypedCmp);

  /* all good */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDelete(KHE_ROOT_TASK_SIG sig, KHE_MTASK_FINDER mtf)   */
/*                                                                           */
/*  Delete sig, by adding it and its atomic task sigs to mtf's free lists.   */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDelete(KHE_ROOT_TASK_SIG sig, KHE_MTASK_FINDER mtf)
{
  int i;
  HaArrayAppend(mtf->atomic_task_sig_free_list, sig->atomic_sigs, i);
  HaArrayAddLast(mtf->root_task_sig_free_list, sig);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigIsNonDegenerate(KHE_ROOT_TASK_SIG sig)                */
/*                                                                           */
/*  Return true if sig is non-degenerate (if it has atomic sigs).            */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigIsNonDegenerate(KHE_ROOT_TASK_SIG sig)
{
  return HaArrayCount(sig->atomic_sigs) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigIsDegenerate(KHE_ROOT_TASK_SIG sig)                   */
/*                                                                           */
/*  Return true if sig is degenerate (if it has no atomic sigs).             */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigIsDegenerate(KHE_ROOT_TASK_SIG sig)
{
  return HaArrayCount(sig->atomic_sigs) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigFixedTimes(KHE_ROOT_TASK_SIG sig, int *sort_index)    */
/*                                                                           */
/*  Assuming that sig is non-degenerate, return true if it has fixed times,  */
/*  and also return its sort index.                                          */
/*                                                                           */
/*  Implementation note.  A root task sig has fixed times if its first       */
/*  atomic signature has fixed times, which is equivalent to saying that     */
/*  all of its atomic signatures have fixed times.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigFixedTimes(KHE_ROOT_TASK_SIG sig, int *sort_index)
{
  KHE_ATOMIC_TASK_SIG asig;
  HnAssert(KheRootTaskSigIsNonDegenerate(sig),
    "KheRootTaskSigFixedTimes internal error:  no atomic sigs");
  asig = HaArrayFirst(sig->atomic_sigs);
  if( asig->fixed_times )
    *sort_index = KheTimeIndex(asig->u.first_time);
  else
    *sort_index = KheMeetSolnIndex(asig->u.encl_meet);
  return asig->fixed_times;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigSimilar(KHE_ROOT_TASK_SIG sig1,                       */
/*    KHE_ROOT_TASK_SIG sig2)                                                */
/*                                                                           */
/*  Return true if sig1 and sig2 are similar.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigSimilar(KHE_ROOT_TASK_SIG sig1,
  KHE_ROOT_TASK_SIG sig2)
{
  int i;  KHE_ATOMIC_TASK_SIG asig1, asig2;

  /* domains and preassts must be equal */
  if( !KheResourceGroupEqual(sig1->root_domain, sig2->root_domain) )
    return false;
  if( sig1->root_preasst != sig2->root_preasst )
    return false;

  /* neither task may have a fixed asst */
  if( sig1->root_asst_is_fixed || sig2->root_asst_is_fixed )
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
/*  KHE_TIME KheRootTaskSigStartTime(KHE_ROOT_TASK_SIG sig)                  */
/*                                                                           */
/*  Return the start time of sig, taken from its first atomic sig, or        */
/*  NULL if there is no assigned time.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_TIME KheRootTaskSigStartTime(KHE_ROOT_TASK_SIG sig)
{
  KHE_ATOMIC_TASK_SIG asig;
  asig = HaArrayFirst(sig->atomic_sigs);
  if( asig->sort_type == KHE_SORT_TYPE_FIRST_TIME )
    return asig->u.first_time;
  else
    return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDebugHeader(KHE_ROOT_TASK_SIG sig, FILE *fp)          */
/*                                                                           */
/*  Debug print of the header part of sig.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDebugHeader(KHE_ROOT_TASK_SIG sig, FILE *fp)
{
  fprintf(fp, "TaskSig(");
  KheResourceGroupDebug(sig->root_domain, 1, -1, fp);
  if( sig->root_preasst != NULL )
    fprintf(fp, ", preasst %s", KheResourceId(sig->root_preasst));
  if( sig->root_asst_is_fixed )
    fprintf(fp, ", fixed");
  fprintf(fp, ", %d atomic sigs)", HaArrayCount(sig->atomic_sigs));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDebug(KHE_ROOT_TASK_SIG sig, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the sig onto fp with the given verbosity and indent.      */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDebug(KHE_ROOT_TASK_SIG sig, int verbosity,
  int indent, FILE *fp)
{
  KHE_ATOMIC_TASK_SIG asig;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheRootTaskSigDebugHeader(sig, fp);
    fprintf(fp, "\n");
    HaArrayForEach(sig->atomic_sigs, asig, i)
      KheAtomicTaskSigDebug(asig, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheRootTaskSigDebugHeader(sig, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_TIME"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_TIME KheMTaskTimeMake(KHE_TIME time,                           */
/*    float workload_per_time, KHE_MTASK_FINDER mtf)                         */
/*                                                                           */
/*  Make a new mtask time object with these attributes.                      */
/*                                                                           */
/*  NB There is no KheMTaskTimeDelete, because deletion is done by           */
/*  appending the mtask's array of mtask times to the free list.             */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_TIME KheMTaskTimeMake(KHE_TIME time,
  float workload_per_time, KHE_MTASK_FINDER mtf)
{
  KHE_MTASK_TIME res;
  if( HaArrayCount(mtf->mtask_time_free_list) > 0 )
    res = HaArrayLastAndDelete(mtf->mtask_time_free_list);
  else
    HaMake(res, mtf->arena);
  res->time = time;
  res->workload_per_time = workload_per_time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK - construction"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskMake(KHE_MTASK_FINDER mtf, KHE_ROOT_TASK_SIG sig)      */
/*                                                                           */
/*  Make a new mtask object, suitable for holding tasks with signature       */
/*  sig, but initially empty.  This will always be followed by adding        */
/*  a first task to the result, so in fact mtasks are never empty.           */
/*                                                                           */
/*  NB This builds the mtask object, it does not add it to mtf's structures. */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK KheMTaskMake(KHE_MTASK_FINDER mtf, KHE_ROOT_TASK_SIG sig)
{
  KHE_MTASK res;  KHE_ATOMIC_TASK_SIG asig;  KHE_MTASK_TIME ct;
  int i, j, bi, sort_index, first_day_index;  KHE_TIME busy_time;

  /* get the basic object and initialize its arrays */
  if( HaArrayCount(mtf->mtask_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->mtask_free_list);
    HaArrayClear(res->internal_mtask_sets);
    HaArrayClear(res->tasks_and_costs);
    HaArrayClear(res->fixed_times_by_day);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->internal_mtask_sets, mtf->arena);
    HaArrayInit(res->tasks_and_costs, mtf->arena);
    HaArrayInit(res->fixed_times_by_day, mtf->arena);
  }

  /* initialize basic fields */
  res->mtask_finder = mtf;
  res->mtasks_index = -1;
  res->sig = sig;
  res->asst_count = 0;

  /* set fixed_times (includes non-degeneracy) */
  res->fixed_times = KheRootTaskSigIsNonDegenerate(sig) &&
    KheRootTaskSigFixedTimes(sig, &sort_index);

  /* if fixed_times, set the other fields related to assigned times */
  if( res->fixed_times )
  {
    /* initialize fixed_times_by_day and fixed_times_no_overlap */
    asig = HaArrayFirst(sig->atomic_sigs);
    first_day_index = KheFrameTimeIndex(mtf->days_frame, asig->u.first_time);
    res->fixed_times_no_overlap = true;
    HaArrayForEach(sig->atomic_sigs, asig, i)
    {
      for( j = 0;  j < asig->duration;  j++ )
      {
	busy_time = KheTimeNeighbour(asig->u.first_time, j);
	bi = KheFrameTimeIndex(mtf->days_frame, busy_time) - first_day_index;
	HnAssert(bi >= 0, "KheMTaskMake internal error (bi < 0)");
	HaArrayFill(res->fixed_times_by_day, bi + 1, NULL);
	if( HaArray(res->fixed_times_by_day, bi) == NULL )
	{
	  ct = KheMTaskTimeMake(busy_time, asig->workload_per_time, mtf);
	  HaArrayPut(res->fixed_times_by_day, bi, ct);
	}
	else
	  res->fixed_times_no_overlap = false;
      }
    }

    /* initialize fixed_times_interval from fixed_times_by_day */
    res->fixed_times_interval = KheIntervalMake(first_day_index,
      first_day_index + HaArrayCount(res->fixed_times_by_day) - 1);

    /* initialize fixed_times_no_gaps */
    res->fixed_times_no_gaps = true;
    HaArrayForEach(res->fixed_times_by_day, ct, bi)
      if( ct == NULL )
	res->fixed_times_no_gaps = false;
  }
  else
  {
    /* initialize no_overlap etc. when not res->fixed_times */
    res->fixed_times_no_overlap = false;
    res->fixed_times_no_gaps = false;
    res->fixed_times_interval = KheIntervalMake(0, -1);
  }
  res->fixed_times_time_set = NULL;   /* made on demand */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDelete(KHE_MTASK mt)                                        */
/*                                                                           */
/*  Delete mt.                                                               */
/*                                                                           */
/*  NB This deletes the object, it does not remove it from mtf's structures. */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskDelete(KHE_MTASK mt)
{
  KHE_MTASK_FINDER mtf;  int i;
  mtf = mt->mtask_finder;
  KheRootTaskSigDelete(mt->sig, mtf);
  HaArrayAppend(mtf->task_and_cost_free_list, mt->tasks_and_costs, i);
  HaArrayAppend(mtf->mtask_time_free_list, mt->fixed_times_by_day, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskContainsTask(KHE_MTASK mt, KHE_TASK task)                   */
/*                                                                           */
/*  Return true if mt contains task.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskContainsTask(KHE_MTASK mt, KHE_TASK task)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEach(mt->tasks_and_costs, tac, i)
    if( tac->task == task )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFixAssignments(KHE_MTASK mt, int pos)                       */
/*                                                                           */
/*  Check whether there is a problem with the assignments at position        */
/*  pos of mt, which must exist.  If there is, fix it.                       */
/*                                                                           */
/*  There is a problem if the task at pos is assigned but the one            */
/*  before it is unassigned, or if the task at pos is unassigned but         */
/*  the one after it is assigned.  But we don't need to check these          */
/*  tasks, because we can use mt->asst_count to work out where we are:       */
/*                                                                           */
/*    * From 0 inclusive to mt->asst_count exclusive, all are assigned;      */
/*                                                                           */
/*    * From mt->asst_count inclusive to HaArrayCount(mt->tasks_and_costs)   */
/*      exclusive, all are unassigned.                                       */
/*                                                                           */
/*  KheMTaskFixAssignments is about preserving this invariant, given         */
/*  that it may be violated at pos.                                          */
/*                                                                           */
/*  There is no need to call KheMTaskBringAsstCountUpToDate(mt), because     */
/*  KheMTaskFixAssignments is called only from KheMTaskAddTask, which is     */
/*  called only from KheMTaskFinderAddTask, which is called only from        */
/*  KheMTaskFinderMake.  Indeed it would be wrong to call it here.           */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFixAssignments(KHE_MTASK mt, int pos)
{
  KHE_TASK_AND_COST pos_tac, other_tac;  KHE_RESOURCE r;
  pos_tac = HaArray(mt->tasks_and_costs, pos);
  if( KheTaskAsstResource(pos_tac->task) != NULL )
  {
    if( pos > mt->asst_count )
    {
      /* move the assignment at pos to the gap at mt->asst_count */
      other_tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      r = KheTaskAsstResource(pos_tac->task);
      if( !KheTaskUnAssignResource(pos_tac->task) ||
	  !KheTaskAssignResource(other_tac->task, r) )
	HnAbort("KheMTaskFixAssignments internal error 1");
    }

    /* one more assignment now */
    mt->asst_count++;
  }
  else
  {
    if( pos < mt->asst_count )
    {
      /* move the assignment at mt->asst_count to the gap at pos */
      /* NB the last assignment was initially at mt->asst_count - 1, but a */
      /* new entry has been inserted at pos and shifted it to mt->asst_count */
      other_tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      r = KheTaskAsstResource(other_tac->task);
      HnAssert(r != NULL, "KheMTaskFixAssignments internal error 2");
      if( !KheTaskUnAssignResource(other_tac->task) ||
	  !KheTaskAssignResource(pos_tac->task, r) )
	HnAbort("KheMTaskFixAssignments internal error 3");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskAddTask(KHE_MTASK mt, KHE_TASK task,                        */
/*    KHE_COST non_asst_cost, KHE_COST asst_cost)                            */
/*                                                                           */
/*  Add task (with the given costs) to mt.                                   */
/*                                                                           */
/*  NB This adds task to mt, it does not update mtf's data structures.       */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskAddTask(KHE_MTASK mt, KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost)
{
  KHE_TASK_AND_COST tac;  int i;  KHE_MTASK_FINDER mtf;
  mtf = mt->mtask_finder;
  HaArrayForEachReverse(mt->tasks_and_costs, tac, i)
    if( tac->non_asst_cost - tac->asst_cost >= non_asst_cost - asst_cost )
      break;
  HaArrayAdd(mt->tasks_and_costs, i + 1,
    KheTaskAndCostMake(task, non_asst_cost, asst_cost, mtf));
  KheMTaskFixAssignments(mt, i + 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDeleteTask(KHE_MTASK mt, KHE_TASK task)                     */
/*                                                                           */
/*  Delete task from mt.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskDeleteTask(KHE_MTASK mt, KHE_TASK task)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEach(mt->tasks_and_costs, tac, i)
    if( tac->task == task )
    {
      HaArrayDeleteAndShift(mt->tasks_and_costs, i);
      KheTaskAndCostDelete(tac, mt->mtask_finder);
      if( KheTaskAsstResource(task) != NULL )
	mt->asst_count--;
      return;
    }
  HnAbort("KheMTaskDeleteTask internal error (task not present)");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK - queries"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheMTaskId(KHE_MTASK mt)                                           */
/*                                                                           */
/*  Return an Id for mt.                                                     */
/*                                                                           */
/*****************************************************************************/

char *KheMTaskId(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;
  tac = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskId(tac->task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheMTaskResourceType(KHE_MTASK mt)                     */
/*                                                                           */
/*  Return the resource type of mt.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheMTaskResourceType(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;
  tac = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskResourceType(tac->task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskIsPreassigned(KHE_MTASK mt, KHE_RESOURCE *r)                */
/*                                                                           */
/*  If mt is preassigned, return true with *r set to the resource.           */
/*  Otherwise return false with *r set to NULL.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskIsPreassigned(KHE_MTASK mt, KHE_RESOURCE *r)
{
  *r = mt->sig->root_preasst;
  return (*r != NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAssignIsFixed(KHE_MTASK mt)                                 */
/*                                                                           */
/*  If the assignments of the tasks of mt are fixed, return true, else false.*/
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAssignIsFixed(KHE_MTASK mt)
{
  return mt->sig->root_asst_is_fixed;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheMTaskDomain(KHE_MTASK mt)                          */
/*                                                                           */
/*  Return the domain shared by the tasks of mt.                             */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheMTaskDomain(KHE_MTASK mt)
{
  return mt->sig->root_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTotalDuration(KHE_MTASK mt)                                  */
/*                                                                           */
/*  Return the total duration of mt.                                         */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTotalDuration(KHE_MTASK mt)
{
  return mt->sig->total_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheMTaskTotalWorkload(KHE_MTASK mt)                                */
/*                                                                           */
/*  Return the total workload of mt.                                         */
/*                                                                           */
/*****************************************************************************/

float KheMTaskTotalWorkload(KHE_MTASK mt)
{
  return mt->sig->total_workload;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskHasSoleMeet(KHE_MTASK mt, KHE_MEET *meet)                   */
/*                                                                           */
/*  If the tasks of mt all come from the same meet, set *meet to that meet   */
/*  and return true.  Otherwise set *meet to NULL and return false.          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskHasSoleMeet(KHE_MTASK mt, KHE_MEET *meet)
{
  KHE_TASK_AND_COST tc;  int i;  KHE_MEET mt_meet;
  tc = HaArrayFirst(mt->tasks_and_costs);
  mt_meet = KheTaskMeet(tc->task);
  for( i = 1;  i < HaArrayCount(mt->tasks_and_costs);  i++ )
  {
    tc = HaArray(mt->tasks_and_costs, i);
    if( mt_meet != KheTaskMeet(tc->task) )
      return *meet = NULL, false;
  }
  return *meet = mt_meet, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskHasFixedTimes(KHE_MTASK mt)                                 */
/*                                                                           */
/*  Return true if mt has fixed times, including non-degeneracy.             */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskHasFixedTimes(KHE_MTASK mt)
{
  return mt->fixed_times;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskInterval(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the interval covered by mt.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheMTaskInterval(KHE_MTASK mt)
{
  return mt->fixed_times_interval;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheMTaskDayTime(KHE_MTASK mt, int day_index,                    */
/*    float *workload_per_time)                                              */
/*                                                                           */
/*  Return one time (the only one if KheMTaskNoOverlap returns true)         */
/*  when mt is busy on day day_index.  Also return the workload per time     */
/*  then.  If mt is not busy on day_index, return NULL (0 for workload).     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheMTaskDayTime(KHE_MTASK mt, int day_index, float *workload_per_time)
{
  KHE_MTASK_TIME ct;
  if( !KheIntervalContains(mt->fixed_times_interval, day_index) )
    return *workload_per_time = 0, NULL;
  ct = HaArray(mt->fixed_times_by_day,
    day_index - KheIntervalFirst(mt->fixed_times_interval));
  if( ct == NULL )
    return *workload_per_time = 0, NULL;
  else
    return *workload_per_time = ct->workload_per_time, ct->time;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheMTaskMakeTimeSet(KHE_MTASK mt)                           */
/*                                                                           */
/*  Make the time set of times covered by mt.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_SET KheMTaskMakeTimeSet(KHE_MTASK mt)
{
  KHE_ATOMIC_TASK_SIG asig;  int i, j;  KHE_TIME_SET res;
  KHE_INSTANCE ins;  KHE_TIME time;
  ins = KheSolnInstance(mt->mtask_finder->soln);
  res = KheTimeSetMake(ins, mt->mtask_finder->arena);
  HaArrayForEach(mt->sig->atomic_sigs, asig, i)
    if( asig->fixed_times )
      for( j = 0;  j < asig->duration;  j++ )
      {
	time = KheTimeNeighbour(asig->u.first_time, j);
	KheTimeSetAddTime(res, time);
      }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheMTaskTimeSet(KHE_MTASK mt)                               */
/*                                                                           */
/*  Return the set of times that m is running.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_SET KheMTaskTimeSet(KHE_MTASK mt)
{
  if( mt->fixed_times_time_set == NULL )
    mt->fixed_times_time_set = KheMTaskMakeTimeSet(mt);
  return mt->fixed_times_time_set;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNoOverlap(KHE_MTASK mt)                                     */
/*                                                                           */
/*  Return true if there are no cases where two of the shared times of       */
/*  the tasks of mt overlap with a single day of days_frame.                 */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNoOverlap(KHE_MTASK mt)
{
  return mt->fixed_times_no_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNoGaps(KHE_MTASK mt)                                        */
/*                                                                           */
/*  Return true when there are no gaps in the days covered by the tasks      */
/*  of mt.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNoGaps(KHE_MTASK mt)
{
  return mt->fixed_times_no_gaps;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTaskCount(KHE_MTASK mt)                                      */
/*                                                                           */
/*  Return the number of tasks in mt.                                        */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTaskCount(KHE_MTASK mt)
{
  return HaArrayCount(mt->tasks_and_costs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheMTaskTask(KHE_MTASK mt, int i, KHE_COST *non_asst_cost,      */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Return the i'th task of mt.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheMTaskTask(KHE_MTASK mt, int i, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  KHE_TASK_AND_COST tac;
  tac = HaArray(mt->tasks_and_costs, i);
  *non_asst_cost = tac->non_asst_cost;
  *asst_cost = tac->asst_cost;
  return tac->task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost, */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Carry out the actual work of KheTaskNonAsstAndAsstCost.                  */
/*                                                                           */
/*  Implementation note.  This function calls KheMonitorKind, making it      */
/*  consistent with other calculations of non-assignment and assignment      */
/*  cost in this module.  However it does not use the KHE_MONITOR_KIND       */
/*  value returned by KheMonitorKind.                                        */
/*                                                                           */
/*****************************************************************************/

void KheTaskDoNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  KHE_TASK child_task;  int i, durn;  KHE_SOLN soln;  KHE_MONITOR m;
  KHE_EVENT_RESOURCE er;

  /* do it for task itself */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  durn = KheTaskDuration(task);
  for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(soln, er, i);
    if( KheMonitorAttachedToSoln(m) && KheMonitorCombinedWeight(m) > 0 )
      KheMonitorKind(m, durn, non_asst_cost, asst_cost);
  }

  /* do it for the tasks assigned to task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskDoNonAsstAndAsstCost(child_task, non_asst_cost, asst_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,   */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Find the non-assignment and assignment cost of task, including all       */
/*  tasks assigned directly or indirectly to task.                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  *non_asst_cost = *asst_cost = 0;
  KheTaskDoNonAsstAndAsstCost(task, non_asst_cost, asst_cost);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK - assignment"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskBringAsstCountUpToDate(KHE_MTASK mt)                        */
/*                                                                           */
/*  Bring the asst_count field of mt up to date.  It can get out of date     */
/*  when marks and paths are used to undo operations on mt's tasks.          */
/*                                                                           */
/*  As usual, mt->asst_count is the number of tasks with a resource          */
/*  assigned to them, or equivalently the index in mt->tasks_and_costs       */
/*  of the first unassigned task.  Even when mt->asst_count is out of        */
/*  date, we still assume that all the assigned tasks are at the front.      */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskBringAsstCountUpToDate(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;

  /* decrease mt->asst_count while it is too large */
  while( mt->asst_count > 0 )
  {
    tac = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
    if( KheTaskAsstResource(tac->task) != NULL )
      break;
    mt->asst_count--;
  }

  /* increase mt->asst_count while it is too small */
  while( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
  {
    tac = HaArray(mt->tasks_and_costs, mt->asst_count);
    if( KheTaskAsstResource(tac->task) == NULL )
      break;
    mt->asst_count++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFindTask(KHE_MTASK mt, KHE_RESOURCE r, KHE_TASK *task)      */
/*                                                                           */
/*  If mt contains a task assigned r (possibly NULL), return true with       */
/*  *task set to its first such task.  Otherwise return false with *task     */
/*  set to NULL.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFindTask(KHE_MTASK mt, KHE_RESOURCE r, KHE_TASK *task,
  int *pos)
{
  int i;  KHE_TASK_AND_COST tac;
  KheMTaskBringAsstCountUpToDate(mt);
  if( r != NULL )
  {
    /* non-NULL resource must be in first mt->asst_count entries */
    /* searching backwards is likely to be faster */
    for( i = mt->asst_count - 1;  i >= 0;  i-- )
    {
      tac = HaArray(mt->tasks_and_costs, i);
      if( KheTaskAsstResource(tac->task) == r )
	return *task = tac->task, *pos = i, true;
    }
  }
  else
  {
    /* NULL resource must be just after the first mt->asst_count entries */
    if( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
    {
      tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      HnAssert(KheTaskAsstResource(tac->task) == NULL,
	"KheMTaskFindTask internal error");
      return *task = tac->task, *pos = mt->asst_count, true;
    }
  }
  return *task = NULL, *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskMoveResourceCheck(KHE_MTASK mt, KHE_RESOURCE from_r,        */
/*    KHE_RESOURCE to_r, bool disallow_preassigned)                          */
/*                                                                           */
/*  Check whether the move of from_r to to_r within mt is possible.  If      */
/*  disallow_preassigned is true, always say no when mt is preassigned.      */
/*                                                                           */
/*  Here is proof that the various reasons for failure are tested here:      */
/*                                                                           */
/*    the move would change nothing, because from_r == to_r.  This is        */
/*    checked by KheTaskMoveCheck, called by KheTaskMoveResourceCheck.       */
/*                                                                           */
/*    mt contains only preassigned or fixed tasks; their assignments         */
/*    cannot change.  This is checked by KheTaskMoveCheck, directly          */
/*    for fixed tasks, and indirectly via domains for preassigned tasks.     */
/*    When disallow_preassigned is true, all changes to the assignments      */
/*    of preassigned tasks are disallowed; when it is false, some changes    */
/*    are allowed, including unassignments.                                  */
/*                                                                           */
/*    from_r != NULL and from_r is not one of the resources currently        */
/*    assigned to mt.  This is checked by KheMTaskFindTask.                  */
/*                                                                           */
/*    to_r != NULL and the domain of mt (the same for all its tasks) does    */
/*    not contain to_r.  This is checked by KheTaskMoveCheck.                */
/*                                                                           */
/*    from_r == NULL (and therefore to_r != NULL) and mt does not contain    */
/*    at least one currently unassigned task to assign to_r to.  This is     */
/*    checked by KheMTaskFindTask.                                           */
/*                                                                           */
/*  And this is everything checked by KheMTaskFindTask and KheTaskMoveCheck. */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskMoveResourceCheck(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned)
{
  KHE_TASK task;  int pos;  KHE_RESOURCE r2;
  return KheMTaskFindTask(mt, from_r, &task, &pos) &&
    !(disallow_preassigned && KheTaskIsPreassigned(task, &r2)) &&
    KheTaskMoveResourceCheck(task, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskMoveResource(KHE_MTASK mt, KHE_RESOURCE from_r,             */
/*    KHE_RESOURCE to_r, bool disallow_preassigned)                          */
/*                                                                           */
/*  Move from_r to to_r within mt.                                           */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskMoveResource(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned)
{
  KHE_TASK task;  int pos;  KHE_TASK_AND_COST pos_tac, other_tac;
  KHE_RESOURCE r, r2;
  if( KheMTaskFindTask(mt, from_r, &task, &pos) &&
    !(disallow_preassigned && KheTaskIsPreassigned(task, &r2)) &&
    KheTaskMoveResource(task, to_r) )
  {
    if( from_r == NULL )
    {
      /* we've added one assignment, which always goes to the right pos */
      mt->asst_count++;
    }
    else if( to_r == NULL )
    {
      /* we've removed one assignment, which might need fixing up */
      if( pos < mt->asst_count - 1 )
      {
	/* move the assignment at mt->asst_count - 1 to the gap at pos */
	pos_tac = HaArray(mt->tasks_and_costs, pos);
	other_tac = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
	r = KheTaskAsstResource(other_tac->task);
	HnAssert(r != NULL, "KheMTaskMoveResource internal error 1");
	if( !KheTaskUnAssignResource(other_tac->task) ||
	    !KheTaskAssignResource(pos_tac->task, r) )
	  HnAbort("KheMTaskMoveResource internal error 2");
      }
      mt->asst_count--;
    }
    else
    {
      /* we've changed one resource to another, which is always fine as is */
    }
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSuggestion(KHE_TASK task, KHE_TASK root_task,                */
/*    KHE_RESOURCE *to_r)                                                    */
/*                                                                           */
/*  If task or its descendants generates a suggestion, set *to_r to that     */
/*  suggestion and return true.  Otherwise leave *to_r alone and return      */
/*  false.                                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSuggestion(KHE_TASK task, KHE_TASK root_task,
  KHE_RESOURCE *to_r)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_RESOURCE r;

  /* see whether task itself produces a suggestion */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
    {
      r = KheTaskAsstResource(KheEventResourceTask(soln, er, i));
      if( r != NULL && KheTaskAssignResourceCheck(root_task, r) )
	return *to_r = r, true;
    }

  /* see whether any tasks assigned to task produce a suggestion */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    if( KheTaskSuggestion(KheTaskAssignedTo(task, i), root_task, to_r) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceAssignSuggestion(KHE_MTASK mt, KHE_RESOURCE *to_r)  */
/*                                                                           */
/*  If a suggestion for which resource to assign next to mt can be made,     */
/*  return true with *to_r set to that suggestion.  Else return false with   */
/*  *to_r set to NULL.                                                       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskResourceAssignSuggestion(KHE_MTASK mt, KHE_RESOURCE *to_r)
{
  KHE_TASK task;  int pos;
  *to_r = NULL;
  return KheMTaskFindTask(mt, NULL, &task, &pos) &&
    KheTaskSuggestion(task, task, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskAsstResourceCount(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the number of resources assigned to mt.                           */
/*                                                                           */
/*****************************************************************************/

int KheMTaskAsstResourceCount(KHE_MTASK mt)
{
  KheMTaskBringAsstCountUpToDate(mt);
  return mt->asst_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheMTaskAsstResource(KHE_MTASK mt, int i)                   */
/*                                                                           */
/*  Return the i'th resource assigned to mt.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheMTaskAsstResource(KHE_MTASK mt, int i)
{
  KHE_TASK_AND_COST tc;
  tc = HaArray(mt->tasks_and_costs, i);
  return KheTaskAsstResource(tc->task);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskAssignedTaskCount(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the number of assigned tasks in mt.                               */
/*                                                                           */
/*****************************************************************************/

int KheMTaskAssignedTaskCount(KHE_MTASK mt)
{
  KheMTaskBringAsstCountUpToDate(mt);
  return mt->asst_count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskUnassignedTaskCount(KHE_MTASK mt)                            */
/*                                                                           */
/*  Return the number of unassigned tasks in mt.                             */
/*                                                                           */
/*****************************************************************************/

int KheMTaskUnassignedTaskCount(KHE_MTASK mt)
{
  KheMTaskBringAsstCountUpToDate(mt);
  return HaArrayCount(mt->tasks_and_costs) - mt->asst_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheMTaskFirstUnassignedTask(KHE_MTASK mt,                       */
/*    KHE_COST *non_asst_cost, KHE_COST *asst_cost)                          */
/*                                                                           */
/*  Return the first unassigned task of mt, together with its                */
/*  non-assignment cost and assignment cost.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** not needed
KHE_TASK KheMTaskFirstUnassignedTask(KHE_MTASK mt,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_TASK_AND_COST tac;
  KheMTaskBringAsstCountUpToDate(mt);
  HnAssert(KheMTaskUnassignedTaskCount(mt) > 0,
    "KheMTaskFirstUnassignedTask: no unassigned tasks");
  tac = HaArray(mt->tasks_and_costs, mt->asst_count);
  *non_asst_cost = tac->non_asst_cost;
  *asst_cost = tac->asst_cost;
  return tac->task;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNeedsAssignment(KHE_MTASK mt)                               */
/*                                                                           */
/*  Return true if mt needs assignment, because it contains an unassigned    */
/*  task for which non_asst_cost - asst_cost > 0, so that assigning this     */
/*  task will reduce solution cost.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNeedsAssignment(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  KheMTaskBringAsstCountUpToDate(mt);
  if( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
  {
    tc = HaArray(mt->tasks_and_costs, mt->asst_count);
    return tc->non_asst_cost - tc->asst_cost > 0;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskContainsNeedlessAssignment(KHE_MTASK mt)                    */
/*                                                                           */
/*  Return true if one of the assignments to mt could be removed without     */
/*  increasing solution cost.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskContainsNeedlessAssignment(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  KheMTaskBringAsstCountUpToDate(mt);
  if( mt->asst_count > 0 )
  {
    /* find the last assignment */
    tc = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
    return tc->non_asst_cost - tc->asst_cost <= 0;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK - task bounds"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAddTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)          */
/*                                                                           */
/*  Check that tb can be added to mt.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAddTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskAddTaskBoundCheck(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAddTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)               */
/*                                                                           */
/*  Try to add tb to mt, returning true if successful.                       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAddTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskAddTaskBound(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskDeleteTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)       */
/*                                                                           */
/*  Check that tb can be deleted from mt.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskDeleteTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskDeleteTaskBoundCheck(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskDeleteTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)            */
/*                                                                           */
/*  Try to delete tb from mt, returning true if successful.                  */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskDeleteTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskDeleteTaskBound(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTaskBoundCount(KHE_MTASK mt)                                 */
/*                                                                           */
/*  Return the number of task bounds in mt.                                  */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTaskBoundCount(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  tc = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskTaskBoundCount(tc->task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheMTaskTaskBound(KHE_MTASK mt, int i)                    */
/*                                                                           */
/*  Return the i'th task bound of mt.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheMTaskTaskBound(KHE_MTASK mt, int i)
{
  KHE_TASK_AND_COST tc;
  tc = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskTaskBound(tc->task, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDebugHeader(KHE_MTASK mt, FILE *fp)                         */
/*                                                                           */
/*  Print the header part of the debug of mt onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskDebugDayRange(KHE_MTASK mt, FILE *fp)
{
  fprintf(fp, "%s", KheIntervalShow(mt->fixed_times_interval,
    mt->mtask_finder->days_frame));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDebug(KHE_MTASK mt, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of mt onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskDebug(KHE_MTASK mt, int verbosity, int indent, FILE *fp)
{
  KHE_TASK_AND_COST tac;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ MTask ", indent, "");
    KheMTaskDebugDayRange(mt, fp);
    fprintf(fp, "\n");
    if( verbosity >= 3 )
      KheRootTaskSigDebug(mt->sig, verbosity, indent + 2, fp);
    HaArrayForEach(mt->tasks_and_costs, tac, i)
    {
      fprintf(fp, "%*s  n%.5f a%.5f ", indent, "",
	KheCostShow(tac->non_asst_cost), KheCostShow(tac->asst_cost));
      KheTaskDebug(tac->task, verbosity, 0, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else if( verbosity == 1 )
  {
    fprintf(fp, "%s", KheMTaskId(mt));
  }
  else
  {
    fprintf(fp, "MTask(");
    KheMTaskDebugDayRange(mt, fp);
    HaArrayForEach(mt->tasks_and_costs, tac, i)
    {
      fprintf(fp, ", ");
      KheTaskDebug(tac->task, verbosity, -1, fp);
    }
    fprintf(fp, ")");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_SET"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskSetMake(KHE_MTASK_FINDER mtf)                      */
/*                                                                           */
/*  Make a new, empty mtask set.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskSetMake(KHE_MTASK_FINDER mtf)
{
  KHE_MTASK_SET res;
  if( HaArrayCount(mtf->mtask_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->mtask_set_free_list);
    HaArrayClear(res->mtasks);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->mtasks, mtf->arena);
  }
  res->interval = KheIntervalMake(0, -1);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDelete(KHE_MTASK_SET mts, KHE_MTASK_FINDER mtf)          */
/*                                                                           */
/*  Delete mtask set mts.  It goes on the free list in mtf.                  */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDelete(KHE_MTASK_SET mts, KHE_MTASK_FINDER mtf)
{
  HaArrayAddLast(mtf->mtask_set_free_list, mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetClear(KHE_MTASK_SET mts)                                 */
/*                                                                           */
/*  Clear mts back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetClear(KHE_MTASK_SET mts)
{
  HaArrayClear(mts->mtasks);
  mts->interval = KheIntervalMake(0, -1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetResetInterval(KHE_MTASK_SET mts)                         */
/*                                                                           */
/*  Reset mts->interval to the correct value for mts->mtasks.  This is       */
/*  necessary after one or more mtasks are deleted.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetResetInterval(KHE_MTASK_SET mts)
{
  KHE_MTASK mt;  int i;
  mts->interval = KheIntervalMake(0, -1);
  HaArrayForEach(mts->mtasks, mt, i)
    mts->interval = KheIntervalUnion(mts->interval, KheMTaskInterval(mt));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetClearFromEnd(KHE_MTASK_SET mts, int count)               */
/*                                                                           */
/*  Clear mts from the end until count elements remain.                      */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetClearFromEnd(KHE_MTASK_SET mts, int count)
{
  while( HaArrayCount(mts->mtasks) > count )
    HaArrayDeleteLast(mts->mtasks);
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDropFromEnd(KHE_MTASK_SET mts, int n)                    */
/*                                                                           */
/*  Drop the last n elements of mts from the end.                            */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDropFromEnd(KHE_MTASK_SET mts, int n)
{
  while( n > 0 )
  {
    HaArrayDeleteLast(mts->mtasks);
    n--;
  }
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetAddMTask(KHE_MTASK_SET mts, KHE_MTASK mt)                */
/*                                                                           */
/*  Add mt to mts.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetAddMTask(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  HaArrayAddLast(mts->mtasks, mt);
  mts->interval = KheIntervalUnion(mts->interval, KheMTaskInterval(mt));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAddMTaskDisjoint(KHE_MTASK_SET mts, KHE_MTASK mt)        */
/*                                                                           */
/*  If mt does not overlap mts's interval, add it to mts and return true.    */
/*  Otherwise change nothing and return false.                               */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskSetAddMTaskDisjoint(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  if( KheIntervalDisjoint(mts->interval, KheMTaskInterval(mt)) )
  {
    KheMTaskSetAddMTask(mts, mt);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetAddMTaskSet(KHE_MTASK_SET dst_mts, KHE_MTASK_SET src_mts)*/
/*                                                                           */
/*  Add the mtasks of src_mts to dst_mts, without disturbing src_mts.        */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetAddMTaskSet(KHE_MTASK_SET dst_mts, KHE_MTASK_SET src_mts)
{
  int i;
  HaArrayAppend(dst_mts->mtasks, src_mts->mtasks, i);
  dst_mts->interval = KheIntervalUnion(dst_mts->interval, src_mts->interval);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAddMTaskSetDisjoint(KHE_MTASK_SET mts,                   */
/*    KHE_MTASK_SET mts2)                                                    */
/*                                                                           */
/*  If mts2 does not overlap mts's interval, add it to mts and return true.  */
/*  Otherwise change nothing and return false.                               */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskSetAddMTaskSetDisjoint(KHE_MTASK_SET mts, KHE_MTASK_SET mts2)
{
  if( KheIntervalDisjoint(mts->interval, mts2->interval) )
  {
    KheMTaskSetAddMTaskSet(mts, mts2);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDeleteMTask(KHE_MTASK_SET mts, KHE_MTASK mt)             */
/*                                                                           */
/*  Find and delete mt from mts.  It must be present.                        */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDeleteMTask(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  int pos;
  if( HaArrayContains(mts->mtasks, mt, &pos) )
    HaArrayDeleteAndPlug(mts->mtasks, pos);
  else
    HnAbort("KheMTaskSetDeleteMTask mt not in mts");
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetLastAndDelete(KHE_MTASK_SET mts)                    */
/*                                                                           */
/*  Assuming mts is non-empty, delete and return the last element.           */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetLastAndDelete(KHE_MTASK_SET mts)
{
  KHE_MTASK res;
  res = HaArrayLastAndDelete(mts->mtasks);
  KheMTaskSetResetInterval(mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetContainsMTask(KHE_MTASK_SET mts, KHE_MTASK mt, int *pos) */
/*                                                                           */
/*  If mts contains mt, set *pos to its position and return true.  Otherwise */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetContainsMTask(KHE_MTASK_SET mts, KHE_MTASK mt, int *pos)
{
  return HaArrayContains(mts->mtasks, mt, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskSetMTaskCount(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Return the number of mtasks in mts.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMTaskSetMTaskCount(KHE_MTASK_SET mts)
{
  return HaArrayCount(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetMTask(KHE_MTASK_SET mts, int i)                     */
/*                                                                           */
/*  Return the i'th mtask of mts.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetMTask(KHE_MTASK_SET mts, int i)
{
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetFirst(KHE_MTASK_SET mts)                            */
/*                                                                           */
/*  Assuming that mts is non-empty, return its first element.                */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetFirst(KHE_MTASK_SET mts)
{
  HnAssert(HaArrayCount(mts->mtasks) > 0, "KheMTaskSetFirst:  mts is empty");
  return HaArrayFirst(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetLast(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Assuming that mts is non-empty, return its last element.                 */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetLast(KHE_MTASK_SET mts)
{
  HnAssert(HaArrayCount(mts->mtasks) > 0, "KheMTaskSetLast:  mts is empty");
  return HaArrayLast(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetSort(KHE_MTASK_SET mts,                                  */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the mtasks of mts using comparison function mts.                    */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetSort(KHE_MTASK_SET mts,
  int(*compar)(const void *, const void *))
{
  HaArraySort(mts->mtasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskSetIncreasingFirstTaskCmp(const void *x1, const void *x2)    */
/*                                                                           */
/*  Comparison function for sorting an array of mtasks so as to bring        */
/*  multiple occurrences of the same mtask together.                         */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskSetIncreasingFirstTaskCmp(const void *x1, const void *x2)
{
  KHE_MTASK mt1, mt2;  KHE_TASK task1, task2;  KHE_COST junk1, junk2;
  KHE_MTASK_TIME mt_time1, mt_time2;  KHE_TIME time1, time2;
  mt1 = * (KHE_MTASK *) x1;
  mt2 = * (KHE_MTASK *) x2;

  /* sort by increasing starting time, if that works */
  if( mt1->fixed_times && mt2->fixed_times )
  {
    mt_time1 = HaArrayFirst(mt1->fixed_times_by_day);
    mt_time2 = HaArrayFirst(mt2->fixed_times_by_day);
    time1 = mt_time1->time;
    time2 = mt_time2->time;
    if( time1 != time2 )
      return KheTimeIndex(time1) - KheTimeIndex(time2);
  }

  /* otherwise sort by increasing index of first task */
  task1 = KheMTaskTask(mt1, 0, &junk1, &junk2);
  task2 = KheMTaskTask(mt2, 0, &junk1, &junk2);
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetUniqueify(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Uniqueify mts.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetUniqueify(KHE_MTASK_SET mts)
{
  HaArraySortUnique(mts->mtasks, &KheMTaskSetIncreasingFirstTaskCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskSetInterval(KHE_MTASK_SET mts)                      */
/*                                                                           */
/*  Find the interval covered by mts, assuming that all its mtasks have      */
/*  fixed times.                                                             */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheMTaskSetInterval(KHE_MTASK_SET mts)
{
  return mts->interval;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetMoveResourceCheck(KHE_MTASK_SET mts,                     */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r)                                */
/*                                                                           */
/*  Return true if every mtask of mts can be moved from from_r to to_r.      */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetMoveResourceCheck(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded)
{
  return KheMTaskSetMoveResourcePartialCheck(mts, 0,
    HaArrayCount(mts->mtasks) - 1, from_r, to_r,
    disallow_preassigned, unassign_extreme_unneeded);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetMoveResource(KHE_MTASK_SET mts,                          */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r)                                */
/*                                                                           */
/*  Move the mtasks of mts from from_r to to_r, returning true if success.   */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetMoveResource(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded)
{
  return KheMTaskSetMoveResourcePartial(mts, 0,
    HaArrayCount(mts->mtasks) - 1, from_r, to_r,
    disallow_preassigned, unassign_extreme_unneeded);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetMoveResourcePartialCheck(KHE_MTASK_SET mts,              */
/*    int first_index, int last_index, KHE_RESOURCE from_r,KHE_RESOURCE to_r,*/
/*    bool disallow_preassigned, bool unassign_extreme_unneeded)             */
/*                                                                           */
/*  Return true if every mtask of mts[first_index .. last_index] can be      */
/*  moved from from_r to to_r.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetMoveResourcePartialCheck(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded)
{
  KHE_MTASK mt;  int i;
  HnAssert(0 <= first_index,
    "KheMTaskSetMoveResourcePartialCheck internal error 1");
  HnAssert(first_index <= last_index + 1,
    "KheMTaskSetMoveResourcePartialCheck internal error 2");
  HnAssert(last_index < KheMTaskSetMTaskCount(mts),
    "KheMTaskSetMoveResourcePartialCheck internal error 3");
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == first_index || i == last_index)
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      if( !KheMTaskMoveResourceCheck(mt, from_r,NULL,disallow_preassigned)
	&& !KheMTaskMoveResourceCheck(mt, from_r,to_r,disallow_preassigned))
	return false;
    }
    else
    {
      if( !KheMTaskMoveResourceCheck(mt, from_r,to_r,disallow_preassigned) )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetMoveResourcePartial(KHE_MTASK_SET mts,                   */
/*    int first_index, int last_index, KHE_RESOURCE from_r,KHE_RESOURCE to_r,*/
/*    bool disallow_preassigned, bool unassign_extreme_unneeded)             */
/*                                                                           */
/*  Move the mtasks of mts[first_index .. last_index] from from_r to to_r,   */
/*  returning true if success.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetMoveResourcePartial(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded)
{
  KHE_MTASK mt;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == first_index || i == last_index )
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      /* extreme task that does not need assignment; unassign it */
      if( !KheMTaskMoveResource(mt, from_r, NULL, disallow_preassigned) &&
	  !KheMTaskMoveResource(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
    else
    {
      /* regular move */
      if( !KheMTaskMoveResource(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDebug(KHE_MTASK_SET mts, int verbosity, int indent,      */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of mts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDebug(KHE_MTASK_SET mts, int verbosity, int indent, FILE *fp)
{
  KHE_MTASK mt;  int i;  KHE_MTASK_FINDER mtf;
  if( indent >= 0 )
  {
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{ MTaskSet(");
    if( HaArrayCount(mts->mtasks) > 0 )
    {
      mt = HaArrayFirst(mts->mtasks);
      mtf = mt->mtask_finder;
      fprintf(fp, "%s", KheIntervalShow(mts->interval, mtf->days_frame));
    }
    fprintf(fp, ")\n");
    HaArrayForEach(mts->mtasks, mt, i)
      KheMTaskDebug(mt, verbosity, indent + 2, fp);
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "}\n");
  }
  else
  {
    fprintf(fp, "{");
    HaArrayForEach(mts->mtasks, mt, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheMTaskDebug(mt, 1, -1, fp);
    }
    if( HaArrayCount(mts->mtasks) > 0 )
    {
      mt = HaArrayFirst(mts->mtasks);
      mtf = mt->mtask_finder;
      fprintf(fp, " [%s]", KheIntervalShow(mts->interval, mtf->days_frame));
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDebugPartial(KHE_MTASK_SET mts, int first_index,         */
/*    int last_index, int verbosity, int indent, FILE *fp)                   */
/*                                                                           */
/*  Debug print of mts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDebugPartial(KHE_MTASK_SET mts, int first_index,
  int last_index, int verbosity, int indent, FILE *fp)
{
  KHE_MTASK mt;  int i;
  HnAssert(0 <= first_index,
    "KheMTaskSetDebugPartial internal error 1");
  HnAssert(first_index <= last_index + 1,
    "KheMTaskSetDebugPartial internal error 2");
  HnAssert(last_index < KheMTaskSetMTaskCount(mts),
    "KheMTaskSetDebugPartial internal error 3");
  if( indent >= 0 )
  {
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{ MTaskSet(%d-%d):\n", first_index, last_index);
    for( i = first_index;  i <= last_index;  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      KheMTaskDebug(mt, verbosity, indent + 2, fp);
    }
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "}\n");
  }
  else
  {
    fprintf(fp, "{%d-%d: ", first_index, last_index);
    for( i = first_index;  i <= last_index;  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      if( i > first_index )
	fprintf(fp, ", ");
      KheMTaskDebug(mt, 1, -1, fp);
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TIME_GROUP_MTASK_SET"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAddTimeGroupMTasks(KHE_MTASK_FINDER mtf,                         */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_MTASK_SET mts)            */
/*                                                                           */
/*  Add to mts the mtasks of type rt running during tg.  This function does  */
/*  not guarantee that no mtask will be added twice, although it does some   */
/*  work in this direction (using variable prev_mt).                         */
/*                                                                           */
/*****************************************************************************/

static void KheAddTimeGroupMTasks(KHE_MTASK_SET_CACHE mtsc,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_MTASK_SET mts)
{
  int i, j, k, meet_count, task_count;  KHE_MTASK_FINDER mtf;
  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;  KHE_MTASK mt, prev_mt;
  if( DEBUG2 )
    fprintf(stderr, "[ KheAddTimeGroupMTasks(mtf, %s, %s, mts)\n",
      rt == NULL ? "NULL" : KheResourceTypeId(rt), KheTimeGroupId(tg));
  mtf = mtsc->mtask_finder;
  prev_mt = NULL;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    meet_count = KheEventTimetableMonitorTimeMeetCount(mtf->etm, t);
    for( j = 0;  j < meet_count;  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(mtf->etm, t, j);
      task_count = KheMeetTaskCount(meet);
      if( DEBUG2 )
	fprintf(stderr, "  examining meet %s (with %d tasks)\n",
	  KheMeetId(meet), task_count);
      for( k = 0;  k < task_count;  k++ )
      {
	task = KheMeetTask(meet, k);
	if( DEBUG2 )
	  fprintf(stderr, "    examining task %s (type %s)\n",
	    KheTaskId(task), KheResourceTypeId(KheTaskResourceType(task)));
	if( KheTaskResourceType(task) == rt )
	{
	  mt = KheMTaskFinderTaskToMTask(mtf, task);
	  if( mt != prev_mt )
	  {
	    KheMTaskSetAddMTask(mts, mt);
	    prev_mt = mt;
	  }
	}
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheAddTimeGroupMTasks returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET KheTimeGroupMTaskSetMake(KHE_MTASK_FINDER mtf,  */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Make a new time group mtask set object for rt and tg, including          */
/*  building and uniqueifying its mtask set.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_MTASK_SET KheTimeGroupMTaskSetMake(
  KHE_MTASK_SET_CACHE mtsc, KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  KHE_TIME_GROUP_MTASK_SET res;  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;  int i;
  mtf = mtsc->mtask_finder;
  if( HaArrayCount(mtsc->tg_mtask_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtsc->tg_mtask_set_free_list);
    KheMTaskSetClear(res->mts);
  }
  else
  {
    HaMake(res, mtf->arena);
    res->mts = KheMTaskSetMake(mtf);
  }
  res->rt = rt;
  res->tg = tg;
  KheAddTimeGroupMTasks(mtsc, rt, tg, res->mts);
  KheMTaskSetUniqueify(res->mts);
  HaArrayForEach(res->mts->mtasks, mt, i)
    HaArrayAddLast(mt->internal_mtask_sets, res->mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupMTaskSetDelete(KHE_TIME_GROUP_MTASK_SET tg_mts,         */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Delete tg_mts.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheTimeGroupMTaskSetDelete(KHE_TIME_GROUP_MTASK_SET tg_mts,
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_MTASK mt;  int i, pos;
  /* *** save tg_mts->mts for next time KheMTaskSetDelete(tg_mts->mts); */
  HaArrayForEach(tg_mts->mts->mtasks, mt, i)
  {
    if( !HaArrayContains(mt->internal_mtask_sets, tg_mts->mts, &pos) )
      HnAbort("KheTimeGroupMTaskSetDelete internal error");
    HaArrayDeleteAndPlug(mt->internal_mtask_sets, pos);
  }
  KheMTaskSetClear(tg_mts->mts);
  HaArrayAddLast(mtsc->tg_mtask_set_free_list, tg_mts);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TIME_GROUP_MTASK_SET_SET"                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET_SET KheTimeGroupMTaskSetSetMake(                */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Make a new, empty time group mtask set set object.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_MTASK_SET_SET KheTimeGroupMTaskSetSetMake(
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_TIME_GROUP_MTASK_SET_SET res;  KHE_MTASK_FINDER mtf;
  if( HaArrayCount(mtsc->tg_mtask_set_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtsc->tg_mtask_set_set_free_list);
    HaArrayClear(res->mtask_sets);
  }
  else
  {
    mtf = mtsc->mtask_finder;
    HaMake(res, mtf->arena);
    HaArrayInit(res->mtask_sets, mtf->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupMTaskSetSetDelete(KHE_TIME_GROUP_MTASK_SET_SET tg_mtss, */
/*    KHE_MTASK_SET_CACHE mtsc)                                              */
/*                                                                           */
/*  Delete tg_mtss.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheTimeGroupMTaskSetSetDelete(KHE_TIME_GROUP_MTASK_SET_SET tg_mtss,
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_TIME_GROUP_MTASK_SET tg_mts;  int i;
  HaArrayForEach(tg_mtss->mtask_sets, tg_mts, i)
    if( tg_mts != NULL )
      KheTimeGroupMTaskSetDelete(tg_mts, mtsc);
  HaArrayAddLast(mtsc->tg_mtask_set_set_free_list, tg_mtss);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,      */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Return an mtask set containing all mtasks of resource type rt running    */
/*  (wholly or partly) in time group tg.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskSetCacheMTasksInTimeGroup(KHE_MTASK_SET_CACHE mtsc,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  int i;  KHE_TIME_GROUP_MTASK_SET_SET tg_mtss;
  KHE_TIME_GROUP_MTASK_SET tg_mts;

  /* empty time group implies empty mtask set */
  if( KheTimeGroupTimeCount(tg) == 0 )
    return mtsc->empty_mtask_set;

  /* make sure that there is a time group mtask set set at tg's first time */
  i = KheTimeIndex(KheTimeGroupTime(tg, 0));
  HaArrayFill(mtsc->mtasks_in_tg, i + 1, NULL);
  tg_mtss = HaArray(mtsc->mtasks_in_tg, i);
  if( tg_mtss == NULL )
  {
    tg_mtss = KheTimeGroupMTaskSetSetMake(mtsc);
    HaArrayPut(mtsc->mtasks_in_tg, i, tg_mtss);
  }

  /* retrieve and return the mtask set for tg if there is one */
  HaArrayForEach(tg_mtss->mtask_sets, tg_mts, i)
    if( tg_mts->rt == rt && tg_mts->tg == tg )
      return tg_mts->mts;

  /* make, add, and return a new mtask set for tg */
  tg_mts = KheTimeGroupMTaskSetMake(mtsc, rt, tg);
  HaArrayAddLast(tg_mtss->mtask_sets, tg_mts);
  return tg_mts->mts;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetCacheClearMTasksInTimeGroup(KHE_MTASK_FINDER mtf)        */
/*                                                                           */
/*  Clear mtf->mtasks_in_tg.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetCacheClearMTasksInTimeGroup(KHE_MTASK_SET_CACHE mtsc)
{
  int i;  KHE_TIME_GROUP_MTASK_SET_SET tg_mtss;
  HaArrayForEach(mtsc->mtasks_in_tg, tg_mtss, i)
    if( tg_mtss != NULL )
      KheTimeGroupMTaskSetSetDelete(tg_mtss, mtsc);
  HaArrayClear(mtsc->mtasks_in_tg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL_MTASK_SET"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET KheIntervalMTaskSetMake(KHE_MTASK_SET_CACHE mtsc, */
/*    KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)                                 */
/*                                                                           */
/*  Make a new interval mtask set object for rt and in, including building   */
/*  and uniqueifying its mtask set.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_MTASK_SET KheIntervalMTaskSetMake(KHE_MTASK_SET_CACHE mtsc,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  KHE_INTERVAL_MTASK_SET res;  int i;  KHE_TIME_GROUP tg;
  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;
  mtf = mtsc->mtask_finder;
  if( HaArrayCount(mtsc->in_mtask_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtsc->in_mtask_set_free_list);
    KheMTaskSetClear(res->mts);
  }
  else
  {
    HaMake(res, mtf->arena);
    res->mts = KheMTaskSetMake(mtf);
  }
  res->rt = rt;
  res->in = in;
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    tg = KheFrameTimeGroup(mtf->days_frame, i);
    KheAddTimeGroupMTasks(mtsc, rt, tg, res->mts);
  }
  KheMTaskSetUniqueify(res->mts);
  HaArrayForEach(res->mts->mtasks, mt, i)
    HaArrayAddLast(mt->internal_mtask_sets, res->mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalMTaskSetDelete(KHE_INTERVAL_MTASK_SET in_mts,            */
/*    KHE_MTASK_SET_CACHE mtsc)                                              */
/*                                                                           */
/*  Delete in_mts.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalMTaskSetDelete(KHE_INTERVAL_MTASK_SET in_mts,
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_MTASK mt;  int i, pos;
  /* *** save tg_mts->mts for next time KheMTaskSetDelete(tg_mts->mts); */
  HaArrayForEach(in_mts->mts->mtasks, mt, i)
  {
    if( !HaArrayContains(mt->internal_mtask_sets, in_mts->mts, &pos) )
      HnAbort("KheIntervalMTaskSetDelete internal error");
    HaArrayDeleteAndPlug(mt->internal_mtask_sets, pos);
  }
  KheMTaskSetClear(in_mts->mts);
  HaArrayAddLast(mtsc->in_mtask_set_free_list, in_mts);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_INTERVAL_MTASK_SET_SET"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET_SET KheIntervalMTaskSetSetMake(                   */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Make a new, empty interval mtask set set object.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_MTASK_SET_SET KheIntervalMTaskSetSetMake(
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_INTERVAL_MTASK_SET_SET res;  KHE_MTASK_FINDER mtf;
  if( HaArrayCount(mtsc->tg_mtask_set_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(mtsc->in_mtask_set_set_free_list);
    HaArrayClear(res->mtask_sets);
  }
  else
  {
    mtf = mtsc->mtask_finder;
    HaMake(res, mtf->arena);
    HaArrayInit(res->mtask_sets, mtf->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalMTaskSetSetDelete(KHE_INTERVAL_MTASK_SET_SET in_mtss,    */
/*    KHE_MTASK_SET_CACHE mtsc)                                              */
/*                                                                           */
/*  Delete in_mtss.                                                          */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalMTaskSetSetDelete(KHE_INTERVAL_MTASK_SET_SET in_mtss,
  KHE_MTASK_SET_CACHE mtsc)
{
  KHE_INTERVAL_MTASK_SET in_mts;  int i;
  HaArrayForEach(in_mtss->mtask_sets, in_mts, i)
    if( in_mts != NULL )
      KheIntervalMTaskSetDelete(in_mts, mtsc);
  HaArrayAddLast(mtsc->in_mtask_set_set_free_list, in_mtss);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,       */
/*    KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)                                 */
/*                                                                           */
/*  Return an mtask set containing all mtasks of resource type rt running    */
/*  (wholly or partly) in interval in.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskSetCacheMTasksInInterval(KHE_MTASK_SET_CACHE mtsc,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  int i;  KHE_INTERVAL_MTASK_SET_SET in_mtss;
  KHE_INTERVAL_MTASK_SET in_mts;

  /* empty interval implies empty mtask set */
  if( KheIntervalEmpty(in) )
    return mtsc->empty_mtask_set;

  /* make sure that there is an interval mtask set set at in's first */
  i = KheIntervalFirst(in);
  HaArrayFill(mtsc->mtasks_in_in, i + 1, NULL);
  in_mtss = HaArray(mtsc->mtasks_in_in, i);
  if( in_mtss == NULL )
  {
    in_mtss = KheIntervalMTaskSetSetMake(mtsc);
    HaArrayPut(mtsc->mtasks_in_in, i, in_mtss);
  }

  /* retrieve and return the mtask set for in if there is one */
  HaArrayForEach(in_mtss->mtask_sets, in_mts, i)
    if( in_mts->rt == rt && KheIntervalEqual(in_mts->in, in) )
      return in_mts->mts;

  /* make, add, and return a new mtask set for in */
  in_mts = KheIntervalMTaskSetMake(mtsc, rt, in);
  HaArrayAddLast(in_mtss->mtask_sets, in_mts);
  return in_mts->mts;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetCacheClearMTasksInInterval(KHE_MTASK_SET_CACHE mtsc)     */
/*                                                                           */
/*  Clear mtsc->mtasks_in_in.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetCacheClearMTasksInInterval(KHE_MTASK_SET_CACHE mtsc)
{
  int i;  KHE_INTERVAL_MTASK_SET_SET in_mtss;
  HaArrayForEach(mtsc->mtasks_in_in, in_mtss, i)
    if( in_mtss != NULL )
      KheIntervalMTaskSetSetDelete(in_mtss, mtsc);
  HaArrayClear(mtsc->mtasks_in_in);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_SET_CACHE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET_CACHE KheMTaskSetCacheMake(KHE_MTASK_FINDER mtf)           */
/*                                                                           */
/*  Make an mtask set cache object for mtf.                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_SET_CACHE KheMTaskSetCacheMake(KHE_MTASK_FINDER mtf)
{
  KHE_MTASK_SET_CACHE res;  HA_ARENA a;

  /* free lists */
  a = mtf->arena;
  HaMake(res, a);
  HaArrayInit(res->tg_mtask_set_free_list, a);
  HaArrayInit(res->tg_mtask_set_set_free_list, a);
  HaArrayInit(res->in_mtask_set_free_list, a);
  HaArrayInit(res->in_mtask_set_set_free_list, a);

  /* constant attributes */
  res->mtask_finder = mtf;
  res->empty_mtask_set = KheMTaskSetMake(mtf);

  /* the actual cache */
  HaArrayInit(res->mtasks_in_tg, a);
  HaArrayInit(res->mtasks_in_in, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskSetCacheRetrieve(KHE_MTASK_SET_CACHE mtsc,         */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_INTERVAL in)              */
/*                                                                           */
/*  Retrieve an mtask set of type rt indexed by tg or in from mtsc.          */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_SET KheMTaskSetCacheRetrieve(KHE_MTASK_SET_CACHE mtsc,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_INTERVAL in)
{
  if( tg == NULL )
    return KheMTaskSetCacheMTasksInInterval(mtsc, rt, in);
  else
    return KheMTaskSetCacheMTasksInTimeGroup(mtsc, rt, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetCacheClear(KHE_MTASK_SET_CACHE mtsc)                     */
/*                                                                           */
/*  Clear mtsc.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetCacheClear(KHE_MTASK_SET_CACHE mtsc)
{
  KheMTaskSetCacheClearMTasksInTimeGroup(mtsc);
  KheMTaskSetCacheClearMTasksInInterval(mtsc);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_FINDER - construction"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderFindMTask(KHE_MTASK_FINDER mtf,                       */
/*    KHE_ROOT_TASK_SIG sig, KHE_MTASK *res, KHE_MTASK_SET *mts,             */
/*    int *index_in_mts)                                                     */
/*                                                                           */
/*  Set *mts to the mtask set that an mtask with this sig should lie in.     */
/*                                                                           */
/*  If there is an mtask with this signature in mts, set *res to that        */
/*  mtask, set *index_in_mts to its index in *mts, and return true.          */
/*                                                                           */
/*  If there is no mtask with this signature in mts, set *res to NULL,       */
/*  set *index_in_mts to -1, and return false.                               */
/*                                                                           */
/*  But either way, also return in *mts the mtask set that an mtask with     */
/*  this signature should be in.  Also if it is present, set *index_in_mts   */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFinderFindMTask(KHE_MTASK_FINDER mtf,
  KHE_ROOT_TASK_SIG sig, KHE_MTASK_SET *mts, KHE_MTASK *res,
  int *index_in_mts)
{
  KHE_MTASK mt;  int i, sort_index;

  /* set *mts to the mtask set that an mtask with this sig should be in */
  if( KheRootTaskSigIsDegenerate(sig) )
  {
    /* just one mtask set holds all the degenerate mtasks */
    *mts = mtf->mtasks_degenerate;
  }
  else if( KheRootTaskSigFixedTimes(sig, &sort_index) )
  {
    /* search for sig among mtasks_at_time with this sort index */
    HaArrayFill(mtf->mtasks_at_time, sort_index + 1, NULL);
    *mts = HaArray(mtf->mtasks_at_time, sort_index);
    if( *mts == NULL )
    {
      *mts = KheMTaskSetMake(mtf);
      HaArrayPut(mtf->mtasks_at_time, sort_index, *mts);
    }
  }
  else
  {
    /* search for sig among mtasks_at_meet with this sort index */
    HaArrayFill(mtf->mtasks_at_meet, sort_index + 1, NULL);
    *mts = HaArray(mtf->mtasks_at_meet, sort_index);
    if( *mts == NULL )
    {
      *mts = KheMTaskSetMake(mtf);
      HaArrayPut(mtf->mtasks_at_meet, sort_index, *mts);
    }
  }

  /* search *mts for an mtask with this signature */
  HaArrayForEach((*mts)->mtasks, mt, i)
    if( KheRootTaskSigSimilar(sig, mt->sig) )
      return *res = mt, *index_in_mts = i, true;

  /* no luck so return false */
  return *res = NULL, *index_in_mts = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderLinkTaskToMTask(KHE_MTASK_FINDER mtf,                 */
/*    KHE_TASK task, KHE_MTASK mt)                                           */
/*                                                                           */
/*  Add a map from task (a proper root task) to mt in mtf.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFinderLinkTaskToMTask(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_MTASK mt)
{
  int index;
  HnAssert(KheTaskIsProperRoot(task),
    "KheMTaskFinderLinkTaskToMTask internal error 1");
  index = KheTaskSolnIndex(task);
  HaArrayFill(mtf->mtasks_by_task, index + 1, NULL);
  HnAssert(HaArray(mtf->mtasks_by_task, index) == NULL,
    "KheMTaskFinderLinkTaskToMTask internal error 2");
  HaArrayPut(mtf->mtasks_by_task, index, mt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderUnLinkTaskFromMTask(KHE_MTASK_FINDER mtf,             */
/*    KHE_TASK task, KHE_MTASK mt)                                           */
/*                                                                           */
/*  Undo KheMTaskFinderLinkTaskToMTask.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFinderUnLinkTaskFromMTask(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_MTASK mt)
{
  int index;
  HnAssert(KheTaskIsProperRoot(task),
    "KheMTaskFinderUnLinkTaskFromMTask internal error 1");
  index = KheTaskSolnIndex(task);
  HaArrayFill(mtf->mtasks_by_task, index + 1, NULL);
  HnAssert(HaArray(mtf->mtasks_by_task, index) == mt,
    "KheMTaskFinderLinkTaskToMTask internal error 2");
  HaArrayPut(mtf->mtasks_by_task, index, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)          */
/*                                                                           */
/*  Add task (which must be a proper root task) to mtf.                      */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFinderAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  KHE_MTASK mt;  KHE_ROOT_TASK_SIG sig;  KHE_COST non_asst_cost, asst_cost;
  KHE_MTASK_SET mts;  int index_in_mts;

  HnAssert(KheTaskIsProperRoot(task), "KheMTaskFinderAddTask internal error");
  sig = KheRootTaskSigMake(mtf, task, &non_asst_cost, &asst_cost);
  if( KheMTaskFinderFindMTask(mtf, sig, &mts, &mt, &index_in_mts) )
  {
    /* an mt with this sig exists, so delete sig; we'll add task to mt */
    HnAssert(!KheMTaskContainsTask(mt, task),
      "KheMTaskFinderAddTask internal error:  task already added");
    KheRootTaskSigDelete(sig, mtf);
  }
  else
  {
    /* no mtask for task, so make one and add it to mts and mtf */
    mt = KheMTaskMake(mtf, sig);
    KheMTaskSetAddMTask(mts, mt);
    HaArrayAddLast(mt->internal_mtask_sets, mts);
    mt->mtasks_index = HaArrayCount(mtf->mtasks);
    HaArrayAddLast(mtf->mtasks, mt);
    /* KheMTaskSetCacheClear(mtf->mts_cache); */
  }
  if( DEBUG1 )
    fprintf(stderr, "  adding task %s\n", KheTaskId(task));
  KheMTaskAddTask(mt, task, non_asst_cost, asst_cost);
  KheMTaskFinderLinkTaskToMTask(mtf, task, mt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderDeleteTask(KHE_MTASK_FINDER mtf, KHE_TASK task)       */
/*                                                                           */
/*  Delete task (which must be a proper root task) from mtf.                 */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFinderDeleteTask(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  KHE_MTASK mt, mt2;  KHE_MTASK_SET mts;  int pos, index_in_mts;
  HnAssert(KheTaskIsProperRoot(task),"KheMTaskFinderDeleteTask internal error");
  mt = KheMTaskFinderTaskToMTask(mtf, task);
  KheMTaskFinderUnLinkTaskFromMTask(mtf, task, mt);
  KheMTaskDeleteTask(mt, task);
  if( HaArrayCount(mt->tasks_and_costs) == 0 )
  {
    /* delete mt from mtf->mtasks_at_time or mtf->mtasks_at_meet */
    if( !KheMTaskFinderFindMTask(mtf, mt->sig, &mts, &mt2, &index_in_mts) )
      HnAbort("KheMTaskFinderDeleteTask internal error 1");
    HnAssert(mt2 == mt, "KheMTaskFinderDeleteTask internal error 2");
    HaArrayDeleteAndPlug(mts->mtasks, index_in_mts);
    KheMTaskSetResetInterval(mts);
    /* KheMTaskSetDeleteMTask(mts, mt); */

    /* delete mt from mtf->mtasks */
    if( !HaArrayContains(mtf->mtasks, mt, &pos) )
      HnAbort("KheMTaskFinderDeleteTask internal error 3");
    HaArrayDeleteAndPlug(mtf->mtasks, pos);

    /* delete mt from mtf->mts_cache */
    KheMTaskSetCacheClear(mtf->mts_cache);

    /* delete mt itself */
    KheMTaskDelete(mt);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER KheMTaskFinderMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, */
/*    KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,                 */
/*    bool fixed_times, HA_ARENA a)                                          */
/*                                                                           */
/*  Make a new mtask finder object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_FINDER KheMTaskFinderMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,
  bool fixed_times, HA_ARENA a)
{
  KHE_MTASK_FINDER res;  int i;  KHE_TASK task;

  if( DEBUG1 )
    fprintf(stderr, "[ KheMTaskFinderMake(soln of %s, rt %s, frame, etm, %s)\n",
      KheInstanceId(KheSolnInstance(soln)),
      rt == NULL ? "-" : KheResourceTypeId(rt),
      bool_show(fixed_times));

  /* make the basic object - free lists */
  HaMake(res, a);
  HaArrayInit(res->task_and_cost_free_list, a);
  HaArrayInit(res->atomic_task_sig_free_list, a);
  HaArrayInit(res->root_task_sig_free_list, a);
  HaArrayInit(res->mtask_time_free_list, a);
  HaArrayInit(res->mtask_free_list, a);
  HaArrayInit(res->mtask_set_free_list, a);

  /* make the basic object - constant attributes */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->days_frame = days_frame;
  res->etm = etm;
  res->fixed_times = fixed_times;

  /* make the basic object - mtasks, indexed in various ways */
  HaArrayInit(res->mtasks, a);
  HaArrayInit(res->mtasks_at_time, a);
  HaArrayInit(res->mtasks_at_meet, a);
  res->mtasks_degenerate = KheMTaskSetMake(res);
  HaArrayInit(res->mtasks_by_task, a);
  /* ***
  HaArrayInit(res->mtasks_in_tg, a);
  HaArrayInit(res->mtasks_in_in, a);
  *** */
  res->mts_cache = KheMTaskSetCacheMake(res);

  /* make the basic object - task grouping*/
  res->group_leader_task = NULL;
  res->group_asst = NULL;
  HaArrayInit(res->group_follower_tasks, a);

  /* add all tasks */
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskIsProperRoot(task) &&
	  (rt == NULL || KheTaskResourceType(task) == rt) )
      KheMTaskFinderAddTask(res, task);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheMTaskFinderMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheMTaskFinderSoln(KHE_MTASK_FINDER mtf)                        */
/*                                                                           */
/*  Return the soln attribute of mtf.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheMTaskFinderSoln(KHE_MTASK_FINDER mtf)
{
  return mtf->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheMTaskFinderResourceType(KHE_MTASK_FINDER mtf)       */
/*                                                                           */
/*  Return the resource type attribute of mtf.                               */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheMTaskFinderResourceType(KHE_MTASK_FINDER mtf)
{
  return mtf->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheMTaskFinderDaysFrame(KHE_MTASK_FINDER mtf)                  */
/*                                                                           */
/*  Return the days_frame attribute of mtf.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheMTaskFinderDaysFrame(KHE_MTASK_FINDER mtf)
{
  return mtf->days_frame;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheMTaskFinderEventTimetableMonitor(         */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Return the event timetable monitor attribute of mtf.                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_TIMETABLE_MONITOR KheMTaskFinderEventTimetableMonitor(
  KHE_MTASK_FINDER mtf)
{
  return mtf->etm;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderFixedTimes(KHE_MTASK_FINDER mtf)                      */
/*                                                                           */
/*  Return the fixed_times attribute of mtf.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderFixedTimes(KHE_MTASK_FINDER mtf)
{
  return mtf->fixed_times;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheMTaskFinderArena(KHE_MTASK_FINDER mtf)                       */
/*                                                                           */
/*  return mtf's arena.                                                      */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheMTaskFinderArena(KHE_MTASK_FINDER mtf)
{
  return mtf->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderLastIndex(KHE_MTASK_FINDER mtf)                        */
/*                                                                           */
/*  Return the last index of mtf.                                            */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderLastIndex(KHE_MTASK_FINDER mtf)
{
  return KheFrameTimeGroupCount(mtf->days_frame) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderMTaskCount(KHE_MTASK_FINDER mtf)                       */
/*                                                                           */
/*  Return the number of mtasks in mtf.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskCount(KHE_MTASK_FINDER mtf)
{
  return HaArrayCount(mtf->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTask(KHE_MTASK_FINDER mtf, int i)               */
/*                                                                           */
/*  Return the i'th mtask of mtf.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTask(KHE_MTASK_FINDER mtf, int i)
{
  return HaArray(mtf->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderMTaskSort(KHE_MTASK_FINDER mtf,                       */
/*    int (*compar)(const void *, const void *))                             */
/*                                                                           */
/*  Sort the mtasks of mtf using comparison function compar.                 */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderMTaskSort(KHE_MTASK_FINDER mtf,
  int (*compar)(const void *, const void *))
{
  HaArraySort(mtf->mtasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskDecreasingDurationTypedCmp(KHE_MTASK mt1, KHE_MTASK mt2)     */
/*                                                                           */
/*  Typed comparison function for sorting mtasks by decreasing duration.     */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskDecreasingDurationTypedCmp(KHE_MTASK mt1, KHE_MTASK mt2)
{
  return mt2->sig->total_duration - mt1->sig->total_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskDecreasingDurationCmp(const void *t1, const void *t2)        */
/*                                                                           */
/*  Untyped comparison function for sorting mtasks by decreasing duration.   */
/*                                                                           */
/*****************************************************************************/

int KheMTaskDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_MTASK mt1 = * (KHE_MTASK *) t1;
  KHE_MTASK mt2 = * (KHE_MTASK *) t2;
  return KheMTaskDecreasingDurationTypedCmp(mt1, mt2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderMTaskAtTimeCount(KHE_MTASK_FINDER mtf, KHE_TIME time)  */
/*                                                                           */
/*  Return the number of mtasks in mtf whose start time is time.             */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskAtTimeCount(KHE_MTASK_FINDER mtf, KHE_TIME time)
{
  KHE_MTASK_SET mts;  int index;
  index = KheTimeIndex(time);
  if( index < HaArrayCount(mtf->mtasks_at_time) )
  {
    mts = HaArray(mtf->mtasks_at_time, index);
    return HaArrayCount(mts->mtasks);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTaskAtTime(KHE_MTASK_FINDER mtf,                */
/*    KHE_TIME time, int i)                                                  */
/*                                                                           */
/*  Return the i'th mtask of mtf whose start time is time.                   */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTaskAtTime(KHE_MTASK_FINDER mtf,
  KHE_TIME time, int i)
{
  KHE_MTASK_SET mts;  int index;
  index = KheTimeIndex(time);
  HnAssert(index < HaArrayCount(mtf->mtasks_at_time),
    "KheMTaskFinderMTaskAtTime internal error: i out of range");
  mts = HaArray(mtf->mtasks_at_time, index);
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/* int KheMTaskFinderMTaskFromMeetCount(KHE_MTASK_FINDER mtf, KHE_MEET meet) */
/*                                                                           */
/*  Return the number of mtasks in mtf whose meet is meet.                   */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskFromMeetCount(KHE_MTASK_FINDER mtf, KHE_MEET meet)
{
  KHE_MTASK_SET mts;  int index;
  index = KheMeetSolnIndex(meet);
  if( index < HaArrayCount(mtf->mtasks_at_meet) )
  {
    mts = HaArray(mtf->mtasks_at_meet, index);
    return HaArrayCount(mts->mtasks);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTaskFromMeet(KHE_MTASK_FINDER mtf,              */
/*    KHE_MEET meet, int i)                                                  */
/*                                                                           */
/*  Return the i'th task derived from meet.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTaskFromMeet(KHE_MTASK_FINDER mtf,
  KHE_MEET meet, int i)
{
  KHE_MTASK_SET mts;  int index;
  index = KheMeetSolnIndex(meet);
  mts = HaArray(mtf->mtasks_at_meet, index);
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderTaskToMTask(KHE_MTASK_FINDER mtf, KHE_TASK t)    */
/*                                                                           */
/*  Return the mtask corresponding to the proper root of task t.             */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderTaskToMTask(KHE_MTASK_FINDER mtf, KHE_TASK t)
{
  int index;  KHE_MTASK res;
  index = KheTaskSolnIndex(KheTaskProperRoot(t));
  HaArrayFill(mtf->mtasks_by_task, index + 1, NULL);
  res = HaArray(mtf->mtasks_by_task, index);
  HnAssert(res != NULL,
    "KheMTaskFinderTaskToMTask: task %s (proper root %s with index %d) has "
    "no mtask", KheTaskId(t), KheTaskId(KheTaskProperRoot(t)), index);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,      */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Return an mtask set containing the mtasks of type rt lying in time       */
/*  group tg.                                                                */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  return KheMTaskSetCacheRetrieve(mtf->mts_cache, rt, tg,
    KheIntervalMake(1, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,       */
/*    KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)                                 */
/*                                                                           */
/*  Return an mtask set containing the mtasks of type rt lying in interval   */
/*  in.                                                                      */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  return KheMTaskSetCacheRetrieve(mtf->mts_cache, rt, NULL, in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderAddResourceMTasksInInterval(KHE_MTASK_FINDER mtf,     */
/*    KHE_RESOURCE r, KHE_INTERVAL in, KHE_MTASK_SET mts)                    */
/*                                                                           */
/*  Add to mts the mtasks that r is assigned to in interval in, in order.    */
/*  The chosen mtasks lie completely (not partially) within in.              */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderAddResourceMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE r, KHE_INTERVAL in, KHE_MTASK_SET mts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j, k;  KHE_TIME_GROUP tg;
  KHE_TIME t;  KHE_TASK task;  KHE_MTASK mt, prev_mt;  KHE_INTERVAL mt_in;
  rtm = KheResourceTimetableMonitor(mtf->soln, r);
  prev_mt = NULL;
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(mtf->days_frame, i);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	mt = KheMTaskFinderTaskToMTask(mtf, task);
	mt_in = KheMTaskInterval(mt);
	if( mt != prev_mt && KheIntervalSubset(mt_in, in) )
	{
	  KheMTaskSetAddMTask(mts, mt);
	  prev_mt = mt;
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddResourceProperRootTasksInInterval(KHE_RESOURCE r,             */
/*    KHE_INTERVAL in, KHE_SOLN soln, KHE_FRAME days_frame, KHE_TASK_SET ts) */
/*                                                                           */
/*  Add to mts the mtasks that r is assigned to in interval in, in order.    */
/*  The chosen mtasks lie completely (not partially) within in.              */
/*                                                                           */
/*****************************************************************************/

void KheAddResourceProperRootTasksInInterval(KHE_RESOURCE r,
  KHE_INTERVAL in, KHE_SOLN soln, KHE_FRAME days_frame, KHE_TASK_SET ts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j, k;  KHE_TIME_GROUP tg;
  KHE_TIME t;  KHE_TASK task, prev_task;  KHE_INTERVAL task_in;
  rtm = KheResourceTimetableMonitor(soln, r);
  prev_task = NULL;
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(days_frame, i);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	task_in = KheTaskInterval(task, days_frame);
	if( task != prev_task && KheIntervalSubset(task_in, in) )
	{
	  KheTaskSetAddTask(ts, task);
	  prev_task = task;
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_FINDER - 'forbidden' operations"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderTaskMove(KHE_MTASK_FINDER mtf, KHE_TASK task,         */
/*    KHE_TASK target_task)                                                  */
/*                                                                           */
/*  Call KheTaskMove but update the mtask structure as well.                 */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderTaskMove(KHE_MTASK_FINDER mtf, KHE_TASK task,
  KHE_TASK target_task)
{
  KHE_TASK task_proper_root, target_task_proper_root;

  /* return early if move is not allowed; also checks task != target_task */
  if( !KheTaskMoveCheck(task, target_task) )
    return false;

  task_proper_root = KheTaskProperRoot(task);
  if( target_task == NULL )
  {
    /* unassignment: delete one task and add two */
    HnAssert(task_proper_root != task, "KheMTaskFinderTaskMove internal error");
    KheMTaskFinderDeleteTask(mtf, task_proper_root);
    KheTaskMove(task, target_task);
    KheMTaskFinderAddTask(mtf, task);
    KheMTaskFinderAddTask(mtf, task_proper_root);
    return true;
  }
  else if( task_proper_root == task )
  {
    /* assignment: delete two tasks and add one */
    HnAssert(KheTaskIsProperRoot(task),"KheMTaskFinderTaskMove internal error");
    KheMTaskFinderDeleteTask(mtf, task);
    target_task_proper_root = KheTaskProperRoot(target_task);
    KheMTaskFinderDeleteTask(mtf, target_task_proper_root);
    KheTaskMove(task, target_task);
    KheMTaskFinderAddTask(mtf, target_task_proper_root);
    return true;
  }
  else
  {
    /* true move: delete two tasks and add two */
    KheMTaskFinderDeleteTask(mtf, task_proper_root);
    target_task_proper_root = KheTaskProperRoot(target_task);
    KheMTaskFinderDeleteTask(mtf, target_task_proper_root);
    KheTaskMove(task, target_task);
    KheMTaskFinderAddTask(mtf, target_task_proper_root);
    KheMTaskFinderAddTask(mtf, task_proper_root);
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderTaskAssign(KHE_MTASK_FINDER mtf, KHE_TASK task,       */
/*    KHE_TASK target_task)                                                  */
/*                                                                           */
/*  Call KheTaskAssign but update the mtask structure as well.               */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderTaskAssign(KHE_MTASK_FINDER mtf, KHE_TASK task,
  KHE_TASK target_task)
{
  return KheTaskAsst(task) == NULL &&
    KheMTaskFinderTaskMove(mtf, task, target_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderTaskUnAssign(KHE_MTASK_FINDER mtf, KHE_TASK task)     */
/*                                                                           */
/*  Call KheTaskUnAssign but update the mtask structure as well.             */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderTaskUnAssign(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  return KheMTaskFinderTaskMove(mtf, task, NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderTaskSwap(KHE_MTASK_FINDER mtf, KHE_TASK task1,        */
/*    KHE_TASK task2)                                                        */
/*                                                                           */
/*  Call KheTaskSwap but update the mtask structure as well.                 */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderTaskSwap(KHE_MTASK_FINDER mtf, KHE_TASK task1,
  KHE_TASK task2)
{
  KHE_TASK target_task1, target_task2;
  KHE_TASK task1_proper_root, task2_proper_root;

  /* return early if swap is not allowed; also checks assts differ */
  if( !KheTaskSwapCheck(task1, task2) )
    return false;

  target_task1 = KheTaskAsst(task1);
  target_task2 = KheTaskAsst(task2);
  HnAssert(target_task1 != target_task2,
    "KheMTaskFinderTaskSwap internal error");
  if( target_task1 == NULL )
  {
    HnAssert(KheTaskIsProperRoot(task1),
      "KheMTaskFinderTaskSwap internal error");
    KheMTaskFinderDeleteTask(mtf, task1);
    task2_proper_root = KheTaskProperRoot(target_task2);
    KheMTaskFinderDeleteTask(mtf, task2_proper_root);
    KheTaskSwap(task1, task2);
    KheMTaskFinderAddTask(mtf, task2);
    KheMTaskFinderAddTask(mtf, task2_proper_root);
  }
  else if( target_task2 == NULL )
  {
    HnAssert(KheTaskIsProperRoot(task2),
      "KheMTaskFinderTaskSwap internal error");
    KheMTaskFinderDeleteTask(mtf, task2);
    task1_proper_root = KheTaskProperRoot(target_task1);
    KheMTaskFinderDeleteTask(mtf, task1_proper_root);
    KheTaskSwap(task1, task2);
    KheMTaskFinderAddTask(mtf, task1);
    KheMTaskFinderAddTask(mtf, task1_proper_root);
  }
  else
  {
    task1_proper_root = KheTaskProperRoot(target_task1);
    KheMTaskFinderDeleteTask(mtf, task1_proper_root);
    task2_proper_root = KheTaskProperRoot(target_task2);
    KheMTaskFinderDeleteTask(mtf, task2_proper_root);
    KheTaskSwap(task1, task2);
    KheMTaskFinderAddTask(mtf, task1_proper_root);
    KheMTaskFinderAddTask(mtf, task2_proper_root);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderTaskAssignFix(KHE_MTASK_FINDER mtf, KHE_TASK task)    */
/*                                                                           */
/*  Call KheTaskAssignFix but update the mtask structure as well.            */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderTaskAssignFix(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  if( !KheTaskAssignIsFixed(task) && KheTaskIsProperRoot(task) )
  {
    /* only have to do anything if task is not fixed and is a proper root */
    KheMTaskFinderDeleteTask(mtf, task);
    KheTaskAssignFix(task);
    KheMTaskFinderAddTask(mtf, task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderTaskAssignUnFix(KHE_MTASK_FINDER mtf, KHE_TASK task)  */
/*                                                                           */
/*  Call KheTaskAssignUnFix but update the mtask structure as well.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderTaskAssignUnFix(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  if( KheTaskAssignIsFixed(task) && KheTaskIsProperRoot(task) )
  {
    /* only have to do anything if task is fixed and is a proper root */
    KheMTaskFinderDeleteTask(mtf, task);
    KheTaskAssignUnFix(task);
    KheMTaskFinderAddTask(mtf, task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderGroupBegin(KHE_MTASK_FINDER mtf, KHE_TASK leader_task)*/
/*                                                                           */
/*  Start a task grouping operation with the given leader task.  This        */
/*  clears out any previous task grouping information and sets leader_task.  */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderGroupBegin(KHE_MTASK_FINDER mtf, KHE_TASK leader_task)
{
  HnAssert(leader_task != NULL,
    "KheMTaskFinderGroupBegin internal error: leader_task == NULL");
  HnAssert(KheTaskIsProperRoot(leader_task),
   "KheMTaskFinderGroupBegin internal error: leader_task is not proper root");
  mtf->group_leader_task = leader_task;
  mtf->group_asst = KheTaskAsstResource(leader_task);
  HaArrayClear(mtf->group_follower_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderGroupAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)     */
/*                                                                           */
/*  Add a task to the current task grouping operation.                       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskFinderGroupAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  KHE_RESOURCE asst;
  HnAssert(mtf->group_leader_task != NULL,
    "KheMTaskFinderGroupAddTask internal error: no leader task");
  HnAssert(task != NULL,
    "KheMTaskFinderGroupAddTask internal error: task == NULL");
  HnAssert(KheTaskIsProperRoot(task),
    "KheMTaskFinderGroupAddTask internal error: task is not proper root");

  /* make sure any resource assignment is compatible; return false if not */
  asst = KheTaskAsstResource(task);
  if( asst != NULL )
  {
    if( mtf->group_asst == NULL )
      mtf->group_asst = asst;
    else if( mtf->group_asst != asst )
      return false;
  }

  /* make sure the move is possible */
  if( KheTaskMoveCheck(task, mtf->group_leader_task) )
  {
    HaArrayAddLast(mtf->group_follower_tasks, task);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderGroupEnd(KHE_MTASK_FINDER mtf)                        */
/*                                                                           */
/*  Complete the current task grouping operation.                            */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderGroupEnd(KHE_MTASK_FINDER mtf)
{
  KHE_TASK task;  int i;
  HnAssert(mtf->group_leader_task != NULL,
    "KheMTaskFinderGroupEnd internal error (called out of order)");
  KheMTaskFinderDeleteTask(mtf, mtf->group_leader_task);
  HaArrayForEach(mtf->group_follower_tasks, task, i)
  {
    KheMTaskFinderDeleteTask(mtf, task);
    KheTaskMove(task, mtf->group_leader_task);
  }
  if( KheTaskAsstResource(mtf->group_leader_task) == NULL &&
      mtf->group_asst != NULL )
    KheTaskAssignResource(mtf->group_leader_task, mtf->group_asst);
  KheMTaskFinderAddTask(mtf, mtf->group_leader_task);

  /* clear out the old stuff */
  mtf->group_leader_task = NULL;
  mtf->group_asst = NULL;
  HaArrayClear(mtf->group_follower_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderDebug(KHE_MTASK_FINDER mtf, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mtf onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderDebug(KHE_MTASK_FINDER mtf, int verbosity,
  int indent, FILE *fp)
{
  KHE_MTASK mt;  int i, j;  KHE_MTASK_SET mts;
  fprintf(fp, "%*s[ MTaskFinder(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(mtf->soln)),
    KheResourceTypeId(mtf->resource_type));
  HaArrayForEach(mtf->mtasks_at_time, mts, i)
  {
    fprintf(fp, "%*s  [ time %d:\n", indent, "", i);
    HaArrayForEach(mts->mtasks, mt, j)
      KheMTaskDebug(mt, verbosity, indent + 4, fp);
    fprintf(fp, "%*s  ]\n", indent, "");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


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
/*  FILE:         khe_sm_parallel_solve.c                                    */
/*  DESCRIPTION:  KheParallelSolve().  If you can't compile this file,       */
/*                see the makefile for a workaround.                         */
/*                                                                           */
/*  If you are having trouble compiling this file, try replacing the 1       */
/*  by 0 in line "#define KHE_USE_PTHREAD 1" in khe_solvers.h.                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>
#include <math.h>
#include <setjmp.h>

#define bool_show(x) ((x) ? "true" : "false")
#define RECOMBINED_DIVERSIFIER 999

#if KHE_USE_PTHREAD
#include <pthread.h>
#endif

#if KHE_USE_SYSINFO
#include <sys/sysinfo.h>
#endif

#define DEBUG1 0
#define DEBUG2 1		/* usually set to 1; reports overall progress */
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0		/* recombination */
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG16 0
#define DEBUG17 0
#define DEBUG18 0


/*****************************************************************************/
/*                                                                           */
/*  PARTIAL_COST - solution cost divided up by resource type                 */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;

typedef struct partial_cost_rec {
  KHE_SOLN		soln;
  ARRAY_KHE_COST	costs;
} *PARTIAL_COST;

typedef HA_ARRAY(PARTIAL_COST) ARRAY_PARTIAL_COST;


/*****************************************************************************/
/*                                                                           */
/*  PINST - an instance to be solved in parallel                             */
/*                                                                           */
/*  These variables keep track of where things are up to:                    */
/*                                                                           */
/*    first_soln                                                             */
/*      The solution returned by the first solve of pinst, or NULL if        */
/*      that solve has not returned yet.  This is not the same thing as      */
/*      the first solution found.  It is an element of all_solns.            */
/*                                                                           */
/*    all_solns                                                              */
/*      All solutions returned by solves.  Their number is the number of     */
/*      completed solves of pinst.                                           */
/*                                                                           */
/*    started_solves                                                         */
/*      The number of solves of pinst->instance that have been started.      */
/*                                                                           */
/*    running_solves                                                         */
/*      The number of solves of pinst->instance that have been started       */
/*      but not finished.  Should equal ps->ps_make - |all_solns|.           */
/*                                                                           */
/*    have_optimal_soln                                                      */
/*      True when one of the solutions of all_solns is optimal.              */
/*                                                                           */
/*    all_solves_ended                                                       */
/*      True when all solves of pinst have ended and their results have      */
/*      been dealt with.                                                     */
/*                                                                           */
/*  These variables can be changed only when the solver is locked.  We       */
/*  could have instance locking rather than solver locking, but we need      */
/*  solver locking anyway and little would be gained by instance locking.    */
/*                                                                           */
/*****************************************************************************/

typedef struct psolver_rec *PSOLVER;
typedef HA_ARRAY(KHE_SOLN) ARRAY_KHE_SOLN;

typedef struct pinst_rec {
  PSOLVER		psolver;		/* enclosing solver          */
  KHE_INSTANCE		instance;		/* the instance to solve     */
  KHE_TIMER		ins_timer;		/* the time the solve began  */
  float			running_time;		/* the final running time    */
  KHE_SOLN		first_soln;		/* first soln (optional)     */
  ARRAY_KHE_SOLN	all_solns;		/* all solns so far          */
  int			started_solves;		/* started solves            */
  int			running_solves;		/* running solves            */
  bool			have_optimal_soln;	/* true if have optimal soln */
  bool			all_solves_ended;	/* true if pinst solve ended */
  ARRAY_PARTIAL_COST	partial_costs;		/* partial costs             */
} *PINST;

typedef HA_ARRAY(PINST) ARRAY_PINST;


/*****************************************************************************/
/*                                                                           */
/*  PTHREAD - one thread                                                     */
/*                                                                           */
/*****************************************************************************/

typedef struct pthread_rec {
  HA_ARENA		arena;
  PSOLVER		psolver;
  KHE_OPTIONS		options;
  HA_ARENA_SET		arena_set;
  int			pt_index;
#if KHE_USE_PTHREAD
  pthread_t		thread;
#endif
} *PTHREAD;

typedef HA_ARRAY(PTHREAD) ARRAY_PTHREAD;


/*****************************************************************************/
/*                                                                           */
/*  PSOLVER - a parallel solver                                              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_PS_TIME_OMIT,
  KHE_PS_TIME_SHARED
} KHE_PS_TIME;

struct psolver_rec {
  KHE_ARCHIVE		archive;		/* as passed in              */
  KHE_GENERAL_SOLVER	solver;			/* as passed in              */
  KHE_OPTIONS		options;		/* as passed in              */
  KHE_SOLN_TYPE		soln_type;		/* as passed in              */
  HA_ARENA_SET		arena_set;		/* as passed in              */
  HA_ARENA		arena;			/* taken from arena_set      */
  KHE_TIMER		global_timer;		/* times whole run           */
  KHE_SOLN_GROUP	first_soln_group;	/* holds first solutions     */
  KHE_SOLN_GROUP	soln_group;		/* holds all kept solutions  */
  int			ps_make;		/* ps_make option            */
  bool			ps_no_diversify;	/* ps_no_diversify option    */
  int			ps_keep;		/* ps_keep option            */
  bool			ps_recombine;		/* ps_recombine option       */
  KHE_PS_TIME		ps_time_measure;	/* ps_time_measure option    */
  float			ps_time_limit;		/* time limit, or -1 if none */
  size_t		ps_avail_mem;		/* total available memory    */
  char			*cache;			/* intermediate results here */
  int			curr_instance;		/* currently on this         */
  ARRAY_PINST		instances;		/* instances to be solved    */
  ARRAY_PARTIAL_COST	partial_cost_free_list;
  int			ps_threads;		/* ps_threads option         */
  PTHREAD		leader_thread;		/* leader thread             */
  ARRAY_PTHREAD		child_threads;		/* child threads             */
#if KHE_USE_PTHREAD
  pthread_mutex_t	mutex;			/* locks ps and its insts    */
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PARTIAL_COST"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PARTIAL_COST PartialCostMake(KHE_SOLN soln, PSOLVER ps)                  */
/*                                                                           */
/*  Make a new, empty partial cost object.                                   */
/*                                                                           */
/*****************************************************************************/

static PARTIAL_COST PartialCostMake(KHE_SOLN soln, PSOLVER ps)
{
  PARTIAL_COST res;
  if( HaArrayCount(ps->partial_cost_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ps->partial_cost_free_list);
    HaArrayClear(res->costs);
  }
  else
  {
    HaMake(res, ps->arena);
    HaArrayInit(res->costs, ps->arena);
  }
  res->soln = soln;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KhePartialCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)             */
/*                                                                           */
/*  Return the partial cost of soln for rt.                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KhePartialCost(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  KHE_COST res;  int i, j;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  /* KHE_EVENT_GROUP eg; */  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;
  KHE_ASSIGN_RESOURCE_MONITOR arm;
  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;
  KHE_AVOID_CLASHES_MONITOR acm;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  KHE_LIMIT_WORKLOAD_MONITOR lwm;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  if( DEBUG10 )
    fprintf(stderr, "[ KhePartialCost(soln of %s, %d monitors, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), KheSolnMonitorCount(soln),
      KheResourceTypeId(rt));
  res = 0;
  for( i = 0;  i < KheSolnMonitorCount(soln);  i++ )
  {
    m = KheSolnMonitor(soln, i);
    if( DEBUG10 )
      KheMonitorDebug(m, 2, 4, stderr);
    if( KheMonitorCost(m) > 0 ) switch( KheMonitorTag(m) )
    {
      case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
	er = KheAssignResourceMonitorEventResource(arm);
	if( KheEventResourceResourceType(er) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_ASSIGN_TIME_MONITOR_TAG:
      case KHE_SPLIT_EVENTS_MONITOR_TAG:
      case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

	/* nothing to do here */
	break;

      case KHE_PREFER_RESOURCES_MONITOR_TAG:

	prm = (KHE_PREFER_RESOURCES_MONITOR) m;
	er = KhePreferResourcesMonitorEventResource(prm);
	if( KheEventResourceResourceType(er) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_PREFER_TIMES_MONITOR_TAG:

	/* nothing to do here */
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

	asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
	asac = KheAvoidSplitAssignmentsMonitorConstraint(asam);
	j = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
	/* eg = KheAvoidSplitAssignmentsConstraintEventGroup(asac, j); */
	if( KheAvoidSplitAssignmentsConstraintEventResourceCount(asac, j) > 0 )
	{
	  er = KheAvoidSplitAssignmentsConstraintEventResource(asac, j, 0);
	  if( KheEventResourceResourceType(er) == rt )
	    res += KheMonitorCost(m);
	}
	break;

      case KHE_SPREAD_EVENTS_MONITOR_TAG:
      case KHE_LINK_EVENTS_MONITOR_TAG:
      case KHE_ORDER_EVENTS_MONITOR_TAG:

	/* nothing to do here */
	break;

      case KHE_AVOID_CLASHES_MONITOR_TAG:

	acm = (KHE_AVOID_CLASHES_MONITOR) m;
	r = KheAvoidClashesMonitorResource(acm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) m;
	r = KheAvoidUnavailableTimesMonitorResource(autm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:

	litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) m;
	r = KheLimitIdleTimesMonitorResource(litm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	r = KheClusterBusyTimesMonitorResource(cbtm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	r = KheLimitBusyTimesMonitorResource(lbtm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

	lwm = (KHE_LIMIT_WORKLOAD_MONITOR) m;
	r = KheLimitWorkloadMonitorResource(lwm);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	r = KheLimitActiveIntervalsMonitorResource(laim);
	if( KheResourceResourceType(r) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	lrc = KheLimitResourcesMonitorConstraint(lrm);
	rg = KheLimitResourcesConstraintDomain(lrc);
	if( KheResourceGroupResourceType(rg) == rt )
	  res += KheMonitorCost(m);
	break;

      case KHE_EVENT_TIMETABLE_MONITOR_TAG:
      case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
      case KHE_ORDINARY_DEMAND_MONITOR_TAG:
      case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
      case KHE_EVENNESS_MONITOR_TAG:
      case KHE_GROUP_MONITOR_TAG:

	/* nothing to do here */
	break;

      default:

	HnAbort("KhePartialCost internal error (monitor tag %d)",
          KheMonitorTag(m));
    }
  }
  if( DEBUG10 )
    fprintf(stderr, "] KhePartialCost returning %.5f\n", KheCostShow(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  PARTIAL_COST PartialCostBuild(KHE_SOLN soln, PSOLVER ps)                 */
/*                                                                           */
/*  Build a partial cost object for soln.                                    */
/*                                                                           */
/*****************************************************************************/

static PARTIAL_COST PartialCostBuild(KHE_SOLN soln, PSOLVER ps)
{
  PARTIAL_COST res;  KHE_INSTANCE ins;  int i;  KHE_RESOURCE_TYPE rt;
  res = PartialCostMake(soln, ps);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    HaArrayAddLast(res->costs, KhePartialCost(soln, rt));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PartialCostDebug(PARTIAL_COST pcost, int verbosity, int indent,     */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of pcost onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void PartialCostDebug(PARTIAL_COST pcost, int verbosity, int indent,
  FILE *fp)
{
  KHE_COST cost, total_cost;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ PartialCost %s div %2d: ",
    KheInstanceId(KheSolnInstance(pcost->soln)),
    KheSolnDiversifier(pcost->soln));
  total_cost = 0;
  HaArrayForEach(pcost->costs, cost, i)
  {
    if( i > 0 )
      fprintf(fp, " + ");
    fprintf(fp, "%.5f", KheCostShow(cost));
    total_cost += cost;
  }
  fprintf(fp, " = %.5f (soln cost %.5f) ]", KheCostShow(total_cost),
    KheCostShow(KheSolnCost(pcost->soln)));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PINST - recombining"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool PInstSuitsRecombine(PINST pinst)                                    */
/*                                                                           */
/*  Return true if pinst suits recombining.                                  */
/*                                                                           */
/*****************************************************************************/

static bool PInstSuitsRecombine(PINST pinst)
{
  if( DEBUG9 )
    fprintf(stderr, "  PInstSuitsRecombine = %s && %d >= 2 && %d >= 2\n",
      bool_show(KheInstanceAllEventsHavePreassignedTimes(pinst->instance)),
      KheInstanceResourceTypeCount(pinst->instance),
      HaArrayCount(pinst->all_solns));
  return KheInstanceAllEventsHavePreassignedTimes(pinst->instance) &&
    KheInstanceResourceTypeCount(pinst->instance) >= 2 &&
    HaArrayCount(pinst->all_solns) >= 2;
}


/*****************************************************************************/
/*                                                                           */
/*  int PInstBestSolnIndex(PINST pinst, int rt_index)                        */
/*                                                                           */
/*  Return the index of the best solution for the resource type whose        */
/*  index is rt_index.                                                       */
/*                                                                           */
/*****************************************************************************/

static int PInstBestSolnIndex(PINST pinst, int rt_index)
{
  int i, best_i;  PARTIAL_COST pcost;  KHE_COST best_cost, cost;
  best_cost = KheCost(INT_MAX, INT_MAX);
  best_i = -1;
  HaArrayForEach(pinst->partial_costs, pcost, i)
  {
    cost = HaArray(pcost->costs, rt_index);
    if( cost < best_cost )
    {
      best_cost = cost;
      best_i = i;
    }
  }
  return best_i;
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstReviseSoln(KHE_SOLN result_soln, KHE_SOLN soln,                */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Change result_soln so that its tasks of type rt are assigned as they     */
/*  are in soln.  If that is not possible for any reason, do nothing.        */
/*                                                                           */
/*****************************************************************************/

static void PInstReviseSoln(KHE_SOLN result_soln, KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt)
{
  KHE_INSTANCE ins;  int i, j, result_task_count, task_count;
  KHE_EVENT_RESOURCE er;  KHE_TASK result_task, task;
  KHE_COST initial_cost;  KHE_RESOURCE result_task_asst, task_asst;
  if( DEBUG8 )
    fprintf(stderr, "[ PInstReviseSoln(result_soln, soln, %s)\n",
      KheResourceTypeId(rt));

  ins = KheSolnInstance(result_soln);
  HnAssert(ins == KheSolnInstance(soln), "PInstReviseSoln internal error");

  /* make sure that the revise will work */
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourceResourceType(er) == rt )
    {
      result_task_count = KheEventResourceTaskCount(result_soln, er);
      task_count = KheEventResourceTaskCount(soln, er);
      if( result_task_count != task_count )
      {
	if( DEBUG8 )
	  fprintf(stderr, "] PInstReviseSoln returning at er %d (%d != %d)",
	    i, result_task_count, task_count);
	return;
      }
      for( j = 0;  j < task_count;  j++ )
      {
	result_task = KheEventResourceTask(result_soln, er, j);
        result_task_asst = KheTaskAsstResource(result_task);
	task = KheEventResourceTask(soln, er, j);
	task_asst = KheTaskAsstResource(task);
	if( task_asst != result_task_asst )
	{
	  if( !KheTaskMoveResourceCheck(result_task, task_asst) )
	  {
	    if( DEBUG8 )
	      fprintf(stderr, "] PInstReviseSoln returning at er %d, task %d",
		i, j);
	    return;
	  }
	}
      }
    }
  }

  /* carry out the revise */
  initial_cost = KheSolnCost(result_soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourceResourceType(er) == rt )
    {
      task_count = KheEventResourceTaskCount(soln, er);
      for( j = 0;  j < task_count;  j++ )
      {
	result_task = KheEventResourceTask(result_soln, er, j);
        result_task_asst = KheTaskAsstResource(result_task);
	task = KheEventResourceTask(soln, er, j);
	task_asst = KheTaskAsstResource(task);
	if( task_asst != result_task_asst )
	{
	  if( !KheTaskMoveResource(result_task, task_asst) )
	    HnAbort("PInstReviseSoln internal error");
	}
      }
    }
  }
  if( DEBUG8 )
    fprintf(stderr, "] PInstReviseSoln returning (cost %.5f --> %.5f)\n",
      KheCostShow(initial_cost), KheCostShow(KheSolnCost(result_soln)));
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstRecombine(PINST pinst)                                         */
/*                                                                           */
/*  Recombine the solutions of pinst.                                        */
/*                                                                           */
/*****************************************************************************/

static void PInstRecombine(PINST pinst)
{
  PSOLVER ps;  KHE_SOLN soln, result_soln;  int i, best_i;  PARTIAL_COST pcost;
  KHE_RESOURCE_TYPE rt;
  if( DEBUG8 )
    fprintf(stderr, "[ PInstRecombine(soln, %s)\n",
      KheInstanceId(pinst->instance));

  /* build partial costs for all solutions */
  ps = pinst->psolver;
  HnAssert(HaArrayCount(pinst->all_solns) >= 2,
    "PInstRecombine internal error");
  HaArrayClear(pinst->partial_costs);
  HaArrayForEach(pinst->all_solns, soln, i)
  {
    pcost = PartialCostBuild(soln, ps);
    if( DEBUG8 )
      PartialCostDebug(pcost, 2, 2, stderr);
    HaArrayAddLast(pinst->partial_costs, pcost);
  }

  /* update the first solution for each resource type where it is not best */
  result_soln = HaArrayFirst(pinst->all_solns);
  for( i = 0;  i < KheInstanceResourceTypeCount(pinst->instance);  i++ )
  {
    rt = KheInstanceResourceType(pinst->instance, i);
    best_i = PInstBestSolnIndex(pinst, i);
    if( DEBUG8 )
      fprintf(stderr, "  best %d (%s): %d\n", i, KheResourceTypeId(rt), best_i);
    if( best_i > 0 )
    {
      soln = HaArray(pinst->all_solns, best_i);
      PInstReviseSoln(result_soln, soln, rt);
      KheSolnSetDiversifier(result_soln, RECOMBINED_DIVERSIFIER);
    }
  }

  /* clear out partial costs and return */
  HaArrayAppend(ps->partial_cost_free_list, pinst->partial_costs, i);
  HaArrayClear(pinst->partial_costs);

  if( DEBUG8 )
    fprintf(stderr, "] PInstRecombine returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PINST"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PINST PInstMake(PSOLVER ps, KHE_INSTANCE ins,                            */
/*    float ps_time_limit, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new pinst object, for solving ins.                                */
/*                                                                           */
/*****************************************************************************/

static PINST PInstMake(PSOLVER ps, KHE_INSTANCE ins, float ps_time_limit,
  HA_ARENA a)
{
  PINST res;
  HaMake(res, a);
  res->psolver = ps;
  res->instance = ins;
  res->ins_timer = KheTimerMake("instance", ps_time_limit, a);
  res->running_time = 0.0;
  res->first_soln = NULL;
  HaArrayInit(res->all_solns, a);
  res->started_solves = 0;
  res->running_solves = 0;
  res->have_optimal_soln = false;
  res->all_solves_ended = false;
  HaArrayInit(res->partial_costs, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool PInstSolveBegin(PINST pinst, int *diversifier, bool *first)         */
/*                                                                           */
/*  If pinst wants a solve, that is, if it has not yet started ps_make       */
/*  solves, we don't yet have an obviously optimal solution, time is         */
/*  available, and we don't already have cached solutions for pinst,         */
/*  return true, otherwise return false.                                     */
/*                                                                           */
/*  If true is returned, pinst understands that a new solve will actually    */
/*  be initiated, and it updates itself accordingly, and returns the         */
/*  appropriate diversifier to use for the new solve.                        */
/*                                                                           */
/*  Also, if this is the first solve of this instance, *first is set to      */
/*  true, otherwise false.                                                   */
/*                                                                           */
/*  This function must be run while the enclosing psolver is locked.         */
/*  Accordingly we do as little as possible; in particular we do not         */
/*  create a new solution.                                                   */
/*                                                                           */
/*****************************************************************************/
static void PSolverLock(PSOLVER ps);
static void PSolverUnLock(PSOLVER ps);
static bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins);

static bool PInstSolveBegin(PINST pinst, PTHREAD pt,
  int *diversifier, bool *first)
{
  PSOLVER ps;  bool res;
  ps = pinst->psolver;
  PSolverLock(ps);
  if( DEBUG16 )
    fprintf(stderr, "[ PInstSolveBegin(%s, thread %d)\n",
      KheInstanceId(pinst->instance), pt->pt_index);
  if( pinst->started_solves == 0 )
    KheTimerResetStartTime(pinst->ins_timer);
  if( pinst->all_solves_ended )
  {
    /* we've already finished with solving pinst */
    *diversifier = -1, *first = false, res = false;
  }
  /* ***
  else if( HaArrayCount(pinst->all_solns) >= pinst->wanted _solves )
  {
    ** we now have all the solutions we need **
    *diversifier = -1, *first = false, res = false;
  }
  *** */
  else if( pinst->started_solves >= ps->ps_make )
  {
    /* we've already started all the solves we need */
    *diversifier = -1, *first = false, res = false;
  }
  else if( pinst->have_optimal_soln )
  {
    /* we already have an optimal solution */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: only %d of %d solves "
	"started (have optimal solution)\n", KheInstanceId(pinst->instance),
	pinst->started_solves, ps->ps_make);
    *diversifier = -1, *first = false, res = false;
  }
  else if( KheTimerTimeLimitReached(pinst->ins_timer) )
  {
    /* we're out of time */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: only %d of %d solves "
	"started (time limit reached)\n", KheInstanceId(pinst->instance),
	pinst->started_solves, ps->ps_make);
    *diversifier = -1, *first = false, res = false;
  }
  else if( pinst->started_solves == 0 &&
    PSolverHasCachedSolns(ps, pinst->instance) )
  {
    /* we've already got cached solutions for pinst */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: using cached solutions from"
	" %s\n", KheInstanceId(pinst->instance), ps->cache);
    *diversifier = -1, *first = false, res = false;
  }
  else
  {
    /* OK to start off another solve of pinst */
    *diversifier = (ps->ps_no_diversify ? 0 : pinst->started_solves);
    *first = (pinst->started_solves == 0);
    pinst->started_solves++;
    pinst->running_solves++;
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: starting %ssolve %d%s\n",
	KheInstanceId(pinst->instance),
	ps->soln_group == NULL ? "NOT SAVING " : "", pinst->started_solves,
	pinst->started_solves == ps->ps_make ? " (last)" : "");
    res = true;
  }
  if( DEBUG16 )
    fprintf(stderr, "] PInstSolveBegin(%s, thread %d) returning %s\n",
      KheInstanceId(pinst->instance), pt->pt_index, bool_show(res));
  PSolverUnLock(ps);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstSolveEnd(PINST pinst, KHE_SOLN soln, bool first_of_pinst)      */
/*                                                                           */
/*  A solve of pinst that produced soln has ended, so save it and check      */
/*  whether it is optimal.  If first_of_pinst is true, this was the first    */
/*  solve of pinst started by the solver.                                    */
/*                                                                           */
/*  Alternatively, soln could be NULL.  This occurs when the solve was       */
/*  aborted because it ran out of memory.                                    */
/*                                                                           */
/*****************************************************************************/

static void PInstSolveEnd(PINST pinst, PTHREAD pt, KHE_SOLN soln,
  bool first_of_pinst)
{
  PSOLVER ps;  /* int i;  KHE_SOLN soln2;  KHE_COST soln_cost; */

  /* lock the solver */
  ps = pinst->psolver;
  PSolverLock(ps);
  if( DEBUG16 )
    fprintf(stderr, "[ PInstSolveEnd(%s, thread %d)\n",
      KheInstanceId(pinst->instance), pt->pt_index);

  /* add soln (uncopied and unreduced) to pinst->first_soln, if first */
  if( soln != NULL && first_of_pinst )
  {
    HnAssert(pinst->first_soln == NULL, "PInstSolveEnd internal error");
    pinst->first_soln = soln;
  }

  /* add soln (uncopied and unreduced) to all_solns, in unsorted order */
  if( soln != NULL )
    HaArrayAddLast(pinst->all_solns, soln);

  /* ***
  ** add soln (uncopied and unreduced) to all_solns, in sorted order **
  soln_cost = KheSolnCost(soln);
  for( i = 0;  i < HaArrayCount(pinst->all_solns);  i++ )
  {
    soln2 = HaArray(pinst->all_solns, i);
    if( soln_cost < KheSolnCost(soln2) )
      break;
  }
  HaArrayAdd(pinst->all_solns, i, soln);  ** works even at end **
  *** */

  /* one less solve running now */
  pinst->running_solves--;

  /* set have_optimal_soln if soln is optimal */
  if( soln != NULL &&
      KheSolnCost(soln) <= KheMonitorLowerBound((KHE_MONITOR) soln) )
    pinst->have_optimal_soln = true;

  /* unlock the solver */
  if( DEBUG16 )
    fprintf(stderr, "] PInstSolveEnd(%s, thread %d) returning\n",
      KheInstanceId(pinst->instance), pt->pt_index);
  PSolverUnLock(ps);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN PInstSolveEndSingle(PINST pinst, bool set_time)                 */
/*                                                                           */
/*  Similar to PInstSolveEnd, except there is supposed to be exactly one     */
/*  soln, which is returned.                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN PInstSolveEndSingle(PINST pinst, bool set_time)
{
  KHE_SOLN soln, res;
  HnAssert(HaArrayCount(pinst->all_solns) == 1,
    "PInstSolveEndSingle internal error");
  soln = HaArrayFirst(pinst->all_solns);
  if( set_time )
    KheSolnSetRunningTime(soln, pinst->running_time);
  /* KheSolnSetArenaSet(soln, pinst->psolver->arena_set); */
  res = KheSolnCopy(soln, pinst->psolver->arena_set);
  KheSolnDelete(soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstAllSolvesEnded(PINST pinst)                                    */
/*                                                                           */
/*  This function must be called when the last solve of pinst has ended.     */
/*  It saves reduced copies of the solutions in the solver's solution        */
/*  groups, optionally does recombining, and sets pinst's running time.      */
/*  The caller must ensure that the solver is locked while it runs.          */
/*                                                                           */
/*****************************************************************************/
static void PInstDebug(PINST pinst, bool omit_final_newline,
  int indent, FILE *fp);

static void PInstAllSolvesEnded(PINST pinst, PTHREAD pt)
{
  PSOLVER ps;  int i;  KHE_SOLN soln, soln_copy;  FILE *fp;
  bool soln_added_to_archive;

  ps = pinst->psolver;
  PSolverLock(ps);
  if( DEBUG14 )
    fprintf(stderr, "[ PInstAllSolvesEnded(%s, thread %d)%s\n",
      KheInstanceId(pinst->instance), pt->pt_index,
      pinst->all_solves_ended ? " - nothing to do" : "");

  if( !pinst->all_solves_ended && pinst->running_solves == 0 )
  {
    /* record that the solve has ended, so that we don't do this twice */
    pinst->all_solves_ended = true;

    /* archive reduced copy of first solution (before recombining) if wanted */
    soln_added_to_archive = false;
    if( ps->first_soln_group != NULL && pinst->first_soln != NULL )
    {
      if( DEBUG12 )
	fprintf(stderr, "  copying first soln %p (diversifier %d)\n",
	  (void *) pinst->first_soln, KheSolnDiversifier(pinst->first_soln));
      soln_copy = KheSolnCopy(pinst->first_soln, ps->arena_set);
      KheSolnTypeReduce(soln_copy, ps->soln_type, NULL);
      KheSolnGroupAddSoln(ps->first_soln_group, soln_copy);
      soln_added_to_archive = true;
    }

    /* set pinst's running time */
    pinst->running_time = KheTimerElapsedTime(pinst->ins_timer);
    if( DEBUG5 )
      fprintf(stderr, "  PInstAllSolvesEnded setting %s running time to %.1f\n",
	KheInstanceId(pinst->instance), pinst->running_time);

    /* sort pinst->all_solns so the best comes first */
    HaArraySort(pinst->all_solns, &KheIncreasingCostCmp);
    if( DEBUG2 )
      PInstDebug(pinst, true, 2, stderr);

    /* recombine if requested and suitable */
    if( ps->ps_recombine && PInstSuitsRecombine(pinst) )
    {
      if( DEBUG8 )
	fprintf(stderr, "  %s recombining\n", KheInstanceId(pinst->instance));
      PInstRecombine(pinst);
    }

    /* finish off the DEBUG2 print */
    if( DEBUG2 )
    {
      if( HaArrayCount(pinst->all_solns) > 0 )
      {
	soln = HaArrayFirst(pinst->all_solns);
	if( KheSolnDiversifier(soln) == RECOMBINED_DIVERSIFIER )
	  fprintf(stderr, " %.5f is cost of best soln (after recombining)",
	    KheCostShow(KheSolnCost(soln)));
	else
	  fprintf(stderr, " %.5f is cost of best soln (diversifier %d)",
	    KheCostShow(KheSolnCost(soln)), KheSolnDiversifier(soln));
      }
      fprintf(stderr, "\n");
    }

    /* remove and delete any solutions beyond ps_keep */
    while( HaArrayCount(pinst->all_solns) > ps->ps_keep )
    {
      soln = HaArrayLastAndDelete(pinst->all_solns);
      if( DEBUG12 )
	fprintf(stderr, "  ps_keep deleting soln %p (diversifier %d)\n",
	  (void *) soln, KheSolnDiversifier(soln));
      KheSolnDelete(soln);
    }

    /* archive reduced copies of the best solutions, if wanted */
    if( ps->soln_group != NULL )
      HaArrayForEach(pinst->all_solns, soln, i)
      {
	if( ps->ps_time_measure == KHE_PS_TIME_SHARED )
	{
	  if( DEBUG5 )
	    fprintf(stderr, "  PInstSolveEnd set soln %p run time to %.1f\n",
	      (void *) soln, pinst->running_time);
	  KheSolnSetRunningTime(soln, pinst->running_time);
	}
	if( DEBUG12 )
	  fprintf(stderr, "{ copying soln %p (diversifier %d)\n",
	    (void *) soln, KheSolnDiversifier(soln));
	soln_copy = KheSolnCopy(soln, ps->arena_set);
	if( DEBUG12 )
	  fprintf(stderr, "}\n");
	KheSolnTypeReduce(soln_copy, ps->soln_type, NULL);
	KheSolnGroupAddSoln(ps->soln_group, soln_copy);
	soln_added_to_archive = true;
	if( DEBUG5 )
	  fprintf(stderr, "   KheSolnGroupAddSoln(%p, soln %p)\n",
	    (void *) ps->soln_group, (void *) soln);
      }

    /* remove and delete all remaining solutions (not the copies) */
    while( HaArrayCount(pinst->all_solns) > 0 )
    {
      soln = HaArrayLastAndDelete(pinst->all_solns);
      if( DEBUG12 )
	fprintf(stderr, "  deleting soln %p (diversifier %d)\n",
	  (void *) soln, KheSolnDiversifier(soln));
      KheSolnDelete(soln);
    }

    /* write the archive if we've added a solution and we're caching */
    if( soln_added_to_archive && ps->cache != NULL )
    {
      HnAssert(ps->archive != NULL, "PInstSolveEnd internal error");
      if( DEBUG2 )
	fprintf(stderr, "  parallel solve of %s: writing cache file %s "
	  "after last solve\n", KheInstanceId(pinst->instance), ps->cache);
      fp = fopen(ps->cache, "w");
      HnAssert(fp != NULL, "KheArchiveParallelSolve: cannot open cache "
	"file \"%s\" for writing", ps->cache);
      KheArchiveWrite(ps->archive, false, fp);
      fclose(fp);
    }
  }
  if( DEBUG14 )
    fprintf(stderr, "] PInstAllSolvesEnded(%s, thread %d)\n",
      KheInstanceId(pinst->instance), pt->pt_index);
  PSolverUnLock(ps);
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstSolveEnded(PINST pinst, KHE_SOLN soln, bool first,             */
/*    KHE_SOLN *unwanted_soln)                                               */
/*                                                                           */
/*  Add soln to pinst.  If first is true, this is the first solution.  If    */
/*  *unwanted_soln is non-NULL on return, it is unwanted (it is neither      */
/*  the first solution nor a best solution) and should be deleted.           */
/*                                                                           */
/*  This function must be run while the enclosing psolver is locked.         */
/*  Accordingly we do as little as possible; in particular we do not         */
/*  delete any unwanted solution.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** now inlined into PInstSolveEnd
static void PInstDoSolveEnded(PINST pinst, KHE_SOLN soln, bool first_of_pinst,
  ** KHE_SOLN *unwanted_soln, ** bool *last_of_pinst)
{
  int i;  KHE_SOLN soln2;  KHE_COST soln_cost;

  ** set running time (it's updated each time a solve ends) **
  pinst->running_time = KheTimerElapsedTime(pinst->ins_timer);

  ** set first_soln, if this is the result of the first solve of pinst **
  if( first_of_pinst )
  {
    HnAssert(pinst->first_soln == NULL, "PInstSolveEnded internal error");
    pinst->first_soln = soln;
  }

  ** add soln to all_solns, in sorted order **
  soln_cost = KheSolnCost(soln);
  for( i = 0;  i < HaArrayCount(pinst->all_solns);  i++ )
  {
    soln2 = HaArray(pinst->all_solns, i);
    if( soln_cost < KheSolnCost(soln2) )
      break;
  }
  HaArrayAdd(pinst->all_solns, i, soln);  ** works even at end **

  ** set have_optimal_soln if soln is optimal **
  if( KheSolnCost(soln) <= KheMonitorLowerBound((KHE_MONITOR) soln) )
    pinst->have_optimal_soln = true;

  ** optionally remove the worst solution and make it unwanted **
  ** ***
  if( HaArrayCount(pinst->best_solns) <= pinst->psolver->ps_keep )
  {
    ** still accumulating solutions here, none are unwanted **
    *unwanted_soln = NULL;
  }
  else
  {
    ** remove the worst solution and make it unwanted if not kept as first **
    *unwanted_soln = HaArrayLastAndDelete(pinst->best_solns);
    if( *unwanted_soln == pinst->first_soln )
      *unwanted_soln = NULL;
  }
  *** **

  *last_of_pinst = (HaArrayCount(pinst->all_solns) >= pinst->wan ted_solves);
  if( DEBUG5 )
    fprintf(stderr, "  PInstSolveEnded updating %s running time to %.1f\n",
      KheInstanceId(pinst->instance), pinst->running_time);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void PInstSolveEnded(PINST pinst, KHE_SOLN soln, bool first)             */
/*                                                                           */
/*  The solve which produced soln for pinst has just ended.  If first is     */
/*  true, this is the solution made by the first solve of pinst.             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void PSolverLock(PSOLVER ps);
static void PSolverUnLock(PSOLVER ps);
static void PInstDebug(PINST pinst, int indent, FILE *fp);

static void PInstSolveEnded(PINST pinst, KHE_SOLN soln, bool first)
{
  bool last_of_pinst;  PSOLVER ps;
  ps = pinst->psolver;
  PSolverLock(ps);
  PInstDoSolveEnded(pinst, soln, first, ** &unwanted_soln, ** &last_of_pinst);
  PSolverUnLock(ps);
  ** ***
  if( unwanted_soln != NULL )
    KheSolnDelete(unwanted_soln);
  *** **
  if( last_of_pinst )
  {
    if( ps->ps_recombine && PInstSuitsRecombine(pinst) )
    {
      if( DEBUG8 )
	fprintf(stderr, "  %s recombining\n", KheInstanceId(pinst->instance));
      PInstRecombine(pinst);
    }
    if( DEBUG2 )
      PInstDebug(pinst, 2, stderr);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void PInstEndRun(PINST pinst, bool set_time)                             */
/*                                                                           */
/*  Solving of pinst has ended.  Don't actually delete pinst (it gets        */
/*  deleted when the solver's arena is deleted), but do what is needed       */
/*  when pinst will not be used any more.  If ps->soln_group is NULL,        */
/*  delete all the saved solutions.  Otherwise, optionally set their         */
/*  running times, and add them to ps->soln_group.                           */
/*                                                                           */
/*  Implementation note.  Because it may add solutions to ps->soln_group,    */
/*  PInstEndRun must be called single-threaded.                              */
/*                                                                           */
/*****************************************************************************/

/* *** now inlined into PInstSolveEnd
static void PInstEndRun(PINST pinst)
{
  int i;  KHE_SOLN soln;  PSOLVER ps;  bool soln_added;  FILE *fp;
  bool set_time;

  ** if wanted, save a copy of pinst->first_soln **
  soln_added = false;
  ps = pinst->psolver;
  set_time = (ps->ps_time_measure == KHE_PS_TIME_SHARED);
  if( pinst->first_soln != NULL )
  {
    ** if wanted, save a copy of pinst->first_soln in ps->first_soln_group **
    if( ps->first_soln_group != NULL )
    {
      KheSolnGroupAddSoln(ps->first_soln_group,
	KheSolnCopy(pinst->first_soln, ps->arena_set));
      soln_added = true;
    }

    ** delete pinst->first_soln unless it is also a best soln **
    if( !HaArrayContains(pinst->all_solns, pinst->first_soln, &i) )
      KheSolnDelete(pinst->first_soln);
    pinst->first_soln = NULL;
  }

  ** if wanted, save copies of the best solns; then delete them **
  HaArrayForEach(pinst->all_solns, soln, i)
  {
    ** if wanted, save a copy of soln in ps->soln_group **
    if( ps->soln_group != NULL )
    {
      if( set_time )
      {
	if( DEBUG5 )
	  fprintf(stderr, "  PInstEndRun set soln %p running time to %.1f\n",
            (void *) soln, pinst->running_time);
	KheSolnSetRunningTime(soln, pinst->running_time);
      }
      KheSolnGroupAddSoln(ps->soln_group, KheSolnCopy(soln, ps->arena_set));
      soln_added = true;
      if( DEBUG5 )
	fprintf(stderr, "   KheSolnGroupAddSoln(%p, soln %p)\n",
	  (void *) ps->soln_group, (void *) soln);
    }

    ** delete soln **
    KheSolnDelete(soln);
  }
  HaArrayClear(pinst->all_solns);

  ** write the archive if we've added a solution and we're caching **
  if( soln_added && ps->cache != NULL )
  {
    HnAssert(ps->archive != NULL, "PInstEndRun internal error");
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: writing cache file %s "
	"after last solve\n", KheInstanceId(pinst->instance), ps->cache);
    fp = fopen(ps->cache, "w");
    HnAssert(fp != NULL, "KheArchiveParallelSolve: cannot open cache "
      "file \"%s\" for writing", ps->cache);
    KheArchiveWrite(ps->archive, false, fp);
    fclose(fp);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void PInstDebug(PINST pinst, int indent, FILE *fp)                       */
/*                                                                           */
/*  Debug print of pinst.                                                    */
/*                                                                           */
/*****************************************************************************/

static void PInstDebug(PINST pinst, bool omit_final_newline,
  int indent, FILE *fp)
{
  int i /* , diversifier */;  char buff[20];  KHE_SOLN soln;
  PSOLVER ps;
  fprintf(fp, "%*s[ \"%s\", ", indent, "", KheInstanceId(pinst->instance));
  ps = pinst->psolver;
  if( ps->ps_make <= 0 )
    fprintf(fp, "0 solutions requested ]");
  else if( ps->ps_make == 1 )
  {
    if( ps->ps_make == HaArrayCount(pinst->all_solns) )
    {
      soln = HaArrayFirst(pinst->all_solns);
      fprintf(fp, "1 solution, in %s: cost %.5f ]",
        KheTimeShow(pinst->running_time, buff),
        KheCostShow(KheSolnCost(soln)));
    }
    else
      fprintf(fp, "1 solution requested, 0 completed ]");
  }
  else
  {
    fprintf(fp, "%d thread%s, ", pinst->psolver->ps_threads,
      pinst->psolver->ps_threads == 1 ? "" : "s");
    if( ps->ps_make == HaArrayCount(pinst->all_solns) )
      fprintf(fp, "%d solves, ", HaArrayCount(pinst->all_solns));
    else
      fprintf(fp, "%d solves (%d completed), ", ps->ps_make,
	HaArrayCount(pinst->all_solns));
    /* ***
    if( pinst->first_soln != NULL )
      fprintf(fp, "first %.5f, ", KheCostShow(KheSolnCost(pinst->first_soln)));
    *** */
    fprintf(fp, "%s:", KheTimeShow(pinst->running_time, buff));
    HaArrayForEach(pinst->all_solns, soln, i)
    {
      if( i % 8 == 0 )
	fprintf(fp, "\n%*s  ", indent, "");
      else
	fprintf(fp, " ");
      fprintf(fp, "%.5f", KheCostShow(KheSolnCost(soln)));
    }
    fprintf(fp, "\n%*s]", indent, "");
    /* ***
    if( HaArrayCount(pinst->all_solns) > 0 )
    {
      soln = HaArrayFirst(pinst->all_solns);
      diversifier = KheSolnDiversifier(soln);
      if( diversifier == RECOMBINED_DIVERSIFIER )
	fprintf(fp, " best soln (recombined) has cost %.5f",
	  KheCostShow(KheSolnCost(soln)));
      else
	fprintf(fp, " best soln (diversifier %d) has cost %.5f",
	  diversifier, KheCostShow(KheSolnCost(soln)));
    }
    *** */
  }
  if( !omit_final_newline )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PTHREAD"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PTHREAD PThreadMake(PSOLVER ps)                                          */
/*                                                                           */
/*  Make a new thread object for ps, either the leader thread or a child.    */
/*                                                                           */
/*  Implementation note 1.  It would be good to initialize the thread field  */
/*  here to a NULL value, but the pthread module does not seem to supply     */
/*  such a value.  It even says explicitly that you can't apply `==' to the  */
/*  thread type.  Setting the thread running immediately would solve this    */
/*  problem, but that leads to contention on ps->arena_set.  So we leave     */
/*  the thread field uninitialized.                                          */
/*                                                                           */
/*  Implementation note 2.  Care is needed to avoid contention for memory    */
/*  when running threads.  This is not a problem for PThreadMake, because    */
/*  it is run single-threaded.  It gives its thread its own arena set and    */
/*  its own arena lying in that arena set, and copies ps->options, so that   */
/*  there can be no contention when the thread runs.                         */
/*                                                                           */
/*****************************************************************************/

static PTHREAD PThreadMake(PSOLVER ps, int pt_index)
{
  PTHREAD res;  HA_ARENA a;  HA_ARENA_SET as;
  as = HaArenaSetMake();
  if( DEBUG11 )
    fprintf(stderr, "[ PThreadMake(pt %d)\n", pt_index);
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->psolver = ps;
  res->options = (ps->options == NULL ? NULL : KheOptionsCopy(ps->options, a));
  res->arena_set = as;
  res->pt_index = pt_index;
  if( DEBUG11 )
    fprintf(stderr, "] PThreadMake returning, arena set %p\n", (void *) as);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PThreadDelete(PTHREAD pt)                                           */
/*                                                                           */
/*  Delete pt, including merging its arena set into the solver's arena set.  */
/*                                                                           */
/*****************************************************************************/

static void PThreadDelete(PTHREAD pt)
{
  PSOLVER ps;  HA_ARENA_SET as;
  if( DEBUG11 )
    fprintf(stderr, "[ PThreadDelete(pt %d)\n", pt->pt_index);
  ps = pt->psolver;
  as = pt->arena_set;
  HaArenaDelete(pt->arena);
  HaArenaSetMerge(ps->arena_set, as);
  if( DEBUG11 )
    fprintf(stderr, "] PThreadDelete returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void PThreadLimitMemory(PTHREAD pt, size_t thread_memory_limit)          */
/*                                                                           */
/*  Limit the memory used by thread pt to thread_memory_limit bytes.         */
/*                                                                           */
/*****************************************************************************/

static void PThreadLimitMemory(PTHREAD pt, size_t thread_memory_limit)
{
  HaArenaSetLimitMemory(pt->arena_set, thread_memory_limit);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PSOLVER"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SolnTypeCheck(KHE_SOLN_TYPE soln_type, char *fn_name)               */
/*                                                                           */
/*  Check that soln_type is OK.                                              */
/*                                                                           */
/*****************************************************************************/

static void SolnTypeCheck(KHE_SOLN_TYPE soln_type, char *fn_name)
{
  switch( soln_type )
  {
    case KHE_SOLN_INVALID_PLACEHOLDER:

      HnAbort("%s: soln_type may not be KHE_SOLN_INVALID_PLACEHOLDER", fn_name);
      break;

    case KHE_SOLN_BASIC_PLACEHOLDER:
    case KHE_SOLN_WRITABLE_PLACEHOLDER:
    case KHE_SOLN_ORDINARY:

      /* these are the acceptable values, so do nothing */
      break;

    default:

      HnAbort("%s: invalid soln_type (value is %d)", fn_name, soln_type);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  PSOLVER PSolverMake(KHE_ARCHIVE archive, KHE_GENERAL_SOLVER solver,      */
/*    KHE_OPTIONS options, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,         */
/*    KHE_SOLN_GROUP first_soln_group, KHE_SOLN_GROUP soln_group,            */
/*    int ps_threads, int ps_make, bool ps_no_diversify, int ps_keep,        */
/*    bool ps_recombine, KHE_PS_TIME ps_time_measure, float ps_time_limit,   */
/*    size_t ps_avail_mem, char *cache, HA_ARENA a)                          */
/*                                                                           */
/*  Make a new psolver object with these attributes, and also with the       */
/*  appropriate number of thread objects, but initially with no instances.   */
/*                                                                           */
/*****************************************************************************/

static PSOLVER PSolverMake(KHE_ARCHIVE archive, KHE_GENERAL_SOLVER solver,
  KHE_OPTIONS options, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,
  KHE_SOLN_GROUP first_soln_group, KHE_SOLN_GROUP soln_group,
  int ps_threads, int ps_make, bool ps_no_diversify, int ps_keep,
  bool ps_recombine, KHE_PS_TIME ps_time_measure, float ps_time_limit,
  size_t ps_avail_mem, char *cache, HA_ARENA a)
{
  PSOLVER res;  int i;  PTHREAD pt;

  HaMake(res, a);
  res->archive = archive;
  res->solver = solver;
  res->options = options;
  res->soln_type = soln_type;
  res->arena_set = as;
  res->arena = a;
  res->global_timer = KheTimerMake("global", KHE_NO_TIME, a);
  res->first_soln_group = first_soln_group;
  res->soln_group = soln_group;
  if( DEBUG5 )
    fprintf(stderr, "  psolver->soln_group = %p\n", (void *) res->soln_group);
  res->ps_make = ps_make;
  res->ps_no_diversify = ps_no_diversify;
  res->ps_keep = ps_keep;
  res->ps_recombine = ps_recombine;
  res->ps_time_measure = ps_time_measure;
  res->ps_time_limit = ps_time_limit;
  res->ps_avail_mem = ps_avail_mem;
  res->cache = cache;
  res->curr_instance = 0;
  HaArrayInit(res->instances, a);
  HaArrayInit(res->partial_cost_free_list, a);
  res->ps_threads = ps_threads;
  res->leader_thread = PThreadMake(res, 0);
  if( ps_avail_mem > 0 )
    PThreadLimitMemory(res->leader_thread, ps_avail_mem / ps_threads);
  HaArrayInit(res->child_threads, a);
  for( i = 1;  i < res->ps_threads;  i++ )
  {
    pt = PThreadMake(res, i);
    if( ps_avail_mem > 0 )
      PThreadLimitMemory(pt, ps_avail_mem / ps_threads);
    HaArrayAddLast(res->child_threads, pt);
    if( DEBUG6 )
      fprintf(stderr, "  adding thread %d (pt = %p)\n", i, (void *) pt);
  }
#if KHE_USE_PTHREAD
  pthread_mutex_init(&res->mutex, NULL);
#endif
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverDelete(PSOLVER ps)                                           */
/*                                                                           */
/*  Delete ps and its thread objects, including tidying up the arena sets.   */
/*                                                                           */
/*****************************************************************************/

static void PSolverDelete(PSOLVER ps)
{
  PTHREAD pt;  int i;
  PThreadDelete(ps->leader_thread);
  HaArrayForEach(ps->child_threads, pt, i)
    PThreadDelete(pt);
  HaArenaDelete(ps->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverLoadInstance(PSOLVER ps, KHE_INSTANCE ins)                   */
/*                                                                           */
/*  Load ins into ps.                                                        */
/*                                                                           */
/*****************************************************************************/

static PINST PSolverLoadInstance(PSOLVER ps, KHE_INSTANCE ins)
{
  PINST res;
  res = PInstMake(ps, ins, ps->ps_time_limit, ps->arena);
  HaArrayAddLast(ps->instances, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverLock(PSOLVER ps)                                             */
/*                                                                           */
/*  Lock ps, or do nothing if we are not running multi-threaded.             */
/*                                                                           */
/*****************************************************************************/

static void PSolverLock(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  pthread_mutex_lock(&ps->mutex);
  if( DEBUG13 )
    fprintf(stderr, "[[[\n");
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverUnLock(PSOLVER ps)                                           */
/*                                                                           */
/*  Unlock ps, or do nothing if we are not running multi-threaded.           */
/*                                                                           */
/*****************************************************************************/

static void PSolverUnLock(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  if( DEBUG13 )
    fprintf(stderr, "]]]\n");
  pthread_mutex_unlock(&ps->mutex);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins)                 */
/*                                                                           */
/*  Return true if ps has cached solutions for ins in ps->soln_group or      */
/*  ps->first_soln_group.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins)
{
  if( ps->cache == NULL )
    return false;
  else if( ps->soln_group != NULL &&
    KheSolnSetSolnCount(KheSolnGroupInstanceSolnSet(ps->soln_group, ins)) > 0 )
    return true;
  else if( ps->first_soln_group != NULL && KheSolnSetSolnCount(
    KheSolnGroupInstanceSolnSet(ps->first_soln_group, ins)) > 0 )
    return true;
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool PSolverSolveBegin(PSOLVER ps, PINST *res, int *diversifier)         */
/*                                                                           */
/*  If there is still work to do, set *res to the pinst needing solving,     */
/*  *diversifier to a suitable diversifier, and *first to true when this     */
/*  is the first solve for this instance, and return true.  Otherwise        */
/*  return false.  Also update ps.                                           */
/*                                                                           */
/*  Implementation note.  Because ps is locked during this operation,        */
/*  we prefer to do as little as possible here, i.e. we do not create a      */
/*  new solution.                                                            */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool PSolverSolveBegin(PSOLVER ps, PINST *res, int *diversifier,
  bool *first)
{
  PINST pinst;
  PSolverLock(ps);
  *res = NULL;
  *diversifier = -1;
  *first = false;
  while( ps->curr_instance < HaArrayCount(ps->instances) )
  {
    pinst = HaArray(ps->instances, ps->curr_instance);
    if( PInstSolveBegin(pinst, diversifier, first) )
    {
      *res = pinst;
      break;
    }
    ps->curr_instance++;
  }
  PSolverUnLock(ps);
  return *res != NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void *PSolverRunOneThread(void *arg)                                     */
/*                                                                           */
/*  Thread execution function.                                               */
/*                                                                           */
/*  Implementation note.  This function is written as though it does the     */
/*  whole job itself, but each step checks whether it has already been done  */
/*  by another thread, and skips that step if so.                            */
/*                                                                           */
/*****************************************************************************/

static void *PSolverRunOneThread(void *arg)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;  bool first_of_pinst;  jmp_buf env;
  int diversifier, i;  KHE_OPTIONS options;  PTHREAD pt;  char description[40];
  pt = (PTHREAD) arg;
  ps = pt->psolver;
  if( DEBUG17 )
  {
    fprintf(stderr, "  arena set for thread #%d (initial):\n", pt->pt_index);
    HaArenaSetDebug(pt->arena_set, 2, 4, stderr);
  }
  HaArrayForEach(ps->instances, pinst, i)
  {
    /* solve pinst */
    while( PInstSolveBegin(pinst, pt, &diversifier, &first_of_pinst) )
    {
      if( DEBUG3 )
	fprintf(stderr, "  PSolverRunOneThread creating new soln\n");
      soln = KheSolnMake(pinst->instance, pt->arena_set);
      KheSolnSetDiversifier(soln, diversifier);
      if( DEBUG12 )
	fprintf(stderr, "  #%d: made soln %p (diversifier %d)\n",
	  pt->pt_index, (void *) soln, KheSolnDiversifier(soln));
      options = (pt->options == NULL ? NULL :
	KheOptionsCopy(pt->options, pt->arena));
      if( setjmp(env) == 0 )
      {
	KheSolnJmpEnvBegin(soln, &env);
	soln = ps->solver(soln, options);
	if( DEBUG12 )
	  fprintf(stderr, "  #%d: solve returning soln %p (diversifier %d)\n",
	    pt->pt_index, (void *) soln, KheSolnDiversifier(soln));
	KheSolnJmpEnvEnd(soln);
	snprintf(description, 40, "diversifier %d", KheSolnDiversifier(soln));
	KheSolnSetDescription(soln, description);  /* will copy description */
      }
      else
      {
	if( DEBUG7 )
	  fprintf(stderr, "  long jump (diversifier %d ran out of memory)\n",
	    KheSolnDiversifier(soln));
	if( DEBUG2 )
	  fprintf(stderr, "  one solve of %s (with diversifer %d) aborted "
	    "(out of memory)\n", KheInstanceId(pinst->instance),
	    KheSolnDiversifier(soln));
	KheSolnJmpEnvEnd(soln);
	KheSolnDelete(soln);
	soln = NULL;
	/* ***
	snprintf(description, 40, "diversifier %d (ran out of memory)",
	  KheSolnDiversifier(soln));
	*** */
      }
      PInstSolveEnd(pinst, pt, soln, first_of_pinst);
    }
    if( DEBUG17 )
    {
      fprintf(stderr, "  arena set for thread #%d (after inst %d):\n",
	pt->pt_index, i);
      HaArenaSetDebug(pt->arena_set, 2, 4, stderr);
    }
    PInstAllSolvesEnded(pinst, pt);
  }
  return NULL;
}

/* ***
static void *PSolverRunOneThread(void *arg)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;  bool first_of_pinst;  jmp_buf env;
  int diversifier;  KHE_OPTIONS options;  PTHREAD pt;  char description[40];
  pt = (PTHREAD) arg;
  ps = pt->psolver;
  while( PSolverSolveBegin(ps, &pinst, &diversifier, &first_of_pinst) )
  {
    if( DEBUG3 )
      fprintf(stderr, "  PSolverRunOneThread creating new soln\n");
    soln = KheSolnMake(pinst->instance, pt->arena_set);
    KheSolnSetDiversifier(soln, diversifier);
    if( DEBUG12 )
      fprintf(stderr, "  #%d: made soln %p (diversifier %d)\n",
       	pt->pt_index, (void *) soln, KheSolnDiversifier(soln));
    options = (pt->options == NULL ? NULL :
      KheOptionsCopy(pt->options, pt->arena));
    if( setjmp(env) == 0 )
    {
      KheSolnJmpEnvBegin(soln, &env);
      soln = ps->solver(soln, options);
      if( DEBUG12 )
	fprintf(stderr, "  #%d: solve returning soln %p (diversifier %d)\n",
	  pt->pt_index, (void *) soln, KheSolnDiversifier(soln));
      KheSolnJmp EnvEnd(soln);
      snprintf(description, 40, "diversifier %d", KheSolnDiversifier(soln));
    }
    else
    {
      if( DEBUG7 )
	fprintf(stderr, "  long jump (diversifier %d ran out of memory)\n",
	  KheSolnDiversifier(soln));
      KheSolnJmpE nvEnd(soln);
      snprintf(description, 40, "diversifier %d (ran out of memory)",
	KheSolnDiversifier(soln));
    }
    KheSolnSetDescription(soln, description);  ** will copy description **
    PInstSolveEnd(pinst, soln, first_of_pinst);
  }
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void PSolverRun(PSOLVER ps)                                              */
/*                                                                           */
/*  Run ps (called by the leader thread).                                    */
/*                                                                           */
/*****************************************************************************/

static void PSolverRun(PSOLVER ps)
{
  int i;  PTHREAD pt;
  if( DEBUG4 )
    fprintf(stderr, "[ PSolverRun(ps)\n");
  ps->curr_instance = 0;

#if KHE_USE_PTHREAD

  /***********************************************************************/
  /*                                                                     */
  /*  multi-threaded version                                             */
  /*                                                                     */
  /***********************************************************************/

  /* set the child threads running */
  HaArrayForEach(ps->child_threads, pt, i)
  {
    if( DEBUG6 )
      fprintf(stderr, "  calling pthread_create %d (pt = %p)\n",
	i, (void *) pt);
    pthread_create(&pt->thread, NULL, &PSolverRunOneThread, (void *) pt);
  }

  /* set this thread running */
  pt = ps->leader_thread;
  pt->thread = pthread_self();  /* actually unused */
  PSolverRunOneThread((void *) pt);

  /* wait for the child threads to terminate */
  HaArrayForEach(ps->child_threads, pt, i)
    pthread_join(pt->thread, NULL);

#else

  /***********************************************************************/
  /*                                                                     */
  /*  single-threaded version                                            */
  /*                                                                     */
  /***********************************************************************/

  /* set this thread running */
  pt = ps->leader_thread;
  PSolverRunOneThread((void *) pt);

#endif

  /* all done */
  if( DEBUG4 )
    fprintf(stderr, "] PSolverRun\n");
}


/* *** old version
static void PSolverRun(PSOLVER ps)
{
  if( DEBUG4 )
    fprintf(stderr, "[ PSolverRun(ps); before evening out\n");
#if KHE_USE_PTHREAD
  {
    int i;  PTHREAD pt;

    ** set the other threads running from the first instance **
    ps->curr_instance = 0;
    HaArrayForEach(ps->other_threads, pt, i)
    {
      if( DEBUG6 )
	fprintf(stderr, "  calling pthread_create %d (pt = %p)\n",
	  i, (void *) pt);
      pthread_create(&pt->thread, NULL, &PSolverRunOneThread, (void *) pt);
    }

    ** run this thread **
    ps->thread = pthread_self();  ** actually unused **
    PSolverRunOneThread((void *) (PTHREAD) ps);

    ** wait for the other threads to terminate **
    HaArrayForEach(ps->other_threads, pt, i)
      pthread_join(pt->thread, NULL);
  }
#else
  PSolverRunOneThread((void *) (PTHREAD) ps);
#endif
  if( DEBUG4 )
    fprintf(stderr, "] PSolverRun\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP KheGetSolnGroup(KHE_ARCHIVE archive,                      */
/*    char *sg_name, bool allow_existing)                                    */
/*                                                                           */
/*  Get a soln group from archive whose name is sg_name.  If allow_existing  */
/*  is true, this solution group may be an existing soln group if there      */
/*  is one with a suitable name, otherwise it must be new.                   */
/*                                                                           */
/*  If sg_name is NULL, return NULL.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN_GROUP KheGetSolnGroup(KHE_ARCHIVE archive,
  char *sg_name, bool allow_existing)
{
  static char version_str[200];  KHE_SOLN_GROUP res;

  /* return NULL if sg_name is NULL */
  if( sg_name == NULL )
    return NULL;

  if( !KheArchiveRetrieveSolnGroup(archive, sg_name, &res) )
  {
    /* no existing soln group with this name, so make one */
    sprintf(version_str, "Produced by KHE Version %s", KheVersionNumber());
    if( !KheSolnGroupMake(archive, sg_name, &res) )
      HnAbort("KheGetSolnGroup internal error");
    KheSolnGroupSetMetaData(res, "Jeffrey H. Kingston",
      KheDateToday(), version_str, NULL, NULL);
    return res;
  }
  else if( allow_existing )
  {
    /* existing soln group with this name; it's allowed, so return it */
    return res;
  }
  else
  {
    /* existing soln group with this name; it's not allowed, so abort */
    HnAbort("KheArchiveParallelSolve: soln group \"%s\" already present",
      sg_name);
    return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  size_t GetAvailMemFromSysInfo(void)                                      */
/*                                                                           */
/*  Get available memory from the Linux sysinfo system call.                 */
/*                                                                           */
/*****************************************************************************/

#if KHE_USE_SYSINFO
static size_t GetAvailMemFromSysInfo(void)
{
  struct sysinfo info;
  return (sysinfo(&info) == 0 ? (size_t) info.freeram * info.mem_unit : 0);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheGetPSOptions(KHE_OPTIONS options, bool keep_is_one,              */
/*    int *ps_threads, int *ps_make, bool *ps_no_diversify, int *ps_keep,    */
/*    char **ps_soln_group, char **ps_first_soln_group,                      */
/*    KHE_PS_TIME *ps_time_measure, float *ps_time_limit,                    */
/*    size_t *ps_avail_mem, bool *ps_use_cache)                              */
/*                                                                           */
/*  Get the parallel solve options from options.  If keep_is_one, set        */
/*  ps_keep to 1 regardless of what options says.                            */
/*                                                                           */
/*****************************************************************************/

static void KheGetPSOptions(KHE_OPTIONS options, bool keep_is_one,
  int *ps_threads, int *ps_make, bool *ps_no_diversify, int *ps_keep,
  bool *ps_recombine, char **ps_soln_group, char **ps_first_soln_group,
  KHE_PS_TIME *ps_time_measure, float *ps_time_limit,
  size_t *ps_avail_mem, bool *ps_use_cache)
{
  char *str, *dft_ps_avail_mem;

  /* ps_threads */
#if KHE_USE_PTHREAD
  *ps_threads = KheOptionsGetInt(options, "ps_threads", 1);
#else
  *ps_threads = 1;
#endif
  HnAssert(*ps_threads >= 1, "KheParallelSolve: ps_threads < 1");

  /* ps_make */
  *ps_make = KheOptionsGetInt(options, "ps_make", 1);
  HnAssert(*ps_make >= 1, "KheParallelSolve: ps_make < 1");

  /* ps_no_diversify */
  *ps_no_diversify = KheOptionsGetBool(options, "ps_no_diversify", false);

  /* ps_keep */
  *ps_keep = (keep_is_one ? 1 : KheOptionsGetInt(options, "ps_keep", 1));
  HnAssert(*ps_keep >= 0, "KheParallelSolve: ps_keep < 0");

  /* ps_recombine */
  *ps_recombine = KheOptionsGetBool(options, "ps_recombine", true);

  /* ps_soln_group */
  *ps_soln_group = KheOptionsGet(options, "ps_soln_group", NULL);

  /* ps_first_soln_group */
  *ps_first_soln_group = KheOptionsGet(options, "ps_first_soln_group", NULL);

  /* ps_time_measure */
  str = KheOptionsGet(options, "ps_time_measure", "auto");
  if( strcmp(str, "auto") == 0 )
    *ps_time_measure = (*ps_keep < *ps_make ?
      KHE_PS_TIME_SHARED : KHE_PS_TIME_OMIT);
  else if( strcmp(str, "omit") == 0 )
    *ps_time_measure = KHE_PS_TIME_OMIT;
  else if( strcmp(str, "shared") == 0 )
    *ps_time_measure = KHE_PS_TIME_SHARED;
  else
  {
    HnAbort("KheParallelSolve: ps_time_measure option has unknown value \"%s\"",
      str);
    *ps_time_measure = KHE_PS_TIME_OMIT;  /* keep compiler happy */
  }

  /* ps_time_limit */
  *ps_time_limit =
    KheTimeFromString(KheOptionsGet(options, "ps_time_limit", "-"));

  /* ps_avail_mem */
#if KHE_USE_SYSINFO
    dft_ps_avail_mem = "sysinfo";
#else
    dft_ps_avail_mem = "0";
#endif
  str = KheOptionsGet(options, "ps_avail_mem", dft_ps_avail_mem);
  if( strcmp(str, "sysinfo") == 0 )
  {
#if KHE_USE_SYSINFO
    *ps_avail_mem = GetAvailMemFromSysInfo();
#else
    HnAbort("KheParallelSolve: invalid ps_avail_mem value \"sysinfo\" "
      "(KHE_USE_SYSINFO not set)");
    *ps_avail_mem = 0;  /* keep compiler happy */
#endif
  }
  else if( sscanf(str, "%lu", ps_avail_mem) != 1 )
    HnAbort("KheParallelSolve: invalid ps_avail_mem value \"%s\"", str);
  if( DEBUG18 )
    fprintf(stderr, "  *ps_avail_mem = %lu\n", *ps_avail_mem);

  /* ps_use_cache */
  *ps_use_cache = KheOptionsGetBool(options, "ps_use_cache", false);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveParallelSolve(KHE_ARCHIVE archive,                        */
/*    KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,                        */
/*    KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)                              */
/*                                                                           */
/*  Solve the instances of archive in parallel.                              */
/*                                                                           */
/*****************************************************************************/

void KheArchiveParallelSolve(KHE_ARCHIVE archive,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)
{
  PSOLVER ps;  /* PINST pinst; */  KHE_SOLN soln;  bool resumed;
  KHE_SOLN_GROUP first_soln_group, soln_group;  HA_ARENA a;
  KHE_INSTANCE ins;  KHE_PS_TIME ps_time_measure;  int i;
  float ps_time_limit; int ps_threads, ps_make, ps_keep;
  bool ps_recombine, ps_no_diversify;  KML_ERROR ke;
  char buff[20];  size_t ps_avail_mem;  bool ps_use_cache;  FILE *fp;
  char *ps_soln_group, *ps_first_soln_group, *cache;

  /* get options */
  KheGetPSOptions(options, false, &ps_threads, &ps_make, &ps_no_diversify,
    &ps_keep, &ps_recombine, &ps_soln_group, &ps_first_soln_group,
    &ps_time_measure, &ps_time_limit, &ps_avail_mem, &ps_use_cache);

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheArchiveParallelSolve(%s) soln_group %s\n",
      KheArchiveId(archive) == NULL ? "unnamed archive" : KheArchiveId(archive),
      ps_soln_group == NULL ? "NOT SAVING" : ps_soln_group);
    fprintf(stderr, "  threads %d, make %d, keep %d, recombine %s, time %s, "
      "limit %s, use_cache %s)\n", ps_threads, ps_make, ps_keep,
      bool_show(ps_recombine),
      ps_time_measure == KHE_PS_TIME_OMIT ? "omit" : "shared",
      KheTimeShow(ps_time_limit, buff), bool_show(ps_use_cache));
  }

  /* optionally get the cache file name and replace archive with its contents */
  a = HaArenaMake(as);
  cache = (ps_use_cache && (ps_soln_group!=NULL || ps_first_soln_group!=NULL) ?
    HnStringMake(a, "tmp_%s.xml", KheArchiveId(archive)) : NULL);
  resumed = false;
  if( cache != NULL && (fp = fopen(cache, "r")) != NULL )
  {
    if( DEBUG2 )
      fprintf(stderr, "  resuming solving using cache file %s\n", cache);
    resumed = true;
    KheArchiveClear(archive);
    if( !KheArchiveLoad(archive, fp, &ke, false, true /* choice still to do */,
      true, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL) )
      HnFatal("%s:%d:%d: %s\n", cache, KmlErrorLineNum(ke),
	KmlErrorColNum(ke), KmlErrorString(ke));
    fclose(fp);
  }

  /* get soln groups */
  first_soln_group = KheGetSolnGroup(archive, ps_first_soln_group, resumed);
  soln_group = KheGetSolnGroup(archive, ps_soln_group, resumed);

  /* create a psolver with all these attributes */
  SolnTypeCheck(soln_type, "KheArchiveParallelSolve");
  ps = PSolverMake(archive, solver, options, soln_type, as, first_soln_group,
    soln_group, ps_threads, ps_make, ps_no_diversify, ps_keep, ps_recombine,
    ps_time_measure, ps_time_limit, ps_avail_mem, cache, a);

  /* solve the archive, either all together or instance by instance */
  switch( ps_time_measure )
  {
    case KHE_PS_TIME_OMIT:

      /* load and solve the whole archive */
      for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
      {
	ins = KheArchiveInstance(archive, i);
	PSolverLoadInstance(ps, ins);
      }
      PSolverRun(ps);
      HaArrayClear(ps->instances);

      /* ***
      ** add the surviving solutions to soln_group, if any **
      HaArrayForEach(ps->instances, pinst, i)
	PInstEndRun(pinst);
      HaArrayClear(ps->instances);
      *** */
      break;

    case KHE_PS_TIME_SHARED:

      for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
      {
	/* load and solve the i'th instance of archive */
	ins = KheArchiveInstance(archive, i);
	PSolverLoadInstance(ps, ins);
	PSolverRun(ps);
	HaArrayClear(ps->instances);

	/* add the surviving solutions to soln_group, if any; set run times */
	/* PInstEndRun(pinst); */
      }
      break;

    default:

      HnAbort("KheArchiveParallelSolve: internal error 1");
      break;
  }

  /* debug prints */
  if( DEBUG1 )
  {
    if( soln_group != NULL && KheSolnGroupSolnCount(soln_group) != 0 )
    {
      fprintf(stderr, "  KheArchiveParallelSolve best solution%s:\n",
	KheSolnGroupSolnCount(soln_group) != 1 ? "s" : "");
      for( i = 0;  i < KheSolnGroupSolnCount(soln_group);  i++ )
      {
	soln = KheSolnGroupSoln(soln_group, i);
	KheSolnDebug(soln, 2, 2, stderr);
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheArchiveParallelSolve returning (%s elapsed)\n",
      KheTimeShow(KheTimerElapsedTime(ps->global_timer), buff));

  /* delete the solver, including tidying up arena sets */
  PSolverDelete(ps);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheInstanceParallelSolve(KHE_INSTANCE ins,                      */
/*    KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,                        */
/*    KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)                              */
/*                                                                           */
/*  Like KheArchiveParallelSolve except that it solves only one instance     */
/*  and saves (and returns) any one best solution.                           */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheInstanceParallelSolve(KHE_INSTANCE ins,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;
  int ps_threads, ps_make, ps_keep;
  bool ps_recombine, ps_no_diversify /*, ps_reverse_order*/;
  KHE_PS_TIME ps_time_measure;  float ps_time_limit;  size_t ps_avail_mem;
  bool ps_use_cache;  HA_ARENA a;  char *ps_soln_group, *ps_first_soln_group;

  /* get options */
  KheGetPSOptions(options, true, &ps_threads, &ps_make, &ps_no_diversify,
    &ps_keep, &ps_recombine, &ps_soln_group, &ps_first_soln_group,
    &ps_time_measure, &ps_time_limit, &ps_avail_mem, &ps_use_cache);
  if( DEBUG2 )
    fprintf(stderr, "[ KheInstanceParallelSolve(threads %d, make %d)\n",
      ps_threads, ps_make);

  /* create a psolver object and load and solve ins */
  SolnTypeCheck(soln_type, "KheInstanceParallelSolve");
  a = HaArenaMake(as);
  ps = PSolverMake(NULL, solver, options, soln_type, as, NULL, NULL,
    ps_threads, ps_make, ps_no_diversify, 1, ps_recombine, ps_time_measure,
    ps_time_limit, /* ps_reverse_order, */ ps_avail_mem, NULL, a);
  pinst = PSolverLoadInstance(ps, ins);
  PSolverRun(ps);

  /* extract the sole surviving solution */
  soln = PInstSolveEndSingle(pinst, ps_time_measure == KHE_PS_TIME_SHARED);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final solution:\n");
    KheSolnDebug(soln, 2, 2, stderr);
    fprintf(stderr, "] KheInstanceParallelSolve returning\n");
  }
  return soln;
}

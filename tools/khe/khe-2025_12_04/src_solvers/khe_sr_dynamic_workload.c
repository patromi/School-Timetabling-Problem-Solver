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
/*  FILE:         khe_sr_dynamic_workload.c                                  */
/*  DESCRIPTION:  Dynamic workload balancing                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#include <float.h>

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_RESOURCE_CLASS                                                  */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_RESOURCE_OVERLOADED,
  KHE_RESOURCE_UNDERLOADED,
  KHE_RESOURCE_NEUTRAL
} KHE_RESOURCE_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DW_SOLVER                                                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dw_solver_rec {
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;
  KHE_RESOURCE_SET		overloaded_resources;
  KHE_RESOURCE_SET		underloaded_resources;
  KHE_RESOURCE_SET		neutral_resources;
  KHE_TASK_SET			scratch_task_set;
  KHE_DYNAMIC_RESOURCE_SOLVER	drs;
} *KHE_DW_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_CLASS"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_CLASS KheResourceClass(KHE_RESOURCE r, KHE_SOLN soln)       */
/*                                                                           */
/*  Return the class of r in soln:  overloaded, underloaded, or neither.     */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_CLASS KheResourceClass(KHE_RESOURCE r, KHE_SOLN soln)
{
  int avail_busy;  float avail_workload;
  KheResourceAvailableBusyTimes(soln, r, &avail_busy);
  KheResourceAvailableWorkload(soln, r, &avail_workload);
  if( avail_busy < 0 || avail_workload < 0.0 )
    return KHE_RESOURCE_OVERLOADED;
  else if( avail_busy > 0 && avail_workload > 0.0 )
    return KHE_RESOURCE_UNDERLOADED;
  else
    return KHE_RESOURCE_NEUTRAL;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceClassShow(KHE_RESOURCE_CLASS rc)                        */
/*                                                                           */
/*  Return a string display of rc.                                           */
/*                                                                           */
/*****************************************************************************/

static char *KheResourceClassShow(KHE_RESOURCE_CLASS rc)
{
  switch(rc)
  {
    case KHE_RESOURCE_OVERLOADED:	return "overloaded";
    case KHE_RESOURCE_UNDERLOADED:	return "underloaded";
    case KHE_RESOURCE_NEUTRAL:		return "neutral";
    default:				return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DW_SOLVER"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DW_SOLVER KheDwSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,       */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new, empty solver with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_DW_SOLVER KheDwSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_FRAME days_frame, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_DW_SOLVER res;
  HaMake(res, a);
  res->soln = soln;
  res->resource_type = rt;
  res->days_frame = days_frame;
  res->overloaded_resources = KheResourceSetMake(rt, a);
  res->underloaded_resources = KheResourceSetMake(rt, a);
  res->neutral_resources = KheResourceSetMake(rt, a);
  res->scratch_task_set = KheTaskSetMake(soln);
  res->drs = KheDynamicResourceSolverMake(soln, rt, options);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDwSolverAddResource(KHE_DW_SOLVER dws, KHE_RESOURCE r)           */
/*                                                                           */
/*  Add r to dws.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheDwSolverAddResource(KHE_DW_SOLVER dws, KHE_RESOURCE r)
{
  switch( KheResourceClass(r, dws->soln) )
  {
    case KHE_RESOURCE_OVERLOADED:

      KheResourceSetAddResource(dws->overloaded_resources, r);
      break;

    case KHE_RESOURCE_UNDERLOADED:

      KheResourceSetAddResource(dws->underloaded_resources, r);
      break;

    case KHE_RESOURCE_NEUTRAL:

      KheResourceSetAddResource(dws->neutral_resources, r);
      break;

    default:

      HnAbort("KheDwSolverAddResource internal error");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMakeResourceFree(KHE_RESOURCE r, KHE_TIME_GROUP tg,              */
/*    KHE_DW_SOLVER dws)                                                     */
/*                                                                           */
/*  Make r free at the times of tg in soln.  Return true if all freed.       */
/*                                                                           */
/*****************************************************************************/

static bool KheMakeResourceFree(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  KHE_DW_SOLVER dws)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i;  KHE_TASK task;  bool res;

  /* find the tasks that are making r busy during tg */
  KheTaskSetClear(dws->scratch_task_set);
  rtm = KheResourceTimetableMonitor(dws->soln, r);
  KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg, true,
    dws->scratch_task_set);

  /* free them all, as far as possible;  NB KheTaskSetUnAssignResource */
  /* does not suit here because it quits early if one unassignment fails */
  res = true;
  for( i = 0;  i < KheTaskSetTaskCount(dws->scratch_task_set);  i++ )
  {
    task = KheTaskSetTask(dws->scratch_task_set, i);
    if( !KheTaskUnAssign(task) )
      res = false;
  }

  /* all done */
  if( DEBUG2 )
    fprintf(stderr, "  %s: freed %s tasks\n", KheResourceId(r),
      res ? "all" : "not all");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDwSolverDebug(KHE_DW_SOLVER dws, int verbosity, int indent,      */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of dws onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDwSolverDebug(KHE_DW_SOLVER dws, int verbosity, int indent,
  FILE *fp)
{
  fprintf(fp, "%*s[ DynamicWorkloadSolver(%s):\n", indent, "",
    KheResourceTypeId(dws->resource_type));
  if( KheResourceSetResourceCount(dws->overloaded_resources) > 0 )
  {
    fprintf(fp, "%*s  %s resources:\n", indent, "",
      KheResourceClassShow(KHE_RESOURCE_OVERLOADED));
    KheResourceSetDebug(dws->overloaded_resources, verbosity, indent + 4, fp);
  }
  if( KheResourceSetResourceCount(dws->underloaded_resources) > 0 )
  {
    fprintf(fp, "%*s  %s resources:\n", indent, "",
      KheResourceClassShow(KHE_RESOURCE_UNDERLOADED));
    KheResourceSetDebug(dws->underloaded_resources, verbosity, indent + 4, fp);
  }
  if( KheResourceSetResourceCount(dws->neutral_resources) > 0 )
  {
    fprintf(fp, "%*s  %s resources:\n", indent, "",
      KheResourceClassShow(KHE_RESOURCE_NEUTRAL));
    KheResourceSetDebug(dws->neutral_resources, verbosity, indent + 4, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDwSolverSolveOneResource(KHE_DW_SOLVER dws, KHE_RESOURCE r)      */
/*                                                                           */
/*  Solve one resource by dynamic programming.                               */
/*                                                                           */
/*  Implementation note.  The arguments of KheDynamicResourceSolverSolve     */
/*  used here are default values from khe_sr_dynamic_vlsn.c.                 */
/*                                                                           */
/*****************************************************************************/

static void KheDwSolverSolveOneResource(KHE_DW_SOLVER dws, KHE_RESOURCE r)
{
  int init_avail_busy, final_avail_busy;
  float init_avail_workload, final_avail_workload;
  KheResourceAvailableBusyTimes(dws->soln, r, &init_avail_busy);
  KheResourceAvailableWorkload(dws->soln, r, &init_avail_workload);
  KheDynamicResourceSolverAddDayRange(dws->drs, 0,
    KheFrameTimeGroupCount(dws->days_frame) - 1);
  KheDynamicResourceSolverAddResource(dws->drs, r);
  KheDynamicResourceSolverSolve(dws->drs,
   /* priqueue */ false,
   /* extra_selection */ true,
   /* expand_by_shifts */ true,
   /* shift_pairs */ false,
   /* correlated_exprs */ false,
   /* daily_expand_limit */ 10000,
   /* daily_prune_trigger */ 0,
   /* resource_expand_limit */ 0,
   /* dom_approx */ 0,
   /* main_dom_kind */ KHE_DRS_DOM_INDEXED_TABULATED,
   /* cache */ false,
   /* cache_dom_kind */ KHE_DRS_DOM_INDEXED_TABULATED);
  KheResourceAvailableBusyTimes(dws->soln, r, &final_avail_busy);
  KheResourceAvailableWorkload(dws->soln, r, &final_avail_workload);
  if( DEBUG1 )
  {
    if( DEBUG2 || final_avail_busy < init_avail_busy )
    {
      if( init_avail_busy == INT_MAX )
	fprintf(stderr, "  %s: avail_busy INT_MAX -> %d\n",
	  KheResourceId(r), final_avail_busy);
      else
	fprintf(stderr, "  %s: avail_busy %d -> %d\n",
	  KheResourceId(r), init_avail_busy, final_avail_busy);
    }
    if( final_avail_workload < init_avail_workload )
    {
      if( init_avail_workload == FLT_MAX )
	fprintf(stderr, "  %s: avail_workload FLT_MAX -> %.2f\n",
	  KheResourceId(r), final_avail_workload);
      else
	fprintf(stderr, "  %s: avail_workload %.2f -> %.2f\n",
	  KheResourceId(r), init_avail_workload, final_avail_workload);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDynamicResourceBalanceWorkloads(KHE_SOLN soln,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)                             */
/*                                                                           */
/*  Carry out workload balancing by dynamic programming.                     */
/*                                                                           */
/*****************************************************************************/

bool KheDynamicResourceBalanceWorkloads(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options)
{
  KHE_RESOURCE r;  HA_ARENA a;  int i;  KHE_DW_SOLVER dws;  bool improved;
  KHE_TIME_GROUP full_tg;  KHE_MARK mark;  KHE_COST init_cost;
  KHE_RESOURCE_GROUP rg;  KHE_FRAME days_frame;

  /* make a solver object and add the resources */
  if( DEBUG1 )
    fprintf(stderr, "[ KheDynamicResourceBalanceWorkloads(soln %.5f, %s)\n",
      KheCostShow(KheSolnCost(soln)), KheResourceTypeId(rt));
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  a = KheSolnArenaBegin(soln);
  dws = KheDwSolverMake(soln, rt, days_frame, options, a);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    KheDwSolverAddResource(dws, r);
  }
  if( DEBUG1 )
    KheDwSolverDebug(dws, 2, 2, stderr);
  init_cost = KheSolnCost(soln);

  /* set up a mark */
  mark = KheMarkBegin(soln);

  /* we need at least one underloaded resource and one overloaded resource */
  if( KheResourceSetResourceCount(dws->overloaded_resources) > 0 &&
      KheResourceSetResourceCount(dws->underloaded_resources) > 0 )
  {
    /* free the overloaded resources; also build a resource group of them */
    full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));
    KheSolnResourceGroupBegin(soln, rt);
    for(i = 0; i < KheResourceSetResourceCount(dws->overloaded_resources); i++)
    {
      r = KheResourceSetResource(dws->overloaded_resources, i);
      KheMakeResourceFree(r, full_tg, dws);
      KheSolnResourceGroupAddResource(soln, r);
    }
    rg = KheSolnResourceGroupEnd(soln);

    /* try reassigning each underloaded resource */
    for(i = 0; i < KheResourceSetResourceCount(dws->underloaded_resources); i++)
    {
      r = KheResourceSetResource(dws->underloaded_resources, i);
      KheDwSolverSolveOneResource(dws, r);
    }

    /* reassign the overloaded resources using time sweep */
    KheTimeSweepAssignResources(soln, rg, options);
  }

  /* undo if not better */
  improved = (KheSolnCost(soln) < init_cost);
  KheMarkEnd(mark, !improved);

  /* wrap up and return */
  KheDynamicResourceSolverDelete(dws->drs);
  HnAssert(KheSolnCost(soln) <= init_cost,
    "KheDynamicResourceBalanceWorkloads internal error");
  if( DEBUG1 )
  {
    fprintf(stderr, "] KheDynamicResourceBalanceWorkloads returning ");
    if( improved )
      fprintf(stderr, "true (%.5f -> %.5f)\n", KheCostShow(init_cost),
	KheCostShow(KheSolnCost(soln)));
    else
      fprintf(stderr, "false (%.5f unchanged)\n", KheCostShow(init_cost));
  }
  KheSolnArenaEnd(soln, a);
  return improved;
}


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
/*  it under the terms of the GNU Generar Public License as published by     */
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
/*  FILE:         khe_sr_rematch.c                                           */
/*  DESCRIPTION:  Resource rematching repair                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_REMATCH_TYPE - value of rs_rematch_select option                     */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_REMATCH_NONE,
  KHE_REMATCH_DEFECTIVE_TASKS,
  KHE_REMATCH_FRAME,
  KHE_REMATCH_INTERVALS
} KHE_REMATCH_TYPE;

typedef HA_ARRAY(KHE_TIME_SET) ARRAY_KHE_TIME_SET;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool TaskAssignmentIsDefective(KHE_TASK task)                            */
/*                                                                           */
/*  Return true if task or any task assigned to task is defective.           */
/*                                                                           */
/*****************************************************************************/

static bool TaskAssignmentIsDefective(KHE_TASK task)
{
  KHE_TASK sub_task;  int i;  KHE_ORDINARY_DEMAND_MONITOR odm;
  KHE_SOLN soln;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;

  /* check whether this task has a defective demand monitor (a clash) */
  for( i = 0;  i < KheTaskDemandMonitorCount(task);  i++ )
  {
    odm = KheTaskDemandMonitor(task, i);
    if( KheMonitorCost((KHE_MONITOR) odm) > 0 )
      return true;
  }

  /* check whether this task has a defective event resource monitor */
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    soln = KheTaskSoln(task);
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( KheMonitorCost(m) > 0 )
	return true;
    }
  }

  /* check for defective sub-tasks */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    sub_task = KheTaskAssignedTo(task, i);
    if( TaskAssignmentIsDefective(sub_task) )  /* bug fix here; was task */
      return true;
  }

  /* no sign of any defects */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRematchAddDefectiveTimeSets(KHE_SOLN soln,                       */
/*    KHE_RESOURCE_GROUP rg, ARRAY_KHE_TIME_SET *time_sets)                  */
/*                                                                           */
/*  Add time sets for the defective tasks of soln whose type is the type of  */
/*  rg and which are either assigned a resource from rg or are unassigned.   */
/*                                                                           */
/*****************************************************************************/

static void KheRematchAddDefectiveTimeSets(KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg, ARRAY_KHE_TIME_SET *time_sets, HA_ARENA a)
{
  KHE_TASK task;  int i;  KHE_TIME_SET ts;
  KHE_RESOURCE_TYPE rt;  bool group_is_all;  KHE_RESOURCE r;
  KHE_INSTANCE ins;

  /* find the tasking of soln whose resource type is the resource type of rg */
  if( DEBUG2 )
    fprintf(stderr, "[ KheRematchAddDefectiveTimeSets(soln, rg, *time_sets)\n");
  ins = KheSolnInstance(soln);
  rt = KheResourceGroupResourceType(rg);
  /* ***
  tasking = NULL;  ** keep compiler happy **
  for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
  {
    tasking = KheSolnTasking(soln, i);
    if( KheTaskingResourceType(tasking) == rt )
      break;
  }
  HnAssert(i < KheSolnTaskingCount(soln),
    "KheResourceRematch: no tasking with type %s", KheResourceTypeId(rt));
  *** */

  /* add one time set for each defective task */
  group_is_all = (KheResourceTypeResourceCount(rt) ==
    KheResourceGroupResourceCount(rg));
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskResourceType(task) == rt )
    {
      r = KheTaskAsstResource(task);
      if( (r == NULL || group_is_all || KheResourceGroupContains(rg, r)) &&
	  TaskAssignmentIsDefective(task) )
      {
	ts = KheTimeSetMake(ins, a);
	KheTimeSetAddTaskTimes(ts, task);
	HaArrayAddLast(*time_sets, ts);
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheRematchAddDefectiveTimeSets returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSmallestGroupSize(KHE_FRAME frame, int max_groups)                */
/*                                                                           */
/*  Return the smallest group size.  The larger the frame is, the closer     */
/*  this will be to max_groups, to reduce run time.                          */
/*                                                                           */
/*****************************************************************************/
#define max(a, b) ((a) > (b) ? (a) : (b))

static int KheSmallestGroupSize(KHE_FRAME frame, int max_groups)
{
  int count;
  count = KheFrameTimeGroupCount(frame);
  if( count <= 4 * 7 )
    return 1;
  else if( count <= 6 * 7 )
    return 2;
  else if( count <= 26 * 7 )
    return max(1, max_groups - 3);
  else
    return max(1, max_groups - 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRematchAddFrameTimeSets(KHE_FRAME frame, int max_groups,         */
/*    ARRAY_KHE_TIME_SET *time_sets)                                         */
/*                                                                           */
/*  Add time sets for subintervals of frame to *time_sets.                   */
/*                                                                           */
/*****************************************************************************/

static void KheRematchAddFrameTimeSets(KHE_FRAME frame, int max_groups,
  ARRAY_KHE_TIME_SET *time_sets, HA_ARENA a)
{
  int p, i, j;  KHE_TIME_SET ts;  KHE_TIME_GROUP tg;  KHE_INSTANCE ins;
  ins = KheFrameInstance(frame);
  for( p = KheSmallestGroupSize(frame, max_groups);  p <= max_groups;  p++ )
  {
    for( i = 0;  i <= KheFrameTimeGroupCount(frame) - p;  i += p )
    {
      ts = KheTimeSetMake(ins, a);
      for( j = 0;  j < p;  j++ )
      {
	tg = KheFrameTimeGroup(frame, i + j);
        KheTimeSetAddTimeGroup(ts, tg);
      }
      HaArrayAddLast(*time_sets, ts);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRematchAddConstraintTimeSets(                                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic,                            */
/*    int max_groups, ARRAY_KHE_TIME_SET *time_sets)                         */
/*                                                                           */
/*  Add the time sets derived from laic.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheRematchAddConstraintTimeSets(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  int max_groups, ARRAY_KHE_TIME_SET *time_sets, HA_ARENA a)
{
  int i, j, parts, count, m, offset;  KHE_TIME_SET ts;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;  KHE_INSTANCE ins;
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  ins = KheConstraintInstance((KHE_CONSTRAINT) c);
  for( m=0; m < KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c); m++ )
  {
    offset = KheLimitActiveIntervalsConstraintAppliesToOffset(c, m);
    for( parts = 1;  parts <= max_groups;  parts++ )
    {
      for( i = 0;  i <= count - parts;  i++ )
      {
	ts = KheTimeSetMake(ins, a);
	for( j = 0;  j < parts;  j++ )
	{
	  tg = KheLimitActiveIntervalsConstraintTimeGroup(c, i+j, offset, &po);
	  KheTimeSetAddTimeGroup(ts, tg);
	}
	HaArrayAddLast(*time_sets, ts);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRematchAddIntervalsTimeSets(KHE_INSTANCE ins,                    */
/*    int max_groups, ARRAY_KHE_TIME_SET *time_sets)                         */
/*                                                                           */
/*  Add time sets for intervals of the limit active intervals constraints    */
/*  of ins to time_sets.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheRematchAddIntervalsTimeSets(KHE_INSTANCE ins,
  int max_groups, ARRAY_KHE_TIME_SET *time_sets, HA_ARENA a)
{
  KHE_CONSTRAINT c;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;  int i;
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG )
    {
      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      KheRematchAddConstraintTimeSets(laic, max_groups, time_sets, a);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_REMATCH_TYPE KheRematchType(KHE_INSTANCE ins, KHE_OPTIONS options)   */
/*                                                                           */
/*  Return the type of rematching requested by the rs_rematch_select option. */
/*                                                                           */
/*****************************************************************************/

static KHE_REMATCH_TYPE KheRematchType(KHE_INSTANCE ins, KHE_OPTIONS options)
{
  char *val;
  val = KheOptionsGet(options, "rs_rematch_select", "auto");
  if( strcmp(val, "none") == 0 )
    return KHE_REMATCH_NONE;
  else if( strcmp(val, "defective_tasks") == 0 )
    return KHE_REMATCH_DEFECTIVE_TASKS;
  else if( strcmp(val, "frame") == 0 )
    return KHE_REMATCH_FRAME;
  else if( strcmp(val, "intervals") == 0 )
    return KHE_REMATCH_INTERVALS;
  else if( strcmp(val, "auto") == 0 )
    return (KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE ?
      KHE_REMATCH_DEFECTIVE_TASKS : KHE_REMATCH_FRAME);
  else
  {
    HnAbort("KheResourceRematch: invalid rs_rematch_select value \"%s\"", val);
    return KHE_REMATCH_NONE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceRematch(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,            */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out resource rematching of the resources of rg in soln.            */
/*                                                                           */
/*****************************************************************************/

bool KheResourceRematch(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_OPTIONS options, int variant)
{
  KHE_TIME_SET ts;  int i, j, max_groups;  KHE_TIMER rematch_timer, timer;
  KHE_RESOURCE_MATCHING_SOLVER rms;  ARRAY_KHE_TIME_SET time_sets;
  KHE_INSTANCE ins;  KHE_COST init_cost, cost;  float time_limit;
  bool edge_adjust1_off, edge_adjust2_off, edge_adjust3_off, edge_adjust4_off;
  bool ejection_off;
  bool /* nocost_off, */ res;  KHE_TIME t;  KHE_REMATCH_TYPE rm_type;
  KHE_FRAME frame;  KHE_RESOURCE_MATCHING_DEMAND_SET demand_set;  HA_ARENA a;

  /* quit now if rematching not wanted; also find the type of rematch */
  if( KheOptionsGetBool(options, "rs_rematch_off", false) )
    return false;
  ins = KheSolnInstance(soln);
  rm_type = KheRematchType(ins, options);
  if( rm_type == KHE_REMATCH_NONE )
    return false;

  /* boilerplate */
  init_cost = KheSolnCost(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceRematch(soln, %s, options) init cost %.5f\n",
      KheResourceGroupId(rg), KheCostShow(init_cost));

  /* get matching_off and edge_adjust123_off options */
  edge_adjust1_off = KheOptionsGetBool(options, "rs_rematch_edge_adjust1_off",
    false);
  edge_adjust2_off = KheOptionsGetBool(options, "rs_rematch_edge_adjust2_off",
    false);
  edge_adjust3_off = KheOptionsGetBool(options, "rs_rematch_edge_adjust3_off",
    false);
  edge_adjust4_off = KheOptionsGetBool(options, "rs_rematch_edge_adjust4_off",
    false);
  ejection_off = KheOptionsGetBool(options, "rs_rematch_ejection_off", false);
  /* nocost_off = KheOptionsGetBool(options, "rs_rematch_nocost_off", false); */

  /* build a solver */
  a = KheSolnArenaBegin(soln);
  rms = KheResourceMatchingSolverMake(soln, rg, options, a);

  /* get the time sets, depending on the rematch type */
  HaArrayInit(time_sets, a);
  switch( rm_type )
  {
    case KHE_REMATCH_DEFECTIVE_TASKS:

      KheRematchAddDefectiveTimeSets(soln, rg, &time_sets, a);
      break;

    case KHE_REMATCH_FRAME:

      frame = KheOptionsFrame(options, "gs_common_frame", soln);
      max_groups = KheOptionsGetInt(options, "rs_rematch_max_groups", 7);
      KheRematchAddFrameTimeSets(frame, max_groups, &time_sets, a);
      break;

    case KHE_REMATCH_INTERVALS:

      frame = KheOptionsFrame(options, "gs_common_frame", soln);
      max_groups = KheOptionsGetInt(options, "rs_rematch_max_groups", 7);
      KheRematchAddFrameTimeSets(frame, max_groups, &time_sets, a);
      KheRematchAddIntervalsTimeSets(ins, max_groups, &time_sets, a);
      break;

    case KHE_REMATCH_NONE:
    default:

      HnAbort("KheResourceRematch internal error");
  }

  /* sort the time sets and solve each distinct one */
  HaArraySortUnique(time_sets,
    variant % 2 == 0 ? &KheTimeSetCmp : &KheTimeSetCmpReverse);
  timer = NULL;
  if( HaArrayCount(time_sets) > 0 &&
      KheOptionsContainsTimer(options, "rematch", &rematch_timer) )
  {
    time_limit = KheTimerTimeLimit(rematch_timer);
    if( time_limit != KHE_NO_TIME )
      timer = KheOptionsAddTimer(options, "per_rematch",
	time_limit / HaArrayCount(time_sets));
  }

  HaArrayForEach(time_sets, ts, i)
  {
    /* reset per-rematch time limit */
    if( timer != NULL )
      KheTimerResetStartTime(timer);

    /* quit early if time limit reached */
    if( KheOptionsTimeLimitReached(options) )
      break;

    if( KheTimeSetTimeCount(ts) > 0 )
    {
      demand_set = KheResourceMatchingDemandSetMake(rms /* , false */);
      /* HaArrayForEach(ts->times, t, j) */
      for( j = 0;  j < KheTimeSetTimeCount(ts);  j++ )
      {
	t = KheTimeSetTime(ts, j);
	KheResourceMatchingDemandSetAddTime(demand_set, t);
      }
      if( DEBUG3 )
      {
	fprintf(stderr, "  calling KheResourceMatchingSolverSolve from"
	  " KheResourceRematch: ");
	KheTimeSetDebug(ts, 1, 0, stderr);
      }
      cost = KheSolnCost(soln);
      res = KheResourceMatchingSolverSolve(rms, demand_set, edge_adjust1_off,
	edge_adjust2_off, edge_adjust3_off, edge_adjust4_off, /* nocost_off, */
	ejection_off, options);
      if( DEBUG3 )
      {
	if( res )
	  fprintf(stderr, "  KheResourceMatchingSolverSolve success: %.5f -> "
	    "%.5f\n", KheCostShow(cost), KheCostShow(KheSolnCost(soln)));
	else
	  fprintf(stderr, "  KheResourceMatchingSolverSolve fail\n");
      }
      KheResourceMatchingDemandSetDelete(demand_set);
    }
  }
  if( timer != NULL )
    KheOptionsDeleteTimer(options, timer);

  /* free the time sets and solver, and return */
  /* KheResourceMatchingSolverDelete(rms); */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
  {
    if( KheSolnCost(soln) < init_cost )
      fprintf(stderr, "] KheResourceRematch returning true (%.5f --> %.5f)\n",
	KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
    else
      fprintf(stderr, "] KheResourceRematch returning false (%.5f)\n",
	KheCostShow(KheSolnCost(soln)));
  }
  return KheSolnCost(soln) < init_cost;
}

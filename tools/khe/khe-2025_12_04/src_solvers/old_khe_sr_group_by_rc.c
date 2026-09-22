
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
/*  FILE:         khe_sr_group_by_rc.c                                       */
/*  DESCRIPTION:  Group by resource constraints                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0  /* KheDoCombinatorialGroupingForInterval */
#define DEBUG3 0  /* KheDoProfile */
#define DEBUG4 0
#define DEBUG5 0  /* KheDoProfileGroupingMonitors */
#define DEBUG6 0  /* combination elimination */
#define	DEBUG7 0  /* profile grouping */
#define	DEBUG8 0  /* profile grouping */
#define	DEBUG9 0  /* special case */
#define	DEBUG10 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_EC_SOLVER - solver for eliminating combinations                      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_COMB_SOLVER_COVER_TYPE) ARRAY_KHE_COMB_SOLVER_COVER_TYPE;
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;

typedef struct khe_ec_solver_rec {
  KHE_SOLN				soln;
  KHE_FRAME				common_frame;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_RESOURCE_SET			resource_set;
  KHE_EVENT_TIMETABLE_MONITOR		etm;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT constraints;
  ARRAY_KHE_COMB_SOLVER_COVER_TYPE	cover_types;
  HA_ARRAY_INT				intersects;
  HA_ARRAY_INT				provisional_prevs;
} *KHE_EC_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_SOLVER - solver for profile grouping                         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;
typedef HA_ARRAY(KHE_RESOURCE_GROUP) ARRAY_KHE_RESOURCE_GROUP;

typedef struct khe_profile_solver_rec {
  KHE_COMB_SOLVER			comb_solver;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR monitors;
  int					min_limit;
  int					max_limit;
  bool					history_before;
  bool					history_after;
  int					groups_count;
  HA_ARRAY_INT				pos_groups;
} *KHE_PROFILE_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand at cluster monitor"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDemandAtTime(KHE_EVENT_TIMETABLE_MONITOR etm,                     */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME t)                                      */
/*                                                                           */
/*  Return the total demand for resources of type rt at time t.              */
/*                                                                           */
/*****************************************************************************/

static int KheDemandAtTime(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_RESOURCE_TYPE rt, KHE_TIME t)
{
  int j, k, res;  KHE_MEET meet;  KHE_TASK task;  KHE_RESOURCE r;
  KHE_EVENT_RESOURCE er;
  res = 0;
  for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(etm, t);  j++ )
  {
    meet = KheEventTimetableMonitorTimeMeet(etm, t, j);
    for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
    {
      task = KheMeetTask(meet, k);
      if( KheTaskResourceType(task) == rt )
      {
	if( KheTaskIsPreassigned(task, &r) )
	{
	  if( DEBUG6 )
	  {
	    fprintf(stderr, "    demand (preassigned) ");
	    KheTaskDebug(task, 2, 0, stderr);
	  }
	  res++;
	}
	else
	{
	  er = KheTaskEventResource(task);
	  if( er != NULL && KheEventResourceNeedsAssignment(er) == KHE_YES )
	  {
	    if( DEBUG6 )
	    {
	      fprintf(stderr, "    demand ");
	      KheTaskDebug(task, 2, 0, stderr);
	    }
	    res++;
	  }
	}
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandAtTimeGroup(KHE_EVENT_TIMETABLE_MONITOR etm,                */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Return the total demand for resources of type rt in the times of tg.     */
/*                                                                           */
/*****************************************************************************/

static int KheDemandAtTimeGroup(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  int i, res;  KHE_TIME t;
  res = 0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    res += KheDemandAtTime(etm, rt, t);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandAtClusterConstraint(KHE_EC_SOLVER ecs,                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int offset)                       */
/*                                                                           */
/*  Return the total demand for resources of type ecs->resource_type in the  */
/*  time groups of cluster busy times constraint c at offset.                */
/*                                                                           */
/*****************************************************************************/

static int KheDemandAtClusterConstraint(KHE_EC_SOLVER ecs,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int offset)
{
  int i, res;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  res = 0;
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(c, i, offset, &po);
    res += KheDemandAtTimeGroup(ecs->etm, ecs->resource_type, tg);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandAtClusterMonitor(KHE_EVENT_TIMETABLE_MONITOR etm,           */
/*    KHE_RESOURCE_TYPE rt, KHE_CLUSTER_BUSY_TIMES_MONITOR c)                */
/*                                                                           */
/*  Return the total demand for resources of type rt in the time groups      */
/*  of cluster busy times monitor m.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static int KheDemandAtClusterMonitor(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_RESOURCE_TYPE rt, KHE_CLUSTER_BUSY_TIMES_MONITOR m)
{
  int i, res;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  res = 0;
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
    res += KheDemandAtTimeGroup(etm, rt, tg);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cluster monitor suits combination elimination"                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterConstraintHasNonSingletonTimeGroup(                       */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc)                                */
/*                                                                           */
/*  Return true if cbtc has at least one time group with 2 or more elements. */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterConstraintHasNonSingletonTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(cbtc);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, 0, &po);
    if( KheTimeGroupTimeCount(tg) >= 2 )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterConstraintSuitsCombinationElimination(                    */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt, int *lim)   */
/*                                                                           */
/*  Return true if c is suited to combination elimination:  if its time      */
/*  groups are all positive and it has a non-trivial maximum limit, or its   */
/*  time groups are all negative and it has a non-trivial minimum limit.     */
/*  We also require the time groups to be disjoint and not all singletons,   */
/*  and that the constraint applies to every resource of type rt.            */
/*                                                                           */
/*  If c is suitable, also set *lim to a maximum limit on the number of      */
/*  its busy (not active) time groups.  This will be c's maximum limit if    */
/*  its time groups are all positive, or the number of time groups minus     */
/*  the minimum limit if its time groups are all negative.                   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static bool KheClusterConstraintSuitsCombinationElimination(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt, int *lim)
{
  int max_lim, min_lim, count, rt_count;
  rt_count = KheResourceTypeResourceCount(rt);
  if( KheClusterBusyTimesConstraintTimeGroupsDisjoint(c) &&
      KheClusterConstraintHasNonSingletonTimeGroup(c) &&
      KheClusterBusyTimesConstraintResourceOfTypeCount(c, rt) == rt_count )
  {
    max_lim = KheClusterBusyTimesConstraintMaximum(c);
    min_lim = KheClusterBusyTimesConstraintMinimum(c);
    count = KheClusterBusyTimesConstraintTimeGroupCount(c);
    if( KheClusterBusyTimesConstraintAllPositive(c) && max_lim < count )
      return *lim = max_lim, true;
    else if( KheClusterBusyTimesConstraintAllNegative(c) && min_lim > 0 )
      return *lim = count - min_lim, true;
    else
      return *lim = -1, false;
  }
  else
    return *lim = -1, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterLim(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                   */
/*                                                                           */
/*  Return the upper limit on the number of busy time groups.                */
/*                                                                           */
/*****************************************************************************/

static int KheClusterLim(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  if( KheClusterBusyTimesConstraintAllPositive(c) )
    return KheClusterBusyTimesConstraintMaximum(c);
  else if( KheClusterBusyTimesConstraintAllNegative(c) )
    return KheClusterBusyTimesConstraintTimeGroupCount(c) -
      KheClusterBusyTimesConstraintMinimum(c);
  else
  {
    HnAbort("KheClusterLim internal error");
    return 0; /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterConstraintSuitsCombinationElimination(                    */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if c is suited to combination elimination:  if its time      */
/*  groups are all positive and it has a non-trivial maximum limit, or its   */
/*  time groups are all negative and it has a non-trivial minimum limit.     */
/*  We also require the constraint weight to be non-zero, and the time       */
/*  groups to be disjoint and not all singletons.                            */
/*                                                                           */
/*  At this stage we do not require the constraint to cover all resources    */
/*  of the type currently being handled, because several constraints with    */
/*  the same time groups may do that collectively.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterConstraintSuitsCombinationElimination(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int max_lim, min_lim, count;
  if( KheConstraintCombinedWeight((KHE_CONSTRAINT) c) > 0 &&
      KheClusterBusyTimesConstraintTimeGroupsDisjoint(c) &&
      KheClusterConstraintHasNonSingletonTimeGroup(c) )
  {
    max_lim = KheClusterBusyTimesConstraintMaximum(c);
    min_lim = KheClusterBusyTimesConstraintMinimum(c);
    count = KheClusterBusyTimesConstraintTimeGroupCount(c);
    return (KheClusterBusyTimesConstraintAllPositive(c) && max_lim < count)
      || (KheClusterBusyTimesConstraintAllNegative(c) && min_lim > 0);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterMonitorSuitsCombinationElimination(                       */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_RESOURCE_TYPE rt, int *lim)      */
/*                                                                           */
/*  Return true if m suits combination elimination.                          */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static bool KheClusterMonitorSuitsCombinationElimination(
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, KHE_RESOURCE_TYPE rt, int *lim)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;
  c = KheClusterBusyTimesMonitorConstraint(m);
  return KheClusterConstraintSuitsCombinationElimination(c, rt, lim);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "eliminating combinations based on supply and demand"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EC_SOLVER KheElimCombSolverMake(KHE_COMB_SOLVER cs,                  */
/*    KHE_FRAME common_frame, KHE_EVENT_TIMETABLE_MONITOR etm,               */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Make an eliminate combinations solver with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_EC_SOLVER KheElimCombSolverMake(KHE_TASKER tr,
  KHE_FRAME common_frame, KHE_EVENT_TIMETABLE_MONITOR etm)
{
  HA_ARENA a;  KHE_EC_SOLVER res;  int count;
  a = KheTaskerArena(tr);
  HaMake(res, a);
  res->soln = KheTaskerSoln(tr);
  res->common_frame = common_frame;
  res->resource_type = KheTaskerResourceType(tr);
  res->resource_set = KheResourceSetMake(res->resource_type, a);
  res->etm = etm;
  HaArrayInit(res->cover_types, a);
  count = KheFrameTimeGroupCount(common_frame);
  HaArrayInit(res->constraints, a);
  HaArrayFill(res->cover_types, count, KHE_COMB_SOLVER_COVER_FREE);
  HaArrayInit(res->intersects, a);
  HaArrayInit(res->provisional_prevs, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int IntCmpIncreasing(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of integers into increasing     */
/*  order.                                                                   */
/*                                                                           */
/*****************************************************************************/

static int IntCmpIncreasing(const void *t1, const void *t2)
{
  int val1 = * (int *) t1;
  int val2 = * (int *) t2;
  return val1 - val2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterFirstAndLastIndex(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,    */
/*    int offset, KHE_FRAME frame, int *first_index, int *last_index)        */
/*                                                                           */
/*  Return the first and last indexes in frame where the time groups         */
/*  intersect with the time groups of c at offset.  If there is no           */
/*  intersection, return an empty interval (*first_index > *last_index).     */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterFirstAndLastIndex(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  int offset, KHE_FRAME frame, int *first_index, int *last_index)
{
  KHE_TIME first_time, last_time;
  if( KheClusterBusyTimesConstraintRange(c, offset, &first_time, &last_time) )
  {
    /* all good, return the indexes */
    *first_index = KheFrameTimeIndex(frame, first_time);
    *last_index = KheFrameTimeIndex(frame, last_time);
    return true;
  }
  else
  {
    /* constraint has no time groups */
    *first_index = 0;
    *last_index = -1;
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterMonitorFirstAndLastIndex(KHE_CLUSTER_BUSY_TIMES_MONITOR m,*/
/*    KHE_FRAME frame, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Return the first and last indexes in frame where the time groups         */
/*  intersect with the time groups of m.  If there is no intersection,       */
/*  return an empty interval (*first_index > *last_index).                   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete
static bool KheClusterMonitorFirstAndLastIndex(KHE_CLUSTER_BUSY_TIMES_MONITOR m,
  KHE_FRAME frame, int *first_index, int *last_index)
{
  KHE_TIME earliest_t, latest_t;
  KheClusterBusyTimesMonitorRange(m, &earliest_t, &latest_t);
  if( earliest_t == NULL )
    return *first_index = 0, *last_index = -1, false;

  ** all good, return the indexes **
  return *first_index = KheFrameTimeIndex(frame, earliest_t),
    *last_index = KheFrameTimeIndex(frame, latest_t), true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheElimCombSolveForConstraintSet(KHE_EC_SOLVER ecs,                 */
/*    int start, int stop, int offset)                                       */
/*                                                                           */
/*  Eliminate combinations based on supply and demand within the times of    */
/*  ecs->constraints[start .. stop-1], a non-empty group of suitable         */
/*  cluster busy times constraints, at the given offset.                     */
/*                                                                           */
/*****************************************************************************/

static void KheElimCombSolveForConstraintSet(KHE_EC_SOLVER ecs,
  int start, int stop, int offset)
{
  KHE_TIME_GROUP c_tg, f_tg, prev_f_tg;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;
  int i, j, val, lim, count, sum, fi, li, supply, demand;  KHE_POLARITY po;
  bool f_tg_overlaps, prev_f_tg_overlaps;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheElimCombSolveForConstraintSet(ecs, %d-%d, offs %d)\n",
      start, stop - 1, offset);
    for( i = start;  i < stop;  i++ )
    {
      c = HaArray(ecs->constraints, i);
      KheClusterBusyTimesConstraintDebug(c, 2, 2, stderr);
    }
  }
  c = HaArray(ecs->constraints, start);
  if( KheClusterFirstAndLastIndex(c, offset, ecs->common_frame, &fi, &li) )
  {
    /* set ecs->intersects and ecs->provisional_prevs */
    HaArrayClear(ecs->intersects);
    HaArrayClear(ecs->provisional_prevs);
    for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(c);  i++ )
    {
      c_tg = KheClusterBusyTimesConstraintTimeGroup(c, i, offset, &po);
      if( DEBUG6 )
	fprintf(stderr, "  KheElimCombSolveForConstraintSet at %d\n", i);
      count = 0;
      prev_f_tg = NULL;
      prev_f_tg_overlaps = false;
      for( j = fi;  j <= li;  j++ )
      {
	f_tg = KheFrameTimeGroup(ecs->common_frame, j);
	f_tg_overlaps = !KheTimeGroupDisjoint(c_tg, f_tg);
	if( f_tg_overlaps )
	  count++;
	if( f_tg_overlaps && prev_f_tg_overlaps )
	{
	  /* if we later find that D(T) >= S(T), then if prev_f_tg is */
	  /* busy, then f_tg must be busy as well, because they both  */
	  /* overlap the same time group of the constraint */
	  if( DEBUG6 )
	  {
	    fprintf(stderr, "  provisionally linked time groups ");
	    KheTimeGroupDebug(prev_f_tg, 1, -1, stderr);
	    fprintf(stderr, " and ");
	    KheTimeGroupDebug(f_tg, 1, -1, stderr);
	    fprintf(stderr, "\n");
	  }

	  /* record the linkage provisionally */
	  HaArrayAddLast(ecs->provisional_prevs, j);
	}
	prev_f_tg = f_tg;
	prev_f_tg_overlaps = f_tg_overlaps;
      }
      HaArrayAddLast(ecs->intersects, count);
    }
    HaArraySort(ecs->intersects, &IntCmpIncreasing);

    /* get the supply:  the largest lim intersect counts, times resources */
    if( DEBUG6 )
      fprintf(stderr, "  KheElimCombSolveForConstraintSet at %d\n", i);
    count = HaArrayCount(ecs->intersects);  /* also number of time groups */
    supply = 0;
    for( i = start;  i < stop;  i++ )
    {
      c = HaArray(ecs->constraints, i);
      lim = KheClusterLim(c);
      HnAssert(lim < count, "KheElimCombSolveForConstraintSet internal error");
      sum = 0;
      for( j = 1;  j <= lim;  j++ )
	sum += HaArray(ecs->intersects, count - j);
      supply += sum *
	KheClusterBusyTimesConstraintResourceOfTypeCount(c, ecs->resource_type);
      /* NB it is safe to use KheClusterBusyTimesConstraintResourceOfTypeCount
	 here, because c is known to contain no duplicate resources, thanks to
         KheConstaintSetCoversAllResources */
    }

    /* get the demand */
    /* c here will be HaArray(ecs->constraints, stop - 1), which is safe */
    demand = KheDemandAtClusterConstraint(ecs, c, offset);

    /* if demand >= supply, provisional linkages become definite */
    if( demand >= supply )
      HaArrayForEach(ecs->provisional_prevs, val, i /* index comes last */)
      {
	HaArrayPut(ecs->cover_types, val, KHE_COMB_SOLVER_COVER_PREV);
	if( DEBUG6 )
	{
	  prev_f_tg = KheFrameTimeGroup(ecs->common_frame, val - 1);
	  f_tg = KheFrameTimeGroup(ecs->common_frame, val);
	  fprintf(stderr, "  linking %s and %s\n", KheTimeGroupId(prev_f_tg),
            KheTimeGroupId(f_tg));
	}
      }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheElimCombSolveForConstraintSet returning,"
      " demand %d, supply %d\n", demand, supply);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElimCombSolveForMonitor(KHE_EC_SOLVER ecs,                       */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int lim)                             */
/*                                                                           */
/*  Eliminate combinations based on supply and demand within the times of    */
/*  m, a suitable cluster busy times monitor with limit lim.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheElimCombSolveForMonitor(KHE_EC_SOLVER ecs,
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int lim)
{
  KHE_TIME_GROUP m_tg, f_tg, prev_f_tg;
  int i, j, count, sum, fi, li, supply, demand;  KHE_POLARITY po;
  bool f_tg_overlaps, prev_f_tg_overlaps;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheElimCombSolveForMonitor(ecs, m, %d), c:\n",
      lim);
    KheClusterBusyTimesMonitorDebug(m, 3, 2, stderr);
  }
  if( KheClusterMonitorFirstAndLastIndex(m, ecs->common_frame, &fi, &li) )
  {
    HaArrayClear(ecs->intersects);
    HaArrayClear(ecs->provisional_prevs);
    for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(m);  i++ )
    {
      m_tg = KheClusterBusyTimesMonitorTimeGroup(m, i, &po);
      count = 0;
      prev_f_tg = NULL;
      prev_f_tg_overlaps = false;
      for( j = fi;  j <= li;  j++ )
      {
	f_tg = KheFrameTimeGroup(ecs->common_frame, j);
	f_tg_overlaps = !KheTimeGroupDisjoint(m_tg, f_tg);
	if( f_tg_overlaps )
	  count++;
	if( f_tg_overlaps && prev_f_tg_overlaps )
	{
	  ** if we later find that D(T) >= S(T), then if prev_f_tg is **
	  ** busy, then f_tg must be busy as well, because they both  **
	  ** overlap the same time group of the monitor **
	  if( DEBUG6 )
	  {
	    fprintf(stderr, "  provisionally linked time groups ");
	    KheTimeGroupDebug(prev_f_tg, 1, -1, stderr);
	    fprintf(stderr, " and ");
	    KheTimeGroupDebug(f_tg, 1, -1, stderr);
	    fprintf(stderr, "\n");
	  }

	  ** record the linkage provisionally **
	  HaArrayAddLast(ecs->provisional_prevs, j);
	}
	prev_f_tg = f_tg;
	prev_f_tg_overlaps = f_tg_overlaps;
      }
      HaArrayAddLast(ecs->intersects, count);
    }

    ** get the supply:  the largest lim intersect counts, times resources **
    HaArraySort(ecs->intersects, &IntCmpIncreasing);
    count = HaArrayCount(ecs->intersects);
    sum = 0;
    for( j = 1;  j <= lim;  j++ )
      sum += HaArray(ecs->intersects, count - j);
    supply = sum * KheResourceTypeResourceCount(ecs->resource_type);

    ** get the demand **
    demand = KheDemandAtClusterMonitor(ecs->etm, ecs->resource_type, m);

    ** if demand >= supply, provisional linkages become definite **
    if( demand >= supply )
      HaArrayForEach(ecs->provisional_prevs, j, i)
	HaArrayPut(ecs->cover_types, j, KHE_COMB_SOLVER_COVER_PREV);
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheElimCombSolveForMonitor returning,"
      " demand %d, supply %d\n", demand, supply);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterTypedCmp(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c1,             */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c2)                                  */
/*                                                                           */
/*  Comparison function for sorting an array of cluster busy times           */
/*  constraints so that constraints with equal time groups are adjacent.     */
/*                                                                           */
/*****************************************************************************/

static int KheClusterTypedCmp(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c1,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int i, cmp, count1, count2;

  /* applies-to time groups must be equal */
  tg1 = KheClusterBusyTimesConstraintAppliesToTimeGroup(c1);
  tg2 = KheClusterBusyTimesConstraintAppliesToTimeGroup(c2);
  if( tg1 == NULL )
  {
    if( tg2 != NULL ) return -1;
  }
  else
  {
    if( tg2 == NULL ) return 1;
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 ) return cmp;
  }

  /* time groups must be equal, with equal polarity */
  count1 = KheClusterBusyTimesConstraintTimeGroupCount(c1);
  count2 = KheClusterBusyTimesConstraintTimeGroupCount(c2);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheClusterBusyTimesConstraintTimeGroup(c1, i, 0, &po1);
    tg2 = KheClusterBusyTimesConstraintTimeGroup(c2, i, 0, &po2);
    if( po1 != po2 )
      return (int) po1 - (int) po2;
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 ) return cmp;
  }

  /* everything that matters here is equal */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped version of KheClusterTypedCmp.                                   */
/*                                                                           */
/*****************************************************************************/

static int KheClusterCmp(const void *t1, const void *t2)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c1 =
    * (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *) t1;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c2 =
    * (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *) t2;
  return KheClusterTypedCmp(c1, c2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstaintSetCoversAllResources(KHE_EC_SOLVER ecs, int start,     */
/*    int stop)                                                              */
/*                                                                           */
/*  Return true if ecs->constraints[start .. stop-1] cover all of ecs->rt,   */
/*  with no duplicates.                                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheConstaintSetCoversAllResources(KHE_EC_SOLVER ecs, int start,
  int stop)
{
  int i, j, rt_count, count;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;
  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  rt = ecs->resource_type;
  rt_count = KheResourceTypeResourceCount(rt);
  KheResourceSetClear(ecs->resource_set);
  count = 0;
  for( i = start;  i < stop;  i++ )
  {
    c = HaArray(ecs->constraints, i);
    for( j = 0;  j < KheClusterBusyTimesConstraintResourceGroupCount(c);  j++ )
    {
      rg = KheClusterBusyTimesConstraintResourceGroup(c, j);
      count += KheResourceGroupResourceCount(rg);
      KheResourceSetAddResourceGroup(ecs->resource_set, rg);
    }
    for( j = 0;  j < KheClusterBusyTimesConstraintResourceCount(c);  j++ )
    {
      r = KheClusterBusyTimesConstraintResource(c, j);
      count += 1;
      KheResourceSetAddResource(ecs->resource_set, r);
    }
  }
  return count == rt_count &&
    KheResourceSetResourceCount(ecs->resource_set) == rt_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElimCombSolve(KHE_EC_SOLVER ecs)                                 */
/*                                                                           */
/*  Eliminate combinations, expressing the eliminations by changes to        */
/*  ecs->cover_types.                                                        */
/*                                                                           */
/*****************************************************************************/

static void KheElimCombSolve(KHE_EC_SOLVER ecs)
{
  int i, j, k, offset, offset_count;  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, cbtc2;
  rt = ecs->resource_type;
  if( DEBUG6 )
    fprintf(stderr, "[ KheElimCombSolve(%s)\n", KheResourceTypeId(rt));

  /* find and sort the relevant constraints */
  ins = KheSolnInstance(ecs->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( KheClusterConstraintSuitsCombinationElimination(cbtc) )
	HaArrayAddLast(ecs->constraints, cbtc);
    }
  }
  HaArraySort(ecs->constraints, &KheClusterCmp);

  /* find groups of equal constraints */
  for( i = 0;  i < HaArrayCount(ecs->constraints);  i += j )
  {
    cbtc = HaArray(ecs->constraints, i);
    for( j = i + 1;  j < HaArrayCount(ecs->constraints);  j++ )
    {
      cbtc2 = HaArray(ecs->constraints, j);
      if( KheClusterTypedCmp(cbtc, cbtc2) != 0 )
	break;
    }

    /* at this point, ecs->constraints[i .. j-1] is one group */
    if( DEBUG6 )
    {
      fprintf(stderr, "  group of constraints (%severy %s):\n",
	KheConstaintSetCoversAllResources(ecs, i, j) ? "" : "not ",
	KheResourceTypeId(ecs->resource_type));
      for( k = i;  k < j;  k++ )
      {
	cbtc = HaArray(ecs->constraints, k);
	KheClusterBusyTimesConstraintDebug(cbtc, 2, 4, stderr);
      }
    }

    /* do the grouping */
    if( KheConstaintSetCoversAllResources(ecs, i, j) )
    {
      cbtc = HaArray(ecs->constraints, i);
      offset_count = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc);
      for( k = 0;  k < offset_count;  k++ )
      {
	offset = KheClusterBusyTimesConstraintAppliesToOffset(cbtc, k);
        KheElimCombSolveForConstraintSet(ecs, i, j, offset);
      }
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheElimCombSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "combinatorial grouping"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDoCombGroupingForInterval(KHE_COMB_SOLVER cs,                     */
/*    KHE_FRAME common_frame, ARRAY_KHE_COMB_SOLVER_COVER_TYPE *cover_types, */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Do combinatorial grouping for interval first_index ... last_index of     */
/*  common_frame, and return the number of groups made.                      */
/*                                                                           */
/*****************************************************************************/

static int KheDoCombGroupingForInterval(KHE_COMB_SOLVER cs,
  KHE_FRAME common_frame, ARRAY_KHE_COMB_SOLVER_COVER_TYPE *cover_types,
  int first_index, int last_index)
{
  int i, j, k, total_groups, groups;  KHE_TASKER tr;  KHE_TASKER_CLASS tc;
  KHE_TIME_GROUP first_tg, last_tg, tg;  KHE_COMB_SOLVER_COVER_TYPE cover_type;
  KHE_TASKER_TIME tt;  KHE_TIME t;
  if( DEBUG2 )
  {
    fprintf(stderr, "  [ KheDoCombGroupingForInterval(gs, ");
    tg = KheFrameTimeGroup(common_frame, first_index);
    KheTimeGroupDebug(tg, 1, -1, stderr);
    fprintf(stderr, " .. ");
    tg = KheFrameTimeGroup(common_frame, last_index);
    KheTimeGroupDebug(tg, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* try one solve for each class in the first time group, but then */
  /* start all over again and have another go if any were successful */
  total_groups = 0;
  tr = KheCombSolverTasker(cs);
  first_tg = KheFrameTimeGroup(common_frame, first_index);
  last_tg = KheFrameTimeGroup(common_frame, last_index);

  RESTART:
  for( i = 0;  i < KheTimeGroupTimeCount(first_tg);  i++ )
  {
    t = KheTimeGroupTime(first_tg, i);
    tt = KheTaskerTime(tr, KheTimeIndex(t));
    for( k = 0;  k < KheTaskerTimeClassCount(tt);  k++ )
    {
      tc = KheTaskerTimeClass(tt, k);
      if( DEBUG4 )
      {
	fprintf(stderr, "    [ combinations starting with %s:", KheTimeId(t));
	KheTaskerClassDebug(tc, 1, 1, stderr);
      }
      KheCombSolverClearRequirements(cs);
      KheCombSolverAddClassRequirement(cs, tc, KHE_COMB_SOLVER_COVER_YES);
      for( j = first_index;  j <= last_index;  j++ )
      {
	tg = KheFrameTimeGroup(common_frame, j);
	if( j == first_index )
	  cover_type = KHE_COMB_SOLVER_COVER_FREE;
	else
	  cover_type = HaArray(*cover_types, j);
	KheCombSolverAddTimeGroupRequirement(cs, tg, cover_type);
      }
      groups = KheCombSolverSolve(cs, INT_MAX, KHE_COMB_SOLVER_COST_SOLE_ZERO,
	"combinatorial grouping");
      if( DEBUG4 )
	fprintf(stderr, "    ] %d groups\n", groups);
      if( groups > 0 )
      {
	total_groups += groups;
	goto RESTART;
      }
    }
  }
  if( last_tg != first_tg )
  {
    for( i = 0;  i < KheTimeGroupTimeCount(last_tg);  i++ )
    {
      t = KheTimeGroupTime(last_tg, i);
      tt = KheTaskerTime(tr, KheTimeIndex(t));
      for( k = 0;  k < KheTaskerTimeClassCount(tt);  k++ )
      {
	tc = KheTaskerTimeClass(tt, k);
	if( DEBUG4 )
	{
	  fprintf(stderr, "    [ combinations ending with %s:", KheTimeId(t));
	  KheTaskerClassDebug(tc, 1, 1, stderr);
	}
	KheCombSolverClearRequirements(cs);
	KheCombSolverAddClassRequirement(cs, tc, KHE_COMB_SOLVER_COVER_YES);
	for( j = first_index;  j <= last_index;  j++ )
	{
	  tg = KheFrameTimeGroup(common_frame, j);
	  if( j == first_index )
	    cover_type = KHE_COMB_SOLVER_COVER_FREE;
	  else
	    cover_type = HaArray(*cover_types, j);
	  KheCombSolverAddTimeGroupRequirement(cs, tg, cover_type);
	}
	groups = KheCombSolverSolve(cs, INT_MAX, KHE_COMB_SOLVER_COST_SOLE_ZERO,
	  "combinatorial grouping");
	if( DEBUG4 )
	  fprintf(stderr, "    ] %d groups\n", groups);
	if( groups > 0 )
	{
	  total_groups += groups;
	  goto RESTART;
	}
      }
    }
  }

  if( DEBUG2 )
    fprintf(stderr, "  ] KheDoCombinatorialGroupingForInterval returning %d\n",
      total_groups);
  return total_groups;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouping(KHE_COMB_SOLVER cs, KHE_OPTIONS options)             */
/*                                                                           */
/*  Do some combinatorial grouping.                                          */
/*                                                                           */
/*****************************************************************************/

int KheCombGrouping(KHE_COMB_SOLVER cs, KHE_OPTIONS options)
{
  int max_days, len, first_index, days, total_groups;  KHE_TASKER tr;
  KHE_FRAME common_frame;  KHE_EC_SOLVER ecs;  KHE_EVENT_TIMETABLE_MONITOR etm;

  /* get options */
  tr = KheCombSolverTasker(cs);
  max_days = KheOptionsGetInt(options, "rs_group_by_rc_max_days", 3);
  common_frame = KheOptionsFrame(options, "gs_common_frame", KheTaskerSoln(tr));
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);

  /* eliminate combinations */
  if( DEBUG10 )
    fprintf(stderr, "  KheCombGrouping before KheElimCombSolve\n");
  ecs = KheElimCombSolverMake(tr, common_frame, etm);
  KheElimCombSolve(ecs);

  /* try each interval */
  total_groups = 0;
  days = KheFrameTimeGroupCount(common_frame);
  for( len = max_days;  len >= 2;  len-- )
  {
    if( DEBUG10 )
      fprintf(stderr, "  KheCombGrouping starting len %d\n", len);
    for( first_index = 0;  first_index <= days - len;  first_index++ )
      total_groups += KheDoCombGroupingForInterval(cs, common_frame,
	&ecs->cover_types, first_index, first_index + len - 1);
  }
  return total_groups;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "profile grouping"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PROFILE_SOLVER KheProfileSolverMake(KHE_COMB_SOLVER cs)              */
/*                                                                           */
/*  Make a new profile solver object.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_PROFILE_SOLVER KheProfileSolverMake(KHE_COMB_SOLVER cs)
{
  KHE_TASKER tr;  HA_ARENA a;  KHE_PROFILE_SOLVER res;
  tr = KheCombSolverTasker(cs);
  a = KheTaskerArena(tr);
  HaMake(res, a);
  res->comb_solver = cs;
  res->soln = KheTaskerSoln(tr);
  res->resource_type = KheTaskerResourceType(tr);
  HaArrayInit(res->monitors, a);
  res->min_limit = res->max_limit = -1;
  res->history_before = res->history_after = -1;
  res->groups_count = 0;
  HaArrayInit(res->pos_groups, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)    */
/*                                                                           */
/*  Return true if the time groups of m are all singletons.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(m, i, &po);
    if( KheTimeGroupTimeCount(tg) != 1 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorSuitsProfileGrouping(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,*/
/*    KHE_RESOURCE_TYPE rt, bool non_strict)                                 */
/*                                                                           */
/*  Return true if m suits profile grouping:  if its constraint applies to   */
/*  all resources of type rt, its time groups are all positive, and its      */
/*  time groups are all singletons (strict) or its limits are equal          */
/*  (non-strict).  Actually we can't enforce the second part here because    */
/*  we have not yet merged pairs of constraints.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorSuitsProfileGrouping(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m,
  KHE_RESOURCE_TYPE rt, bool non_strict)
{
  int rt_count;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;

  /* there must be at least two time groups */
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(m) < 2 )
    return false;

  /* it must monitor all resources of type rt */
  rt_count = KheResourceTypeResourceCount(rt);
  c = KheLimitActiveIntervalsMonitorConstraint(m);
  if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(c, rt) < rt_count )
    return false;

  /* its time groups must be all positive */
  if( !KheLimitActiveIntervalsConstraintAllPositive(c) )
    return false;

  if( non_strict )
  {
    /* its limits must be equal (but we can't enforce that here) */
    return true;
    /* return !KheTimeGroupsAllSingletons(m); */
  }
  else
  {
    /* its time groups must be all singletons */
    return KheTimeGroupsAllSingletons(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsMonitorsCanGroupTogether(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)                              */
/*                                                                           */
/*  Return true if laim1 and laim2 can be handled together, because they     */
/*  have equal time groups, one has a maximum limit only, and the other      */
/*  has a minimum limit only.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitActiveIntervalsMonitorsCanGroupTogether(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)
{
  int i;  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  bool has_min1, has_min2, has_max1, has_max2;

  /* time groups must be equal */
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(laim1) !=
      KheLimitActiveIntervalsMonitorTimeGroupCount(laim2) )
    return false;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);  i++ )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, i, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, i, &po2);
    if( !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }

  /* one must have a min limit only, the other a max limit only */
  has_min1 = KheLimitActiveIntervalsMonitorMinimum(laim1) > 0;
  has_min2 = KheLimitActiveIntervalsMonitorMinimum(laim2) > 0;
  has_max1 = KheLimitActiveIntervalsMonitorMaximum(laim1) <
    KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
  has_max2 = KheLimitActiveIntervalsMonitorMaximum(laim2) <
    KheLimitActiveIntervalsMonitorTimeGroupCount(laim2);
  if( (has_min1 && !has_max1) && (!has_min2 && has_max2) )
    return true;
  if( (!has_min1 && has_max1) && (has_min2 && !has_max2) )
    return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool IndexIsLegal(KHE_TASKER tr, int index)                              */
/*                                                                           */
/*  Return true if index is a legal index of a profile time group.           */
/*                                                                           */
/*****************************************************************************/

static bool IndexIsLegal(KHE_TASKER tr, int index)
{
  return index >= 0 && index < KheTaskerProfileTimeGroupCount(tr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetYesTimeGroups(KHE_PROFILE_SOLVER ps,           */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Set a sequence of Yes time groups for profile grouping.                  */
/*                                                                           */
/*****************************************************************************/

static void KheProfileGroupingSetYesTimeGroups(KHE_PROFILE_SOLVER ps,
  int first_index, int last_index)
{
  KHE_TASKER tr;  KHE_TIME_GROUP tg;  int index;
  tr = KheCombSolverTasker(ps->comb_solver);
  for( index = first_index;  index <= last_index;  index++ )
    if( IndexIsLegal(tr, index) )
    {
      tg = KheProfileTimeGroupTimeGroup(KheTaskerProfileTimeGroup(tr, index));
      KheCombSolverAddTimeGroupRequirement(ps->comb_solver, tg,
	KHE_COMB_SOLVER_COVER_YES);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingSetNoTimeGroups(KHE_PROFILE_SOLVER ps,            */
/*    int index1, int index2)                                                */
/*                                                                           */
/*  Set the two No time groups for profile grouping.                         */
/*                                                                           */
/*****************************************************************************/

static void KheProfileGroupingSetNoTimeGroups(KHE_PROFILE_SOLVER ps,
  int index1, int index2)
{
  KHE_TASKER tr;  KHE_TIME_GROUP tg;
  tr = KheCombSolverTasker(ps->comb_solver);
  if( IndexIsLegal(tr, index1) )
  {
    tg = KheProfileTimeGroupTimeGroup(KheTaskerProfileTimeGroup(tr, index1));
    KheCombSolverAddTimeGroupRequirement(ps->comb_solver, tg,
      KHE_COMB_SOLVER_COVER_NO);
  }
  if( IndexIsLegal(tr, index2) )
  {
    tg = KheProfileTimeGroupTimeGroup(KheTaskerProfileTimeGroup(tr, index2));
    KheCombSolverAddTimeGroupRequirement(ps->comb_solver, tg,
      KHE_COMB_SOLVER_COVER_NO);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileDebug(KHE_PROFILE_SOLVER ps, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of current profile.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheProfileDebug(KHE_PROFILE_SOLVER ps, int verbosity,
  int indent, FILE *fp)
{
  KHE_PROFILE_TIME_GROUP ptg;  int i, val;  KHE_TASKER tr;
  tr = KheCombSolverTasker(ps->comb_solver);
  fprintf(fp, "%*s%s[", indent, "", ps->history_before ? "inf" : "0");
  for( i = 0;  i < KheTaskerProfileTimeGroupCount(tr);  i++ )
  {
    if( i > 0 )
      fprintf(fp, ":");
    ptg = KheTaskerProfileTimeGroup(tr, i);
    fprintf(fp, "%d", KheProfileTimeGroupCover(ptg));

  }
  fprintf(fp, "]%s\n", ps->history_after ? "inf" : "0");
  fprintf(fp, "%*s%s<", indent, "", ps->history_before ? "inf" : "0");
  HaArrayForEach(ps->pos_groups, val, i)
  {
    if( i > 0 )
      fprintf(fp, ":");
    fprintf(fp, "%d", val);

  }
  fprintf(fp, ">%s\n", ps->history_after ? "inf" : "0");
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheProfileTimeGroupId(KHE_PROFILE_SOLVER ps, int index)            */
/*                                                                           */
/*  Return the name of the profile time group at index, or "-" if none.      */
/*                                                                           */
/*****************************************************************************/

static char *KheProfileTimeGroupId(KHE_PROFILE_SOLVER ps, int index)
{
  KHE_TASKER tr;  KHE_PROFILE_TIME_GROUP ptg;  KHE_TIME_GROUP tg;
  tr = KheCombSolverTasker(ps->comb_solver);
  if( IndexIsLegal(tr, index) )
  {
    ptg = KheTaskerProfileTimeGroup(tr, index);
    tg = KheProfileTimeGroupTimeGroup(ptg);
    return KheTimeGroupId(tg);
  }
  else
    return "-";
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfile(KHE_PROFILE_SOLVER ps, int first, int last,              */
/*    int no1, int no2, KHE_PROFILE_TIME_GROUP ptg,                          */
/*    KHE_PROFILE_TIME_GROUP adj_ptg, int max_num, char *debug_str)          */
/*                                                                           */
/*  Find up to max_num groups for indexes first .. last, taking account      */
/*  of task domains.  Exclude indexes no1 and no2.  Return true if any       */
/*  groups were made.                                                        */
/*                                                                           */
/*  Here ptg is the profile time group at index first (or last) that         */
/*  we use to sort out the domains.                                          */
/*                                                                           */
/*****************************************************************************/

static int min(int a, int b) { return a < b ? a : b; }

static bool KheProfile(KHE_PROFILE_SOLVER ps, int first, int last,
  int no1, int no2, KHE_PROFILE_TIME_GROUP ptg,
  KHE_PROFILE_TIME_GROUP adj_ptg, int max_num, char *debug_str)
{
  int i, cover, adj_cover, groups, singles, phase, want;  bool res;
  KHE_RESOURCE_GROUP domain;

  /* set up the time groups and other requirements, and count singles */
  KheCombSolverClearRequirements(ps->comb_solver);
  KheProfileGroupingSetYesTimeGroups(ps, first, last);
  KheProfileGroupingSetNoTimeGroups(ps, no1, no2);
  KheCombSolverAddProfileMaxLenRequirement(ps->comb_solver);
  singles = KheCombSolverSingleTasks(ps->comb_solver);
  KheCombSolverAddNoSinglesRequirement(ps->comb_solver);
  if( DEBUG5 )
  {
    KHE_TIME_GROUP ptg_tg, aptg_tg;
    ptg_tg = KheProfileTimeGroupTimeGroup(ptg);
    aptg_tg = (adj_ptg == NULL ? NULL : KheProfileTimeGroupTimeGroup(adj_ptg));
    fprintf(stderr, "  [ KheProfile(ps, %s .. %s, not %s, not %s, "
      "ptg %s, adj %s, max_num %d - %d)\n",
      KheProfileTimeGroupId(ps, first), KheProfileTimeGroupId(ps, last),
      KheProfileTimeGroupId(ps, no1), KheProfileTimeGroupId(ps, no2),
      KheTimeGroupId(ptg_tg), aptg_tg == NULL ? "-" : KheTimeGroupId(aptg_tg),
      max_num, singles);
  }

  max_num -= singles;
  res = false;
  for( phase = 0;  phase < 1;  phase++ )  /* yes, this currently does nothing */
  for( i = 0;  i < KheProfileTimeGroupDomainCount(ptg) && max_num > 0;  i++ )
  {
    /* find one domain, its cover, and its cover in adj_ptg */
    domain = KheProfileTimeGroupDomain(ptg, i, &cover);
    if( adj_ptg == NULL ||
	!KheProfileTimeGroupContainsDomain(adj_ptg, domain, &adj_cover) )
      adj_cover = 0;
    want = (phase == 0 ? cover - adj_cover : cover - adj_cover + 1);
    if( DEBUG5 )
    {
      fprintf(stderr, "    domain ");
      KheResourceGroupDebug(domain, 1, -1, stderr);
      fprintf(stderr, ", cover %d, adj_cover %d, phase %d, want %d\n",
	cover, adj_cover, phase, want);
    }

    /* do some grouping if the domain has less cover in adj_ptg */
    if( want > 0 )
    {
      /* do the grouping and adjust max_num and groups */
      KheCombSolverAddProfileGroupRequirement(ps->comb_solver, ptg, domain);
      groups = KheCombSolverSolve(ps->comb_solver,
	min(want, max_num), KHE_COMB_SOLVER_COST_ZERO, debug_str);
      KheCombSolverDeleteProfileGroupRequirement(ps->comb_solver, ptg);
      if( groups > 0 )
      {
	HaArrayPut(ps->pos_groups, first,
	  HaArray(ps->pos_groups, first) + groups);
	ps->groups_count += groups;
	max_num -= groups;
	res = true;
	if( DEBUG3 )
	  fprintf(stderr, "  %s made %d groups for %s", debug_str, groups,
	    KheResourceGroupId(domain));
	if( DEBUG5 )
	  KheProfileDebug(ps, 1, 2, stderr);
	i = -1;  /* start again, domain list may be out of date */
      }
    }
  }
  if( DEBUG5 )
    fprintf(stderr, "  ] KheProfile returning %s (groups_count %d)\n",
      bool_show(res), ps->groups_count);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps)                 */
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are unequal, and   */
/*  return true if any groups were made.  Make as many groups as possible.   */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileTryUnequalGrouping(KHE_PROFILE_SOLVER ps)
{
  int tg_count, i, prev_prof, curr_prof, next_prof;  bool res, progressing;
  KHE_TASKER tr;  KHE_PROFILE_TIME_GROUP curr_ptg, prev_ptg, next_ptg;

  /* boilerplate */
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheProfileTryUnequalGrouping\n");
    KheProfileDebug(ps, 1, 2, stderr);
  }
  tr = KheCombSolverTasker(ps->comb_solver);
  tg_count = KheTaskerProfileTimeGroupCount(tr);
  res = false;

  do
  {
    progressing = false;

    /* forward pass to handle places where sequences start */
    prev_ptg = NULL;
    prev_prof = ps->history_before ? INT_MAX : 0;
    for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
    {
      curr_ptg = KheTaskerProfileTimeGroup(tr, i);
      curr_prof = KheProfileTimeGroupCover(curr_ptg);
      if( curr_prof > prev_prof )
      {
	/* curr_prof - prev_prof sequences must start at i */
	if( DEBUG5 )
	  fprintf(stderr, "  start at %d (curr %d > prev %d)\n", i,
	    curr_prof, prev_prof);
	if( KheProfile(ps, i, i + ps->min_limit - 1, i - 1,
	      i + ps->max_limit, curr_ptg, prev_ptg,
	      curr_prof - prev_prof, "forward profile grouping") )
	  res = progressing = true;
      }

      /* reset prev_prof for next iteration */
      prev_ptg = curr_ptg;
      prev_prof = KheProfileTimeGroupCover(prev_ptg);
    }

    /* backward pass to handle places where sequences end */
    next_ptg = NULL;
    next_prof = ps->history_after ? INT_MAX : 0;
    for( i = tg_count - 1;  i >= ps->min_limit - 1;  i-- )
    {
      curr_ptg = KheTaskerProfileTimeGroup(tr, i);
      curr_prof = KheProfileTimeGroupCover(curr_ptg);

      if( curr_prof > next_prof )
      {
	/* curr_prof - next_prof sequences must end at i */
	if( DEBUG5 )
	  fprintf(stderr, "  end at %d (curr %d > next %d)\n", i,
	    curr_prof, next_prof);
	if( KheProfile(ps, i - ps->min_limit + 1, i, i - ps->max_limit,
	      i + 1, curr_ptg, next_ptg, curr_prof - next_prof,
	      "backward profile grouping") )
	  res = progressing = true;
      }

      /* reset next_prof for next iteration */
      next_ptg = curr_ptg;
      next_prof = KheProfileTimeGroupCover(next_ptg);
    }
  } while( progressing );

  /* all done */
  if( DEBUG5 )
    fprintf(stderr, "] KheProfileTryUnequalGrouping returning %s\n",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps)                   */
/*                                                                           */
/*  Try grouping at places where adjacent profile entries are equal, and     */
/*  return true if any groups were made.  Make at most one group.            */
/*                                                                           */
/*****************************************************************************/

static bool KheProfileTryEqualGrouping(KHE_PROFILE_SOLVER ps)
{
  int val, i, min_groups, tg_count, curr_prof, count, /* extra, */ index;
  KHE_TASKER tr;  KHE_PROFILE_TIME_GROUP curr_ptg;

  /* find min_groups, the minimum number of groupings */
  tr = KheCombSolverTasker(ps->comb_solver);
  min_groups = INT_MAX;
  tg_count = KheTaskerProfileTimeGroupCount(tr);
  for( i = 0;  i <= tg_count - ps->min_limit;  i++ )
  {
    curr_ptg = KheTaskerProfileTimeGroup(tr, i);
    curr_prof = KheProfileTimeGroupCover(curr_ptg);
    val = HaArray(ps->pos_groups, i);
    if( curr_prof > 0 && val < min_groups )
      min_groups = val;
  }

  /* for each pos with a minimum number of groups, try to group */
  count = tg_count - ps->min_limit + 1;
  /* extra = KheSolnDiversifier(ps->soln) * 19; */
  for( i = 0;  i < count;  i++ )
  {
    /* index = (i + extra) % count; */
    index = i;
    curr_ptg = KheTaskerProfileTimeGroup(tr, index);
    curr_prof = KheProfileTimeGroupCover(curr_ptg);
    val = HaArray(ps->pos_groups, index);
    if( curr_prof > 0 && val == min_groups &&
	KheProfile(ps, index, index + ps->min_limit - 1,
	  index - 1, index + ps->max_limit, curr_ptg, NULL, 1,
	  "non-strict profile grouping") )
      return true;
  }

  /* for each pos with just above a min number of groups, try to group */
  /* extra = KheSolnDiversifier(ps->soln) * 7; */
  for( i = 0;  i < count;  i++ )
  {
    /* index = (i + extra) % count; */
    index = i;
    curr_ptg = KheTaskerProfileTimeGroup(tr, index);
    curr_prof = KheProfileTimeGroupCover(curr_ptg);
    val = HaArray(ps->pos_groups, index);
    if( curr_prof > 0 && val == min_groups + 1 &&
	KheProfile(ps, index, index + ps->min_limit - 1,
	  index - 1, index + ps->max_limit, curr_ptg, NULL, 1,
	  "non-strict profile grouping") )
      return true;
  }

  /* no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,                              */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2, bool non_strict)             */
/*                                                                           */
/*  Carry out profile grouping based on laim1 and optionally laim2.          */
/*                                                                           */
/*****************************************************************************/

static void KheProfileGroupingForMonitors(KHE_PROFILE_SOLVER ps,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2, bool non_strict)
{
  int tg_count, i;  KHE_POLARITY po;  KHE_TASKER tr;  KHE_TIME_GROUP tg;

  if( DEBUG8 )
  {
    fprintf(stderr, "[ KheProfileGroupingForMonitors(ps, laim1, %s%s)\n",
      laim2 != NULL ? "laim2" : "-", non_strict ? ", non_strict" : "");
    KheLimitActiveIntervalsMonitorDebug(laim1, 1, 2, stderr);
    if( laim2 != NULL )
      KheLimitActiveIntervalsMonitorDebug(laim2, 1, 2, stderr);
  }

  /* get min_limit, max_limit, history_before, and history_after */
  ps->min_limit = KheLimitActiveIntervalsMonitorMinimum(laim1);
  ps->max_limit = KheLimitActiveIntervalsMonitorMaximum(laim1);
  ps->history_before = KheLimitActiveIntervalsMonitorHistoryBefore(laim1)> 0;
  ps->history_after  = KheLimitActiveIntervalsMonitorHistoryAfter(laim1) > 0;
  if( laim2 != NULL )
  {
    if( KheLimitActiveIntervalsMonitorMinimum(laim2) > ps->min_limit )
      ps->min_limit = KheLimitActiveIntervalsMonitorMinimum(laim2);
    if( KheLimitActiveIntervalsMonitorMaximum(laim2) < ps->max_limit )
      ps->max_limit = KheLimitActiveIntervalsMonitorMaximum(laim2);
    if( KheLimitActiveIntervalsMonitorHistoryBefore(laim2) > 0 )
      ps->history_before = true;
    if( KheLimitActiveIntervalsMonitorHistoryAfter(laim2)  > 0 )
      ps->history_after = true;
  }

  /* do it if non-trivial min limit and either strict or limits equal */
  if( ps->min_limit >= 2 && (!non_strict || ps->min_limit == ps->max_limit) )
  {
    /* initialize the tasker and pos_groups for profile grouping of laim1 */
    tr = KheCombSolverTasker(ps->comb_solver);
    KheTaskerSetProfileMaxLen(tr, ps->max_limit - 1);
    tg_count = KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
    HaArrayClear(ps->pos_groups);
    for( i = 0;  i < tg_count;  i++ )
    {
      tg = KheLimitActiveIntervalsMonitorTimeGroup(laim1, i, &po);
      KheProfileTimeGroupMake(tr, tg);
      HaArrayAddLast(ps->pos_groups, 0);
    }

    /* keep grouping while groups are being found */
    do
    {
      if( DEBUG8 )
	KheProfileDebug(ps, 1, 2, stderr);
      KheProfileTryUnequalGrouping(ps);
      if( DEBUG8 )
	KheProfileDebug(ps, 1, 2, stderr);
    } while( non_strict && KheProfileTryEqualGrouping(ps) );

    /* clear the tasker */
    KheTaskerDeleteProfileTimeGroups(tr);
  }

  if( DEBUG8 )
    fprintf(stderr, "] KheProfileGroupingForMonitors returning "
      "(groups_count = %d)\n", ps->groups_count);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheProfileGrouping(KHE_COMB_SOLVER cs, bool non_strict)              */
/*                                                                           */
/*  Carry out profile grouping for cs.  If non_strict is true, carry out     */
/*  non_strict profile grouping.  Return the number of groups made.          */
/*                                                                           */
/*****************************************************************************/

int KheProfileGrouping(KHE_COMB_SOLVER cs, bool non_strict)
{
  KHE_MONITOR m;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laimi, laimj;
  int i, j;  KHE_RESOURCE r;  KHE_RESOURCE_TYPE rt;
  KHE_PROFILE_SOLVER ps;

  /* find suitable limit active intervals monitors */
  ps = KheProfileSolverMake(cs);
  HaArrayClear(ps->monitors);
  rt = ps->resource_type;
  r = KheResourceTypeResource(rt, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(ps->soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(ps->soln, r, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laimi = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      if( KheMonitorSuitsProfileGrouping(laimi, rt, non_strict) )
      {
	if( DEBUG7 )
	{
	  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;
	  c = KheLimitActiveIntervalsMonitorConstraint(laimi);
	  fprintf(stderr, "  found suitable monitor, constraint %s\n",
	    KheConstraintId((KHE_CONSTRAINT) c));
	}
	HaArrayAddLast(ps->monitors, laimi);
      }
    }
  }

  /* find pairs of suitable monitors and do the job for each pair */
  for( i = 0;  i < HaArrayCount(ps->monitors);  i++ )
  {
    laimi = HaArray(ps->monitors, i);
    for( j = i + 1;  j < HaArrayCount(ps->monitors);  j++ )
    {
      laimj = HaArray(ps->monitors, j);
      if( KheLimitActiveIntervalsMonitorsCanGroupTogether(laimi, laimj) )
      {
	KheProfileGroupingForMonitors(ps, laimi, laimj, non_strict);
	HaArrayDeleteAndPlug(ps->monitors, j);  /* order matters! */
	HaArrayDeleteAndPlug(ps->monitors, i);  /* order matters! */
	i--;
	break;
      }
    }
  }

  /* find single suitable monitors and do the job for them */
  HaArrayForEach(ps->monitors, laimi, i)
    KheProfileGroupingForMonitors(ps, laimi, NULL, non_strict);
  return ps->groups_count;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheBCDTSpecialAddClass(KHE_TASKER tr, KHE_TIME t)                   */
/*                                                                           */
/*  Add to tr a class corresponding to t.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheBCDTSpecialAddClass(KHE_TASKER tr, KHE_TIME t)
{
  KHE_TASK ER_TIME tt;  KHE_TASKER_CLASS tc;
  tt = KheTaskerTime(tr, KheTimeIndex(t));
  HnAssert(KheTaskerTimeClassCount(tt) == 1,
    "KheBCDTSpecialAddClass(tr, %s): class count is %d\n", KheTimeId(t),
    KheTaskerTimeClassCount(tt));
  tc = KheTaskerTimeClass(tt, 0);
  if( !KheTaskerGroupingAddClass(tr, tc) )
    HnAbort("KheBCDTSpecialAddClass(tr, %s): cannot add class", KheTimeId(t));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheBCDTSpecialCaseGrouping(KHE_TASKER tr, KHE_FRAME common_frame)   */
/*                                                                           */
/*  This function groups the {Wed, Thu, Fri, Sat} and {Sun, Mon, Tue} night  */
/*  shifts.  It is used only on instance COI-BCDT-Sep-A, an experimental     */
/*  version of COI-BCDT-Sep.  It is not for production use.                  */
/*                                                                           */
/*****************************************************************************/

/* *** this works but it was only ever an exploratory thing
static void KheBCDTSpecialCaseGrouping(KHE_TASKER tr, KHE_FRAME common_frame)
{
  int i;  KHE_TIME t1, t2, t3, t4, t5, t6, t7;
  if( DEBUG9 )
    fprintf(stderr, "[ KheBCDTSpecialCaseGrouping()\n");
  for( i = 0;  i < KheFrameTimeGroupCount(common_frame) - 7;  i += 7 )
  {
    ** Wed, Thu, Fri, Sat **
    t1 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 0), 3);
    t2 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 1), 3);
    t3 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 2), 3);
    t4 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 3), 3);
    if( DEBUG9 )
      fprintf(stderr, "  {%s, %s, %s, %s}\n",
	KheTimeId(t1), KheTimeId(t2), KheTimeId(t3), KheTimeId(t4));
    KheTaskerGroupingClear(tr);
    KheBCDTSpecialAddClass(tr, t1);
    KheBCDTSpecialAddClass(tr, t2);
    KheBCDTSpecialAddClass(tr, t3);
    KheBCDTSpecialAddClass(tr, t4);
    KheTaskerGroupingBuild(tr, 2, "BCDT special");

    ** Sun, Mon, Tue **
    t5 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 4), 3);
    t6 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 5), 3);
    t7 = KheTimeGroupTime(KheFrameTimeGroup(common_frame, i + 6), 3);
    if( DEBUG9 )
      fprintf(stderr, "  {%s, %s, %s}\n",
	KheTimeId(t5), KheTimeId(t6), KheTimeId(t7));
    KheTaskerGroupingClear(tr);
    KheBCDTSpecialAddClass(tr, t5);
    KheBCDTSpecialAddClass(tr, t6);
    KheBCDTSpecialAddClass(tr, t7);
    KheTaskerGroupingBuild(tr, 2, "BCDT special");
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheBCDTSpecialCaseGrouping()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,         */
/*    KHE_TASK_SET ts, bool history_off, bool comb_off, bool profile_off,    */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Carry out group by resource constraints for resource type rt.  There     */
/*  is no point in doing anything if there are fewer than two resources.     */
/*                                                                           */
/*****************************************************************************/

static int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_TASK_SET ts, /* bool history_off, */ bool comb_off, bool profile_off,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_TASKER tr;  KHE_COMB_SOLVER cs;  KHE_FRAME common_frame;  int res;
  res = 0;

  /* history grouping */
  /* *** not sure about the whole grouping by history thing
  if( !history_off )
    res += KheGroupByHistory(soln, rt, options, r_ts);
  *** */

  if( KheResourceTypeResourceCount(rt) > 1 )
  {
    /* make a tasker */
    tr = KheTaskerMake(soln, rt, ts, /* false, */ a);

    /* forbid overlaps in common frame time groups */
    common_frame = KheOptionsFrame(options, "gs_common_frame", soln);
    KheTaskerAddOverlapFrame(tr, common_frame);

    /* special case grouping */
    /* ***
    if( strcmp(KheInstanceId(KheSolnInstance(soln)), "COI-BCDT-Sep-A") == 0 )
      KheBCDTSpecialCaseGrouping(tr, common_frame);
    *** */

    /* combinatorial grouping */
    if( DEBUG10 )
      fprintf(stderr, "  KheGroupForResourceType(%s) before comb grouping\n",
	KheResourceTypeId(rt));
    cs = KheCombSolverMake(tr, common_frame);
    if( !comb_off )
      res += KheCombGrouping(cs, options);

    /* profile grouping */
    if( DEBUG10 )
      fprintf(stderr, "  KheGroupForResourceType(%s) before profile(false)\n",
	KheResourceTypeId(rt));
    if( !profile_off )
      res += KheProfileGrouping(cs, false);
    if( DEBUG10 )
      fprintf(stderr, "  KheGroupForResourceType(%s) before profile(true)\n",
	KheResourceTypeId(rt));
    res += KheProfileGrouping(cs, true);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,  */
/*    KHE_OPTIONS options, KHE_TASK_SET ts)                                  */
/*                                                                           */
/*  Group the tasks of soln of type rt by resource constraints.  If rt is    */
/*  NULL, do this for each of the resource types of soln's instance.         */
/*                                                                           */
/*  If ts is non-NULL, add every task that is assigned to another task       */
/*  to ts.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_SET ts)
{
  int i, groups;  KHE_INSTANCE ins;  HA_ARENA a;
  bool /* history_off, */ comb_off, profile_off;
  /* KHE_FLOW flow; */

  /* boilerplate */
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByResourceConstraints(%s %s, -)\n",
      KheInstanceId(ins), rt == NULL ? "all types" : KheResourceTypeId(rt));

  /* return immediately if option rs_group_by_frame_off is true */
  if( KheOptionsGetBool(options, "rs_group_by_rc_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr,"] KheGroupByResourceConstraints returning false (off)\n");
    return false;
  }

  /* temporary:  find a resource flow */
  /* *** crashes with node capacity 0 when run
  flow = KheFlowMake(soln, rt, true, true);
  KheFlowDelete(flow);
  *** */

  /* ***
  history_off = KheOptionsGetBool(options, "rs_group_by_history_off", false);
  *** */
  comb_off = KheOptionsGetBool(options, "rs_group_by_rc_combinatorial_off",
    false);
  profile_off = KheOptionsGetBool(options, "rs_group_by_rc_profile_off", false);

  /* do grouping by resource constraints for each resource type */
  a = KheSolnArenaBegin(soln);
  groups = 0;
  if( rt != NULL )
    groups += KheGroupForResourceType(soln, rt, ts, /* history_off, */
      comb_off, profile_off, options, a);
  else for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    groups += KheGroupForResourceType(soln, rt, ts, /* history_off, */
      comb_off, profile_off, options, a);
  }
  KheSolnArenaEnd(soln, a);

  /* all done */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByResourceConstraints returning %s\n",
      bool_show(groups > 0));
  return groups > 0;
}

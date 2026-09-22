
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
/*  FILE:         khe_sr_comb_finder.c                                       */
/*  DESCRIPTION:  Combinatorial grouping with combination reduction          */
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

typedef HA_ARRAY(KHE_COMB_COVER_TYPE) ARRAY_KHE_COMB_COVER_TYPE;
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;

typedef struct khe_ec_solver_rec {
  KHE_SOLN				soln;
  KHE_FRAME				days_frame;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_RESOURCE_SET			resource_set;
  KHE_EVENT_TIMETABLE_MONITOR		etm;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT constraints;
  ARRAY_KHE_COMB_COVER_TYPE		cover_types;
  HA_ARRAY_INT				intersects;
  HA_ARRAY_INT				provisional_prevs;
} *KHE_EC_SOLVER;


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
/*    KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,               */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Make an eliminate combinations solver with these attributes.             */
/*                                                                           */
/*****************************************************************************/

static KHE_EC_SOLVER KheElimCombSolverMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  HA_ARENA a;  KHE_EC_SOLVER res;  int count;
  a = KheMTaskFinderArena(mtf);
  HaMake(res, a);
  res->soln = KheMTaskFinderSoln(mtf);
  res->days_frame = days_frame;
  res->resource_type = rt;  /* KheMTaskFinderResourceType(mtf); */
  res->resource_set = KheResourceSetMake(res->resource_type, a);
  res->etm = etm;
  HaArrayInit(res->cover_types, a);
  count = KheFrameTimeGroupCount(days_frame);
  HaArrayInit(res->constraints, a);
  HaArrayFill(res->cover_types, count, KHE_COMB_COVER_FREE);
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
  if( KheClusterFirstAndLastIndex(c, offset, ecs->days_frame, &fi, &li) )
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
	f_tg = KheFrameTimeGroup(ecs->days_frame, j);
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
	HaArrayPut(ecs->cover_types, val, KHE_COMB_COVER_PREV);
	if( DEBUG6 )
	{
	  prev_f_tg = KheFrameTimeGroup(ecs->days_frame, val - 1);
	  f_tg = KheFrameTimeGroup(ecs->days_frame, val);
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
  if( KheClusterMonitorFirstAndLastIndex(m, ecs->days_frame, &fi, &li) )
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
	f_tg = KheFrameTimeGroup(ecs->days_frame, j);
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
  for( i = 0;  i < HaArrayCount(ecs->constraints);  i = j )
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
/*  char *KheCombCoverTypesShow(ARRAY_KHE_COMB_COVER_TYPE *cover_types)      */
/*                                                                           */
/*  Brief display of an array of cover types.                                */
/*                                                                           */
/*****************************************************************************/

#define MAX_DISP 100

static int min(int a, int b) { return (a < b ? a : b); }

static char *KheCombCoverTypesShow(ARRAY_KHE_COMB_COVER_TYPE *cover_types)
{
  static char disp[MAX_DISP + 3];  int count, i;  char ch;
  count = min(HaArrayCount(*cover_types), MAX_DISP);
  disp[0] = '<';
  for( i = 0;  i < count;  i++ )
  {
    switch( HaArray(*cover_types, i) )
    {
      case KHE_COMB_COVER_YES:

	ch = 'Y';
	break;

      case KHE_COMB_COVER_NO:

	ch = 'N';
	break;

      case KHE_COMB_COVER_PREV:

	ch = 'P';
	break;

      case KHE_COMB_COVER_FREE:

	ch = 'F';
	break;

      default:

	ch = '?';
	break;
    }
    disp[i + 1] = ch;
  }
  disp[i + 1] = '>';
  disp[i + 2] = '\0';
  return disp;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoCombGroupingForTimeGroup(KHE_COMB_GROUPER cg,                   */
/*    KHE_TIME_GROUP tg, KHE_INTERVAL in, KHE_FRAME days_frame,              */
/*    ARRAY_KHE_COMB_COVER_TYPE *cover_types, KHE_SOLN_ADJUSTER sa)          */
/*                                                                           */
/*  Do combinatorial grouping for interval in of days_frame, for each mtask  */
/*  in tg, and return the number of groups made.                             */
/*                                                                           */
/*****************************************************************************/

static int KheDoCombGroupingForTimeGroup(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_INTERVAL in, KHE_FRAME days_frame,
  ARRAY_KHE_COMB_COVER_TYPE *cover_types, KHE_SOLN_ADJUSTER sa)
{
  KHE_RESOURCE_TYPE rt;  KHE_MTASK_SET mts;
  KHE_MTASK_FINDER mtf;  KHE_MTASK mt;
  KHE_COMB_COVER_TYPE cover_type;
  int first_index, last_index, i, j, groups;
  KHE_TIME_GROUP tg2;

  first_index = KheIntervalFirst(in);
  last_index = KheIntervalLast(in);
  rt = KheCombGrouperResourceType(cg);
  mtf = KheCombGrouperMTaskFinder(cg);
  mts = KheMTaskFinderMTasksInTimeGroup(mtf, rt, tg);
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskAssignIsFixed(mt) && KheMTaskUnassignedTaskCount(mt) > 0 )
    {
      if( DEBUG4 )
	fprintf(stderr, "    [ combinations using %s: %s",
	  KheTimeGroupId(tg), KheMTaskId(mt));
      KheCombGrouperClearRequirements(cg);
      KheCombGrouperAddMTaskRequirement(cg, mt, KHE_COMB_COVER_YES);
      for( j = first_index;  j <= last_index;  j++ )
      {
	tg2 = KheFrameTimeGroup(days_frame, j);
	if( j == first_index )
	  cover_type = KHE_COMB_COVER_FREE;
	else
	  cover_type = HaArray(*cover_types, j);
	KheCombGrouperAddTimeGroupRequirement(cg, tg2, cover_type);
      }
      groups = KheCombGrouperSolve(cg, INT_MAX, KHE_COMB_VARIANT_SOLE_ZERO,
	sa, "combinatorial grouping");
      if( DEBUG4 )
	fprintf(stderr, "    ] %d groups\n", groups);
      if( groups > 0 )
	return groups;
    }
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDoCombGroupingForInterval(KHE_COMB_GROUPER cg, KHE_INTERVAL in,   */
/*    KHE_FRAME days_frame, ARRAY_KHE_COMB_COVER_TYPE *cover_types,          */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Do combinatorial grouping for interval in of days_frame, and return      */
/*  the number of groups made.                                               */
/*                                                                           */
/*  Implementation note.  This code keeps trying for as long as it is        */
/*  finding groups in the interval, either forwards or backwards.            */
/*                                                                           */
/*****************************************************************************/

static int KheDoCombGroupingForInterval(KHE_COMB_GROUPER cg, KHE_INTERVAL in,
  KHE_FRAME days_frame, ARRAY_KHE_COMB_COVER_TYPE *cover_types,
  KHE_SOLN_ADJUSTER sa)
{
  int res, groups;  KHE_TIME_GROUP first_tg, last_tg;
  if( DEBUG2 )
    fprintf(stderr, "  [ KheDoCombGroupingForInterval(cg, %s, %s)\n",
      KheIntervalShow(in, days_frame), KheCombCoverTypesShow(cover_types));
  first_tg = KheFrameTimeGroup(days_frame, KheIntervalFirst(in));
  last_tg = KheFrameTimeGroup(days_frame, KheIntervalLast(in));
  res = 0;
  do
  {
    groups = KheDoCombGroupingForTimeGroup(cg, first_tg, in, days_frame,
      cover_types, sa);
    res += groups;
    if( groups == 0 && last_tg != first_tg )
    {
      groups = KheDoCombGroupingForTimeGroup(cg, last_tg, in, days_frame,
	cover_types, sa);
      res += groups;
    }
  } while( groups > 0 );

  if( DEBUG2 )
    fprintf(stderr, "  ] KheDoCombGroupingForInterval returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,            */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Do some combinatorial grouping.                                          */
/*                                                                           */
/*****************************************************************************/

int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,
  KHE_SOLN_ADJUSTER sa)
{
  int max_days, len, first_index, days, total_groups;  KHE_MTASK_FINDER mtf;
  KHE_FRAME days_frame;  KHE_EC_SOLVER ecs;  KHE_EVENT_TIMETABLE_MONITOR etm;

  /* get options */
  mtf = KheCombGrouperMTaskFinder(cg);
  max_days = KheOptionsGetInt(options, "rs_group_by_rc_max_days", 3);
  days_frame = KheOptionsFrame(options, "gs_common_frame",
    KheMTaskFinderSoln(mtf));
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);

  /* eliminate combinations */
  if( DEBUG10 )
    fprintf(stderr, "  KheCombGrouping before KheElimCombSolve\n");
  ecs = KheElimCombSolverMake(mtf, KheCombGrouperResourceType(cg),
    days_frame, etm);
  KheElimCombSolve(ecs);

  /* try each interval */
  total_groups = 0;
  days = KheFrameTimeGroupCount(days_frame);
  for( len = max_days;  len >= 2;  len-- )
  {
    if( DEBUG10 )
      fprintf(stderr, "  KheCombGrouping starting len %d\n", len);
    for( first_index = 0;  first_index <= days - len;  first_index++ )
      total_groups += KheDoCombGroupingForInterval(cg,
	KheIntervalMake(first_index, first_index + len - 1),
	days_frame, &ecs->cover_types, sa);
  }
  return total_groups;
}

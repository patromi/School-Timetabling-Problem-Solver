
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
/*  FILE:         khe_sr_comb_grouping.c                                     */
/*  DESCRIPTION:  Combinatorial task grouping                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG1 0  /* combination elimination */
#define DEBUG2 0
#define DEBUG3 0	/* recursive search */
#define DEBUG4 0	/* KheCombGrouperAddCombMTask */
#define DEBUG5 0	/* adding requirements */
#define DEBUG7 0	/* bases of recursion */
#define DEBUG8 0
#define	DEBUG10 0


/*****************************************************************************/
/*                                                                           */
/*  type KHE_COMB_GROUPER - for combinatorial grouping                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_comb_grouper_rec *KHE_COMB_GROUPER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_EC_SOLVER - a separate solver for eliminating combinations      */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;

typedef struct khe_ec_solver_rec {
  KHE_SOLN				soln;
  KHE_FRAME				days_frame;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_RESOURCE_SET			resource_set;
  KHE_EVENT_TIMETABLE_MONITOR		etm;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT constraints;
  HA_ARRAY_INT				intersects;
  HA_ARRAY_INT				provisional_prevs;
} *KHE_EC_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_COVER_TYPE - type of cover                                 */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_COMB_COVER_YES,
  KHE_COMB_COVER_NO,
  KHE_COMB_COVER_PREV,
  KHE_COMB_COVER_FREE
} KHE_COMB_COVER_TYPE;

typedef HA_ARRAY(KHE_COMB_COVER_TYPE) ARRAY_KHE_COMB_COVER_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_VARIANT_TYPE - variant of the solve algorithm              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_COMB_VARIANT_MIN,
  KHE_COMB_VARIANT_ZERO,
  KHE_COMB_VARIANT_SOLE_ZERO,
  KHE_COMB_VARIANT_SINGLES
} KHE_COMB_VARIANT_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COVER_REQUIREMENT                                               */
/*                                                                           */
/*  One cover requirement (one time group or mtask requirement), including   */
/*  how many times it has been covered.                                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_cover_requirement_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_TIME_GROUP		time_group;	 /* if reqt is a time group  */
  KHE_MTASK			mtask;		 /* if reqt is am mtask      */
  KHE_TIME_SET			time_set;
  KHE_COMB_COVER_TYPE		cover_type;
  bool				last_cover_done; /* last mtask covering e set*/
  int				cover_count;
} *KHE_COVER_REQUIREMENT;

typedef HA_ARRAY(KHE_COVER_REQUIREMENT) ARRAY_KHE_COVER_REQUIREMENT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_MTASK                                                      */
/*                                                                           */
/*  Information about one mtask, including the cover_reqts it covers.        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_comb_mtask_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_MTASK			mtask;
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  ARRAY_KHE_COVER_REQUIREMENT	last_cover_reqts;
} *KHE_COMB_MTASK;

typedef HA_ARRAY(KHE_COMB_MTASK) ARRAY_KHE_COMB_MTASK;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_GROUPER - solver for combinatorial grouping                */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;
/* typedef HA_ARRAY(KHE_MTASK_GROUP) ARRAY_KHE_MTASK_GROUP; */

struct khe_comb_grouper_rec {

  /* free lists */
  /* ARRAY_KHE_MTASK_GROUP	mtask_group_free_list; */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqt_free_list;
  ARRAY_KHE_COMB_MTASK		comb_mtask_free_list;

  /* constant for the lifetime of the solver */
  HA_ARENA			arena;
  KHE_MTASK_FINDER		mtask_finder;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;

  /* requirements, may vary freely but always defined */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  bool				no_singles;
  KHE_RESOURCE_GROUP		preferred_domain;

  /* derived from the requirements; need resetting only when they are reset */
  bool				derived_info_up_to_date;
  ARRAY_KHE_MTASK		reqd_mtasks;
  ARRAY_KHE_COMB_MTASK		comb_mtasks;

  /* varying between and during solves */
  KHE_COMB_VARIANT_TYPE		variant;
  KHE_MTASK_GROUPER		curr_mtg;
  KHE_MTASK_GROUPER		best_mtg;	/* passed in by caller */
  KHE_COST			best_mtg_cost;
  int				zero_cost_groups_count;
  int				singles_tasks_count;
};


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
	  if( DEBUG1 )
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
	    if( DEBUG1 )
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
/*    KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,                 */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Make an eliminate combinations solver with these attributes.             */
/*                                                                           */
/*****************************************************************************/

KHE_EC_SOLVER KheElimCombSolverMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  HA_ARENA a;  KHE_EC_SOLVER res;  /* int count; */
  a = KheMTaskFinderArena(mtf);
  HaMake(res, a);
  res->soln = KheMTaskFinderSoln(mtf);
  res->days_frame = days_frame;
  res->resource_type = rt;  /* KheMTaskFinderResourceType(mtf); */
  res->resource_set = KheResourceSetMake(res->resource_type, a);
  res->etm = etm;
  /* HaArrayInit(res->cover_types, a); */
  /* count = KheFrameTimeGroupCount(days_frame); */
  HaArrayInit(res->constraints, a);
  /* HaArrayFill(res->cover_types, count, KHE_COMB_COVER_FREE); */
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
  int start, int stop, int offset, ARRAY_KHE_COMB_COVER_TYPE *cover_types)
{
  KHE_TIME_GROUP c_tg, f_tg, prev_f_tg;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c;
  int i, j, val, lim, count, sum, fi, li, supply, demand;  KHE_POLARITY po;
  bool f_tg_overlaps, prev_f_tg_overlaps;
  if( DEBUG1 )
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
      if( DEBUG1 )
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
	  if( DEBUG1 )
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
    if( DEBUG1 )
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
	HaArrayPut(*cover_types, val, KHE_COMB_COVER_PREV);
	if( DEBUG1 )
	{
	  prev_f_tg = KheFrameTimeGroup(ecs->days_frame, val - 1);
	  f_tg = KheFrameTimeGroup(ecs->days_frame, val);
	  fprintf(stderr, "  linking %s and %s\n", KheTimeGroupId(prev_f_tg),
            KheTimeGroupId(f_tg));
	}
      }
  }
  if( DEBUG1 )
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
  if( DEBUG1 )
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
	  if( DEBUG1 )
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
  if( DEBUG1 )
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
/*  void KheElimCombSolve(KHE_EC_SOLVER ecs,                                 */
/*    ARRAY_KHE_COMB_COVER_TYPE *cover_types)                                */
/*                                                                           */
/*  Eliminate combinations, expressing the eliminations by changes to        */
/*  ecs->cover_types.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheElimCombSolve(KHE_EC_SOLVER ecs, ARRAY_KHE_COMB_COVER_TYPE *cover_types)
{
  int i, j, k, offset, offset_count;  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, cbtc2;
  rt = ecs->resource_type;
  if( DEBUG1 )
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
    if( DEBUG1 )
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
        KheElimCombSolveForConstraintSet(ecs, i, j, offset, cover_types);
      }
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheElimCombSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  ARRAY_KHE_COMB_COVER_TYPE *KheElimSolverCoverTypes(KHE_EC_SOLVER ecs)    */
/*                                                                           */
/*  Return the cover types array that is the result of ecs.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
ARRAY_KHE_COMB_COVER_TYPE *KheElimSolverCoverTypes(KHE_EC_SOLVER ecs)
{
  return &ecs->cover_types;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombinationElimination(KHE_MTASK_FINDER mtf,                     */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,                            */
/*    KHE_EVENT_TIMETABLE_MONITOR etm,                                       */
/*    ARRAY_KHE_COMB_COVER_TYPE *cover_types)                                */
/*                                                                           */
/*  Make a combination elimination solver for rt, solve it, and place        */
/*  the result in previously initiated array *cover_types.                   */
/*                                                                           */
/*****************************************************************************/

static void KheCombinationElimination(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm,
  ARRAY_KHE_COMB_COVER_TYPE *cover_types)
{
  KHE_EC_SOLVER ecs;  int count;
  ecs = KheElimCombSolverMake(mtf, rt, days_frame, etm);
  HaArrayClear(*cover_types);
  count = KheFrameTimeGroupCount(days_frame);
  HaArrayFill(*cover_types, count, KHE_COMB_COVER_FREE);
  KheElimCombSolve(ecs, cover_types);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_COVER_TYPE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)                      */
/*                                                                           */
/*  Show a cover type.                                                       */
/*                                                                           */
/*****************************************************************************/

static char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)
{
  switch( cover_type )
  {
    case KHE_COMB_COVER_YES:	return "comb_cover_yes";
    case KHE_COMB_COVER_NO:	return "comb_cover_no";
    case KHE_COMB_COVER_PREV:	return "comb_cover_prev";
    case KHE_COMB_COVER_FREE:	return "comb_cover_free";

    default:

      HnAbort("CoverTypeShow: unexpected cover type (%d)", cover_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_VARIANT_TYPE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)                      */
/*                                                                           */
/*  Show a comb variant.                                                     */
/*                                                                           */
/*****************************************************************************/

static char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)
{
  switch( cgv )
  {
    case KHE_COMB_VARIANT_MIN:	 	return "comb_variant_min";
    case KHE_COMB_VARIANT_ZERO:	 	return "comb_variant_zero";
    case KHE_COMB_VARIANT_SOLE_ZERO:	return "comb_variant_sole_zero";
    case KHE_COMB_VARIANT_SINGLES:	return "comb_variant_singles";

    default:

      HnAbort("KheCombVariantShow: unexpected cost type (%d)", cgv);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COVER_REQUIREMENT"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)            */
/*                                                                           */
/*  Make a cover requirement object.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  if( HaArrayCount(cg->cover_reqt_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->cover_reqt_free_list);
    KheTimeSetClear(res->time_set);
  }
  else
  {
    HaMake(res, cg->arena);
    res->time_set = KheTimeSetMake(KheSolnInstance(cg->soln), cg->arena);
  }
  res->grouper = cg;
  res->time_group = tg;
  res->mtask = mt;
  if( tg != NULL )
    KheTimeSetAddTimeGroup(res->time_set, tg);
  res->cover_type = cover;
  res->last_cover_done = false;
  res->cover_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)  */
/*                                                                           */
/*  Return true if cr is covered by mt.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)
{
  if( cr->time_group != NULL )
    return !KheTimeSetDisjoint(cr->time_set, KheMTaskTimeSet(mt));
  else
    return cr->mtask == mt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr, FILE *fp)  */
/*                                                                           */
/*  Debug print of the header of a cover requirement.                        */
/*                                                                           */
/*****************************************************************************/

static void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr,
  FILE *fp)
{
  if( cr->time_group != NULL )
    fprintf(fp, "CoverRequirement(tg %s, %s, %d covers)",
      KheTimeGroupId(cr->time_group), CoverTypeShow(cr->cover_type),
      cr->cover_count);
  else
    fprintf(fp, "CoverRequirement(mt %p %s %s, %d tasks, %s, %d covers)",
      (void *) cr->mtask, KheMTaskId(cr->mtask),
      KheIntervalShow(KheMTaskInterval(cr->mtask), cr->grouper->days_frame),
      KheMTaskTaskCount(cr->mtask), CoverTypeShow(cr->cover_type),
      cr->cover_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp);

static void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCoverRequirementHeaderDebug(cr, fp);
    fprintf(fp, "\n");
  }
  else
    KheCoverRequirementHeaderDebug(cr, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_MTASK"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)       */
/*                                                                           */
/*  Make an mtask info object.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COMB_MTASK res;
  if( HaArrayCount(cg->comb_mtask_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->comb_mtask_free_list);
    HaArrayClear(res->cover_reqts);
    HaArrayClear(res->last_cover_reqts);
  }
  else
  {
    HaMake(res, cg->arena);
    HaArrayInit(res->cover_reqts, cg->arena);
    HaArrayInit(res->last_cover_reqts, cg->arena);
  }
  res->grouper = cg;
  res->mtask = mt;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)                           */
/*                                                                           */
/*  Return the "size" of cmt's domain, which is either the number of         */
/*  resources in cmt->mtask's domain, or INT_MAX if that does not suit.      */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)
{
  KHE_RESOURCE_GROUP domain;
  domain = KheMTaskDomain(cmt->mtask);
  if( KheResourceGroupSubset(cmt->grouper->preferred_domain, domain) )
    return KheResourceGroupResourceCount(domain);
  else
    return INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting an array of comb mtask objects.          */
/*                                                                           */
/*  The first priority is to sort the array so that mtasks that start on     */
/*  earlier days come before mtasks that start on later days.  This is       */
/*  vital because we use intervals during the solve and they assume that     */
/*  sets of mtasks have no gaps.                                             */
/*                                                                           */
/*  The second priority is to sort the array so that mtasks whose domains    */
/*  are a better fit for cg->preferred_domain (if present) come before       */
/*  mtasks whose fit is worse.  If the mtask's domain is not a superset      */
/*  of the preferred domain, it is maximally bad.  If it is a superset,      */
/*  then smaller domains are preferred to larger ones.                       */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskCmp(const void *t1, const void *t2)
{
  KHE_COMB_MTASK cmt1, cmt2;  KHE_INTERVAL in1, in2;  int cmp;
  cmt1 = * (KHE_COMB_MTASK *) t1;
  cmt2 = * (KHE_COMB_MTASK *) t2;

  /* first priority: chronologically earlier mtasks precede later ones */
  in1 = KheMTaskInterval(cmt1->mtask);
  in2 = KheMTaskInterval(cmt2->mtask);
  cmp = KheIntervalFirst(in1) - KheIntervalFirst(in2);
  if( cmp != 0 )
    return cmp;

  /* second priority: preferred domain, if requested */
  if( cmt1->grouper->preferred_domain != NULL )
  {
    cmp = KheCombMTaskDomainSize(cmt1) - KheCombMTaskDomainSize(cmt2);
    if( cmp != 0 )
      return cmp;
  }

  /* no difference */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)                      */
/*                                                                           */
/*  Return true if cmt is the last possible cover for a cover requirement    */
/*  that is not yet covered, so that consequently it must be included.       */
/*                                                                           */
/*****************************************************************************/

static bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->last_cover_reqts, cr, i)
    if( cr->cover_count == 0 )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header of cmt onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)
{
  fprintf(fp, "CombMTask(%p %s, %d tasks, %d unassigned, %d cover_reqts, "
    "%d last_cover)", (void *) cmt->mtask, KheMTaskId(cmt->mtask),
    KheMTaskTaskCount(cmt->mtask), KheMTaskUnassignedTaskCount(cmt->mtask),
    HaArrayCount(cmt->cover_reqts), HaArrayCount(cmt->last_cover_reqts));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cmt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCombMTaskHeaderDebug(cmt, fp);
    fprintf(fp, "\n");
  }
  else
    KheCombMTaskHeaderDebug(cmt, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - construction and query"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,                */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new comb grouper based on mtf.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_COMB_GROUPER res;

  /* free lists */
  HaMake(res, a);
  /* HaArrayInit(res->mtask_group_free_list, a); */
  HaArrayInit(res->cover_reqt_free_list, a);
  HaArrayInit(res->comb_mtask_free_list, a);

  /* constant for the lifetime of the solver */
  res->arena = a;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->resource_type = rt;  /* KheMTaskFinderResourceType(mtf); */
  res->days_frame = KheMTaskFinderDaysFrame(mtf);

  /* requirements, may vary freely but always defined */
  HaArrayInit(res->cover_reqts, a);
  res->no_singles = false;
  res->preferred_domain = NULL;

  /* derived from the requirements; need resetting only when they are reset */
  res->derived_info_up_to_date = false;
  HaArrayInit(res->reqd_mtasks, a);
  HaArrayInit(res->comb_mtasks, a);

  /* varying between and during solves */
  res->variant = KHE_COMB_VARIANT_SINGLES;
  res->curr_mtg = KheMTaskGrouperMake(a);
  res->best_mtg = NULL;   /* passed in by caller */
  res->best_mtg_cost = KheCost(INT_MAX, INT_MAX);
  res->zero_cost_groups_count = 0;
  res->singles_tasks_count = 0;

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Return the mtask finder attribute of cg.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)
{
  return cg->mtask_finder;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheCombGrouperSoln(KHE_COMB_GROUPER cg)                         */
/*                                                                           */
/*  Return cg's soln.                                                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_SOLN KheCombGrouperSoln(KHE_COMB_GROUPER cg)
{
  return cg->soln;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)        */
/*                                                                           */
/*  Return the resource type handled by cg.                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)
{
  return cg->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheCombGrouperArena(KHE_COMB_GROUPER cg)                        */
/*                                                                           */
/*  Return cg's arena.                                                       */
/*                                                                           */
/*****************************************************************************/

static HA_ARENA KheCombGrouperArena(KHE_COMB_GROUPER cg)
{
  return cg->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - mtask group free list"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUP KheCombGrouperGetMTaskGroup(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Get a free mtask group object from cg's free list; or return NULL        */
/*  if the free list is empty.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_MTASK_GROUP KheCombGrouperGetMTaskGroup(KHE_COMB_GROUPER cg)
{
  if( HaArrayCount(cg->mtask_group_free_list) > 0 )
    return HaArrayLastAndDelete(cg->mtask_group_free_list);
  else
    return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperPutMTaskGroup(KHE_COMB_GROUPER cg, KHE_MTASK_GROUP mg)*/
/*                                                                           */
/*  Put mg onto cg's free list.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperPutMTaskGroup(KHE_COMB_GROUPER cg, KHE_MTASK_GROUP mg)
{
  HaArrayAddLast(cg->mtask_group_free_list, mg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - loading requirements"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Clear away all requirements.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)
{
  int i;
  HaArrayAppend(cg->cover_reqt_free_list, cg->cover_reqts, i);
  HaArrayClear(cg->cover_reqts);
  cg->no_singles = false;
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperClearRequirements(cg) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)                          */
/*                                                                           */
/*  Add a time group cover requirement to the solver.                        */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, tg, NULL, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddTimeGroupRequirement(cg, %s, %s) ]\n",
      KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-",
      CoverTypeShow(cover));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Undo the effect of a corresponding time group add.                       */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->time_group == tg )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteTimeGroupRequirement: tg not present");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,              */
/*    KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)                               */
/*                                                                           */
/*  Add an mtask cover requirement to the solver.                            */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, NULL, mt, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddMTaskRequirement(cg, mt) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,           */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Undo the effect of a corresponding mtask add.                            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->mtask == mt )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteMTaskRequirement: mt not present");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Add a requirement that there be no singles.                              */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)       */
/*                                                                           */
/*  Undo the effect of a corresponding no singles add.                       */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,    */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add a preferred domain requirement (for rg) to cg.                       */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,
  KHE_RESOURCE_GROUP rg)
{
  cg->preferred_domain = rg;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg) */
/*                                                                           */
/*  Delete any preferred domain requirement from cg.                         */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg)
{
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - setting up for solving"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Set the required mtasks of cg.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayClear(cg->reqd_mtasks);
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_YES && cr->mtask != NULL )
      HaArrayAddLast(cg->reqd_mtasks, cr->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,    */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Return true if mt clashes with any of the required mtasks of cg,         */
/*  without being required itself.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_MTASK mt2;  int i, pos;
  if( !HaArrayContains(cg->reqd_mtasks, mt, &pos) )
  {
    HaArrayForEach(cg->reqd_mtasks, mt2, i)
      if( !KheIntervalDisjoint(KheMTaskInterval(mt), KheMTaskInterval(mt2)) )
	return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)      */
/*                                                                           */
/*  Return true if mt is wanted, because it has not already been chosen, it  */
/*  it does not cover any No requirement, it is not fixed and it needs       */
/*  assignment, and it does not clash with any required mtask.               */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;  KHE_COMB_MTASK cmt;

  /* if mt has already been chosen, we don't want it again */
  HaArrayForEach(cg->comb_mtasks, cmt, i)
    if( cmt->mtask == mt )
      return false;

  /* if mt covers any No requirement, we don't want it (2) */
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_NO &&
	KheCoverRequirementCovered(cr, mt) )
      return false;

  /* if mt is fixed or does not need assignment, we don't want it (3) */
  if( KheMTaskAssignIsFixed(mt) || !KheMTaskNeedsAssignment(mt) )
    return false;

  /* if mt is not in cg->reqd_mtasks but it does clash with them, no */
  if( KheCombGrouperMTaskClashesWithRequiredMTask(cg, mt) )
    return false;

  /* otherwise all good and we want mt */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperSetUpForSolving(KHE_COMB_GROUPER cg,                  */
/*    KHE_COMB_VARIANT_TYPE cg_variant)                                      */
/*                                                                           */
/*  Set up cg for solving.  Return true if it is worth doing the solve.      */
/*                                                                           */
/*****************************************************************************/
static void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp);

static bool KheCombGrouperSetUpForSolving(KHE_COMB_GROUPER cg,
  KHE_COMB_VARIANT_TYPE cg_variant)
{
  KHE_COVER_REQUIREMENT cr;  int i, j;
  KHE_MTASK mt;  KHE_COMB_MTASK cmt;  KHE_MTASK_SET mts;

  /* derived from the requirements */
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSetUpForSolving(cg, cg_variant)\n");
  if( !cg->derived_info_up_to_date )
  {
    /* initialize reqd_mtasks */
    KheCombGrouperSetRequiredMTasks(cg);

    /* initialize comb_mtasks for this solve, first removing any old mtasks */
    HaArrayAppend(cg->comb_mtask_free_list, cg->comb_mtasks, i);
    HaArrayClear(cg->comb_mtasks);
    HaArrayForEach(cg->cover_reqts, cr, i)
      if( cr->cover_type != KHE_COMB_COVER_NO )
      {
	if( cr->time_group != NULL )
	{
	  /* cr is a time group */
	  mts = KheMTaskFinderMTasksInTimeGroup(cg->mtask_finder,
	    cg->resource_type, cr->time_group);
	  for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
	  {
	    mt = KheMTaskSetMTask(mts, j);
	    if( KheCombGrouperMTaskIsWanted(cg, mt) )
	      HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, mt));
	  }
	}
	else
	{
	  /* cr is an mtask */
	  if( KheCombGrouperMTaskIsWanted(cg, cr->mtask) )
	    HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, cr->mtask));
	}
      }

    /* sort comb_mtasks into chronological order */
    HaArraySort(cg->comb_mtasks, &KheCombMTaskCmp);

    /* clear out last_cover_done fields */
    HaArrayForEach(cg->cover_reqts, cr, j)
      cr->last_cover_done = false;

    /* set last_cover_done and last_cover_reqts fields */
    HaArrayForEachReverse(cg->comb_mtasks, cmt, i)
      HaArrayForEach(cg->cover_reqts, cr, j)
        if( KheCoverRequirementCovered(cr, cmt->mtask) )
	{
	  /* add cr to cmt->cover_reqts */
	  HaArrayAddLast(cmt->cover_reqts, cr);

	  /* sort out last_cover */
	  if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
	  {
	    /* cmt is e's last cover */
            cr->last_cover_done = true;
	    HaArrayAddLast(cmt->last_cover_reqts, cr);
	  }
	}

    /* if any Yes cover requirement has no last cover, we can quit now */
    HaArrayForEach(cg->cover_reqts, cr, j)
      if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
      {
	if( DEBUG2 )
	{
	  KheCombGrouperDebug(cg, 2, 2, stderr);
	  fprintf(stderr, "  not covered: ");
	  KheCoverRequirementDebug(cr, 2, 0, stderr);
	  fprintf(stderr, "] KheCombGrouperSetUpForSolving returning false\n");
	}
	return false;
      }

    /* derived info is up to date now */
    cg->derived_info_up_to_date = true;
  }

  /* varying between and during solves */
  cg->variant = cg_variant;
  KheMTaskGrouperClear(cg->curr_mtg);
  cg->zero_cost_groups_count = 0;
  cg->singles_tasks_count = 0;

  /* check that cover counts are all zero */
  HaArrayForEach(cg->cover_reqts, cr, i)
    HnAssert(cr->cover_count == 0,
      "KheCombGrouperSetUpForSolving internal error 1 (index %d)\n", i);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSetUpForSolving returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - solving and debug"                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt) */
/*                                                                           */
/*  Add cmt and return true, or return false and change nothing if adding    */
/*  cmt would not work with the current group.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt,
  int curr_index)
{
  KHE_COVER_REQUIREMENT cr;  int i;

  /* make sure that cmt's mtask is acceptable to the current group */
  if( !KheMTaskGrouperAddMTask(cg->curr_mtg, cmt->mtask) )    /* (6), (7) */
  {
    if( DEBUG4 )
      fprintf(stderr, "%*s  add mtask info failed on group (%d mtasks)\n",
	2*curr_index, "", KheMTaskGrouperMTaskCount(cg->curr_mtg));
    return false;
  }

  /* all good, update the cover counts and return true */
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,                  */
/*    KHE_COMB_MTASK cmt)                                                    */
/*                                                                           */
/*  Undo the corresponding call to KheCombGrouperAddCombMTask.               */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count--;
  KheMTaskGrouperDeleteMTask(cg->curr_mtg, cmt->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperCurrGroupCoversRequirements(KHE_COMB_GROUPER cg)      */
/*                                                                           */
/*  Return true if cg's current mtask group has valid cover.  This is (11).  */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperCurrGroupCoversRequirements(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr, prev_cr;  int i;
  prev_cr = NULL;
  HaArrayForEach(cg->cover_reqts, cr, i)
  {
    switch( cr->cover_type )
    {
      case KHE_COMB_COVER_YES:

	/* should be covered at least once by now */
	if( DEBUG8 && cr->cover_count <= 0 )
	{
	  fprintf(stderr, "  KheCurrGroupCoversRequirements failing:\n");
          KheCombGrouperDebug(cg, 2, 2, stderr);
	}
	HnAssert(cr->cover_count > 0,
	  "KheCombGrouperCurrGroupCoversRequirements internal error 1");
	break;

      case KHE_COMB_COVER_NO:

	/* should be not covered, in fact no mtasks that cover it are tried */
	HnAssert(cr->cover_count == 0,
	  "KheCombGrouperCurrGroupCoversRequirements internal error 2");
	break;

      case KHE_COMB_COVER_PREV:

	/* should be covered if and only if prev is covered */
	if( prev_cr != NULL &&
	    (cr->cover_count > 0) != (prev_cr->cover_count > 0) )
	  return false;
	break;

      case KHE_COMB_COVER_FREE:

	/* any cover is acceptable here */
	break;

      default:

	HnAbort("KheCombGrouperCurrGroupCoversRequirements internal error"
	  " (%d)", cr->cover_type);
    }
    prev_cr = cr;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)            */
/*                                                                           */
/*  Return true if the current group satisfies cg's singles requirement.     */
/*                                                                           */
/*****************************************************************************/

static bool KheCurrGroupSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)
{
  return KheMTaskGrouperMTaskCount(cg->curr_mtg) >= (cg->no_singles ? 2 : 1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)                      */
/*                                                                           */
/*  Return true if singles requires all mtasks from now on to be excluded.   */
/*                                                                           */
/*****************************************************************************/

static bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)
{
  return cg->variant == KHE_COMB_VARIANT_SINGLES &&
    KheMTaskGrouperMTaskCount(cg->curr_mtg) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)        */
/*                                                                           */
/*  Solve cg from cg->comb_mtask[curr_index] onwards.  Return true if it's   */
/*  time to stop.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)
{
  KHE_MTASK mt;  KHE_COMB_MTASK curr_cmt;  bool stopping;  KHE_COST cost;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cg, %d):\n", 2*curr_index+2,"",curr_index);
  if( curr_index >= HaArrayCount(cg->comb_mtasks) )
  {
    /* end of recursion; if suitable mtasks, try them */
    if( KheCurrGroupSatisfiesSinglesRequirement(cg) &&        /* (9)  */
        KheCombGrouperCurrGroupCoversRequirements(cg) )       /* (11) */
    {
      switch( cg->variant )
      {
	case KHE_COMB_VARIANT_MIN:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  break;

	case KHE_COMB_VARIANT_ZERO:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  if( cost == 0 )
	  {
	    /* new zero cost; save group and stop */
	    cg->zero_cost_groups_count++;
	    if( DEBUG3 )
	      fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	    return true;
	  }
	  break;

	case KHE_COMB_VARIANT_SOLE_ZERO:

          /* if( KheMTaskGroupHasCost(cg->curr_mg, &cost) ) */
	  cost = KheMTaskGrouperCost(cg->curr_mtg);
	  if( DEBUG3 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 2*curr_index + 4, stderr);
	  if( DEBUG7 )
	    KheMTaskGrouperDebug(cg->curr_mtg, 1, 4, stderr);
	  if( cost < cg->best_mtg_cost )
	  {
	    KheMTaskGrouperCopy(cg->best_mtg, cg->curr_mtg);
	    cg->best_mtg_cost = cost;
	  }
	  if( cost == 0 )
	  {
	    /* new zero cost; record its presence */
	    cg->zero_cost_groups_count++;
	    if( cg->zero_cost_groups_count > 1 )
	    {
	      if( DEBUG3 )
		fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	      return true;
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SINGLES:

	  if( KheMTaskGrouperMTaskCount(cg->curr_mtg) == 1 )
	  {
	    mt = KheMTaskGrouperMTask(cg->curr_mtg, 0);
	    cg->singles_tasks_count += KheMTaskUnassignedTaskCount(mt);
	  }
	  break;

	default:

	  HnAbort("KheCombGrouperSolveFrom internal error (variant %d)",
	    cg->variant);
	  break;
      }
    }
  }
  else
  {
    /* try including curr_cmt */
    curr_cmt = HaArray(cg->comb_mtasks, curr_index);
    if( DEBUG3 )
      KheCombMTaskDebug(curr_cmt, 2, 2*curr_index + 4, stderr);
    if( KheCombGrouperAddCombMTask(cg, curr_cmt, curr_index) )
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      KheCombGrouperDeleteCombMTask(cg, curr_cmt);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }

    /* try excluding curr_cmt, unless it is the last cover for some reqt */
    if( !KheCombMTaskMustBeIncluded(curr_cmt) &&
        !KheSinglesRequiresExclude(cg) )              /* (8) */
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", 2*curr_index + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperSolve(KHE_COMB_GROUPER cg,                            */
/*    KHE_COMB_VARIANT_TYPE cg_variant, KHE_MTASK_GROUP mg)                  */
/*                                                                           */
/*  Solve cg for the previously loaded requirements.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSolve(KHE_COMB_GROUPER cg,
  KHE_COMB_VARIANT_TYPE cg_variant, KHE_MTASK_GROUPER mtg)
{
  bool res;

  /* boilerplate */
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSolve(cg, %s)\n",
      KheCombVariantShow(cg_variant));
  HnAssert(cg_variant >= KHE_COMB_VARIANT_MIN &&
    cg_variant <= KHE_COMB_VARIANT_SOLE_ZERO,
    "KheCombGrouperSolve: invalid cg_variant");
  HnAssert(mtg != NULL, "KheCombGrouperSolve: mtg is NULL");

  /* do the solve */
  cg->best_mtg = mtg;
  cg->best_mtg_cost = KheCost(INT_MAX, INT_MAX);
  KheMTaskGrouperClear(cg->best_mtg);
  if( KheCombGrouperSetUpForSolving(cg, cg_variant) )
    KheCombGrouperSolveFrom(cg, 0);

  /* sort out the consequences */
  switch( cg_variant )
  {
    case KHE_COMB_VARIANT_MIN:

      /* successful if there is at least one solution */
      res = (KheMTaskGrouperMTaskCount(cg->best_mtg) > 0);
      break;

    case KHE_COMB_VARIANT_ZERO:

      /* successful if there is at least one zero cost solution */
      res = (cg->zero_cost_groups_count >= 1);
      break;

    case KHE_COMB_VARIANT_SOLE_ZERO:

      /* successful if there is exactly one zero cost solution */
      res = (cg->zero_cost_groups_count == 1);
      break;

    case KHE_COMB_VARIANT_SINGLES:
    default:

      HnAbort("KheCombGrouperSolve: unknown cg_variant (%d)", cg_variant);
      res = false;  /* keep compiler happy */
      break;
  }

  /* all done */
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSolve returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouperSolveSingles(KHE_COMB_GROUPER cg)                      */
/*                                                                           */
/*  Find the number of singles.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheCombGrouperSolveSingles(KHE_COMB_GROUPER cg)
{
  int res;

  ** boilerplate **
  if( DEBUG2 )
    fprintf(stderr, "[ KheCombGrouperSolveSingles(cg)\n");

  ** do the solve **
  if( KheCombGrouperSetUpForSolving(cg, KHE_COMB_VARIANT_SINGLES) )
    KheCombGrouperSolveFrom(cg, 0);
  res = cg->singles_tasks_count;

  ** all done **
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSolveSingles returning %d\n", res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp)
{
  KHE_COVER_REQUIREMENT cr;  int i;  KHE_COMB_MTASK cmt;
  fprintf(fp, "%*s[ CombGrouper(%s %s, no_singles %s, domain %s)\n",
    indent, "", KheInstanceId(KheSolnInstance(cg->soln)),
    KheResourceTypeId(cg->resource_type),
    bool_show(cg->no_singles), cg->preferred_domain != NULL ?
      KheResourceGroupId(cg->preferred_domain) : "-");
  HaArrayForEach(cg->cover_reqts, cr, i)
    KheCoverRequirementDebug(cr, verbosity, indent + 2, fp);
  fprintf(fp, "%*s------------\n", indent + 2, "");
  HaArrayForEach(cg->comb_mtasks, cmt, i)
  {
    fprintf(fp, "%*s", indent + 2, "");
    if( KheMTaskGrouperContainsMTask(cg->curr_mtg, cmt->mtask) )
      fprintf(fp, "** ");
    else
      fprintf(fp, "   ");
    KheCombMTaskDebug(cmt, verbosity, 0, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
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

/* static int min(int a, int b) { return (a < b ? a : b); } */

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
  ARRAY_KHE_COMB_COVER_TYPE *cover_types, KHE_SOLN_ADJUSTER sa,
  KHE_MTASK_GROUPER scratch_mtg)
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
  /* mg = KheMTaskGroupMake(cg); */
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskAssignIsFixed(mt) && KheMTaskNeedsAssignment(mt) )
    {
      if( DEBUG2 )
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
      KheMTaskGrouperClear(scratch_mtg);
      if( KheCombGrouperSolve(cg, KHE_COMB_VARIANT_SOLE_ZERO, scratch_mtg) )
	groups = KheMTaskGrouperMakeGroups(scratch_mtg, INT_MAX,
	  sa /* , "combinatorial grouping" */);
      else
	groups = 0;
      if( DEBUG2 )
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
/*    KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)                   */
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
  KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)
{
  int res, groups;  KHE_TIME_GROUP first_tg, last_tg;
  if( DEBUG4 )
    fprintf(stderr, "  [ KheDoCombGroupingForInterval(cg, %s, %s)\n",
      KheIntervalShow(in, days_frame), KheCombCoverTypesShow(cover_types));
  first_tg = KheFrameTimeGroup(days_frame, KheIntervalFirst(in));
  last_tg = KheFrameTimeGroup(days_frame, KheIntervalLast(in));
  res = 0;
  do
  {
    groups = KheDoCombGroupingForTimeGroup(cg, first_tg, in, days_frame,
      cover_types, sa, scratch_mtg);
    res += groups;
    if( groups == 0 && last_tg != first_tg )
    {
      groups = KheDoCombGroupingForTimeGroup(cg, last_tg, in, days_frame,
	cover_types, sa, scratch_mtg);
      res += groups;
    }
  } while( groups > 0 );

  if( DEBUG4 )
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

static int KheCombGrouping(KHE_COMB_GROUPER cg, KHE_OPTIONS options,
  KHE_SOLN_ADJUSTER sa, KHE_MTASK_GROUPER scratch_mtg)
{
  int max_days, len, first_index, days, total_groups;  KHE_MTASK_FINDER mtf;
  KHE_FRAME days_frame;
  KHE_EVENT_TIMETABLE_MONITOR etm;
  ARRAY_KHE_COMB_COVER_TYPE cover_types;

  /* get options */
  mtf = KheCombGrouperMTaskFinder(cg);
  max_days = KheOptionsGetInt(options, "rs_combinatorial_grouping_max_days", 3);
  days_frame = KheOptionsFrame(options, "gs_common_frame",
    KheMTaskFinderSoln(mtf));
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);

  /* eliminate combinations */
  if( DEBUG3 )
    fprintf(stderr, "  KheCombGrouping before KheElimCombSolve\n");
  HaArrayInit(cover_types, KheCombGrouperArena(cg));
  KheCombinationElimination(mtf, KheCombGrouperResourceType(cg),
    days_frame, etm, &cover_types);

  /* try each interval */
  total_groups = 0;
  days = KheFrameTimeGroupCount(days_frame);
  for( len = max_days;  len >= 2;  len-- )
  {
    if( DEBUG3 )
      fprintf(stderr, "  KheCombGrouping starting len %d\n", len);
    for( first_index = 0;  first_index <= days - len;  first_index++ )
      total_groups += KheDoCombGroupingForInterval(cg,
	KheIntervalMake(first_index, first_index + len - 1),
	days_frame, &cover_types, sa, scratch_mtg);
  }
  return total_groups;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheGroupByResourceConstraints"                                */
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
   prof ile_off KheTaskerGroupingBuild(tr, 2, "BCDT special");
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheBCDTSpecialCaseGrouping()\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,         */
/*    KHE_TASK_SET ts, bool history_off, bool comb_off, bool interval_off,   */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Carry out group by resource constraints for resource type rt.  There     */
/*  is no point in doing anything if there are fewer than two resources.     */
/*                                                                           */
/*****************************************************************************/

static int KheGroupForResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa, bool comb_off, bool interval_off,
  KHE_OPTIONS options, HA_ARENA a, KHE_MTASK_GROUPER scratch_mtg)
{
  KHE_MTASK_FINDER mtf;  KHE_COMB_GROUPER cg;  KHE_FRAME days_frame;  int res;

  res = 0;
  if( KheResourceTypeResourceCount(rt) > 1 && (!comb_off || !interval_off) )
  {
    /* make an mtask finder */
    days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
    mtf = KheMTaskFinderMake(soln, rt, days_frame, true, a);

    /* combinatorial grouping */
    if( !comb_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) comb grouping:\n",
	  KheResourceTypeId(rt));
      cg = KheCombGrouperMake(mtf, rt, a);
      res += KheCombGrouping(cg, options, sa, scratch_mtg);
    }

    /* interval grouping */
    /* *** moved to khe_sr_interval_grouping.c
    if( !interval_off )
    {
      if( DEBUG10 )
	fprintf(stderr, "  KheGroupForResourceType(%s) interval grouping:\n",
	  KheResourceTypeId(rt));
      res += KheintervalGrouping(mtf, rt, sa);
      ** res += KhePro fileGrouping(cg, sa); **
    }
    *** */
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,  */
/*    KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)                             */
/*                                                                           */
/*  Group the tasks of soln of type rt by resource constraints.  If rt is    */
/*  NULL, do this for each of the resource types of soln's instance.         */
/*                                                                           */
/*  If sa is non-NULL, record what was done in sa so that it can be undone   */
/*  later.                                                                   */
/*                                                                           */
/*****************************************************************************/
/* ***
bool KheGroupByResourceConstraints(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
*** */

int KheCombinatorialGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  int i, res;  KHE_INSTANCE ins;  HA_ARENA a;  KHE_MTASK_GROUPER scratch_mtg;

  /* boilerplate */
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByResourceConstraints(%s %s, -)\n",
      KheInstanceId(ins), rt == NULL ? "all types" : KheResourceTypeId(rt));

  /* return immediately if option rs_group_by_frame_off is true */
  /* ***
  if( KheOptionsGetBool(options, "rs_group_by_rc_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr,"] KheGroupByResourceConstraints returning false (off)\n");
    return false;
  }

  comb_off = KheOptionsGetBool(options, "rs_group_by_rc_combinatorial_off",
    false);
  interval_off=KheOptionsGetBool(options, "rs_group_by_rc_interval_off", false);
  *** */

  /* do grouping by resource constraints for each resource type */
  a = KheSolnArenaBegin(soln);
  scratch_mtg = KheMTaskGrouperMake(a);
  res = 0;
  if( rt != NULL )
    res += KheGroupForResourceType(soln, rt, sa, false, false, options, a,
      scratch_mtg);
  else for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    res += KheGroupForResourceType(soln, rt, sa, false, false, options, a,
      scratch_mtg);
  }
  KheSolnArenaEnd(soln, a);

  /* all done */
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByResourceConstraints returning %d\n", res);
  return res;
}

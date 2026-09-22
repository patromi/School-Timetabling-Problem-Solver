
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
/*  FILE:         khe_sr_cluster_minimum.c                                   */
/*  DESCRIPTION:  Cluster minimum limits                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0			/* supply                            */
#define DEBUG2 0			/* demand                            */
#define DEBUG3 0			/* build groups                      */
#define DEBUG4 0			/* increase limits                   */

/*****************************************************************************/
/*                                                                           */
/*  Type "KHE_CLUSTER_MINIMUM_GROUP"                                         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT;

struct khe_cluster_minimum_group_rec {
  int						offset;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT	constraints;
  /* ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR	monitors; */
  int						supply;
  int						demand;
};

typedef HA_ARRAY(KHE_CLUSTER_MINIMUM_GROUP) ARRAY_KHE_CLUSTER_MINIMUM_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  Type "KHE_CLUSTER_MINIMUM_SOLVER"                                        */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CLUSTER_BUSY_TIMES_MONITOR)
  ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR;

struct khe_cluster_minimum_solver_rec {
  HA_ARENA					arena;
  KHE_SOLN					soln;
  KHE_OPTIONS					options;
  KHE_RESOURCE_TYPE				resource_type;
  KHE_FRAME					common_frame;
  KHE_EVENT_TIMETABLE_MONITOR			etm;
  ARRAY_KHE_CLUSTER_MINIMUM_GROUP		groups;
  ARRAY_KHE_CLUSTER_MINIMUM_GROUP		free_groups;
  bool						adjusting_now;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_MONITOR		adjusted_monitors;
  HA_ARRAY_INT					adjusted_original_values;
  ARRAY_KHE_CLUSTER_BUSY_TIMES_CONSTRAINT	tmp_constraints;
  HA_ARRAY_INT					tmp_intersects;
  HA_ARRAY_INT					tmp_supplies;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply of and demand for resources"                           */
/*                                                                           */
/*****************************************************************************/

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
/*  int KheClusterLim(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                   */
/*                                                                           */
/*  Assuming that c has at least one time group and that its time groups     */
/*  are either all positive or all negative (these conditions are            */
/*  guaranteed by KheClusterConstraintSuitsSolver), return the limit of c.   */
/*                                                                           */
/*****************************************************************************/

static int KheClusterLim(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  KHE_POLARITY po;
  KheClusterBusyTimesConstraintTimeGroup(c, 0, 0, &po);
  return (po == KHE_POSITIVE ? KheClusterBusyTimesConstraintMaximum(c) :
    KheClusterBusyTimesConstraintTimeGroupCount(c) -
    KheClusterBusyTimesConstraintMinimum(c));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupGetSupply(KHE_CLUSTER_MINIMUM_GROUP cmg,       */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms)                                        */
/*                                                                           */
/*  Get cmg's supply.                                                        */
/*                                                                           */
/*****************************************************************************/

static int KheClusterMinimumGroupGetSupply(KHE_CLUSTER_MINIMUM_GROUP cmg,
  KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_TIME_GROUP c_tg, f_tg;
  int i, j, k, m, lim, count, fi, li, val, rcount, index, tg_count;
  int total_supply, cbtc_supply, unconstrained_supply;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_POLARITY po;

  cbtc = HaArrayFirst(cmg->constraints);
  if( DEBUG1 )
    fprintf(stderr, "[ KheClusterMinimumGroupGetSupply(%s ..., cms)\n",
      KheConstraintId((KHE_CONSTRAINT) cbtc));

  /* find how many frame time groups each constraint time group intersects */
  tg_count = KheClusterBusyTimesConstraintTimeGroupCount(cbtc);
  KheClusterFirstAndLastIndex(cbtc, cmg->offset, cms->common_frame, &fi, &li);
  HaArrayClear(cms->tmp_intersects);
  for( i = 0;  i < tg_count;  i++ )
  {
    c_tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, cmg->offset, &po);
    count = 0;
    for( j = fi;  j <= li;  j++ )
    {
      f_tg = KheFrameTimeGroup(cms->common_frame, j);
      if( !KheTimeGroupDisjoint(c_tg, f_tg) )
	count++;
    }
    HaArrayAddLast(cms->tmp_intersects, count);
  }
  HaArraySort(cms->tmp_intersects, &IntCmpIncreasing);

  /* find unconstrained_supply, the supply of an unconstrained resource */
  unconstrained_supply = 0;
  HaArrayForEach(cms->tmp_intersects, val, i)
    unconstrained_supply += val;

  /* initialize tmp_supplies to unconstrained_supply for each resource */
  HaArrayClear(cms->tmp_supplies);
  rcount = KheResourceTypeResourceCount(cms->resource_type);
  HaArrayFill(cms->tmp_supplies, rcount, unconstrained_supply);

  /* reduce tmp_supplies to the minimum allowed by the constraints */
  HaArrayForEach(cmg->constraints, cbtc, i)
  {
    lim = KheClusterLim(cbtc);
    HnAssert(lim < tg_count, "KheClusterMinimumGroupGetSupply internal error");
    cbtc_supply = 0;
    for( j = 1;  j <= lim;  j++ )
      cbtc_supply += HaArray(cms->tmp_intersects, tg_count - j);
    for( k = 0; k < KheClusterBusyTimesConstraintResourceGroupCount(cbtc); k++ )
    {
      rg = KheClusterBusyTimesConstraintResourceGroup(cbtc, k);
      if( KheResourceGroupResourceType(rg) == cms->resource_type )
	for( m = 0;  m < KheResourceGroupResourceCount(rg);  m++ )
	{
	  r = KheResourceGroupResource(rg, m);
	  index = KheResourceResourceTypeIndex(r);
	  if( HaArray(cms->tmp_supplies, index) > cbtc_supply )
	    HaArrayPut(cms->tmp_supplies, index, cbtc_supply);
	}
    }
    for( m = 0;  m < KheClusterBusyTimesConstraintResourceCount(cbtc);  m++ )
    {
      r = KheClusterBusyTimesConstraintResource(cbtc, m);
      if( KheResourceResourceType(r) == cms->resource_type )
      {
	index = KheResourceResourceTypeIndex(r);
	if( HaArray(cms->tmp_supplies, index) > cbtc_supply )
	  HaArrayPut(cms->tmp_supplies, index, cbtc_supply);
      }
    }
  }

  /* add up tmp_supplies for all resources to get the total supply */
  total_supply = 0;
  HaArrayForEach(cms->tmp_supplies, val, index)
    total_supply += val;
  if( DEBUG1 )
  {
    HaArrayForEach(cms->tmp_supplies, val, index)
    {
      r = KheResourceTypeResource(cms->resource_type, index);
      fprintf(stderr, "  supply of %s: %d\n", KheResourceId(r), val);
    }
    fprintf(stderr, "] KheClusterMinimumGroupGetSupply returning %d\n",
      total_supply);
  }
  return total_supply;
}


/* *** old version, a bit inexact
static int KheClusterMinimumGroupGetSupply(KHE_CLUSTER_MINIMUM_GROUP cmg,
  KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_TIME_GROUP c_tg, f_tg;
  int i, j, lim, count, sum, fi, li, res, rcount, tmp;  KHE_POLARITY po;

  ** find how many frame time groups each constraint time group intersects **
  cbtc = HaArrayFirst(cmg->constraints);
  KheClusterFirstAndLastIndex(cbtc, cmg->offset, cms->common_frame, &fi, &li);
  HaArrayClear(cms->tmp_intersects);
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(cbtc);  i++ )
  {
    c_tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, cmg->offset, &po);
    count = 0;
    for( j = fi;  j <= li;  j++ )
    {
      f_tg = KheFrameTimeGroup(cms->common_frame, j);
      if( !KheTimeGroupDisjoint(c_tg, f_tg) )
	count++;
    }
    HaArrayAddLast(cms->tmp_intersects, count);
  }
  HaArraySort(cms->tmp_intersects, &IntCmpIncreasing);

  ** get the supply:  the largest lim intersect counts, times resources **
  rcount = 0;
  res = 0;
  count = HaArrayCount(cms->tmp_intersects);  ** also number of time groups **
  HaArrayForEach(cmg->constraints, cbtc, i)
  {
    lim = KheClusterBusyTimesConstraintMaximum(cbtc);
    HnAssert(lim < count, "KheClusterMinimumGroupSolve internal error");
    sum = 0;
    for( j = 1;  j <= lim;  j++ )
      sum += HaArray(cms->tmp_intersects, count - j);
    tmp = KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc,
	cms->resource_type);
    rcount += tmp;
    res += sum * tmp;
    ** NB it is safe to use KheClusterBusyTimesConstraintResourceOfTypeCount
       here, because c is known to contain no duplicate resources, thanks to
       KheConstaintSetCoversAllResources **
    ** look at this again, still to do **
    ** really need a specific limit for each resource in cms->resource_type **
    ** initialize to other sum below, then min for each resource in each c **
  }

  ** other resources not covered by constraints **
  sum = 0;
  HaArrayForEach(cms->tmp_intersects, tmp, i)
    sum += tmp;
  res += sum * (KheResourceTypeResourceCount(cms->resource_type) - rcount);
  return res;
}
*** */


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
  int i, j, res;  KHE_MEET meet;  KHE_TASK task;  KHE_RESOURCE r;
  KHE_EVENT_RESOURCE er;
  res = 0;
  for( i = 0;  i < KheEventTimetableMonitorTimeMeetCount(etm, t);  i++ )
  {
    meet = KheEventTimetableMonitorTimeMeet(etm, t, i);
    for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
    {
      task = KheMeetTask(meet, j);
      if( KheTaskResourceType(task) == rt )
      {
	if( KheTaskIsPreassigned(task, &r) )
	  res++;
	else
	{
	  er = KheTaskEventResource(task);
	  if( er != NULL && KheEventResourceNeedsAssignment(er) == KHE_YES )
	    res++;
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
/*  int KheClusterMinimumGroupGetDemand(KHE_CLUSTER_MINIMUM_GROUP cmg,       */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms)                                        */
/*                                                                           */
/*  Get cmg's demand.                                                        */
/*                                                                           */
/*****************************************************************************/

static int KheClusterMinimumGroupGetDemand(KHE_CLUSTER_MINIMUM_GROUP cmg,
  KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  int i, res, val;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  cbtc = HaArrayFirst(cmg->constraints);
  if( DEBUG2 )
    fprintf(stderr, "[ KheClusterMinimumGroupGetDemand(%s ..., cms)\n",
      KheConstraintId((KHE_CONSTRAINT) cbtc));
  res = 0;
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(cbtc);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(cbtc, i, cmg->offset, &po);
    val = KheDemandAtTimeGroup(cms->etm, cms->resource_type, tg);
    if( DEBUG2 )
      fprintf(stderr, "  demand at %s: %d\n", KheTimeGroupId(tg), val);
    res += val;
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheClusterMinimumGroupGetDemand returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "building groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheClusterTypedCmp(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c1,             */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c2)                                  */
/*                                                                           */
/*  Comparison function for sorting an array of cluster busy times           */
/*  constraints so that constraints with equal applies-to time groups        */
/*  and equal time groups are adjacent.                                      */
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

  /* time groups must be equal; no need to check polarity (always positive) */
  count1 = KheClusterBusyTimesConstraintTimeGroupCount(c1);
  count2 = KheClusterBusyTimesConstraintTimeGroupCount(c2);
  if( count1 != count2 )
    return count1 - count2;
  for( i = 0;  i < count1;  i++ )
  {
    tg1 = KheClusterBusyTimesConstraintTimeGroup(c1, i, 0, &po1);
    tg2 = KheClusterBusyTimesConstraintTimeGroup(c2, i, 0, &po2);
    /* ***
    if( po1 != po2 )
      return (int) po1 - (int) po2;
    *** */
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 ) return cmp;
  }

  /* everything we care about is equal */
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
/*  bool KheClusterConstraintSuitsSolver(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return true if c suits cluster minimum solving.                          */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterConstraintSuitsSolver(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int tg_count;
  tg_count = KheClusterBusyTimesConstraintTimeGroupCount(c);
  if( KheConstraintCombinedWeight((KHE_CONSTRAINT) c) <= 0 )
    return false;
  else if( tg_count == 0 )
    return false;
  else if( !KheClusterBusyTimesConstraintTimeGroupsDisjoint(c) )
    return false;
  else if( KheClusterBusyTimesConstraintAllPositive(c) )
    return KheClusterBusyTimesConstraintMaximum(c) < tg_count;
  else if( KheClusterBusyTimesConstraintAllNegative(c) )
    return KheClusterBusyTimesConstraintMinimum(c) > 0;
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_MINIMUM_GROUP KheClusterMinimumGroupMake(                    */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms, int offset)                            */
/*                                                                           */
/*  Make a new, empty cluster minimum group with these attributes.           */
/*                                                                           */
/*****************************************************************************/

static KHE_CLUSTER_MINIMUM_GROUP KheClusterMinimumGroupMake(
  KHE_CLUSTER_MINIMUM_SOLVER cms, int offset)
{
  KHE_CLUSTER_MINIMUM_GROUP res;
  if( HaArrayCount(cms->free_groups) > 0 )
  {
    res = HaArrayLastAndDelete(cms->free_groups);
    HaArrayClear(res->constraints);
  }
  else
  {
    HaMake(res, cms->arena);
    HaArrayInit(res->constraints, cms->arena);
  }
  res->offset = offset;
  /* HaArrayInit(res->monitors, cms->arena); */
  res->supply = res->demand = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverBuildGroups(KHE_CLUSTER_MINIMUM_SOLVER cms)  */
/*                                                                           */
/*  Build cms's groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheClusterMinimumSolverBuildGroups(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  KHE_INSTANCE ins;  int i, j, k, pos, offset_count, offset;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, cbtc2;
  KHE_CLUSTER_MINIMUM_GROUP cmg;
  if( DEBUG3 )
    fprintf(stderr, "[ KheClusterMinimumSolverBuildGroups(cms)\n");

  /* add the constraints to cms->tmp_constraints and sort them */
  HaArrayClear(cms->tmp_constraints);
  ins = KheSolnInstance(cms->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( KheClusterConstraintSuitsSolver(cbtc) )
	HaArrayAddLast(cms->tmp_constraints, cbtc);
    }
  }
  HaArraySort(cms->tmp_constraints, &KheClusterCmp);

  /* find groups of equal constraints */
  HaArrayClear(cms->groups);
  for( i = 0;  i < HaArrayCount(cms->tmp_constraints);  i += j )
  {
    cbtc = HaArray(cms->tmp_constraints, i);
    for( j = i + 1;  j < HaArrayCount(cms->tmp_constraints);  j++ )
    {
      cbtc2 = HaArray(cms->tmp_constraints, j);
      if( KheClusterTypedCmp(cbtc, cbtc2) != 0 )
	break;
    }

    /* at this point, cms->tmp_constraints[i .. j-1] is one group */
    cbtc = HaArray(cms->tmp_constraints, i);
    offset_count = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc);
    for( k = 0;  k < offset_count;  k++ )
    {
      offset = KheClusterBusyTimesConstraintAppliesToOffset(cbtc, k);
      cmg = KheClusterMinimumGroupMake(cms, offset);
      for( pos = i;  pos < j;  pos++ )
	HaArrayAddLast(cmg->constraints, HaArray(cms->tmp_constraints, pos));
      cmg->supply = KheClusterMinimumGroupGetSupply(cmg, cms);
      cmg->demand = KheClusterMinimumGroupGetDemand(cmg, cms);
      if( DEBUG3 )
	KheClusterMinimumGroupDebug(cmg, 2, 2, stderr);
      HaArrayAddLast(cms->groups, cmg);
    }
  }
  HaArrayClear(cms->tmp_constraints);  /* these were always only temporary */
  HaArrayClear(cms->tmp_intersects);   /* these were always only temporary */
  if( DEBUG3 )
    fprintf(stderr, "] KheClusterMinimumSolverBuildGroups returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_MINIMUM_SOLVER KheClusterMinimumSolverMake(KHE_SOLN soln,    */
/*    KHE_OPTIONS options, KHE_RESOURCE_TYPE rt, HA_ARENA a)                 */
/*                                                                           */
/*  Make a new cluster minimum solver with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_MINIMUM_SOLVER KheClusterMinimumSolverMake(HA_ARENA a)
{
  KHE_CLUSTER_MINIMUM_SOLVER res;

  /* build the basic object */
  HaMake(res, a);
  res->arena = a;
  res->soln = NULL;
  res->options = NULL;
  res->resource_type = NULL;
  res->common_frame = NULL;
  res->etm = NULL;
  HaArrayInit(res->groups, a);
  HaArrayInit(res->free_groups, a);
  res->adjusting_now = false;
  HaArrayInit(res->adjusted_monitors, a);
  HaArrayInit(res->adjusted_original_values, a);
  HaArrayInit(res->tmp_constraints, a);
  HaArrayInit(res->tmp_intersects, a);
  HaArrayInit(res->tmp_supplies, a);

  /* all good, return now */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverSolve(KHE_CLUSTER_MINIMUM_SOLVER cms,        */
/*    KHE_SOLN soln, KHE_OPTIONS options, KHE_RESOURCE_TYPE rt)              */
/*                                                                           */
/*  Solve for resources of type rt.                                          */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverSolve(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_SOLN soln, KHE_OPTIONS options, KHE_RESOURCE_TYPE rt)
{
  int i;

  /* set up the solver with these attributes */
  cms->soln = soln;
  cms->options = options;
  cms->resource_type = rt;
  cms->common_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  cms->etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  HaArrayAppend(cms->free_groups, cms->groups, i);
  HaArrayClear(cms->groups);

  /* build its groups */
  KheClusterMinimumSolverBuildGroups(cms);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheClusterMinimumSolverSoln(KHE_CLUSTER_MINIMUM_SOLVER cms)     */
/*                                                                           */
/*  Return the soln of cms's previous solve, or NULL if there has been no    */
/*  previous solve.                                                          */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheClusterMinimumSolverSoln(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  return cms->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS KheClusterMinimumSolverOptions(                              */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms)                                        */
/*                                                                           */
/*  Return the options attribute of cms's previous solve, or NULL if there   */
/*  has been no previous solve.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_OPTIONS KheClusterMinimumSolverOptions(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  return cms->options;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheClusterMinimumSolverResourceType(                   */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms)                                        */
/*                                                                           */
/*  Return the resource_type attribute of cms's previous solve, or NULL if   */
/*  there has been no previous solve.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheClusterMinimumSolverResourceType(
  KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  return cms->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverDelete(KHE_CLUSTER_MINIMUM_SOLVER cms)       */
/*                                                                           */
/*  Delete cms.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheClusterMinimumSolverDelete(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  KheSolnArenaEnd(cms->soln, cms->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumSolverGroupCount(KHE_CLUSTER_MINIMUM_SOLVER cms)    */
/*                                                                           */
/*  Return the number of constraint groups in cms.                           */
/*                                                                           */
/*****************************************************************************/

int KheClusterMinimumSolverGroupCount(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  return HaArrayCount(cms->groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_MINIMUM_GROUP KheClusterMinimumSolverGroup(                  */
/*    KHE_CLUSTER_MINIMUM_SOLVER cms, int i)                                 */
/*                                                                           */
/*  Return the i'th constraint group of cms.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_MINIMUM_GROUP KheClusterMinimumSolverGroup(
  KHE_CLUSTER_MINIMUM_SOLVER cms, int i)
{
  return HaArray(cms->groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupConstraintCount(KHE_CLUSTER_MINIMUM_GROUP cmg) */
/*                                                                           */
/*  Return the number of constraints in cmg.                                 */
/*                                                                           */
/*****************************************************************************/

int KheClusterMinimumGroupConstraintCount(KHE_CLUSTER_MINIMUM_GROUP cmg)
{
  return HaArrayCount(cmg->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterMinimumGroupConstraint(      */
/*    KHE_CLUSTER_MINIMUM_GROUP cmg, int i)                                  */
/*                                                                           */
/*  Return the i'th constraint of cmg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_CLUSTER_BUSY_TIMES_CONSTRAINT KheClusterMinimumGroupConstraint(
  KHE_CLUSTER_MINIMUM_GROUP cmg, int i)
{
  return HaArray(cmg->constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupConstraintOffset(KHE_CLUSTER_MINIMUM_GROUP cmg)*/
/*                                                                           */
/*  Return the constraint offset of cmg.                                     */
/*                                                                           */
/*****************************************************************************/

int KheClusterMinimumGroupConstraintOffset(KHE_CLUSTER_MINIMUM_GROUP cmg)
{
  return cmg->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupSupply(KHE_CLUSTER_MINIMUM_GROUP cmg)          */
/*                                                                           */
/*  Return the supply of cmg (that resources can supply during its times).   */
/*                                                                           */
/*****************************************************************************/

int KheClusterMinimumGroupSupply(KHE_CLUSTER_MINIMUM_GROUP cmg)
{
  return cmg->supply;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupDemand(KHE_CLUSTER_MINIMUM_GROUP cmg)          */
/*                                                                           */
/*  Return the demand of cmg (the number of tasks running during its times). */
/*                                                                           */
/*****************************************************************************/

int KheClusterMinimumGroupDemand(KHE_CLUSTER_MINIMUM_GROUP cmg)
{
  return cmg->demand;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterMinimumGroupMonitorCount(KHE_CLUSTER_MINIMUM_GROUP cmg)    */
/*                                                                           */
/*  Return the number of monitors associated with cmg.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheClusterMinimumGroupMonitorCount(KHE_CLUSTER_MINIMUM_GROUP cmg)
{
  return HaArrayCount(cmg->monitors);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterMinimumGroupMonitor(            */
/*    KHE_CLUSTER_MINIMUM_GROUP cmg, int i)                                  */
/*                                                                           */
/*  Return the i'th monitor associated with cmg.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_CLUSTER_BUSY_TIMES_MONITOR KheClusterMinimumGroupMonitor(
  KHE_CLUSTER_MINIMUM_GROUP cmg, int i)
{
  return HaArray(cmg->monitors, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterMinimumSolverMonitorGroup(KHE_CLUSTER_MINIMUM_SOLVER cms, */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_CLUSTER_MINIMUM_GROUP *cmg)   */
/*                                                                           */
/*  Retrieve the group containing cbtm, return true if successful.           */
/*                                                                           */
/*****************************************************************************/

bool KheClusterMinimumSolverMonitorGroup(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_CLUSTER_MINIMUM_GROUP *cmg)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  int offset, i, pos;
  KHE_CLUSTER_MINIMUM_GROUP g;
  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
  offset = KheClusterBusyTimesMonitorOffset(cbtm);
  HaArrayForEach(cms->groups, g, i)
    if( g->offset == offset && HaArrayContains(g->constraints, cbtc, &pos) )
      return *cmg = g, true;
  return *cmg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumGroupDebug(KHE_CLUSTER_MINIMUM_GROUP cmg,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of cmg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumGroupDebug(KHE_CLUSTER_MINIMUM_GROUP cmg,
  int verbosity, int indent, FILE *fp)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  int i;
  /* KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm; */
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ClusterMinimumGroup (offset %d, supply %d, demand %d)\n",
      indent, "", cmg->offset, cmg->supply, cmg->demand);
    HaArrayForEach(cmg->constraints, cbtc, i)
      if( verbosity >= 3 )
	KheClusterBusyTimesConstraintDebug(cbtc, verbosity, indent + 2, fp);
      else
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheConstraintId((KHE_CONSTRAINT) cbtc));
    /* ***
    if( verbosity >= 3 )
      HaArrayForEach(cmg->monitors, cbtm, i)
	KheClusterBusyTimesMonitorDebug(cbtm, verbosity, indent + 2, fp);
    *** */
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    cbtc = HaArrayFirst(cmg->constraints);
    fprintf(fp, "ClusterMinimumGroup(%s.. offset %d, supply %d, demand %d)\n",
      KheConstraintId((KHE_CONSTRAINT) cbtc), cmg->offset, cmg->supply,
      cmg->demand);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverSetBegin(KHE_CLUSTER_MINIMUM_SOLVER cms)     */
/*                                                                           */
/*  Begin a run of monitor setting.                                          */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverSetBegin(KHE_CLUSTER_MINIMUM_SOLVER cms)
{
  HnAssert(!cms->adjusting_now,
    "KheClusterMinimumSolverSetBegin called out of order (already adjusting)");
  HaArrayClear(cms->adjusted_monitors);
  HaArrayClear(cms->adjusted_original_values);
  cms->adjusting_now = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverSet(KHE_CLUSTER_MINIMUM_SOLVER cms,          */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)                             */
/*                                                                           */
/*  Set m's minimum limit to val and remember to have done it.               */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverSet(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_CLUSTER_BUSY_TIMES_MONITOR m, int val)
{
  HnAssert(cms->adjusting_now,
    "KheClusterMinimumSolverSet called out of order (not adjusting)");
  HaArrayAddLast(cms->adjusted_monitors, m);
  HaArrayAddLast(cms->adjusted_original_values,
    KheClusterBusyTimesMonitorMinimum(m));
  KheClusterBusyTimesMonitorSetMinimum(m, val);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverSetEnd(KHE_CLUSTER_MINIMUM_SOLVER cms,       */
/*    bool undo)                                                             */
/*                                                                           */
/*  End a run of monitor setting, possibly undoing what was done.            */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverSetEnd(KHE_CLUSTER_MINIMUM_SOLVER cms, bool undo)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR m;  int val, i;
  HnAssert(cms->adjusting_now,
    "KheClusterMinimumSolverSetEnd called out of order (not adjusting)");
  if( undo )
    for( i = HaArrayCount(cms->adjusted_monitors) - 1;  i >= 0;  i-- )
    {
      m = HaArray(cms->adjusted_monitors, i);
      val = HaArray(cms->adjusted_original_values, i);
      KheClusterBusyTimesMonitorSetMinimum(m, val);
    }
  HaArrayClear(cms->adjusted_monitors);
  HaArrayClear(cms->adjusted_original_values);
  cms->adjusting_now = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverSetMulti(KHE_CLUSTER_MINIMUM_SOLVER cms,     */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  For each cluster busy times monitor m for a resource of rg which is      */
/*  related to a group of cms whose demand is at least as large as its       */
/*  supply, use KheClusterMinimumSolverSet to increase m's minimum limit     */
/*  to its maximum limit.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverSetMulti(KHE_CLUSTER_MINIMUM_SOLVER cms,
  KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE r;  int i, j;  KHE_CLUSTER_MINIMUM_GROUP g;
  KHE_MONITOR m;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  if( DEBUG4 )
    fprintf(stderr, "[ KheClusterMinimumSolverSetMulti(cms, %s)\n",
      KheResourceGroupId(rg));
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(cms->soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(cms->soln, r, j);
      if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
      {
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	if( KheClusterMinimumSolverMonitorGroup(cms, cbtm, &g) &&
	    KheClusterMinimumGroupDemand(g) >= KheClusterMinimumGroupSupply(g) )
	{
	  if( DEBUG4 )
	  {
	    fprintf(stderr, "    monitor %s min %d -> %d based on group:\n",
	      KheMonitorId(m), KheClusterBusyTimesMonitorMinimum(cbtm),
	      KheClusterBusyTimesMonitorMaximum(cbtm));
	    KheClusterMinimumGroupDebug(g, 2, 4, stderr);
	  }
	  KheClusterMinimumSolverSet(cms, cbtm,
	    KheClusterBusyTimesMonitorMaximum(cbtm));
	}
      }
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "] KheClusterMinimumSolverSetMulti returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterMinimumSolverDebug(KHE_CLUSTER_MINIMUM_SOLVER cms,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of cms onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheClusterMinimumSolverDebug(KHE_CLUSTER_MINIMUM_SOLVER cms,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_CLUSTER_MINIMUM_GROUP cmg;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Cluster Minimum Solver (soln of %s, %s)\n", indent, "",
      cms->soln == NULL ? "-" : KheInstanceId(KheSolnInstance(cms->soln)),
      cms->resource_type == NULL ? "-" : KheResourceTypeId(cms->resource_type));
    HaArrayForEach(cms->groups, cmg, i)
      KheClusterMinimumGroupDebug(cmg, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}

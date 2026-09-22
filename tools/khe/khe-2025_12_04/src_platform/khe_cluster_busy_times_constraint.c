
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
/*  FILE:         khe_cluster_busy_times_constraint.c                        */
/*  DESCRIPTION:  A cluster busy times constraint                            */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT - a cluster busy times constraint      */
/*                                                                           */
/*****************************************************************************/

struct khe_cluster_busy_times_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  int				history_before;		/* history: ai       */
  int				history_after;		/* history: ci       */
  HA_ARRAY_INT			history;		/* history: xi       */
  KHE_TIME_GROUP		applies_to_tg;		/* applies to tg     */
  HA_ARRAY_INT			applies_to_offsets;	/* applies_to offsets*/
  int				minimum;		/* minimum           */
  int				maximum;		/* maximum           */
  bool				allow_zero;		/* allow zero        */
  /* bool			limit_busy_recode; */	/* recodes limit busy*/
  /* bool			limits_whole_cycle; */	/* limits whole cycle*/
  ARRAY_KHE_TIME_GROUP		time_groups;		/* time groups       */
  int				min_time_index;		/* min time index    */
  int				max_time_index;		/* min time index    */
  ARRAY_KHE_POLARITY		polarities;		/* polarities        */
  bool				all_positive;		/* if all positive   */
  bool				all_negative;		/* if all negative   */
  bool				time_groups_disjoint;	/* if disjoint       */
  bool				time_groups_cover_cycle; /* if cover cycle   */
  HA_ARRAY_INT			resource_of_type_count;  /* resource of type */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintMakeInternal(KHE_INSTANCE ins,         */
/*    char *id, char *name, bool required, int weight, KHE_COST_FUNCTION cf, */
/*    KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,                */
/*    bool allow_zero, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *c)                 */
/*                                                                           */
/*  Make a new cluster busy times constraint with these attributes, add it   */
/*  to ins, and return it.  This is the internal form that does not require  */
/*  the instance to be unfinalized.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintMakeInternal(KHE_INSTANCE ins,
  char *id, char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *c)
{
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  int i, count;
  if( KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
  {
    HnAssert(applies_to_tg == NULL, "KheClusterBusyTimesConstraintMake: "
      "applies_to_tg != NULL in high school model");
    HnAssert(!allow_zero, "KheClusterBusyTimesConstraintMake: "
      "allow_zero in high school model");
  }
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  count = KheInstanceResourceTypeCount(ins);
  /* ***
  res = HaAlloc(a, sizeof(struct khe_cluster_busy_times_constraint_rec)
    + (count - 1) * sizeof(int));
  *** */
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG, ins, id, name,required,weight,cf,a);
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->history_before = 0;  /* now the default value */
  res->history_after = 0;  /* now the default value */
  HaArrayInit(res->history, a);
  res->applies_to_tg = applies_to_tg;
  HaArrayInit(res->applies_to_offsets, a);
  HaArrayAddLast(res->applies_to_offsets, -1);  /* signals not initialized */
  res->minimum = minimum;
  res->maximum = maximum;
  res->allow_zero = allow_zero;
  /* res->limits_whole_cycle = false; */ /* placeholder until finalized */
  HaArrayInit(res->time_groups, a);
  res->min_time_index = -1;
  res->max_time_index = -1;
  HaArrayInit(res->polarities, a);
  res->all_positive = true;
  res->all_negative = true;
  res->time_groups_disjoint = false;
  res->time_groups_cover_cycle = false;
  HaArrayInit(res->resource_of_type_count, a);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->resource_of_type_count, 0);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,       */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,                */
/*    bool allow_zero, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *c)                 */
/*                                                                           */
/*  Make a new cluster busy times constraint with these attributes, add it   */
/*  to ins, and return it.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT *c)
{
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheClusterBusyTimesConstraintMake called after KheInstanceMakeEnd");
  return KheClusterBusyTimesConstraintMakeInternal(ins, id, name, required,
    weight, cf, applies_to_tg, minimum, maximum, allow_zero, c);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheClusterBusyTimesConstraintAppliesToTimeGroup(          */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the applies-to time group attribute of c (it may be NULL).        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheClusterBusyTimesConstraintAppliesToTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->applies_to_tg;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintMinimum(                                */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintMinimum(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintMaximum(                                */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintMaximum(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintAllowZero(                             */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the allow_zero attribute of c.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintAllowZero(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintLimitBusyRecode(                       */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if c is a recoded limit busy times constraint.               */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheClusterBusyTimesConstraintLimitBusyRecode(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->limit_busy_recode;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintAppliesToTimes(                         */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the number of applies-to offsets of c.                            */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintAppliesToOffsetCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  if( HaArrayCount(c->applies_to_offsets) == 1 &&
      HaArrayFirst(c->applies_to_offsets) == -1 )
    HnAbort("KheClusterBusyTimesConstraintAppliesToOffsetCount"
      " called before KheInstanceMakeEnd");
  return HaArrayCount(c->applies_to_offsets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintAppliesToOffset(                        */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)                            */
/*                                                                           */
/*  Return the i'th applies-to offset of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintAppliesToOffset(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->applies_to_offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesResourceCount(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the number of resources that c applies to, including resources    */
/*  in resource groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static int KheClusterBusyTimesResourceCount(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintAppliesToCount(                         */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintAppliesToCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return KheClusterBusyTimesResourceCount(c) *
    KheClusterBusyTimesConstraintAppliesToOffsetCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupsCoverTime(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,         */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Return true if t lies in any of c's time groups.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupsCoverTime(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  KHE_TIME t)
{
  KHE_TIME_GROUP tg;  int i, pos;
  HaArrayForEach(c->time_groups, tg, i)
    if( KheTimeGroupContains(tg, t, &pos) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintResetResourceOfType(                   */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Reset the resource_of_type attributes of c.  This is only done when      */
/*  resource type partitioning is happening.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddToResourceOfType(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArrayFill(c->resource_of_type_count, index + 1, 0);
  HaArray(c->resource_of_type_count, index) += 1;
}

void KheClusterBusyTimesConstraintResetResourceOfType(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayClear(c->resource_of_type_count);
  HaArrayForEach(c->resource_groups, rg, i)
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      KheAddToResourceOfType(c, KheResourceGroupResource(rg, j));
  HaArrayForEach(c->resources, r, i)
    KheAddToResourceOfType(c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintFinalize(                              */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintFinalize(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  int base_index, i, j, time_count, count, index;
  KHE_TIME_GROUP tg1, tg2;  /* SSET times_covered; */  KHE_TIME time;

  /* finalize c->applies_to_offsets */
  HaArrayClear(c->applies_to_offsets);
  if( c->applies_to_tg == NULL )
    HaArrayAddLast(c->applies_to_offsets, 0);
  else if( KheTimeGroupTimeCount(c->applies_to_tg) >= 1 )
  {
    /* add legal offsets to applies_to_offsets */
    time_count = KheInstanceTimeCount(c->instance);
    base_index = KheTimeIndex(KheTimeGroupTime(c->applies_to_tg, 0));
    for( i = 0;  i < KheTimeGroupTimeCount(c->applies_to_tg);  i++ )
    {
      index = KheTimeIndex(KheTimeGroupTime(c->applies_to_tg, i));
      if( c->max_time_index + (index - base_index) >= time_count )
	break;
      HaArrayAddLast(c->applies_to_offsets, index - base_index);
    }
  }

  /* work out whether the time groups are disjoint */
  c->time_groups_disjoint = true;
  HaArrayForEach(c->time_groups, tg1, i)
  {
    for( j = i + 1;  j < HaArrayCount(c->time_groups);  j++ )
    {
      tg2 = HaArray(c->time_groups, j);
      if( !KheTimeGroupDisjoint(tg1, tg2) )
      {
        c->time_groups_disjoint = false;
	goto CONTINUE;
      }
    }
  }
  CONTINUE:

  /* work out whether the time groups cover the whole cycle */
  if( c->time_groups_disjoint )
  {
    count = 0;
    HaArrayForEach(c->time_groups, tg1, i)
      count += KheTimeGroupTimeCount(tg1);
    c->time_groups_cover_cycle = (count == KheInstanceTimeCount(c->instance));
  }
  else
  {
    c->time_groups_cover_cycle = true;
    for( i = 0;  i < KheInstanceTimeCount(c->instance);  i++ )
    {
      time = KheInstanceTime(c->instance, i);
      if( !KheTimeGroupsCoverTime(c, time) )
      {
	c->time_groups_cover_cycle = false;
	break;
      }
    }
  }

  /* finalize c->limits_whole_cycle, but only try if no applies_to_tg */
  /* ***
  if( c->applies_to_tg == NULL )
  {
    SSetInit(times_covered);
    HaArrayForEach(c->time_groups, tg, i)
      SSetUnion(times_covered, *KheTimeGroupTimeSet(tg));
    c->limits_whole_cycle =
      (SSetCount(times_covered) == KheInstanceTimeCount(c->instance));
    SSetFree(times_covered);
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintDensityCount(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintDensityCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return KheClusterBusyTimesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddResourceGroup(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)            */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddResourceGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  int i, rt_index;  KHE_RESOURCE r;
  HaArrayAddLast(c->resource_groups, rg);
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
  }
  rt_index = KheResourceTypeIndex(KheResourceGroupResourceType(rg));
  HaArray(c->resource_of_type_count, rt_index)
    += KheResourceGroupResourceCount(rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintResourceGroupCount(                     */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintResourceGroupCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheClusterBusyTimesConstraintResourceGroup(           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)                            */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheClusterBusyTimesConstraintResourceGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->resource_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resources"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddResource(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)                   */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddResource(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  int rt_index;
  HaArrayAddLast(c->resources, r);
  rt_index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArray(c->resource_of_type_count, rt_index) += 1;
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintResourceCount(                          */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintResourceCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheClusterBusyTimesConstraintResource(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)                            */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheClusterBusyTimesConstraintResource(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintResourceOfTypeCount(                    */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)             */
/*                                                                           */
/*  Return the number of resources in c of type rt.                          */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintResourceOfTypeCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)
{
  return HaArray(c->resource_of_type_count, KheResourceTypeIndex(rt));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource history"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddHistoryBefore(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val)                          */
/*                                                                           */
/*  Add a history before value to c.                                         */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddHistoryBefore(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val)
{
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheClusterBusyTimesConstraintAddHistoryBefore: wrong model");
  HnAssert(c->history_before == 0,
    "KheClusterBusyTimesConstraintAddHistoryBefore called twice");
  HnAssert(val >= 0, "KheClusterBusyTimesConstraintAddHistoryBefore: val < 0");
  c->history_before = val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintHistoryBefore(                          */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Retrieve a history before value from c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintHistoryBefore(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->history_before;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddHistoryAfter(                       */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val)                          */
/*                                                                           */
/*  Add a history after value to c.                                          */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddHistoryAfter(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int val)
{
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheClusterBusyTimesConstraintAddHistoryAfter: wrong model");
  HnAssert(c->history_after == 0,
    "KheClusterBusyTimesConstraintAddHistoryAfter called twice");
  HnAssert(val >= 0, "KheClusterBusyTimesConstraintAddHistoryAfter: val < 0");
  c->history_after = val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintHistoryAfter(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Retrieve a history after value from c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintHistoryAfter(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->history_after;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddHistory(                            */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r, int val)          */
/*                                                                           */
/*  Add a history value to c.                                                */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddHistory(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r, int val)
{
  int index;
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheClusterBusyTimesConstraintAddHistory: wrong model");
  HnAssert(val >= 0, "KheClusterBusyTimesConstraintAddHistory: val < 0");
  index = KheResourceInstanceIndex(r);
  HaArrayFill(c->history, index + 1, 0);
  HnAssert(HaArray(c->history, index) == 0,
    "KheClusterBusyTimesConstraintAddHistory: history added twice");
  HaArrayPut(c->history, index, val);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintHistory(                                */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)                   */
/*                                                                           */
/*  Retrieve a history value from c.                                         */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintHistory(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  int index;
  index = KheResourceInstanceIndex(r);
  return (index < HaArrayCount(c->history) ? HaArray(c->history, index) : 0);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintAddTimeGroup(                          */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg,KHE_POLARITY po)*/
/*                                                                           */
/*  Add tg with polarity po to c.                                            */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintAddTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg, KHE_POLARITY po)
{
  int first_time_index, last_time_index, count;
  HnAssert(tg != NULL, "KheClusterBusyTimesConstraintAddTimeGroup: tg is NULL");
  if( KheInstanceModel(c->instance) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
    HnAssert(po == KHE_POSITIVE, "KheClusterBusyTimesConstraintAddTimeGroup:"
     " KHE_NEGATIVE in high school model");
  HaArrayAddLast(c->time_groups, tg);
  HaArrayAddLast(c->polarities, po);
  if( po == KHE_POSITIVE )
    c->all_negative = false;
  else
    c->all_positive = false;
  if( KheTimeGroupTimeCount(tg) > 0 )
  {
    count = KheTimeGroupTimeCount(tg);
    first_time_index = KheTimeIndex(KheTimeGroupTime(tg, 0));
    last_time_index = KheTimeIndex(KheTimeGroupTime(tg, count - 1));
    if( c->min_time_index == -1 || first_time_index < c->min_time_index )
      c->min_time_index = first_time_index;
    if( c->max_time_index == -1 || last_time_index > c->max_time_index )
      c->max_time_index = last_time_index;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheClusterBusyTimesConstraintTimeGroupCount(                         */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheClusterBusyTimesConstraintTimeGroupCount(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheClusterBusyTimesConstraintTimeGroup(                   */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i)                            */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheClusterBusyTimesConstraintTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int i, int offset, KHE_POLARITY *po)
{
  *po = HaArray(c->polarities, i);
  return KheTimeGroupNeighbour(HaArray(c->time_groups, i), offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintAllPositive(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if all time groups are positive.                             */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintAllPositive(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->all_positive;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintAllNegative(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if all time groups are negative.                             */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintAllNegative(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->all_negative;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintTimeGroupsDisjoint(                    */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if the time groups of c are pairwise disjoint.               */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintTimeGroupsDisjoint(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->time_groups_disjoint;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintTimeGroupsCoverWholeCycle(             */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintTimeGroupsCoverWholeCycle(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->time_groups_cover_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintRange(                                 */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int offset,                       */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the smallest and largest times monitored by c at offset.  Here    */
/*  offset must be a legal offset.                                           */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintRange(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, int offset,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int first_time_index, last_time_index;
  if( c->min_time_index == -1 )
  {
    *first_time = *last_time = NULL;
    return false;
  }
  else
  {
    first_time_index = c->min_time_index + offset;
    last_time_index = c->max_time_index + offset;
    HnAssert(first_time_index >= 0,
      "KheClusterBusyTimesConstraintRange: invalid offset (%d)", offset);
    HnAssert(last_time_index < KheInstanceTimeCount(c->instance),
      "KheClusterBusyTimesConstraintRange: invalid offset (%d)", offset);
    *first_time = KheInstanceTime(c->instance, first_time_index);
    *last_time = KheInstanceTime(c->instance, last_time_index);
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintLimitsWholeCycle(                      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)                                   */
/*                                                                           */
/*  Return true if time groups of c conver the whole cycle.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheClusterBusyTimesConstraintLimitsWholeCycle(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c)
{
  return c->limits_whole_cycle;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(           */
/*    KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,                    */
/*    KHE_RESOURCE_IN_SOLN rs, int offset)                                   */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

static void DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR m;
  m = KheClusterBusyTimesMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoClusterBusyTimesConstraintMonitorsForResource(                    */
/*    KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)    */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

static void DoClusterBusyTimesConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheClusterBusyTimesConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoClusterBusyTimesConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheClusterBusyTimesConstraintAppliesToOffset(c, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintMakeAndAttachMonitors(                 */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                    */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintMakeAndAttachMonitors(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoClusterBusyTimesConstraintMonitorsForResource(soln, c, r);
    }
  }
  HaArrayForEach(c->resources, r, i)
    DoClusterBusyTimesConstraintMonitorsForResource(soln, c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesConstraintMakeFromKml(KML_ELT cons_elt,          */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a cluster busy times constraint based on cons_elt and add to ins.   */
/*                                                                           */
/*****************************************************************************/

bool KheClusterBusyTimesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name, *ref;  int i, wt, val, minimum, maximum;
  bool reqd, allow_zero;  KHE_COST_FUNCTION cf;  KML_ELT history_elt, elt;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT res;  KHE_TIME_GROUP applies_to_tg;
  KHE_RESOURCE r;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  switch( KheInstanceModel(ins) )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE:

      if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	  " AppliesTo TimeGroups #Minimum #Maximum", ke) )
	return false;
      break;

    case KHE_MODEL_EMPLOYEE_SCHEDULE:

      if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	  " AppliesTo +ResourceHistory +AppliesToTimeGroup TimeGroups"
          " #Minimum #Maximum +$AllowZero", ke) )
	return false;
      break;

    default:

      HnAbort("KheClusterBusyTimesConstraintMakeFromKml internal error");
      break;
  }
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum */
  KmlContainsChild(cons_elt, "Minimum", &elt);
  sscanf(KmlText(elt), "%d", &minimum);
  KmlContainsChild(cons_elt, "Maximum", &elt);
  sscanf(KmlText(elt), "%d", &maximum);

  /* get optional applies_to_tg */
  applies_to_tg = NULL;  /* default value */
  if( KmlContainsChild(cons_elt, "AppliesToTimeGroup", &elt) )
  {
    if( !KmlCheck(elt, "Reference", ke) )
      return false;
    ref = KmlAttributeValue(elt, 0);
    if( !KheInstanceRetrieveTimeGroup(ins, ref, &applies_to_tg) )
      return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "<AppliesToTimeGroup> Reference \"%s\" unknown", ref);
  }

  /* get optional allow_zero */
  allow_zero = false;  /* default value */
  if( KmlContainsChild(cons_elt, "AllowZero", &elt) )
  {
    if( strcmp(KmlText(elt), "true") == 0 )
      allow_zero = true;
    else if( strcmp(KmlText(elt), "false") == 0 )
      allow_zero = false;
    else
      return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	"<AllowZero> is neither true nor false");
  }

  /* build and insert the constraint object */
  if( !KheClusterBusyTimesConstraintMake(ins, id, name, reqd, wt, cf,
        applies_to_tg, minimum, maximum, allow_zero, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<ClusterBusyTimesConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheClusterBusyTimesResourceCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<ClusterBusyTimesConstraint> applies to 0 resources");

  /* add the optional resource history */
  if( KmlContainsChild(cons_elt, "ResourceHistory", &history_elt) )
  {
    /* attributes before_dft and after_dft */
    if( !KmlCheck(history_elt, "#before #after : *Resource", ke) )
      return false;
    sscanf(KmlAttributeValue(history_elt, 0), "%d", &val);
    KheClusterBusyTimesConstraintAddHistoryBefore(res, val);
    sscanf(KmlAttributeValue(history_elt, 1), "%d", &val);
    KheClusterBusyTimesConstraintAddHistoryAfter(res, val);

    /* children */
    for( i = 0;  i < KmlChildCount(history_elt);  i++ )
    {
      elt = KmlChild(history_elt, i);
      if( !KmlCheck(elt, "Reference", ke) )
	return false;
      if( !KheInstanceRetrieveResource(ins, KmlAttributeValue(elt, 0), &r) )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "unknown resource %s in <Resource>", KmlAttributeValue(elt, 0));
      if( KheClusterBusyTimesConstraintHistory(res, r) > 0 )
        return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s appears twice in <ResourceHistory>",
	  KmlAttributeValue(elt, 0));
      if( sscanf(KmlText(elt), "%d", &val) != 1 )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s has a non-integer body", KmlAttributeValue(elt, 0));
      if( val < 0 )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s has a negative body", KmlAttributeValue(elt, 0));
      KheClusterBusyTimesConstraintAddHistory(res, r, val);
    }
  }

  /* add the positive and negative time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintWrite(                                 */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KML_FILE kf)                      */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintWrite(KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i, val;
  KHE_TIME_GROUP tg;  char buff[50];
  /* ***
  if( c->limit_busy_recode )  ** don't print if recodes limit busy **
    return;
  *** */
  KmlBegin(kf, "ClusterBusyTimesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in ClusterBusyTimesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);

  /* <AppliesTo> */
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg)!=NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from ClusterBusyTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", KheResourceGroupId(rg));
    }
    KmlEnd(kf, "ResourceGroups");
  }
  if( HaArrayCount(c->resources) > 0 )
  {
    KmlBegin(kf, "Resources");
    HaArrayForEach(c->resources, r, i)
    {
      HnAssert(KheResourceId(r) != NULL, "KheArchiveWrite:  Id missing in "
        "Resource referenced from ClusterBusyTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");

  /* <ResourceHistory> */
  if( c->history_before != 0 || c->history_after != 0 ||
      HaArrayCount(c->history) > 0 )
  {
    KmlBegin(kf, "ResourceHistory");
    sprintf(buff, "%d", c->history_before);
    KmlAttribute(kf, "before", buff);
    sprintf(buff, "%d", c->history_after);
    KmlAttribute(kf, "after", buff);
    HaArrayForEach(c->history, val, i)
      if( val != 0 )
      {
	r = KheInstanceResource(c->instance, i);
	KmlBegin(kf, "Resource");
        KmlAttribute(kf, "Reference", KheResourceId(r));
	KmlFmtText(kf, "%d", val);
	KmlEnd(kf, "Resource");
      }
    KmlEnd(kf, "ResourceHistory");
  }

  /* <AppliesToTimeGroup> */
  if( c->applies_to_tg != NULL )
    KmlEltAttribute(kf, "AppliesToTimeGroup", "Reference",
      KheTimeGroupId(c->applies_to_tg));

  /* <TimeGroups> */
  KmlBegin(kf, "TimeGroups");
  HaArrayForEach(c->time_groups, tg, i)
  {
    HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
      "TimeGroup referenced from ClusterBusyTimesConstraint %s", c->id);
    KmlBegin(kf, "TimeGroup");
    KmlAttribute(kf, "Reference", KheTimeGroupId(tg));
    if( HaArray(c->polarities, i) == KHE_NEGATIVE )
      KmlAttribute(kf, "Polarity", "negative");
    KmlEnd(kf, "TimeGroup");
  }
  KmlEnd(kf, "TimeGroups");

  /* <Minimum>, <Maximum>, and <AllowZero> */
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  if( c->allow_zero && c->minimum > 0 )
    KmlEltFmtText(kf, "AllowZero", "%s", "true");
  KmlEnd(kf, "ClusterBusyTimesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClusterBusyTimesConstraintDebug(                                 */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,                                   */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheClusterBusyTimesConstraintDebug(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  KHE_TIME_GROUP tg;  char *tg_id;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      tg_id = (c->applies_to_tg == NULL ? "null" :
        KheTimeGroupId(c->applies_to_tg) == NULL ? "-" :
        KheTimeGroupId(c->applies_to_tg));
      fprintf(fp, "%*s[ applies_to %s, %d-%d, allow_zero %s\n",
	indent, "", tg_id, c->minimum, c->maximum,
	c->allow_zero ? "true" : "false");
	/* c->limi t_busy_recode ? "true" : "false"); */
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      if( HaArrayCount(c->resources) > 0 )
      {
	HaArrayForEach(c->resources, r, i)
	{
	  if( i == 0 )
	    fprintf(fp, "%*s  ", indent, "");
	  else if( i % 8 == 0 )
	    fprintf(fp, ",\n%*s  ", indent, "");
	  else
	    fprintf(fp, ", ");
	  fprintf(fp, "%s", KheResourceId(r) != NULL ? KheResourceId(r) :"-");
	}
	fprintf(fp, "\n");
      }
      HaArrayForEach(c->time_groups, tg, i)
      {
        fprintf(fp, "%*s  %c", indent, "",
	  HaArray(c->polarities, i) == KHE_NEGATIVE ? '-' : '+');
	KheTimeGroupDebug(tg, 1, 0, fp);
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}


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
/*  FILE:         khe_limit_active_intervals_constraint.c                    */
/*  DESCRIPTION:  A limit active intervals constraint                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT - limit active intervals constraint*/
/*                                                                           */
/*****************************************************************************/

struct khe_limit_active_intervals_constraint_rec {
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
  ARRAY_KHE_TIME_GROUP		time_groups;		/* time groups       */
  ARRAY_KHE_POLARITY		polarities;		/* polarities        */
  bool				all_positive;		/* if all positive   */
  bool				all_negative;		/* if all negative   */
  HA_ARRAY_INT			resource_of_type_count;	/* resource of type  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsConstraintMake(KHE_INSTANCE ins, char *id,   */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT *c)                              */
/*                                                                           */
/*  Make a new limit active intervals constraint with these attributes, add  */
/*  it to ins, and return it.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT *c)
{
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  int i, count;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitActiveIntervalsConstraintMake called after KheInstanceMakeEnd");
  HnAssert(KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitActiveIntervalsConstraintMake called in high school model");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  count = KheInstanceResourceTypeCount(ins);
  /* ***
  res = HaAlloc(a, sizeof(struct khe_limit_active_intervals_constraint_rec)
    + (count - 1) * sizeof(int));
  *** */
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG, ins, id, name, required,
    weight, cf, a);
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
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->polarities, a);
  res->all_positive = true;
  res->all_negative = true;
  HaArrayInit(res->resource_of_type_count, a);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->resource_of_type_count, 0);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitActiveIntervalsConstraintAppliesToTimeGroup(      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the applies-to time group attribute of c (it may be NULL).        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitActiveIntervalsConstraintAppliesToTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->applies_to_tg;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintMinimum(                            */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintMinimum(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintMaximum(                            */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintMaximum(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintAppliesToTimes(                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of applies-to offsets of c.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintAppliesToOffsetCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  if( HaArrayCount(c->applies_to_offsets) == 1 &&
      HaArrayFirst(c->applies_to_offsets) == -1 )
    HnAbort("KheLimitActiveIntervalsConstraintAppliesToOffsetCount"
      " called before KheInstanceMakeEnd");
  return HaArrayCount(c->applies_to_offsets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintAppliesToOffset(                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)                        */
/*                                                                           */
/*  Return the i'th applies-to offset of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintAppliesToOffset(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)
{
  return HaArray(c->applies_to_offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsResourceCount(                                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of resources that c applies to, including resources    */
/*  in resource groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveIntervalsResourceCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintAppliesToCount(                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintAppliesToCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return KheLimitActiveIntervalsResourceCount(c) *
    KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintResetResourceOfType(               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Reset the resource_of_type attributes of c.  This is only done when      */
/*  resource type partitioning is happening.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddToResourceOfType(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArrayFill(c->resource_of_type_count, index + 1, 0);
  HaArray(c->resource_of_type_count, index) += 1;
}

void KheLimitActiveIntervalsConstraintResetResourceOfType(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
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
/*  void KheLimitActiveIntervalsConstraintFinalize(                          */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintFinalize(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  int max_index, base_index, tg_count, i, time_count, index;  KHE_TIME_GROUP tg;

  /* finalize c->applies_to_offsets */
  HaArrayClear(c->applies_to_offsets);
  if( c->applies_to_tg == NULL || KheTimeGroupTimeCount(c->applies_to_tg) == 0 )
    HaArrayAddLast(c->applies_to_offsets, 0);
  else if( KheTimeGroupTimeCount(c->applies_to_tg) >= 1 )
  {
    /* find max_index, the index of the largest time in c->time_groups, or -1 */
    max_index = -1;
    HaArrayForEach(c->time_groups, tg, i)
    {
      tg_count = KheTimeGroupTimeCount(tg);
      if( tg_count > 0 )
      {
	index = KheTimeIndex(KheTimeGroupTime(tg, tg_count - 1));
	if( index > max_index )
	  max_index = index;
      }
    }

    /* add legal offsets to applies_to_offsets */
    time_count = KheInstanceTimeCount(c->instance);
    base_index = KheTimeIndex(KheTimeGroupTime(c->applies_to_tg, 0));
    for( i = 0;  i < KheTimeGroupTimeCount(c->applies_to_tg);  i++ )
    {
      index = KheTimeIndex(KheTimeGroupTime(c->applies_to_tg, i));
      if( max_index + (index - base_index) >= time_count )
	break;
      HaArrayAddLast(c->applies_to_offsets, index - base_index);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintDensityCount(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintDensityCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  if( DEBUG1 )
    fprintf(stderr,
      "KheLimitActiveIntervalsConstraintDensityCount returning %d",
      KheLimitActiveIntervalsConstraintAppliesToCount(c));
  return KheLimitActiveIntervalsConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintAddResourceGroup(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_GROUP rg)        */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddResourceGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  int i, rt_index;  KHE_RESOURCE r;
  HaArrayAddLast(c->resource_groups, rg);
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
  }
  rt_index = KheResourceTypeIndex(KheResourceGroupResourceType(rg));
  HaArray(c->resource_of_type_count, rt_index) +=
    KheResourceGroupResourceCount(rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintResourceGroupCount(                 */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintResourceGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitActiveIntervalsConstraintResourceGroup(       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)                        */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitActiveIntervalsConstraintResourceGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)
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
/*  void KheLimitActiveIntervalsConstraintAddResource(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)               */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddResource(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)
{
  int rt_index;
  HaArrayAddLast(c->resources, r);
  rt_index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArray(c->resource_of_type_count, rt_index) += 1;
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintResourceCount(                      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintResourceCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitActiveIntervalsConstraintResource(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)                        */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitActiveIntervalsConstraintResource(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintResourceOfTypeCount(                */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_TYPE rt)         */
/*                                                                           */
/*  Return the number of resources in c of type rt.                          */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintResourceOfTypeCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE_TYPE rt)
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
/*  void KheLimitActiveIntervalsConstraintAddHistoryBefore(                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val)                      */
/*                                                                           */
/*  Add a history before value to c.                                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val)
{
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitActiveIntervalsConstraintAddHistoryBefore: wrong model");
  HnAssert(c->history_before == 0,
    "KheLimitActiveIntervalsConstraintAddHistoryBefore called twice");
  HnAssert(val>=0,"KheLimitActiveIntervalsConstraintAddHistoryBefore: val < 0");
  c->history_before = val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintHistoryBefore(                      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Retrieve a history before value from c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintHistoryBefore(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->history_before;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintAddHistoryAfter(                   */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val)                      */
/*                                                                           */
/*  Add a history after value to c.                                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int val)
{
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitActiveIntervalsConstraintAddHistoryAfter: wrong model");
  HnAssert(c->history_after == 0,
    "KheLimitActiveIntervalsConstraintAddHistoryAfter called twice");
  HnAssert(val>=0, "KheLimitActiveIntervalsConstraintAddHistoryAfter: val < 0");
  c->history_after = val;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintHistoryAfter(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Retrieve a history after value from c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintHistoryAfter(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->history_after;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintAddHistory(                        */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, int val)      */
/*                                                                           */
/*  Add a history value to c.                                                */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r, int val)
{
  int index;
  HnAssert(KheInstanceModel(c->instance) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitActiveIntervalsConstraintAddHistory: wrong model");
  HnAssert(val >= 0, "KheLimitActiveIntervalsConstraintAddHistory: val < 0");
  index = KheResourceInstanceIndex(r);
  HaArrayFill(c->history, index + 1, 0);
  HnAssert(HaArray(c->history, index) == 0,
    "KheLimitActiveIntervalsConstraintAddHistory: history added twice");
  HaArrayPut(c->history, index, val);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintHistory(                            */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)               */
/*                                                                           */
/*  Retrieve a history value from c.                                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintHistory(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)
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
/*  void KheLimitActiveIntervalsConstraintAddTimeGroup(                      */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_TIME_GROUP tg,            */
/*    KHE_POLARITY po)                                                       */
/*                                                                           */
/*  Add tg with polarity po to c.                                            */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintAddTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_TIME_GROUP tg, KHE_POLARITY po)
{
  HnAssert(tg != NULL,
    "KheLimitActiveIntervalsConstraintAddTimeGroup: tg is NULL");
  if( KheInstanceModel(c->instance) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
    HnAssert(po == KHE_POSITIVE, "KheLimitActiveIntervalsConstraintAddTimeGroup:"
     " KHE_NEGATIVE in high school model");
  HaArrayAddLast(c->time_groups, tg);
  HaArrayAddLast(c->polarities, po);
  if( po == KHE_POSITIVE )
    c->all_negative = false;
  else
    c->all_positive = false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveIntervalsConstraintTimeGroupCount(                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitActiveIntervalsConstraintTimeGroupCount(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitActiveIntervalsConstraintTimeGroup(               */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i, int offset,            */
/*    KHE_POLARITY *po)                                                      */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitActiveIntervalsConstraintTimeGroup(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, int i, int offset, KHE_POLARITY *po)
{
  KHE_TIME_GROUP tg;
  *po = HaArray(c->polarities, i);
  tg = HaArray(c->time_groups, i);
  return offset == 0 ? tg : KheTimeGroupNeighbour(tg, offset);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsConstraintAllPositive(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return true if all time groups are positive.                             */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsConstraintAllPositive(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->all_positive;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsConstraintAllNegative(                       */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Return true if all time groups are negative.                             */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsConstraintAllNegative(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  return c->all_negative;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(       */
/*    KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,                */
/*    KHE_RESOURCE_IN_SOLN rs, int offset)                                   */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

static void DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m;
  m = KheLimitActiveIntervalsMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitActiveIntervalsConstraintMonitorsForResource(                */
/*    KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)*/
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

static void DoLimitActiveIntervalsConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitActiveIntervalsConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitActiveIntervalsConstraintAppliesToOffset(c, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintMakeAndAttachMonitors(             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_SOLN soln)                */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintMakeAndAttachMonitors(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitActiveIntervalsConstraintMonitorsForResource(soln, c, r);
    }
  }
  HaArrayForEach(c->resources, r, i)
    DoLimitActiveIntervalsConstraintMonitorsForResource(soln, c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsConstraintMakeFromKml(KML_ELT cons_elt,      */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit active intervals constraint based on cons_elt; add to ins.  */
/*                                                                           */
/*****************************************************************************/

bool KheLimitActiveIntervalsConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name, *ref;  bool reqd;  int i, wt, val, minimum, maximum;
  KHE_COST_FUNCTION cf; KML_ELT elt, history_elt;  KHE_RESOURCE r;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT res;  KHE_TIME_GROUP applies_to_tg;
  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  HnAssert(KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitActiveIntervalsConstraintMakeFromKml: not employee secheduling");
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction AppliesTo"
      " +ResourceHistory +AppliesToTimeGroup TimeGroups #Minimum #Maximum",ke) )
    return false;
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

  /* build and insert the constraint object */
  if( !KheLimitActiveIntervalsConstraintMake(ins, id, name, reqd, wt, cf,
        applies_to_tg, minimum, maximum, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitActiveIntervalsConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitActiveIntervalsResourceCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitActiveIntervalsConstraint> applies to 0 resources");

  /* add the optional resource history */
  if( KmlContainsChild(cons_elt, "ResourceHistory", &history_elt) )
  {
    /* attributes before_dft and after_dft */
    if( !KmlCheck(history_elt, "#before #after : *Resource", ke) )
      return false;
    sscanf(KmlAttributeValue(history_elt, 0), "%d", &val);
    KheLimitActiveIntervalsConstraintAddHistoryBefore(res, val);
    sscanf(KmlAttributeValue(history_elt, 1), "%d", &val);
    KheLimitActiveIntervalsConstraintAddHistoryAfter(res, val);

    /* children */
    for( i = 0;  i < KmlChildCount(history_elt);  i++ )
    {
      elt = KmlChild(history_elt, i);
      if( !KmlCheck(elt, "Reference", ke) )
	return false;
      if( !KheInstanceRetrieveResource(ins, KmlAttributeValue(elt, 0), &r) )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "unknown resource %s in <Resource>", KmlAttributeValue(elt, 0));
      if( KheLimitActiveIntervalsConstraintHistory(res, r) > 0 )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s appears twice in <ResourceHistory>",
	  KmlAttributeValue(elt, 0));
      if( sscanf(KmlText(elt), "%d", &val) != 1 )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s has a non-integer body", KmlAttributeValue(elt, 0));
      if( val < 0 )
	return KmlError(ke, a, KmlLineNum(elt), KmlColNum(elt),
	  "resource %s has a negative body", KmlAttributeValue(elt, 0));
      KheLimitActiveIntervalsConstraintAddHistory(res, r, val);
    }
  }

  /* time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintWrite(                             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KML_FILE kf)                  */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintWrite(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c, KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i, val;
  KHE_TIME_GROUP tg;  char buff[50];
  KmlBegin(kf, "LimitActiveIntervalsConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LimitActiveIntervalsConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);

  /* <AppliesTo> */
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing "
        "in ResourceGroup referenced from LimitActiveIntervalsConstraint %s",
	c->id);
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
        "Resource referenced from LimitActiveIntervalsConstraint %s", c->id);
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
	KmlEltAttributeFmtText(kf, "Resource", "Reference", KheResourceId(r),
	  "%d", val);
      }
    KmlEnd(kf, "ResourceHistory");
  }

  /* <AppliesToTimeGroup> */
  if( c->applies_to_tg != NULL )
  {
    KmlBegin(kf, "AppliesToTimeGroup");
    KmlAttribute(kf, "Reference", KheTimeGroupId(c->applies_to_tg));
    KmlEnd(kf, "AppliesToTimeGroup");
  }

  /* <TimeGroups> */
  KmlBegin(kf, "TimeGroups");
  HaArrayForEach(c->time_groups, tg, i)
  {
    HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
      "TimeGroup referenced from LimitActiveIntervalsConstraint %s", c->id);
    KmlBegin(kf, "TimeGroup");
    KmlAttribute(kf, "Reference", KheTimeGroupId(tg));
    if( HaArray(c->polarities, i) == KHE_NEGATIVE )
      KmlAttribute(kf, "Polarity", "negative");
    KmlEnd(kf, "TimeGroup");
  }
  KmlEnd(kf, "TimeGroups");

  /* <Minimum> and <Maximum> */
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  KmlEnd(kf, "LimitActiveIntervalsConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitActiveIntervalsConstraintDebug(                             */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,                               */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitActiveIntervalsConstraintDebug(
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
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
      fprintf(fp, "%*s[ applies_to %s, %d-%d\n",
	indent, "", tg_id, c->minimum, c->maximum);
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      HaArrayForEach(c->resources, r, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceId(r) != NULL ? KheResourceId(r) : "-");
      HaArrayForEach(c->time_groups, tg, i)
      {
        fprintf(fp, "%*s  %c%s\n", indent, "",
	  HaArray(c->polarities, i) == KHE_NEGATIVE ? '*' : ' ',
	  KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-");
      }
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

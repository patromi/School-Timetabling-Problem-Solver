
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
/*  FILE:         khe_limit_effort_constraint.c                              */
/*  DESCRIPTION:  A limit effort (busy times or workload) constraint         */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_EFFORT_CONSTRAINT - a limit effort constraint                  */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_effort_constraint_rec {
  INHERIT_CONSTRAINT
  bool				is_workload;		/* limit workload    */
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  KHE_TIME_GROUP		applies_to_tg;		/* applies to tg     */
  HA_ARRAY_INT			applies_to_offsets;	/* times used        */
  int				minimum;		/* minimum           */
  int				maximum;		/* maximum           */
  bool				allow_zero;		/* allow_zero        */
  bool				limits_whole_cycle;	/* limits whole cycle*/
  ARRAY_KHE_TIME_GROUP		time_groups;		/* time groups       */
  KHE_TIME_GROUP		domain;			/* time groups union */
  HA_ARRAY_INT			resource_of_type_count;	/* resource of type  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEffortConstraintMake(KHE_INSTANCE ins, char *id,            */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    bool is_workload, KHE_TIME_GROUP applies_to_tg, int minimum,           */
/*    int maximum, bool allow_zero, KHE_LIMIT_EFFORT_CONSTRAINT *c)          */
/*                                                                           */
/*  Make a new limit effort constraint with these attributes, add it         */
/*  to ins, and return it.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheLimitEffortConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  bool is_workload, KHE_TIME_GROUP applies_to_tg, int minimum,
  int maximum, bool allow_zero, KHE_LIMIT_EFFORT_CONSTRAINT *c)
{
  KHE_LIMIT_EFFORT_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  int i, count;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitEffortConstraintMake called after KheInstanceMakeEnd");
  if( KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
  {
    HnAssert(applies_to_tg == NULL, "KheLimitEffortConstraintMake: "
      "applies_to_tg != NULL in high school model");
    if( is_workload )
      HnAssert(!allow_zero, "KheLimitEffortConstraintMake: "
	"allow_zero workload in high school model");
    else
      HnAssert(allow_zero, "KheLimitEffortConstraintMake: "
	"!allow_zero busy times in high school model");
  }
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_EFFORT_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  res->is_workload = is_workload;
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->applies_to_tg = applies_to_tg;
  HaArrayInit(res->applies_to_offsets, a);
  HaArrayAddLast(res->applies_to_offsets, -1);  /* signals not initialized */
  res->minimum = minimum;
  res->maximum = maximum;
  res->allow_zero = allow_zero;
  res->limits_whole_cycle = false;  /* placeholder */
  HaArrayInit(res->time_groups, a);
  res->domain = NULL;
  count = KheInstanceResourceTypeCount(ins);
  HaArrayInit(res->resource_of_type_count, a);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->resource_of_type_count, 0);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitEffortConstraintAppliesToTimeGroup(               */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the applies-to time group attribute of c (it may be NULL).        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitEffortConstraintAppliesToTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return c->applies_to_tg;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintMinimum(KHE_LIMIT_EFFORT_CONSTRAINT c)       */
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintMinimum(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintMaximum(KHE_LIMIT_EFFORT_CONSTRAINT c)       */
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintMaximum(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEffortConstraintAllowZero(KHE_LIMIT_EFFORT_CONSTRAINT c)    */
/*                                                                           */
/*  Return the allow_zero attribute of c.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitEffortConstraintAllowZero(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return c->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintAppliesToOffsetCount(                        */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of applies-to offsets of c.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintAppliesToOffsetCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  if( HaArrayCount(c->applies_to_offsets) == 1 &&
      HaArrayFirst(c->applies_to_offsets) == -1 )
    HnAbort("KheLimitEffortConstraintAppliesToOffsetCount"
      " called before KheInstanceMakeEnd");
  return HaArrayCount(c->applies_to_offsets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintAppliesToOffset(                             */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th applies-to offset of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintAppliesToOffset(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i)
{
  return HaArray(c->applies_to_offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortResourceCount(KHE_LIMIT_EFFORT_CONSTRAINT c)           */
/*                                                                           */
/*  Return the number of resources that c applies to, including resources    */
/*  in resource groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static int KheLimitEffortResourceCount(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintAppliesToCount(                              */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintAppliesToCount(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return KheLimitEffortResourceCount(c) *
    KheLimitEffortConstraintAppliesToOffsetCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintResetResourceOfType(                        */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Reset the resource_of_type attributes of c.  This is only done when      */
/*  resource type partitioning is happening.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddToResourceOfType(KHE_LIMIT_EFFORT_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArrayFill(c->resource_of_type_count, index + 1, 0);
  HaArray(c->resource_of_type_count, index) += 1;
}

void KheLimitEffortConstraintResetResourceOfType(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
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
/*  void KheLimitEffortConstraintFinalize(KHE_LIMIT_EFFORT_CONSTRAINT c)     */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintFinalize(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  int tg_count, max_index, base_index, i, time_count, index;  KHE_TIME_GROUP tg;
  SSET ss;  HA_ARENA a;

  /* if is_workload, add sole time group if there are none right now */
  if( DEBUG1 )
    fprintf(stderr, "[ KheLimitEffortConstraintFinalize(c)\n");
  if( c->is_workload && HaArrayCount(c->time_groups) == 0 )
    HaArrayAddLast(c->time_groups, KheInstanceFullTimeGroup(c->instance));
    /* *** this one fails a model test
    KheLimitEffortConstraintAddTimeGroup(c,
      KheInstanceFullTimeGroup(c->instance));
    *** */

  /* build c->domain */
  a = KheInstanceArena(c->instance);
  SSetInit(ss, a);
  HaArrayForEach(c->time_groups, tg, i)
    SSetUnion(ss, *KheTimeGroupTimeSet(tg));
  c->domain = KheTimeGroupMakeAndFinalize(c->instance,
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, /* NULL, */ false, a);
  if( DEBUG1 )
  {
    fprintf(stderr, "  domain: ");
    KheTimeGroupDebug(c->domain, 2, 2, stderr);
    fprintf(stderr, "  applies_to_tg: ");
    KheTimeGroupDebug(c->applies_to_tg, 2, 2, stderr);
  }

  /* set c->limits_whole_cycle */
  c->limits_whole_cycle = false;
  HaArrayForEach(c->time_groups, tg, i)
    if( KheTimeGroupTimeCount(tg) == KheInstanceTimeCount(c->instance) )
    {
      c->limits_whole_cycle = true;
      break;
    }

  /* finalize c->applies_to_offsets */
  HaArrayClear(c->applies_to_offsets);
  if( c->applies_to_tg == NULL || KheTimeGroupTimeCount(c->applies_to_tg) == 0 )
    HaArrayAddLast(c->applies_to_offsets, 0);
  else
  {
    tg_count = KheTimeGroupTimeCount(c->domain);
    if( tg_count > 0 )
    {
      /* find max_index, the index of the largest time in c->time_groups */
      max_index = KheTimeIndex(KheTimeGroupTime(c->domain, tg_count - 1));

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
  if( DEBUG1 )
  {
    HaArrayForEach(c->applies_to_offsets, index, i)
      fprintf(stderr, "  %d", index);
    fprintf(stderr, "\n");
    fprintf(stderr, "] KheLimitEffortConstraintFinalize\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintDensityCount(                                */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintDensityCount(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return KheLimitEffortConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintAddTimeGroup(                               */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_TIME_GROUP tg)                      */
/*                                                                           */
/*  Add tg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintAddTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_TIME_GROUP tg)
{
  if( c->is_workload )
    HnAssert(KheInstanceModel(c->instance) != KHE_MODEL_HIGH_SCHOOL_TIMETABLE,
      "KheLimitEffortConstraintAddTimeGroup not allowed in high school model");
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintTimeGroupCount(                              */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintTimeGroupCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitEffortConstraintTimeGroup(                        */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, int i, int offset)                      */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitEffortConstraintTimeGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i, int offset)
{
  HnAssert(i >= 0 && i < HaArrayCount(c->time_groups),
    "KheLimitEffortConstraintTimeGroup:  i (%d) out of range 0 .. %d",
    i, HaArrayCount(c->time_groups) - 1);
  if( offset == 0 )
    return HaArray(c->time_groups, i);
  else
    return KheTimeGroupNeighbour(HaArray(c->time_groups, i), offset);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitEffortConstraintDomain(                           */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the domain (the union of the time groups) of c.                   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitEffortConstraintDomain(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitEffortConstraintDomain called before instance ends");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEffortConstraintLimitsWholeCycle(                           */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return true when c contains a time group that limits the whole cycle.    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitEffortConstraintLimitsWholeCycle(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitEffortConstraintLimitsWholeCycle called before instance ends");
  return c->limits_whole_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintAddResourceGroup(                           */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_GROUP rg)                  */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintAddResourceGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
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
/*  int KheLimitEffortConstraintResourceGroupCount(                          */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintResourceGroupCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitEffortConstraintResourceGroup(                */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitEffortConstraintResourceGroup(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i)
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
/*  void KheLimitEffortConstraintAddResource(                                */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE r)                         */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintAddResource(KHE_LIMIT_EFFORT_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int rt_index;
  HaArrayAddLast(c->resources, r);
  rt_index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArray(c->resource_of_type_count, rt_index) += 1;
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintResourceCount(                               */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintResourceCount(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitEffortConstraintResource(                           */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, int i)                                  */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitEffortConstraintResource(
  KHE_LIMIT_EFFORT_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortConstraintResourceOfTypeCount(                         */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_TYPE rt)                   */
/*                                                                           */
/*  Return the number of resources in c of type rt.                          */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortConstraintResourceOfTypeCount(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE_TYPE rt)
{
  return HaArray(c->resource_of_type_count, KheResourceTypeIndex(rt));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DoLimitEffortConstraintMonitorsForResourceAndOffset(                */
/*    KHE_SOLN soln, KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE r,          */
/*    int offset)                                                            */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

static void DoLimitEffortConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_EFFORT_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_EFFORT_MONITOR m;
  m = KheLimitEffortMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitEffortConstraintMonitorsForResource(                         */
/*    KHE_SOLN soln, KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE r)          */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

static void DoLimitEffortConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitEffortConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitEffortConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitEffortConstraintAppliesToOffset(c, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintMakeAndAttachMonitors(                      */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_SOLN soln)                          */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintMakeAndAttachMonitors(
  KHE_LIMIT_EFFORT_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitEffortConstraintMonitorsForResource(soln, c, r);
    }
  }
  HaArrayForEach(c->resources, r, i)
    DoLimitEffortConstraintMonitorsForResource(soln, c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitEffortConstraintTypeName(bool is_workload)                 */
/*                                                                           */
/*  Return "LimitBusyTimesConstraint" or "LimitWorkloadConstraint".          */
/*  depending on is_workload.                                                */
/*                                                                           */
/*****************************************************************************/

static char *KheLimitEffortConstraintTypeName(bool is_workload)
{
  return is_workload ? "LimitWorkloadConstraint" : "LimitBusyTimesConstraint";
}


/*****************************************************************************/
/*                                                                           */
/*    KHE_INSTANCE ins, bool is_workload, KML_ERROR *ke)                     */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit effort constraint based on cons_elt and add it to ins.      */
/*                                                                           */
/*****************************************************************************/

bool KheLimitEffortConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, bool is_workload, KML_ERROR *ke)
{
  char *id, *name, *ref;  bool reqd;  int wt;  HA_ARENA a;
  KHE_COST_FUNCTION cf;  KML_ELT elt;  KHE_LIMIT_EFFORT_CONSTRAINT res;
  int minimum, maximum;  bool allow_zero;  KHE_TIME_GROUP applies_to_tg;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  switch( KheInstanceModel(ins) )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE:

      if( is_workload )
      {
	if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	    " AppliesTo #Minimum #Maximum", ke) )
	  return false;
      }
      else
      {
	if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	    " AppliesTo TimeGroups #Minimum #Maximum", ke) )
	  return false;
      }
      break;

    case KHE_MODEL_EMPLOYEE_SCHEDULE:

      if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	  " AppliesTo +AppliesToTimeGroup +TimeGroups #Minimum #Maximum"
	  " +$AllowZero", ke) )
	return false;
      break;

    default:

      HnAbort("KheLimitEffortConstraintMakeFromKml internal error");
      break;
  }
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum (both must be present, see check above) */
  if( KmlContainsChild(cons_elt, "Minimum", &elt) )
    sscanf(KmlText(elt), "%d", &minimum);
  if( KmlContainsChild(cons_elt, "Maximum", &elt) )
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
  allow_zero = (is_workload ? false : true);  /* default value */
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
  is_workload = false;  /* still to do */
  if( !KheLimitEffortConstraintMake(ins, id, name, reqd, wt, cf,
        is_workload, applies_to_tg, minimum, maximum, allow_zero, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<%s> Id \"%s\" used previously",
      KheLimitEffortConstraintTypeName(is_workload), id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitEffortResourceCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<%s> applies to 0 resources",
      KheLimitEffortConstraintTypeName(is_workload));

  /* add the time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a))
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEffortConstraintIsWorkload(KHE_LIMIT_EFFORT_CONSTRAINT c)   */
/*                                                                           */
/*  Return true if c monitors workload, and false if it monitors busy times. */
/*                                                                           */
/*****************************************************************************/

bool KheLimitEffortConstraintIsWorkload(KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return c->is_workload;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitEffortConstraintHasFullTimeGroupOnly(                       */
/*    KHE_LIMIT_EFFORT_CONSTRAINT c)                                         */
/*                                                                           */
/*  Return true if c has just one time group, and it is the instance full    */
/*  time group.  (In that case the time group does not have to be printed.)  */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitEffortConstraintHasFullTimeGroupOnly(
  KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups) == 1 && KheInstanceTimeCount(c->instance)
    == KheTimeGroupTimeCount(HaArrayFirst(c->time_groups));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintWrite(KHE_LIMIT_EFFORT_CONSTRAINT c,        */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintWrite(KHE_LIMIT_EFFORT_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;  int i;
  KmlBegin(kf, KheLimitEffortConstraintTypeName(c->is_workload));
  HnAssert(c->id != NULL, "KheArchiveWrite: Id missing in %s",
    KheLimitEffortConstraintTypeName(c->is_workload));
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg)!=NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from LimitEffortConstraint %s", c->id);
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
        "Resource referenced from LimitEffortConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  if( c->applies_to_tg != NULL )
    KmlEltAttribute(kf, "AppliesToTimeGroup", "Reference",
      KheTimeGroupId(c->applies_to_tg));
  if( c->is_workload && HaArrayCount(c->time_groups) > 0 &&
      !KheLimitEffortConstraintHasFullTimeGroupOnly(c) )
  {
    KmlBegin(kf, "TimeGroups");
    HaArrayForEach(c->time_groups, tg, i)
    {
      HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
	"TimeGroup referenced from %s %s",
	KheLimitEffortConstraintTypeName(c->is_workload), c->id);
      KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
    }
    KmlEnd(kf, "TimeGroups");
  }
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  if( c->allow_zero )
    KmlEltFmtText(kf, "AllowZero", "%s", "true");
  KmlEnd(kf, KheLimitEffortConstraintTypeName(c->is_workload));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortConstraintDebug(KHE_LIMIT_EFFORT_CONSTRAINT c,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortConstraintDebug(KHE_LIMIT_EFFORT_CONSTRAINT c,
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
      fprintf(fp, "%*s[ is_workload %s, applies_to %s, %d-%d, allow_zero %s\n",
	indent, "", bool_show(c->is_workload), tg_id, c->minimum, c->maximum,
	c->allow_zero ? "true" : "false");
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      HaArrayForEach(c->resources, r, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceId(r) != NULL ? KheResourceId(r) : "-");
      HaArrayForEach(c->time_groups, tg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

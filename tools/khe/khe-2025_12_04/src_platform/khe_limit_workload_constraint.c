
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
/*  FILE:         khe_limit_workload_constraint.c                            */
/*  DESCRIPTION:  A limit workload constraint                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_WORKLOAD_CONSTRAINT - a limit workload constraint              */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_workload_constraint_rec {
  INHERIT_CONSTRAINT
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
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadConstraintMake(KHE_INSTANCE ins, char *id,          */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,                */
/*    bool allow_zero, KHE_LIMIT_WORKLOAD_CONSTRAINT *c)                     */
/*                                                                           */
/*  Make a new limit workload constraint with these attributes, add it       */
/*  to ins, and return it.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_LIMIT_WORKLOAD_CONSTRAINT *c)
{
  KHE_LIMIT_WORKLOAD_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitWorkloadConstraintMake called after KheInstanceMakeEnd");
  if( KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
  {
    HnAssert(applies_to_tg == NULL, "KheLimitWorkloadConstraintMake: "
      "applies_to_tg != NULL in high school model");
    HnAssert(!allow_zero, "KheLimitWorkloadConstraintMake: "
      "allow_zero in high school model");
  }
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_WORKLOAD_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
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
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitWorkloadConstraintAppliesToTimeGroup(             */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the applies-to time group attribute of c (it may be NULL).        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitWorkloadConstraintAppliesToTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return c->applies_to_tg;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintMinimum(KHE_LIMIT_WORKLOAD_CONSTRAINT c)   */
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintMinimum(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintMaximum(KHE_LIMIT_WORKLOAD_CONSTRAINT c)   */
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintMaximum(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadConstraintAllowZero(KHE_LIMIT_WORKLOAD_CONSTRAINT c)*/
/*                                                                           */
/*  Return the allow_zero attribute of c.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadConstraintAllowZero(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return c->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintAppliesToOffsetCount(                      */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the number of applies-to offsets of c.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintAppliesToOffsetCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  if( HaArrayCount(c->applies_to_offsets) == 1 &&
      HaArrayFirst(c->applies_to_offsets) == -1 )
    HnAbort("KheLimitWorkloadConstraintAppliesToOffsetCount"
      " called before KheInstanceMakeEnd");
  return HaArrayCount(c->applies_to_offsets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintAppliesToOffset(                           */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)                                */
/*                                                                           */
/*  Return the i'th applies-to offset of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintAppliesToOffset(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)
{
  return HaArray(c->applies_to_offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadResourceCount(KHE_LIMIT_WORKLOAD_CONSTRAINT c)       */
/*                                                                           */
/*  Return the number of resources that c applies to, including resources    */
/*  in resource groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static int KheLimitWorkloadResourceCount(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintAppliesToCount(                            */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintAppliesToCount(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return KheLimitWorkloadResourceCount(c) *
    KheLimitWorkloadConstraintAppliesToOffsetCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintFinalize(KHE_LIMIT_WORKLOAD_CONSTRAINT c) */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintFinalize(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  int tg_count, max_index, base_index, i, time_count, index;  KHE_TIME_GROUP tg;
  SSET ss;  HA_ARENA a;

  /* add sole time group if there are none right now */
  if( DEBUG1 )
    fprintf(stderr, "[ KheLimitWorkloadConstraintFinalize(c)\n");
  if( HaArrayCount(c->time_groups) == 0 )
    HaArrayAddLast(c->time_groups, KheInstanceFullTimeGroup(c->instance));
    /* *** this one fails a model test
    KheLimitWorkloadConstraintAddTimeGroup(c,
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
    fprintf(stderr, "] KheLimitWorkloadConstraintFinalize\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintDensityCount(                              */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintDensityCount(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return KheLimitWorkloadConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintAddTimeGroup(                             */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_TIME_GROUP tg)                    */
/*                                                                           */
/*  Add tg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintAddTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_TIME_GROUP tg)
{
  HnAssert(KheInstanceModel(c->instance) != KHE_MODEL_HIGH_SCHOOL_TIMETABLE,
    "KheLimitWorkloadConstraintAddTimeGroup not allowed in high school model");
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintTimeGroupCount(                            */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintTimeGroupCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitWorkloadConstraintTimeGroup(                      */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i, int offset)                    */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitWorkloadConstraintTimeGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i, int offset)
{
  HnAssert(i >= 0 && i < HaArrayCount(c->time_groups),
    "KheLimitWorkloadConstraintTimeGroup:  i (%d) out of range 0 .. %d",
    i, HaArrayCount(c->time_groups) - 1);
  if( offset == 0 )
    return HaArray(c->time_groups, i);
  else
    return KheTimeGroupNeighbour(HaArray(c->time_groups, i), offset);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitWorkloadConstraintDomain(                         */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the domain (the union of the time groups) of c.                   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitWorkloadConstraintDomain(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitWorkloadConstraintDomain called before instance ends");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadConstraintLimitsWholeCycle(                         */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return true when c contains a time group that limits the whole cycle.    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadConstraintLimitsWholeCycle(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitWorkloadConstraintLimitsWholeCycle called before instance ends");
  return c->limits_whole_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintAddResourceGroup(                         */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE_GROUP rg)                */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintAddResourceGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  int i;  KHE_RESOURCE r;
  HaArrayAddLast(c->resource_groups, rg);
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintResourceGroupCount(                        */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintResourceGroupCount(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitWorkloadConstraintResourceGroup(              */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)                                */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitWorkloadConstraintResourceGroup(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)
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
/*  void KheLimitWorkloadConstraintAddResource(                              */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r)                       */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintAddResource(KHE_LIMIT_WORKLOAD_CONSTRAINT c,
  KHE_RESOURCE r)
{
  HaArrayAddLast(c->resources, r);
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadConstraintResourceCount(                             */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadConstraintResourceCount(KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitWorkloadConstraintResource(                         */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)                                */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitWorkloadConstraintResource(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void DoLimitWorkloadConstraintMonitorsForResourceAndOffset(              */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r,        */
/*    int offset)                                                            */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

static void DoLimitWorkloadConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_WORKLOAD_MONITOR m;
  m = KheLimitWorkloadMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitWorkloadConstraintMonitorsForResource(                       */
/*    KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r)        */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

static void DoLimitWorkloadConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitWorkloadConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitWorkloadConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitWorkloadConstraintAppliesToOffset(c, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintMakeAndAttachMonitors(                    */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_SOLN soln)                        */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintMakeAndAttachMonitors(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      DoLimitWorkloadConstraintMonitorsForResource(soln, c, r);
    }
  }
  HaArrayForEach(c->resources, r, i)
    DoLimitWorkloadConstraintMonitorsForResource(soln, c, r);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadConstraintMakeFromKml(KML_ELT cons_elt,             */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit workload constraint based on cons_elt and add it to ins.    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name, *ref;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_LIMIT_WORKLOAD_CONSTRAINT res;  HA_ARENA a;
  int minimum, maximum;  bool allow_zero;  KHE_TIME_GROUP applies_to_tg;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  switch( KheInstanceModel(ins) )
  {
    case KHE_MODEL_HIGH_SCHOOL_TIMETABLE:

      if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	  " AppliesTo #Minimum #Maximum", ke) )
	return false;
      break;

    case KHE_MODEL_EMPLOYEE_SCHEDULE:

      if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction"
	  " AppliesTo +AppliesToTimeGroup +TimeGroups #Minimum #Maximum"
	  " +$AllowZero", ke) )
	return false;
      break;

    default:

      HnAbort("KheLimitBusyTimesConstraintMakeFromKml internal error");
      break;
  }
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum (both are present, see check above) */
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
  if( !KheLimitWorkloadConstraintMake(ins, id, name, reqd, wt, cf,
        applies_to_tg, minimum, maximum, allow_zero, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitWorkloadConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitWorkloadResourceCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitWorkloadConstraint> applies to 0 resources");

  /* add the time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a))
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadConstraintHasFullTimeGroupOnly(                     */
/*    KHE_LIMIT_WORKLOAD_CONSTRAINT c)                                       */
/*                                                                           */
/*  Return true if c has just one time group, and it is the instance full    */
/*  time group.  (In that case the time group does not have to be printed.)  */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitWorkloadConstraintHasFullTimeGroupOnly(
  KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups) == 1 && KheInstanceTimeCount(c->instance)
    == KheTimeGroupTimeCount(HaArrayFirst(c->time_groups));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintWrite(KHE_LIMIT_WORKLOAD_CONSTRAINT c,    */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintWrite(KHE_LIMIT_WORKLOAD_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;  int i;
  KmlBegin(kf, "LimitWorkloadConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LimitWorkloadConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg)!=NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from LimitWorkloadConstraint %s", c->id);
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
        "Resource referenced from LimitWorkloadConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  if( c->applies_to_tg != NULL )
    KmlEltAttribute(kf, "AppliesToTimeGroup", "Reference",
      KheTimeGroupId(c->applies_to_tg));
  if( HaArrayCount(c->time_groups) > 0 &&
      !KheLimitWorkloadConstraintHasFullTimeGroupOnly(c) )
  {
    KmlBegin(kf, "TimeGroups");
    HaArrayForEach(c->time_groups, tg, i)
    {
      HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
	"TimeGroup referenced from LimitWorkloadConstraint %s", c->id);
      KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
    }
    KmlEnd(kf, "TimeGroups");
  }
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  if( c->allow_zero )
    KmlEltFmtText(kf, "AllowZero", "%s", "true");
  KmlEnd(kf, "LimitWorkloadConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadConstraintDebug(KHE_LIMIT_WORKLOAD_CONSTRAINT c,    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadConstraintDebug(KHE_LIMIT_WORKLOAD_CONSTRAINT c,
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

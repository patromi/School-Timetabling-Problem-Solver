
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
/*  FILE:         khe_limit_busy_times_constraint.c                          */
/*  DESCRIPTION:  A limit busy times constraint                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0	/* recoding */

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_BUSY_TIMES_CONSTRAINT - a limit busy times constraint          */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_busy_times_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  KHE_TIME_GROUP		applies_to_tg;		/* applies to tg     */
  HA_ARRAY_INT			applies_to_offsets;	/* times used        */
  int				minimum;		/* minimum           */
  int				maximum;		/* maximum           */
  bool				allow_zero;		/* allow_zero        */
  /* bool			limit_busy_recode; */	/* recode as cluster */
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
/*  bool KheLimitBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,         */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,                */
/*    bool allow_zero, KHE_LIMIT_BUSY_TIMES_CONSTRAINT *c)                   */
/*                                                                           */
/*  Make a new limit busy times constraint with these attributes, add it     */
/*  to ins, and return it.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_TIME_GROUP applies_to_tg, int minimum, int maximum,
  bool allow_zero, KHE_LIMIT_BUSY_TIMES_CONSTRAINT *c)
{
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  int i, count;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitBusyTimesConstraintMake called after KheInstanceMakeEnd");
  if( KheInstanceModel(ins) == KHE_MODEL_HIGH_SCHOOL_TIMETABLE )
  {
    HnAssert(applies_to_tg == NULL, "KheLimitBusyTimesConstraintMake: "
      "applies_to_tg != NULL in high school model");
    HnAssert(allow_zero, "KheLimitBusyTimesConstraintMake: "
      "!allow_zero in high school model");
  }
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  count = KheInstanceResourceTypeCount(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG, ins, id, name, required, weight, cf,a);
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->applies_to_tg = applies_to_tg;
  HaArrayInit(res->applies_to_offsets, a);
  HaArrayAddLast(res->applies_to_offsets, -1);  /* signals not initialized */
  res->minimum = minimum;
  res->maximum = maximum;
  res->allow_zero = allow_zero;
  /* res->limit_busy_recode = false; */  /* placeholder until finalized */
  res->limits_whole_cycle = false; /* placeholder until finalized */
  HaArrayInit(res->time_groups, a);
  res->domain = NULL;
  HaArrayInit(res->resource_of_type_count, a);
  for( i = 0;  i < count;  i++ )
    HaArrayAddLast(res->resource_of_type_count, 0);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitBusyTimesConstraintAppliesToTimeGroup(            */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the applies-to time group attribute of c (it may be NULL).        */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitBusyTimesConstraintAppliesToTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return c->applies_to_tg;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintMinimum(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintMinimum(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintMaximum(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintMaximum(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesConstraintAllowZero(                               */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the allow_zero attribute of c.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesConstraintAllowZero(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return c->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintAppliesToOffsetCount(                     */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of applies-to offsets of c.                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintAppliesToOffsetCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  if( HaArrayCount(c->applies_to_offsets) == 1 &&
      HaArrayFirst(c->applies_to_offsets) == -1 )
    HnAbort("KheLimitBusyTimesConstraintAppliesToOffsetCount"
      " called before KheInstanceMakeEnd");
  return HaArrayCount(c->applies_to_offsets);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintAppliesToOffset(                          */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th applies-to offset of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintAppliesToOffset(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->applies_to_offsets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesResourceCount(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)    */
/*                                                                           */
/*  Return the number of resources that c applies to, including resources    */
/*  in resource groups.                                                      */
/*                                                                           */
/*****************************************************************************/

static int KheLimitBusyTimesResourceCount(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintAppliesToCount(                           */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintAppliesToCount(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return KheLimitBusyTimesResourceCount(c) *
    KheLimitBusyTimesConstraintAppliesToOffsetCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesConstraintRecode(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return true if c is being recoded.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** not needed
bool KheLimitBusyTimesConstraintRecode(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return c->limit_busy_recode;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheRecodedName(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,                  */
/*    KHE_TIME_GROUP tg, int tg_index, HA_ARENA a)                           */
/*                                                                           */
/*  Return a suitable name for the recoded version of tg, the tg_index'th    */
/*  time group of c.  Or tg may be NULL, if there is only one time group.    */
/*                                                                           */
/*****************************************************************************/

/* *** not recoding now
static char *KheRecodedName(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  KHE_TIME_GROUP tg, int tg_index, HA_ARENA a)
{
  char *id;  KHE_CONSTRAINT cc;  int j;

  ** first attempt **
  if( tg == NULL )
    id =  HnStringMake(a, "%s:R", c->id);
  else if( KheTimeGroupId(tg) == NULL )
    id =  HnStringMake(a, "%s:%d", c->id, tg_index + 1);
  else
    id =  HnStringMake(a, "%s:%s", c->id, KheTimeGroupId(tg));

  ** further attempts **
  for( j = 1;  KheInstanceRetrieveConstraint(c->instance, id, &cc);  j++ )
  {
    if( tg == NULL )
      id = HnStringMake(a, "%s:%d", c->id, j);
    else if( KheTimeGroupId(tg) == NULL )
      id = HnStringMake(a, "%s:%d:%d", c->id, tg_index + 1, j);
    else
      id = HnStringMake(a, "%s:%s:%d", c->id, KheTimeGroupId(tg), j);
  }
  return id;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintResetResourceOfType(                     */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Reset the resource_of_type attributes of c.  This is only done when      */
/*  resource type partitioning is happening.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddToResourceOfType(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE r)
{
  int index = KheResourceTypeIndex(KheResourceResourceType(r));
  HaArrayFill(c->resource_of_type_count, index + 1, 0);
  HaArray(c->resource_of_type_count, index) += 1;
}

void KheLimitBusyTimesConstraintResetResourceOfType(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
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
/*  void KheLimitBusyTimesConstraintFinalize(                                */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintFinalize(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
  /* bool limit_busy_recode) */
{
  int tg_count, max_index, base_index, i, time_count, index;  HA_ARENA a;
  KHE_TIME_GROUP tg;  SSET ss;  /* KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r; */
  /* KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  char *id, *name; */

  /* build c->domain */
  if( DEBUG1 )
    fprintf(stderr, "[ KheLimitBusyTimesConstraintFinalize(c)\n");
  a = KheInstanceArena(c->instance);
  SSetInit(ss, a);
  HaArrayForEach(c->time_groups, tg, i)
    SSetUnion(ss, *KheTimeGroupTimeSet(tg));
  c->domain = KheTimeGroupMakeAndFinalize(c->instance,
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, /* NULL, */ false, a);
  /* ***
  c->domain = KheTimeGroup MakeInternal(c->instance,
    ** KHE_TIME_GROUP_TYPE_CONSTRUCTED, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
    NULL, NULL ** , LSetNew() **);
  HaArrayForEach(c->time_groups, tg, i)
    KheTimeGroupUnion Internal(c->domain, tg);
  KheTimeGroup Finalize(c->domain, NULL, false ** , NULL, -1 **);
  *** */
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
  }

  /* recode, if requested and c has a non-trivial minimum limit */
  /* *** this is not exact when the cost function is not linear
  if( limit_busy_recode && c->minimum > 0 )
  {
    c->limit_busy_recode = true;
    if( DEBUG2 )
    {
      fprintf(stderr, "[ KheLimitBusyTimesConstraintFinalize recoding:\n");
      KheConstraintDebug((KHE_CONSTRAINT) c, 4, 2, stderr);
      fprintf(stderr, "  recoded to:\n");
    }
    HaArrayForEach(c->time_groups, tg, i)
    {
      ** make a unique id for the recoded constraint, and a name **
      if( HaArrayCount(c->time_groups) == 1 )
        id = KheRecodedName(c, NULL, 0, a);
      else
        id = KheRecodedName(c, tg, i, a);
      name = (c->name==NULL ? NULL : HnStringMake(a, "%s (recoded)", c->name));

      ** make the constraint and mark it as a recoding **
      if( !KheClusterBusyTimesConstraintMakeInternal(c->instance, id, name,
	    c->required, c->weight, c->cost_function, c->applies_to_tg,
	    c->minimum, c->maximum, c->allow_zero, true, &cbtc) )
	HnAbort("KheLimitBusyTimesConstraintFinalize internal error 1");
      ** KheClusterBusyTimesConstraintSetLimitBusyRecode(cbtc); **

      ** add one positive time group from each time in tg **
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	KheClusterBusyTimesConstraintAddTimeGroup(cbtc,
	  KheTimeSingletonTimeGroup(KheTimeGroupTime(tg, j)), KHE_POSITIVE);

      ** add resource groups and resources **
      HaArrayForEach(c->resource_groups, rg, j)
	KheClusterBusyTimesConstraintAddResourceGroup(cbtc, rg);
      HaArrayForEach(c->resources, r, j)
	KheClusterBusyTimesConstraintAddResource(cbtc, r);
      if( DEBUG2 )
	KheConstraintDebug((KHE_CONSTRAINT) cbtc, 4, 2, stderr);
    }
    if( DEBUG2 )
      fprintf(stderr, "] end KheLimitBusyTimesConstraintFinalize recoding\n");
  }
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheLimitBusyTimesConstraintFinalize\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintDensityCount(                             */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintDensityCount(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return KheLimitBusyTimesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintAddTimeGroup(                            */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)                  */
/*                                                                           */
/*  Add tg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintAddTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)
{
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintTimeGroupCount(                           */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitBusyTimesConstraintTimeGroup(                     */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i, int offset)                  */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitBusyTimesConstraintTimeGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i, int offset)
{
  HnAssert(i >= 0 && i < HaArrayCount(c->time_groups),
    "KheLimitBusyTimesConstraintTimeGroup:  i (%d) out of range 0 .. %d",
    i, HaArrayCount(c->time_groups) - 1);
  if( offset == 0 )
    return HaArray(c->time_groups, i);
  else
    return KheTimeGroupNeighbour(HaArray(c->time_groups, i), offset);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitBusyTimesConstraintDomain(                        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the domain (the union of the time groups) of c.                   */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitBusyTimesConstraintDomain(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitBusyTimesConstraintDomain called before instance ends");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesConstraintLimitsWholeCycle(                        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return true when c contains a time group that limits the whole cycle.    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesConstraintLimitsWholeCycle(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitBusyTimesConstraintLimitsWholeCycle called before instance ends");
  return c->limits_whole_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintAddResourceGroup(                        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)              */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintAddResourceGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
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
/*  int KheLimitBusyTimesConstraintResourceGroupCount(                       */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintResourceGroupCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitBusyTimesConstraintResourceGroup(             */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitBusyTimesConstraintResourceGroup(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)
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
/*  void KheLimitBusyTimesConstraintAddResource(                             */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)                     */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintAddResource(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
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
/*  int KheLimitBusyTimesConstraintResourceCount(                            */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintResourceCount(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitBusyTimesConstraintResource(                        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitBusyTimesConstraintResource(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesConstraintResourceOfTypeCount(                      */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)               */
/*                                                                           */
/*  Return the number of resources in c of type rt.                          */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesConstraintResourceOfTypeCount(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE_TYPE rt)
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
/*  void DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(             */
/*    KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r,      */
/*    int offset)                                                            */
/*                                                                           */
/*  Make and attach the monitor for c that monitors r at offset.             */
/*                                                                           */
/*****************************************************************************/

static void DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(
  KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  KHE_RESOURCE_IN_SOLN rs, int offset)
{
  KHE_LIMIT_BUSY_TIMES_MONITOR m;
  m = KheLimitBusyTimesMonitorMake(rs, offset, c);
  KheMonitorAttachToSoln((KHE_MONITOR) m);
  KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoLimitBusyTimesConstraintMonitorsForResource(                      */
/*    KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)      */
/*                                                                           */
/*  Make and attach the monitors for this constraint that monitor r.         */
/*                                                                           */
/*****************************************************************************/

static void DoLimitBusyTimesConstraintMonitorsForResource(
  KHE_SOLN soln, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN rs;  int i, count;
  rs = KheSolnResourceInSoln(soln, r);
  count = KheLimitBusyTimesConstraintAppliesToOffsetCount(c);
  for( i = 0;  i < count;  i++ )
    DoLimitBusyTimesConstraintMonitorsForResourceAndOffset(soln, c, rs,
      KheLimitBusyTimesConstraintAppliesToOffset(c, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintMakeAndAttachMonitors(                   */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint, unless it is being     */
/*  recoded.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintMakeAndAttachMonitors(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  /* ***
  if( !c->limit_busy_recode )
  {
  *** */
    HaArrayForEach(c->resource_groups, rg, i)
    {
      for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      {
	r = KheResourceGroupResource(rg, j);
	DoLimitBusyTimesConstraintMonitorsForResource(soln, c, r);
      }
    }
    HaArrayForEach(c->resources, r, i)
      DoLimitBusyTimesConstraintMonitorsForResource(soln, c, r);
  /* ***
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesConstraintMakeFromKml(KML_ELT cons_elt,            */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit busy times constraint based on cons_elt and add it to ins.  */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name, *ref;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT res;  HA_ARENA a;
  int minimum, maximum;  bool allow_zero;  KHE_TIME_GROUP applies_to_tg;

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
	  " AppliesTo +AppliesToTimeGroup TimeGroups #Minimum #Maximum"
	  " +$AllowZero", ke) )
	return false;
      break;

    default:

      HnAbort("KheLimitBusyTimesConstraintMakeFromKml internal error");
      break;
  }
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum */
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
  allow_zero = true;  /* default value */
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
  if( !KheLimitBusyTimesConstraintMake(ins, id, name, reqd, wt, cf,
        applies_to_tg, minimum, maximum, allow_zero, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitBusyTimesConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitBusyTimesResourceCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitBusyTimesConstraint> applies to 0 resources");

  /* add the time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintWrite(                                   */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KML_FILE kf)                        */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintWrite(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i;  KHE_TIME_GROUP tg;
  KmlBegin(kf, "LimitBusyTimesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LimitBusyTimesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg)!=NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from LimitBusyTimesConstraint %s", c->id);
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
        "Resource referenced from LimitBusyTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  if( c->applies_to_tg != NULL )
    KmlEltAttribute(kf, "AppliesToTimeGroup", "Reference",
      KheTimeGroupId(c->applies_to_tg));
  KmlBegin(kf, "TimeGroups");
  HaArrayForEach(c->time_groups, tg, i)
  {
    HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
      "TimeGroup referenced from LimitBusyTimesConstraint %s", c->id);
    KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
  }
  KmlEnd(kf, "TimeGroups");
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  if( !c->allow_zero )
    KmlEltFmtText(kf, "AllowZero", "%s", "false");
  KmlEnd(kf, "LimitBusyTimesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesConstraintDebug(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesConstraintDebug(KHE_LIMIT_BUSY_TIMES_CONSTRAINT c,
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
	/* c->limit_busy_recode ? "true" : "false"); */
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

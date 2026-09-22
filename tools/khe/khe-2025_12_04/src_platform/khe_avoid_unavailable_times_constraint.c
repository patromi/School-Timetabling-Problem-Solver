
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
/*  FILE:         khe_avoid_unavailable_times_constraint.c                   */
/*  DESCRIPTION:  An avoid unavailable times constraint                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT - an avoid unavail. times constr. */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_unavailable_times_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  ARRAY_KHE_TIME_GROUP		time_groups;		/* the times         */
  ARRAY_KHE_TIME		times;			/* the times         */
  KHE_TIME_GROUP		unavailable_times;	/* all unavailables  */
  KHE_TIME_GROUP		available_times;	/* all availables    */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesConstraintMake(KHE_INSTANCE ins, char *id,  */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT *c)                             */
/*                                                                           */
/*  Make an avoid unavailable times constraint with these attributes, add    */
/*  it to ins, and return it.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidUnavailableTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT *c)
{
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheAvoidUnavailableTimesConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG, ins, id, name, required,
    weight, cf, a);
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  HaArrayInit(res->time_groups, a);
  HaArrayInit(res->times, a);
  res->unavailable_times = NULL;  /* set when finalizing */
  res->available_times = NULL;    /* set when finalizing */
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesConstraintAppliesToCount(                    */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintAppliesToCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintFinalize(                         */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintFinalize(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  int i;  KHE_TIME_GROUP tg;  KHE_TIME t;  SSET ss;  HA_ARENA a;

  /* find the unavailable times */
  a = KheInstanceArena(c->instance);
  if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 0 )
    c->unavailable_times = KheInstanceEmptyTimeGroup(c->instance);
  else if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 1 )
    c->unavailable_times = KheTimeSingletonTimeGroup(HaArrayFirst(c->times));
  else if( HaArrayCount(c->time_groups) == 1 && HaArrayCount(c->times) == 0 )
    c->unavailable_times = HaArrayFirst(c->time_groups);
  else
  {
    SSetInit(ss, a);
    HaArrayForEach(c->time_groups, tg, i)
      SSetUnion(ss, *KheTimeGroupTimeSet(tg));
    HaArrayForEach(c->times, t, i)
      SSetInsert(ss, KheTimeIndex(t));
    c->unavailable_times = KheTimeGroupMakeAndFinalize(c->instance,
      KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, /* NULL, */ false, a);
    if( DEBUG1 )
    {
      fprintf(stderr, "KheAvoidUnavailableTimesConstraintFinalize (a) %p: ",
	(void *) c->unavailable_times);
      KheTimeGroupDebug(c->unavailable_times, 2, 0, stderr);
    }
  }

  /* find the available times by complementing the unavailable times */
  SSetInit(ss, a);
  SSetUnion(ss, *KheTimeGroupTimeSet(KheInstanceFullTimeGroup(c->instance)));
  SSetDifference(ss, *KheTimeGroupTimeSet(c->unavailable_times));
  c->available_times = KheTimeGroupMakeAndFinalize(c->instance,
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, false, a);
  if( DEBUG1 )
  {
    fprintf(stderr, "KheAvoidUnavailableTimesConstraintFinalize (b) %p: ",
      (void *) c->available_times);
    KheTimeGroupDebug(c->available_times, 2, 0, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintFinalize(                         */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

/* *** version which attempted to optimize by building lsets first
void KheAvoidUnavailableTimesConstraintFinalize(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  int i;  KHE_TIME_GROUP tg;  KHE_TIME t;  LSET ls;

  ** find the unavailable times **
  if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 0 )
    c->unavailable_times = KheInstanceEmptyTimeGroup(c->instance);
  else if( HaArrayCount(c->time_groups) == 0 && HaArrayCount(c->times) == 1 )
    c->unavailable_times = KheTimeSingletonTimeGroup(HaArrayFirst(c->times));
  else if( HaArrayCount(c->time_groups) == 1 && HaArrayCount(c->times) == 0 )
    c->unavailable_times = HaArrayFirst(c->time_groups);
  else
  {
    ls = LSetNew();
    HaArrayForEach(c->time_groups, tg, i)
      LSetUnion(&ls, KheTimeGroupTime Set(tg));
    HaArrayForEach(c->times, t, i)
      LSetInsert(&ls, KheTimeIndex(t));
    if( !KheInstanceTimeGroupLSetTrieRetrieve(c->instance, ls,
	  &c->unavailable_times) )
    {
      c->unavailable_times = KheTimeGroup MakeInternal(c->instance,
	** KHE_TIME_GROUP_TYPE_CONSTRUCTED, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
	NULL, NULL, ls);
      KheInstanceTimeGroupLSetTrieInsert(c->instance, ls,c->unavailable_times);
      KheTimeGroup Finalize(c->unavailable_times, NULL, false, NULL, -1);
      if( DEBUG1 )
      {
	fprintf(stderr, "KheAvoidUnavailableTimesConstraintFinalize made %p: ",
	  (void *) c->unavailable_times);
	KheTimeGroupDebug(c->unavailable_times, 2, 0, stderr);
      }
    }
    else if( DEBUG1 )
    {
      fprintf(stderr, "KheAvoidUnavailableTimesConstraintFinalize found %p: ",
	(void *) c->unavailable_times);
      KheTimeGroupDebug(c->unavailable_times, 2, 0, stderr);
    }

    ** ***
    c->unavailable_times = KheTimeGroup MakeInternal(c->instance,
      KHE_TIME_GROUP_TYPE_CONSTRUCTED, NULL, KHE_TIME_GROUP_KIND_ORDINARY,
      NULL, NULL, LSetNew());
    HaArrayForEach(c->time_groups, tg, i)
      KheTimeGroupUnion Internal(c->unavailable_times, tg);
    HaArrayForEach(c->times, t, i)
      KheTimeGroupAdd TimeInternal(c->unavailable_times, t);
    KheTimeGroup Finalize(c->unavailable_times, NULL, NULL, -1);
    *** **
  }

  ** find the available times by complementing the unavailable times **
  ls = LSetNew();
  LSetUnion(&ls, KheTimeGroupTime Set(KheInstanceFullTimeGroup(c->instance)));
  LSetDifference(ls, KheTimeGroup TimeSet(c->unavailable_times));
  if( !KheInstanceTimeGroupLSetTrieRetrieve(c->instance, ls,
	&c->available_times) )
  {
    c->available_times = KheTimeGroup MakeInternal(c->instance,
      ** KHE_TIME_GROUP_TYPE_CONSTRUCTED, ** NULL, KHE_TIME_GROUP_KIND_AUTO,
      NULL, NULL, ls);
    KheInstanceTimeGroupLSetTrieInsert(c->instance, ls, c->available_times);
    KheTimeGroup Finalize(c->available_times, NULL, false, NULL, -1);
  }
  ** ***
  c->available_times = KheTimeGroup MakeInternal(c->instance,
    KHE_TIME_GROUP_TYPE_CONSTRUCTED, NULL, KHE_TIME_GROUP_KIND_ORDINARY,
    NULL, NULL, LSetNew());
  KheTimeGroupUnion Internal(c->available_times,
    KheInstanceFullTimeGroup(c->instance));
  KheTimeGroupDifferenceInternal(c->available_times, c->unavailable_times);
  KheTimeGroup Finalize(c->available_times, NULL, NULL, -1);
  *** **

}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesConstraintDensityCount(                      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintDensityCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return KheAvoidUnavailableTimesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintAddResourceGroup(                 */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)       */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintAddResourceGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
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
/*  int KheAvoidUnavailableTimesConstraintResourceGroupCount(                */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintResourceGroupCount(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheAvoidUnavailableTimesConstraintResourceGroup(      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheAvoidUnavailableTimesConstraintResourceGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)
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
/*  void KheAvoidUnavailableTimesConstraintAddResource(                      */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE r)              */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintAddResource(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_RESOURCE r)
{
  HaArrayAddLast(c->resources, r);
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesConstraintResourceCount(                     */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintResourceCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheAvoidUnavailableTimesConstraintResource(                 */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheAvoidUnavailableTimesConstraintResource(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintAddTimeGroup(                     */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)           */
/*                                                                           */
/*  Add tg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintAddTimeGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)
{
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesConstraintTimeGroupCount(                    */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of time groups in c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintTimeGroupCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintTimeGroup(              */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintTimeGroup(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->time_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "times"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintAddTime(                          */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME t)                  */
/*                                                                           */
/*  Add t to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintAddTime(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_TIME t)
{
  HnAssert(t != NULL, "KheAvoidUnavailableTimesConstraintAddTime: t is NULL");
  HaArrayAddLast(c->times, t);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidUnavailableTimesConstraintTimeCount(                         */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the number of times of c.                                         */
/*                                                                           */
/*****************************************************************************/

int KheAvoidUnavailableTimesConstraintTimeCount(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheAvoidUnavailableTimesConstraintTime(                         */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)                       */
/*                                                                           */
/*  Return the i'th time of c.                                               */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheAvoidUnavailableTimesConstraintTime(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->times, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "unavailable and available times"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintUnavailableTimes(       */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the unavailable times of c.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintUnavailableTimes(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  HnAssert(c->unavailable_times != NULL,
   "KheAvoidUnavailableTimesConstraintUnavailableTimes "
   "called before KheInstanceMakeEnd");
  return c->unavailable_times;
}

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintAvailableTimes(         */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)                              */
/*                                                                           */
/*  Return the available times of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheAvoidUnavailableTimesConstraintAvailableTimes(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c)
{
  HnAssert(c->available_times != NULL,
   "KheAvoidUnavailableTimesConstraintAvailableTimes "
   "called before KheInstanceMakeEnd");
  return c->available_times;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintMakeAndAttachMonitors(            */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_SOLN soln)               */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintMakeAndAttachMonitors(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR m;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheAvoidUnavailableTimesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  HaArrayForEach(c->resources, r, i)
  {
    rs = KheSolnResourceInSoln(soln, r);
    m = KheAvoidUnavailableTimesMonitorMake(rs, c);
    KheMonitorAttachToSoln((KHE_MONITOR) m);
    KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesConstraintMakeFromKml(KML_ELT cons_elt,     */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make an avoid unavailable times constraint based on cons_elt and         */
/*  add it to ins.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidUnavailableTimesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT res;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo +TimeGroups +Times", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* build and insert the constraint object */
  if( !KheAvoidUnavailableTimesConstraintMake(ins, id, name, reqd, wt,cf,&res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidUnavailableTimesConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheAvoidUnavailableTimesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<AvoidUnavailableTimesConstraint> applies to 0 resources");

  /* add the time groups and times */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  if( !KheConstraintAddTimesFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a) )
    return false;
  return true;

}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintWrite(                            */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KML_FILE kf)                 */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintWrite(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c, KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i;
  KHE_TIME_GROUP tg;  KHE_TIME t;
  KmlBegin(kf, "AvoidUnavailableTimesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in AvoidUnavailableTimesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from AvoidUnavailableTimesConstraint %s",
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
        "Resource referenced from AvoidUnavailableTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  if( HaArrayCount(c->time_groups) > 0 )
  {
    KmlBegin(kf, "TimeGroups");
    HaArrayForEach(c->time_groups, tg, i)
    {
      HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
        "TimeGroup referenced from AvoidUnavailableTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
    }
    KmlEnd(kf, "TimeGroups");
  }
  if( HaArrayCount(c->times) > 0 )
  {
    KmlBegin(kf, "Times");
    HaArrayForEach(c->times, t, i)
    {
      HnAssert(KheTimeId(t) != NULL, "KheArchiveWrite:  Id missing in "
        "Time referenced from AvoidUnavailableTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Time", "Reference", KheTimeId(t));
    }
    KmlEnd(kf, "Times");
  }
  KmlEnd(kf, "AvoidUnavailableTimesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidUnavailableTimesConstraintDebug(                            */
/*    KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c,                              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAvoidUnavailableTimesConstraintDebug(
  KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  KHE_TIME_GROUP tg;  KHE_TIME t;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[\n", indent, "");
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      HaArrayForEach(c->resources, r, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceId(r) != NULL ? KheResourceId(r) : "-");
      HaArrayForEach(c->time_groups, tg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-");
      HaArrayForEach(c->times, t, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheTimeId(t) != NULL ? KheTimeId(t) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

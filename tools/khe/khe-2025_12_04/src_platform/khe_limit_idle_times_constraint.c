
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
/*  FILE:         khe_limit_idle_times_constraint.c                          */
/*  DESCRIPTION:  A limit idle times constraint                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_IDLE_TIMES_CONSTRAINT - a limit idle times constraint          */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_idle_times_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* applies to        */
  ARRAY_KHE_RESOURCE		resources;		/* applies to        */
  int				minimum;		/* minimum           */
  int				maximum;		/* maximum           */
  ARRAY_KHE_TIME_GROUP		time_groups;		/* time groups       */
  bool				time_groups_disjoint;	/* if disjoint       */
  bool				time_groups_cover_cycle; /* if cover cycle   */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesConstraintMake(KHE_INSTANCE ins, char *id,         */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    int minimum, maximum, KHE_LIMIT_IDLE_TIMES_CONSTRAINT *c)              */
/*                                                                           */
/*  Make a new limit idle times constraint with these attributes, add it     */
/*  to ins, and return it.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int minimum, int maximum, KHE_LIMIT_IDLE_TIMES_CONSTRAINT *c)
{
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitIdleTimesConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_IDLE_TIMES_CONSTRAINT_TAG, ins, id, name, required, weight, cf,a);
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->minimum = minimum;
  res->maximum = maximum;
  HaArrayInit(res->time_groups, a);
  res->time_groups_disjoint = false;
  res->time_groups_cover_cycle = false;
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintMinimum(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintMinimum(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintMaximum(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintMaximum(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintAppliesToCount(                           */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintAppliesToCount(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  int i, res;  KHE_RESOURCE_GROUP rg;
  res = HaArrayCount(c->resources);
  HaArrayForEach(c->resource_groups, rg, i)
    res += KheResourceGroupResourceCount(rg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupsCoverTime(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,           */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Return true if t lies in any of c's time groups.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupsCoverTime(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
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
/*  bool KheLimitIdleTimesConstraintFinalize(                                */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KML_ERROR *ke)                      */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesConstraintFinalize(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
  KML_ERROR *ke)
{
  int i, j, count;  KHE_TIME_GROUP tg1, tg2;  KHE_TIME time;  HA_ARENA a;

  /* check compactness */
  a = KheInstanceArena(c->instance);
  HaArrayForEach(c->time_groups, tg1, i)
    if( !KheTimeGroupIsCompact(tg1) )
      return KmlError(ke, a, 0, 0, "in limit idle times constraint %s, time "
	"group  %d (%s) is not compact", KheConstraintId((KHE_CONSTRAINT) c),
	i+1, KheTimeGroupId(tg1) == NULL ? "with no id" : KheTimeGroupId(tg1));

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
  *ke = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintDensityCount(                             */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintDensityCount(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return KheLimitIdleTimesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesConstraintAddTimeGroup(                            */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)                  */
/*                                                                           */
/*  Add tg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintAddTimeGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_TIME_GROUP tg)
{
  /* *** checking compactness here is premature
  HnAssert(KheTimeGroupIsComp act(tg),
    "KheLimitIdleTimesConstraintAddTimeGroup: time group is not compact");
  *** */
  HaArrayAddLast(c->time_groups, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintTimeGroupCount(                           */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of time groups of c.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintTimeGroupCount(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitIdleTimesConstraintTimeGroup(                     */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th time group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitIdleTimesConstraintTimeGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)
{
  return HaArray(c->time_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesConstraintTimeGroupsDisjoint(                      */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return true if the time groups of c are pairwise disjoint.               */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesConstraintTimeGroupsDisjoint(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return c->time_groups_disjoint;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesConstraintTimeGroupsCoverWholeCycle(               */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return true if the time groups of c cover the whole cycle.               */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesConstraintTimeGroupsCoverWholeCycle(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return c->time_groups_cover_cycle;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesConstraintAddResourceGroup(                        */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)              */
/*                                                                           */
/*  Add rg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintAddResourceGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
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
/*  int KheLimitIdleTimesConstraintResourceGroupCount(                       */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resource groups in c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintResourceGroupCount(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitIdleTimesConstraintResourceGroup(             */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitIdleTimesConstraintResourceGroup(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)
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
/*  void KheLimitIdleTimesConstraintAddResource(                             */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_RESOURCE r)                     */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintAddResource(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
  KHE_RESOURCE r)
{
  HaArrayAddLast(c->resources, r);
  KheResourceAddConstraint(r, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesConstraintResourceCount(                            */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesConstraintResourceCount(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitIdleTimesConstraintResource(                        */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitIdleTimesConstraintResource(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, int i)
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
/*  void KheLimitIdleTimesConstraintMakeAndAttachMonitors(                   */
/*    KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_SOLN soln)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintMakeAndAttachMonitors(
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_RESOURCE_IN_SOLN rs;
  KHE_LIMIT_IDLE_TIMES_MONITOR m;
  HaArrayForEach(c->resource_groups, rg, i)
  {
    for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
    {
      r = KheResourceGroupResource(rg, j);
      rs = KheSolnResourceInSoln(soln, r);
      m = KheLimitIdleTimesMonitorMake(rs, c);
      KheMonitorAttachToSoln((KHE_MONITOR) m);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) soln, (KHE_MONITOR) m);
    }
  }
  HaArrayForEach(c->resources, r, i)
  {
    rs = KheSolnResourceInSoln(soln, r);
    m = KheLimitIdleTimesMonitorMake(rs, c);
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
/*  bool KheLimitIdleTimesConstraintMakeFromKml(KML_ELT cons_elt,            */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit idle times constraint based on cons_elt and add it to ins.  */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;  HA_ARENA a;
  KML_ELT elt;  KHE_LIMIT_IDLE_TIMES_CONSTRAINT res;  int minimum, maximum;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo TimeGroups #Minimum #Maximum", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum */
  sscanf(KmlText(KmlChild(cons_elt, 6)), "%d", &minimum);
  sscanf(KmlText(KmlChild(cons_elt, 7)), "%d", &maximum);

  /* build and insert the constraint object */
  if( !KheLimitIdleTimesConstraintMake(ins, id, name, reqd, wt, cf,
        minimum, maximum, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitIdleTimesConstraint> Id \"%s\" used previously", id);

  /* add the resource groups and resources */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +ResourceGroups +Resources", ke) )
    return false;
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitIdleTimesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitIdleTimesConstraint> applies to 0 resources");


  /* add the time groups */
  if( !KheConstraintAddTimeGroupsFromKml((KHE_CONSTRAINT) res, cons_elt, ke,a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesConstraintWrite(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintWrite(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
  KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  int i;  KHE_TIME_GROUP tg;
  KmlBegin(kf, "LimitIdleTimesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LimitIdleTimesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing in "
        "ResourceGroup referenced from LimitIdleTimesConstraint %s", c->id);
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
        "Resource referenced from LimitIdleTimesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  KmlEnd(kf, "AppliesTo");
  KmlBegin(kf, "TimeGroups");
  HaArrayForEach(c->time_groups, tg, i)
  {
    HnAssert(KheTimeGroupId(tg) != NULL, "KheArchiveWrite:  Id missing in "
      "TimeGroup referenced from LimitIdleTimesConstraint %s", c->id);
    KmlEltAttribute(kf, "TimeGroup", "Reference", KheTimeGroupId(tg));
  }
  KmlEnd(kf, "TimeGroups");
  KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  KmlEltFmtText(kf, "Maximum", "%d", c->maximum);
  KmlEnd(kf, "LimitIdleTimesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesConstraintDebug(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c, */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesConstraintDebug(KHE_LIMIT_IDLE_TIMES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[ min %d, max %d\n", indent, "", c->minimum, c->maximum);
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

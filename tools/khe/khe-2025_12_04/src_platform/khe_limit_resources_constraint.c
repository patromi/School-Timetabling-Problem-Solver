
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
/*  FILE:         khe_limit_resources_constraint.c                           */
/*  DESCRIPTION:  A limit resources constraint                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  LRC_POINT - one point of application of a limit resources constraint     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_lrc_eg_point_rec {
  KHE_EVENT_GROUP		event_group;		/* event group       */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* event resources   */
} *LRC_POINT;

typedef HA_ARRAY(LRC_POINT) ARRAY_LRC_POINT;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_RESOURCES_CONSTRAINT - a limit resources constraint            */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_resources_constraint_rec {
  INHERIT_CONSTRAINT
  ARRAY_LRC_POINT		lrc_points;		/* points of applic. */
  KHE_RESOURCE_TYPE		resource_type;		/* resource type     */
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* resource groups   */
  ARRAY_KHE_RESOURCE		resources;		/* resources         */
  KHE_RESOURCE_GROUP		domain; 		/* total of above    */
  KHE_RESOURCE_GROUP		neg_domain; 		/* complement of d.  */
  int				minimum;		/* minimum           */
  int				maximum;		/* maximum           */
  HA_ARRAY_NSTRING		roles;			/* roles             */
};


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesConstraintMake(KHE_INSTANCE ins, char *id,         */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    int minimum, int maximum, KHE_LIMIT_RESOURCES_CONSTRAINT *c)           */
/*                                                                           */
/*  Make a new limit resources constraint with these attributes, add it to   */
/*  ins, and return it.                                                      */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  int minimum, int maximum, KHE_LIMIT_RESOURCES_CONSTRAINT *c)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheLimitResourcesConstraintMake called after KheInstanceMakeEnd");
  HnAssert(KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE,
    "KheLimitResourcesConstraintMake called in high school model");
  HnAssert(minimum <= maximum, "KheLimitResourcesConstraintMake: "
    "minimum (%d) exceeds maximum (%d)", minimum, maximum);
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_LIMIT_RESOURCES_CONSTRAINT_TAG, ins, id, name, required, weight, cf, a);
  HaArrayInit(res->lrc_points, a);
  res->resource_type = NULL;
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->domain = NULL;
  res->neg_domain = NULL;
  res->minimum = minimum;
  res->maximum = maximum;
  HaArrayInit(res->roles, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintMinimum(KHE_LIMIT_RESOURCES_CONSTRAINT c) */
/*                                                                           */
/*  Return the minimum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintMinimum(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return c->minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintMaximum(KHE_LIMIT_RESOURCES_CONSTRAINT c) */
/*                                                                           */
/*  Return the maximum attribute of c.                                       */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintMaximum(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return c->maximum;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintAppliesToCount(                           */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintAppliesToCount(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->lrc_points);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintDensityCount(                             */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the density count of c.                                           */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintDensityCount(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  int i, res;
  res = 0;
  for( i = 0; i < KheLimitResourcesConstraintEventGroupCount(c);  i++ )
    res += KheLimitResourcesConstraintEventResourceCount(c, i);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeIsConsistent(KHE_LIMIT_RESOURCES_CONSTRAINT c,       */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Return true if rt is consistent with the resource type of c, possibly    */
/*  setting the resource type of c.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeIsConsistent(KHE_LIMIT_RESOURCES_CONSTRAINT c,
  KHE_RESOURCE_TYPE rt)
{
  if( c->resource_type == NULL )
    c->resource_type = rt;
  return c->resource_type == rt;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesConstraintAddResourceGroup(                        */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)               */
/*                                                                           */
/*  Add rg to c, unless resource type is inconsistent.                       */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesConstraintAddResourceGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  if( !KheResourceTypeIsConsistent(c, KheResourceGroupResourceType(rg)) )
    return false;
  HaArrayAddLast(c->resource_groups, rg);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintResourceGroupCount(                       */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of resource groups of c.                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintResourceGroupCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitResourcesConstraintResourceGroup(             */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitResourcesConstraintResourceGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)
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
/*  bool KheLimitResourcesConstraintAddResource(                             */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE r)                      */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesConstraintAddResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_RESOURCE r)
{
  if( !KheResourceTypeIsConsistent(c, KheResourceResourceType(r)) )
    return false;
  HaArrayAddLast(c->resources, r);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintResourceCount(                            */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintResourceCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitResourcesConstraintResource(                        */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitResourcesConstraintResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "domain"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomain(                    */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the domain of c (the union of its resource groups and resources). */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomain(
  KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KheLimitResourcesConstraintDomain called before KheInstanceMakeEnd");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomainComplement(          */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the complement of the domain of c.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheLimitResourcesConstraintDomainComplement(
  KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  HnAssert(c->neg_domain != NULL, "KheLimitResourcesConstraintDomainComplement"
    " called before KheInstanceMakeEnd");
  return c->neg_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintAddEventGroup(                           */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg)                  */
/*                                                                           */
/*  Add eg to c.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintAddEventGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg)
{
  LRC_POINT p;  HA_ARENA a;
  a = KheInstanceArena(c->instance);
  HaMake(p, a);
  p->event_group = eg;
  HaArrayInit(p->event_resources, a);
  HaArrayAddLast(c->lrc_points, p);
  if( eg != NULL )
    KheEventGroupAddConstraint(eg, (KHE_CONSTRAINT) c);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintEventGroupCount(                          */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of event groups in c.                                  */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintEventGroupCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->lrc_points);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KheLimitResourcesConstraintEventGroup(                   */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)                               */
/*                                                                           */
/*  Return the i'th event group of c.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheLimitResourcesConstraintEventGroup(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->lrc_points, i)->event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "roles"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintAddRole(KHE_LIMIT_RESOURCES_CONSTRAINT c,*/
/*    char *role)                                                            */
/*                                                                           */
/*  Add role to c.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintAddRole(KHE_LIMIT_RESOURCES_CONSTRAINT c,
  char *role)
{
  HA_ARENA a = KheInstanceArena(c->instance);
  HaArrayAddLast(c->roles, HnStringCopy(role, a));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintRoleCount(                                */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Return the number of roles in c.                                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintRoleCount(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->roles);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitResourcesConstraintRole(KHE_LIMIT_RESOURCES_CONSTRAINT c,  */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th role of c.                                               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitResourcesConstraintRole(KHE_LIMIT_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->roles, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintAddEventResource(                        */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index,                        */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Add er to the eg_index'th point of application of c.                     */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintAddEventResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index, KHE_EVENT_RESOURCE er)
{
  LRC_POINT ap;
  HnAssert(eg_index >= 0 && eg_index < HaArrayCount(c->lrc_points),
    "KheLimitResourcesConstraintAddEventResource: eg_index (%d) out of "
    "range (0 .. %d)", eg_index, HaArrayCount(c->lrc_points) - 1);
  ap = HaArray(c->lrc_points, eg_index);
  HnAssert(ap->event_group == NULL,
   "KheLimitResourcesConstraintAddEventResource: eg_index (%d) has event group",
   eg_index);
  HaArrayAddLast(ap->event_resources, er);
  KheEventResourceAddConstraint(er, (KHE_CONSTRAINT) c, eg_index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesConstraintEventResourceCount(                       */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index)                        */
/*                                                                           */
/*  Return the number of event resources of the eg_index'th set of event     */
/*  resources of c.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesConstraintEventResourceCount(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index)
{
  LRC_POINT ap;
  HnAssert(eg_index >= 0 && eg_index < HaArrayCount(c->lrc_points),
    "KheLimitResourcesConstraintEventResourceCount: eg_index (%d) out of "
    "range (0 .. %d)", eg_index, HaArrayCount(c->lrc_points) - 1);
  ap = HaArray(c->lrc_points, eg_index);
  return HaArrayCount(ap->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheLimitResourcesConstraintEventResource(             */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index, int er_index)          */
/*                                                                           */
/*  Return the er_index'th event resource of point of appl. eg_index.        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheLimitResourcesConstraintEventResource(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index, int er_index)
{
  LRC_POINT ap;
  ap = HaArray(c->lrc_points, eg_index);
  return HaArray(ap->event_resources, er_index);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "finalize"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintFinalize(                                */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c)                                      */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintFinalize(KHE_LIMIT_RESOURCES_CONSTRAINT c)
{
  int i, j, k, eg_index;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  LRC_POINT ap;  KHE_EVENT e;  char *role;  KHE_EVENT_RESOURCE er;

  /* set the domain of c */
  if( HaArrayCount(c->resource_groups) == 0 && HaArrayCount(c->resources)==1 )
    c->domain = KheResourceSingletonResourceGroup(HaArrayFirst(c->resources));
  else if( HaArrayCount(c->resource_groups) == 1 && HaArrayCount(c->resources)==0 )
    c->domain = HaArrayFirst(c->resource_groups);
  else
  {
    c->domain = KheResourceGroupMakeInternal(c->resource_type,
      KHE_RESOURCE_GROUP_TYPE_CONSTRUCTED, NULL, NULL, NULL);
    HaArrayForEach(c->resource_groups, rg, i)
      KheResourceGroupUnionInternal(c->domain, rg);
    HaArrayForEach(c->resources, r, i)
      KheResourceGroupAddResourceInternal(c->domain, r);
    KheResourceGroupFinalize(c->domain);
  }

  /* set the complement of the domain of c */
  c->neg_domain = KheResourceGroupMakeInternal(c->resource_type,
    KHE_RESOURCE_GROUP_TYPE_CONSTRUCTED, NULL, NULL, NULL);
  KheResourceGroupUnionInternal(c->neg_domain,
    KheResourceTypeFullResourceGroup(KheResourceGroupResourceType(c->domain)));
  KheResourceGroupDifferenceInternal(c->neg_domain, c->domain);
  KheResourceGroupFinalize(c->neg_domain);

  /* add the event resources of c */
  HaArrayForEach(c->lrc_points, ap, eg_index)
    if( ap->event_group != NULL )
    {
      for( j = 0;  j < KheEventGroupEventCount(ap->event_group);  j++ )
      {
	e = KheEventGroupEvent(ap->event_group, j);
	HaArrayForEach(c->roles, role, k)
	  if( KheEventRetrieveEventResource(e, role, &er) )
	  {
	    HaArrayAddLast(ap->event_resources, er);
	    KheEventResourceAddConstraint(er, (KHE_CONSTRAINT) c, eg_index);
	  }
      }
    }

  /* record the number of points of application */
  if( c->resource_type != NULL )
    KheResourceTypeIncrementLimitResourcesCount(c->resource_type,
      HaArrayCount(c->lrc_points));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesConstraintDoEquiv(                                 */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c1, KHE_LIMIT_RESOURCES_CONSTRAINT c2)  */
/*                                                                           */
/*  Return true if these produce the same penalties when the same resource   */
/*  is assigned.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesConstraintDoEquiv(
  KHE_LIMIT_RESOURCES_CONSTRAINT c1, KHE_LIMIT_RESOURCES_CONSTRAINT c2)
{
  return c1->minimum == c2->minimum && c1->maximum == c2->maximum &&
    KheResourceGroupEqual(c1->domain, c2->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintMakeAndAttachMonitors(                   */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_SOLN soln)                       */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintMakeAndAttachMonitors(
  KHE_LIMIT_RESOURCES_CONSTRAINT c, KHE_SOLN soln)
{
  int i;  KHE_LIMIT_RESOURCES_MONITOR m;
  for( i = 0;  i < KheLimitResourcesConstraintEventGroupCount(c);  i++ )
  {
    m = KheLimitResourcesMonitorMake(soln, c, i);
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
/*  bool KheLimitResourcesConstraintMakeFromKml(KML_ELT cons_elt,            */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a limit resources constraint based on cons_elt and add it to ins.   */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt, minimum, maximum, i;  HA_ARENA a;
  KHE_COST_FUNCTION cf;  KML_ELT elt;  KHE_LIMIT_RESOURCES_CONSTRAINT res;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight $CostFunction "
      "AppliesTo +ResourceGroups +Resources +#Minimum +#Maximum Roles", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* get minimum and maximum */
  if( KmlContainsChild(cons_elt, "Minimum", &elt) )
    sscanf(KmlText(elt), "%d", &minimum);
  else
    minimum = 0;
  if( KmlContainsChild(cons_elt, "Maximum", &elt) )
    sscanf(KmlText(elt), "%d", &maximum);
  else
    maximum = INT_MAX;

  /* build and insert the constraint object */
  if( !KheLimitResourcesConstraintMake(ins, id, name, reqd, wt, cf,
        minimum, maximum, &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitResourcesConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KheLimitResourcesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<LimitResourcesConstraint> applies to 0 event groups/events");

  /* add the resource groups and resources */
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT)res,cons_elt,ke,a))
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a) )
    return false;

  /* add the roles */
  if( KmlContainsChild(cons_elt, "Roles", &elt) )
  {
    if( !KmlCheck(elt, ": *Role", ke) )
      return false;
    for( i = 0;  i < KmlChildCount(elt);  i++ )
      KheLimitResourcesConstraintAddRole(res, KmlText(KmlChild(elt, i)));
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintWrite(KHE_LIMIT_RESOURCES_CONSTRAINT c,  */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintWrite(KHE_LIMIT_RESOURCES_CONSTRAINT c,
  KML_FILE kf)
{
  LRC_POINT ap;  int i;  bool started;  KHE_EVENT e;  char *role;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in LimitResourcesConstraint");
  KmlBegin(kf, "LimitResourcesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);

  /* AppliesTo/EventGroups */
  KmlBegin(kf, "AppliesTo");
  started = false;
  HaArrayForEach(c->lrc_points, ap, i)
  {
    HnAssert(ap->event_group != NULL,
      "KheArchiveWrite:  LimitResourcesConstraint %s was constructed, not "
      "read, and cannot be written", c->id);
    if( KheEventGroupEventCount(ap->event_group) != 1 )
    {
      if( !started )
      {
	KmlBegin(kf, "EventGroups");
	started = true;
      }
      HnAssert(KheEventGroupId(ap->event_group) != NULL,
	"KheArchiveWrite:  Id missing in EventGroup referenced from"
	" LimitResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "EventGroup", "Reference",
	KheEventGroupId(ap->event_group));
    }
  }
  if( started )
    KmlEnd(kf, "EventGroups");

  /* AppliesTo/Events */
  started = false;
  HaArrayForEach(c->lrc_points, ap, i)
  {
    if( KheEventGroupEventCount(ap->event_group) == 1 )
    {
      if( !started )
      {
	KmlBegin(kf, "Events");
	started = true;
      }
      e = KheEventGroupEvent(ap->event_group, 0);
      HnAssert(KheEventId(e) != NULL,
	"KheArchiveWrite:  Id missing in Event referenced from"
	" LimitResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
  }
  if( started )
    KmlEnd(kf, "Events");
  KmlEnd(kf, "AppliesTo");

  /* ResourceGroups */
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing in "
	"ResourceGroup referenced from LimitResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", KheResourceGroupId(rg));
    }
    KmlEnd(kf, "ResourceGroups");
  }

  /* Resources */
  if( HaArrayCount(c->resources) > 0 )
  {
    KmlBegin(kf, "Resources");
    HaArrayForEach(c->resources, r, i)
    {
      HnAssert(KheResourceId(r) != NULL, "KheArchiveWrite:  Id missing in "
        "Resource referenced from LimitResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }

  /* Minimum and Maximum */
  if( c->minimum > 0 )
    KmlEltFmtText(kf, "Minimum", "%d", c->minimum);
  if( c->maximum < INT_MAX )
    KmlEltFmtText(kf, "Maximum", "%d", c->maximum);

  /* roles */
  KmlBegin(kf, "Roles");
  HaArrayForEach(c->roles, role, i)
    KmlEltPlainText(kf, "Role", role);
  KmlEnd(kf, "Roles");
  KmlEnd(kf, "LimitResourcesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesConstraintDebug(KHE_LIMIT_RESOURCES_CONSTRAINT c,  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesConstraintDebug(KHE_LIMIT_RESOURCES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      /* details still to do here */
      fprintf(fp, "%*s[\n", indent, "");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

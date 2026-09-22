
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
/*  FILE:         khe_prefer_resources_constraint.c                          */
/*  DESCRIPTION:  A prefer resources constraint                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_RESOURCES_CONSTRAINT - a prefer resources constraint          */
/*                                                                           */
/*****************************************************************************/

struct khe_prefer_resources_constraint_rec {
  INHERIT_CONSTRAINT
  char				*role;			/* Role              */
  KHE_RESOURCE_TYPE		resource_type;		/* resource type     */
  ARRAY_KHE_RESOURCE_GROUP	resource_groups;	/* resource groups   */
  ARRAY_KHE_RESOURCE		resources;		/* resources         */
  KHE_RESOURCE_GROUP		domain; 		/* total of above    */
  KHE_RESOURCE_GROUP		neg_domain; 		/* complement of d.  */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* applies to        */
  ARRAY_KHE_EVENT		events;			/* Events            */
  ARRAY_KHE_EVENT_GROUP		event_groups;		/* EventGroups       */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesConstraintMake(KHE_INSTANCE ins, char *id,        */
/*    char *name, bool required, int weight, KHE_COST_FUNCTION cf,           */
/*    char *role, KHE_PREFER_RESOURCES_CONSTRAINT *c)                        */
/*                                                                           */
/*  Make a prefer resources constraint, add it to the instance, and          */
/*  return it.                                                               */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintMake(KHE_INSTANCE ins, char *id,
  char *name, bool required, int weight, KHE_COST_FUNCTION cf,
  char *role, KHE_PREFER_RESOURCES_CONSTRAINT *c)
{
  KHE_PREFER_RESOURCES_CONSTRAINT res;  KHE_CONSTRAINT cc;  HA_ARENA a;
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KhePreferResourcesConstraintMake called after KheInstanceMakeEnd");
  if( id != NULL && KheInstanceRetrieveConstraint(ins, id, &cc) )
  {
    *c = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  KheConstraintInitCommonFields((KHE_CONSTRAINT) res,
    KHE_PREFER_RESOURCES_CONSTRAINT_TAG, ins, id, name, required, weight, cf,a);
  res->role = HnStringCopy(role, a);
  res->resource_type = NULL;
  HaArrayInit(res->resource_groups, a);
  HaArrayInit(res->resources, a);
  res->domain = NULL;
  res->neg_domain = NULL;
  HaArrayInit(res->events, a);
  HaArrayInit(res->event_groups, a);
  HaArrayInit(res->event_resources, a);
  KheInstanceAddConstraint(ins, (KHE_CONSTRAINT) res);
  *c = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePreferResourcesConstraintRole(KHE_PREFER_RESOURCES_CONSTRAINT c)*/
/*                                                                           */
/*  Return the role of c.                                                    */
/*                                                                           */
/*****************************************************************************/

char *KhePreferResourcesConstraintRole(KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return c->role;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventApplies(KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT e,     */
/*    KHE_EVENT_RESOURCE *er)                                                */
/*                                                                           */
/*  Return true if e is applicable to c; in that case, also set *er to the   */
/*  event resource it applies to.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheEventApplies(KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT e,
  KHE_EVENT_RESOURCE *er)
{
  return c->role != NULL && KheEventRetrieveEventResource(e, c->role, er) &&
    KheEventResourcePreassignedResource(*er) == NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeIsConsistent(KHE_PREFER_RESOURCES_CONSTRAINT c,      */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Return true if rt is consistent with the resource type of c, possibly    */
/*  setting the resource type of c.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeIsConsistent(KHE_PREFER_RESOURCES_CONSTRAINT c,
  KHE_RESOURCE_TYPE rt)
{
  if( c->resource_type == NULL )
    c->resource_type = rt;
  return c->resource_type == rt;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintAppliesToCount(                          */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of points of application of c.                         */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintAppliesToCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferResourcesConstraintFinalize(                               */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Finalize c, since KheInstanceMakeEnd has been called.                    */
/*                                                                           */
/*  Implementation note.  What needs to be done is to set the domain of c.   */
/*                                                                           */
/*****************************************************************************/

void KhePreferResourcesConstraintFinalize(KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;

  /* set the domain of c */
  if( HaArrayCount(c->resource_groups) == 0 && HaArrayCount(c->resources) == 1 )
    c->domain = KheResourceSingletonResourceGroup(HaArrayFirst(c->resources));
  else if( HaArrayCount(c->resource_groups) == 1 &&
    HaArrayCount(c->resources)==0 )
    c->domain = HaArrayFirst(c->resource_groups);
  else
  {
    c->domain = KheResourceGroupMakeInternal(c->resource_type,
      KHE_RESOURCE_GROUP_TYPE_CONSTRUCTED, NULL, NULL, NULL /* , LSetNew() */);
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
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesConstraintDoEquiv(                                */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c1, KHE_PREFER_RESOURCES_CONSTRAINT c2)*/
/*                                                                           */
/*  Return true if penalizing from c1 is the same as penalizing from c2.     */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintDoEquiv(
  KHE_PREFER_RESOURCES_CONSTRAINT c1, KHE_PREFER_RESOURCES_CONSTRAINT c2)
{
  return KheResourceGroupEqual(c1->domain, c2->domain);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintDensityCount(                            */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the density count of c; just the applies to count in this case.   */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintDensityCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return KhePreferResourcesConstraintAppliesToCount(c);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesConstraintAddResourceGroup(                       */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)              */
/*                                                                           */
/*  Add rg to c, unless resource type is inconsistent.                       */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintAddResourceGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE_GROUP rg)
{
  if( !KheResourceTypeIsConsistent(c, KheResourceGroupResourceType(rg)) )
    return false;
  HaArrayAddLast(c->resource_groups, rg);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintResourceGroupCount(                      */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resource groups of c.                               */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintResourceGroupCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KhePreferResourcesConstraintResourceGroup(            */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource group of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KhePreferResourcesConstraintResourceGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i)
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
/*  bool KhePreferResourcesConstraintAddResource(                            */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE r)                     */
/*                                                                           */
/*  Add r to c.                                                              */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintAddResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_RESOURCE r)
{
  if( !KheResourceTypeIsConsistent(c, KheResourceResourceType(r)) )
    return false;
  HaArrayAddLast(c->resources, r);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintResourceCount(                           */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of resources of c.                                     */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintResourceCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KhePreferResourcesConstraintResource(                       */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th resource of c.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KhePreferResourcesConstraintResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i)
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
/*  KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomain(                   */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the domain of c (the union of its resource groups and resources). */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomain(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  HnAssert(c->domain != NULL,
    "KhePreferResourcesConstraintDomain called before KheInstanceMakeEnd");
  return c->domain;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomainComplement(         */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the complement of the domain of c.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KhePreferResourcesConstraintDomainComplement(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  HnAssert(c->neg_domain != NULL, "KhePreferResourcesConstraintDomainComplement"
    " called before KheInstanceMakeEnd");
  return c->neg_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesConstraintAddEventResource(                       */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_RESOURCE er)              */
/*                                                                           */
/*  Add er to c, and also add c to er.                                       */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintAddEventResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_RESOURCE er)
{
  if( !KheResourceTypeIsConsistent(c, KheEventResourceResourceType(er)) )
    return false;
  HaArrayAddLast(c->event_resources, er);
  KheEventResourceAddConstraint(er, (KHE_CONSTRAINT) c, -1);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintEventResourceCount(                      */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of event resources that c applies to.                  */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintEventResourceCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KhePreferResourcesConstraintEventResource(            */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th event resource of c.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KhePreferResourcesConstraintEventResource(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->event_resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "events"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferResourcesConstraintAddEvent(                               */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT e)                        */
/*                                                                           */
/*  Add e to c, and also add the corresponding event resource.               */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintAddEvent(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT e)
{
  KHE_EVENT_RESOURCE er;
  if( KheEventApplies(c, e, &er) )
  {
    if( !KhePreferResourcesConstraintAddEventResource(c, er) )
      return false;
    HaArrayAddLast(c->events, e);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintEventCount(                              */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of events of c.                                        */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintEventCount(KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KhePreferResourcesConstraintEvent(                             */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the i'th event of c.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KhePreferResourcesConstraintEvent(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->events, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event groups"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesConstraintAddEventGroup(                          */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg,                 */
/*    KHE_EVENT *problem_event)                                              */
/*                                                                           */
/*  Add eg to c, and also add the corresponding event resources to c.        */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintAddEventGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_EVENT_GROUP eg,
  KHE_EVENT *problem_event)
{
  int i;  KHE_EVENT e;  KHE_EVENT_RESOURCE er;
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    if( KheEventApplies(c, e, &er) )
    {
      if( !KhePreferResourcesConstraintAddEventResource(c, er) )
      {
	*problem_event = e;
	return false;
      }
    }
  }
  HaArrayAddLast(c->event_groups, eg);
  *problem_event = NULL;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferResourcesConstraintEventGroupCount(                         */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c)                                     */
/*                                                                           */
/*  Return the number of event groups of c.                                  */
/*                                                                           */
/*****************************************************************************/

int KhePreferResourcesConstraintEventGroupCount(
  KHE_PREFER_RESOURCES_CONSTRAINT c)
{
  return HaArrayCount(c->event_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_GROUP KhePreferResourcesConstraintEventGroup(                  */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, int i)                              */
/*                                                                           */
/*  Return the ith event group of c.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KhePreferResourcesConstraintEventGroup(
  KHE_PREFER_RESOURCES_CONSTRAINT c, int i)
{
  return HaArray(c->event_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "modules"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferResourcesConstraintMakeAndAttachMonitors(                  */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_SOLN soln)                      */
/*                                                                           */
/*  Make and attach the monitors for this constraint.                        */
/*                                                                           */
/*****************************************************************************/

void KhePreferResourcesConstraintMakeAndAttachMonitors(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_SOLN soln)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_PREFER_RESOURCES_MONITOR m;
  HaArrayForEach(c->event_resources, er, i)
  {
    ers = KheSolnEventResourceInSoln(soln, er);
    m = KhePreferResourcesMonitorMake(ers, c);
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
/*  bool KhePreferResourcesConstraintMakeFromKml(KML_ELT cons_elt,           */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Add a prefer resources constraint based on cons_elt to ins.              */
/*                                                                           */
/*****************************************************************************/

bool KhePreferResourcesConstraintMakeFromKml(KML_ELT cons_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  bool reqd;  int wt;  KHE_COST_FUNCTION cf;
  KML_ELT elt;  KHE_PREFER_RESOURCES_CONSTRAINT res;  HA_ARENA a;

  /* verify cons_elt and get the common fields */
  a = KheInstanceArena(ins);
  if( !KmlCheck(cons_elt, "Id : $Name $Required #Weight "
      "$CostFunction AppliesTo +ResourceGroups +Resources $Role", ke) )
    return false;
  if( !KheConstraintCheckKml(cons_elt, &id, &name, &reqd, &wt, &cf, ke, a) )
    return false;

  /* make the constraint object and add it to ins */
  if( !KhePreferResourcesConstraintMake(ins, id, name, reqd, wt, cf,
	KmlText(KmlChild(cons_elt, -1)), &res) )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<PreferResourcesConstraint> Id \"%s\" used previously", id);

  /* add the event groups and events */
  elt = KmlChild(cons_elt, 4);
  if( !KmlCheck(elt, ": +EventGroups +Events", ke) )
    return false;
  if( !KheConstraintAddEventGroupsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( !KheConstraintAddEventsFromKml((KHE_CONSTRAINT) res, elt, ke, a) )
    return false;
  if( KhePreferResourcesConstraintAppliesToCount(res) == 0 )
    return KmlError(ke, a, KmlLineNum(cons_elt), KmlColNum(cons_elt),
      "<PreferResourcesConstraint> Id \"%s\" applies to 0 event resources", id);

  /* add the resource groups and resources */
  if( !KheConstraintAddResourceGroupsFromKml((KHE_CONSTRAINT)res,cons_elt,ke,a))
    return false;
  if( !KheConstraintAddResourcesFromKml((KHE_CONSTRAINT) res, cons_elt, ke, a) )
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferResourcesConstraintWrite(                                  */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KML_FILE kf)                        */
/*                                                                           */
/*  Write c to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KhePreferResourcesConstraintWrite(
  KHE_PREFER_RESOURCES_CONSTRAINT c, KML_FILE kf)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  KmlBegin(kf, "PreferResourcesConstraint");
  HnAssert(c->id != NULL,
    "KheArchiveWrite: Id missing in PreferResourcesConstraint");
  KmlAttribute(kf, "Id", c->id);
  KheConstraintWriteCommonFields((KHE_CONSTRAINT) c, kf);
  KmlBegin(kf, "AppliesTo");
  if( HaArrayCount(c->event_groups) > 0 )
  {
    KmlBegin(kf, "EventGroups");
    HaArrayForEach(c->event_groups, eg, i)
    {
      HnAssert(KheEventGroupId(eg) != NULL, "KheArchiveWrite:  Id missing"
        " in EventGroup referenced from PreferResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "EventGroup", "Reference", KheEventGroupId(eg));
    }
    KmlEnd(kf, "EventGroups");
  }
  if( HaArrayCount(c->events) > 0 )
  {
    KmlBegin(kf, "Events");
    HaArrayForEach(c->events, e, i)
    {
      HnAssert(KheEventId(e) != NULL, "KheArchiveWrite:  Id missing"
        " in Event referenced from PreferResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "Event", "Reference", KheEventId(e));
    }
    KmlEnd(kf, "Events");
  }
  KmlEnd(kf, "AppliesTo");
  if( HaArrayCount(c->resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(c->resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg) != NULL, "KheArchiveWrite:  Id missing in"
        " ResourceGroup referenced from PreferResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", KheResourceGroupId(rg));
    }
    KmlEnd(kf, "ResourceGroups");
  }
  if( HaArrayCount(c->resources) > 0 )
  {
    KmlBegin(kf, "Resources");
    HaArrayForEach(c->resources, r, i)
    {
      HnAssert(KheResourceId(r) != NULL, "KheArchiveWrite:  Id missing in"
        " Resource referenced from PreferResourcesConstraint %s", c->id);
      KmlEltAttribute(kf, "Resource", "Reference", KheResourceId(r));
    }
    KmlEnd(kf, "Resources");
  }
  HnAssert(c->role != NULL,
    "KheArchiveWrite: Role missing in PreferResourcesConstraint %s", c->id);
  KmlEltPlainText(kf, "Role", c->role);
  KmlEnd(kf, "PreferResourcesConstraint");
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferResourcesConstraintDebug(KHE_PREFER_RESOURCES_CONSTRAINT c,*/
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of c onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KhePreferResourcesConstraintDebug(KHE_PREFER_RESOURCES_CONSTRAINT c,
  int verbosity, int indent, FILE *fp)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  int i;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;
  if( verbosity >= 1 )
  {
    KheConstraintDebugCommonFields((KHE_CONSTRAINT) c, indent, fp);
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "%*s[ %s:\n", indent, "", c->role);
      HaArrayForEach(c->event_groups, eg, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventGroupId(eg) != NULL ? KheEventGroupId(eg) : "-");
      HaArrayForEach(c->events, e, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheEventId(e) != NULL ? KheEventId(e) : "-");
      HaArrayForEach(c->resource_groups, rg, i)
        fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceGroupId(rg) != NULL ? KheResourceGroupId(rg) : "-");
      HaArrayForEach(c->resources, r, i)
	fprintf(fp, "%*s  %s\n", indent, "",
	  KheResourceId(r) != NULL ? KheResourceId(r) : "-");
      fprintf(fp, "%*s]\n", indent, "");
    }
  }
}

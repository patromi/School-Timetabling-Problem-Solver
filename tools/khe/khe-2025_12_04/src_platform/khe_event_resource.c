
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
/*  FILE:         khe_event_resource.c                                       */
/*  DESCRIPTION:  A resource member of an event                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0	/* needs_assignment */
#define DEBUG4 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE - a resource member of an event                       */
/*                                                                           */
/*****************************************************************************/

struct khe_event_resource_rec {
  void			*back;			/* back pointer              */
  KHE_EVENT		event;			/* enclosing event           */
  KHE_EVENT_RESOURCE_GROUP event_resource_group; /* optional erg             */
  KHE_RESOURCE_TYPE	resource_type;		/* resource type             */
  KHE_RESOURCE		preassigned_resource;	/* preassigned resource      */
  char			*role;			/* optional role             */
  int			workload;		/* workload (compulsory!)    */
  float			workload_per_time;	/* workload per time         */
  int			event_index;		/* index in event            */
  /* int		event_equiv_class; */	/* equiv class within event  */
  int			instance_index;		/* index in instance         */
  ARRAY_KHE_CONSTRAINT	constraints;		/* constraints on this e.r.  */
  HA_ARRAY_INT		constraint_eg_indexes;	/* eg_indexes of asac's      */
  KHE_RESOURCE_GROUP	hard_domain;		/* suitable domain for e.r.  */
  KHE_RESOURCE_GROUP	hard_and_soft_domain;	/* suitable domain for e.r.  */
  KHE_MAYBE_TYPE	needs_assignment;	/* true if needs assignment  */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceMake(KHE_EVENT event, KHE_RESOURCE_TYPE rt,         */
/*    KHE_RESOURCE preassigned_resource, char *role, KHE_EVENT_RESOURCE *er) */
/*                                                                           */
/*  Make a new event resource and add it to event.                           */
/*                                                                           */
/*****************************************************************************/

bool KheEventResourceMake(KHE_EVENT event, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE preassigned_resource, char *role, int workload,
  KHE_EVENT_RESOURCE *er)
{
  KHE_EVENT_RESOURCE res;  KHE_INSTANCE ins;  HA_ARENA a;
  HnAssert(event != NULL, "KheEventResourceMake: event is NULL");
  HnAssert(
    KheInstanceFinalized(KheEventInstance(event)) == KHE_FINALIZED_NONE,
    "KheEventResourceMake called after KheInstanceMakeEnd");
  HnAssert(preassigned_resource == NULL ||
    KheResourceResourceType(preassigned_resource) == rt,
    "KheEventResourceMake given preassigned resource of wrong resource type");
  if( role != NULL && KheEventRetrieveEventResource(event, role, &res) )
  {
    *er = NULL;
    return false;
  }
  ins = KheEventInstance(event);
  a = KheInstanceArena(ins);
  HaMake(res, a);
  res->back = NULL;
  res->event = event;
  res->event_resource_group = NULL;
  res->resource_type = rt;
  res->preassigned_resource = preassigned_resource;
  if( preassigned_resource == NULL )
    KheResourceTypeDemandNotAllPreassigned(rt);
  res->role = HnStringCopy(role, a);
  res->workload = workload;
  res->workload_per_time = (float) workload / (float) KheEventDuration(event);
  KheResourceTypeAddMaxWorkloadPerTime(rt, res->workload_per_time);
  if( preassigned_resource != NULL )
    KheResourceAddPreassignedEventResource(preassigned_resource, res);
  res->hard_domain = NULL;  /* set later, when finalizing */
  res->hard_and_soft_domain = NULL;  /* set later, when finalizing */
  res->needs_assignment = KHE_MAYBE;  /* set later, when finalizing */
  HaArrayInit(res->constraints, a);
  HaArrayInit(res->constraint_eg_indexes, a);
  KheEventAddEventResource(event, res, &res->event_index);
  /* res->event_equiv_class = -1; */  /* set later, when finalizing */
  res->instance_index = KheInstanceEventResourceCount(ins);
  KheInstanceAddEventResource(ins, res);
  *er = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceSetBack(KHE_EVENT_RESOURCE er, void *back)          */
/*                                                                           */
/*  Set the back pointer of er.                                              */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceSetBack(KHE_EVENT_RESOURCE er, void *back)
{
  HnAssert(
    KheInstanceFinalized(KheEventInstance(er->event)) == KHE_FINALIZED_NONE,
    "KheEventResourceSetBack called after KheInstanceMakeEnd");
  er->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheEventResourceBack(KHE_EVENT_RESOURCE er)                        */
/*                                                                           */
/*  Return the back pointer of er.                                           */
/*                                                                           */
/*****************************************************************************/

void *KheEventResourceBack(KHE_EVENT_RESOURCE er)
{
  return er->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheEventResourceEvent(KHE_EVENT_RESOURCE er)                   */
/*                                                                           */
/*  Return the event containing er.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheEventResourceEvent(KHE_EVENT_RESOURCE er)
{
  return er->event;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheEventResourceResourceType(KHE_EVENT_RESOURCE er)    */
/*                                                                           */
/*  Return the resource type of er.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheEventResourceResourceType(KHE_EVENT_RESOURCE er)
{
  return er->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheEventResourcePreassignedResource(KHE_EVENT_RESOURCE er)  */
/*                                                                           */
/*  Return the preassigned resource attribute of er.  This will be NULL      */
/*  if there is no preassigned resource.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheEventResourcePreassignedResource(KHE_EVENT_RESOURCE er)
{
  return er->preassigned_resource;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEventResourceRole(KHE_EVENT_RESOURCE er)                        */
/*                                                                           */
/*  Return the role attribute of er.                                         */
/*                                                                           */
/*****************************************************************************/

char *KheEventResourceRole(KHE_EVENT_RESOURCE er)
{
  return er->role;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceWorkload(KHE_EVENT_RESOURCE er)                      */
/*                                                                           */
/*  Return the workload of er.                                               */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceWorkload(KHE_EVENT_RESOURCE er)
{
  return er->workload;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheEventResourceWorkloadPerTime(KHE_EVENT_RESOURCE er)             */
/*                                                                           */
/*  Return the workload per time of er.                                      */
/*                                                                           */
/*****************************************************************************/

float KheEventResourceWorkloadPerTime(KHE_EVENT_RESOURCE er)
{
  return er->workload_per_time;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceEventIndex(KHE_EVENT_RESOURCE er)                    */
/*                                                                           */
/*  Return the index number of er in the enclosing event.                    */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceEventIndex(KHE_EVENT_RESOURCE er)
{
  return er->event_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheEventResourceInstance(KHE_EVENT_RESOURCE er)             */
/*                                                                           */
/*  Return the enclosing instance.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheEventResourceInstance(KHE_EVENT_RESOURCE er)
{
  return KheEventInstance(er->event);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceInstanceIndex(KHE_EVENT_RESOURCE er)                 */
/*                                                                           */
/*  Return the index number of er in the enclosing instance.                 */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceInstanceIndex(KHE_EVENT_RESOURCE er)
{
  return er->instance_index;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resource group"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceSetEventResourceGroup(KHE_EVENT_RESOURCE er,        */
/*    KHE_EVENT_RESOURCE_GROUP erg)                                          */
/*                                                                           */
/*  Sete the enclosing event resource group of er to erg.                    */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceSetEventResourceGroup(KHE_EVENT_RESOURCE er,
  KHE_EVENT_RESOURCE_GROUP erg)
{
  er->event_resource_group = erg;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_GROUP KheEventResourceEventResourceGroup(             */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Return the enclosing event resource group of er, or NULL if none.        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_GROUP KheEventResourceEventResourceGroup(
  KHE_EVENT_RESOURCE er)
{
  return er->event_resource_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraints"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceAddConstraint(KHE_EVENT_RESOURCE er,                */
/*    KHE_CONSTRAINT c)                                                      */
/*                                                                           */
/*  Add c to er.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceAddConstraint(KHE_EVENT_RESOURCE er, KHE_CONSTRAINT c,
  int eg_index)
{
  HaArrayAddLast(er->constraints, c);
  HaArrayAddLast(er->constraint_eg_indexes, eg_index);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceConstraintCount(KHE_EVENT_RESOURCE er)               */
/*                                                                           */
/*  Return the number of constraints applicable to er.                       */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceConstraintCount(KHE_EVENT_RESOURCE er)
{
  return HaArrayCount(er->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheEventResourceConstraint(KHE_EVENT_RESOURCE er, int i)  */
/*                                                                           */
/*  Return the i'th constraint applicable to er.                             */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheEventResourceConstraint(KHE_EVENT_RESOURCE er, int i)
{
  return HaArray(er->constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceConstraintEventGroupIndex(KHE_EVENT_RESOURCE er,     */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the event group index of the i'th constraint applicable to er,    */
/*  if it is an avoid split assignments constraint; otherwise return -1.     */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceConstraintEventGroupIndex(KHE_EVENT_RESOURCE er, int i)
{
  return HaArray(er->constraint_eg_indexes, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MAYBE_TYPE KheEventResourceNeedsAssignment(KHE_EVENT_RESOURCE er)    */
/*                                                                           */
/*  Return KHE_NO, KHE_MAYBE, or KHE_YES to say whether er needs assignment. */
/*                                                                           */
/*****************************************************************************/

KHE_MAYBE_TYPE KheEventResourceNeedsAssignment(KHE_EVENT_RESOURCE er)
{
  return er->needs_assignment;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource type partitioning"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourcePreventsResourceTypePartitioning(                   */
/*    KHE_EVENT_RESOURCE er, KHE_RESOURCE_TYPE *rt)                          */
/*                                                                           */
/*  Return true and set *rt to er's resource type if er prevents resource    */
/*  type partitioning for *rt, by not being subject to a hard prefer         */
/*  resources constraint of non-zero cost.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheEventResourcePreventsResourceTypePartitioning(KHE_EVENT_RESOURCE er,
  KHE_RESOURCE_TYPE *rt)
{
  KHE_CONSTRAINT c;  int i;  KHE_RESOURCE_GROUP domain;
  *rt = er->resource_type;
  HaArrayForEach(er->constraints, c, i)
    if( KheConstraintAffectsResourceTypePartitioning(c, &domain) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceResetResourceType(KHE_EVENT_RESOURCE er)            */
/*                                                                           */
/*  Reset the resource type of er; it may have changed owing to resource     */
/*  type partitioning.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceResetResourceType(KHE_EVENT_RESOURCE er)
{
  KHE_RESOURCE_TYPE new_rt;
  KheResourceGroupResetResourceType(er->hard_domain);
  KheResourceGroupResetResourceType(er->hard_and_soft_domain);
  new_rt = KheResourceGroupResourceType(er->hard_and_soft_domain);
  if( DEBUG4 )
  {
    fprintf(stderr, "  partitioning changing ");
    KheEventResourceDebug(er, 1, -1, stderr);
    fprintf(stderr, "  resource type from %s to %s\n",
      KheResourceTypeId(er->resource_type), KheResourceTypeId(new_rt));
  }
  er->resource_type = new_rt;
  if( er->preassigned_resource == NULL )
    KheResourceTypeDemandNotAllPreassigned(er->resource_type);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource domains and finalization"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SET_DOMAIN - fields needing to change as domains are found           */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SET_DOMAIN_ZERO,
  KHE_SET_DOMAIN_ONE,
  KHE_SET_DOMAIN_MANY
} KHE_SET_DOMAIN_TYPE;

typedef struct khe_set_domain_rec {
  KHE_RESOURCE_TYPE		resource_type;
  KHE_RESOURCE_GROUP		resource_group;
  KHE_SET_DOMAIN_TYPE		type;
} KHE_SET_DOMAIN;


/*****************************************************************************/
/*                                                                           */
/*  void KheSetDomainInit(KHE_SET_DOMAIN *sd, KHE_RESOURCE_TYPE rt)          */
/*                                                                           */
/*  Initialize sd.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheSetDomainInit(KHE_SET_DOMAIN *sd, KHE_RESOURCE_TYPE rt)
{
  sd->resource_type = rt;
  sd->resource_group = NULL;
  sd->type = KHE_SET_DOMAIN_ZERO;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetDomainAddDomain(KHE_SET_DOMAIN *sd, KHE_RESOURCE_GROUP rg)    */
/*                                                                           */
/*  Add rg to the growing domain (that is, intersect it into it).            */
/*                                                                           */
/*****************************************************************************/

static void KheSetDomainAddDomain(KHE_SET_DOMAIN *sd, KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE_GROUP rg2;
  switch( sd->type )
  {
    case KHE_SET_DOMAIN_ZERO:

      if( KheResourceGroupResourceCount(rg) <
	  KheResourceTypeResourceCount(sd->resource_type) )
      {
	sd->resource_group = rg;
	sd->type = KHE_SET_DOMAIN_ONE;
      }
      break;

    case KHE_SET_DOMAIN_ONE:

      if( !KheResourceGroupSubset(sd->resource_group, rg) )
      {
	rg2 = sd->resource_group;
	sd->resource_group = KheResourceGroupMakeInternal(
	  KheResourceGroupResourceType(rg), KHE_RESOURCE_GROUP_TYPE_CONSTRUCTED,
	  NULL, NULL, NULL);
	KheResourceGroupUnionInternal(sd->resource_group, rg2);
	KheResourceGroupIntersectInternal(sd->resource_group, rg);
	sd->type = KHE_SET_DOMAIN_MANY;
      }
      break;

    case KHE_SET_DOMAIN_MANY:

      if( !KheResourceGroupSubset(sd->resource_group, rg) )
	KheResourceGroupIntersectInternal(sd->resource_group, rg);
      break;

    default:

      HnAbort("KheSetDomainAddDomain internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheSetDomainFinalize(KHE_SET_DOMAIN *sd)              */
/*                                                                           */
/*  Return the final value of *sd.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheSetDomainFinalize(KHE_SET_DOMAIN *sd)
{
  switch( sd->type )
  {
    case KHE_SET_DOMAIN_ZERO:

      return KheResourceTypeFullResourceGroup(sd->resource_type);

    case KHE_SET_DOMAIN_ONE:

      return sd->resource_group;

    case KHE_SET_DOMAIN_MANY:

      KheResourceGroupFinalize(sd->resource_group);
      return sd->resource_group;

    default:

      HnAbort("KheSetDomainFinalize internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceSetDomain(KHE_EVENT_RESOURCE er,                    */
/*    bool hard_and_soft, HA_ARENA a)                                        */
/*                                                                           */
/*  As part of finalizing er, set the hard domain (or hard_and_soft domain   */
/*  if hard_and_soft is true) of er and all event resources linked to er.    */
/*                                                                           */
/*****************************************************************************/

static void KheEventResourceSetDomain(KHE_EVENT_RESOURCE er,
  bool hard_and_soft)
{
  ARRAY_KHE_EVENT_RESOURCE event_resources;  KHE_EVENT_RESOURCE er2;
  KHE_CONSTRAINT c;  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;
  KHE_RESOURCE_GROUP domain, prc_domain;  int i, j, k, egi, pos, x;
  KHE_SET_DOMAIN sd;  HA_ARENA a;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEventResourceSetDomain(");
    KheEventResourceDebug(er, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", hard_and_soft ? "true" : "false");
  }

  /* initialize sd to empty */
  KheSetDomainInit(&sd, er->resource_type);

  /* build the set of linked event resources, and their domain */
  /* arena is used only for event_resources, which is freed at the end */
  a = KheInstanceArena(KheEventInstance(er->event));
  HaArrayInit(event_resources, a);
  HaArrayAddLast(event_resources, er);
  HaArrayForEach(event_resources, er, i)
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  [ ");
      KheEventResourceDebug(er, 1, 0, stderr);
    }
    if( er->preassigned_resource != NULL )
    {
      prc_domain = KheResourceSingletonResourceGroup(er->preassigned_resource);
      KheSetDomainAddDomain(&sd, prc_domain);
    }
    HaArrayForEach(er->constraints, c, j)
    {
      if( DEBUG1 )
	KheConstraintDebug(c, 1, 4, stderr);
      if( KheConstraintWeight(c) > 0 &&
	  (hard_and_soft || KheConstraintRequired(c)) )
	switch( KheConstraintTag(c) )
	{
	  case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	    /* intersect the domain of this constraint with domain */
	    prc_domain = KhePreferResourcesConstraintDomain(
	      (KHE_PREFER_RESOURCES_CONSTRAINT) c);
	    KheSetDomainAddDomain(&sd, prc_domain);
	    break;

	  case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	    /* find other event resources linked to er by this constraint */
	    asac = (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c;
	    egi = KheEventResourceConstraintEventGroupIndex(er, j);
	    x =KheAvoidSplitAssignmentsConstraintEventResourceCount(asac,egi);
	    for( k = 0;  k < x;  k++ )
	    {
	      er2=KheAvoidSplitAssignmentsConstraintEventResource(asac,egi,k);
	      if( !HaArrayContains(event_resources, er2, &pos) )
	      {
		HaArrayAddLast(event_resources, er2);
		if( DEBUG1 )
		{
		  fprintf(stderr, "    add ");
		  KheEventResourceDebug(er2, 1, 0, stderr);
		}
	      }
	    }
	    break;

	  default:

	    break;
	}
    }
    if( DEBUG1 )
      fprintf(stderr, "  ]\n");
  }

  /* choose the domain depending on how many resource groups were intersected */
  domain = KheSetDomainFinalize(&sd);

  /* set the domain of every event resource touched here */
  if( hard_and_soft )
  {
    HaArrayForEach(event_resources, er, i)
    {
      HnAssert(er->hard_and_soft_domain == NULL,
	"KheEventResourceSetDomain internal error 2");
      er->hard_and_soft_domain = domain;
      if( DEBUG1 )
      {
	fprintf(stderr, "  setting hard_and_soft domain of ");
	KheEventResourceDebug(er, 1, -1, stderr);
	fprintf(stderr, " to ");
	KheResourceGroupDebug(domain, 1, 0, stderr);
      }
    }
  }
  else
  {
    HaArrayForEach(event_resources, er, i)
    {
      HnAssert(er->hard_domain == NULL,
	"KheEventResourceFindDomain internal error 3");
      er->hard_domain = domain;
      if( DEBUG1 )
      {
	fprintf(stderr, "  setting hard domain of ");
	KheEventResourceDebug(er, 1, -1, stderr);
	fprintf(stderr, " to ");
	KheResourceGroupDebug(domain, 1, 0, stderr);
      }
    }
  }
  HaArrayFree(event_resources);
  if( DEBUG1 )
    fprintf(stderr, "] KheEventResourceSetDomain returning\n");
}


/* old version which is sloppy in its use of memory
static void KheEventResourceSetDomain(KHE_EVENT_RESOURCE er,
  bool hard_and_soft, HA_ARENA a)
{
  ARRAY_KHE_EVENT_RESOURCE event_resources;  KHE_EVENT_RESOURCE er2;
  KHE_CONSTRAINT c;  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;
  KHE_RESOURCE_GROUP domain, prc_domain;  int i, j, k, egi, pos, count, x;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEventResourceFinalize(");
    KheEventResourceDebug(er, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", hard_and_soft ? "true" : "false");
  }

  ** start out with every resource in the resource group **
  domain = KheResourceGroupMakeInternal(er->resource_type,
    KHE_RESOURCE_GROUP_TYPE_CONSTRUCTED, NULL, NULL, NULL ** , LSetNew() **);
  KheResourceGroupUnionInternal(domain,
    KheResourceTypeFullResourceGroup(er->resource_type));
  count = 0;
  prc_domain = NULL;

  ** build the set of linked event resources, and their domain **
  HaArrayInit(event_resources, a);
  HaArrayAddLast(event_resources, er);
  HaArrayForEach(event_resources, er, i)
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  [ ");
      KheEventResourceDebug(er, 1, 0, stderr);
    }
    if( er->preassigned_resource != NULL )
    {
      prc_domain = KheResourceSingletonResourceGroup(er->preassigned_resource);
      if( !KheResourceGroupSubset(domain, prc_domain) )
      {
	count++;
	KheResourceGroupIntersectInternal(domain, prc_domain);
	if( DEBUG1 )
	{
	  fprintf(stderr, "    %d: preassigned intersect ", count);
	  KheResourceGroupDebug(prc_domain, 1, 0, stderr);
	}
      }
    }
    HaArrayForEach(er->constraints, c, j)
    {
      if( DEBUG1 )
	KheConstraintDebug(c, 1, 4, stderr);
      if( KheConstraintWeight(c) > 0 &&
	  (hard_and_soft || KheConstraintRequired(c)) )
	switch( KheConstraintTag(c) )
	{
	  case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

	    ** not relevant to this operation **
	    break;

	  case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	    ** intersect the domain of this constraint with domain **
	    prc_domain = KhePreferResourcesConstraintDomain(
	      (KHE_PREFER_RESOURCES_CONSTRAINT) c);
	    if( !KheResourceGroupSubset(domain, prc_domain) )
	    {
	      count++;
	      KheResourceGroupIntersectInternal(domain, prc_domain);
	      if( DEBUG1 )
	      {
		fprintf(stderr, "    %d: intersect ", count);
		KheResourceGroupDebug(prc_domain, 1, 0, stderr);
	      }
	    }
	    break;

	  case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	    ** find other event resources linked to er by this constraint **
	    asac = (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c;
	    egi = KheEventResourceConstraintEventGroupIndex(er, j);
	    x =KheAvoidSplitAssignmentsConstraintEventResourceCount(asac,egi);
	    for( k = 0;  k < x;  k++ )
	    {
	      er2=KheAvoidSplitAssignmentsConstraintEventResource(asac,egi,k);
	      if( !HaArrayContains(event_resources, er2, &pos) )
	      {
		HaArrayAddLast(event_resources, er2);
		if( DEBUG1 )
		{
		  fprintf(stderr, "    add ");
		  KheEventResourceDebug(er2, 1, 0, stderr);
		}
	      }
	    }
	    break;

	  default:

	    HnAbort("KheEventResourceFindDomain internal error 1");
	}
    }
    if( DEBUG1 )
      fprintf(stderr, "  ]\n");
  }

  ** choose the domain depending on how many resource groups were intersected **
  if( count == 0 )
  {
    ** no resource groups, so the full resource group is what we need **
    ** KheResourceGroupDelete(domain); hmmm, sti ll to do **
    domain = KheResourceTypeFullResourceGroup(er->resource_type);
    if( DEBUG1 )
    {
      fprintf(stderr, "  final count zero: ");
      KheResourceGroupDebug(domain, 1, 0, stderr);
    }
  }
  else if( count == 1 )
  {
    ** one resource group, so that one is what we need **
    ** *** KheResourceGroupDelete(domain);  hmmm, sti ll to do **
    domain = prc_domain;
    if( DEBUG1 )
    {
      fprintf(stderr, "  final count one: ");
      KheResourceGroupDebug(domain, 1, 0, stderr);
    }
  }
  else
  {
    ** more than one resource group, so we need a genuinely new group **
    KheResourceGroupFinalize(domain);
    if( DEBUG1 )
    {
      fprintf(stderr, "  final count %d: ", count);
      KheResourceGroupDebug(domain, 1, 0, stderr);
    }
  }

  ** set the domain of every event resource touched here **
  if( hard_and_soft )
  {
    HaArrayForEach(event_resources, er, i)
    {
      HnAssert(er->hard_and_soft_domain == NULL,
	"KheEventResourceFindDomain internal error 2");
      er->hard_and_soft_domain = domain;
    }
  }
  else
  {
    HaArrayForEach(event_resources, er, i)
    {
      HnAssert(er->hard_domain == NULL,
	"KheEventResourceFindDomain internal error 3");
      er->hard_domain = domain;
    }
  }
  HaArrayFree(event_resources);
  if( DEBUG1 )
    fprintf(stderr, "] KheEventResourceFinalize returning\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceFinalize(KHE_EVENT_RESOURCE er)                     */
/*                                                                           */
/*  Finalize er.  This means to initialize its domain attributes.            */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceFinalize(KHE_EVENT_RESOURCE er)
{
  KHE_CONSTRAINT c;  int i;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;

  /* finalize hard_domain and soft_domain */
  if( er->hard_domain == NULL )
    KheEventResourceSetDomain(er, false);
  if( er->hard_and_soft_domain == NULL )
    KheEventResourceSetDomain(er, true);

  /* finalize needs_assignment */
  if( DEBUG3 )
  {
    fprintf(stderr, "  [ ");
    KheEventResourceDebug(er, 2, -1, stderr);
    fprintf(stderr, " needs_asst\n");
  }
  er->needs_assignment = KHE_NO;
  HaArrayForEach(er->constraints, c, i)
  {
    if( DEBUG3 )
      KheConstraintDebug(c, 2, 4, stderr);
    if( KheConstraintWeight(c) > 0 )  switch( KheConstraintTag(c) )
    {
      case KHE_ASSIGN_RESOURCE_CONSTRAINT_TAG:

	er->needs_assignment = KHE_YES;
	if( DEBUG3 )
	  fprintf(stderr, "      er->needs_assignment = KHE_YES\n");
	return;

      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:
      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	/* nothing to do here */
	break;

      case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

	lrc = (KHE_LIMIT_RESOURCES_CONSTRAINT) c;
	if( KheLimitResourcesConstraintMinimum(lrc) > 0 )
	{
	  if( DEBUG3 )
	    fprintf(stderr, "      er->needs_assignment = KHE_MAYBE\n");
	  er->needs_assignment = KHE_MAYBE;
	}
	break;

      default:

	HnAbort("KheEventResourceFinalize: unexpected constraint type (%d)\n",
          KheConstraintTag(c));
	break;
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "  ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheEventResourceHardDomain(KHE_EVENT_RESOURCE er)     */
/*                                                                           */
/*  Return a suitable domain for er, taking hard prefer resources and        */
/*  avoid split assignments constraints into account.  Constraints of        */
/*  weight 0 are ignored.                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheEventResourceHardDomain(KHE_EVENT_RESOURCE er)
{
  HnAssert(er->hard_domain != NULL,
    "KheEventResourceHardDomain called before KheInstanceMakeEnd");
  return er->hard_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheEventResourceHardAndSoftDomain(                    */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Return a suitable domain for er, taking hard and soft prefer resources   */
/*  and avoid split assignments constraints into account.  Constraints of    */
/*  weight 0 are ignored.                                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheEventResourceHardAndSoftDomain(
  KHE_EVENT_RESOURCE er)
{
  HnAssert(er->hard_and_soft_domain != NULL,
    "KheEventResourceHardAndSoftDomain called before KheInstanceMakeEnd");
  return er->hard_and_soft_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceDoEquiv(KHE_EVENT_RESOURCE er1,                     */
/*    KHE_EVENT_RESOURCE er2)                                                */
/*                                                                           */
/*  Return true if er1 and er2 are equivalent, in the sense that the same    */
/*  resources can be assigned to both, and assigning a resource to one has   */
/*  the same cost as assigning it to the other.                              */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
bool KheEventResourceDoEquiv(KHE_EVENT_RESOURCE er1, KHE_EVENT_RESOURCE er2)
{
  KHE_CONSTRAINT c1, c2;  int i;

  ** er1 and er2 must have a lot of attributes in common **
  if( er1->event != er2->event )
    return false;
  if( er1->resource_type != er2->resource_type )
    return false;
  if( er1->preassigned_resource != er2->preassigned_resource )
    return false;
  if( er1->workload != er2->workload )
    return false;

  ** and they must have equivalent constraints (assumed sorted) **
  if( HaArrayCount(er1->constraints) != HaArrayCount(er2->constraints) )
    return false;
  for( i = 0;  i < HaArrayCount(er1->constraints);  i++ )
  {
    c1 = HaArray(er1->constraints, i);
    c2 = HaArray(er2->constraints, i);
    if( !KheConstraintDoEquiv(c1, c2) )
      return false;
  }

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceSetEquivClass(KHE_EVENT_RESOURCE er, int val)       */
/*                                                                           */
/*  Set the equivalence class of er.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEventResourceSetEquivClass(KHE_EVENT_RESOURCE er, int val)
{
  er->event_equiv_class = val;
  if( DEBUG2 )
  {
    fprintf(stderr, "  KheEventResourceSetEquivClass(");
    KheEventResourceDebug(er, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", val);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceEquivalent(KHE_EVENT_RESOURCE er1,                  */
/*    KHE_EVENT_RESOURCE er2)                                                */
/*                                                                           */
/*  Return true if er1 and er2 are equivalent, meaning that they lie in      */
/*  the same event, and for each resource r, assigning r to er1 incurs       */
/*  the same cost as assigning r to er2.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheEventResourceEquivalent(KHE_EVENT_RESOURCE er1, KHE_EVENT_RESOURCE er2)
{
  return er1->event == er2->event &&
    er1->event_equiv_class == er2->event_equiv_class;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceTimeRange(KHE_EVENT_RESOURCE er,                    */
/*    KHE_SOLN soln, KHE_TIME *first_time, KHE_TIME *last_time)              */
/*                                                                           */
/*  Return the time range of er in soln.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheEventResourceTimeRange(KHE_EVENT_RESOURCE er,
  KHE_SOLN soln, KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i;  KHE_TASK task;  KHE_MEET meet;  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
  {
    task = KheEventResourceTask(soln, er, i);
    meet = KheTaskMeet(task);
    HnAssert(meet != NULL, "KheEventResourceTimeRange internal error");
    ft = KheMeetAsstTime(meet);
    if( ft == NULL )
      return false;
    lt = KheTimeNeighbour(ft, KheMeetDuration(meet) - 1);
    if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
      *first_time = ft;
    if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
      *last_time = lt;
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceMakeFromKml(KML_ELT resource_elt,                   */
/*    KHE_EVENT e, KML_ERROR *ke)                                            */
/*                                                                           */
/*  Make an event resource from resource_elt and add it to e.                */
/*                                                                           */
/*****************************************************************************/

bool KheEventResourceMakeFromKml(KML_ELT resource_elt, KHE_EVENT e,
  KML_ERROR *ke)
{
  KML_ELT resource_type_elt, role_elt, workload_elt;  char *ref, *role;
  KHE_RESOURCE preassigned_resource;  KHE_RESOURCE_TYPE rt;  int workload;
  KHE_EVENT_RESOURCE er;  KHE_INSTANCE ins;  HA_ARENA a;
  ins = KheEventInstance(e);
  a = KheInstanceArena(ins);
  if( strcmp( KmlLabel(resource_elt), "Resource") == 0 )
  {
    /* traditional event resource introduced by <Resource> */
    if( !KmlCheck(resource_elt,
	  "+Reference : +$Role +ResourceType +#Workload", ke) )
      return false;

    /* optional preassigned resource */
    if( KmlAttributeCount(resource_elt) == 1 )
    {
      ref = KmlAttributeValue(resource_elt, 0);
      if( !KheInstanceRetrieveResource(ins, ref, &preassigned_resource) )
	return KmlError(ke, a, KmlLineNum(resource_elt),
	  KmlColNum(resource_elt), "in <Resource>, Reference \"%s\" unknown",
	  ref);
    }
    else
      preassigned_resource = NULL;

    /* optional role */
    if( KmlContainsChild(resource_elt, "Role", &role_elt) )
      role = KmlText(role_elt);
    else
    {
      if( preassigned_resource == NULL )
	return KmlError(ke, a, KmlLineNum(resource_elt),
	  KmlColNum(resource_elt),
	  "<Resource> with no preassignment and no <Role>");
      role = NULL;
    }

    /* optional ResourceType */
    if( KmlContainsChild(resource_elt, "ResourceType", &resource_type_elt) )
    {
      if( !KmlCheck(resource_type_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(resource_type_elt, 0);
      if( !KheInstanceRetrieveResourceType(ins, ref, &rt) )
	return KmlError(ke, a, KmlLineNum(resource_type_elt),
	  KmlColNum(resource_type_elt),
	  "in <ResourceType>, Reference \"%s\" unknown", ref);
      if( preassigned_resource != NULL &&
	  KheResourceResourceType(preassigned_resource) != rt )
	return KmlError(ke, a, KmlLineNum(resource_type_elt),
	  KmlColNum(resource_type_elt),
	"<ResourceType> \"%s\" inconsistent with preassigned resource \"%s\"",
	  ref, KheResourceId(preassigned_resource));
    }
    else
    {
      if( preassigned_resource == NULL )
	return KmlError(ke, a, KmlLineNum(resource_elt),
	  KmlColNum(resource_elt),
	  "<Resource> with no preassignment and no <ResourceType>");
      rt = KheResourceResourceType(preassigned_resource);
    }

    /* optional workload */
    if( KmlContainsChild(resource_elt, "Workload", &workload_elt) )
      sscanf(KmlText(workload_elt), "%d", &workload);
    else
      workload = KheEventWorkload(e);
  }
  else if( strcmp(KmlLabel(resource_elt), "R") == 0 )
  {
    /* abbreviated event resource introduced by <R> */
    if( !KmlCheck(resource_elt, "+Reference", ke) )
      return false;

    /* optional preassigned resource */
    if( KmlAttributeCount(resource_elt) == 1 )
    {
      ref = KmlAttributeValue(resource_elt, 0);
      if( !KheInstanceRetrieveResource(ins, ref, &preassigned_resource) )
	return KmlError(ke, a, KmlLineNum(resource_elt),
	  KmlColNum(resource_elt), "in <R>, Reference \"%s\" unknown", ref);
    }
    else
      preassigned_resource = NULL;

    /* optional role */
    if( strlen(KmlText(resource_elt)) > 0 )
      role = KmlText(resource_elt);
    else
    {
      if( preassigned_resource == NULL )
	return KmlError(ke, a,
	  KmlLineNum(resource_elt), KmlColNum(resource_elt),
	  "<R> with no preassignment and empty body denoting no role");
      role = NULL;
    }

    /* ResourceType is the first in the instance */
    if( KheInstanceResourceTypeCount(ins) == 0 )
      return KmlError(ke, a, KmlLineNum(resource_elt), KmlColNum(resource_elt),
	"<R> in instance with no resource types");
    rt = KheInstanceResourceType(ins, 0);

    /* workload taken from event */
    workload = KheEventWorkload(e);
  }
  else
  {
    HnAbort("unexpected event resource tag %s", KmlLabel(resource_elt));
    rt = NULL;				/* keep compiler happy */
    preassigned_resource = NULL;	/* keep compiler happy */
    role = NULL; 			/* keep compiler happy */
    workload = 0;			/* keep compiler happy */
  }

  /* make and add the event resource */
  if( !KheEventResourceMake(e, rt, preassigned_resource, role, workload, &er) )
    return KmlError(ke, a, KmlLineNum(resource_elt), KmlColNum(resource_elt),
      "<Event> Role \"%s\" used previously", role);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceWriteAbbreviated(KHE_EVENT_RESOURCE er)             */
/*                                                                           */
/*  Return true when er can be written abbreviated.                          */
/*                                                                           */
/*****************************************************************************/

bool KheEventResourceWriteAbbreviated(KHE_EVENT_RESOURCE er)
{
  KHE_RESOURCE_TYPE orig_rt;
  orig_rt = KheResourceTypeOriginalResourceType(er->resource_type);
  return KheResourceTypeIndex(orig_rt) == 0 &&
    er->workload == KheEventWorkload(er->event);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceWrite(KHE_EVENT_RESOURCE er, bool abbreviated,      */
/*    KML_FILE kf)                                                           */
/*                                                                           */
/*  Write er onto kf.  If abbreviated is true, write it abbreviated.         */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceWrite(KHE_EVENT_RESOURCE er, bool abbreviated, KML_FILE kf)
{
  char *id;
  if( abbreviated )
  {
    /* single-line abbreviated output */
    if( er->preassigned_resource != NULL )
    {
      id = KheResourceId(er->preassigned_resource);
      HnAssert(id != NULL, "KheArchiveWrite: Id missing in"
	" Resource referenced by Event %s", KheEventId(er->event));
      KmlEltAttributeFmtText(kf, "R", "Reference", id, "%s",
        er->role != NULL ? er->role : "");
    }
    else
      KmlEltFmtText(kf, "R", "%s", er->role != NULL ? er->role : "");
  }
  else if( er->preassigned_resource == NULL )
  {
    /* multi-line output including role and resource type */
    HnAssert(er->resource_type != NULL, "KheArchiveWrite: missing ResourceType "
      "in unpreassigned Resource of Event %s", KheEventId(er->event));
    id = KheResourceTypeId(
      KheResourceTypeOriginalResourceType(er->resource_type));
    KmlBegin(kf, "Resource");
    HnAssert(er->role != NULL, "KheArchiveWrite: missing Role in unpreassigned "
      "Resource of Event %s", KheEventId(er->event));
    KmlEltPlainText(kf, "Role", er->role);
    HnAssert(id != NULL, "KheArchiveWrite: Id"
      " missing in ResourceType referenced by Event %s", KheEventId(er->event));
    KmlEltAttribute(kf, "ResourceType", "Reference", id);
    if( er->workload != KheEventWorkload(er->event) )
      KmlEltFmtText(kf, "Workload", "%d", er->workload);
    KmlEnd(kf, "Resource");
  }
  else if( er->role != NULL || er->workload != KheEventWorkload(er->event) )
  {
    /* multi-line output omitting resource type */
    KmlBegin(kf, "Resource");
    HnAssert(KheResourceId(er->preassigned_resource) != NULL, "KheArchiveWrite:"
      " Id missing in Resource referenced by Event %s", KheEventId(er->event));
    KmlAttribute(kf, "Reference", KheResourceId(er->preassigned_resource));
    if( er->role != NULL )
      KmlEltPlainText(kf, "Role", er->role);
    if( er->workload != KheEventWorkload(er->event) )
      KmlEltFmtText(kf, "Workload", "%d", er->workload);
    KmlEnd(kf, "Resource");
  }
  else
  {
    /* single-line output showing preassignment only */
    HnAssert(KheResourceId(er->preassigned_resource) != NULL, "KheArchiveWrite:"
      " Id missing in Resource referenced by Event %s", KheEventId(er->event));
    KmlEltAttribute(kf, "Resource", "Reference",
      KheResourceId(er->preassigned_resource));
  }

  /* *** old version, prints ResourceType unnecessarily sometimes
  if( er->role == NULL && er->workload == KheEventWorkload(er->event) )
  {
    HnAssert(er->preassigned_resource != NULL,
      "KheArchiveWrite: no role or preassignment in Resource of event %s",
      KheEventId(er->event));
    HnAssert(KheResourceId(er->preassigned_resource) != NULL, "KheArchiveWrite:"
      " Id missing in Resource referenced by Event %s", KheEventId(er->event));
    KmlEltAttribute(kf, "Resource", "Reference",
      KheResourceId(er->preassigned_resource));
  }
  else
  {
    KmlBegin(kf, "Resource");
    if( er->preassigned_resource != NULL )
      KmlAttribute(kf, "Reference", KheResourceId(er->preassigned_resource));
    if( er->role != NULL )
    {
      KmlEltPlainText(kf, "Role", er->role);
      KmlEltAttribute(kf, "ResourceType", "Reference",
	KheResourceTypeId(er->resource_type));
    }
    if( er->workload != KheEventWorkload(er->event) )
      KmlEltFmtText(kf, "Workload", "%d", er->workload);
    KmlEnd(kf, "Resource");
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceDebug(KHE_EVENT_RESOURCE er, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of er onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceDebug(KHE_EVENT_RESOURCE er, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    KheEventDebug(er->event, 1, -1, fp);
      fprintf(fp, ".%d", er->event_index);
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

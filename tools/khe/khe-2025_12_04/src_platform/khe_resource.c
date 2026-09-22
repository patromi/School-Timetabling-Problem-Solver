
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
/*  FILE:         khe_resource.c                                             */
/*  DESCRIPTION:  A resource                                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE - a resource                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_rec {
  void				*back;			/* back pointer      */
  KHE_RESOURCE_TYPE		resource_type;		/* resource type     */
  char				*id;			/* Id                */
  char				*name;			/* Name              */
  KHE_RESOURCE_GROUP		partition;		/* partition         */
  ARRAY_KHE_RESOURCE_GROUP	user_resource_groups;	/* user resource gps */
  int				instance_index;		/* index in instance */
  int				resource_type_index;	/* index in r. type  */
  ARRAY_KHE_EVENT_RESOURCE	preassigned_event_resources;
  ARRAY_KHE_EVENT		layer_events;		/* events in layer   */
  int				layer_duration;		/* duration of layer */
  ARRAY_KHE_CONSTRAINT		constraints;		/* constraints       */
  KHE_RESOURCE_GROUP		singleton_resource_group; /* singleton       */
  KHE_TIME_GROUP		hard_unavail;		/* when r is unavail */
  KHE_TIME_GROUP		hard_and_soft_unavail;	/* when r is unavail */
  KHE_RESOURCE			rt_partition_parent;	/* rt partitioning   */
  ARRAY_KHE_RESOURCE		rt_partition;		/* rt partition      */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMake(KHE_RESOURCE_TYPE rt, char *id, char *name,         */
/*    KHE_RESOURCE_GROUP partition, KHE_RESOURCE *r)                         */
/*                                                                           */
/*  Make a new resource, add it to the `all' resource group of its resource  */
/*  type, and to its partition if any, and return it.                        */
/*                                                                           */
/*  This function returns false if any resource of rt's instance has the     */
/*  same id, unlike KheResourceGroupMake, which checks only for resource     */
/*  groups of the same resource type with the same id.                       */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMake(KHE_RESOURCE_TYPE rt, char *id, char *name,
  KHE_RESOURCE_GROUP partition, KHE_RESOURCE *r)
{
  KHE_RESOURCE res;  HA_ARENA a;
  HnAssert(rt != NULL, "KheResourceMake given NULL rt");
  HnAssert(
    KheInstanceFinalized(KheResourceTypeInstance(rt)) == KHE_FINALIZED_NONE,
    "KheResourceMake called after KheInstanceMakeEnd");
  if( id != NULL &&
      KheInstanceRetrieveResource(KheResourceTypeInstance(rt), id, &res) )
  {
    *r = NULL;
    return false;
  }
  a = KheInstanceArena(KheResourceTypeInstance(rt));
  HaMake(res, a);
  res->back = NULL;
  res->resource_type = rt;
  res->id = HnStringCopy(id, a);
  res->name = HnStringCopy(name, a);
  res->partition = partition;
  HaArrayInit(res->user_resource_groups, a);
  res->instance_index =
    KheInstanceResourceCount(KheResourceTypeInstance(rt));
  KheInstanceAddResource(KheResourceTypeInstance(rt), res);
  /* res->resource_type_index = KheResourceTypeResourceCount(rt); */
  KheResourceTypeAddResource(rt, res);  /* will set resource_type_index */
  HaArrayInit(res->preassigned_event_resources, a);
  HaArrayInit(res->layer_events, a);
  res->layer_duration = 0;
  HaArrayInit(res->constraints, a);
  if( partition != NULL )
  {
    HnAssert(KheResourceGroupIsPartition(partition),
      "KheResourceMake: partition attribute is not a defined partition");
    KheResourceGroupAddResourceInternal(partition, res);
  }
  /* *** this is done by KheResourceTypeAddResource now
  KheResourceGroupAddResourceInternal(KheResourceTypeFullResourceGroup(rt),res);
  *** */
  res->singleton_resource_group = KheResourceGroupMakeInternal(rt,
    KHE_RESOURCE_GROUP_TYPE_SINGLETON, NULL, res->id, res->name /*,LSetNew()*/);
  KheResourceGroupAddResourceInternal(res->singleton_resource_group, res);
  res->hard_unavail = NULL;  /* finalized at instance end */
  res->hard_and_soft_unavail = NULL;  /* finalized at instance end */
  res->rt_partition_parent = NULL;
  HaArrayInit(res->rt_partition, a);
  *r = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetBack(KHE_RESOURCE r, void *back)                      */
/*                                                                           */
/*  Set the back pointer of r.                                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetBack(KHE_RESOURCE r, void *back)
{
  HnAssert(KheInstanceFinalized(KheResourceInstance(r)) == KHE_FINALIZED_NONE,
    "KheResourceSetBack called after KheInstanceMakeEnd");
  r->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheResourceBack(KHE_RESOURCE r)                                    */
/*                                                                           */
/*  Return the back pointer of r.                                            */
/*                                                                           */
/*****************************************************************************/

void *KheResourceBack(KHE_RESOURCE r)
{
  return r->back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceAddUserResourceGroup(KHE_RESOURCE r,                     */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  This internal function adds rg to r's list of user resource groups.      */
/*                                                                           */
/*****************************************************************************/

void KheResourceAddUserResourceGroup(KHE_RESOURCE r, KHE_RESOURCE_GROUP rg)
{
  HnAssert(KheResourceGroupType(rg) == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceAddUserResourceGroup: rg is not a user resource group");
  HaArrayAddLast(r->user_resource_groups, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheResourceInstance(KHE_RESOURCE r)                         */
/*                                                                           */
/*  Return the instance attribute of r.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheResourceInstance(KHE_RESOURCE r)
{
  return KheResourceTypeInstance(r->resource_type);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInstanceIndex(KHE_RESOURCE r)                             */
/*                                                                           */
/*  Return the index number of r in its instance.                            */
/*                                                                           */
/*****************************************************************************/

int KheResourceInstanceIndex(KHE_RESOURCE r)
{
  return r->instance_index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheResourceResourceType(KHE_RESOURCE r)                */
/*                                                                           */
/*  Return the resource type of r, always non-NULL.                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheResourceResourceType(KHE_RESOURCE r)
{
  return r->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceResourceTypeIndex(KHE_RESOURCE r)                         */
/*                                                                           */
/*  Return the index number of r in its resource type.                       */
/*                                                                           */
/*****************************************************************************/

int KheResourceResourceTypeIndex(KHE_RESOURCE r)
{
  return r->resource_type_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetResourceTypeIndex(KHE_RESOURCE r, int val)            */
/*                                                                           */
/*  Set the index of r in its resource type to val.                          */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetResourceTypeIndex(KHE_RESOURCE r, int val)
{
  r->resource_type_index = val;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceId(KHE_RESOURCE r)                                      */
/*                                                                           */
/*  Return the id attribute of r.                                            */
/*                                                                           */
/*****************************************************************************/

char *KheResourceId(KHE_RESOURCE r)
{
  return r->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceName(KHE_RESOURCE r)                                    */
/*                                                                           */
/*  Return the name attribute of r.                                          */
/*                                                                           */
/*****************************************************************************/

char *KheResourceName(KHE_RESOURCE r)
{
  return r->name;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourcePartition(KHE_RESOURCE r)                  */
/*                                                                           */
/*  Return the partition containing r.  This will be NULL if r's resource    */
/*  type does not have partitions.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourcePartition(KHE_RESOURCE r)
{
  return r->partition;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetPartition(KHE_RESOURCE r, KHE_RESOURCE_GROUP rg)      */
/*                                                                           */
/*  Set r's partition attribute to rg.                                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetPartition(KHE_RESOURCE r, KHE_RESOURCE_GROUP rg)
{
  HnAssert(r->partition == NULL || r->partition == rg,
    "KheResourceSetPartition internal error");
  r->partition = rg;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "similarity"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSimilar(int pos, int neg)                                        */
/*                                                                           */
/*  Return true if two resources with this positive and negative evidence    */
/*  are to be considered similar.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheSimilar(int pos, int neg)
{
  return pos >= neg + 2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSimilar(KHE_RESOURCE r1, KHE_RESOURCE r2)                */
/*                                                                           */
/*  Return true if r1 and r2 are similar.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSimilar(KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  int pos, neg, i1, i2;  KHE_RESOURCE_GROUP rg1, rg2;  HA_ARENA a;
  KHE_EVENT_RESOURCE er;  KHE_EVENT e1, e2;  ARRAY_KHE_EVENT events1, events2;
  ARRAY_KHE_RESOURCE_GROUP resource_groups1, resource_groups2;
  KHE_INSTANCE ins;
  if( DEBUG1 )
    fprintf(stderr, "  [ KheResourceSimilar(%s: %d, %s: %d)\n",
     r1->id==NULL ? "-" : r1->id, HaArrayCount(r1->preassigned_event_resources),
     r2->id==NULL ? "-" : r2->id,HaArrayCount(r2->preassigned_event_resources));

  /* scratch memory for this operation */
  ins = KheResourceInstance(r1);
  a = KheInstanceArenaBegin(ins);

  /* find the admissible resource groups of the two resources */
  HaArrayInit(resource_groups1, a);
  HaArrayForEach(r1->user_resource_groups, rg1, i1)
    if( KheResourceGroupPartitionAdmissible(rg1) )
      HaArrayAddLast(resource_groups1, rg1);
  HaArrayInit(resource_groups2, a);
  HaArrayForEach(r2->user_resource_groups, rg2, i2)
    if( KheResourceGroupPartitionAdmissible(rg2) )
      HaArrayAddLast(resource_groups2, rg2);

  /* gather positive and negative evidence from resource groups */
  pos = neg = 0;
  HaArrayForEach(resource_groups1, rg1, i1)
  {
    HaArrayForEach(resource_groups2, rg2, i2)
      if( KheResourceGroupEqual(rg1, rg2) )
	break;
    if( i2 < HaArrayCount(resource_groups2) )
    {
      /* found a match, so delete both */
      pos += 2;
      HaArrayDeleteAndShift(resource_groups2, i2);
      HaArrayDeleteAndShift(resource_groups1, i1);
      i1--;
    }
  }
  neg += HaArrayCount(resource_groups1) + HaArrayCount(resource_groups2);

  /* find the admissible events of the two resources */
  HaArrayInit(events1, a);
  HaArrayForEach(r1->preassigned_event_resources, er, i1)
    if( KheEventPartitionAdmissible(KheEventResourceEvent(er)) )
      HaArrayAddLast(events1, KheEventResourceEvent(er));
  HaArrayInit(events2, a);
  HaArrayForEach(r2->preassigned_event_resources, er, i2)
    if( KheEventPartitionAdmissible(KheEventResourceEvent(er)) )
      HaArrayAddLast(events2, KheEventResourceEvent(er));

  /* gather positive and negative evidence from events */
  HaArrayForEach(events1, e1, i1)
  {
    HaArrayForEach(events2, e2, i2)
      if( KheEventPartitionSimilar(e1,e2,&resource_groups1,&resource_groups2) )
	break;
    if( i2 < HaArrayCount(events2) )
    {
      /* found a match, so delete the events concerned */
      pos += 2;
      HaArrayDeleteAndShift(events2, i2);
      HaArrayDeleteAndShift(events1, i1);
      i1--;
    }
  }
  neg += HaArrayCount(events1) + HaArrayCount(events2);

  /* return true if pos outweighs neg */
  KheInstanceArenaEnd(ins, a);
  /* ***
  MArrayFree(events1);
  MArrayFree(events2);
  MArrayFree(resource_groups1);
  MArrayFree(resource_groups2);
  *** */
  if( DEBUG2 && KheSimilar(pos, neg) )
    fprintf(stderr, "  resources %s and %s similar (pos %d, neg %d)\n",
      r1->id==NULL ? "-" : r1->id, r2->id==NULL ? "-" : r2->id, pos, neg);
  if( DEBUG1 )
    fprintf(stderr, "  ] KheResourceSimilar returning %s (p%d, n%d)\n",
      KheSimilar(pos, neg) ? "true" : "false", pos, neg);
  return KheSimilar(pos, neg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceSingletonResourceGroup(KHE_RESOURCE r)     */
/*                                                                           */
/*  Return the automatically generated singleton resource group holding r.   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceSingletonResourceGroup(KHE_RESOURCE r)
{
  return r->singleton_resource_group;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceResourceGroupCount(KHE_RESOURCE r)                        */
/*                                                                           */
/*  Return the number of user resource groups containing r.                  */
/*                                                                           */
/*****************************************************************************/

int KheResourceResourceGroupCount(KHE_RESOURCE r)
{
  return HaArrayCount(r->user_resource_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceResourceGroup(KHE_RESOURCE r, int i)       */
/*                                                                           */
/*  Return the i'th user resource group containing r.                        */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceResourceGroup(KHE_RESOURCE r, int i)
{
  return HaArray(r->user_resource_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preassigned event resources and layers"                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceAddPreassignedEventResource(KHE_RESOURCE r,              */
/*    KHE_EVENT_RESOURCE er)                                                 */
/*                                                                           */
/*  Add er to r's list of event resources that r is preassigned to.          */
/*                                                                           */
/*****************************************************************************/

void KheResourceAddPreassignedEventResource(KHE_RESOURCE r,
  KHE_EVENT_RESOURCE er)
{
  HaArrayAddLast(r->preassigned_event_resources, er);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourcePreassignedEventResourceCount(KHE_RESOURCE r)             */
/*                                                                           */
/*  Return the number of ppreassigned event resources of r.                  */
/*                                                                           */
/*****************************************************************************/

int KheResourcePreassignedEventResourceCount(KHE_RESOURCE r)
{
  return HaArrayCount(r->preassigned_event_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheResourcePreassignedEventResource(KHE_RESOURCE r,   */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th preassigned event resource of r.                         */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheResourcePreassignedEventResource(KHE_RESOURCE r, int i)
{
  return HaArray(r->preassigned_event_resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceLayerEventCount(KHE_RESOURCE r)                           */
/*                                                                           */
/*  Return the number of events in r's layer.                                */
/*                                                                           */
/*****************************************************************************/

int KheResourceLayerEventCount(KHE_RESOURCE r)
{
  return HaArrayCount(r->layer_events);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheResourceLayerEvent(KHE_RESOURCE r, int i)                   */
/*                                                                           */
/*  Return the i'th event of r's layer.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheResourceLayerEvent(KHE_RESOURCE r, int i)
{
  return HaArray(r->layer_events, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceLayerDuration(KHE_RESOURCE r)                             */
/*                                                                           */
/*  Return the duration of r's layer.                                        */
/*                                                                           */
/*****************************************************************************/

int KheResourceLayerDuration(KHE_RESOURCE r)
{
  return r->layer_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraints"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheEventIndexCmp(const void *p1, const void *p2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of events by increasing index.  */
/*                                                                           */
/*****************************************************************************/

static int KheEventIndexCmp(const void *p1, const void *p2)
{
  KHE_EVENT e1 = * (KHE_EVENT *) p1;
  KHE_EVENT e2 = * (KHE_EVENT *) p2;
  return KheEventIndex(e1) - KheEventIndex(e2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceAddConstraint(KHE_RESOURCE r, KHE_CONSTRAINT c)          */
/*                                                                           */
/*  Add c to r.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceAddConstraint(KHE_RESOURCE r, KHE_CONSTRAINT c)
{
  KHE_EVENT_RESOURCE er;  KHE_EVENT e;  int i, pos;
  HaArrayAddLast(r->constraints, c);

  /* build the layer of r, if required but not built yet */
  if( KheConstraintTag(c) == KHE_AVOID_CLASHES_CONSTRAINT_TAG &&
      KheConstraintRequired(c) && KheResourceLayerDuration(r) == 0 )
  {
    HaArrayForEach(r->preassigned_event_resources, er, i)
    {
      e = KheEventResourceEvent(er);
      if( !HaArrayContains(r->layer_events, e, &pos) )
      {
	HaArrayAddLast(r->layer_events, e);
	r->layer_duration += KheEventDuration(e);
      }
    }
    HaArraySort(r->layer_events, &KheEventIndexCmp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceConstraintCount(KHE_RESOURCE r)                           */
/*                                                                           */
/*  Return the number of constraints applicable to r.                        */
/*                                                                           */
/*****************************************************************************/

int KheResourceConstraintCount(KHE_RESOURCE r)
{
  return HaArrayCount(r->constraints);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT KheResourceConstraint(KHE_RESOURCE r, int i)              */
/*                                                                           */
/*  Return the i'th constraint applicable to r.                              */
/*                                                                           */
/*****************************************************************************/

KHE_CONSTRAINT KheResourceConstraint(KHE_RESOURCE r, int i)
{
  return HaArray(r->constraints, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource type partitioning"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceResourceTypePartitioningInit(KHE_RESOURCE r)             */
/*                                                                           */
/*  Initialize r for resource type partitioning, but only if its resource    */
/*  type is on for it.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceResourceTypePartitioningInit(KHE_RESOURCE r)
{
  r->rt_partition_parent = NULL;
  if( KheResourceTypeResourceTypePartitioningOn(r->resource_type) )
    HaArrayAddLast(r->rt_partition, r);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceResourceTypePartitioningRoot(KHE_RESOURCE r)     */
/*                                                                           */
/*  Return the root of the partition containing r.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE KheResourceResourceTypePartitioningRoot(KHE_RESOURCE r)
{
  while( r->rt_partition_parent != NULL )
    r = r->rt_partition_parent;
  return r;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceResourceTypePartitioningMerge(KHE_RESOURCE r1,           */
/*    KHE_RESOURCE r2)                                                       */
/*                                                                           */
/*  Merge the partitions containing r1 and r2.                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceResourceTypePartitioningMerge(KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  int i;
  if( HaArrayCount(r1->rt_partition) > 0 && HaArrayCount(r2->rt_partition) > 0 )
  {
    r1 = KheResourceResourceTypePartitioningRoot(r1);
    r2 = KheResourceResourceTypePartitioningRoot(r2);
    if( r1 != r2 )
    {
      if( DEBUG4 )
	fprintf(stderr, "  partitioning merging %s (%d) into %s (%d)\n",
	  KheResourceId(r2), HaArrayCount(r2->rt_partition),
	  KheResourceId(r1), HaArrayCount(r1->rt_partition));
      r2->rt_partition_parent = r1;
      HaArrayAppend(r1->rt_partition, r2->rt_partition, i);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMoveToResourceType(KHE_RESOURCE r, KHE_RESOURCE_TYPE rt) */
/*                                                                           */
/*  Move r from its current resource type to rt.  Also its singleton rg.     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMoveToResourceType(KHE_RESOURCE r, KHE_RESOURCE_TYPE rt)
{
  KheResourceTypeDeleteResource(r->resource_type, r, r->resource_type_index);
  KheResourceTypeAddResource(rt, r);  /* will set resource_type_index */
  r->resource_type = rt;
  KheResourceGroupResetResourceType(r->singleton_resource_group);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupMatchesPartition(KHE_RESOURCE_GROUP rg,             */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return true if rg matches r's partition exactly.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceGroupMatchesPartition(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE r)
{
  int i;  KHE_RESOURCE r2;
  if( KheResourceGroupResourceCount(rg) != HaArrayCount(r->rt_partition) )
    return false;
  HaArrayForEach(r->rt_partition, r2, i)
    if( !KheResourceGroupContains(rg, r2) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourcePartitionEqualsResourceGroup(KHE_RESOURCE r,             */
/*    KHE_RESOURCE_GROUP *rg)                                                */
/*                                                                           */
/*  If r's resource partition equals some user resource group, return true   */
/*  with *rg set to that resource group.  Otherwise return false.            */
/*                                                                           */
/*****************************************************************************/

static bool KheResourcePartitionEqualsResourceGroup(KHE_RESOURCE r,
  KHE_RESOURCE_GROUP *rg)
{
  KHE_RESOURCE_GROUP rg2;  int i;
  HaArrayForEach(r->user_resource_groups, rg2, i)
    if( KheResourceGroupMatchesPartition(rg2, r) )
      return *rg = rg2, true;
  return *rg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceResourceTypePartitionMakeResourceType(KHE_RESOURCE r)    */
/*                                                                           */
/*  Make a resource type based on r, if it is its own root.                  */
/*                                                                           */
/*  Implementation note.  This function will not make too many resource      */
/*  types, because it makes one resource type for each resource r such that  */
/*  r->rt_partition_parent == NULL, and only when r's partition contains     */
/*  at most half the number of resources in r's *current* resource type.     */
/*                                                                           */
/*  Obsolete below here                                                      */
/*  -------------------                                                      */
/*                                                                           */
/*  Implementation note.  This function will not make too many resource      */
/*  types, because it makes one resource type for each resource such that    */
/*  r->rt_partition_parent == NULL, and not when r's current resource type   */
/*  contains the same number of resources as r's partition.  (obsolete)      */
/*                                                                           */
/*  Implementation note.  This function will not make too many resource      */
/*  types, because it makes one resource type for each resource such that    */
/*  r->rt_partition_parent == NULL, and not when r is the first in its       */
/*  resource type.  (obsolete)                                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceResourceTypePartitionMakeResourceType(KHE_RESOURCE r)
{
  KHE_RESOURCE r2;  int i;  KHE_RESOURCE_TYPE rt, new_rt;
  char buff[1000], *str;  KHE_INSTANCE ins;  KHE_RESOURCE_GROUP rg;
  rt = r->resource_type;
  if( r->rt_partition_parent == NULL &&
      KheResourceTypeResourceTypePartitioningOn(rt) &&
      HaArrayCount(r->rt_partition) <= KheResourceTypeResourceCount(rt) / 2 )
  {
    /* make resource type new_rt */
    HnAssert(KheResourceTypeResourceCount(rt) > 0,
      "KheResourceResourceTypePartitionMakeResourceType internal error 1");
    rt = r->resource_type;
    ins = KheResourceTypeInstance(rt);
    str = (KheResourcePartitionEqualsResourceGroup(r, &rg) ?
      KheResourceGroupId(rg) : KheResourceId(r));
    snprintf(buff, 1000, "%s:%s", KheResourceTypeId(rt), str);
    i = 1;
    while( !KheResourceTypeMakeInternal(ins, buff, buff, false, rt, &new_rt) )
      snprintf(buff, 1000, "%s:%s:%d", KheResourceTypeId(rt), str, i++);

    /* move r and its partition to new_rt */
    HaArrayForEach(r->rt_partition, r2, i)
      KheResourceMoveToResourceType(r2, new_rt);

    /* debug the new resource type */
    if( DEBUG3 )
    {
      fprintf(stderr, "  adding resource type %s = {",
	KheResourceTypeId(new_rt));
      for( i = 0;  i < KheResourceTypeResourceCount(new_rt);  i++ )
      {
	r2 = KheResourceTypeResource(new_rt, i);
	if( i > 0 )
	  fprintf(stderr, ", ");
	fprintf(stderr, "%s", KheResourceId(r2));
      }
      fprintf(stderr, "}\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "finalize"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheResourceFindUnavail(KHE_RESOURCE r, bool hard_and_soft)*/
/*                                                                           */
/*  Make a single time group for the unavailable times of r, either just     */
/*  the hard ones (hard_and_soft == false) or hard and soft (.. == true).    */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheResourceFindUnavail(KHE_RESOURCE r,
  bool hard_and_soft, HA_ARENA a)
{
  KHE_TIME_GROUP tg;  KHE_CONSTRAINT c;  int i;  SSET ss;

  SSetInit(ss, a);
  HaArrayForEach(r->constraints, c, i)
    if( KheConstraintTag(c) == KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT_TAG &&
	KheConstraintWeight(c) > 0 &&
	(hard_and_soft || KheConstraintRequired(c)) )
    {
      tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(
	(KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT) c);
      SSetUnion(ss, *KheTimeGroupTimeSet(tg));
    }
  return KheTimeGroupMakeAndFinalize(KheResourceInstance(r),
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, /* NULL, */ true, a);
}

/* *** this version is total rubbish, how did it ever sneak through?
   *** by being barely used (just khe_st_cluster_and_limit.c)
static KHE_TIME_GROUP KheResourceFindUnavail(KHE_RESOURCE r,
  bool hard_and_soft, HA_ARENA a)
{
  KHE_TIME_GROUP tg;  KHE_CONSTRAINT c;  int i;  SSET ss;

  SSetInit(ss, a);
  HaArrayForEach(r->constraints, c, i)
    if( KheConstraintTag(c) == KHE_PREFER_TIMES_CONSTRAINT_TAG &&
	KheConstraintWeight(c) > 0 &&
	(hard_and_soft || KheConstraintRequired(c)) )
    {
      tg = KhePreferTimesConstraintDomain((KHE_PREFER_TIMES_CONSTRAINT) c);
      SSetUnion(ss, *KheTimeGroupTimeSet(tg));
    }
  return KheTimeGroupMake AndFinalize(KheResourceInstance(r),
    KHE_TIME_GROUP_KIND_AUTO, NULL, NULL, &ss, NULL, true);
}
*** */

/* *** this works but the new version is simpler and quite fast enough
static KHE_TIME_GROUP KheResourceFindUnavail(KHE_RESOURCE r, bool hard_and_soft)
{
  KHE_TIME_GROUP res, tg, tmp;  KHE_CONSTRAINT c;  int i;
  enum { UNAVAIL_NONE, UNAVAIL_ONE, UNAVAIL_MANY } state;

  res = NULL;
  state = UNAVAIL_NONE;
  HaArrayForEach(r->constraints, c, i)
    if( KheConstraintTag(c) == KHE_PREFER_TIMES_CONSTRAINT_TAG &&
	KheConstraintWeight(c) > 0 &&
	(hard_and_soft || KheConstraintRequired(c)) )
    {
      tg = KhePreferTimesConstraintDomain((KHE_PREFER_TIMES_CONSTRAINT) c);
      switch( state )
      {
	case UNAVAIL_NONE:

	  res = tg;
	  state = UNAVAIL_ONE;
	  break;

	case UNAVAIL_ONE:

	  if( !KheTimeGroupSubset(tg, res) )
	  {
	    tmp = res;
	    res = KheTimeGroup MakeInternal(KheResourceInstance(r),
	      ** KHE_TIME_GROUP_TYPE_CONSTRUCTED, ** NULL,
	      KHE_TIME_GROUP_KIND_AUTO, NULL, NULL ** , LSetNew() **);
	    KheTimeGroupUnion Internal(res, tmp);
	    KheTimeGroupUnion Internal(res, tg);
	    state = UNAVAIL_MANY;
	  }
	  break;

	case UNAVAIL_MANY:

	  KheTimeGroupUnion Internal(res, tg);
	  break;

	default:

          MAbort("KheResourceFindUnavail internal error 1");
	  break;
      }
    }

  ** sort out the final value, depending on count **
  switch( state )
  {
    case UNAVAIL_NONE:

      res = KheInstanceEmptyTimeGroup(KheResourceInstance(r));
      break;

    case UNAVAIL_ONE:

      ** OK as is **
      break;

    case UNAVAIL_MANY:

      KheTimeGroup Finalize(res, NULL, true ** , NULL, -1 **);
      break;

    default:

      MAbort("KheResourceFindUnavail internal error 2");
      break;
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceFinalize(KHE_RESOURCE r)                                 */
/*                                                                           */
/*  Finalize r.  This must be done after finalizing constraints, because     */
/*  it calls KhePreferTimesConstraintDomain.                                 */
/*                                                                           */
/*****************************************************************************/

void KheResourceFinalize(KHE_RESOURCE r)
{
  HA_ARENA a;
  a = KheInstanceArena(KheResourceTypeInstance(r->resource_type));
  HnAssert(r->hard_unavail == NULL, "KheResourceFinalize called twice");
  r->hard_unavail = KheResourceFindUnavail(r, false, a);
  r->hard_and_soft_unavail = KheResourceFindUnavail(r, true, a);
  KheResourceGroupFinalize(r->singleton_resource_group);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheResourceHardUnavailableTimeGroup(KHE_RESOURCE r)       */
/*                                                                           */
/*  Return a time group containing the union of the domains of the hard      */
/*  prefer times constraints of r.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheResourceHardUnavailableTimeGroup(KHE_RESOURCE r)
{
  HnAssert(r->hard_unavail != NULL,
    "KheResourceHardUnavailableTimeGroup called before KheInstanceEnd");
  return r->hard_unavail;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheResourceHardAndSoftUnavailableTimeGroup(KHE_RESOURCE r)*/
/*                                                                           */
/*  Return a time group containing the union of the domains of the hard      */
/*  and soft prefer times constraints of r.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheResourceHardAndSoftUnavailableTimeGroup(KHE_RESOURCE r)
{
  HnAssert(r->hard_and_soft_unavail != NULL,
    "KheResourceHardAndSoftUnavailableTimeGroup called before KheInstanceEnd");
  return r->hard_and_soft_unavail;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasAvoidClashesConstraint(KHE_RESOURCE r, KHE_COST cost) */
/*                                                                           */
/*  Return true if r is subject to an avoid clashes constraint of combined   */
/*  weight greater than cost.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheResourceHasAvoidClashesConstraint(KHE_RESOURCE r, KHE_COST cost)
{
  int i;  KHE_CONSTRAINT c;
  for( i = 0;  i < KheResourceConstraintCount(r);  i++ )
  {
    c = KheResourceConstraint(r, i);
    if( KheConstraintTag(c) == KHE_AVOID_CLASHES_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > cost )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventHasAssignTimeConstraint(KHE_EVENT e, KHE_COST cost)         */
/*                                                                           */
/*  Return true if e is subject to an assign time constraint of combined     */
/*  weight greater than cost.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheEventHasAssignTimeConstraint(KHE_EVENT e, KHE_COST cost)
{
  int i;  KHE_CONSTRAINT c;
  for( i = 0;  i < KheEventConstraintCount(e);  i++ )
  {
    c = KheEventConstraint(e, i);
    if( KheConstraintTag(c) == KHE_ASSIGN_TIME_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > cost )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourcePreassignedEventsDuration(KHE_RESOURCE r, KHE_COST cost)  */
/*                                                                           */
/*  Return the total duration of events which are both preassigned r and     */
/*  either preassigned a time or subject to an assign time constraint of     */
/*  combined weight greater than cost.                                       */
/*                                                                           */
/*****************************************************************************/

int KheResourcePreassignedEventsDuration(KHE_RESOURCE r, KHE_COST cost)
{
  int i, res;  KHE_EVENT e;
  res = 0;
  for( i = 0;  i < KheResourcePreassignedEventResourceCount(r);  i++ )
  {
    e = KheEventResourceEvent(KheResourcePreassignedEventResource(r, i));
    if( KheEventPreassignedTime(e) != NULL ||
        KheEventHasAssignTimeConstraint(e, cost) )
      res += KheEventDuration(e);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "sorting"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypedCmp(KHE_RESOURCE r1, KHE_RESOURCE r2)                */
/*                                                                           */
/*  Comparison function for sorting resources by increasing instance index.  */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypedCmp(KHE_RESOURCE r1, KHE_RESOURCE r2)
{
  return r1->instance_index - r2->instance_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceCmp(const void *t1, const void *t2)                       */
/*                                                                           */
/*  Comparison function, suitable for passing to qsort, for sorting an       */
/*  array of resources by increasing instance index.                         */
/*                                                                           */
/*****************************************************************************/

int KheResourceCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE r1 = * (KHE_RESOURCE *) t1;
  KHE_RESOURCE r2 = * (KHE_RESOURCE *) t2;
  return KheResourceTypedCmp(r1, r2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMakeFromKml(KML_ELT resource_elt, KHE_INSTANCE ins,      */
/*    KML_ERROR *ke)                                                         */
/*                                                                           */
/*  Add a resource based on resource_elt to ins.                             */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMakeFromKml(KML_ELT resource_elt, KHE_INSTANCE ins,
  KML_ERROR *ke)
{
  KML_ELT resource_groups_elt, resource_group_elt, rt_elt;
  int j;  char *name, *id, *ref;  HA_ARENA a;
  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE_TYPE rt;

  a = KheInstanceArena(ins);
  if( !KmlCheck(resource_elt, "Id : $Name ResourceType +ResourceGroups", ke) )
    return false;
  id = KmlAttributeValue(resource_elt, 0);
  name = KmlText(KmlChild(resource_elt, 0));
  rt_elt = KmlChild(resource_elt, 1);
  if( !KmlCheck(rt_elt, "Reference", ke) )
    return false;
  ref = KmlAttributeValue(rt_elt, 0);
  if( !KheInstanceRetrieveResourceType(ins, ref, &rt) )
    return KmlError(ke, a, KmlLineNum(rt_elt), KmlColNum(rt_elt),
      "in <ResourceType>, unknown Reference \"%s\"", ref);
  if( !KheResourceMake(rt, id, name, NULL, &r) )
    return KmlError(ke, a, KmlLineNum(resource_elt), KmlColNum(resource_elt),
      "<Resource> Id \"%s\" used previously", id);

  /* connect r to its subgroups, if any */
  if( KmlContainsChild(resource_elt, "ResourceGroups", &resource_groups_elt) )
  {
    if( !KmlCheck(resource_groups_elt, ": *ResourceGroup", ke) )
      return false;
    for( j = 0;  j < KmlChildCount(resource_groups_elt);  j++ )
    {
      resource_group_elt = KmlChild(resource_groups_elt, j);
      if( !KmlCheck(resource_group_elt, "Reference", ke) )
	return false;
      ref = KmlAttributeValue(resource_group_elt, 0);
      if( !KheInstanceRetrieveResourceGroup(ins, ref, &rg) )
	return KmlError(ke, a, KmlLineNum(resource_group_elt),
	  KmlColNum(resource_group_elt),
	  "in <ResourceGroup>, unknown Reference \"%s\"", ref);
      KheResourceGroupAddResource(rg, r);
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceWrite(KHE_RESOURCE r, KML_FILE kf)                       */
/*                                                                           */
/*  Write r to kf.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheResourceWrite(KHE_RESOURCE r, KML_FILE kf)
{
  KHE_RESOURCE_GROUP rg;  int i;
  KmlBegin(kf, "Resource");
  KmlAttribute(kf, "Id", r->id);
  HnAssert(r->id != NULL, "KheArchiveWrite: Id missing in Resource");
  KmlEltPlainText(kf, "Name", r->name);
  HnAssert(r->name != NULL, "KheArchiveWrite: Name missing in Resource");
  KmlEltAttribute(kf, "ResourceType", "Reference",
    KheResourceTypeId(KheResourceTypeOriginalResourceType(r->resource_type)));
  if( HaArrayCount(r->user_resource_groups) > 0 )
  {
    KmlBegin(kf, "ResourceGroups");
    HaArrayForEach(r->user_resource_groups, rg, i)
    {
      HnAssert(KheResourceGroupId(rg), "KheArchiveWrite: Id missing"
	" in ResourceGroup referenced by Resource %s", r->id);
      KmlEltAttribute(kf, "ResourceGroup", "Reference", KheResourceGroupId(rg));
    }
    KmlEnd(kf, "ResourceGroups");
  }
  KmlEnd(kf, "Resource");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceDebug(KHE_RESOURCE r, int verbosity,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of r onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheResourceDebug(KHE_RESOURCE r, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "%s", r->id != NULL ? r->id : "-");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

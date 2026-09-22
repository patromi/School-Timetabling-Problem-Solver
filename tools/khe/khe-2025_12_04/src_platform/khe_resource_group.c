
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
/*  FILE:         khe_resource_group.c                                       */
/*  DESCRIPTION:  A resource group                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP - a resource group                                    */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Implementation note.  The resources_set attribute is always defined      */
/*  and it holds the true current value of the resource group as a set of    */
/*  resources, represented as the set of their indices in the enclosing      */
/*  instance.  The resource_indexes attribute is a redundant alternative     */
/*  representation of the same information, updated lazily:  when the        */
/*  resources change, it is cleared, and when a traversal is required, or    */
/*  the resource group needs to become immutable, it is made consistent      */
/*  with resources_set.                                                      */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_group_rec {
  void				*back;			/* back pointer      */
  KHE_RESOURCE_GROUP_TYPE	resource_group_type;	/* user-defined etc  */
  int				partition_index;	/* index in instance */
  struct khe_resource_set_rec	rs_rec;			/* resource set      */
  char				*id;			/* Id                */
  char				*name;			/* Name              */
  bool				finalized;		/* rs finalized      */
  KHE_RESOURCE_GROUP		partition;		/* partition, if any */
};
/* ***
  KHE_RESOURCE_TYPE		resource_type;		** resource type     **
  KHE_SET			resource_set;		** resource set      **
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "internal operations"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP_TYPE KheResourceGroupType(KHE_RESOURCE_GROUP rg)      */
/*                                                                           */
/*  Return the resource group type of rg.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP_TYPE KheResourceGroupType(KHE_RESOURCE_GROUP rg)
{
  return rg->resource_group_type;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceGroupMakeInternal(KHE_RESOURCE_TYPE rt,    */
/*    KHE_RESOURCE_GROUP_TYPE resource_group_type, KHE_SOLN soln,            */
/*    char *id, char *name, LSET lset)                                       */
/*                                                                           */
/*  Make a resource group of the given type, but do not add it to rt.        */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceGroupMakeInternal(KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE_GROUP_TYPE resource_group_type, KHE_SOLN soln,
  char *id, char *name /* , LSET lset */)
{
  KHE_RESOURCE_GROUP res;  HA_ARENA a;
  a = (soln != NULL ? KheSolnArena(soln) :
    KheInstanceArena(KheResourceTypeInstance(rt)));
  HaMake(res, a);
  res->back = NULL;
  res->resource_group_type = resource_group_type;
  res->partition_index = -1;  /* means "not a partition" */
  res->rs_rec.rt = rt;
  KheSetInit(res->rs_rec.resource_indexes, a);
  res->id = HnStringCopy(id, a);
  res->name = HnStringCopy(name, a);
  res->finalized = false;
  res->partition = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceGroupCopy(KHE_RESOURCE_GROUP rg,           */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make and return a copy of rg in arena a.  There is no need for a         */
/*  Phase 1 and Phase 2.  If the resource group is not a solution            */
/*  resource group, just return the current resource group.                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceGroupCopy(KHE_RESOURCE_GROUP rg,
  HA_ARENA a)
{
  KHE_RESOURCE_GROUP res;
  if( rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_SOLN )
  {
    /* need a real copy */
    HaMake(res, a);
    res->back = rg->back;
    res->resource_group_type = rg->resource_group_type;
    res->partition_index = -1;
    res->rs_rec.rt = rg->rs_rec.rt;
    KheSetCopy(res->rs_rec.resource_indexes, rg->rs_rec.resource_indexes, a);
    res->id = HnStringCopy(rg->id, a);
    res->name = HnStringCopy(rg->name, a);
    res->finalized = true;  /* no need to finalize here */
    res->partition = NULL;
    return res;
  }
  else
  {
    /* OK to share */
    return rg;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupAddResourceInternal(KHE_RESOURCE_GROUP rg,          */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Add r to rg.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupAddResourceInternal(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)
{
  KheResourceSetAddResource(&rg->rs_rec, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupSubResourceInternal(KHE_RESOURCE_GROUP rg,          */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Subtract r from rg.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupSubResourceInternal(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)
{
  KheResourceSetDeleteResource(&rg->rs_rec, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupUnionInternal(KHE_RESOURCE_GROUP rg,                */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Update rg's resources to be their union with rg2's resources.            */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupUnionInternal(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetUnion(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupIntersectInternal(KHE_RESOURCE_GROUP rg,            */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Update rg's resources to be their intersection with rg2's resources.     */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupIntersectInternal(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetIntersect(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupDifferenceInternal(KHE_RESOURCE_GROUP rg,           */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Update rg's resources to be the set difference of them with rg2's        */
/*  resources.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupDifferenceInternal(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetDifference(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupUnionCount(KHE_RESOURCE_GROUP rg,                    */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return the cardinality of the union of rg and rg2.                       */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupUnionCount(KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetUnionCount(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupIntersectCount(KHE_RESOURCE_GROUP rg,                */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return the cardinality of the intersection of rg and rg2.                */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupIntersectCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetIntersectCount(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupDifferenceCount(KHE_RESOURCE_GROUP rg,               */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return the cardinality of the difference of rg and rg2.                  */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupDifferenceCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetDifferenceCount(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupSymmetricDifferenceCount(KHE_RESOURCE_GROUP rg,      */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return the cardinality of the symmetric difference of rg and rg2.        */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupSymmetricDifferenceCount(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetSymmetricDifferenceCount(&rg->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupResetResourceType(KHE_RESOURCE_GROUP rg)            */
/*                                                                           */
/*  Reset the type of rg to the type of its first resource.                  */
/*                                                                           */
/*  This function is called only by resource type partitioning.              */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupResetResourceType(KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE r;
  if( KheResourceGroupResourceCount(rg) > 0 )
  {
    r = KheResourceGroupResource(rg, 0);
    rg->rs_rec.rt = KheResourceResourceType(r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupFinalize(KHE_RESOURCE_GROUP rg)                     */
/*                                                                           */
/*  Finalize rg.  Since its set of resources will not change after this,     */
/*  finalize the set, and add rg to its resources' lists of user resource    */
/*  groups (if appropriate).                                                 */
/*                                                                           */
/*  Also set rg->partition, although it may have to be done again, if        */
/*  partitions are inferred later on.                                        */
/*                                                                           */
/*  Also search for a better Id, if rg->id is currently NULL.                */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupFinalize(KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE r;  int i;  /* short ix; */  KHE_RESOURCE_GROUP rg2;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceGroupFinalize(%s %p)\n",
      rg->id == NULL ? "-" : rg->id, (void *) rg);
  if( !rg->finalized )
  {
    rg->finalized = true;
    if( rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER )
      for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
      {
	r = KheResourceGroupResource(rg, i);
	KheResourceAddUserResourceGroup(r, rg);
      }
    KheResourceGroupSetPartition(rg);

    if( rg->id == NULL && KheResourceGroupResourceCount(rg) > 0 )
    {
      r = KheResourceGroupResource(rg, 0);
      for( i = 0;  i < KheResourceResourceGroupCount(r);  i++ )
      {
	rg2 = KheResourceResourceGroup(r, i);
	if( rg2 != rg && KheResourceGroupEqual(rg, rg2) && rg2->id != NULL )
	{
	  rg->id = rg2->id;
	  break;
	}
      }
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceGroupFinalize\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "domains and partitions"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SET *KheResourceGroupKheSet(KHE_RESOURCE_GROUP rg)                   */
/*                                                                           */
/*  Return the Khe set of the resource set defining rg.                      */
/*                                                                           */
/*****************************************************************************/

KHE_SET *KheResourceGroupKheSet(KHE_RESOURCE_GROUP rg)
{
  return &rg->rs_rec.resource_indexes;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceGroupResourceSet(KHE_RESOURCE_GROUP rg)      */
/*                                                                           */
/*  Return the resource set defining rg.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_SET KheResourceGroupResourceSet(KHE_RESOURCE_GROUP rg)
{
  return &rg->rs_rec;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupPartitionAdmissible(KHE_RESOURCE_GROUP rg)          */
/*                                                                           */
/*  Return true if this resource group is admissible when inferring          */
/*  resource partitions.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupPartitionAdmissible(KHE_RESOURCE_GROUP rg)
{
  int count = KheResourceGroupResourceCount(rg);
  return count >= 2 &&
    3 * count <= KheResourceTypeResourceCount(rg->rs_rec.rt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupDeclarePartition(KHE_RESOURCE_GROUP rg)             */
/*                                                                           */
/*  Declare that rg is a partition.                                          */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupDeclarePartition(KHE_RESOURCE_GROUP rg)
{
  int i;  KHE_RESOURCE r;
  HnAssert(rg->partition_index == -1,
    "KheResourceGroupDeclarePartition internal error (called twice on rg)");

  /* inform resource type */
  KheResourceTypeAddPartition(rg->rs_rec.rt, rg);

  /* inform instance and obtain index */
  KheInstanceAddPartition(KheResourceTypeInstance(rg->rs_rec.rt), rg,
    &rg->partition_index);

  /* inform rg's resources */
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceSetPartition(r, rg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupSetPartition(KHE_RESOURCE_GROUP rg)                 */
/*                                                                           */
/*  Set the partition attribute of rg as appropriate.                        */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupSetPartition(KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE r;  KHE_RESOURCE_GROUP part_rg;  /* short ix; */
  rg->partition = NULL;
  if( KheResourceGroupResourceCount(rg) > 0 )
  {
    r = KheResourceGroupResource(rg, 0);
    part_rg = KheResourcePartition(r);
    if( part_rg != NULL && KheResourceGroupSubset(rg, part_rg) )
      rg->partition = part_rg;
  }
}

/* ***
void KheResourceGroupSetPartition(KHE_RESOURCE_GROUP rg)
{
  KHE_RESOURCE r;  short ix;
  rg->partition = NULL;
  if( MArraySize(rg->resource_indexes) > 0 )
  {
    ix = MArrayFirst(rg->resource_indexes);
    r = KheInstanceResource(KheResourceTypeInstance(rg->rs_rec.rt), ix);
    if( KheResourcePart ition(r) != NULL &&
	KheResourceGroupSubset(rg, KheResourcePart ition(r)) )
      rg->partition = KheResourcePar tition(r);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceGroupPartition(KHE_RESOURCE_GROUP rg)      */
/*                                                                           */
/*  Return rg's partition, or NULL if none.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceGroupPartition(KHE_RESOURCE_GROUP rg)
{
  return rg->partition;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupMake(KHE_RESOURCE_TYPE rt, char *id, char *name,    */
/*    bool is_partition, KHE_RESOURCE_GROUP *rg)                             */
/*                                                                           */
/*  Make a user-defined resource group with these attributes.                */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupMake(KHE_RESOURCE_TYPE rt, char *id, char *name,
  bool is_partition, KHE_RESOURCE_GROUP *rg)
{
  KHE_RESOURCE_GROUP res;
  HnAssert(
    KheInstanceFinalized(KheResourceTypeInstance(rt)) == KHE_FINALIZED_NONE,
    "KheResourceGroupMake called after KheInstanceMakeEnd");
  HnAssert(rt != NULL, "KheResourceGroupMake: rt parameter is NULL");
  HnAssert(!is_partition || KheResourceTypeHasPartitions(rt),
    "KheResourceGroupMake is_partition when rt has no partitions");
  if( id != NULL && KheResourceTypeRetrieveResourceGroup(rt, id, rg) )
  {
    *rg = NULL;
    return false;
  }
  res = KheResourceGroupMakeInternal(rt, KHE_RESOURCE_GROUP_TYPE_USER,
    NULL, id, name /* , LSetNew() */);
  KheResourceTypeAddResourceGroup(rt, res);
  if( is_partition )
    KheResourceGroupDeclarePartition(res);
  *rg = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupSetBack(KHE_RESOURCE_GROUP rg, void *back)          */
/*                                                                           */
/*  Set the back pointer of rg.                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupSetBack(KHE_RESOURCE_GROUP rg, void *back)
{
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_SOLN ||
      KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupSetBack called after KheInstanceMakeEnd");
  rg->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheResourceGroupBack(KHE_RESOURCE_GROUP rg)                        */
/*                                                                           */
/*  Return the back pointer of rg.                                           */
/*                                                                           */
/*****************************************************************************/

void *KheResourceGroupBack(KHE_RESOURCE_GROUP rg)
{
  return rg->back;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheResourceGroupResourceType(KHE_RESOURCE_GROUP rg)    */
/*                                                                           */
/*  Return the resource_type attribute of rg.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheResourceGroupResourceType(KHE_RESOURCE_GROUP rg)
{
  return rg->rs_rec.rt;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheResourceGroupInstance(KHE_RESOURCE_GROUP rg)             */
/*                                                                           */
/*  Return the instance attribute of rg.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheResourceGroupInstance(KHE_RESOURCE_GROUP rg)
{
  return KheResourceTypeInstance(rg->rs_rec.rt);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceGroupId(KHE_RESOURCE_GROUP rg)                          */
/*                                                                           */
/*  Return the id attribute of rg.                                           */
/*                                                                           */
/*****************************************************************************/

char *KheResourceGroupId(KHE_RESOURCE_GROUP rg)
{
  return rg->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceGroupName(KHE_RESOURCE_GROUP rg)                        */
/*                                                                           */
/*  Return the name attribute of rg.                                         */
/*                                                                           */
/*****************************************************************************/

char *KheResourceGroupName(KHE_RESOURCE_GROUP rg)
{
  return rg->name;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupIsPartition(KHE_RESOURCE_GROUP rg)                  */
/*                                                                           */
/*  Return true if rg is a partition.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupIsPartition(KHE_RESOURCE_GROUP rg)
{
  return rg->partition_index != -1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupPartitionIndex(KHE_RESOURCE_GROUP rg)                */
/*                                                                           */
/*  If rg is a partition, return its index in the instance.  If not,         */
/*  return -1.                                                               */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupPartitionIndex(KHE_RESOURCE_GROUP rg)
{
  return rg->partition_index;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resources"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupAddResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)  */
/*                                                                           */
/*  Add r to rg, first checking that rg is a user-defined resource group.    */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupAddResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)
{
  HnAssert(
    KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupAddResource called after KheInstanceMakeEnd");
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceGroupAddResource given unchangeable resource group");
  KheResourceGroupAddResourceInternal(rg, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupSubResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)  */
/*                                                                           */
/*  Remove r from rg, first checking that rg is user-defined.                */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupSubResource(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)
{
  HnAssert(
    KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupSubResource called after KheInstanceMakeEnd");
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceGroupSubResource given unchangeable resource group");
  KheResourceGroupSubResourceInternal(rg, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupUnion(KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP rg2)*/
/*                                                                           */
/*  Set rg's set of resources to its union with rg2's.                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupUnion(KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(
    KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupUnion called after KheInstanceMakeEnd");
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceGroupUnion given unchangeable resource group");
  KheResourceGroupUnionInternal(rg, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupIntersect(KHE_RESOURCE_GROUP rg,                    */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Set rg's set of resources to its intersection with rg2's.                */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupIntersect(KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(
    KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupIntersect called after KheInstanceMakeEnd");
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceGroupIntersect given unchangeable resource group");
  KheResourceGroupIntersectInternal(rg, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupDifference(KHE_RESOURCE_GROUP rg,                   */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Set rg's set of resources to its difference with rg2's.                  */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupDifference(KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP rg2)
{
  HnAssert(
    KheInstanceFinalized(KheResourceGroupInstance(rg)) == KHE_FINALIZED_NONE,
    "KheResourceGroupDifference called after KheInstanceMakeEnd");
  HnAssert(rg->resource_group_type == KHE_RESOURCE_GROUP_TYPE_USER,
    "KheResourceGroupDifference given unchangeable resource group");
  KheResourceGroupDifferenceInternal(rg, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupResourceCount(KHE_RESOURCE_GROUP rg)                 */
/*                                                                           */
/*  Return the number of resources in rg.                                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupResourceCount(KHE_RESOURCE_GROUP rg)
{
  return KheResourceSetResourceCount(&rg->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceGroupResource(KHE_RESOURCE_GROUP rg, int i)      */
/*                                                                           */
/*  Return the i'th resource of rg.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheResourceGroupResource(KHE_RESOURCE_GROUP rg, int i)
{
  return KheResourceSetResource(&rg->rs_rec, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "set operations"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupContains(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)     */
/*                                                                           */
/*  Return true if rg contains r.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupContains(KHE_RESOURCE_GROUP rg, KHE_RESOURCE r)
{
  return /* KheResourceResourceType(r) == rg->rs_rec.rt && */
    KheResourceSetContainsResource(&rg->rs_rec, r);
}


/*****************************************************************************/
/*                                                                           */
/* bool KheResourceGroupEqual(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)*/
/*                                                                           */
/*  Return true if these two resource groups are equal.                      */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupEqual(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)
{
  return rg1 == rg2 || KheResourceSetEqual(&rg1->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupSubset(KHE_RESOURCE_GROUP rg1,                      */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return true if rg1 is a subset of rg2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupSubset(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetSubset(&rg1->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupDisjoint(KHE_RESOURCE_GROUP rg1,                    */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return true if rg1 and rg2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupDisjoint(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetDisjoint(&rg1->rs_rec, &rg2->rs_rec);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupTypedCmp(KHE_RESOURCE_GROUP rg1,                     */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of resource groups.       */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupTypedCmp(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetTypedCmp(&(rg1->rs_rec), &(rg2->rs_rec));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupCmp(const void *t1, const void *t2)                  */
/*                                                                           */
/*  Untyped comparison function for sorting an array of resource groups.     */
/*                                                                           */
/*****************************************************************************/

int KheResourceGroupCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE_GROUP rg1 = * (KHE_RESOURCE_GROUP *) t1;
  KHE_RESOURCE_GROUP rg2 = * (KHE_RESOURCE_GROUP *) t2;
  return KheResourceGroupTypedCmp(rg1, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupMakeFromKml(KML_ELT resource_group_elt,             */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Make a resource group and add it to ins.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupMakeFromKml(KML_ELT resource_group_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name, *rt_ref;  KML_ELT resource_type_elt;  KHE_RESOURCE_TYPE rt;
  KHE_RESOURCE_GROUP rg;  HA_ARENA a;
  if( !KmlCheck(resource_group_elt, "Id : $Name ResourceType", ke) )
      return false;
  id = KmlAttributeValue(resource_group_elt, 0);
  name = KmlText(KmlChild(resource_group_elt, 0));
  resource_type_elt = KmlChild(resource_group_elt, 1);
  a = KheInstanceArena(ins);
  if( !KmlCheck(resource_type_elt, "Reference", ke) )
    return false;
  rt_ref = KmlAttributeValue(resource_type_elt, 0);
  if( !KheInstanceRetrieveResourceType(ins, rt_ref, &rt) )
    return KmlError(ke, a, KmlLineNum(resource_type_elt),
      KmlColNum(resource_type_elt),
      "<ResourceType> Reference %s unknown", rt_ref);
  if( !KheResourceGroupMake(rt, id, name, false, &rg) )
    return KmlError(ke, a, KmlLineNum(resource_group_elt),
      KmlColNum(resource_group_elt),
      "<ResourceGroup> Id \"%s\" used previously", id);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupWrite(KHE_RESOURCE_GROUP rg, KML_FILE kf)           */
/*                                                                           */
/*  Write rg (just its id, name, and resource type) to kf.                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupWrite(KHE_RESOURCE_GROUP rg, KML_FILE kf)
{
  KmlBegin(kf, "ResourceGroup");
  KmlAttribute(kf, "Id", rg->id);
  HnAssert(rg->id != NULL, "KheArchiveWrite: Id missing from ResourceGroup");
  KmlEltPlainText(kf, "Name", rg->name);
  HnAssert(rg->name != NULL,"KheArchiveWrite: Name missing from ResourceGroup");
  KmlEltAttribute(kf, "ResourceType", "Reference",
    KheResourceTypeId(KheResourceTypeOriginalResourceType(rg->rs_rec.rt)));
  KmlEnd(kf, "ResourceGroup");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupDebug(KHE_RESOURCE_GROUP rg, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheResourceGroupDebug(KHE_RESOURCE_GROUP rg, int verbosity,
  int indent, FILE *fp)
{
  KHE_RESOURCE r1, r2;  int i;
  if( verbosity == 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( KheResourceGroupResourceCount(rg) == 0 )
      fprintf(fp, "{}");
    else if( KheResourceGroupResourceCount(rg) == 1 )
    {
      r1 = KheResourceGroupResource(rg, 0);
      if( KheResourceId(r1) != NULL )
	fprintf(fp, "{%s}", KheResourceId(r1));
      else if( KheResourceGroupId(rg) != NULL )
	fprintf(fp, "%s", KheResourceGroupId(rg));
      else
	fprintf(fp, "{-}");
    }
    else if( KheResourceGroupId(rg) != NULL )
      fprintf(fp, "%s", KheResourceGroupId(rg));
    else
    {
      r1 = KheResourceGroupResource(rg, 0);
      r2 = KheResourceGroupResource(rg, KheResourceGroupResourceCount(rg) - 1);
      fprintf(fp, "{%s%s%s}",
	KheResourceId(r1) != NULL ? KheResourceId(r1) : "-",
	KheResourceGroupResourceCount(rg) == 2 ? "," : "..",
	KheResourceId(r2) != NULL ? KheResourceId(r2) : "-");
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
  else if( verbosity >= 2 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    if( KheResourceGroupId(rg) != NULL )
      fprintf(fp, "%s", KheResourceGroupId(rg));
    fprintf(fp, "{");
    for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
    {
      r1 = KheResourceGroupResource(rg, i);
      if( i > 0 )
	fprintf(fp, ", ");
      fprintf(fp, "%s", KheResourceId(r1) != NULL ? KheResourceId(r1) : "-");
    }
    fprintf(fp, "}");
    /* *** sort this out some time
    if( verbosity >= 3 )
      fprintf(fp, " %s", KheResourceSetShow(&rg->rs_rec));
    *** */
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}

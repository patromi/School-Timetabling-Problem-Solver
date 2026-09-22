
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
/*  FILE:         khe_resource_type.c                                        */
/*  DESCRIPTION:  A type of resource                                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0		/* resource type partitioning */
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE - a type of resource                                   */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_type_rec {
  void				*back;			/* back pointer      */
  KHE_INSTANCE			instance;		/* enclosing instance*/
  int				index;			/* index in instance */
  char				*id;			/* Id                */
  char				*name;			/* Name              */
  bool				has_partitions;		/* has partitions    */
  bool				demand_all_preassigned;	/* all preassigned   */
  int				avoid_split_assts_count; /* asacount         */
  int				limit_resources_count;	/* no of limit r.    */
  float				max_workload_per_time;	/* max workload per t*/
  bool				rt_partitioning_on;	/* rt partitioning   */
  KHE_RESOURCE_TYPE		original_rt;		/* before rt part.   */
  ARRAY_KHE_RESOURCE_GROUP	resource_group_array;	/* resource groups   */
  TABLE_KHE_RESOURCE_GROUP	resource_group_table;	/* resource groups   */
  ARRAY_KHE_RESOURCE_GROUP	partition_array;	/* partitions        */
  KHE_RESOURCE_GROUP		full_resource_group;	/* full resource grp */
  KHE_RESOURCE_GROUP		empty_resource_group;	/* empty resource grp*/
  ARRAY_KHE_RESOURCE		resource_array;		/* resources         */
  TABLE_KHE_RESOURCE		resource_table;		/* resources         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeMake(KHE_INSTANCE ins, char *id, char *name,         */
/*    bool has_partitions, KHE_RESOURCE_TYPE *rt)                            */
/*                                                                           */
/*  Create a new resource type with these attributes and add it to ins.      */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeMakeInternal(KHE_INSTANCE ins, char *id, char *name,
  bool has_partitions, KHE_RESOURCE_TYPE orig_rt, KHE_RESOURCE_TYPE *rt)
{
  KHE_RESOURCE_TYPE res;  HA_ARENA a;
  if( id != NULL && KheInstanceRetrieveResourceType(ins, id, &res) )
  {
    *rt = NULL;
    return false;
  }
  a = KheInstanceArena(ins);
  HaMake(res, a);
  res->back = NULL;
  res->instance = ins;
  res->id = HnStringCopy(id, a);  /* must precede KheInstanceAddResourceType! */
  KheInstanceAddResourceType(ins, res, &res->index);
  res->name = HnStringCopy(name, a);
  res->has_partitions = has_partitions;
  res->demand_all_preassigned = true;  /* until proved false */
  res->avoid_split_assts_count = 0;
  res->limit_resources_count = 0;
  res->max_workload_per_time = 0.0;
  res->rt_partitioning_on = false;
  res->original_rt = (orig_rt == NULL ? res : orig_rt);
  HaArrayInit(res->resource_group_array, a);
  HaArrayInit(res->partition_array, a);
  HnTableInit(res->resource_group_table, a);
  res->full_resource_group = KheResourceGroupMakeInternal(res,
    KHE_RESOURCE_GROUP_TYPE_FULL, NULL, NULL, NULL /* , LSetNew() */);
  res->empty_resource_group = KheResourceGroupMakeInternal(res,
    KHE_RESOURCE_GROUP_TYPE_EMPTY, NULL, NULL, NULL /* , LSetNew() */);
  HaArrayInit(res->resource_array, a);
  HnTableInit(res->resource_table, a);
  *rt = res;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeMake(KHE_INSTANCE ins, char *id, char *name,         */
/*    bool has_partitions, KHE_RESOURCE_TYPE *rt)                            */
/*                                                                           */
/*  Create a new resource type with these attributes and add it to ins.      */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeMake(KHE_INSTANCE ins, char *id, char *name,
  bool has_partitions, KHE_RESOURCE_TYPE *rt)
{
  HnAssert(KheInstanceFinalized(ins) == KHE_FINALIZED_NONE,
    "KheResourceTypeMake called after KheInstanceMakeEnd");
  return KheResourceTypeMakeInternal(ins, id, name, has_partitions, NULL, rt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeSetBack(KHE_RESOURCE_TYPE rt, void *back)            */
/*                                                                           */
/*  Set the back pointer of rt.                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeSetBack(KHE_RESOURCE_TYPE rt, void *back)
{
  HnAssert(KheInstanceFinalized(rt->instance) == KHE_FINALIZED_NONE,
    "KheResourceTypeSetBack called after KheInstanceMakeEnd");
  rt->back = back;
}


/*****************************************************************************/
/*                                                                           */
/*  void *KheResourceTypeBack(KHE_RESOURCE_TYPE rt)                          */
/*                                                                           */
/*  Return the back pointer of rt.                                           */
/*                                                                           */
/*****************************************************************************/

void *KheResourceTypeBack(KHE_RESOURCE_TYPE rt)
{
  return rt->back;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeResetResourceGroupResourceTypes(KHE_RESOURCE_TYPE rt)*/
/*                                                                           */
/*  Reset the resource types of the resource groups of rt.                   */
/*                                                                           */
/*  This function is called only by resource type partitioning.              */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeResetResourceGroupResourceTypes(KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_RESOURCE_GROUP rg;
  HaArrayForEach(rt->resource_group_array, rg, i)
    KheResourceGroupResetResourceType(rg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeFinalize(KHE_RESOURCE_TYPE rt)                       */
/*                                                                           */
/*  Finalize rt.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeFinalize(KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_RESOURCE_GROUP rg;  /* KHE_RESOURCE r; */
  HaArrayForEach(rt->resource_group_array, rg, i)
    KheResourceGroupFinalize(rg);
  KheResourceGroupFinalize(rt->full_resource_group);
  KheResourceGroupFinalize(rt->empty_resource_group);
  /* *** resources are finalized separately, later on after constraints
  HaArrayForEach(rt->resource_array, r, i)
    KheResourceFinalize(r);
    ** KheResourceGroupFinalize(KheResourceSingletonResourceGroup(r)); **
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INSTANCE KheResourceTypeInstance(KHE_RESOURCE_TYPE rt)               */
/*                                                                           */
/*  Return the instance attribute of rt.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_INSTANCE KheResourceTypeInstance(KHE_RESOURCE_TYPE rt)
{
  return rt->instance;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeIndex(KHE_RESOURCE_TYPE rt)                           */
/*                                                                           */
/*  Return the index of rt in the enclosing instance.                        */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypeIndex(KHE_RESOURCE_TYPE rt)
{
  return rt->index;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceTypeId(KHE_RESOURCE_TYPE rt)                            */
/*                                                                           */
/*  Return the id attribute of rt.                                           */
/*                                                                           */
/*****************************************************************************/

char *KheResourceTypeId(KHE_RESOURCE_TYPE rt)
{
  return rt == NULL ? "(NULL)" : rt->id;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceTypeName(KHE_RESOURCE_TYPE rt)                          */
/*                                                                           */
/*  Return the name attribute of rt.                                         */
/*                                                                           */
/*****************************************************************************/

char *KheResourceTypeName(KHE_RESOURCE_TYPE rt)
{
  return rt->name;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeHasPartitions(KHE_RESOURCE_TYPE rt)                  */
/*                                                                           */
/*  Return the has_partitions attribute of rt.                               */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeHasPartitions(KHE_RESOURCE_TYPE rt)
{
  return rt->has_partitions;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource groups"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeAddResourceGroup(KHE_RESOURCE_TYPE rt,               */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add rg to rt.                                                            */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeAddResourceGroup(KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE_GROUP rg)
{
  HaArrayAddLast(rt->resource_group_array, rg);
  if( KheResourceGroupId(rg) != NULL )
    HnTableAdd(rt->resource_group_table, KheResourceGroupId(rg), rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeResourceGroupCount(KHE_RESOURCE_TYPE rt)              */
/*                                                                           */
/*  Return the number of resource groups in rt.                              */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypeResourceGroupCount(KHE_RESOURCE_TYPE rt)
{
  return HaArrayCount(rt->resource_group_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceTypeResourceGroup(KHE_RESOURCE_TYPE rt,    */
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th resource group of rt.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceTypeResourceGroup(KHE_RESOURCE_TYPE rt,
  int i)
{
  return HaArray(rt->resource_group_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeRetrieveResourceGroup(KHE_RESOURCE_TYPE rt,          */
/*    char *id, KHE_RESOURCE_GROUP *rg)                                      */
/*                                                                           */
/*  Retrieve a resource group by id from rt.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeRetrieveResourceGroup(KHE_RESOURCE_TYPE rt,
  char *id, KHE_RESOURCE_GROUP *rg)
{
  int pos;
  return HnTableRetrieve(rt->resource_group_table, id, *rg, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "partitions"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeAddPartition(KHE_RESOURCE_TYPE rt,                   */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Inform rt that rg is a partition.                                        */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeAddPartition(KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP rg)
{
  HaArrayAddLast(rt->partition_array, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypePartitionCount(KHE_RESOURCE_TYPE rt)                  */
/*                                                                           */
/*  Return the number of partitions in rt.                                   */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypePartitionCount(KHE_RESOURCE_TYPE rt)
{
  return HaArrayCount(rt->partition_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceTypePartition(KHE_RESOURCE_TYPE rt, int i) */
/*                                                                           */
/*  Return the i'th partition of rt.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceTypePartition(KHE_RESOURCE_TYPE rt, int i)
{
  return HaArray(rt->partition_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheResourceTypeFullResourceGroup(KHE_RESOURCE_TYPE rt)*/
/*                                                                           */
/*  Return the "all" resource group of rt.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceTypeFullResourceGroup(KHE_RESOURCE_TYPE rt)
{
  return rt->full_resource_group;
}


/*****************************************************************************/
/*                                                                           */
/* KHE_RESOURCE_GROUP KheResourceTypeEmptyResourceGroup(KHE_RESOURCE_TYPE rt)*/
/*                                                                           */
/*  Return the empty resource group of rt.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheResourceTypeEmptyResourceGroup(KHE_RESOURCE_TYPE rt)
{
  return rt->empty_resource_group;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resources"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeAddResource(KHE_RESOURCE_TYPE rt, KHE_RESOURCE r)    */
/*                                                                           */
/*  Add r to rt, including a call to KheResourceSetResourceTypeIndex.        */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeAddResource(KHE_RESOURCE_TYPE rt, KHE_RESOURCE r)
{
  char *id;

  /* add r to rt's symbol table, if it has an Id */
  id = KheResourceId(r);
  if( DEBUG2 )
    fprintf(stderr, " KheResourceTypeAddResource(%s, %s)\n",
      KheResourceTypeId(rt), id == NULL ? "-" : id);
  if( id != NULL )
    HnTableAdd(rt->resource_table, id, r);

  /* add r to rt's array, including setting r's index */
  KheResourceSetResourceTypeIndex(r, HaArrayCount(rt->resource_array));
  HaArrayAddLast(rt->resource_array, r);

  /* add r to rt's full resource group */
  KheResourceGroupAddResourceInternal(rt->full_resource_group, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeDeleteResource(KHE_RESOURCE_TYPE rt, KHE_RESOURCE r, */
/*    int resource_type_index)                                               */
/*                                                                           */
/*  Delete r from rt.  Its index in rt's array is resource_type_index.       */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeDeleteResource(KHE_RESOURCE_TYPE rt, KHE_RESOURCE r,
  int resource_type_index)
{
  int pos;  char *id;  KHE_RESOURCE r2;

  /* delete r from rt's symbol table, if it has an Id */
  id = KheResourceId(r);
  if( DEBUG2 )
    fprintf(stderr, " KheResourceTypeDeleteResource(%s, %s, %d)\n",
      KheResourceTypeId(rt), id == NULL ? "-" : id, resource_type_index);
  if( id != NULL )
  {
    if( !HnTableContains(rt->resource_table, id, pos) )
      HnAbort("KheResourceTypeDeleteResource internal error 1");
    HnTableDelete(rt->resource_table, pos);
  }

  /* delete r from rt's array, being careful to reset r2's index */
  HnAssert(HaArray(rt->resource_array, resource_type_index) == r,
    "KheResourceTypeDeleteResource internal error 2");
  HaArrayDeleteAndPlug(rt->resource_array, resource_type_index);
  if( resource_type_index < HaArrayCount(rt->resource_array) )
  {
    r2 = HaArray(rt->resource_array, resource_type_index);
    KheResourceSetResourceTypeIndex(r2, resource_type_index);
  }

  /* delete r from rt's full resource group */
  KheResourceGroupSubResourceInternal(rt->full_resource_group, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeResourceCount(KHE_RESOURCE_TYPE rt)                   */
/*                                                                           */
/*  Return the number of resources in rt.                                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypeResourceCount(KHE_RESOURCE_TYPE rt)
{
  return HaArrayCount(rt->resource_array);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceTypeResource(KHE_RESOURCE_TYPE rt, int i)        */
/*                                                                           */
/*  Return the i'th resource of rt.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheResourceTypeResource(KHE_RESOURCE_TYPE rt, int i)
{
  return HaArray(rt->resource_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeRetrieveResource(KHE_RESOURCE_TYPE rt,               */
/*    char *id, KHE_RESOURCE *r)                                             */
/*                                                                           */
/*  Retrieve a resource with the given id from rt.                           */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeRetrieveResource(KHE_RESOURCE_TYPE rt,
  char *id, KHE_RESOURCE *r)
{
  int pos;
  return HnTableRetrieve(rt->resource_table, id, *r, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "inferring partitions"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_CLUSTER - a cluster of resources                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_cluster_rec {
  ARRAY_KHE_RESOURCE		resources;	/* resources in cluster      */
} *KHE_RESOURCE_CLUSTER;

typedef HA_ARRAY(KHE_RESOURCE_CLUSTER) ARRAY_KHE_RESOURCE_CLUSTER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_CLUSTER KheResourceClusterMake(KHE_RESOURCE r, HA_ARENA a)  */
/*                                                                           */
/*  Make a new cluster containing just r, in arena a.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_CLUSTER KheResourceClusterMake(KHE_RESOURCE r, HA_ARENA a)
{
  KHE_RESOURCE_CLUSTER res;
  HaMake(res, a);
  HaArrayInit(res->resources, a);
  HaArrayAddLast(res->resources, r);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceClusterFree(KHE_RESOURCE_CLUSTER rc)                     */
/*                                                                           */
/*  Free the memory consumed by rc.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceClusterFree(KHE_RESOURCE_CLUSTER rc)
{
  MArrayFree(rc->resources);
  MFree(rc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceClusterMerge(KHE_RESOURCE_CLUSTER rc1,                   */
/*    KHE_RESOURCE_CLUSTER rc2, ARRAY_KHE_RESOURCE_CLUSTER *clusters)        */
/*                                                                           */
/*  Merge rc2 into rc1 and update *clusters appropriately.                   */
/*                                                                           */
/*****************************************************************************/

static void KheResourceClusterMerge(KHE_RESOURCE_CLUSTER rc1,
  KHE_RESOURCE_CLUSTER rc2, ARRAY_KHE_RESOURCE_CLUSTER *clusters)
{
  KHE_RESOURCE r;  int i;
  HaArrayForEach(rc2->resources, r, i)
    HaArrayPut(*clusters, KheResourceResourceTypeIndex(r), rc1);
  HaArrayAppend(rc1->resources, rc2->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceClusterDebug(KHE_RESOURCE_CLUSTER rc, FILE *fp)          */
/*                                                                           */
/*  Debug print of rc onto fp.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheResourceClusterDebug(KHE_RESOURCE_CLUSTER rc, FILE *fp)
{
  KHE_RESOURCE r;  int i;
  fprintf(fp, "{");
  HaArrayForEach(rc->resources, r, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%s", KheResourceId(r) == NULL ? "-" : KheResourceId(r));
  }
  fprintf(fp, "}");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupEqualsCluster(KHE_RESOURCE_GROUP rg,                */
/*    KHE_RESOURCE_CLUSTER rc)                                               */
/*                                                                           */
/*  Return true if the resources of rc equal the resources of rg.            */
/*                                                                           */
/*****************************************************************************/

bool KheResourceGroupEqualsCluster(KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_CLUSTER rc)
{
  KHE_RESOURCE r;  int i;
  if( HaArrayCount(rc->resources) != KheResourceGroupResourceCount(rg) )
    return false;
  HaArrayForEach(rc->resources, r, i)
    if( !KheResourceGroupContains(rg, r) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeClusterIsResourceGroup(KHE_RESOURCE_TYPE rt,         */
/*    KHE_RESOURCE_CLUSTER rc, KHE_RESOURCE_GROUP *rg)                       */
/*                                                                           */
/*  If rt contains a resource group whose elements are equal to the          */
/*  elements of rc, return true and set *rg to that resource group,          */
/*  else return false.                                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTypeClusterIsResourceGroup(KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE_CLUSTER rc, KHE_RESOURCE_GROUP *rg)
{
  int i;
  HaArrayForEach(rt->resource_group_array, *rg, i)
    if( KheResourceGroupEqualsCluster(*rg, rc) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeInferPartitions(KHE_RESOURCE_TYPE rt)                */
/*                                                                           */
/*  Infer partitions for rt.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeInferPartitions(KHE_RESOURCE_TYPE rt)
{
  KHE_RESOURCE r1, r2;  int i1, i2;  KHE_RESOURCE_CLUSTER rc1, rc2;
  ARRAY_KHE_RESOURCE_CLUSTER all_clusters, clusters_by_resource;
  KHE_RESOURCE_GROUP rg;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceTypeInferPartitions(%s)\n",
      rt->id == NULL ? "-" : rt->id);

  /* arena for temporary memory, all deleted at the end */
  a = KheInstanceArenaBegin(rt->instance);

  /* initialize to one cluster per resource */
  HaArrayInit(all_clusters, a);
  HaArrayInit(clusters_by_resource, a);
  HaArrayForEach(rt->resource_array, r1, i1)
  {
    rc1 = KheResourceClusterMake(r1, a);
    HaArrayAddLast(all_clusters, rc1);
    HaArrayAddLast(clusters_by_resource, rc1);
  }

  /* merge clusters that contain similar resources */
  for( i1 = 0;  i1 < HaArrayCount(rt->resource_array);  i1++ )
  {
    r1 = HaArray(rt->resource_array, i1);
    rc1 = HaArray(clusters_by_resource, i1);
    for( i2 = i1 + 1;  i2 < HaArrayCount(rt->resource_array);  i2++ )
    {
      r2 = HaArray(rt->resource_array, i2);
      rc2 = HaArray(clusters_by_resource, i2);
      if( rc1 != rc2 && KheResourceSimilar(r1, r2) )
        KheResourceClusterMerge(rc1, rc2, &clusters_by_resource);
    }
  }

  /* add partitions to rt, one partition for each cluster */
  rt->has_partitions = true;
  HaArrayForEach(clusters_by_resource, rc1, i1)
    if( HaArray(rt->resource_array, i1) == HaArrayFirst(rc1->resources) )
    {
      /* at this point we are visiting the clusters once each */
      if( DEBUG1 )
      {
	fprintf(stderr, "  adding partition ");
	KheResourceClusterDebug(rc1, stderr);
	fprintf(stderr, "\n");
      }

      /* find or make a resource group rg with rc1's resources */
      if( HaArrayCount(rc1->resources) == 1 )
	rg = KheResourceSingletonResourceGroup(HaArrayFirst(rc1->resources));
      else if( HaArrayCount(rc1->resources) == HaArrayCount(rt->resource_array) )
	rg = KheResourceTypeFullResourceGroup(rt);
      else if( !KheResourceTypeClusterIsResourceGroup(rt, rc1, &rg) )
      {
	/* make a new resource group with these resources */
	rg = KheResourceGroupMakeInternal(rt, KHE_RESOURCE_GROUP_TYPE_PARTITION,
	  NULL, NULL, NULL /* , LSetNew() */);
	HaArrayForEach(rc1->resources, r2, i2)
	  KheResourceGroupAddResourceInternal(rg, r2);
	KheResourceGroupFinalize(rg);
      }

      /* declare rg to be a partition */
      KheResourceGroupDeclarePartition(rg);

      /* ***
      KheResourceGroupSetIsPartition(rg, true);
      HaArrayAddLast(rt->partition_array, rg);
      HaArrayForEach(rc1->resources, r2, i2)
	KheResourceS etPartition(r2, rg);
      *** */
    }

  /* recalculate the partitions of resource groups */
  HaArrayForEach(rt->resource_group_array, rg, i1)
    KheResourceGroupSetPartition(rg);

  /* free the clusters and arrays of clusters */
  KheInstanceArenaEnd(rt->instance, a);
  /* ***
  HaArrayForEach(all_clusters, rc1, i1)
    KheResourceClusterFree(rc1);
  MArrayFree(all_clusters);
  MArrayFree(clusters_by_resource);
  *** */
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceTypeInferPartitions returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource type partitioning"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeResourceTypePartitioningOn(KHE_RESOURCE_TYPE rt)     */
/*                                                                           */
/*  Return the value of the rt_partitioning_on attribute of rt.              */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeResourceTypePartitioningOn(KHE_RESOURCE_TYPE rt)
{
  return rt->rt_partitioning_on;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeSetResourceTypePartitioningOn(KHE_RESOURCE_TYPE rt,  */
/*    bool val)                                                              */
/*                                                                           */
/*  Set the rt_partitioning_on attribute of rt to val.                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeSetResourceTypePartitioningOn(KHE_RESOURCE_TYPE rt,
  bool val)
{
  rt->rt_partitioning_on = val;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheResourceTypeOriginalResourceType(                   */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Return the resource type that rt is a resource type partition of, or     */
/*  rt itself if it was never partitioned.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheResourceTypeOriginalResourceType(
  KHE_RESOURCE_TYPE rt)
{
  return rt->original_rt;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeMakeFromKml(KML_ELT resource_type_elt,               */
/*    KHE_INSTANCE ins, KML_ERROR *ke)                                       */
/*                                                                           */
/*  Add a resource type based on resource_type_elt to ins.                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeMakeFromKml(KML_ELT resource_type_elt,
  KHE_INSTANCE ins, KML_ERROR *ke)
{
  char *id, *name;  KHE_RESOURCE_TYPE rt;
  if( !KmlCheck(resource_type_elt, "Id : $Name", ke) )
    return false;
  id = KmlAttributeValue(resource_type_elt, 0);
  name = KmlText(KmlChild(resource_type_elt, 0));
  if( !KheResourceTypeMake(ins, id, name, false, &rt) )
    return KmlError(ke, KheInstanceArena(ins),
      KmlLineNum(resource_type_elt), KmlColNum(resource_type_elt),
      "<ResourceType> Id \"%s\" used previously", id);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeWrite(KHE_RESOURCE_TYPE rt, KML_FILE kf)             */
/*                                                                           */
/*  Write rt (just its name, not its members) to kf.                         */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeWrite(KHE_RESOURCE_TYPE rt, KML_FILE kf)
{
  HnAssert(rt->id != NULL, "KheArchiveWrite: Id missing from ResourceType");
  HnAssert(rt->name != NULL, "KheArchiveWrite: Name missing from ResourceType");
  KmlEltAttributeEltPlainText(kf, "ResourceType", "Id", rt->id,
    "Name", rt->name);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeWriteResourceGroups(KHE_RESOURCE_TYPE rt,            */
/*    KML_FILE kf, bool *rg_written)                                         */
/*                                                                           */
/*  Write the resource groups (just their names) of rt to kf.                */
/*                                                                           */
/*  If *rg_written == false, then the initial <ResourceGroups> has not       */
/*  yet been written, so do that and set *rg_written to true.                */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeWriteResourceGroups(KHE_RESOURCE_TYPE rt, KML_FILE kf,
  bool *rg_written)
{
  KHE_RESOURCE_GROUP rg;  int i;
  HaArrayForEach(rt->resource_group_array, rg, i)
  {
    if( !(*rg_written) )
    {
      KmlBegin(kf, "ResourceGroups");
      *rg_written = true;
    }
    KheResourceGroupWrite(rg, kf);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeWriteResources(KHE_RESOURCE_TYPE rt, KML_FILE kf)    */
/*                                                                           */
/*  Write the resources of rt to kf.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
void KheResourceTypeWriteResources(KHE_RESOURCE_TYPE rt, KML_FILE kf)
{
  KHE_RESOURCE r;  int i;
  HaArrayForEach(rt->resource_array, r, i)
    KheResourceWrite(r, kf);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand and avoid split assignments count"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeDemandNotAllPreassigned(KHE_RESOURCE_TYPE rt)        */
/*                                                                           */
/*  Called to inform rt that demands for its resources are not all           */
/*  preassigned, as it initially assumes.                                    */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeDemandNotAllPreassigned(KHE_RESOURCE_TYPE rt)
{
  rt->demand_all_preassigned = false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTypeDemandIsAllPreassigned(KHE_RESOURCE_TYPE rt)         */
/*                                                                           */
/*  Return true if all resource demands which could be satisfied by          */
/*  resources of type rt are preassignments.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTypeDemandIsAllPreassigned(KHE_RESOURCE_TYPE rt)
{
  HnAssert(KheInstanceFinalized(rt->instance) >= KHE_FINALIZED_ALL,
    "KheResourceTypeDemandIsAllPreassigned called before KheInstanceMakeEnd");
  return rt->demand_all_preassigned;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeIncrementAvoidSplitAssignmentsCount(                 */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Increment the record of the number of avoid split assignments            */
/*  constraints of type rt.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeIncrementAvoidSplitAssignmentsCount(KHE_RESOURCE_TYPE rt)
{
  rt->avoid_split_assts_count++;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeAvoidSplitAssignmentsCount(KHE_RESOURCE_TYPE rt)      */
/*                                                                           */
/*  Return the number of points of application of avoid split assignments    */
/*  constraints that apply to event resources of type rt.                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypeAvoidSplitAssignmentsCount(KHE_RESOURCE_TYPE rt)
{
  return rt->avoid_split_assts_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeIncrementLimitResourcesCount(KHE_RESOURCE_TYPE rt,   */
/*    int i)                                                                 */
/*                                                                           */
/*  Increase the record of the number of points of application of limit      */
/*  resources constraints of type rt by i.                                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeIncrementLimitResourcesCount(KHE_RESOURCE_TYPE rt, int i)
{
  rt->limit_resources_count += i;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeLimitResourcesCount(KHE_RESOURCE_TYPE rt)             */
/*                                                                           */
/*  Return the number of points of application of limit resources            */
/*  constraints that apply to event resources of type rt.                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceTypeLimitResourcesCount(KHE_RESOURCE_TYPE rt)
{
  return rt->limit_resources_count;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeAddMaxWorkloadPerTime(KHE_RESOURCE_TYPE rt, float x) */
/*                                                                           */
/*  Report to rt an event resource with max workload per time x.             */
/*                                                                           */
/*****************************************************************************/

void KheResourceTypeAddMaxWorkloadPerTime(KHE_RESOURCE_TYPE rt, float x)
{
  if( x > rt->max_workload_per_time )
  {
    rt->max_workload_per_time = x;
    if( DEBUG3 )
      fprintf(stderr, "  KheResourceTypeAddMaxWorkloadPerTime(%s, %.1f)\n",
	KheResourceTypeId(rt), x);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  float KheResourceTypeMaxWorkloadPerTime(KHE_RESOURCE_TYPE rt)            */
/*                                                                           */
/*  Return the maximum, over all event resources er of type rt, of the       */
/*  workload per time of er.                                                 */
/*                                                                           */
/*****************************************************************************/

float KheResourceTypeMaxWorkloadPerTime(KHE_RESOURCE_TYPE rt)
{
  return rt->max_workload_per_time;
}

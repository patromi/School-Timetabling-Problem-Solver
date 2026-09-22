
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
/*  FILE:         khe_resource_set.c                                         */
/*  DESCRIPTION:  Resource sets                                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET                                                         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_interns.h
struct khe_resource_set_rec
{
  KHE_RESOURCE_TYPE		rt;			** encl resource type**
  KHE_SET			resource_indexes;	** indexes in rt     **
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceSetMake(KHE_RESOURCE_TYPE rt, HA_ARENA a)    */
/*                                                                           */
/*  Make a new, empty resource set with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_SET KheResourceSetMake(KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_RESOURCE_SET res;
  HaMake(res, a);
  res->rt = rt;
  KheSetInit(res->resource_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_SET KheResourceSetCopy(KHE_RESOURCE_SET rs, HA_ARENA a)     */
/*                                                                           */
/*  Return a copy of rs in a.                                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_SET KheResourceSetCopy(KHE_RESOURCE_SET rs, HA_ARENA a)
{
  KHE_RESOURCE_SET res;
  HaMake(res, a);
  res->rt = rs->rt;
  KheSetCopy(res->resource_indexes, rs->resource_indexes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetCopyElements(KHE_RESOURCE_SET dst_rs,                 */
/*    KHE_RESOURCE_SET src_rs)                                               */
/*                                                                           */
/*  Replace whatever dst_rs contains now by whatever src_rs contains.        */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetCopyElements(KHE_RESOURCE_SET dst_rs,
  KHE_RESOURCE_SET src_rs)
{
  dst_rs->rt = src_rs->rt;
  KheSetCopyElements(dst_rs->resource_indexes, src_rs->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheResourceSetResourceType(KHE_RESOURCE_SET rs)        */
/*                                                                           */
/*  Return the resource type attribute of rs.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheResourceSetResourceType(KHE_RESOURCE_SET rs)
{
  return rs->rt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetSetResourceType(KHE_RESOURCE_SET rs,                  */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Set (that is, reset) the resource type of rs to rt.  This is done        */
/*  only by resource type partitioning.                                      */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetSetResourceType(KHE_RESOURCE_SET rs, KHE_RESOURCE_TYPE rt)
{
  rs->rt = rt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetClear(KHE_RESOURCE_SET rs)                            */
/*                                                                           */
/*  Clear rs.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetClear(KHE_RESOURCE_SET rs)
{
  KheSetClear(rs->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGetResourceIndex(KHE_RESOURCE r)                                  */
/*                                                                           */
/*  Return the index of r, or -1 if r is NULL.                               */
/*                                                                           */
/*  Implementation note.  As part of implementing resource type              */
/*  partitioning I have changed the representation from a set                */
/*  of indexes into the resources of the resource type to a set of           */
/*  indexes into the resources of the instance.  This does not need          */
/*  to be changed if the resource type changes.                              */
/*                                                                           */
/*****************************************************************************/

static int KheGetResourceIndex(KHE_RESOURCE r)
{
  return (r == NULL ? -1 : KheResourceInstanceIndex(r));
  /* return (r == NULL ? -1 : KheResourceResourceTypeIndex(r)); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetAddResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r)      */
/*                                                                           */
/*  Add r to rs, or do nothing if r is already present.                      */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetAddResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r)
{
  KheSetInsert(rs->resource_indexes, KheGetResourceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetAddResourceGroup(KHE_RESOURCE_SET rs,                 */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add the resources of rg to rs.                                           */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetAddResourceGroup(KHE_RESOURCE_SET rs, KHE_RESOURCE_GROUP rg)
{
  /* doing a set union is still to do */
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    KheResourceSetAddResource(rs, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetDeleteResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r)   */
/*                                                                           */
/*  Delete r from rs, if present.                                            */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetDeleteResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r)
{
  KheSetDelete(rs->resource_indexes, KheGetResourceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetDeleteLastResource(KHE_RESOURCE_SET rs)               */
/*                                                                           */
/*  Delete the last element of rs.                                           */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetDeleteLastResource(KHE_RESOURCE_SET rs)
{
  KheSetDeleteLast(rs->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetResourceCount(KHE_RESOURCE_SET rs)                     */
/*                                                                           */
/*  Return the number of resources in rs.                                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetResourceCount(KHE_RESOURCE_SET rs)
{
  return KheSetCount(rs->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceSetResource(KHE_RESOURCE_SET rs, int i)          */
/*                                                                           */
/*  Return the i'th resource of rs.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheResourceSetResource(KHE_RESOURCE_SET rs, int i)
{
  int index;  KHE_INSTANCE ins;
  ins = KheResourceTypeInstance(rs->rt);
  index = KheSetGet(rs->resource_indexes, i);
  return (index == -1 ? NULL : KheInstanceResource(ins, index));
  /* return (index == -1 ? NULL : KheResourceTypeResource(rs->rt, index)); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetResourceIndex(KHE_RESOURCE_SET rs, int i)              */
/*                                                                           */
/*  Return the index of the i'th resource of rs.                             */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used, and rather dangerous; better deleted
int KheResourceSetResourceIndex(KHE_RESOURCE_SET rs, int i)
{
  return KheSetGet(rs->resource_indexes, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "union operations"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetUnion(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)     */
/*                                                                           */
/*  Update rs1 to contain the union of its initial value with rs2.           */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetUnion(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  KheSetUnion(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetUnionGroup(KHE_RESOURCE_SET rs1,                      */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetUnion, only the second parameter is a resource group. */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetUnionGroup(KHE_RESOURCE_SET rs1, KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetUnion(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetUnionCount(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2) */
/*                                                                           */
/*  Return the number of elements in the union of rs1 with rs2.              */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetUnionCount(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetUnionCount(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetUnionCountGroup(KHE_RESOURCE_SET rs1,                  */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetUnionCount, only the second parameter is a            */
/*  resource group.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetUnionCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetUnionCount(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "intersect operations"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetIntersect(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2) */
/*                                                                           */
/*  Update rs1 to contain the intersection of its initial value with rs2.    */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetIntersect(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  KheSetIntersect(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetIntersectGroup(KHE_RESOURCE_SET rs1,                  */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetIntersect, only the second parameter is a resource    */
/*  group.                                                                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetIntersectGroup(KHE_RESOURCE_SET rs1, KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetIntersect(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetIntersectCount(KHE_RESOURCE_SET rs1,                   */
/*    KHE_RESOURCE_SET rs2)                                                  */
/*                                                                           */
/*  Return the number of elements in the intersection of rs1 with rs2.       */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetIntersectCount(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetIntersectCount(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetIntersectCountGroup(KHE_RESOURCE_SET rs1,              */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetIntersectCount, only the second parameter is a        */
/*  resource group.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetIntersectCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetIntersectCount(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "difference operations"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetDifference(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)*/
/*                                                                           */
/*  Update rs1 to contain the difference of its initial value with rs2,      */
/*  that is, rs1 - rs2.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetDifference(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  KheSetDifference(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetDifferenceGroup(KHE_RESOURCE_SET rs1,                 */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetDifference, only the second parameter is a resource   */
/*  group.                                                                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetDifferenceGroup(KHE_RESOURCE_SET rs1, KHE_RESOURCE_GROUP rg2)
{
  KheResourceSetDifference(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetDifferenceCount(KHE_RESOURCE_SET rs1,                  */
/*    KHE_RESOURCE_SET rs2)                                                  */
/*                                                                           */
/*  Return the number of elements in the difference of rs1 with rs2.         */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetDifferenceCount(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetDifferenceCount(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetDifferenceCountGroup(KHE_RESOURCE_SET rs1,             */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetDifferenceCount, only the second parameter is a       */
/*  resource group.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetDifferenceCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetDifferenceCount(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "symmetric difference operations"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetSymmetricDifferenceCount(KHE_RESOURCE_SET rs1,         */
/*    KHE_RESOURCE_SET rs2)                                                  */
/*                                                                           */
/*  Return the number of elements in the symmetric difference of rs1 and rs2.*/
/*                                                                           */
/*****************************************************************************/

int KheResourceSetSymmetricDifferenceCount(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_SET rs2)
{
  return KheSetSymmetricDifferenceCount(rs1->resource_indexes,
    rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetSymmetricDifferenceCountGroup(KHE_RESOURCE_SET rs1,    */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetSymmetricDifferenceCount, only the second parameter   */
/*  is a resource group.                                                     */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetSymmetricDifferenceCountGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetSymmetricDifferenceCount(rs1,
    KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "equality operations"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetEqual(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)     */
/*                                                                           */
/*  Return true if rs1 and rs2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetEqual(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetEqual(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetEqualGroup(KHE_RESOURCE_SET rs1,                      */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetEqual, only the second parameter is a resource group. */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetEqualGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetEqual(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "subset operations"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetSubset(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)    */
/*                                                                           */
/*  Return true if rs1 is a subset of rs2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetSubset(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetSubset(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetSubsetGroup(KHE_RESOURCE_SET rs1,                     */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetSubset, only the second parameter is a resource group.*/
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetSubsetGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetSubset(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "disjoint operations"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetDisjoint(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)  */
/*                                                                           */
/*  Return true if rs1 and rs2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetDisjoint(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetDisjoint(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetDisjointGroup(KHE_RESOURCE_SET rs1,                   */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Like KheResourceSetDisjoint, only the second parameter is a resource     */
/*  group.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetDisjointGroup(KHE_RESOURCE_SET rs1,
  KHE_RESOURCE_GROUP rg2)
{
  return KheResourceSetDisjoint(rs1, KheResourceGroupResourceSet(rg2));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "other operations"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceSetContainsResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r) */
/*                                                                           */
/*  Return true if rs contains r.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheResourceSetContainsResource(KHE_RESOURCE_SET rs, KHE_RESOURCE r)
{
  return KheSetContains(rs->resource_indexes, KheGetResourceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Comparison function for sorting resource sets.                           */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE_SET rs1 = * (KHE_RESOURCE_SET *) t1;
  KHE_RESOURCE_SET rs2 = * (KHE_RESOURCE_SET *) t2;
  return KheResourceSetTypedCmp(rs1, rs2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceSetTypedCmp(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)   */
/*                                                                           */
/*  Typed version of comparison function.                                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceSetTypedCmp(KHE_RESOURCE_SET rs1, KHE_RESOURCE_SET rs2)
{
  return KheSetTypedCmp(rs1->resource_indexes, rs2->resource_indexes);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceSetDebug(KHE_RESOURCE_SET rs, int verbosity, int indent, */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Produce a debug print of rs on fp with the given verbosity and indent.   */
/*                                                                           */
/*****************************************************************************/

void KheResourceSetDebug(KHE_RESOURCE_SET rs, int verbosity, int indent,
  FILE *fp)
{
  KHE_RESOURCE r, r2;  int count, i;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  if( verbosity >= 2 && (indent >= 0 || KheResourceSetResourceCount(rs) < 8) )
  {
    /* full print */
    for( i = 0;  i < KheResourceSetResourceCount(rs);  i++ )
    {
      r = KheResourceSetResource(rs, i);
      if( i > 0 )
      {
	if( i % 8 == 0 )
	  fprintf(fp, ",\n%*s", indent + 1, "");
	else
	  fprintf(fp, ", ");
      }
      fprintf(fp, "%s", r == NULL ? "NULL" : KheResourceId(r));
    }
  }
  else
  {
    /* abbreviated print */
    count = KheResourceSetResourceCount(rs);
    if( count == 0 )
    {
      /* print nothing */
    }
    else if( count == 1 )
    {
      /* print sole element */
      r = KheResourceSetResource(rs, 0);
      fprintf(fp, "%s", r == NULL ? "NULL" : KheResourceId(r));
    }
    else if( count == 2 )
    {
      /* print both elements */
      r = KheResourceSetResource(rs, 0);
      r2 = KheResourceSetResource(rs, 1);
      fprintf(fp, "%s, %s", r == NULL ? "NULL" : KheResourceId(r),
	KheResourceId(r2));
    }
    else
    {
      /* print first and last elements, with ellipsis */
      r = KheResourceSetResource(rs, 0);
      r2 = KheResourceSetResource(rs, count - 1);
      fprintf(fp, "%s, ... %s", r == NULL ? "NULL" : KheResourceId(r),
	KheResourceId(r2));
    }
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}

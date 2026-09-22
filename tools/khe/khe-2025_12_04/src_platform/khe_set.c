
/*****************************************************************************/
/*                                                                           */
/*  THE `KHE_SET' MODULE                                                     */
/*  COPYRIGHT (C) 2017 Jeffrey H. Kingston                                   */
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
/*  FILE:         khe_set.c                                                  */
/*  DESCRIPTION:  Sorted sets of integers                                    */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "khe_set.h"
#include "howard_n.h"
#define value(sp, i) HaArray((sp)->ss_array, (i))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  Submodule "helper functions" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplBinarySearch(KHE_SET *sp, int item, int *pos)              */
/*                                                                           */
/*  Carry out a binary search of sorted array sp->ss_items, looking for      */
/*  item.  If item is present, set *pos to its index and return true.        */
/*  If item is not present, set *pos to the index it would occupy if it      */
/*  was present, and return false.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheSetImplBinarySearch(KHE_SET *sp, int item, int *pos)
{
  int l, r, mid, mid_el;
  l = 0;
  r = HaArrayCount(sp->ss_array) - 1;
  while( l <= r )
  {
    /* invariant:  item is in ss_items[l..r] or nowhere */
    mid = (l + r) / 2;
    mid_el = HaArray(sp->ss_array, mid);
    if( item < mid_el )
      r = mid - 1;
    else if( item > mid_el )
      l = mid + 1;
    else
      return *pos = mid, true;
  }
  return *pos = l, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplCheck(KHE_SET *sp, char *str)                             */
/*                                                                           */
/*  Check sp; specifically, check that the values are distinct and           */
/*  increasing.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheSetImplCheck(KHE_SET *sp, char *str)
{
  if( DEBUG1 )
  {
    int i;
    for( i = 1;  i < HaArrayCount(sp->ss_array);  i++ )
      HnAssert(HaArray(sp->ss_array, i - 1) < HaArray(sp->ss_array, i),
        "KheSetImplCheck bad items in %s: %d: %d and %d: %d", str,
	i - 1, HaArray(sp->ss_array, i - 1), i, HaArray(sp->ss_array, i));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplInit(KHE_SET *sp)                                         */
/*                                                                           */
/*  Initialize *sp to the empty set.                                         */
/*                                                                           */
/*****************************************************************************/

void KheSetImplInit(KHE_SET *sp, HA_ARENA a)
{
  HaArrayInit(sp->ss_array, a);
  KheSetImplCheck(sp, "KheSetImplInit final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplCopy(KHE_SET *to_sp, KHE_SET *from_sp, HA_ARENA a)        */
/*                                                                           */
/*  Copy from_sp to to_sp.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheSetImplCopy(KHE_SET *to_sp, KHE_SET *from_sp, HA_ARENA a)
{
  int i;
  KheSetImplCheck(from_sp, "KheSetImplCopy from_sp initial value");
  HaArrayInit(to_sp->ss_array, a);
  HaArrayAppend(to_sp->ss_array, from_sp->ss_array, i);
  KheSetImplCheck(from_sp, "KheSetImplCopy from_sp final value");
  KheSetImplCheck(to_sp, "KheSetImplCopy to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplCopyElements(KHE_SET *to_sp, KHE_SET *from_sp)            */
/*                                                                           */
/*  Copy the elements of from_sp to to_sp.                                   */
/*                                                                           */
/*****************************************************************************/

void KheSetImplCopyElements(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int i;
  HaArrayClear(to_sp->ss_array);
  HaArrayAppend(to_sp->ss_array, from_sp->ss_array, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplClear(KHE_SET *sp)                                        */
/*                                                                           */
/*  Clear *sp back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheSetImplClear(KHE_SET *sp)
{
  KheSetImplCheck(sp, "KheSetImplClear initial value");
  HaArrayClear(sp->ss_array);
  KheSetImplCheck(sp, "KheSetImplClear final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplInsert(KHE_SET *sp, int item)                             */
/*                                                                           */
/*  Insert item into *sp if not already present, else do nothing.            */
/*                                                                           */
/*****************************************************************************/

void KheSetImplInsert(KHE_SET *sp, int item)
{
  int pos;
  KheSetImplCheck(sp, "KheSetImplInsert initial value");
  if( !KheSetImplBinarySearch(sp, item, &pos) )
    HaArrayAdd(sp->ss_array, pos, item);
  KheSetImplCheck(sp, "KheSetImplInsert final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplDelete(KHE_SET *sp, int item)                             */
/*                                                                           */
/*  Delete item from *sp if present, else do nothing.                        */
/*                                                                           */
/*****************************************************************************/

void KheSetImplDelete(KHE_SET *sp, int item)
{
  int pos;
  KheSetImplCheck(sp, "KheSetImplDelete initial value");
  if( KheSetImplBinarySearch(sp, item, &pos) )
    HaArrayDeleteAndShift(sp->ss_array, pos);
  KheSetImplCheck(sp, "KheSetImplDelete final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplUnion(KHE_SET *to_sp, KHE_SET *from_sp)                   */
/*                                                                           */
/*  Change *to_sp to contain the union of *to_sp with *from_sp.              */
/*                                                                           */
/*  Implementation note.  This function first removes duplicates from        */
/*  to_sp using KheSetImplDifference(to_sp, from_sp), and then carries out   */
/*  a disjoint merge of from_sp into to_sp.  A one-pass non-disjoint merge   */
/*  would be quite feasible if we knew how many items there will be in the   */
/*  result (value new_ss_csize below); but we don't.                         */
/*                                                                           */
/*  We optimize the case where either set is empty or the last item of       */
/*  one is smaller than the first item of the other.  In these cases         */
/*  the sets are already disjoint and we don't need a set difference.        */
/*                                                                           */
/*****************************************************************************/

void KheSetImplUnion(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int target_i, to_i, from_i, to_val, from_val;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp, "KheSetImplUnion from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplUnion to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplUnion(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));

  /* if to_sp and from_sp are the exact same object, do nothing */
  if( to_sp == from_sp )
    return;

  /* if the sets are not clearly disjoint, use KheSetDifference to make them */
  if( HaArrayCount(to_sp->ss_array) != 0 &&
      HaArrayCount(from_sp->ss_array) != 0 &&
      value(to_sp, 0) <= value(from_sp, HaArrayCount(from_sp->ss_array) - 1) &&
      value(from_sp, 0) <= value(to_sp, HaArrayCount(to_sp->ss_array) - 1) )
    KheSetImplDifference(to_sp, from_sp);

  /* carry out a disjoint merge of from_sp->ss_array into to_sp->ss_array */
  to_i = HaArrayCount(to_sp->ss_array) - 1;
  from_i = HaArrayCount(from_sp->ss_array) - 1;
  target_i = HaArrayCount(to_sp->ss_array) + HaArrayCount(from_sp->ss_array) -1;
  if( DEBUG3 && target_i > 10000 )
    fprintf(stderr, "  KheSetImplUnion target_i = %d + %d - 1\n",
      HaArrayCount(to_sp->ss_array), HaArrayCount(from_sp->ss_array));
  HaArrayFill(to_sp->ss_array, target_i + 1, 0);
  while( from_i >= 0 && to_i >= 0 )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( to_val > from_val )  /* to_val is next, so add it now */
    {
      HaArrayPut(to_sp->ss_array, target_i, to_val);
      target_i--;
      to_i--;
    }
    else /* must have from_val > to_val; from_val is next, so add it now */
    {
      HnAssert(from_val > to_val, "KheSetImplUnion internal error 1");
      HaArrayPut(to_sp->ss_array, target_i, from_val);
      target_i--;
      from_i--;
    }
  }
  while( from_i >= 0 )  /* from_val is next, so add it now */
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    HaArrayPut(to_sp->ss_array, target_i, from_val);
    target_i--;
    from_i--;
  }
  if( DEBUG2 )
    fprintf(stderr, "%s\n", KheSetImplShow(to_sp, buff1));
  KheSetImplCheck(from_sp, "KheSetImplUnion from_sp final value");
  KheSetImplCheck(to_sp, "KheSetImplUnion to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplIntersect(KHE_SET *to_sp, KHE_SET *from_sp)               */
/*                                                                           */
/*  Change *to_sp to contain the intersection of *to_sp with *from_sp.       */
/*                                                                           */
/*****************************************************************************/

void KheSetImplIntersect(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int target_i, to_i, from_i, to_val, from_val;
  KheSetImplCheck(from_sp, "KheSetImplIntersect from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplIntersect to_sp initial value");
  if( to_sp == from_sp )
    return;
  from_i = to_i = target_i = 0;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
          to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val ) /* from_val is not in intersection, so drop it */
      from_i++;
    else if( to_val < from_val ) /* to_val is not in intersection, so drop it */
      to_i++;
    else /* from_val == to_val and this value is in the intersection */
    {
      HaArrayPut(to_sp->ss_array, target_i, to_val);
      target_i++;
      to_i++;
      from_i++;
    }
  }
  HaArrayDeleteLastSlice(to_sp->ss_array,
    HaArrayCount(to_sp->ss_array) - target_i);
  KheSetImplCheck(from_sp, "KheSetImplIntersect from_sp final value");
  KheSetImplCheck(to_sp, "KheSetImplIntersect to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplDifference(KHE_SET *to_sp, KHE_SET *from_sp)              */
/*                                                                           */
/*  Change *to_sp to contain the difference of *to_sp with *from_sp.         */
/*                                                                           */
/*****************************************************************************/

void KheSetImplDifference(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int target_i, to_i, from_i, to_val, from_val;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp, "KheSetImplDifference from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplDifference to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplDifference(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));
  from_i = to_i = target_i = 0;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
         to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val )  /* from_val not in to_sp, so ignore it */
      from_i++;
    else if( to_val < from_val )  /* to_val not in from_sp, so include it */
    {
      HaArrayPut(to_sp->ss_array, target_i, to_val);
      target_i++;
      to_i++;
    }
    else /* to_val in from_sp, so don't include it */
      from_i++, to_i++;
  }
  while( to_i < HaArrayCount(to_sp->ss_array) )
  {
    /* to_val not in from_sp, so include it */
    to_val = HaArray(to_sp->ss_array, to_i);
    HaArrayPut(to_sp->ss_array, target_i, to_val);
    target_i++;
    to_i++;
  }
  HaArrayDeleteLastSlice(to_sp->ss_array,
    HaArrayCount(to_sp->ss_array) - target_i);
  KheSetImplCheck(from_sp, "KheSetImplDifference from_sp final value");
  KheSetImplCheck(to_sp, "KheSetImplDifference to_sp final value");
  if( DEBUG2 )
    fprintf(stderr, "%s\n", KheSetImplShow(to_sp, buff1));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "counts"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheSetImplUnionCount(KHE_SET *to_sp, KHE_SET *from_sp)               */
/*                                                                           */
/*  Return the number of elements in either *to_sp or *from_sp.              */
/*                                                                           */
/*****************************************************************************/

int KheSetImplUnionCount(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int to_i, from_i, to_val, from_val, res;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp, "KheSetImplUnionCount from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplUnionCount to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplUnionCount(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));
  from_i = to_i = res = 0;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
         to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val )
      res++, from_i++;
    else if( to_val < from_val )
      res++, to_i++;
    else
      res++, from_i++, to_i++;
  }
  res += HaArrayCount(to_sp->ss_array) - to_i;
  res += HaArrayCount(from_sp->ss_array) - from_i;
  if( DEBUG2 )
    fprintf(stderr, "%d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSetImplIntersectCount(KHE_SET *to_sp, KHE_SET *from_sp)           */
/*                                                                           */
/*  Return the number of elements in both *to_sp and *from_sp.               */
/*                                                                           */
/*  Implementation note.  This function begins by ensuring that              */
/*  from_sp is the smaller of the two sets.  It then finds the               */
/*  indexes of the first and last elements of from_sp from to_sp,            */
/*  and does the actual intersecting only over that range.  In               */
/*  this way it runs much more efficiently when intersecting                 */
/*  a small set with a large one.                                            */
/*                                                                           */
/*****************************************************************************/

int KheSetImplIntersectCount(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int to_i, from_i, to_val, from_val, res, first_pos, last_pos; KHE_SET *tmp_sp;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp, "KheSetImplIntersectCount from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplIntersectCount to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplIntersectCount(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));

  /* make from_sp into the smaller of the two sets */
  if( HaArrayCount(from_sp->ss_array) > HaArrayCount(to_sp->ss_array) )
    tmp_sp = to_sp, to_sp = from_sp, from_sp = tmp_sp;

  /* return if from_sp is empty (since can't search for first or last then) */
  if( HaArrayCount(from_sp->ss_array) == 0 )
    return false;

  /* find the range within to_sp that the elements of from_sp occupy */
  KheSetImplBinarySearch(to_sp, HaArrayFirst(from_sp->ss_array), &first_pos);
  KheSetImplBinarySearch(to_sp, HaArrayLast(from_sp->ss_array), &last_pos);
  if( last_pos >= HaArrayCount(to_sp->ss_array) ) last_pos--;

  /* now do a conventional intersection search, but only over the range */
  from_i = res = 0, to_i = first_pos;
  while( from_i < HaArrayCount(from_sp->ss_array) && to_i <= last_pos )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val )
      from_i++;
    else if( to_val < from_val )
      to_i++;
    else
      res++, from_i++, to_i++;
  }
  if( DEBUG2 )
    fprintf(stderr, "%d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSetImplDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp)          */
/*                                                                           */
/*  Return the number of elements in the difference of *to_sp and *from_sp;  */
/*  that is, the number of elements in *to_sp that are not in *from_sp.      */
/*                                                                           */
/*****************************************************************************/

int KheSetImplDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int to_i, from_i, to_val, from_val, res;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp, "KheSetImplDifferenceCount from_sp initial value");
  KheSetImplCheck(to_sp, "KheSetImplDifferenceCount to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplDifferenceCount(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));
  from_i = to_i = res = 0;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
         to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val )
      from_i++;
    else if( to_val < from_val )
      res++, to_i++;
    else
      from_i++, to_i++;
  }
  res += HaArrayCount(to_sp->ss_array) - to_i;
  if( DEBUG2 )
    fprintf(stderr, "%d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSetImplSymmetricDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp) */
/*                                                                           */
/*  Return the number of elements in the symmetric difference of *to_sp      */
/*  with *from_sp; that is, the number of elements in one set but not both.  */
/*                                                                           */
/*****************************************************************************/

int KheSetImplSymmetricDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp)
{
  int to_i, from_i, to_val, from_val, res;
  char buff1[200], buff2[200];
  KheSetImplCheck(from_sp,
    "KheSetImplSymmetricDifferenceCount from_sp initial value");
  KheSetImplCheck(to_sp,
    "KheSetImplSymmetricDifferenceCount to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "KheSetImplSymmetricDifferenceCount(%s, %s) = ",
      KheSetImplShow(to_sp, buff1), KheSetImplShow(from_sp, buff2));
  from_i = to_i = res = 0;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
         to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i);
    to_val = HaArray(to_sp->ss_array, to_i);
    if( from_val < to_val )
      res++, from_i++;
    else if( to_val < from_val )
      res++, to_i++;
    else
      from_i++, to_i++;
  }
  res += HaArrayCount(to_sp->ss_array) - to_i;
  res += HaArrayCount(from_sp->ss_array) - from_i;
  if( DEBUG2 )
    fprintf(stderr, "%d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "query"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplEqual(KHE_SET *sp1, KHE_SET *sp2)                         */
/*                                                                           */
/*  Return true if *sp1 and *sp2 are equal (contain the same items).  This   */
/*  function may assume that KheSetCount(*sp1) == KheSetCount(*sp2).         */
/*                                                                           */
/*****************************************************************************/

bool KheSetImplEqual(KHE_SET *sp1, KHE_SET *sp2)
{
  int i;
  for( i = 0;  i < HaArrayCount(sp1->ss_array);  i++ )
    if( value(sp1, i) != value(sp2, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplSubset(KHE_SET *sp1, KHE_SET *sp2)                        */
/*                                                                           */
/*  Return true if *sp1 is a subset of *sp2.  This function may assume that  */
/*  KheSetCount(*sp1) <= KheSetCount(*sp2).                                  */
/*                                                                           */
/*****************************************************************************/

bool KheSetImplSubset(KHE_SET *sp1, KHE_SET *sp2)
{
  int i1, i2;  int val1, val2;
  i1 = i2 = 0;
  while( i1 < HaArrayCount(sp1->ss_array) && i2 < HaArrayCount(sp2->ss_array) )
  {
    val1 = value(sp1, i1);
    val2 = value(sp2, i2);
    if( val1 < val2 ) /* val1 is not in sp2, so not a subset */
      return false;
    else if( val1 > val2 ) /* val2 is not in sp1, which says nothing useful */
      i2++;
    else /* val1 and val2 are in both */
      i1++, i2++;
  }
  if( i1 < HaArrayCount(sp1->ss_array) )  /* not in sp2, so not a subset */
    return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplDisjoint(KHE_SET *sp1, KHE_SET *sp2)                      */
/*                                                                           */
/*  Return true if *sp1 and *sp2 are disjoint.                               */
/*                                                                           */
/*****************************************************************************/

bool KheSetImplDisjoint(KHE_SET *sp1, KHE_SET *sp2)
{
  int i1, i2;  int val1, val2;
  i1 = i2 = 0;
  while( i1 < HaArrayCount(sp1->ss_array) && i2 < HaArrayCount(sp2->ss_array) )
  {
    val1 = value(sp1, i1);
    val2 = value(sp2, i2);
    if( val1 < val2 )  /* val1 is not in sp2, which is fine */
      i1++;
    else if( val1 > val2 ) /* val2 is not in sp1, which is also fine */
      i2++;
    else /* val1 and val2 are in both, so they are not disjoint */
      return false;
  }
  return true;  /* either can have leftovers but not both, so fine */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSetImplTypedCmp(KHE_SET *sp1, KHE_SET *sp2)                       */
/*                                                                           */
/*  Typed comparison function for sorting an array of ssets.                 */
/*                                                                           */
/*****************************************************************************/

int KheSetImplTypedCmp(KHE_SET *sp1, KHE_SET *sp2)
{
  int i;  int val1, val2;
  if( HaArrayCount(sp1->ss_array) != HaArrayCount(sp2->ss_array) )
    return HaArrayCount(sp1->ss_array) - HaArrayCount(sp2->ss_array);
  for( i = 0;  i < HaArrayCount(sp1->ss_array);  i++ )
  {
    val1 = value(sp1, i);
    val2 = value(sp2, i);
    if( val1 != val2 )
      return val1 - val2;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplContains(KHE_SET *sp, int item)                           */
/*                                                                           */
/*  Return true if *sp contains item.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheSetImplContains(KHE_SET *sp, int item)
{
  int pos;
  return KheSetImplBinarySearch(sp, item, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheSetImplShow(KHE_SET *sp)                                        */
/*                                                                           */
/*  Return a string representation of ss in static memory.                   */
/*                                                                           */
/*****************************************************************************/

char *KheSetImplShow(KHE_SET *sp, char buff[200])
{
  char result_so_far[170], one_range[20], tmp[200];  int i, j, k;
  snprintf(result_so_far, 170, "{");
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i = j )
  {
    /* find one range of groupable elements sp->ss_items[i .. j-1] */
    for( j = i + 1;  j < HaArrayCount(sp->ss_array);  j++ )
      if( HaArray(sp->ss_array, j) != HaArray(sp->ss_array, j-1) + 1 )
	break;

    /* print the range */
    if( j - 1 == i )
      snprintf(one_range, 20, "%u", HaArray(sp->ss_array, i));
    else
      snprintf(one_range, 20, "%u-%u", HaArray(sp->ss_array, i),
	HaArray(sp->ss_array, j-1));

    /* update result_so_far to contain the range */
    snprintf(tmp, 200, "%s%s%s", result_so_far, i > 0 ? ", " : "", one_range);
    for( k = 0;  k < 169 && tmp[k] != '\0';  k++ )
      result_so_far[k] = tmp[k];
    result_so_far[k] = '\0';
  }

  /* print the closing brace and return */
  snprintf(buff, 200, "%s%s", result_so_far, "}");
  return buff;
}


/* *** this code is perfectly safe, but the C compiler can't see that
#define do_add(str) (strcpy(&buff[bpos], str), bpos += strlen(str))

char *KheSetImplShow(KHE_SET *sp)
{
  static char buffers[4][200];  static int curr_buffer = 0;
  char *buff, tmp[20];  int i, j, bpos;
  curr_buffer = (curr_buffer + 1) % 4;
  buff = buffers[curr_buffer];
  bpos = 0;
  do_add("{");
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i = j )
  {
    ** if running out of space, abbreviate and exit **
    if( bpos > 180 )
    {
      do_add(", ...");
      break;
    }

    ** find range of groupable elements sp->ss_items[i .. j-1] **
    for( j = i + 1;  j < HaArrayCount(sp->ss_array);  j++ )
      if( HaArray(sp->ss_array, j) != HaArray(sp->ss_array, j-1) + 1 )
	break;

    ** print the range, optionally preceded by a command and space **
    if( i > 0 )
      do_add(", ");
    if( j - 1 == i )
      sprintf(tmp, "%u", HaArray(sp->ss_array, i));
    else
      sprintf(tmp, "%u-%u", HaArray(sp->ss_array, i),
	HaArray(sp->ss_array, j-1));
    do_add(tmp);
  }
  do_add("}");
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSetBuild(KHE_SET *sp, HA_ARENA a, int count, ...)                */
/*                                                                           */
/*  Initialize *sp to an KheSet containing count elements, taken from the    */
/*  following arguments of KheSetBuild.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheSetBuild(KHE_SET *sp, HA_ARENA a, int count, ...)
{
  va_list ap;  int i, item;
  KheSetInit(*sp, a);
  va_start(ap, count);
  for( i = 0;  i < count;  i++ )
  {
    item = va_arg(ap, int);
    KheSetInsert(*sp, item);
  }
  va_end(ap);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetTest(FILE *fp)                                                */
/*                                                                           */
/*  Test this module, sending results to fp.                                 */
/*                                                                           */
/*****************************************************************************/

void KheSetTest(FILE *fp)
{
  KHE_SET ss[5];  int i, j;  HA_ARENA a;  HA_ARENA_SET as;
  char buff1[200], buff2[200];
  fprintf(fp, "[ KheSetTest\n");

  /* initialize ss[0] */
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  KheSetBuild(&ss[0], a, 5, 2, 4, 6, 3, 1);
  fprintf(fp, "  ss[0] = %s\n", KheSetShow(ss[0], buff1));

  /* initialize ss[1] */
  KheSetBuild(&ss[1], a, 4, 8, 6, 4, 2);
  fprintf(fp, "  ss[1] = %s\n", KheSetShow(ss[1], buff1));

  /* initialize ss[2] */
  KheSetInit(ss[2], a);
  fprintf(fp, "  ss[2] = %s\n", KheSetShow(ss[2], buff1));

  /* initialize ss[3] */
  KheSetBuild(&ss[3], a, 1, 3);
  fprintf(fp, "  ss[3] = %s\n", KheSetShow(ss[3], buff1));

  /* initialize ss[4] */
  KheSetBuild(&ss[4], a, 5, 2, 4, 6, 3, 1);
  fprintf(fp, "  ss[4] = %s\n", KheSetShow(ss[4], buff1));

  /* testing count and the others */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  KheSetCount(%s) = %d\n", KheSetShow(ss[i], buff1),
      KheSetCount(ss[i]));
    if( KheSetCount(ss[i]) > 0 )
    {
      fprintf(fp, "  KheSetMin(%s) = %d\n", KheSetShow(ss[i], buff1),
	KheSetMin(ss[i]));
      fprintf(fp, "  KheSetMax(%s) = %d\n", KheSetShow(ss[i], buff1),
	KheSetMax(ss[i]));
    }
  }

  /* testing KheSetContains */
  fprintf(fp, "\n");
  for( i = 0;  i < 4;  i++ )
    for( j = 0;  j <= 8;  j += 2 )
      fprintf(fp, "  KheSetContains(%s, %d) = %s\n",
	KheSetShow(ss[i], buff1), j, bool_show(KheSetContains(ss[i], j)));

  /* testing KheSetEquals */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  KheSetEqual(%s, %s) = %s\n", KheSetShow(ss[i], buff1),
	KheSetShow(ss[j], buff2), bool_show(KheSetEqual(ss[i], ss[j])));

  /* testing KheSetSubset */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  KheSetSubset(%s, %s) = %s\n", KheSetShow(ss[i], buff1),
	KheSetShow(ss[j], buff2), bool_show(KheSetSubset(ss[i], ss[j])));

  /* testing KheSetDisjoint */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  KheSetDisjoint(%s, %s) = %s\n", KheSetShow(ss[i], buff1),
	KheSetShow(ss[j], buff2), bool_show(KheSetDisjoint(ss[i], ss[j])));

  /* testing KheSetUnion */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  KheSetUnion(%s, %s) = ", KheSetShow(ss[3], buff1),
      KheSetShow(ss[i], buff2));
    KheSetUnion(ss[3], ss[i]);
    fprintf(fp, "%s\n", KheSetShow(ss[3], buff1));
  }

  /* testing KheSetDifference */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )  if( i != 3 )
  {
    fprintf(fp, "  KheSetDifference(%s, %s) = ", KheSetShow(ss[3], buff1),
      KheSetShow(ss[i], buff2));
    KheSetDifference(ss[3], ss[i]);
    fprintf(fp, "%s\n", KheSetShow(ss[3], buff1));
  }

  /* testing KheSetIntersect */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  KheSetIntersect(%s, %s) = ", KheSetShow(ss[0], buff1),
      KheSetShow(ss[i], buff2));
    KheSetIntersect(ss[0], ss[i]);
    fprintf(fp, "%s\n", KheSetShow(ss[0], buff1));
  }

  fprintf(fp, "] KheSetTest\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tables indexed by KheSets"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SET_TABLE - a table of KheSets                                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_set_table_entry_rec {
  KHE_SET		*key;
  void			*value;
} KHE_SET_TABLE_ENTRY;

typedef HA_ARRAY(KHE_SET_TABLE) ARRAY_KHE_SET_TABLE;

struct khe_set_table_rec {
  KHE_SET_TABLE_ENTRY	entry;
  ARRAY_KHE_SET_TABLE	sub_tables;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_SET_TABLE KheSetTableMake(HA_ARENA a)                                */
/*                                                                           */
/*  Make a new, empty table.                                                 */
/*                                                                           */
/*****************************************************************************/

KHE_SET_TABLE KheSetTableMake(HA_ARENA a)
{
  KHE_SET_TABLE res;
  HaMake(res, a);
  res->entry.key = NULL;
  res->entry.value = NULL;
  HaArrayInit(res->sub_tables, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetImplTableInsert(KHE_SET_TABLE st, KHE_SET *sp, void *val)     */
/*                                                                           */
/*  Insert sp into st.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheSetImplTableInsert(KHE_SET_TABLE st, KHE_SET *sp, void *val)
{
  int i, key;  KHE_SET_TABLE st2;  char buff[200];
  HnAssert(val != NULL, "KheSetImplTableInsert:  NULL val");
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i++ )
  {
    key = (i == 0 ? HaArrayFirst(sp->ss_array) :
      HaArray(sp->ss_array, i) - HaArray(sp->ss_array, i-1));
    HaArrayFill(st->sub_tables, key + 1, NULL);
    st2 = HaArray(st->sub_tables, key);
    if( st2 == NULL )
    {
      st2 = KheSetTableMake(HaArrayArena(st->sub_tables));
      HaArrayPut(st->sub_tables, key, st2);
    }
    st = st2;
  }
  HnAssert(st->entry.key == NULL, "KheSetTableInsert: key %s already present",
    KheSetImplShow(sp, buff));
  st->entry.key = sp;
  st->entry.value = val;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSetImplTableRetrieve(KHE_SET_TABLE st, KHE_SET *sp, void **val)  */
/*                                                                           */
/*  Retrieve the value associated with sp from st.                           */
/*                                                                           */
/*****************************************************************************/

bool KheSetImplTableRetrieve(KHE_SET_TABLE st, KHE_SET *sp, void **val)
{
  int i, key;
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i++ )
  {
    key = (i == 0 ? HaArrayFirst(sp->ss_array) :
      HaArray(sp->ss_array, i) - HaArray(sp->ss_array, i-1));
    if( key >= HaArrayCount(st->sub_tables) )
      return *val = NULL, false;
    st = HaArray(st->sub_tables, key);
    if( st == NULL )
      return *val = NULL, false;
  }
  if( st->entry.key != NULL )
    return *val = st->entry.value, true;
  else
    return *val = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetTableDebug(KHE_SET_TABLE st, int indent, FILE *fp)            */
/*                                                                           */
/*  Debug print of st onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void KheSetTableDoDebug(KHE_SET_TABLE st, int encl_index, int indent, FILE *fp)
{
  KHE_SET_TABLE st2;  int i;  char buff[200];
  if( encl_index == -1 )
    fprintf(fp, "%*s[", indent, "");
  else
    fprintf(fp, "%*s%d [", indent, "", encl_index);
  if( st->entry.key != NULL )
    fprintf(fp, " %s -> %p", KheSetImplShow(st->entry.key, buff),
      st->entry.value);
  if( HaArrayCount(st->sub_tables) == 0 )
    fprintf(fp, " ]\n");
  else
  {
    fprintf(fp, "\n");
    HaArrayForEach(st->sub_tables, st2, i)
      if( st2 != NULL )
	KheSetTableDoDebug(st2, i, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}

void KheSetTableDebug(KHE_SET_TABLE st, int indent, FILE *fp)
{
  KheSetTableDoDebug(st, -1, indent, fp);
}

/*****************************************************************************/
/*                                                                           */
/*  void KheSetDebugRetrieve(KHE_SET_TABLE st, KHE_SET *sp, FILE *fp)        */
/*                                                                           */
/*  Retrieve *sp from st with a debug print of how it went.                  */
/*                                                                           */
/*****************************************************************************/

static void KheSetDebugRetrieve(KHE_SET_TABLE st, KHE_SET *sp, FILE *fp)
{
  void *val;  char buff[200];
  fprintf(fp, "KheSetTableRetrieve(st, %s, &val) = ", KheSetShow(*sp, buff));
  if( KheSetTableRetrieve(st, *sp, &val) )
    fprintf(fp, "true, val = %p\n", val);
  else
    fprintf(fp, "false\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSetTableTest(FILE *fp)                                           */
/*                                                                           */
/*  Test KheSet tables.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheSetTableTest(FILE *fp)
{
  KHE_SET_TABLE st;  KHE_SET ss[5];  int i;  HA_ARENA a;  HA_ARENA_SET as;
  char buff[200];

  /* initialize */
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  st = KheSetTableMake(a);
  fprintf(fp, "\nafter initializing, table is:\n");
  KheSetTableDebug(st, 0, fp);

  /* first insertion */
  KheSetBuild(&ss[0], a, 4, 1, 2, 3, 5);
  KheSetTableInsert(st, ss[0], (void *) 0x1);
  fprintf(fp, "\nafter inserting %s, table is:\n", KheSetShow(ss[0], buff));
  KheSetTableDebug(st, 0, fp);

  /* second insertion */
  KheSetBuild(&ss[1], a, 3, 1, 2, 3);
  KheSetTableInsert(st, ss[1], (void *) 0x2);
  fprintf(fp, "\nafter inserting %s, table is:\n", KheSetShow(ss[1], buff));
  KheSetTableDebug(st, 0, fp);

  /* third insertion */
  KheSetBuild(&ss[2], a, 3, 1, 3, 5);
  KheSetTableInsert(st, ss[2], (void *) 0x3);
  fprintf(fp, "\nafter inserting %s, table is:\n", KheSetShow(ss[2], buff));
  KheSetTableDebug(st, 0, fp);

  /* two more, not in the table */
  KheSetBuild(&ss[3], a, 3, 1, 2, 5);
  KheSetBuild(&ss[4], a, 0);

  /* retrievals */
  for( i = 0;  i < 5;  i++ )
    KheSetDebugRetrieve(st, &ss[i], fp);
}

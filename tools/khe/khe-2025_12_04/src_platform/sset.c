
/*****************************************************************************/
/*                                                                           */
/*  THE `SSET' SMALL-SETS MODULE                                             */
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
/*  FILE:         sset.c                                                     */
/*  DESCRIPTION:  Small sets with operations needed for sets of times        */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sset.h"
#include "howard_n.h"
#define value(sp, i) (HaArray((sp)->ss_array, (i)) + (sp)->ss_shift)
#define max(a, b) ((a) > (b) ? (a) : (b))

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Submodule "helper functions" (private)                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool SSetImplBinarySearch(SSET *sp, int item, int *pos)                  */
/*                                                                           */
/*  Carry out a binary search of sorted array sp->ss_items, looking for      */
/*  item (adjusted for the shift).  If item is present, set *pos to its      */
/*  index and return true.  If item is not present, set *pos to the index    */
/*  it would occupy if it was present, and return false.                     */
/*                                                                           */
/*****************************************************************************/

static bool SSetImplBinarySearch(SSET *sp, int item, int *pos)
{
  int l, r, mid;
  item -= sp->ss_shift;
  l = 0;
  r = HaArrayCount(sp->ss_array) - 1;
  while( l <= r )
  {
    /* invariant:  item is in ss_items[l..r] or nowhere */
    mid = (l + r) / 2;
    if( item < HaArray(sp->ss_array, mid) )
      r = mid - 1;
    else if( item > HaArray(sp->ss_array, mid) )
      l = mid + 1;
    else
      return *pos = mid, true;
  }
  return *pos = l, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplCheckNotFinalized(SSET *sp, char *fun_name,                 */
/*    char *param_name)                                                      */
/*                                                                           */
/*  Make sure that *sp is not finalized, by aborting with an error message   */
/*  if it is.                                                                */
/*                                                                           */
/*****************************************************************************/

static void SSetImplCheckNotFinalized(SSET *sp, char *fun_name,
  char *param_name)
{
  if( sp->ss_finalized )
  {
    fprintf(stderr, "%s: %s is finalized\n", fun_name, param_name);
    abort();
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplCheckFinalized(SSET *sp, char *fun_name, char *param_name)  */
/*                                                                           */
/*  Make sure that *sp is finalized, by aborting with an error message if    */
/*  it isn't.                                                                */
/*                                                                           */
/*****************************************************************************/

static void SSetImplCheckFinalized(SSET *sp, char *fun_name, char *param_name)
{
  if( !sp->ss_finalized )
  {
    fprintf(stderr, "%s: %s is not finalized\n", fun_name, param_name);
    abort();
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplEnlarge(SSET *sp, int new_size)                             */
/*                                                                           */
/*  Ensure that sp can hold at least new_size elements.                      */
/*                                                                           */
/*****************************************************************************/

/* *** maybe not; could use Fill, however
static void SSetImplEnlarge(SSET *sp, int new_size)
{
  if( sp->ss_msize < new_size )
  {
    sp->ss_msize = new_size;
    sp->ss_items = reall oc(sp->ss_items, new_size * sizeof(int));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void SSetImplCheck(SSET *sp, char *str)                                  */
/*                                                                           */
/*  Check sp; specifically, check that the values are distinct and           */
/*  increasing.                                                              */
/*                                                                           */
/*****************************************************************************/

static void SSetImplCheck(SSET *sp, char *str)
{
  if( DEBUG1 )
  {
    int i;
    for( i = 1;  i < HaArrayCount(sp->ss_array);  i++ )
      HnAssert(HaArray(sp->ss_array, i - 1) < HaArray(sp->ss_array, i),
        "SSetImplCheck bad items in %s: %d: %d and %d: %d", str,
	i - 1, HaArray(sp->ss_array, i - 1), i, HaArray(sp->ss_array, i));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplInit(SSET *sp)                                              */
/*                                                                           */
/*  Initialize *sp to the empty set.                                         */
/*                                                                           */
/*****************************************************************************/

void SSetImplInit(SSET *sp, HA_ARENA a)
{
  HaArrayInit(sp->ss_array, a);
  /* sp->ss_msize = sp->ss_csize = 0; */
  /* sp->ss_items = NULL; */
  sp->ss_shift = 0;
  /* sp->ss_slice = */ sp->ss_finalized = false;
  SSetImplCheck(sp, "SSetImplInit final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplCopy(SSET *to_sp, SSET *from_sp, HA_ARENA a)                */
/*                                                                           */
/*  Copy from_sp to to_sp.                                                   */
/*                                                                           */
/*****************************************************************************/

void SSetImplCopy(SSET *to_sp, SSET *from_sp, HA_ARENA a)
{
  int i;
  SSetImplCheck(from_sp, "SSetImplCopy from_sp initial value");
  HaArrayInit(to_sp->ss_array, a);
  HaArrayAppend(to_sp->ss_array, from_sp->ss_array, i);
  to_sp->ss_shift = from_sp->ss_shift;
  /* to_sp->ss_slice = false; */
  to_sp->ss_finalized = true;
  /* to_sp->ss_finalized = false; bug fix JeffK 14/11/25 */
  SSetImplCheck(from_sp, "SSetImplCopy from_sp final value");
  SSetImplCheck(to_sp, "SSetImplCopy to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplClear(SSET *sp)                                             */
/*                                                                           */
/*  Clear *sp back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

void SSetImplClear(SSET *sp)
{
  SSetImplCheckNotFinalized(sp, "SSetClear", "ss");
  SSetImplCheck(sp, "SSetImplClear initial value");
  HaArrayClear(sp->ss_array);
  SSetImplCheck(sp, "SSetImplClear final value");
  /* sp->ss_csize = 0; */
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplInsert(SSET *sp, int item)                                  */
/*                                                                           */
/*  Insert item into *sp if not already present, else do nothing.            */
/*                                                                           */
/*****************************************************************************/

void SSetImplInsert(SSET *sp, int item)
{
  int pos;
  SSetImplCheckNotFinalized(sp, "SSetInsert", "ss");
  SSetImplCheck(sp, "SSetImplInsert initial value");
  if( !SSetImplBinarySearch(sp, item, &pos) )
    HaArrayAdd(sp->ss_array, pos, item);
  SSetImplCheck(sp, "SSetImplInsert final value");
  /* ***
  {
    if( sp->ss_csize == sp->ss_msize )
      SSetImplEnlarge(sp, 2 * sp->ss_msize + 5);
    for( j = sp->ss_csize;  j > pos;  j-- )
      sp->ss_items[j] = sp->ss_items[j-1];
    sp->ss_items[j] = item;
    sp->ss_csize++;
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplDelete(SSET *sp, int item)                                  */
/*                                                                           */
/*  Delete item from *sp if present, else do nothing.                        */
/*                                                                           */
/*****************************************************************************/

void SSetImplDelete(SSET *sp, int item)
{
  int pos;
  SSetImplCheckNotFinalized(sp, "SSetDelete", "ss");
  SSetImplCheck(sp, "SSetImplDelete initial value");
  if( SSetImplBinarySearch(sp, item, &pos) )
    HaArrayDeleteAndShift(sp->ss_array, pos);
  SSetImplCheck(sp, "SSetImplDelete final value");
  /* ***
  {
    sp->ss_csize--;
    for( j = pos;  j < sp->ss_csize;  j++ )
      sp->ss_items[j] = sp->ss_items[j+1];
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplUnion(SSET *to_sp, SSET *from_sp)                           */
/*                                                                           */
/*  Change *to_sp to contain the union of *to_sp with *from_sp.              */
/*                                                                           */
/*  Implementation note.  This function first removes duplicates from        */
/*  to_sp using SSetImplDifference(to_sp, from_sp), and then carries out     */
/*  a disjoint merge of from_sp into to_sp.  A one-pass non-disjoint merge   */
/*  would be quite feasible if we knew how many items there will be in the   */
/*  result (value new_ss_csize below); but we don't.                         */
/*                                                                           */
/*  We optimize the case where either set is empty or the last item of       */
/*  one is smaller than the first item of the other.  In these cases         */
/*  the sets are already disjoint and we don't need a set difference.        */
/*                                                                           */
/*****************************************************************************/

void SSetImplUnion(SSET *to_sp, SSET *from_sp)
{
  int target_i, to_i, from_i, shift, to_val, from_val;
  SSetImplCheckNotFinalized(to_sp, "SSetUnion", "to_ss");
  SSetImplCheck(from_sp, "SSetImplUnion from_sp initial value");
  SSetImplCheck(to_sp, "SSetImplUnion to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "SSetImplUnion(%s, %s) = ",
      SSetImplShow(to_sp), SSetImplShow(from_sp));

  /* if to_sp and from_sp are the exact same object, do nothing */
  if( to_sp == from_sp )
    return;

  /* if the sets are not clearly disjoint, make them so using SSetDifference */
  if( HaArrayCount(to_sp->ss_array) != 0 &&
      HaArrayCount(from_sp->ss_array) != 0 &&
      value(to_sp, 0) <= value(from_sp, HaArrayCount(from_sp->ss_array) - 1) &&
      value(from_sp, 0) <= value(to_sp, HaArrayCount(to_sp->ss_array) - 1) )
    SSetImplDifference(to_sp, from_sp);

  /* make sure that to_sp has enough space to hold the disjoint merge */
  /* ***
  if( new_count > to_sp->ss_msize )
    SSetImplEnlarge(to_sp, max(new_ss_csize, 2 * to_sp->ss_msize + 5));
  *** */

  /* carry out a disjoint merge of from_sp->ss_array into to_sp->ss_array */
  to_i = HaArrayCount(to_sp->ss_array) - 1;
  from_i = HaArrayCount(from_sp->ss_array) - 1;
  target_i = HaArrayCount(to_sp->ss_array) + HaArrayCount(from_sp->ss_array) -1;
  HaArrayFill(to_sp->ss_array, target_i + 1, 0);
  shift = from_sp->ss_shift - to_sp->ss_shift;
  while( from_i >= 0 && to_i >= 0 )
  {
    from_val = HaArray(from_sp->ss_array, from_i) + shift;
    to_val = HaArray(to_sp->ss_array, to_i);
    if( to_val > from_val )  /* to_val is next, so add it now */
    {
      HaArrayPut(to_sp->ss_array, target_i, to_val);
      target_i--;
      to_i--;
    }
    else /* must have from_val > to_val; from_val is next, so add it now */
    {
      HnAssert(from_val > to_val, "SSetImplUnion internal error 1");
      HaArrayPut(to_sp->ss_array, target_i, from_val);
      target_i--;
      from_i--;
    }
  }
  while( from_i >= 0 )  /* from_val is next, so add it now */
  {
    from_val = HaArray(from_sp->ss_array, from_i) + shift;
    HaArrayPut(to_sp->ss_array, target_i, from_val);
    target_i--;
    from_i--;
  }
  /*  done by Fill to_sp->ss_csize = new_ss_csize; */
  if( DEBUG2 )
    fprintf(stderr, "%s\n", SSetImplShow(to_sp));
  SSetImplCheck(from_sp, "SSetImplUnion from_sp final value");
  SSetImplCheck(to_sp, "SSetImplUnion to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplIntersect(SSET *to_sp, SSET *from_sp)                       */
/*                                                                           */
/*  Change *to_sp to contain the intersection of *to_sp with *from_sp.       */
/*                                                                           */
/*****************************************************************************/

void SSetImplIntersect(SSET *to_sp, SSET *from_sp)
{
  int target_i, to_i, from_i, shift, to_val, from_val;
  SSetImplCheckNotFinalized(to_sp, "SSetIntersect", "to_ss");
  SSetImplCheck(from_sp, "SSetImplIntersect from_sp initial value");
  SSetImplCheck(to_sp, "SSetImplIntersect to_sp initial value");
  if( to_sp == from_sp )
    return;
  from_i = to_i = target_i = 0;
  shift = from_sp->ss_shift - to_sp->ss_shift;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
          to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i) + shift;
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
  /* to_sp->ss_csize = target_i; */
  SSetImplCheck(from_sp, "SSetImplIntersect from_sp final value");
  SSetImplCheck(to_sp, "SSetImplIntersect to_sp final value");
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplDifference(SSET *to_sp, SSET *from_sp)                      */
/*                                                                           */
/*  Change *to_sp to contain the difference of *to_sp with *from_sp.         */
/*                                                                           */
/*****************************************************************************/

void SSetImplDifference(SSET *to_sp, SSET *from_sp)
{
  int target_i, to_i, from_i, shift, to_val, from_val;
  SSetImplCheckNotFinalized(to_sp, "SSetDifference", "to_ss");
  SSetImplCheck(from_sp, "SSetImplDifference from_sp initial value");
  SSetImplCheck(to_sp, "SSetImplDifference to_sp initial value");
  if( DEBUG2 )
    fprintf(stderr, "SSetImplDifference(%s, %s) = ",
      SSetImplShow(to_sp), SSetImplShow(from_sp));
  from_i = to_i = target_i = 0;
  shift = from_sp->ss_shift - to_sp->ss_shift;
  while( from_i < HaArrayCount(from_sp->ss_array) &&
         to_i < HaArrayCount(to_sp->ss_array) )
  {
    from_val = HaArray(from_sp->ss_array, from_i) + shift;
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
  /* to_sp->ss_csize = target_i; */
  SSetImplCheck(from_sp, "SSetImplDifference from_sp final value");
  SSetImplCheck(to_sp, "SSetImplDifference to_sp final value");
  if( DEBUG2 )
    fprintf(stderr, "%s\n", SSetImplShow(to_sp));
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplFinalize(SSET *sp)                                          */
/*                                                                           */
/*  Finalize *sp; all subsequent attempts to change its value will abort.    */
/*                                                                           */
/*****************************************************************************/

void SSetImplFinalize(SSET *sp)
{
  SSetImplCheckNotFinalized(sp, "SSetFinalize", "ss");
  SSetImplCheck(sp, "SSetImplFinalize initial value");
  sp->ss_finalized = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplInitShifted(SSET *to_sp, SSET *from_sp, int shift)          */
/*                                                                           */
/*  Replace *to_sp with the set which has the same items as *from_sp,        */
/*  with shift (positive or negative) added to each value.                   */
/*                                                                           */
/*****************************************************************************/

void SSetImplInitShifted(SSET *to_sp, SSET *from_sp, int shift)
{
  SSetImplCheckFinalized(from_sp, "SSetInitShifted", "from_ss");
  to_sp->ss_array = from_sp->ss_array;
  to_sp->ss_array.length = 0;  /* why are we doing this? */
  to_sp->ss_shift = from_sp->ss_shift + shift;
  to_sp->ss_finalized = true;
  /* to_sp->ss_slice = true; */

  /* ***
  to_sp->ss_msize = 0;
  to_sp->ss_csize = from_sp->ss_csize;
  to_sp->ss_shift = from_sp->ss_shift + shift;
  to_sp->ss_finalized = true;
  to_sp->ss_slice = true;
  to_sp->ss_items = from_sp->ss_items;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplInitShiftedAndSliced(SSET *to_sp, SSET *from_sp, int shift, */
/*    int lower_lim, int upper_lim)                                          */
/*                                                                           */
/*  Replace *to_sp with the set which is *from_sp shifted by SSetImplShift,  */
/*  but then trim it as required at each end so that no value is less than   */
/*  lower_lim and no value is greater than upper_lim.                        */
/*                                                                           */
/*****************************************************************************/

void SSetImplInitShiftedAndSliced(SSET *to_sp, SSET *from_sp, int shift,
  int lower_lim, int upper_lim)
{
  int start_pos, stop_pos;
  SSetImplInitShifted(to_sp, from_sp, shift);
  SSetImplBinarySearch(to_sp, lower_lim, &start_pos);
  if( SSetImplBinarySearch(to_sp, upper_lim, &stop_pos) )
    stop_pos++;
  /* to_sp->ss_slice = true; */
  if( stop_pos > start_pos )
  {
    to_sp->ss_array.count = stop_pos - start_pos;
    to_sp->ss_array.items = &to_sp->ss_array.items[start_pos];
  }
  else
  {
    to_sp->ss_array.count = 0;
    to_sp->ss_array.items = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplFree(SSET *sp)                                              */
/*                                                                           */
/*  Free sp.  If *sp was created by a slice and shift operation, this        */
/*  does not free the items, otherwise it does free the items.  It does      */
/*  not free sp itself because *sp is always part of a larger record,        */
/*  which will be freed separately.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void SSetImplFree(SSET *sp)
{
  if( !sp->ss_slice )
    free(sp->ss_items);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "query"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool SSetImplEqual(SSET *sp1, SSET *sp2)                                 */
/*                                                                           */
/*  Return true if *sp1 and *sp2 are equal (contain the same items).  This   */
/*  function may assume that SSetCount(*sp1) == SSetCount(*sp2).             */
/*                                                                           */
/*****************************************************************************/

bool SSetImplEqual(SSET *sp1, SSET *sp2)
{
  int i;
  for( i = 0;  i < HaArrayCount(sp1->ss_array);  i++ )
    if( value(sp1, i) != value(sp2, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SSetImplSubset(SSET *sp1, SSET *sp2)                                */
/*                                                                           */
/*  Return true if *sp1 is a subset of *sp2.  This function may assume that  */
/*  SSetCount(*sp1) <= SSetCount(*sp2).                                      */
/*                                                                           */
/*****************************************************************************/

bool SSetImplSubset(SSET *sp1, SSET *sp2)
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
/*  bool SSetImplDisjoint(SSET *sp1, SSET *sp2)                              */
/*                                                                           */
/*  Return true if *sp1 and *sp2 are disjoint.                               */
/*                                                                           */
/*  This code has been carefully optimized, including for the common         */
/*  case where the last element of one set is smaller than the first         */
/*  element of the other.                                                    */
/*                                                                           */
/*****************************************************************************/

bool SSetImplDisjoint(SSET *sp1, SSET *sp2)
{
  int i1, i2, val1, val2, count1, count2;
  count1 = HaArrayCount(sp1->ss_array);
  count2 = HaArrayCount(sp2->ss_array);
  if( count1 == 0 || count2 == 0 )
    return true;
  else if( value(sp1, count1 - 1) < value(sp2, 0) )
    return true;
  else if( value(sp2, count2 - 1) < value(sp1, 0) )
    return true;
  else
  {
    i1 = i2 = 0;
    val1 = value(sp1, i1);
    val2 = value(sp2, i2);
    for( ; ; )
    {
      if( val1 < val2 )  /* val1 is not in sp2, which is fine */
      {
	if( ++i1 >= count1 )
	  return true;
	val1 = value(sp1, i1);
      }
      else if( val1 > val2 ) /* val2 is not in sp1, which is also fine */
      {
	if( ++i2 >= count2 )
	  return true;
	val2 = value(sp2, i2);
      }
      else /* val1 and val2 are equal, so the sets are not disjoint */
	return false;
    }
    return true;  /* actually unreachable */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int SSetImplTypedCmp(SSET *sp1, SSET *sp2)                               */
/*                                                                           */
/*  Typed comparison function for sorting an array of ssets.                 */
/*                                                                           */
/*****************************************************************************/

int SSetImplTypedCmp(SSET *sp1, SSET *sp2)
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
/*  bool SSetImplContains(SSET *sp, int item, int *pos)                      */
/*                                                                           */
/*  Return true if *sp contains item, and set *pos to its position.          */
/*                                                                           */
/*****************************************************************************/

bool SSetImplContains(SSET *sp, int item, int *pos)
{
  return SSetImplBinarySearch(sp, item, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *SSetImplShow(SSET *sp)                                             */
/*                                                                           */
/*  Return a string representation of ss in static memory.  This is stupid   */
/*  code, but it passes the C compiler's static array bound checks.          */
/*                                                                           */
/*  Loop invariant:  at the start of each iteration of the loop, tmp_buff    */
/*  is defined and contains everything we've done so far; buff is undefined. */
/*                                                                           */
/*****************************************************************************/

char *SSetImplShow(SSET *sp)
{
  static char buff[200], tmp_buff[170], tmp[20];  int i, j, k;
  snprintf(tmp_buff, 170, "{");
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i = j )
  {
    /* find range of groupable elements sp->ss_items[i .. j-1] */
    for( j = i + 1;  j < HaArrayCount(sp->ss_array);  j++ )
      if( HaArray(sp->ss_array, j) != HaArray(sp->ss_array, j-1) + 1 )
	break;

    /* print the range, optionally preceded by a comma and space */
    if( j - 1 == i )
      snprintf(tmp, 20, "%s%u", i > 0 ? ", " : "",
	HaArray(sp->ss_array, i) + sp->ss_shift);
    else
      snprintf(tmp, 20, "%s%u-%u", i > 0 ? ", " : "",
	HaArray(sp->ss_array, i) + sp->ss_shift,
	HaArray(sp->ss_array, j-1) + sp->ss_shift);
    snprintf(buff, 200, "%s%s", tmp_buff, tmp);
    for( k = 0;  k < 170;  k++ )
    {
      tmp_buff[k] = buff[k];
      if( tmp_buff[k] == '\0' )
	break;
    }
  }

  /* print the closing brace and return */
  snprintf(buff, 200, "%s%s", tmp_buff, sp->ss_finalized ? "}*" : "}");
  return buff;
}

/* *** this code is perfectly safe, but the C compiler can't see that
#define do_add(str) (strcpy(&buff[bpos], str), bpos += strlen(str))

char *SSetImplShow(SSET *sp)
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
      sprintf(tmp, "%u", HaArray(sp->ss_array, i) + sp->ss_shift);
    else
      sprintf(tmp, "%u-%u", HaArray(sp->ss_array, i) +sp->ss_shift,
	HaArray(sp->ss_array, j-1) + sp->ss_shift);
    do_add(tmp);
  }
  do_add(sp->ss_finalized ? "}*" : "}");
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SSetBuild(SSET *sp, HA_ARENA a, int count, ...)                     */
/*                                                                           */
/*  Initialize *sp to an SSet containing count elements, taken from the      */
/*  following arguments of SSetBuild.                                        */
/*                                                                           */
/*****************************************************************************/

static void SSetBuild(SSET *sp, HA_ARENA a, int count, ...)
{
  va_list ap;  int i, item;
  SSetInit(*sp, a);
  va_start(ap, count);
  for( i = 0;  i < count;  i++ )
  {
    item = va_arg(ap, int);
    SSetInsert(*sp, item);
  }
  va_end(ap);
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetTest(FILE *fp)                                                  */
/*                                                                           */
/*  Test this module, sending results to fp.                                 */
/*                                                                           */
/*****************************************************************************/

void SSetTest(FILE *fp)
{
  SSET ss[5];  int i, j, pos;  HA_ARENA a;  HA_ARENA_SET as;
  fprintf(fp, "[ SSetTest\n");

  /* initialize ss[0] */
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  SSetBuild(&ss[0], a, 5, 2, 4, 6, 3, 1);
  fprintf(fp, "  ss[0] = %s\n", SSetShow(ss[0]));

  /* initialize ss[1] */
  SSetBuild(&ss[1], a, 4, 8, 6, 4, 2);
  SSetFinalize(ss[1]);
  fprintf(fp, "  ss[1] = %s\n", SSetShow(ss[1]));

  /* initialize ss[2] */
  SSetInit(ss[2], a);
  fprintf(fp, "  ss[2] = %s\n", SSetShow(ss[2]));

  /* initialize ss[3] */
  SSetBuild(&ss[3], a, 1, 3);
  fprintf(fp, "  ss[3] = %s\n", SSetShow(ss[3]));

  /* initialize ss[4] */
  SSetBuild(&ss[4], a, 5, 2, 4, 6, 3, 1);
  fprintf(fp, "  ss[4] = %s\n", SSetShow(ss[4]));

  /* testing count and the others */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  SSetCount(%s) = %d\n", SSetShow(ss[i]), SSetCount(ss[i]));
    if( SSetCount(ss[i]) > 0 )
    {
      fprintf(fp, "  SSetMin(%s) = %d\n", SSetShow(ss[i]), SSetMin(ss[i]));
      fprintf(fp, "  SSetMax(%s) = %d\n", SSetShow(ss[i]), SSetMax(ss[i]));
    }
  }

  /* testing SSetContains */
  fprintf(fp, "\n");
  for( i = 0;  i < 4;  i++ )
    for( j = 0;  j <= 8;  j += 2 )
      fprintf(fp, "  SSetContains(%s, %d) = %s\n",
	SSetShow(ss[i]), j, SSetContains(ss[i], j, &pos) ? "true" : "false");

  /* testing SSetEquals */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  SSetEqual(%s, %s) = %s\n", SSetShow(ss[i]),
	SSetShow(ss[j]), SSetEqual(ss[i], ss[j]) ? "true" : "false");

  /* testing SSetSubset */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  SSetSubset(%s, %s) = %s\n", SSetShow(ss[i]),
	SSetShow(ss[j]), SSetSubset(ss[i], ss[j]) ? "true" : "false");

  /* testing SSetDisjoint */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
    for( j = 0;  j < 5;  j++ )
      fprintf(fp, "  SSetDisjoint(%s, %s) = %s\n", SSetShow(ss[i]),
	SSetShow(ss[j]), SSetDisjoint(ss[i], ss[j]) ? "true" : "false");

  /* testing SSetUnion */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  SSetUnion(%s, %s) = ", SSetShow(ss[3]), SSetShow(ss[i]));
    SSetUnion(ss[3], ss[i]);
    fprintf(fp, "%s\n", SSetShow(ss[3]));
  }

  /* testing SSetDifference */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )  if( i != 3 )
  {
    fprintf(fp, "  SSetDifference(%s, %s) = ", SSetShow(ss[3]),SSetShow(ss[i]));
    SSetDifference(ss[3], ss[i]);
    fprintf(fp, "%s\n", SSetShow(ss[3]));
  }

  /* testing SSetIntersect */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )
  {
    fprintf(fp, "  SSetIntersect(%s, %s) = ", SSetShow(ss[0]),SSetShow(ss[i]));
    SSetIntersect(ss[0], ss[i]);
    fprintf(fp, "%s\n", SSetShow(ss[0]));
  }

  /* testing SSetInitShiftedAndSliced */
  fprintf(fp, "\n");
  for( i = 0;  i < 5;  i++ )  if( i != 1 )
  {
    fprintf(fp, "  SSetInitShiftedAndSliced(-, %s, %d, %d, %d) = ",
      SSetShow(ss[1]), 1, 2, 5);
    SSetInitShiftedAndSliced(ss[i], ss[1], 1, 2, 5);
    fprintf(fp, "%s\n", SSetShow(ss[i]));
  }

  fprintf(fp, "] SSetTest\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tables indexed by SSets"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  SSET_TABLE - a table of SSets                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct sset_table_entry_rec {
  SSET			*key;
  void			*value;
} SSET_TABLE_ENTRY;

typedef HA_ARRAY(SSET_TABLE) ARRAY_SSET_TABLE;

struct sset_table_rec {
  SSET_TABLE_ENTRY	entry;
  ARRAY_SSET_TABLE	sub_tables;
};


/*****************************************************************************/
/*                                                                           */
/*  SSET_TABLE SSetTableMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Make a new, empty table.                                                 */
/*                                                                           */
/*****************************************************************************/

SSET_TABLE SSetTableMake(HA_ARENA a)
{
  SSET_TABLE res;
  HaMake(res, a);
  res->entry.key = NULL;
  res->entry.value = NULL;
  HaArrayInit(res->sub_tables, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetTableFree(SSET_TABLE st)                                        */
/*                                                                           */
/*  Free st.                                                                 */
/*                                                                           */
/*****************************************************************************/

/* as usual, we free by freeing the arena
void SSetTableFree(SSET_TABLE st)
{
  SSET_TABLE st2;  int i;
  MArrayForEach(st->sub_tables, &st2, &i)
    if( st2 != NULL )
      SSetTableFree(st2);
  MArrayFree(st->sub_tables);
  MFree(st);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SSetImplTableInsert(SSET_TABLE st, SSET *sp, void *val)             */
/*                                                                           */
/*  Insert sp into st.                                                       */
/*                                                                           */
/*****************************************************************************/

void SSetImplTableInsert(SSET_TABLE st, SSET *sp, void *val)
{
  int i, key;  SSET_TABLE st2;
  HnAssert(val != NULL, "SSetImplTableInsert:  NULL val");
  /* *** allowing this now
  MAssert(sp->ss_finalized, "SSetTableInsert: key %s not finalized",
    SSetImplShow(sp));
  *** */
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i++ )
  {
    key = (i == 0 ? HaArrayFirst(sp->ss_array) + sp->ss_shift :
      HaArray(sp->ss_array, i) - HaArray(sp->ss_array, i-1));
    HaArrayFill(st->sub_tables, key + 1, NULL);
    st2 = HaArray(st->sub_tables, key);
    if( st2 == NULL )
    {
      st2 = SSetTableMake(HaArrayArena(st->sub_tables));
      HaArrayPut(st->sub_tables, key, st2);
    }
    st = st2;
  }
  HnAssert(st->entry.key == NULL, "SSetTableInsert: key %s already present",
    SSetImplShow(sp));
  st->entry.key = sp;
  st->entry.value = val;
}


/*****************************************************************************/
/*                                                                           */
/*  bool SSetImplTableRetrieve(SSET_TABLE st, SSET *sp, void **val)          */
/*                                                                           */
/*  Retrieve the value associated with sp from st.                           */
/*                                                                           */
/*****************************************************************************/

bool SSetImplTableRetrieve(SSET_TABLE st, SSET *sp, void **val)
{
  int i, key;
  for( i = 0;  i < HaArrayCount(sp->ss_array);  i++ )
  {
    key = (i == 0 ? HaArrayFirst(sp->ss_array) + sp->ss_shift :
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
/*  void SSetTableDebug(SSET_TABLE st, int indent, FILE *fp)                 */
/*                                                                           */
/*  Debug print of st onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

void SSetTableDoDebug(SSET_TABLE st, int encl_index, int indent, FILE *fp)
{
  SSET_TABLE st2;  int i;
  if( encl_index == -1 )
    fprintf(fp, "%*s[", indent, "");
  else
    fprintf(fp, "%*s%d [", indent, "", encl_index);
  if( st->entry.key != NULL )
    fprintf(fp, " %s -> %p", SSetImplShow(st->entry.key), st->entry.value);
  if( HaArrayCount(st->sub_tables) == 0 )
    fprintf(fp, " ]\n");
  else
  {
    fprintf(fp, "\n");
    HaArrayForEach(st->sub_tables, st2, i)
      if( st2 != NULL )
	SSetTableDoDebug(st2, i, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}

void SSetTableDebug(SSET_TABLE st, int indent, FILE *fp)
{
  SSetTableDoDebug(st, -1, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetDebugRetrieve(SSET_TABLE st, SSET *sp, FILE *fp)                */
/*                                                                           */
/*  Retrieve *sp from st with a debug print of how it went.                  */
/*                                                                           */
/*****************************************************************************/

static void SSetDebugRetrieve(SSET_TABLE st, SSET *sp, FILE *fp)
{
  void *val;
  fprintf(fp, "SSetTableRetrieve(st, %s, &val) = ", SSetShow(*sp));
  if( SSetTableRetrieve(st, *sp, &val) )
    fprintf(fp, "true, val = %p\n", val);
  else
    fprintf(fp, "false\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void SSetTableTest(FILE *fp)                                             */
/*                                                                           */
/*  Test SSet tables.                                                        */
/*                                                                           */
/*****************************************************************************/

void SSetTableTest(FILE *fp)
{
  SSET_TABLE st;  SSET ss[5];  int i;  HA_ARENA a;  HA_ARENA_SET as;

  /* initialize */
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  st = SSetTableMake(a);
  fprintf(fp, "\nafter initializing, table is:\n");
  SSetTableDebug(st, 0, fp);

  /* first insertion */
  SSetBuild(&ss[0], a, 4, 1, 2, 3, 5);
  SSetFinalize(ss[0]);
  SSetTableInsert(st, ss[0], (void *) 0x1);
  fprintf(fp, "\nafter inserting %s, table is:\n", SSetShow(ss[0]));
  SSetTableDebug(st, 0, fp);

  /* second insertion */
  SSetBuild(&ss[1], a, 3, 1, 2, 3);
  SSetFinalize(ss[1]);
  SSetTableInsert(st, ss[1], (void *) 0x2);
  fprintf(fp, "\nafter inserting %s, table is:\n", SSetShow(ss[1]));
  SSetTableDebug(st, 0, fp);

  /* third insertion */
  SSetBuild(&ss[2], a, 3, 1, 3, 5);
  SSetFinalize(ss[2]);
  SSetTableInsert(st, ss[2], (void *) 0x3);
  fprintf(fp, "\nafter inserting %s, table is:\n", SSetShow(ss[2]));
  SSetTableDebug(st, 0, fp);

  /* two more, not in the table */
  SSetBuild(&ss[3], a, 3, 1, 2, 5);
  SSetFinalize(ss[3]);
  SSetBuild(&ss[4], a, 0);
  SSetFinalize(ss[4]);

  /* retrievals */
  for( i = 0;  i < 5;  i++ )
    SSetDebugRetrieve(st, &ss[i], fp);
}

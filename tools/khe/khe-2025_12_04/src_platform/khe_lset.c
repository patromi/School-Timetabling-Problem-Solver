
/*****************************************************************************/
/*                                                                           */
/*  THE KTS TIMETABLING SYSTEM                                               */
/*  COPYRIGHT (C) 2004, 2008 Jeffrey H. Kingston                             */
/*                                                                           */
/*  Jeffrey H. Kingston (jeff@it.usyd.edu.au)                                */
/*  School of Information Technologies                                       */
/*  The University of Sydney 2006                                            */
/*  AUSTRALIA                                                                */
/*                                                                           */
/*  FILE:         khe_lset.c                                                 */
/*  MODULE:       Sets represented as arbitrary length bit vectors           */
/*                                                                           */
/*****************************************************************************/
#include "khe_lset.h"
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "howard_a.h"

#define LSET_TRIE_BUCKET_LIMIT 50

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG8 0

#define INT_BIT (sizeof(unsigned int) * CHAR_BIT)

/*****************************************************************************/
/*                                                                           */
/*  LSET - a set of small non-negative integers                              */
/*                                                                           */
/*  An LSET is represented by a bit vector, preceded by a length field       */
/*  which says how many words the bit vector occupies.  Element i of         */
/*  lset s resides in s->elems[i/INT_BIT] at position i % INT_BIT,           */
/*  counting the least significant bit as position 0.  This is unaffected    */
/*  by whether the machine is big-endian or little-endian.                   */
/*                                                                           */
/*  Although the elements are expected to be small integers (typically       */
/*  up to 100 or so), there is no actual upper limit; the bit vectors        */
/*  are enlarged as required to hold the elements inserted into them.        */
/*  For this reason, LSetInsert and LSetUnion are passed the lsets           */
/*  which receive the result by reference.  This reference will be altered   */
/*  to a new lset if the existing lset is unable to hold the result.         */
/*  Care must be taken not to share lsets under these circumstances.         */
/*                                                                           */
/*  There is no guarantee that the last word will be non-zero.  In           */
/*  practice is usually will.  The length is always at least 1.  For         */
/*  example, the empty lset contains one word whose value is 0.              */
/*                                                                           */
/*****************************************************************************/

struct lset_rec {
  int		length;				/* number of words in elems  */
  unsigned int	elems[1];			/* actually length elems     */
};


/*****************************************************************************/
/*                                                                           */
/*  LSET LSetNew(HA_ARENA a)                                                 */
/*                                                                           */
/*  Return a new, empty LSET.                                                */
/*                                                                           */
/*****************************************************************************/

LSET LSetNew(HA_ARENA a)
{
  LSET res;
  HaMake(res, a);
  res = (LSET) malloc(sizeof(struct lset_rec));
  res->length = 1;
  res->elems[0] = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  LSET LSetCopy(LSET s)                                                    */
/*                                                                           */
/*  Return a copy of s.                                                      */
/*                                                                           */
/*****************************************************************************/

LSET LSetCopy(LSET s)
{
  LSET res;  int i;
  res = (LSET) malloc(sizeof(struct lset_rec) + (s->length - 1) * sizeof(int));
  res->length = s->length;
  for( i = 0;  i < s->length;  i++ )
    res->elems[i] = s->elems[i];
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetShift(LSET s, LSET *res, int k, int lim)                        */
/*                                                                           */
/*  Set *res to the set obtained by adding k (which could be negative) to    */
/*  every element of a, except that no element equal to or larger than       */
/*  lim is added to *res.                                                    */
/*                                                                           */
/*****************************************************************************/

void LSetShift(LSET s, LSET *res, int k, int lim)
{
  int word, word_base_pos, pos;  int new;
  LSetClear(*res);
  word_base_pos = 0;
  for( word = 0;  word < s->length;  word++ )
  {
    if( s->elems[word] )
      for( pos = 0;  pos < INT_BIT;  pos++ )
	if( (s->elems[word] & (1 << pos)) )
	{
	  new = word_base_pos + pos + k;
	  if( new >= 0 && new < lim )
	    LSetInsert(res, new);
	}
    word_base_pos += INT_BIT;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetClear(LSET s)                                                   */
/*                                                                           */
/*  Clear s.                                                                 */
/*                                                                           */
/*****************************************************************************/

void LSetClear(LSET s)
{
  int i;
  for( i = 0;  i < s->length;  i++ )
    s->elems[i] = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  LSET LSetEnlarge(LSET s, int len)                                        */
/*                                                                           */
/*  Copy s, returning a larger set utilizing len words, then free s.         */
/*                                                                           */
/*****************************************************************************/

static LSET LSetEnlarge(LSET s, int len)
{
  LSET res;  int i;
  res = (LSET) malloc(sizeof(struct lset_rec) + (len - 1) * sizeof(int));
  res->length = len;
  for( i = 0;  i < s->length;  i++ )
    res->elems[i] = s->elems[i];
  for( ; i < len;  i++ )
    res->elems[i] = 0;
  free(s);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetInsert(LSET *s, unsigned int i)                                 */
/*                                                                           */
/*  Add i to s, possibly reallocating s.  The element may already be         */
/*  present, that does not matter.                                           */
/*                                                                           */
/*****************************************************************************/

void LSetInsert(LSET *s, unsigned int i)
{
  int pos = i / INT_BIT;
  if( pos >= (*s)->length )
    *s = LSetEnlarge(*s, pos+1);
  (*s)->elems[pos] |= 1 << (i % INT_BIT);
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetDelete(LSET s, unsigned int i)                                  */
/*                                                                           */
/*  Delete i from s.  It may not be present, that does not matter.           */
/*                                                                           */
/*****************************************************************************/

void LSetDelete(LSET s, unsigned int i)
{
  s->elems[i/INT_BIT] &= ~(1 << (i % INT_BIT));
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetAssign(LSET *target, LSET source)                               */
/*                                                                           */
/*  Update target to be equal to source, possibly reallocating target.       */
/*                                                                           */
/*****************************************************************************/

void LSetAssign(LSET *target, LSET source)
{
  int i;
  if( (*target)->length < source->length )
    *target = LSetEnlarge(*target, source->length);
  for( i = 0;  i < source->length;  i++ )
    (*target)->elems[i] = source->elems[i];
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetUnion(LSET *target, LSET source)                                */
/*                                                                           */
/*  Update target to be the union of itself with source, possibly            */
/*  reallocating target.                                                     */
/*                                                                           */
/*****************************************************************************/

void LSetUnion(LSET *target, LSET source)
{
  int i;
  if( (*target)->length < source->length )
    *target = LSetEnlarge(*target, source->length);
  for( i = 0;  i < source->length;  i++ )
    (*target)->elems[i] |= source->elems[i];
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetIntersection(LSET target, LSET source)                          */
/*                                                                           */
/*  Update target to be the intersection of itself with source.              */
/*                                                                           */
/*****************************************************************************/

void LSetIntersection(LSET target, LSET source)
{
  int i;
  if( target->length <= source->length )
  {
    for( i = 0;  i < target->length;  i++ )
      target->elems[i] &= source->elems[i];
  }
  else
  {
    for( i = 0;  i < source->length;  i++ )
      target->elems[i] &= source->elems[i];
    for( ;  i < target->length;  i++ )
      target->elems[i] = 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetDifference(LSET target, LSET source)                            */
/*                                                                           */
/*  Update target to be the set difference target - source.                  */
/*                                                                           */
/*****************************************************************************/

void LSetDifference(LSET target, LSET source)
{
  int i;
  if( target->length <= source->length )
  {
    for( i = 0;  i < target->length;  i++ )
      target->elems[i] &= ~source->elems[i];
  }
  else
  {
    for( i = 0;  i < source->length;  i++ )
      target->elems[i] &= ~source->elems[i];
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetEmpty(LSET s)                                                   */
/*                                                                           */
/*  Return true if s is empty.                                               */
/*                                                                           */
/*****************************************************************************/

bool LSetEmpty(LSET s)
{
  int i;
  for( i = 0;  i < s->length;  i++ )
    if( s->elems[i] != 0 )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetEqual(LSET s1, LSET s2)                                         */
/*                                                                           */
/*  Return true if these two sets are equal.                                 */
/*                                                                           */
/*****************************************************************************/

bool LSetEqual(LSET s1, LSET s2)
{
  int i;
  if( s1->length <= s2->length )
  {
    for( i = 0;  i < s1->length;  i++ )
      if( s1->elems[i] != s2->elems[i] )
	return false;
    for( ;  i < s2->length;  i++ )
      if( s2->elems[i] != 0 )
	return false;
  }
  else
  {
    for( i = 0;  i < s2->length;  i++ )
      if( s2->elems[i] != s1->elems[i] )
	return false;
    for( ;  i < s1->length;  i++ )
      if( s1->elems[i] != 0 )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetSubset(LSET s1, LSET s2)                                        */
/*                                                                           */
/*  Return true if s1 is a subset of s2.                                     */
/*                                                                           */
/*****************************************************************************/

bool LSetSubset(LSET s1, LSET s2)
{
  int i;
  if( s1->length <= s2->length )
  {
    for( i = 0;  i < s1->length;  i++ )
      if( s1->elems[i] & ~s2->elems[i] )
	return false;
  }
  else
  {
    for( i = 0;  i < s2->length;  i++ )
      if( s1->elems[i] & ~s2->elems[i] )
	return false;
    for( ;  i < s1->length;  i++ )
      if( s1->elems[i] != 0 )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetDisjoint(LSET s1, LSET s2)                                      */
/*                                                                           */
/*  Return true if s1 and s2 are disjoint; that is, if they have an empty    */
/*  intersection.                                                            */
/*                                                                           */
/*****************************************************************************/

bool LSetDisjoint(LSET s1, LSET s2)
{
  int i, len;
  len = s1->length <= s2->length ? s1->length : s2->length;
  for( i = 0;  i < len;  i++ )
    if( s1->elems[i] & s2->elems[i] )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetDifferenceDisjoint(LSET s1a, LSET s1b, LSET s2)                 */
/*                                                                           */
/*  Return true if (s1a - s1b) and s2 are disjoint; that is, if there        */
/*  exists no element which lies in s1a and s2 but not in s1b.               */
/*                                                                           */
/*****************************************************************************/

/* ***
bool LSetDifferenceDisjoint(LSET s1a, LSET s1b, LSET s2)
{
  int i, len;
  len = s1a->length <= s2->length ? s1a->length : s2->length;
  if( len <= s1b->length )
  {
    for( i = 0;  i < len;  i++ )
      if( s1a->elems[i] & ~s1b->elems[i] & s2->elems[i] )
	return false;
  }
  else
  {
    for( i = 0;  i < s1b->length;  i++ )
      if( s1a->elems[i] & ~s1b->elems[i] & s2->elems[i] )
	return false;
    for( ;  i < len;  i++ )
      if( s1a->elems[i] & s2->elems[i] )
	return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool LSetIntersectionEqual(LSET s1a, LSET s1b, LSET s2)                  */
/*                                                                           */
/*  Return true if the intersection of s1a with s1b equals s2.               */
/*                                                                           */
/*****************************************************************************/

/* *** works but not currently used
bool LSetIntersectionEqual(LSET s1a, LSET s1b, LSET s2)
{
  int s1len, i;
  s1len = s1a->length <= s1b->length ? s1a->length : s1b->length;
  if( s1len < s2->length )
  {
    for( i = 0;  i < s1len;  i++ )
      if( (s1a->elems[i] & s1b->elems[i]) != s2->elems[i] )
	return false;
    for( ;  i < s2->length;  i++ )
      if( s2->elems[i] != 0 )
	return false;
  }
  else
  {
    for( i = 0;  i < s2->length;  i++ )
      if( (s1a->elems[i] & s1b->elems[i]) != s2->elems[i] )
	return false;
    for( ;  i < s1len;  i++ )
      if( (s1a->elems[i] & s1b->elems[i]) != 0 )
	return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool LSetContains(LSET s, unsigned int i)                                */
/*                                                                           */
/*  Return true if s contains i.                                             */
/*                                                                           */
/*****************************************************************************/

bool LSetContains(LSET s, unsigned int i)
{
  int pos = i / INT_BIT;
  return pos < s->length && (s->elems[pos] & (1 << (i % INT_BIT)));
}


/*****************************************************************************/
/*                                                                           */
/*  int LSetMin(LSET s)                                                      */
/*                                                                           */
/*  Return the minimum element of s, assuming s is non-empty.                */
/*                                                                           */
/*  Implementation note 1.  This function searches the words of s for the    */
/*  first non-zero word, then the bytes of that word for the first non-zero  */
/*  byte, and finally uses a table lookup to find the first non-zero bit     */
/*  of that byte.  Processors and languages should support this better!      */
/*                                                                           */
/*  Implementation note 2.  The table for finding the first non-zero bit     */
/*  of a byte was generated by a separate program I wrote (function          */
/*  LSetGenTables at the end of this file).  It can be initialized           */
/*  statically, and it is initialized statically, for the very good          */
/*  reason that doing it dynamically could go wrong with multi-threading.    */
/*                                                                           */
/*****************************************************************************/

unsigned int LSetMin(LSET s)
{
  int i, j, byte;
  static char first_nonzero_bit[1 << CHAR_BIT] = {
    8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
  for( i = 0;  i < s->length;  i++ )
    if( s->elems[i] != 0 )
    {
      for( j = 0;  j < INT_BIT;  j += CHAR_BIT )
      {
	byte = (s->elems[i] >> j) & ~(~0 << CHAR_BIT);
	if( byte != 0 )
	  return i * INT_BIT + j + first_nonzero_bit[byte];
      }
      assert(false);
    }
  assert(false);
  return 0;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  unsigned int LSetMax(LSET s)                                             */
/*                                                                           */
/*  Return the maximum element of s, assuming s is non-empty.                */
/*  This function is implemented similarly to LSetMax (q.v.).                */
/*                                                                           */
/*****************************************************************************/

unsigned int LSetMax(LSET s)
{
  int i, j, byte;
  static char last_nonzero_bit[1 << CHAR_BIT] = {
    8, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
  for( i = s->length - 1;  i >= 0;  i-- )
    if( s->elems[i] != 0 )
    {
      for( j = INT_BIT - CHAR_BIT;  j >= 0;  j -= CHAR_BIT )
      {
	byte = (s->elems[i] >> j) & ~(~0 << CHAR_BIT);
	if( byte != 0 )
	  return i * INT_BIT + j + last_nonzero_bit[byte];
      }
      assert(false);
    }
  assert(false);
  return 0;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  int LSetLexicalCmp(LSET s1, LSET s2)                                     */
/*                                                                           */
/*  Return an integer which is negative, zero, or positive accordingly       */
/*  as s1 is less than, equal to, or greater than s2, in lexicographical     */
/*  order.                                                                   */
/*                                                                           */
/*  Implementation note.  It would be more efficient to use unsigned         */
/*  word-length comparisons, but efficiency is not currently needed for      */
/*  this function and the idea is too frightening.                           */
/*                                                                           */
/*****************************************************************************/

int LSetLexicalCmp(LSET s1, LSET s2)
{
  int i, len;
  len = INT_BIT * (s1->length <= s2->length ? s2->length : s1->length);
  for( i = 0;  i < len;  i++ )
  {
    if( LSetContains(s1, i) )
    {
      if( !LSetContains(s2, i) )
	return 1;
    }
    else if( LSetContains(s2, i) )
      return -1;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  unsigned int LSetHash(LSET s)                                            */
/*                                                                           */
/*  Hash s.                                                                  */
/*                                                                           */
/*****************************************************************************/

unsigned int LSetHash(LSET s)
{
  unsigned int res, e1, e2, ix;
  static unsigned int flips1[4] = {0x3A56, 0x9C93, 0x65A3, 0x39C9};
  static unsigned int flips2[4] = {0x39A6, 0x5A93, 0x35C3, 0x39C9};
  size_t i = 0;
  res = 0;
  for( i = 0;  i < s->length;  i++ )
  {
    res <<= 1;
    e1 = s->elems[i] & 0xFFFF;
    e2 = s->elems[i] >> 16;
    ix = i % 4;
    res ^= (e1 ^ flips1[ix]) * (e2 ^ flips2[ix]);
  }
  return res;
  /* ***
  unsigned long res;
  size_t i = 0;
  res = 0;
  for( i = 0;  i < s->length;  i++ )
    res = (res ^ s->elems[i]);
  return (unsigned int) ((res % UINT_MAX) ^ (res / UINT_MAX));
  *** */
  /* ***
  res = 0;
  for( i = 1;  i < s->length;  i += 2 )
    res += ((unsigned long) s->elems[i-1] + i * 9557) *
           ((unsigned long) s->elems[i]   + i *  113);
  if( i - 1 < s->length )
    res += 23 * (unsigned long) s->elems[i-1];
  return (unsigned int) (res % UINT_MAX + res / UINT_MAX);
  *** */
  /* ***
  int i;  unsigned int res;
  res = 0;
  for( i = 0;  i < s->length;  i++ )
    res += s->elems[i] + i * 9577;
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  ARRAY_INT64 LSetToIntArray(LSET s)                                       */
/*                                                                           */
/*  Convert s to a static array of integers.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
ARRAY_INT64 LSetToIntArray(LSET s)
{
  int i;
  ARRAY_INT64 res = NULL;
  ArrayFresh(res);
  for( i = 0;  i < s->length * INT_BIT;  i++ )
    if( LSetContains(s, i) )
      ArrayAddLast(res, i);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void LSetSelect(LSET s, ARRAY_VOIDP *select_from, ARRAY_VOIDP *add_to)   */
/*                                                                           */
/*  Add to add_to each element of select_from whose index is in s.           */
/*                                                                           */
/*****************************************************************************/

/* ***
void LSetSelect(LSET s, ARRAY_VOIDP *select_from, ARRAY_VOIDP *add_to)
{
  int word, word_base_pos, pos;
  word_base_pos = 0;
  for( word = 0;  word < s->length;  word++ )
  {
    if( s->elems[word] )
      for( pos = 0;  pos < INT_BIT;  pos++ )
	if( s->elems[word] & (1 << pos) )
	  MArrayAddLast(*add_to, MArrayGet(*select_from, word_base_pos + pos));
    word_base_pos += INT_BIT;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void LSetExpand(LSET s, ARRAY_SHORT *add_to)                             */
/*                                                                           */
/*  Add to add_to each index in s.                                           */
/*                                                                           */
/*****************************************************************************/

void LSetExpand(LSET s, ARRAY_SHORT *add_to)
{
  short word, word_base_pos, pos;
  word_base_pos = 0;
  for( word = 0;  word < s->length;  word++ )
  {
    if( s->elems[word] )
      for( pos = 0;  pos < INT_BIT;  pos++ )
	if( s->elems[word] & (1 << pos) )
	  MArrayAddLast(*add_to, word_base_pos + pos);
    word_base_pos += INT_BIT;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetFree(LSET s)                                                    */
/*                                                                           */
/*  Free s.                                                                  */
/*                                                                           */
/*****************************************************************************/

void LSetFree(LSET s)
{
  free(s);
}


/*****************************************************************************/
/*                                                                           */
/*  char *LSetShow(LSET s)                                                   */
/*                                                                           */
/*  Static display of s.                                                     */
/*                                                                           */
/*****************************************************************************/
/* #define end_buff &buff[bp][strlen(buff[bp])] */

static void show_interval(char *buff, bool first, int from, int to)
{
  if( from == to )
    sprintf(buff, "%s%d", first ? "" : ", ", from);
  else
    sprintf(buff, "%s%d-%d", first ? "" : ", ", from, to);
}

typedef enum { INTERVAL_INSIDE, INTERVAL_OUTSIDE } INTERVAL_STATE;
char *LSetShow(LSET s)
{
  static char buff[4][200], tmp[200];
  static int bp = 0;
  int i, card, start_interval, bpos, npos;  bool first;  INTERVAL_STATE state;
  bp = (bp + 1) % 4;
  card = s->length * INT_BIT;
  sprintf(buff[bp], "{");
  bpos = 1;
  first = true;
  state = INTERVAL_OUTSIDE;
  start_interval = 0;  /* really undefined */
  for( i = 0;  i < card;  i++ ) switch( state )
  {
    case INTERVAL_INSIDE:

      if( !LSetContains(s, i) )
      {
	show_interval(tmp, first, start_interval, i-1);
	npos = bpos + strlen(tmp);
	if( npos >= 195 )
	{
	  sprintf(&buff[bp][bpos], ", ...}");
	  return buff[bp];
	}
        strcpy(&buff[bp][bpos], tmp);
	bpos = npos;
	first = false;
	state = INTERVAL_OUTSIDE;
      }
      break;

    case INTERVAL_OUTSIDE:

      if( LSetContains(s, i) )
      {
	start_interval = i;
	state = INTERVAL_INSIDE;
      }
      break;
  }
  if( state == INTERVAL_INSIDE )
  {
    show_interval(tmp, first, start_interval, i-1);
    npos = bpos + strlen(tmp);
    if( npos >= 195 )
    {
      sprintf(&buff[bp][bpos], ", ...}");
      return buff[bp];
    }
    strcpy(&buff[bp][bpos], tmp);
    bpos = npos;
  }
  strcpy(&buff[bp][bpos], "}");
  return buff[bp];
}


/*****************************************************************************/
/*                                                                           */
/*  char *show_bool(bool val)                                                */
/*                                                                           */
/*  Return a string representation of val.                                   */
/*                                                                           */
/*****************************************************************************/

static char *show_bool(bool val)
{
  return val ? "true" : "false";
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetGenerateTables(FILE *fp)                                        */
/*                                                                           */
/*  Generate the tables that given the index of the first and last non-zero  */
/*  bit of a given non-zero byte.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** no longer needed now that tables have been generated
static int LSetFirstNonZeroBit(int byte)
{
  int i;
  for( i = 0;  i < CHAR_BIT;  i++ )
    if( byte & (1 << i) )
      return i;
  return CHAR_BIT;
}

static int LSetLastNonZeroBit(int byte)
{
  int i;
  for( i = CHAR_BIT - 1;  i >= 0;  i-- )
    if( byte & (1 << i) )
      return i;
  return CHAR_BIT;
}

static void LSetGenerateTables(FILE *fp)
{
  int i, j;

  ** generate first_nonzero_bit **
  fprintf(fp, "static char first_nonzero_bit[1 << CHAR_BIT] = {\n");
  for( i = 0;  i < (1 << CHAR_BIT);  i += (1 << (CHAR_BIT/2)) )
  {
    fprintf(fp, "  ");
    for( j = 0;  j < (1 << (CHAR_BIT/2));  j++ )
    {
      fprintf(fp, "%d%s", LSetFirstNonZeroBit(i + j),
	i + j + 1 == (1 << CHAR_BIT) ? "};\n" :
	j + 1 == (1 << (CHAR_BIT/2)) ? ",\n" : ", ");
    }
  }

  ** generate last_nonzero_bit **
  fprintf(fp, "static char last_nonzero_bit[1 << CHAR_BIT] = {\n");
  for( i = 0;  i < (1 << CHAR_BIT);  i += (1 << (CHAR_BIT/2)) )
  {
    fprintf(fp, "  ");
    for( j = 0;  j < (1 << (CHAR_BIT/2));  j++ )
    {
      fprintf(fp, "%d%s", LSetLastNonZeroBit(i + j),
	i + j + 1 == (1 << CHAR_BIT) ? "};\n" :
	j + 1 == (1 << (CHAR_BIT/2)) ? ",\n" : ", ");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void LSetTest(FILE *fp)                                                  */
/*                                                                           */
/*  Test this module.                                                        */
/*                                                                           */
/*****************************************************************************/

void LSetTest(FILE *fp)
{
  LSET s0, s1, s2, s3, cs0, cs1, cs2, cs3;
  fprintf(fp, "[ LSetTest()\n");

  /* set s0 to the empty set */
  s0 = LSetNew();
  fprintf(fp, "  s0 := %s\n", LSetShow(s0));

  /* set s1 to {0, 10, 20, 30, 40, 50, 60, 70, 80, 90} */
  s1 = LSetNew();
  LSetInsert(&s1,  0);
  LSetInsert(&s1, 10);
  LSetInsert(&s1, 20);
  LSetInsert(&s1, 30);
  LSetInsert(&s1, 40);
  LSetInsert(&s1, 50);
  LSetInsert(&s1, 60);
  LSetInsert(&s1, 70);
  LSetInsert(&s1, 80);
  LSetInsert(&s1, 90);
  fprintf(fp, "  s1 := %s\n", LSetShow(s1));

  /* set s2 to {10, 20, 30} */
  s2 = LSetNew();
  LSetInsert(&s2, 10);
  LSetInsert(&s2, 20);
  LSetInsert(&s2, 30);
  fprintf(fp, "  s2 := %s\n", LSetShow(s2));

  /* set s3 to {80, 85, 90, 95} */
  s3 = LSetNew();
  LSetInsert(&s3, 80);
  LSetInsert(&s3, 85);
  LSetInsert(&s3, 90);
  LSetInsert(&s3, 95);
  fprintf(fp, "  s3 := %s\n", LSetShow(s3));
  fprintf(fp, "\n");

  /* test empty */
  fprintf(fp, "  LSetEmpty(s0) == %s\n", show_bool(LSetEmpty(s0)));
  fprintf(fp, "  LSetEmpty(s1) == %s\n", show_bool(LSetEmpty(s1)));
  fprintf(fp, "\n");

  /* test equal */
  fprintf(fp, "  LSetEqual(s0, s0) == %s\n", show_bool(LSetEqual(s0, s0)));
  fprintf(fp, "  LSetEqual(s1, s1) == %s\n", show_bool(LSetEqual(s1, s1)));
  fprintf(fp, "  LSetEqual(s1, s2) == %s\n", show_bool(LSetEqual(s1, s2)));
  fprintf(fp, "  LSetEqual(s2, s3) == %s\n", show_bool(LSetEqual(s2, s3)));
  fprintf(fp, "\n");

  /* test subset */
  fprintf(fp, "  LSetSubset(s0, s0) == %s\n", show_bool(LSetSubset(s0, s0)));
  fprintf(fp, "  LSetSubset(s1, s1) == %s\n", show_bool(LSetSubset(s1, s1)));
  fprintf(fp, "  LSetSubset(s0, s1) == %s\n", show_bool(LSetSubset(s0, s1)));
  fprintf(fp, "  LSetSubset(s1, s0) == %s\n", show_bool(LSetSubset(s1, s0)));
  fprintf(fp, "  LSetSubset(s1, s2) == %s\n", show_bool(LSetSubset(s1, s2)));
  fprintf(fp, "  LSetSubset(s2, s1) == %s\n", show_bool(LSetSubset(s2, s1)));
  fprintf(fp, "  LSetSubset(s2, s3) == %s\n", show_bool(LSetSubset(s2, s3)));
  fprintf(fp, "  LSetSubset(s3, s2) == %s\n", show_bool(LSetSubset(s3, s2)));
  fprintf(fp, "\n");

  /* test disjoint */
  fprintf(fp,"  LSetDisjoint(s0, s0) == %s\n",show_bool(LSetDisjoint(s0,s0)));
  fprintf(fp,"  LSetDisjoint(s1, s1) == %s\n",show_bool(LSetDisjoint(s1,s1)));
  fprintf(fp,"  LSetDisjoint(s1, s2) == %s\n",show_bool(LSetDisjoint(s1,s2)));
  fprintf(fp,"  LSetDisjoint(s2, s3) == %s\n",show_bool(LSetDisjoint(s2,s3)));
  fprintf(fp,"\n");

  /* test contains */
  fprintf(fp,"  LSetContains(s0, 0) == %s\n", show_bool(LSetContains(s0, 0)));
  fprintf(fp,"  LSetContains(s1, 0) == %s\n", show_bool(LSetContains(s1, 0)));
  fprintf(fp,"  LSetContains(s1, 3) == %s\n", show_bool(LSetContains(s1, 3)));
  fprintf(fp,"  LSetContains(s3, 94) == %s\n",show_bool(LSetContains(s3,94)));
  fprintf(fp,"  LSetContains(s3, 95) == %s\n",show_bool(LSetContains(s3,95)));
  fprintf(fp,"  LSetContains(s3, 96) == %s\n",show_bool(LSetContains(s3,96)));
  fprintf(fp,"\n");

  /* test copy */
  cs0 = LSetCopy(s0);
  fprintf(fp, "  LSetCopy(s0) = %s\n", LSetShow(cs0));
  cs1 = LSetCopy(s1);
  fprintf(fp, "  LSetCopy(s1) = %s\n", LSetShow(cs1));
  cs2 = LSetCopy(s2);
  fprintf(fp, "  LSetCopy(s2) = %s\n", LSetShow(cs2));
  cs3 = LSetCopy(s3);
  fprintf(fp, "  LSetCopy(s3) = %s\n", LSetShow(cs3));
  fprintf(fp, "\n");

  /* test union */
  LSetUnion(&cs1, cs0);
  fprintf(fp, "  LSetUnion(s1, s0) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  LSetUnion(&cs1, cs1);
  fprintf(fp, "  LSetUnion(s1, s1) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  LSetUnion(&cs1, cs2);
  fprintf(fp, "  LSetUnion(s1, s2) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  LSetUnion(&cs1, cs3);
  fprintf(fp, "  LSetUnion(s1, s3) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  fprintf(fp, "\n");

  /* test intersection */
  LSetIntersection(cs1, cs3);
  fprintf(fp, "  LSetIntersection(s1, s3) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  LSetIntersection(cs1, cs2);
  fprintf(fp, "  LSetIntersection(s1, s2) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  LSetIntersection(cs1, cs0);
  fprintf(fp, "  LSetIntersection(s1, s0) == %s\n", LSetShow(cs1));
  cs1 = LSetCopy(s1);
  fprintf(fp, "\n");

  /* test difference */
  LSetDifference(cs2, cs0);
  fprintf(fp, "  LSetDifference(s2, s0) == %s\n", LSetShow(cs2));
  cs2 = LSetCopy(s2);
  LSetDifference(cs3, cs2);
  fprintf(fp, "  LSetDifference(s3, s2) == %s\n", LSetShow(cs3));
  cs3 = LSetCopy(s3);
  LSetDifference(cs3, cs1);
  fprintf(fp, "  LSetDifference(s3, s1) == %s\n", LSetShow(cs3));
  cs3 = LSetCopy(s3);
  fprintf(fp, "\n");

  /* test min */
  fprintf(fp, "  LSetMin(s1) == %d\n", LSetMin(s1));
  fprintf(fp, "  LSetMin(s2) == %d\n", LSetMin(s2));
  fprintf(fp, "  LSetMin(s3) == %d\n", LSetMin(s3));
  fprintf(fp, "\n");

  /* test max */
  fprintf(fp, "  LSetMax(s1) == %d\n", LSetMax(s1));
  fprintf(fp, "  LSetMax(s2) == %d\n", LSetMax(s2));
  fprintf(fp, "  LSetMax(s3) == %d\n", LSetMax(s3));
  fprintf(fp, "\n");

  /* generate tables */
  /* LSetGenerateTables(fp); */

  fprintf(fp, "] LSetTest()\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "hash tables indexed by lsets"                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct lset_table_entry_rec {
  LSET		key;		/* table is indexed by lset                  */
  unsigned int	hashed_key;	/* hashed key before % table size            */
  void		*value;		/* associated value                          */
} LSET_TABLE_ENTRY;

typedef MARRAY(LSET_TABLE_ENTRY) ARRAY_LSET_TABLE_ENTRY;

struct lset_table_rec {
  int			trigger;	/* trigger for rehashing             */
  ARRAY_LSET_TABLE_ENTRY entries;	/* extensible array of values        */
};


/*****************************************************************************/
/*                                                                           */
/*  void LSetTableDebug(LSET_TABLE lt, int indent, FILE *fp)                 */
/*                                                                           */
/*  Debug print of lt.                                                       */
/*                                                                           */
/*****************************************************************************/

static void LSetTableDebug(LSET_TABLE lt, int indent, FILE *fp)
{
  LSET_TABLE_ENTRY e;  int i, count;
  fprintf(fp, "%*s[ LSetTable (size %d):\n", indent, "",
    MArraySize(lt->entries));
  count = 0;
  MArrayForEach(lt->entries, &e, &i)
    if( e.key != NULL )
    {
      fprintf(fp, "%*s  %d: %d %u %s -> %p\n", indent, "", i,
	e.hashed_key % MArraySize(lt->entries), e.hashed_key,
	LSetShow(e.key), e.value);
      count++;
    }
    else
      fprintf(fp, "%*s  %d: NULL\n", indent, "", i);
  fprintf(fp, "%*s] density %.1f\n", indent, "",
    (float) count / MArraySize(lt->entries));
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTableRehash(LSET_TABLE lt)                                      */
/*                                                                           */
/*  Rehash lt.                                                               */
/*                                                                           */
/*****************************************************************************/
static void LSetTableInsertHashed(LSET_TABLE lt, LSET s,
  unsigned int hashed_key, void *val);

static void LSetTableRehash(LSET_TABLE lt)
{
  ARRAY_LSET_TABLE_ENTRY old_entries;  LSET_TABLE_ENTRY e;
  int i, new_size;
  if( DEBUG4 )
    fprintf(stderr, "[ LSetTableRehash(old_size %d)\n",
      MArraySize(lt->entries));
  if( DEBUG1 )
  {
    fprintf(stderr, "[ LSetTableRehash(lt)\n");
    fprintf(stderr, "  initial table:\n");
    LSetTableDebug(lt, 2, stderr);
  }

  /* save old entries */
  old_entries = lt->entries;

  /* reinitialize lt to an empty table of the next larger size */
  new_size = 2 * MArraySize(old_entries) + 5;
  lt->trigger = (4 * new_size) / 5;
  e.key = NULL, e.hashed_key = 0, e.value = NULL;
  MArrayInit(lt->entries);
  MArrayFill(lt->entries, new_size, e);
  if( DEBUG1 )
  {
    fprintf(stderr, "  middle table:\n");
    LSetTableDebug(lt, 2, stderr);
  }

  /* insert the old entries into the table, then free their array */
  if( DEBUG4 )
    fprintf(stderr, "  LSetTableRehash transferring entries\n");
  MArrayForEach(old_entries, &e, &i)
  {
    if( DEBUG4 && i % 20000 == 0 )
      fprintf(stderr, "  LSetTableRehash at position %d\n", i);
    if( e.key != NULL )
    {
      LSetTableInsertHashed(lt, e.key, e.hashed_key, e.value);
      if( DEBUG1 )
      {
	fprintf(stderr, "  after an insert:\n");
	LSetTableDebug(lt, 2, stderr);
      }
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "  LSetTableRehash freeing keys and values\n");
  MArrayFree(old_entries);
  if( DEBUG4 )
    fprintf(stderr, "] LSetTableRehash returning(new_size %d)\n",
      MArraySize(lt->entries));
  if( DEBUG1 )
  {
    fprintf(stderr, "  final table:\n");
    LSetTableDebug(lt, 2, stderr);
    fprintf(stderr, "] LSetTableRehash returning)\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  LSET_TABLE LSetTableMake(void)                                           */
/*                                                                           */
/*  Make a new, empty lset table.                                            */
/*                                                                           */
/*****************************************************************************/

LSET_TABLE LSetTableMake(void)
{
  LSET_TABLE res;
  MMake(res);
  res->trigger = 0;
  MArrayInit(res->entries);
  LSetTableRehash(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTableFree(LSET_TABLE lt)                                        */
/*                                                                           */
/*  Free lt.                                                                 */
/*                                                                           */
/*****************************************************************************/

void LSetTableFree(LSET_TABLE lt)
{
  MArrayFree(lt->entries);
  MFree(lt);
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTableInsertHashed(LSET_TABLE lt, LSET s,                        */
/*    unsigned int hashed_key, void *val)                                    */
/*                                                                           */
/*  Insert (s, val) into lt, without knowing or caring whether s is          */
/*  already present (it is up to the user to call Retrieve first).           */
/*                                                                           */
/*****************************************************************************/

static void LSetTableInsertHashed(LSET_TABLE lt, LSET s,
  unsigned int hashed_key, void *val)
{
  unsigned int i, size;  LSET_TABLE_ENTRY e;
  if( DEBUG1 )
    fprintf(stderr, "  LSetTableInsertHashed(lt, %s, %d, %p)\n",
      LSetShow(s), hashed_key, val);
  if( lt->trigger <= 1 )
    LSetTableRehash(lt);
  e.key = s;
  e.hashed_key = hashed_key;
  e.value = val;
  size = (unsigned int) MArraySize(lt->entries);
  i = hashed_key % size;
  while( MArrayGet(lt->entries, i).key != NULL )
    i = (i + 1) % size;
  MArrayPut(lt->entries, i, e);
  lt->trigger--;
  if( DEBUG1 )
    LSetTableDebug(lt, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTableInsert(LSET_TABLE lt, LSET s, void *val)                   */
/*                                                                           */
/*  Insert (s, val) into lt, without knowing or caring whether s is          */
/*  already present (it is up to the user to call Retrieve first).           */
/*                                                                           */
/*****************************************************************************/

void LSetTableInsert(LSET_TABLE lt, LSET s, void *val)
{
  LSetTableInsertHashed(lt, s, LSetHash(s), val);
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetTableRetrieve(LSET_TABLE lt, LSET s, void **val)                */
/*                                                                           */
/*  If s is present in lt, return true and set *val to the corresponding     */
/*  value.  Otherwise return false,                                          */
/*                                                                           */
/*****************************************************************************/

bool LSetTableRetrieve(LSET_TABLE lt, LSET s, void **val)
{
  unsigned int i, size;  int start, limit;  LSET_TABLE_ENTRY e;
  if( DEBUG1 )
  {
    fprintf(stderr, "  [ LSetTableRetrieve(lt, %s)", LSetShow(s));
    LSetTableDebug(lt, 2, stderr);
  }
  size = (unsigned int) MArraySize(lt->entries);
  start = i = LSetHash(s) % size;
  MAssert(i >= 0, "LSetTableRetrieve internal error (%d %% %d == %d)",
    LSetHash(s), size, i);
  limit = 0;
  e = MArrayGet(lt->entries, i);
  while( e.key != NULL )
  {
    if( DEBUG1 )
      fprintf(stderr, "   trying %d: %s\n", i, LSetShow(e.key));
    if( LSetEqual(e.key, s) )
    {
      *val = e.value;
      if( DEBUG1 )
	fprintf(stderr, "  ] LSetTableRetrieve returning true (%p)\n", *val);
      return true;
    }
    i = (i + 1) % size;
    e = MArrayGet(lt->entries, i);
    if( DEBUG3 && ++limit > 500 )
    {
      if( size < 6000 )
	LSetTableDebug(lt, 4, stderr);
      MAbort("  LSetTableRetrieve limit %d exceeded (size %u, start %d, i %u)",
	limit, size, start, i);
    }
  }
  *val = NULL;
  if( DEBUG2 )
  {
    static int calls = 0;  int len;
    calls++;
    if( calls % 100 == 0 )
    {
      len = (start <= i ? i - start : (size - start) + i);
      fprintf(stderr,
	"    LSetTableRetrieve len %d (size %u, start %d, i %u)\n",
	len, size, start, i);
      if( len > 1000 )
      {
	LSetTableDebug(lt, 4, stderr);
	exit(0);
      }
    }

  }
  if( DEBUG1 )
    fprintf(stderr, "  ] LSetTableRetrieve returning false\n");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "tries indexed by lsets"                                       */
/*                                                                           */
/*  This trie data structure has two optimizations, intended to allow tries  */
/*  with many thousands of entries to be handled reasonably efficiently.     */
/*                                                                           */
/*  First, the external nodes at the bottom of the tree are not individual   */
/*  entries, they are buckets holding between 1 and 50 entries.  They are    */
/*  only expanded into internal nodes when their size reaches 50, as a       */
/*  space optimization.  Each path down the tree ends with either a bucket   */
/*  node or with NULL.                                                       */
/*                                                                           */
/*  Second, because there are likely to be many zero bytes in the lsets,     */
/*  zero bytes are handled specially.  Instead of treating each lset as      */
/*  a simple sequence of bytes:                                              */
/*                                                                           */
/*    <byte>, <byte>, ... , <byte>                                           */
/*                                                                           */
/*  adjacent zero bytes are conceptually grouped together, and their         */
/*  number is used for indexing:                                             */
/*                                                                           */
/*    (<zero-bytes>, <non-zero-byte>), ..., (<zero-bytes>, <non-zero-byte>)  */
/*                                                                           */
/*  Each (<zero-bytes>, <non-zero-byte>) pair is the index for one internal  */
/*  node, with <zero-bytes> counting the number of zero bytes immediately    */
/*  preceding <non-zero-byte>, and indexing an extensible array of pointers, */
/*  each of which is either NULL or points to an array of 256 children       */
/*  indexed by the <non-zero-byte> part of the pair.                         */
/*                                                                           */
/*  Trailing zero bytes do not contribute to the indexing.  For example,     */
/*  an empty lset is indexed by an empty sequence, and goes into the root,   */
/*  which is always an internal node.                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE_ENTRY - one (key, value) entry in the trie                     */
/*                                                                           */
/*****************************************************************************/

typedef struct lset_trie_entry_rec {
  LSET		key;
  void		*value;
  /* ***
  LSET		key2;
  void		*value2;
  *** */
} LSET_TRIE_ENTRY;

typedef MARRAY(LSET_TRIE_ENTRY) ARRAY_LSET_TRIE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE_NODE - one node of the trie (internal or external)             */
/*                                                                           */
/*****************************************************************************/

typedef struct lset_trie_node_rec *LSET_TRIE_NODE;
typedef MARRAY(LSET_TRIE_NODE) ARRAY_LSET_TRIE_NODE;

typedef enum {
  LSET_TRIE_NODE_TYPE_INTERNAL,
  LSET_TRIE_NODE_TYPE_EXTERNAL,
} LSET_TRIE_NODE_TYPE;

typedef struct lset_trie_subnode_rec {
  LSET_TRIE_NODE	children[256];		/* indexed by <non-zero-byte>*/
} *LSET_TRIE_SUBNODE;

typedef MARRAY(LSET_TRIE_SUBNODE) ARRAY_LSET_TRIE_SUBNODE;

struct lset_trie_node_rec {
  LSET_TRIE_NODE_TYPE		node_type;
  union {
    struct { /* internal node case:  subnodes, plus one optional entry */
      LSET_TRIE_ENTRY		entry;		/* if key sequence ends here */
      ARRAY_LSET_TRIE_SUBNODE	subnodes;	/* indexed by <zero-bytes>   */
    } i;
    struct { /* internal node case:  a bucket of entries */
      ARRAY_LSET_TRIE_ENTRY	entries;
    } e;
  } u;
};


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE - the trie as a whole                                          */
/*                                                                           */
/*****************************************************************************/

struct lset_trie_rec
{
  LSET_TRIE_NODE	root;
  ARRAY_LSET_TRIE_NODE	free_external_nodes;
  int			count;
};


/*****************************************************************************/
/*                                                                           */
/*  unsigned char LSetByte(LSET s, int i)                                    */
/*                                                                           */
/*  Return the i'th byte of s, counting from 0 as usual.                     */
/*                                                                           */
/*****************************************************************************/

static unsigned char LSetByte(LSET s, int i)
{
  unsigned int word;
  word = s->elems[i / sizeof(unsigned int)];
  return (word >> (8 * (3 - i % sizeof(unsigned int)))) & 0xFF;
}


/*****************************************************************************/
/*                                                                           */
/*  int LSetByteCount(LSET s)                                                */
/*                                                                           */
/*  Return the number of bytes in s, not counting zero bytes at the end.     */
/*                                                                           */
/*****************************************************************************/

static int LSetByteCount(LSET s)
{
  int i, j;

  /* find the last non-zero word, and return 0 if there isn't any */
  for( i = s->length - 1;  i >= 0;  i-- )
    if( s->elems[i] != 0 )
      break;
  if( i < 0 )
    return 0;

  /* find the last non-zero byte of the last non-zero word */
  for( j = (i + 1) * sizeof(unsigned int) - 1;  LSetByte(s, j) == 0;  j-- );

  /* the desired count is one more than the index of the last non-zero byte */
  return j + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieNodeDebug(LSET_TRIE_NODE ltn, LSET_TRIE_NODE mark_ltn,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ltn onto fp with the given indent.  Mark it with an       */
/*  asterisk if it is mark_ltn.                                              */
/*                                                                           */
/*  Implementation note.  The indent is omitted on the first line.           */
/*                                                                           */
/*****************************************************************************/

static void LSetTrieNodeDebug(LSET_TRIE_NODE ltn, LSET_TRIE_NODE mark_ltn,
  int indent, FILE *fp)
{
  LSET_TRIE_ENTRY e;  int i, j;  LSET_TRIE_SUBNODE sn;
  switch( ltn->node_type )
  {
    case LSET_TRIE_NODE_TYPE_INTERNAL:

      fprintf(fp, "[ %sInternal", ltn == mark_ltn ? "*" : "");
      if( ltn->u.i.entry.key != NULL )
	fprintf(fp, " %s -> %p", LSetShow(ltn->u.i.entry.key),
	  ltn->u.i.entry.value);
      fprintf(fp, "\n");
      MArrayForEach(ltn->u.i.subnodes, &sn, &i)
        if( sn != NULL )
	  for( j = 0;  j < 256;  j++ )
	    if( sn->children[j] != NULL )
	    {
	      fprintf(fp, "%*s(%d, %x): ", indent + 2, "", i, j);
              LSetTrieNodeDebug(sn->children[j], mark_ltn, indent + 2, fp);
	    }
      fprintf(fp, "%*s]\n", indent, "");
      break;

    case LSET_TRIE_NODE_TYPE_EXTERNAL:

      fprintf(fp, "[ %sExternal\n", ltn == mark_ltn ? "*" : "");
      MArrayForEach(ltn->u.e.entries, &e, &i)
	fprintf(fp, "%*s%s -> %p\n", indent + 2, "", LSetShow(e.key), e.value);
      fprintf(fp, "%*s]\n", indent, "");
      break;

    default:

      MAbort("LSetNodeDebug: unknown node type %d", ltn->node_type);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieDebug(LSET_TRIE lt, LSET_TRIE_NODE mark_ltn,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of lt onto fp with the given indent.  Mark mark_ltn with an  */
/*  asterisk if present.                                                     */
/*                                                                           */
/*****************************************************************************/

static void LSetTrieDebug(LSET_TRIE lt, LSET_TRIE_NODE mark_ltn,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s[ LSetTrie (%d entries)\n", indent, "", lt->count);
  fprintf(fp, "%*s  ", indent, "");
  LSetTrieNodeDebug(lt->root, mark_ltn, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE_NODE LSetTrieNodeExternalMake(LSET_TRIE lt, LSET_TRIE_ENTRY e) */
/*                                                                           */
/*  Return a new bucket node holding entry e.                                */
/*                                                                           */
/*****************************************************************************/

static LSET_TRIE_NODE LSetTrieNodeExternalMake(LSET_TRIE lt, LSET_TRIE_ENTRY e)
{
  LSET_TRIE_NODE res;
  if( MArraySize(lt->free_external_nodes) > 0 )
  {
    res = MArrayRemoveLast(lt->free_external_nodes);
    MArrayClear(res->u.e.entries);
  }
  else
  {
    MMake(res);
    MArrayInit(res->u.e.entries);
  }
  res->node_type = LSET_TRIE_NODE_TYPE_EXTERNAL;
  MArrayAddLast(res->u.e.entries, e);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE_NODE LSetTrieNodeInternalMake(void)                            */
/*                                                                           */
/*  Make a new, empty internal node.                                         */
/*                                                                           */
/*****************************************************************************/

static LSET_TRIE_NODE LSetTrieNodeInternalMake(void)
{
  LSET_TRIE_NODE res;
  MMake(res);
  res->node_type = LSET_TRIE_NODE_TYPE_INTERNAL;
  res->u.i.entry.key = NULL;
  res->u.i.entry.value = NULL;
  /* ***
  res->u.i.entry.key2 = NULL;
  res->u.i.entry.value2 = NULL;
  *** */
  MArrayInit(res->u.i.subnodes);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE LSetTrieMake(void)                                             */
/*                                                                           */
/*  Make a new, empty lset trie.                                             */
/*                                                                           */
/*****************************************************************************/

LSET_TRIE LSetTrieMake(void)
{
  LSET_TRIE res;
  MMake(res);
  res->root = LSetTrieNodeInternalMake();
  MArrayInit(res->free_external_nodes);
  res->count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieNodeFree(LSET_TRIE_NODE ltn)                                */
/*                                                                           */
/*  Free ltn, assumed to be non-NULL.                                        */
/*                                                                           */
/*****************************************************************************/

static void LSetTrieNodeFree(LSET_TRIE_NODE ltn)
{
  int i, j;  LSET_TRIE_SUBNODE sn;
  switch( ltn->node_type )
  {
    case LSET_TRIE_NODE_TYPE_INTERNAL:

      MArrayForEach(ltn->u.i.subnodes, &sn, &i)
	if( sn != NULL )
	{
	  for( j = 0;  j < 256;  j++ )
	    if( sn->children[j] != NULL )
              LSetTrieNodeFree(sn->children[j]);
	  MFree(sn);
	}
      MArrayFree(ltn->u.i.subnodes);
      break;

    case LSET_TRIE_NODE_TYPE_EXTERNAL:

      MArrayFree(ltn->u.e.entries);
      break;

    default:

      MAbort("LSetTrieNodeFree internal error:  type %d", ltn->node_type);
  }
  MFree(ltn);
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieFree(LSET_TRIE lt)                                          */
/*                                                                           */
/*  Free lt.                                                                 */
/*                                                                           */
/*****************************************************************************/

void LSetTrieFree(LSET_TRIE lt)
{
  LSET_TRIE_NODE ltn;  int i;
  LSetTrieNodeFree(lt->root);
  MArrayForEach(lt->free_external_nodes, &ltn, &i)
    LSetTrieNodeFree(ltn);
  MFree(lt);
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieNextKey(LSET s, int *byte_index,                            */
/*    int *zero_bytes, unsigned char *non_zero_byte)                         */
/*                                                                           */
/*  Return the next internal node key in (*zero_bytes, *non_zero_bytes),     */
/*  starting at *byte_index.  Also increase *byte_index appropriately.       */
/*                                                                           */
/*****************************************************************************/

static void LSetTrieNextKey(LSET s, int *byte_index,
  int *zero_bytes, unsigned char *non_zero_byte)
{
  unsigned char b;  int i;
  for( i = *byte_index, b = LSetByte(s, i);  b == 0;  i++, b = LSetByte(s, i) );
  *zero_bytes = i - *byte_index;
  *non_zero_byte = b;
  *byte_index = i + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  LSET_TRIE_NODE LSetTrieExternalExpand(LSET_TRIE_SUBNODE sn,              */
/*    unsigned char j, LSET_TRIE lt)                                         */
/*                                                                           */
/*  sn->children[j] holds ltn, an external node.  Replace that node by       */
/*  an internal node holding the same entries, and return that node.         */
/*                                                                           */
/*****************************************************************************/

static LSET_TRIE_NODE LSetTrieExternalExpand(LSET_TRIE_SUBNODE sn,
  unsigned char j, LSET_TRIE lt)
{
  LSET_TRIE_ENTRY e;  int i;  LSET_TRIE_NODE old_ltn;

  /* replace ltn by a new, empty internal node */
  old_ltn = sn->children[j];
  MAssert(old_ltn->node_type == LSET_TRIE_NODE_TYPE_EXTERNAL,
    "LSetTrieExternalExpand internal error 1");
  sn->children[j] = LSetTrieNodeInternalMake();
  MAssert(sn->children[j]->node_type == LSET_TRIE_NODE_TYPE_INTERNAL,
    "LSetTrieExternalExpand internal error 2");
  lt->count -= MArraySize(old_ltn->u.e.entries);

  /* reinsert the contents of ltn */
  MArrayForEach(old_ltn->u.e.entries, &e, &i)
    LSetTrieInsert(lt, e.key, e.value);

  /* add old_ltn to the list of free external nodes, and return the new node */
  MArrayAddLast(lt->free_external_nodes, old_ltn);
  return sn->children[j];
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieInsert(LSET_TRIE lt, LSET s, void *val)                     */
/*                                                                           */
/*  Insert (s, val), aborting if s is already present.                       */
/*                                                                           */
/*****************************************************************************/

void LSetTrieInsert(LSET_TRIE lt, LSET s, void *val)
{
  LSET_TRIE_NODE ltn, ltn2;  int i, j, zero_bytes, byte_count, byte_index;
  unsigned char non_zero_byte;  LSET_TRIE_ENTRY e, se;  LSET_TRIE_SUBNODE sn;
  MAssert(val != NULL, "LSetTrieInsert:  val == NULL");
  /* if( DEBUG6 ) LSetTrieCheck(lt); */
  if( DEBUG5 && lt->count % 10000 == 0 )
  {
    fprintf(stderr, "[ LSetTrieInsert(lt, %s, %p):\n", LSetShow(s), val);
    LSetTrieDebug(lt, NULL, 2, stderr);
  }
  if( DEBUG8 )
    fprintf(stderr, "LSetTrieInsert(%p, %p %s, %p):\n", (void *) lt,
      (void *) s, LSetShow(s), val);
  lt->count++;
  se.key = s, se.value = val /* , se.key2 = s, se.value2 = val */;
  byte_count = LSetByteCount(s);
  byte_index = 0;
  ltn = lt->root;
  /* loop invariant:  ltn is an internal node */
  while( byte_index < byte_count )
  {
    /* must be an unused non-zero byte in s; find the first one */
    MAssert(ltn->node_type == LSET_TRIE_NODE_TYPE_INTERNAL,
      "LSetTrieInsert internal error 1");
    LSetTrieNextKey(s, &byte_index, &zero_bytes, &non_zero_byte);

    /* get or make a subnode at position zero_bytes */
    MArrayFill(ltn->u.i.subnodes, zero_bytes + 1, NULL);
    sn = MArrayGet(ltn->u.i.subnodes, zero_bytes);
    if( sn == NULL )
    {
      MMake(sn);
      for( j = 0;  j < 256;  j++ )
	sn->children[j] = NULL;
      MArrayPut(ltn->u.i.subnodes, zero_bytes, sn);
    }

    /* get or make a child at position non_zero_byte */
    ltn2 = sn->children[non_zero_byte];
    if( ltn2 == NULL )
    {
      /* make an external node, add (s, val) to it, and return */
      sn->children[non_zero_byte] = LSetTrieNodeExternalMake(lt, se);
      /* if( DEBUG6 ) LSetTrieCheck(lt); */
      if( DEBUG5 && lt->count % 10000 == 1 )
	fprintf(stderr, "] LSetTrieInsert returning\n");
      return;
    }
    else if( ltn2->node_type == LSET_TRIE_NODE_TYPE_INTERNAL )
    {
      /* continue the insertion in ltn2 */
      ltn = ltn2;
    }
    else if( MArraySize(ltn2->u.e.entries) >= LSET_TRIE_BUCKET_LIMIT )
    {
      /* replace external node by internal node and continue inserting */
      ltn = LSetTrieExternalExpand(sn, non_zero_byte, lt);
    }
    else
    {
      /* add to external node and return */
      MArrayForEach(ltn2->u.e.entries, &e, &i)
	if( LSetEqual(e.key, se.key) )
	{
	  if( DEBUG5 )
	  {
	    void *val;
	    fprintf(stderr, "  LSetTrieInsert failing on %s:\n", LSetShow(s));
	    LSetTrieDebug(lt, ltn2, 2, stderr);
	    fprintf(stderr, "  when LSetTrieRetrieve(lt, %s, &val) is %s\n",
	      LSetShow(s), LSetTrieRetrieve(lt, s, &val) ? "true" : "false");
	  }
	  MAbort("LSetTrieInsert: key already present");
	}
      MArrayAddLast(ltn2->u.e.entries, se);
      /* if( DEBUG6 ) LSetTrieCheck(lt); */
      if( DEBUG5 && lt->count % 10000 == 1 )
	fprintf(stderr, "] LSetTrieInsert returning\n");
      return;
    }
  }

  /* if we get to this point, then ltn != NULL && byte_index >= byte_count, */
  /* so the new entry goes into ltn->u.i.entry, or nowhere */
  MAssert(ltn->node_type == LSET_TRIE_NODE_TYPE_INTERNAL,
    "LSetTrieInsert internal error 2");
  if( ltn->u.i.entry.key == NULL )
  {
    ltn->u.i.entry = se;
    /* if( DEBUG6 ) LSetTrieCheck(lt); */
    if( DEBUG5 && lt->count % 10000 == 1 )
      fprintf(stderr, "] LSetTrieInsert returning\n");
    return;
  }
  else
  {
    if( DEBUG5 )
    {
      fprintf(stderr, "  LSetTrieInsert failing on %s in:\n", LSetShow(s));
      LSetTrieDebug(lt, ltn, 2, stderr);
    }
    MAbort("LSetTrieInsert: key already present");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool LSetTrieRetrieve(LSET_TRIE lt, LSET s, void **val)                  */
/*                                                                           */
/*  Retrieve the value associated with s, returning true if present.         */
/*                                                                           */
/*****************************************************************************/
/* static void LSetTrieNodeCheck(LSET_TRIE_NODE ltn); */

bool LSetTrieRetrieve(LSET_TRIE lt, LSET s, void **val)
{
  LSET_TRIE_NODE ltn, ltn2;  int i, zero_bytes, byte_count, byte_index;
  unsigned char non_zero_byte;  LSET_TRIE_ENTRY e;  LSET_TRIE_SUBNODE sn;
  /* if( DEBUG6 ) LSetTrieCheck(lt); */
  byte_count = LSetByteCount(s);
  byte_index = 0;
  ltn = lt->root;
  /* loop invariant:  ltn is an internal node */
  while( byte_index < byte_count )
  {
    /* must be an unused non-zero byte in s; find the first one */
    MAssert(ltn->node_type == LSET_TRIE_NODE_TYPE_INTERNAL,
      "LSetTrieRetrieve internal error 1");
    LSetTrieNextKey(s, &byte_index, &zero_bytes, &non_zero_byte);

    /* get subnode at position zero_bytes; if none, return false */
    if( zero_bytes >= MArraySize(ltn->u.i.subnodes) )
      return *val = NULL, false;
    sn = MArrayGet(ltn->u.i.subnodes, zero_bytes);
    if( sn == NULL )
      return *val = NULL, false;

    /* get child at position non_zero_byte and finish if NULL or external */
    ltn2 = sn->children[non_zero_byte];
    if( ltn2 == NULL )
      return *val = NULL, false;
    else if( ltn2->node_type == LSET_TRIE_NODE_TYPE_EXTERNAL )
    {
      MArrayForEach(ltn2->u.e.entries, &e, &i)
	if( LSetEqual(e.key, s) )
	{
	  /* ***
          LSetTrieNodeCheck(ltn2);
	  MAssert(e.value != NULL, "LSetTrieRetrieve(lt, %s) has NULL value",
	    LSetShow(s));
	  *** */
	  if( DEBUG8 )
	    fprintf(stderr, "LSetTrieRetrieve(%p, %p %s) returning %s %p\n",
	      (void *) lt, (void *) s, LSetShow(s), "true", e.value);
	  return *val = e.value, true;
	}
      return *val = NULL, false;
    }

    /* ltn2 is internal, so search it */
    ltn = ltn2;
  }

  /* if we get to this point, then ltn != NULL && byte_index >= byte_count, */
  /* so the entry we are searching for is in ltn->u.i.entry, or nowhere */
  /* ***
  LSetTrieNodeCheck(ltn);
  MAssert(ltn->node_type == LSET_TRIE_NODE_TYPE_INTERNAL,
    "LSetTrieRetrieve internal error 2");
  *** */
  if( ltn->u.i.entry.key == NULL || !LSetEqual(ltn->u.i.entry.key, s) )
    return *val = NULL, false;
  else
  {
    /* ***
    MAssert(ltn->u.i.entry.value != NULL, "LSetTrieRetrieve(lt, %s) has NULL",
      LSetShow(s));
    *** */
    if( DEBUG8 )
      fprintf(stderr, "LSetTrieRetrieve(%p, %p %s) returning %s %p\n",
	(void *) lt, (void *) s, LSetShow(s), "true", ltn->u.i.entry.value);
    return *val = ltn->u.i.entry.value, true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieNodeCheck(LSET_TRIE_NODE ltn)                               */
/*                                                                           */
/*  Check ltn.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void LSetTrieNodeCheck(LSET_TRIE_NODE ltn)
{
  LSET_TRIE_ENTRY e;  LSET_TRIE_SUBNODE sn;  int i, j;  LSET_TRIE_NODE ltn2;
  switch( ltn->node_type )
  {
    case LSET_TRIE_NODE_TYPE_INTERNAL:

      MAssert(ltn->u.i.entry.key == ltn->u.i.entry.key2,
	"LSetTrieNodeCheck internal error 1");
      MAssert(ltn->u.i.entry.value == ltn->u.i.entry.value2,
	"LSetTrieNodeCheck internal error 2");
      MAssert(ltn->u.i.entry.value != (void *) 0x4fee38,
        "LSetTrieNodeCheck internal error 3");
      MArrayForEach(ltn->u.i.subnodes, &sn, &i)
	if( sn != NULL )
	  for( j = 0;  j < 256;  j++ )
	  {
	    ltn2 = sn->children[j];
	    if( ltn2 != NULL )
              LSetTrieNodeCheck(ltn2);
	  }
      break;

    case LSET_TRIE_NODE_TYPE_EXTERNAL:

      MArrayForEach(ltn->u.e.entries, &e, &i)
      {
	MAssert(e.key == e.key2, "LSetTrieNodeCheck internal error 4");
	MAssert(e.value == e.value2, "LSetTrieNodeCheck internal error 5");
	MAssert(e.value != (void *) 0x4fee38,
	  "LSetTrieNodeCheck internal error 6");
      }
      break;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void LSetTrieCheck(LSET_TRIE lt)                                         */
/*                                                                           */
/*  Debug check of lt to find fault.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void LSetTrieCheck(LSET_TRIE lt)
{
  LSetTrieNodeCheck(lt->root);
}
*** */

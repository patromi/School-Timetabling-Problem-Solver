
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
/*  FILE:         partition.c                                                */
/*  MODULE:       Partitions                                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_partition.h"
#include "howard_n.h"
#include <string.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

/*****************************************************************************/
/*                                                                           */
/*  IMPLEMENTATION-SPECIFIC CODE                                             */
/*                                                                           */
/*  The code in this section is implementation-specific.                     */
/*                                                                           */
/*  The elems array contains, in position i, the number of occurrences       */
/*  of the number i in the partition.  Position 0 (if present) always        */
/*  contains 0.  There are no trailing zeros in the array, they are          */
/*  always removed.                                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_partition_rec {
  HA_ARRAY_SHORT	elems;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionMake(void)                                     */
/*                                                                           */
/*  Return a new, empty partition.                                           */
/*                                                                           */
/*****************************************************************************/

KHE_PARTITION KhePartitionMake(HA_ARENA a)
{
  KHE_PARTITION res;
  HaMake(res, a);
  HaArrayInit(res->elems, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionFree(KHE_PARTITION p)                                   */
/*                                                                           */
/*  Free the memory occupied by p.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePartitionFree(KHE_PARTITION p)
{
  free(p->elems);
  free(p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionClear(KHE_PARTITION p)                                  */
/*                                                                           */
/*  Reset p to empty.                                                        */
/*                                                                           */
/*****************************************************************************/

extern void KhePartitionClear(KHE_PARTITION p)
{
  HaArrayClear(p->elems);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionCopy(KHE_PARTITION p)                          */
/*                                                                           */
/*  Return a copy of partition p.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_PARTITION KhePartitionCopy(KHE_PARTITION p, HA_ARENA a)
{
  KHE_PARTITION res;  int i;
  HaMake(res, a);
  HaArrayInit(res->elems, a);
  HaArrayAppend(res->elems, p->elems, i);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionEqual(KHE_PARTITION p1, KHE_PARTITION p2)               */
/*                                                                           */
/*  Return true if these two partitions are equal.                           */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionEqual(KHE_PARTITION p1, KHE_PARTITION p2)
{
  int i;
  if( HaArrayCount(p1->elems) != HaArrayCount(p2->elems) )
    return false;
  for( i = 0;  i < HaArrayCount(p1->elems);  i++ )
    if( HaArray(p1->elems, i) != HaArray(p2->elems, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionIsEmpty(KHE_PARTITION p)                                */
/*                                                                           */
/*  Return true if partition p is empty, else false.                         */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionIsEmpty(KHE_PARTITION p)
{
  return HaArrayCount(p->elems) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartitionSize(KHE_PARTITION p)                                    */
/*                                                                           */
/*  The size (sum of all the elems) of p.                                    */
/*                                                                           */
/*****************************************************************************/

int KhePartitionSize(KHE_PARTITION p)
{
  int i, res;  short el;
  res = 0;
  HaArrayForEach(p->elems, el, i)
    res += i * el;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartitionPartsWithSize(KHE_PARTITION p, int i)                    */
/*                                                                           */
/*  Return the number of parts of p that have size i.                        */
/*                                                                           */
/*****************************************************************************/

int KhePartitionPartsWithSize(KHE_PARTITION p, int i)
{
  return i < 0 || i >= HaArrayCount(p->elems) ? 0 : HaArray(p->elems, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartitionMax(KHE_PARTITION p)                                     */
/*                                                                           */
/*  Return a maximum element of p.  Here p must be non-empty.                */
/*                                                                           */
/*****************************************************************************/

int KhePartitionMax(KHE_PARTITION p)
{
  HnAssert(!KhePartitionIsEmpty(p), "KhePartitionMax given empty partition");
  return HaArrayCount(p->elems) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartitionMin(KHE_PARTITION p)                                     */
/*                                                                           */
/*  Return a minimum element of p.  Here p must be non-empty.                */
/*                                                                           */
/*****************************************************************************/

int KhePartitionMin(KHE_PARTITION p)
{
  int i;  short el;
  HnAssert(!KhePartitionIsEmpty(p), "KhePartitionMin given empty partition");
  HaArrayForEach(p->elems, el, i)
    if( el != 0 )
      return i;
  HnAbort("KhePartitionMin internal error");
  return 0; /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionContains(KHE_PARTITION p, int i)                        */
/*                                                                           */
/*  Return true if partition p contains an i.                                */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionContains(KHE_PARTITION p, int i)
{
  return i < 0 || i >= HaArrayCount(p->elems) ? false :
    HaArray(p->elems, i) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionContainsAtLeast(KHE_PARTITION p, int i, int &res)       */
/*                                                                           */
/*  Search p to see whether it has at least one part greater than or         */
/*  equal to i.  If it does, set *res to a least part greater than or        */
/*  equal to i and return true.  Otherwise return false.                     */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionContainsAtLeast(KHE_PARTITION p, int i, int *res)
{
  for( ; i < HaArrayCount(p->elems);  i++ )
    if( HaArray(p->elems, i) > 0 )
    {
      *res = i;
      return true;
    }
  *res = 0;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionContainsAtMost(KHE_PARTITION p, int i, int &res)        */
/*                                                                           */
/*  Search p to see whether it has at least one part less than or            */
/*  equal to i.  If it does, set *res to a greatest part less than or        */
/*  equal to i and return true.  Otherwise return false.                     */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionContainsAtMost(KHE_PARTITION p, int i, int *res)
{
  for( ;  i > 0;  i-- )
    if( HaArray(p->elems, i) > 0 )
    {
      *res = i;
      return true;
    }
  *res = 0;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionCovers(KHE_PARTITION p1, KHE_PARTITION p2)              */
/*                                                                           */
/*  Return true if p1 covers p2, that is, p2 is a subset of p1 when the      */
/*  two partitions are considered as multisets.                              */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionCovers(KHE_PARTITION p1, KHE_PARTITION p2)
{
  int i;
  if( KhePartitionIsEmpty(p2) )
    return true;
  else if( KhePartitionIsEmpty(p1) )
    return false;
  else if( KhePartitionMax(p2) > KhePartitionMax(p1) )
    return false;
  for( i = 0;  i < HaArrayCount(p2->elems);  i++ )
    if( HaArray(p2->elems, i) > HaArray(p1->elems, i) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePartitionParts(KHE_PARTITION p)                                   */
/*                                                                           */
/*  The number of parts in partition p.                                      */
/*                                                                           */
/*****************************************************************************/

int KhePartitionParts(KHE_PARTITION p)
{
  int i, res;  short el;
  res = 0;
  HaArrayForEach(p->elems, el, i)
    res += el;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionAdd(KHE_PARTITION p, int i)                             */
/*                                                                           */
/*  Add i to partition p.                                                    */
/*                                                                           */
/*****************************************************************************/

void KhePartitionAdd(KHE_PARTITION p, int i)
{
  HnAssert(i > 0, "KhePartitionAdd: i is 0 or negative");

  /* make sure p can hold i */
  HaArrayFill(p->elems, i + 1, 0);

  /* add i to p */
  HaArray(p->elems, i)++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionStripTrailingZeros(KHE_PARTITION p)                     */
/*                                                                           */
/*  Strip any trailing zeros from p.                                         */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionStripTrailingZeros(KHE_PARTITION p)
{
  while( HaArrayCount(p->elems) > 0 && HaArrayLast(p->elems) == 0 )
    HaArrayDeleteLast(p->elems);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSub(KHE_PARTITION p, int i)                             */
/*                                                                           */
/*  Take i away from partition p.  There must be an i in it to begin with.   */
/*                                                                           */
/*****************************************************************************/

void KhePartitionSub(KHE_PARTITION p, int i)
{
  HnAssert(0 < i && i < HaArrayCount(p->elems) && HaArray(p->elems, i) > 0,
    "KhePartitionSub: p does not contain i (%d)", i);
  HaArray(p->elems, i)--;
  KhePartitionStripTrailingZeros(p);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionMul(KHE_PARTITION p, int i)                             */
/*                                                                           */
/*  Multiply each element of p by i.                                         */
/*                                                                           */
/*****************************************************************************/

void KhePartitionMul(KHE_PARTITION p, int i)
{
  int j;  short el;
  HnAssert(i > 0, "KhePartitionMul: i is 0 or negative");
  HaArrayForEach(p->elems, el, j)
    HaArrayPut(p->elems, j, el * i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDiv(KHE_PARTITION p, int i)                             */
/*                                                                           */
/*  Divide p by i, that is, divide each multiplicity by i.                   */
/*                                                                           */
/*****************************************************************************/

void KhePartitionDiv(KHE_PARTITION p, int i)
{
  int j;  short el;
  HnAssert(i > 0, "KhePartitionDiv: i is 0 or negative");
  HaArrayForEach(p->elems, el, j)
    HaArrayPut(p->elems, j, el / i);
  KhePartitionStripTrailingZeros(p);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDivAndRound(KHE_PARTITION p, int i)                     */
/*                                                                           */
/*  Like KhePartitionDiv, but rounding rather than truncating.               */
/*                                                                           */
/*****************************************************************************/

void KhePartitionDivAndRound(KHE_PARTITION p, int i)
{
  int j;  short el;
  HnAssert(i > 0, "KhePartitionDivAndRound: i is 0 or negative");
  HaArrayForEach(p->elems, el, j)
    HaArrayPut(p->elems, j, 0.5 + (float) el / i);
  KhePartitionStripTrailingZeros(p);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDivAndRoundUp(KHE_PARTITION p, int i)                   */
/*                                                                           */
/*  Like KhePartitionDivAndRound, only rounding up.                          */
/*                                                                           */
/*****************************************************************************/

void KhePartitionDivAndRoundUp(KHE_PARTITION p, int i)
{
  int j;  short el;
  HnAssert(i > 0, "KhePartitionDivAndRoundUp: i is 0 or negative");
  HaArrayForEach(p->elems, el, j)
    HaArrayPut(p->elems, j, (el + i - 1) / i);
  KhePartitionStripTrailingZeros(p);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionReduce(KHE_PARTITION p, int i)                          */
/*                                                                           */
/*  Reduce p by repeatedly removing its smallest element, until its          */
/*  total size is at most i.                                                 */
/*                                                                           */
/*****************************************************************************/

void KhePartitionReduce(KHE_PARTITION p, int i)
{
  HnAssert(i >= 0, "KhePartitionReduce: i is negative");
  while( KhePartitionSize(p) > i )
    KhePartitionSub(p, KhePartitionMin(p));
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSum(KHE_PARTITION p1, KHE_PARTITION p2)                 */
/*                                                                           */
/*  Set *p1 to the set union p1 + p2, e.g.                                   */
/*                                                                           */
/*     <6: 2 2 1 1> + <7: 5 1 1> = <13: 5 2 2 1 1 1 1>                       */
/*                                                                           */
/*****************************************************************************/

void KhePartitionSum(KHE_PARTITION p1, KHE_PARTITION p2)
{
  /* add each element of p2 to p1, largest first to minimize reallocation */
  int i, j;  short el;
  HaArrayForEachReverse(p2->elems, el, i)
    for( j = 1;  j <= el;  j++ )
      KhePartitionAdd(p1, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionAssign(KHE_PARTITION p1, KHE_PARTITION p2)              */
/*                                                                           */
/*  Replace the existing value of p1 with the value of p2.                   */
/*                                                                           */
/*****************************************************************************/

void KhePartitionAssign(KHE_PARTITION p1, KHE_PARTITION p2)
{
  int i;
  HaArrayClear(p1->elems);
  HaArrayAppend(p1->elems, p2->elems, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDifference(KHE_PARTITION p1, KHE_PARTITION p2)          */
/*                                                                           */
/*  Update p1 to contain the set difference p1 - p2, e.g.                    */
/*                                                                           */
/*     <6: 2 2 1 1> - <7: 5 1 1> = <4: 2 2>                                  */
/*                                                                           */
/*****************************************************************************/

void KhePartitionDifference(KHE_PARTITION p1, KHE_PARTITION p2)
{
  int i;
  for( i = 0; i < HaArrayCount(p1->elems) && i < HaArrayCount(p2->elems); i++ )
    HaArrayPut(p1->elems, i,
      max(HaArray(p1->elems, i) - HaArray(p2->elems, i), 0));
  KhePartitionStripTrailingZeros(p1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionMaxToLimit(KHE_PARTITION p1, KHE_PARTITION p2,int limit)*/
/*                                                                           */
/*  Replace p1 by max(p1, p2) in which each multiplicity is maxed, but       */
/*  do not exceed limit as the total size of p1.                             */
/*                                                                           */
/*****************************************************************************/

void KhePartitionMaxToLimit(KHE_PARTITION p1, KHE_PARTITION p2, int limit)
{
  int p1_size, i, j, extra;
  p1_size = KhePartitionSize(p1);
  for( i = HaArrayCount(p2->elems);  i >= 1;  i-- )
  {
    /* find out how many extra i's p2 has over p1 */
    if( i >= HaArrayCount(p1->elems) )
      extra = HaArray(p2->elems, i);
    else
      extra = max(HaArray(p2->elems, i) - HaArray(p1->elems, i), 0);

    /* add as many of these extra i's as will fit into limit */
    for( j = 0;  j < extra && p1_size + i <= limit;  j++ )
    {
      KhePartitionAdd(p1, i);
      p1_size += i;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBuffInit(char **buff, int *curr_len, int *max_len)               */
/*  void KheBuffAddChar(char **buff, int *curr_len, int *max_len, char ch)   */
/*  void KheBuffAddStr(char **buff, int *curr_len, int *max_len, char *str)  */
/*                                                                           */
/*  Micro-module for building arbitrary length strings.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheBuffInit(char **buff, int *curr_len, int *max_len)
{
  (*buff) = NULL;
  (*curr_len) = 0;
  (*max_len) = 0;
}

static void KheBuffAddChar(char **buff, int *curr_len, int *max_len, char ch)
{
  if( curr_len >= max_len )
  {
    *max_len = *max_len + 16;
    *buff = reallo c(*buff, *max_len);
  }
  (*buff)[(*curr_len)++] = ch;
}

static void KheBuffAddStr(char **buff, int *curr_len, int *max_len, char *str)
{
  char *p;
  for( p = str;  *p != '\0';  p++ )
    KheBuffAddChar(buff, curr_len, max_len, *p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KhePartitionShow(KHE_PARTITION p)                                  */
/*                                                                           */
/*  Return a string representation of p in malloced memory (useful for       */
/*  debugging only).                                                         */
/*                                                                           */
/*****************************************************************************/
/* #define end_buff &buff[strlen(buff)] */

static char *show(KHE_PARTITION p, bool parens, bool none, bool brief,
  HA_ARENA a)
{
  int i, j;  bool first;
  short el; HA_ARRAY_NCHAR str;
  if( KhePartitionIsEmpty(p) )
  {
    if( parens )
      return "()";
    else if( none )
      return "None";
    else
      return "";
  }
  else
  {
    HnStringBegin(str, a);
    first = true;
    if( parens )
      HnStringAdd(&str, "(");
    HaArrayForEachReverse(p->elems, el, i)
      if( brief )
      {
	if( el == 0 )
	{
	  /* print nothing */
	}
	else if( el == 1 )
	{
	  /* print i */
	  if( !first )
	    HnStringAdd(&str, " ");
	  HnStringAdd(&str, "%d", i);
	  first = false;
	}
	else
	{
	  /* print num x i */
	  if( !first )
	    HnStringAdd(&str, " ");
          HnStringAdd(&str, "%dx%d", HaArray(p->elems, i), i);
	  first = false;
	}
      }
      else
      {
	for( j = 1;  j <= HaArray(p->elems, i);  j++ )
	{
	  if( !first )
	    HnStringAdd(&str, " ");
	  HnStringAdd(&str, "%d", i);
	  first = false;
	}
      }
    if( parens )
      HnStringAdd(&str, ")");
    return HnStringEnd(str);
  }
}

char *KhePartitionShow(KHE_PARTITION p, HA_ARENA a)
{
  return show(p, true, false, false, a);
}

char *KhePartitionShowRaw(KHE_PARTITION p, HA_ARENA a)
{
  return show(p, false, true, false, a);
}

char *KhePartitionShowBrief(KHE_PARTITION p, HA_ARENA a)
{
  return show(p, false, true, true, a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDebug(KHE_PARTITION p, FILE *fp)                        */
/*                                                                           */
/*  Debug print of p onto fp with no leading or trailing space.              */
/*                                                                           */
/*****************************************************************************/

void KhePartitionDebug(KHE_PARTITION p, FILE *fp)
{
  int i;  short el;  bool first;
  HnAssert(HaArrayCount(p->elems) != 1, "KhePartitionDebug internal error");
  fprintf(fp, "(");
  first = true;
  HaArrayForEachReverse(p->elems, el, i)
  {
    if( el == 0 )
    {
      /* print nothing */
    }
    else if( el == 1 )
    {
      /* print i */
      fprintf(fp, "%s%d", first ? "" : " ", i);
      first = false;
    }
    else
    {
      /* print num x i */
      fprintf(fp, "%s%dx%d", first ? "" : " ", el, i);
      first = false;
    }
  }
  fprintf(fp, ")");
}


/*****************************************************************************/
/*                                                                           */
/*  IMPLEMENTATION-INDEPENDENT CODE                                          */
/*                                                                           */
/*  The code in this section is implementation-independent, because it does  */
/*  not access the representation directly, it merely calls on the previous  */
/*  implementation-specific code.                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionFromString(char *str, int max_el, KHE_PARTITION *p,     */
/*                                                                           */
/*  If str contains a legal partition (just the numbers, e.g. "3 2 2")       */
/*  none of whose elements exceed max_el, then return true and set *p to     */
/*  the indicated partition.  Else return false and set *diagnosis to an     */
/*  explanation of the problem.                                              */
/*                                                                           */
/*  The string may also have value "" or "None", indicating the empty        */
/*  partition.                                                               */
/*                                                                           */
/*****************************************************************************/
#define is_digit(ch) ((ch) >= '0' && (ch) <= '9')

bool KhePartitionFromString(char *str, int max_el, KHE_PARTITION *p,
  char **diagnosis, HA_ARENA a)
{
  bool in_num = false;  int i, len, curr_num;  unsigned ch;
  *p = KhePartitionMake(a);
  if( strcmp(str, "") == 0 || strcmp(str, "None") == 0 )
    return true;
  len = strlen(str);
  curr_num = 0;  /* keep compiler happy */
  for( i = 0;  i < len;  i++ )
  {
    ch = str[i];
    if( in_num )
    {
      if( is_digit(ch) )
	curr_num = curr_num * 10 + (ch - '0');
      else if( ch == ' ' )
      {
	if( curr_num == 0 )
	{
	  *diagnosis = "partition string contains 0";
	  return false;
	}
	if( curr_num > max_el )
	{
	  *diagnosis = "partition string contains an oversize number";
	  return false;
	}
	KhePartitionAdd(*p, curr_num);
	in_num = false;
      }
      else
      {
	*diagnosis = "partition string contains an unexpected character";
	return false;
      }
    }
    else
    {
      if( is_digit(ch) )
      {
	curr_num = (ch - '0');
	in_num = true;
      }
      else if( ch == ' ' )
	/* do nothing */ ;
      else
      {
	*diagnosis = "partition string contains an unexpected character";
	return false;
      }
    }
  }
  if( in_num )
  {
    if( curr_num == 0 )
    {
      *diagnosis = "partition string contains 0";
      return false;
    }
    if( curr_num > max_el )
    {
      *diagnosis = "partition string contains an oversize number";
      return false;
    }
    KhePartitionAdd(*p, curr_num);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionListEqual(ARRAY_KHE_PARTITION *sap,                     */
/*    ARRAY_KHE_PARTITION *iap)                                              */
/*                                                                           */
/*  Return true if these lists of partitions are equal.                      */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionListEqual(ARRAY_KHE_PARTITION *sap, ARRAY_KHE_PARTITION *iap)
{
  int i;  KHE_PARTITION sp, ip;
  if( HaArrayCount(*sap) != HaArrayCount(*iap) )
    return false;
  for( i = 0;  i < HaArrayCount(*sap);  i++ )
  {
    sp = HaArray(*sap, i);
    ip = HaArray(*iap, i);
    if( !KhePartitionEqual(sp, ip) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionFromString(char *str)                          */
/*                                                                           */
/*  Convert a string representation of a partition, such as "<5: 3 2>",      */
/*  into the partition represented.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_PARTITION KhePartitionFromString(char *str)
{
  KHE_PARTITION res;
  int i, curr_num, total_num;
  enum { INIT, TOTAL1, TOTAL2, SKIP, INPART, DONE } state;
  state = INIT;
  res = KhePartitionMake();
  for( i = 0;  str[i] != '\0';  i++ ) switch( state )
  {
    case INIT:

      if( str[i] != '<' )
	MAbort("missing '<' in partition \"%s\"", str);
      state = TOTAL1;
      break;


    case TOTAL1:

      if( !is_digit(str[i]) )
	MAbort("no total in partition \"%s\"", str);
      curr_num = str[i] - '0';
      state = TOTAL2;
      break;


    case TOTAL2:

      if( is_digit(str[i]) )
	curr_num = curr_num * 10 + (str[i] - '0');
      else if( str[i] == ':' )
      {
	total_num = curr_num;
	state = SKIP;
      }
      else if( str[i] == '>' )
      {
	total_num = curr_num;
	state = DONE;
      }
      else
	MAbort("missing colon in partition \"%s\"", str);
      break;


    case SKIP:

      if( is_digit(str[i]) )
      {
	curr_num = str[i] - '0';
	state = INPART;
      }
      else if( str[i] == ' ' )
      {
	** do nothing, skipping white space **
      }
      else
	MAbort("missing number in partition \"%s\"", str);
      break;


    case INPART:

      if( is_digit(str[i]) )
	curr_num = curr_num * 10 + (str[i] - '0');
      else if( str[i] == ' ' )
      {
	KhePartitionAdd(res, curr_num);
	state = SKIP;
      }
      else if( str[i] == '>' )
      {
	KhePartitionAdd(res, curr_num);
	state = DONE;
      }
      else
	MAbort("format error after number in partition \"%s\"", str);
      break;


    case DONE:

      MAbort("character follows > in partition \"%s\"", str);
      break;
  }
  if( state != DONE )
    MAbort("missing > in partition \"%s\"", str);
  if( KhePartitionSize(res) != total_num )
    MAbort("inconsistent total in partition \"%s\" (should be %d)\n",
      str, KhePartitionSize(res));
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionUnitary(int width, HA_ARENA a)                 */
/*                                                                           */
/*  Return the unitary partition (all 1's) of this width.                    */
/*                                                                           */
/*****************************************************************************/

KHE_PARTITION KhePartitionUnitary(int width, HA_ARENA a)
{
  KHE_PARTITION res;  int i;
  res = KhePartitionMake(a);
  for( i = 0;  i < width;  i++ )
    KhePartitionAdd(res, 1);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionBuild(KHE_PARTITION stem, int n, int m,                 */
/*    ARRAY_KHE_PARTITION *ap, HA_ARENA a)                                   */
/*                                                                           */
/*  Add to ap the sum of stem with each partitition of n whose maximum       */
/*  part is at most m.  Here n >= 0 and m >= 1.                              */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionBuild(KHE_PARTITION stem, int n, int m,
  ARRAY_KHE_PARTITION *ap, HA_ARENA a)
{
  HnAssert(n >= 0, "KhePartitionBuild: n out of range");
  HnAssert(m >= 1, "KhePartitionBuild: m out of range");
  if( n == 0 )
    HaArrayAddLast(*ap, KhePartitionCopy(stem, a));
  else
  {
    /* partitions whose maximum part is equal to m */
    if( n >= m )
    {
      KhePartitionAdd(stem, m);
      KhePartitionBuild(stem, n - m, m, ap, a);
      KhePartitionSub(stem, m);
    }

    /* partitions whose maximum part is less than m */
    if( m > 1 )
      KhePartitionBuild(stem, n, m - 1, ap, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionsOf(int n, int m, ARRAY_KHE_PARTITION *res, HA_ARENA a) */
/*                                                                           */
/*  Add to *res all the partitions of n whose maximum part is at most m.     */
/*                                                                           */
/*****************************************************************************/

void KhePartitionsOf(int n, int m, ARRAY_KHE_PARTITION *res, HA_ARENA a)
{
  KHE_PARTITION stem;
  stem = KhePartitionMake(a);
  KhePartitionBuild(stem, n, m, res, a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionBuildUpTo(KHE_PARTITION stem, int n, int m,             */
/*    ARRAY_KHE_PARTITION ap)                                                */
/*                                                                           */
/*  Add to ap the sum of stem with each partitition of size less than or     */
/*  equal to n whose maximum part is at most m.  Here n >= 0 and m >= 0.     */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionBuildUpTo(KHE_PARTITION stem, int n, int m,
  ARRAY_KHE_PARTITION *ap, HA_ARENA a)
{
  HnAssert(n >= 0, "KhePartitionBuild: n is negative");
  HnAssert(m >= 0, "KhePartitionBuild: m is negative");
  if( m == 0 )
    HaArrayAddLast(*ap, KhePartitionCopy(stem, a));
  else
  {
    /* partitions whose maximum part is equal to m > 0 */
    if( n >= m )
    {
      KhePartitionAdd(stem, m);
      KhePartitionBuildUpTo(stem, n - m, m, ap, a);
      KhePartitionSub(stem, m);
    }

    /* partitions whose maximum part is less than m > 0 */
    KhePartitionBuildUpTo(stem, n, m - 1, ap, a);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  ARRAY_KHE_PARTITION KhePartitionsUpTo(int n, int m)                      */
/*                                                                           */
/*  Add to *res all partitions of size less than or equal to n whose         */
/*  maximum part is at most m.                                               */
/*                                                                           */
/*****************************************************************************/

void KhePartitionsUpTo(int n, int m, ARRAY_KHE_PARTITION *res, HA_ARENA a)
{
  KHE_PARTITION stem;
  stem = KhePartitionMake(a);
  KhePartitionBuildUpTo(stem, n, m, res, a);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionBinPack(KHE_PARTITION p1, KHE_PARTITION p2)             */
/*                                                                           */
/*  Return true if p1 is packable into p2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionDoBinPack(KHE_PARTITION p1, KHE_PARTITION p2)
{
  bool res;  int m, i;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KhePartitionDoBinPack(");
    KhePartitionDebug(p1, stderr);
    fprintf(stderr, ", ");
    KhePartitionDebug(p2, stderr);
    fprintf(stderr, ")\n");
  }

  if( KhePartitionIsEmpty(p1) )
    res = true;
  else if( KhePartitionIsEmpty(p2) )
    res = false;
  else if( KhePartitionMax(p1) > KhePartitionMax(p2) )
    res = false;
  else if( KhePartitionMax(p1) == 1 )
    res = true;
  else if( KhePartitionContains(p2, KhePartitionMax(p1)) )
  {
    m = KhePartitionMax(p1);
    KhePartitionSub(p1, m);
    KhePartitionSub(p2, m);
    res = KhePartitionDoBinPack(p1, p2);
    KhePartitionAdd(p2, m);
    KhePartitionAdd(p1, m);
  }
  else /* KhePartitionMax(p1) < KhePartitionMax(p2) */
  {
    m = KhePartitionMax(p1);
    KhePartitionSub(p1, m);
    res = false;
    for( i = m + 1;  !res && i <= KhePartitionMax(p2);  i++ )
    {
      if( KhePartitionContains(p2, i) )
      {
	KhePartitionSub(p2, i);
	KhePartitionAdd(p2, i - m);
	res = KhePartitionDoBinPack(p1, p2);
	KhePartitionSub(p2, i - m);
	KhePartitionAdd(p2, i);
      }
    }
    KhePartitionAdd(p1, m);
  }
  if( DEBUG4 )
    fprintf(stderr, "] KhePartitionDoBinPack returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionBinPack(KHE_PARTITION p1, KHE_PARTITION p2)             */
/*                                                                           */
/*  Return true if p1 is packable into p2.                                   */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionBinPack(KHE_PARTITION p1, KHE_PARTITION p2)
{
  bool res;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KhePartitionBinPack(");
    KhePartitionDebug(p1, stderr);
    fprintf(stderr, ", ");
    KhePartitionDebug(p2, stderr);
    fprintf(stderr, ")\n");
  }

  res = KhePartitionSize(p1) <= KhePartitionSize(p2) &&
    KhePartitionDoBinPack(p1, p2);
  if( DEBUG4 )
    fprintf(stderr, "] KhePartitionBinPack returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_BIN - bins for bin packing                                           */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_bin_rec *KHE_BIN;
typedef HA_ARRAY(KHE_BIN) ARRAY_KHE_BIN;

struct khe_bin_rec {
  int			remaining_capacity;
  KHE_PARTITION		partition;
};


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionDoBinPackAndHow(KHE_PARTITION p1, ARRAY_KHE_BIN *bins)  */
/*                                                                           */
/*  Do the real work of KhePartitionBinPackAndHow: pack whatever is left     */
/*  of p1 into the remaining capacity of *bins, returning true on success.   */
/*                                                                           */
/*****************************************************************************/

static bool KhePartitionDoBinPackAndHow(KHE_PARTITION p1, ARRAY_KHE_BIN *bins)
{
  bool res;  int m, i, count;  KHE_BIN bin;

  if( KhePartitionIsEmpty(p1) )
    return true;
  else
  {
    m = KhePartitionMax(p1);
    if( m == 1 )
    {
      /* special case: just drop all remaining items into any bin */
      count = KhePartitionPartsWithSize(p1, m);
      HaArrayForEach(*bins, bin, i)
	while( bin->remaining_capacity >= 1 && count > 0 )
	{
	  KhePartitionAdd(bin->partition, m);
	  bin->remaining_capacity -= m;
	  count--;
	}
      HnAssert(count == 0, "KhePartitionDoBinPackAndHow internal error");
      return true;
    }
    else
    {
      KhePartitionSub(p1, m);
      HaArrayForEach(*bins, bin, i)
	if( bin->remaining_capacity == m )
	  break;
      if( i < HaArrayCount(*bins) )
      {
	/* m packs exactly into bin */
	KhePartitionAdd(bin->partition, m);
	bin->remaining_capacity -= m;
	res = KhePartitionDoBinPackAndHow(p1, bins);
	if( !res )
	{
	  KhePartitionSub(bin->partition, m);
	  bin->remaining_capacity += m;
	}
      }
      else
      {
	/* m does not pack exactly into any bin */
	res = false;
	HaArrayForEach(*bins, bin, i)
	  if( !res && bin->remaining_capacity > m )
	  {
	    KhePartitionAdd(bin->partition, m);
	    bin->remaining_capacity -= m;
	    res = KhePartitionDoBinPackAndHow(p1, bins);
	    if( !res )
	    {
	      KhePartitionSub(bin->partition, m);
	      bin->remaining_capacity += m;
	    }
	  }
      }
    }
    KhePartitionAdd(p1, m);
    return res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionBinPackAndHow(KHE_PARTITION p1, KHE_PARTITION p2,       */
/*    ARRAY_KHE_PARTITION *partitions)                                       */
/*                                                                           */
/*  Like KhePartitionBinPack except that if true is returned, *partitions    */
/*  contains one element for each part of p2, in decreasing order.  Each     */
/*  element contains some of the parts of p1, the ones that get packed       */
/*  into that element of p2.                                                 */
/*                                                                           */
/*****************************************************************************/

bool KhePartitionBinPackAndHow(KHE_PARTITION p1, KHE_PARTITION p2,
  ARRAY_KHE_PARTITION *partitions, HA_ARENA a)
{
  int i, j;  ARRAY_KHE_BIN bins;  KHE_BIN bin;  bool res;

  /* fail now if total size is wrong */
  if( KhePartitionSize(p1) > KhePartitionSize(p2) )
    return false;

  /* initialize bins representing p2 */
  HaArrayInit(bins, a);
  for( i = KhePartitionMax(p2);  i >= 1;  i-- )
  {
    for( j = 0;  j < KhePartitionPartsWithSize(p2, i);  j++ )
    {
      HaMake(bin, a);
      bin->remaining_capacity = i;
      bin->partition = KhePartitionMake(a);
      HaArrayAddLast(bins, bin);
    }
  }

  /* do the pack and set *partitions if successful */
  res = KhePartitionDoBinPackAndHow(p1, &bins);
  if( res )
  {
    HaArrayClear(*partitions);
    HaArrayForEach(bins, bin, i)
      HaArrayAddLast(*partitions, bin->partition);
  }
  /* *** old version that freed memory
  HaArrayForEach(bins, bin, i)
  {
    if( res )
      HaArrayAddLast(*partitions, bin->partition);
    else
    {
      KhePartitionFree(bin->partition);
    }
    ** HaFree(bin); still to do here **
  }
  HaArrayFree(bins);
  *** */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionMake4(int p1, int p2, int p3, int p4)          */
/*                                                                           */
/*  Make a partition with p1 ones, p2 twos, p3 threes, and p4 fours.         */
/*                                                                           */
/*****************************************************************************/

static KHE_PARTITION KhePartitionMake4(int p1, int p2, int p3, int p4,
  HA_ARENA a)
{
  int i;  KHE_PARTITION res;
  res = KhePartitionMake(a);
  for( i = 0;  i < p1;  i++ )
    KhePartitionAdd(res, 1);
  for( i = 0;  i < p2;  i++ )
    KhePartitionAdd(res, 2);
  for( i = 0;  i < p3;  i++ )
    KhePartitionAdd(res, 3);
  for( i = 0;  i < p4;  i++ )
    KhePartitionAdd(res, 4);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionBinPackTest(KHE_PARTITION p1, KHE_PARTITION p2,         */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Test KhePartitionBinPack(p1, p2) and print the results on fp.            */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionBinPackTest(KHE_PARTITION p1, KHE_PARTITION p2,
  FILE *fp, HA_ARENA a)
{
  bool res;  ARRAY_KHE_PARTITION partitions;  KHE_PARTITION p;  int i;
  HaArrayInit(partitions, a);
  fprintf(fp, "KhePartitionBinPackAndHow(%s, %s)", KhePartitionShowBrief(p1, a),
    KhePartitionShowBrief(p2, a));
  res = KhePartitionBinPackAndHow(p1, p2, &partitions, a);
  if( res )
  {
    fprintf(fp, " = true:");
    HaArrayForEach(partitions, p, i)
      fprintf(fp, " %s", KhePartitionShowBrief(p, a));
    fprintf(fp, "\n");
  }
  else
    fprintf(fp, " = false\n");
}


/* ***
static void KhePartitionBinPackTest(KHE_PARTITION p1, KHE_PARTITION p2,
  FILE *fp)
{
  bool res;
  fprintf(fp, "KhePartitionBinPack(%s, %s)", KhePartitionShow(p1),
    KhePartitionShow(p2));
  res = KhePartitionBinPack(p1, p2);
  fprintf(fp, " = %s\n", bool(res));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionTest2(FILE *fp)                                         */
/*                                                                           */
/*  Test the use of partitions as indexes for marrays.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePartitionTest2(FILE *fp)
{
  KHE_PARTITION items1 = KhePartitionMake4(0, 3, 0, 0);
  KHE_PARTITION items2 = KhePartitionMake4(25, 6, 1, 0);
  KHE_PARTITION items3 = KhePartitionMake4(50, 12, 2, 0);
  KHE_PARTITION items4 = KhePartitionMake4(100, 24, 4, 0);
  KHE_PARTITION items5 = KhePartitionMake4(200, 49, 8, 0);

  KHE_PARTITION bins1 = KhePartitionMake4(0, 3, 0, 0);
  KHE_PARTITION bins2 = KhePartitionMake4(0, 0, 2, 0);
  KHE_PARTITION bins3 = KhePartitionMake4(0, 9, 2, 4);
  KHE_PARTITION bins4 = KhePartitionMake4(0, 18, 4, 8);
  KHE_PARTITION bins5 = KhePartitionMake4(0, 36, 8, 16);
  KHE_PARTITION bins6 = KhePartitionMake4(0, 72, 16, 32);

  KhePartitionBinPackTest(items1, bins1, fp);
  KhePartitionBinPackTest(items1, bins2, fp);
  KhePartitionBinPackTest(items2, bins3, fp);
  KhePartitionBinPackTest(items3, bins4, fp);
  KhePartitionBinPackTest(items4, bins5, fp);
  KhePartitionBinPackTest(items5, bins6, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionTest(FILE *fp)                                          */
/*                                                                           */
/*  Test this module, writing results to fp.                                 */
/*                                                                           */
/*****************************************************************************/

void KhePartitionTest(FILE *fp)
{
  HA_ARENA_SET as;  HA_ARENA a;
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used for testing only */
  KHE_PARTITION items1 = KhePartitionMake4(0, 3, 0, 0, a);
  KHE_PARTITION items2 = KhePartitionMake4(25, 6, 1, 0, a);
  KHE_PARTITION items3 = KhePartitionMake4(50, 12, 2, 0, a);
  KHE_PARTITION items4 = KhePartitionMake4(100, 24, 4, 0, a);
  KHE_PARTITION items5 = KhePartitionMake4(200, 49, 8, 0, a);

  KHE_PARTITION bins1 = KhePartitionMake4(0, 3, 0, 0, a);
  KHE_PARTITION bins2 = KhePartitionMake4(0, 0, 2, 0, a);
  KHE_PARTITION bins3 = KhePartitionMake4(0, 9, 2, 4, a);
  KHE_PARTITION bins4 = KhePartitionMake4(0, 18, 4, 8, a);
  KHE_PARTITION bins5 = KhePartitionMake4(0, 36, 8, 16, a);
  KHE_PARTITION bins6 = KhePartitionMake4(0, 72, 16, 32, a);

  KhePartitionBinPackTest(items1, bins1, fp, a);
  KhePartitionBinPackTest(items1, bins2, fp, a);
  KhePartitionBinPackTest(items2, bins3, fp, a);
  KhePartitionBinPackTest(items3, bins4, fp, a);
  KhePartitionBinPackTest(items4, bins5, fp, a);
  KhePartitionBinPackTest(items5, bins6, fp, a);
  HaArenaDelete(a);  /* this arena is ised for testing only */
}


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
/*  FILE:         khe_set.h                                                  */
/*  DESCRIPTION:  Sorted sets of integers                                    */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_SET_HEADER_FILE
#define KHE_SET_HEADER_FILE

#include <stdbool.h>
#include <stdio.h>
#include "howard_a.h"

typedef struct khe_set_rec {
  HA_ARRAY_INT	ss_array;		/* underlying array        */
} KHE_SET;

typedef struct khe_set_table_rec *KHE_SET_TABLE;

/*****************************************************************************/
/*                                                                           */
/*  Macros                                                                   */
/*                                                                           */
/*****************************************************************************/

/* construction */
#define KheSetInit(s, a) KheSetImplInit(&(s), a)
#define KheSetCopy(to_s, from_s, a) KheSetImplCopy(&(to_s), &(from_s), a)
#define KheSetCopyElements(to_s, from_s)	\
  KheSetImplCopyElements(&(to_s), &(from_s))
#define KheSetClear(s) KheSetImplClear(&(s))
#define KheSetInsert(s, item) KheSetImplInsert(&(s), item)
#define KheSetDelete(s, item) KheSetImplDelete(&(s), item)
#define KheSetDeleteLast(s) HaArrayDeleteLast((s).ss_array)
#define KheSetUnion(to_s, from_s) KheSetImplUnion(&(to_s), &(from_s))
#define KheSetIntersect(to_s, from_s) KheSetImplIntersect(&(to_s), &(from_s))
#define KheSetDifference(to_s, from_s) KheSetImplDifference(&(to_s), &(from_s))

#define KheSetUnionCount(to_s, from_s) KheSetImplUnionCount(&(to_s), &(from_s))
#define KheSetIntersectCount(to_s, from_s) \
  KheSetImplIntersectCount(&(to_s), &(from_s))
#define KheSetDifferenceCount(to_s, from_s) \
  KheSetImplDifferenceCount(&(to_s), &(from_s))
#define KheSetSymmetricDifferenceCount(to_s, from_s) \
  KheSetImplSymmetricDifferenceCount(&(to_s), &(from_s))

/* query */
#define KheSetCount(s) HaArrayCount((s).ss_array)
#define KheSetGet(s, i) HaArray((s).ss_array, (i))
#define KheSetGetLast(s) HaArray((s).ss_array, KheSetCount(s) - 1)
#define KheSetEmpty(s) (KheSetCount(s) == 0)
#define KheSetEqual(s1, s2)						\
  (KheSetCount(s1) == KheSetCount(s2) && KheSetImplEqual(&(s1), &(s2)))
#define KheSetSubset(s1, s2)						\
  (KheSetCount(s1) <= KheSetCount(s2) && KheSetImplSubset(&(s1), &(s2)))
#define KheSetDisjoint(s1, s2) KheSetImplDisjoint(&(s1), &(s2))
#define KheSetTypedCmp(s1, s2) KheSetImplTypedCmp(&(s1), &(s2))
#define KheSetContains(s, item) KheSetImplContains(&(s), item)
#define KheSetMin(s) KheSetGet((s), 0)
#define KheSetMax(s) KheSetGet((s), KheSetCount(s) - 1)

/* iterators */
#define KheSetForEach(s, t, i)						\
  for( (i) = 0;								\
       (i) < KheSetCount(s) ? ((t) = KheSetGet((s),(i)), true) : false;	\
       (i)++ )

#define KheSetForEachReverse(s, t, i)					\
  for( (i) = KheSetCount(s) - 1;					\
       (i) >= 0 ? ((t) = KheSetGet((s), (i)), true) : false;		\
       (i)-- )

/* tables indexed by KHE Sets */
#define KheSetTableInsert(st, s, val) KheSetImplTableInsert(st, &(s), val)
#define KheSetTableRetrieve(st, s, val) KheSetImplTableRetrieve(st, &(s), val)

/* debug */
#define KheSetShow(s, buff) KheSetImplShow(&(s), buff)


/*****************************************************************************/
/*                                                                           */
/*  Functions                                                                */
/*                                                                           */
/*****************************************************************************/

/* construction */
extern void KheSetImplInit(KHE_SET *sp, HA_ARENA a);
extern void KheSetImplCopy(KHE_SET *to_sp, KHE_SET *from_sp, HA_ARENA a);
extern void KheSetImplCopyElements(KHE_SET *to_sp, KHE_SET *from_sp);
extern void KheSetImplClear(KHE_SET *sp);
extern void KheSetImplInsert(KHE_SET *sp, int item);
extern void KheSetImplDelete(KHE_SET *sp, int item);
extern void KheSetImplUnion(KHE_SET *to_sp, KHE_SET *from_sp);
extern void KheSetImplIntersect(KHE_SET *to_sp, KHE_SET *from_sp);
extern void KheSetImplDifference(KHE_SET *to_sp, KHE_SET *from_sp);

/* counts */
extern int KheSetImplUnionCount(KHE_SET *to_sp, KHE_SET *from_sp);
extern int KheSetImplIntersectCount(KHE_SET *to_sp, KHE_SET *from_sp);
extern int KheSetImplDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp);
extern int KheSetImplSymmetricDifferenceCount(KHE_SET *to_sp, KHE_SET *from_sp);

/* query */
extern bool KheSetImplEqual(KHE_SET *sp1, KHE_SET *sp2);
extern bool KheSetImplSubset(KHE_SET *sp1, KHE_SET *sp2);
extern bool KheSetImplDisjoint(KHE_SET *sp1, KHE_SET *sp2);
extern int  KheSetImplTypedCmp(KHE_SET *sp1, KHE_SET *sp2);
extern bool KheSetImplContains(KHE_SET *sp, int item);

/* debug */
extern char *KheSetImplShow(KHE_SET *sp, char buff[200]);
extern void KheSetTest(FILE *fp);

/* tables indexed by KheSets */
extern KHE_SET_TABLE KheSetTableMake(HA_ARENA a);
extern void KheSetImplTableInsert(KHE_SET_TABLE st, KHE_SET *sp, void *val);
extern bool KheSetImplTableRetrieve(KHE_SET_TABLE st, KHE_SET *sp, void **val);
extern void KheSetTableDebug(KHE_SET_TABLE st, int indent, FILE *fp);
extern void KheSetTableTest(FILE *fp);

#endif

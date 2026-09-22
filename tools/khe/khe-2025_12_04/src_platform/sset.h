
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
/*  FILE:         sset.h                                                     */
/*  DESCRIPTION:  Small sets with operations needed for sets of times        */
/*                                                                           */
/*****************************************************************************/
#ifndef SSET_HEADER_FILE
#define SSET_HEADER_FILE

#include <stdbool.h>
#include <stdio.h>
#include "howard_a.h"


typedef struct sset_rec {
  HA_ARRAY_INT	ss_array;		/* underlying array        */
  int		ss_shift;		/* add to each value       */
  /* bool	ss_slice; */		/* is slice of other sset  */
  bool		ss_finalized;		/* finalized               */
} SSET;

typedef struct sset_table_rec *SSET_TABLE;


/*****************************************************************************/
/*                                                                           */
/*  Macros                                                                   */
/*                                                                           */
/*****************************************************************************/

/* construction */
#define SSetInit(ss, a) SSetImplInit(&(ss), a)
#define SSetCopy(to_ss, from_ss, a) SSetImplCopy(&(to_ss), &(from_ss), a)
#define SSetClear(ss) SSetImplClear(&(ss))
#define SSetInsert(ss, item) SSetImplInsert(&(ss), item)
#define SSetDelete(ss, item) SSetImplDelete(&(ss), item)
#define SSetDeleteLast(ss) HaArrayDeleteLast((ss).ss_array)
#define SSetUnion(to_ss, from_ss) SSetImplUnion(&(to_ss), &(from_ss));
#define SSetIntersect(to_ss, from_ss) SSetImplIntersect(&(to_ss), &(from_ss));
#define SSetDifference(to_ss, from_ss) SSetImplDifference(&(to_ss), &(from_ss));
#define SSetFinalize(ss) SSetImplFinalize(&(ss));
#define SSetIsFinalized(ss) ((ss).ss_finalized)
#define SSetInitShifted(to_ss, from_ss, shift)		\
  SSetImplInitShifted(&(to_ss), &(from_ss), shift)
#define SSetInitShiftedAndSliced(to_ss, from_ss, shift, lower_lim, upper_lim) \
  SSetImplInitShiftedAndSliced(&(to_ss), &(from_ss), shift,lower_lim,upper_lim)
/* #define SSetFree(ss) SSetImplFree(&(ss)) */

/* query */
#define SSetCount(ss) HaArrayCount((ss).ss_array)
#define SSetGet(ss, i) (HaArray((ss).ss_array, (i)) + (ss).ss_shift)
#define SSetGetLast(ss) (HaArray((ss).ss_array, SSetCount(ss)-1)+(ss).ss_shift)
#define SSetEmpty(ss) (SSetCount(ss) == 0)
#define SSetEqual(ss1, ss2)						\
  (SSetCount(ss1) == SSetCount(ss2) && SSetImplEqual(&(ss1), &(ss2)))
#define SSetSubset(ss1, ss2)						\
  (SSetCount(ss1) <= SSetCount(ss2) && SSetImplSubset(&(ss1), &(ss2)))
#define SSetDisjoint(ss1, ss2) SSetImplDisjoint(&(ss1), &(ss2))
#define SSetTypedCmp(ss1, ss2) SSetImplTypedCmp(&(ss1), &(ss2))
#define SSetContains(ss, item, pos) SSetImplContains(&(ss), item, pos)
#define SSetMin(ss) SSetGet((ss), 0)
#define SSetMax(ss) SSetGet((ss), SSetCount(ss) - 1)
#define SSetShift(ss) ((ss).ss_shift)

/* iterators */
#define SSetForEach(ss, t, i)						\
  for( *(i) = 0;							\
       *(i) < SSetCount(ss) ? (*(t) = SSetGet((ss),*(i)),true) : false;	\
       (*(i))++ )

#define SSetForEachReverse(ss, t, i)					\
  for( *(i) = SSetCount(ss) - 1;					\
       *(i) >= 0 ? (*(t) = SSetGet((ss), *(i)), true) : false;		\
       (*(i))-- )

/* tables indexed by SSets */
#define SSetTableInsert(st, ss, val) SSetImplTableInsert(st, &(ss), val)
#define SSetTableRetrieve(st, ss, val) SSetImplTableRetrieve(st, &(ss), val)

/* debug */
#define SSetShow(ss) SSetImplShow(&(ss))


/*****************************************************************************/
/*                                                                           */
/*  Functions                                                                */
/*                                                                           */
/*****************************************************************************/

/* construction */
extern void SSetImplInit(SSET *sp, HA_ARENA a);
extern void SSetImplCopy(SSET *to_sp, SSET *from_sp, HA_ARENA a);
extern void SSetImplClear(SSET *sp);
extern void SSetImplInsert(SSET *sp, int item);
extern void SSetImplDelete(SSET *sp, int item);
extern void SSetImplUnion(SSET *to_sp, SSET *from_sp);
extern void SSetImplIntersect(SSET *to_sp, SSET *from_sp);
extern void SSetImplDifference(SSET *to_sp, SSET *from_sp);
extern void SSetImplFinalize(SSET *sp);
extern void SSetImplInitShifted(SSET *to_sp, SSET *from_sp, int shift);
extern void SSetImplInitShiftedAndSliced(SSET *to_sp, SSET *from_sp, int shift,
  int lower_limit, int upper_limit);
/* extern void SSetImplFree(SSET *sp); */

/* query */
extern bool SSetImplEqual(SSET *sp1, SSET *sp2);
extern bool SSetImplSubset(SSET *sp1, SSET *sp2);
extern bool SSetImplDisjoint(SSET *sp1, SSET *sp2);
extern int  SSetImplTypedCmp(SSET *sp1, SSET *sp2);
extern bool SSetImplContains(SSET *sp, int item, int *pos);

/* debug */
extern char *SSetImplShow(SSET *sp);
extern void SSetTest(FILE *fp);

/* tables indexed by SSets */
extern SSET_TABLE SSetTableMake(HA_ARENA a);
/* extern void SSetTableFree(SSET_TABLE st); */
extern void SSetImplTableInsert(SSET_TABLE st, SSET *sp, void *val);
extern bool SSetImplTableRetrieve(SSET_TABLE st, SSET *sp, void **val);
extern void SSetTableDebug(SSET_TABLE st, int indent, FILE *fp);
extern void SSetTableTest(FILE *fp);

#endif


/*****************************************************************************/
/*                                                                           */
/*  THE HOWARD OBJECT-ORIENTED COMPILER TOOLKIT                              */
/*  COPYRIGHT (C) 2011 Jeffrey H. Kingston                                   */
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
/*  along with this program; if not, write to Free Software Foundation       */
/*  Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA               */
/*                                                                           */
/*  FILE:     howard_a.h                                                     */
/*  PURPOSE:  header file for the Howard Arenas and Arrays Library           */
/*                                                                           */
/*****************************************************************************/

#ifndef HOWARD_A_HEADER
#define HOWARD_A_HEADER

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>


/*****************************************************************************/
/*                                                                           */
/*  Types; HA_ALIGN_TYPE is changeable to anything at least a pointer wide   */
/*                                                                           */
/*****************************************************************************/

typedef void *HA_ALIGN_TYPE;
typedef struct ha_arena_rec *HA_ARENA;
typedef struct ha_arena_set_rec *HA_ARENA_SET;


/*****************************************************************************/
/*                                                                           */
/*  Arenas                                                                   */
/*                                                                           */
/*****************************************************************************/

extern HA_ARENA HaArenaMake(HA_ARENA_SET as);
extern void HaArenaDelete(HA_ARENA a);

extern void *HaAlloc(HA_ARENA a, size_t size);
#define HaMake(res, a) ((res) = HaAlloc(a, sizeof(*(res))))

extern void *HaResizableAlloc(HA_ARENA a);
extern void *HaResizableReAlloc(void *resizable, size_t size);
extern void HaResizableFree(void *resizable);

extern HA_ARENA HaResizableArena(void *resizable);
extern size_t HaResizableSize(void *resizable);

extern void HaArenaCheck(HA_ARENA a);

extern void HaArenaDebug(HA_ARENA a, int verbosity, int indent, FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*  Arena sets                                                               */
/*                                                                           */
/*****************************************************************************/

extern HA_ARENA_SET HaArenaSetMake(void);
extern void HaArenaSetMerge(HA_ARENA_SET dest_as, HA_ARENA_SET src_as);
extern void HaArenaSetDelete(HA_ARENA_SET as);

extern void HaArenaSetJmpEnvBegin(HA_ARENA_SET as, jmp_buf *env);
extern void HaArenaSetJmpEnvEnd(HA_ARENA_SET as);
extern void HaArenaSetLimitMemory(HA_ARENA_SET as, size_t limit);

extern void HaArenaSetDebug(HA_ARENA_SET as, int verbosity, int indent,
  FILE *fp);


/*****************************************************************************/
/*                                                                           */
/*  Arrays                                                                   */
/*                                                                           */
/*****************************************************************************/

/* the HA_ARRAY macro; the struct fields should not be referred to directly */
#define HA_ARRAY(TYPE)							\
  struct {								\
    TYPE	*items;		/* items                             */	\
    int		length;		/* number of items storable in items */	\
    int		count;		/* number of items stored in items   */	\
  }

/* defining, initializing, and freeing */
typedef HA_ARRAY(bool)		HA_ARRAY_BOOL;
typedef HA_ARRAY(char)		HA_ARRAY_NCHAR;
typedef HA_ARRAY(wchar_t)	HA_ARRAY_CHAR;
typedef HA_ARRAY(short)		HA_ARRAY_SHORT;
typedef HA_ARRAY(int)		HA_ARRAY_INT;
typedef HA_ARRAY(int64_t)	HA_ARRAY_INT64;
typedef HA_ARRAY(void *)	HA_ARRAY_VOIDP;
typedef HA_ARRAY(char *)	HA_ARRAY_NSTRING;
typedef HA_ARRAY(wchar_t *)	HA_ARRAY_STRING;
typedef HA_ARRAY(float)		HA_ARRAY_FLOAT;
typedef HA_ARRAY(double)	HA_ARRAY_DOUBLE;

#define HaArrayInit(a, arena)						\
( (a).items = HaResizableAlloc(arena), (a).length = (a).count = 0 )

#define HaArrayArena(a) HaResizableArena((a).items)

#define HaArrayFree(a) HaResizableFree((a).items)

/* block moves */
#define HaArrayMove(a, d, s, len)					\
  memmove(&(a).items[d], &(a).items[s], (len) * sizeof((a).items[0]))	\

/* adding and replacing array elements */
#define HaArrayAdd(a, i, x)						\
(									\
  HaArrayAddLast((a), (x)),						\
  HaArrayMove((a), (i) + 1, (i), HaArrayCount(a) - (i) - 1),		\
  HaArrayPut((a), (i), (x))						\
)

#define HaArrayAddFirst(a, x)    HaArrayAdd((a), 0, (x))

#define HaArrayAddLast(a, x)						\
(									\
  (a).count < (a).length ? (a).items[(a).count++] = (x) :		\
  (									\
    (a).items = HaResizableReAlloc((a).items,				\
      (2 * (a).length + 1) * sizeof((a).items[0])),			\
    (a).length = HaResizableSize((a).items) / sizeof((a).items[0]),	\
    (a).items[(a).count++] = (x)					\
  )									\
)

#define HaArrayFill(a, len, x)						\
  while( HaArrayCount(a) < (len) ) HaArrayAddLast((a), (x))

#define HaArrayPut(a, i, x)     ((a).items[i] = (x))

/* accessing and searching for array elements */
#define HaArrayCount(a)	((a).count)
#define HaArray(a, i)	((a).items[i])
#define	HaArrayFirst(a)	((a).items[0])
#define	HaArrayLast(a)	((a).items[HaArrayCount(a) - 1])

/* *** yikes! no good in parallel
#define HaArrayContains(a, x, pos)					\
(									\
  HaArrayAddLast((a), (x)),						\
  HaArrayImplFind((char *) (a).items, sizeof((a).items[0]),		\
    HaArrayCount(a), (pos)),						\
  HaArrayDeleteLast(a),							\
  *(pos) < HaArrayCount(a)						\
)
*** */
#define HaArrayContains(a, t, pos)					\
  HaArrayImplFind((char *) (a).items, (char *) &(t),			\
    sizeof((a).items[0]), HaArrayCount(a), (pos))

/* shifting */
#define HaArrayShiftRight(a, n, x)					\
{									\
  int zxz_places_to_move, zxz_number_to_move, zxz_i;			\
  zxz_places_to_move = (n);						\
  zxz_number_to_move = HaArrayCount(a);					\
  for( zxz_i = 0;  zxz_i < zxz_places_to_move;  zxz_i++ )		\
    HaArrayAddLast((a), (x));						\
  HaArrayMove((a), zxz_places_to_move, 0, zxz_number_to_move);		\
  for( zxz_i = 0;  zxz_i < zxz_places_to_move;  zxz_i++ )		\
    HaArrayPut((a), zxz_i, (x));					\
}

#define HaArrayShiftLeft(a, n)						\
{									\
  int zxz_places_to_move, zxz_number_to_move;				\
  zxz_places_to_move = (n);						\
  zxz_number_to_move = HaArrayCount(a) - zxz_places_to_move;		\
  HaArrayMove((a), 0, zxz_places_to_move, zxz_number_to_move);		\
  (a).count -= zxz_places_to_move;					\
}

/* deleting elements */
#define HaArrayDeleteAndShift(a, i)					\
(									\
  HaArrayMove((a), (i), (i) + 1, (a).count - (i) - 1),			\
  (a).count--								\
)

#define HaArrayDeleteAndPlug(a, i)					\
(									\
  HaArrayPut((a), (i), HaArrayLast(a)),					\
  HaArrayDeleteLast(a)							\
)

#define HaArrayDeleteAndPlugIndexed(a, i, field)			\
(									\
  HaArrayLast(a)->field = (i),						\
  HaArrayPut((a), (i), HaArrayLast(a)),					\
  HaArrayDeleteLast(a)							\
)

#define HaArrayFindDeleteAndShift(a, x, pos)				\
(									\
  HaArrayContains((a), (x), (pos)) ?					\
  (HaArrayDeleteAndShift((a), (pos)), true) : false			\
)

#define HaArrayFindDeleteAndPlug(a, x, pos)				\
(									\
  HaArrayContains((a), (x), &(pos)) ?					\
  (HaArrayDeleteAndPlug((a), (pos)), true) : false			\
)

#define HaArrayDeleteLast(a)		((a).count--)
#define HaArrayDeleteLastSlice(a, n)	((a).count -= (n))
#define HaArrayClear(a)			((a).count = 0)
#define	HaArrayLastAndDelete(a)		((a).items[--((a).count)])

/* other update operations */
#define HaArraySwap(a, i, j, tmp)					\
(									\
  (tmp) = HaArray((a), (i)),						\
  HaArrayPut((a), (i), HaArray((a), (j))),				\
  HaArrayPut((a), (j), (tmp))						\
)

#define HaArrayWholeSwap(a, b, tmp) ((tmp) = (a), (a) = (b), (b) = (tmp))

#define HaArrayAppend(d, s, i)						\
  for( (i) = 0;  (i) < HaArrayCount(s);  (i)++ )			\
    HaArrayAddLast((d), HaArray((s), (i)))

#define HaArraySort(a, compar)						\
  qsort((a).items, HaArrayCount(a), sizeof((a).items[0]), (compar))

#define HaArraySortUnique(a, compar)					\
  (a).count = HaArrayUniqSort((a).items, HaArrayCount(a),		\
  sizeof((a).items[0]), (compar))


/* traversal */
#define HaArrayForEach(a, x, i)						\
  for( (i) = 0;								\
       (i) < HaArrayCount(a) ? ((x) = (a).items[i], true) : false;	\
       (i)++ )

#define HaArrayForEachReverse(a, x, i)					\
  for( (i) = HaArrayCount(a) - 1;					\
       (i) >= 0 ? ((x) = (a).items[i], true) : false;			\
       (i)-- )


/* should not be called directly */
extern int HaArrayUniqSort(void *base, size_t nmemb, size_t size,
  int(*compar)(const void *, const void *));
extern bool HaArrayImplFind(char *a, char *target, size_t elem_size,
  int count, int *pos);


/*****************************************************************************/
/*                                                                           */
/*  Version string                                                           */
/*                                                                           */
/*****************************************************************************/

#endif

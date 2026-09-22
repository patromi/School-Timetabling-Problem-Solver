
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
/*  FILE:     howard_p.h                                                     */
/*  PURPOSE:  header file for the Howard Pointer Table Library               */
/*                                                                           */
/*****************************************************************************/

#ifndef HOWARD_P_HEADER
#define HOWARD_P_HEADER
#include "howard_a.h"
#include <stdarg.h>

#define HP_DEBUG_PROBE_LENGTH 1

typedef int (*HP_HASH_FN)(void *p);
typedef bool (*HP_EQUAL_FN)(void *p1, void *p2);
typedef void (*HP_DEBUG_FN)(void *p1, FILE *fp);

/* 3.2 Defining, initializing, and freeing pointer tables */
typedef struct {
  int		size;		/* current size of key and value arrays      */
  int		trigger;	/* trigger for rehashing                     */
  int		pos;		/* temporary return value (insert and delete)*/
  int		value_size;	/* size of values                            */
#if HP_DEBUG_PROBE_LENGTH
  int		op_count;
  int		op_cost;
#endif
  HP_HASH_FN	key_hash;	/* function for hashing void *keys           */
  HP_EQUAL_FN	key_equal;	/* function for equality testing keys        */
  HP_DEBUG_FN	key_debug;	/* function for debug printing of keys       */
  void		**keys;		/* extensible array of keys                  */
  int		*hash_codes;	/* parallel array of hash codes              */
} HP_GROUP;

#define HP_TABLE(TYPE)							\
  struct {								\
    HP_GROUP	hp_table_u;	/* untyped part of table            */	\
    TYPE	*hp_table_v;	/* parallel typed array of values   */	\
  }

#if HP_DEBUG_PROBE_LENGTH
#define HpTableInit(t, key_hash_fn, key_equal_fn, key_debug_fn, a)	\
( (t).hp_table_u.size = (t).hp_table_u.trigger = (t).hp_table_u.pos = 0,\
  (t).hp_table_u.value_size = sizeof((t).hp_table_v[0]),		\
  (t).hp_table_u.op_count = (t).hp_table_u.op_cost = 0,			\
  (t).hp_table_u.key_hash = (key_hash_fn),				\
  (t).hp_table_u.key_equal = (key_equal_fn),				\
  (t).hp_table_u.key_debug = (key_debug_fn),				\
  (t).hp_table_u.keys = HaResizableAlloc(a),				\
  (t).hp_table_u.hash_codes = HaResizableAlloc(a),			\
  (t).hp_table_v = HaResizableAlloc(a),					\
  (t).hp_table_v = HpTableImplCheckRehash(&(t).hp_table_u,		\
    (char *) (t).hp_table_v)						\
)
#else
#define HpTableInit(t, key_hash_fn, key_equal_fn, key_debug_fn, a)	\
( (t).hp_table_u.size = (t).hp_table_u.trigger = (t).hp_table_u.pos = 0,\
  (t).hp_table_u.value_size = sizeof((t).hp_table_v[0]),		\
  (t).hp_table_u.key_hash = (key_hash_fn),				\
  (t).hp_table_u.key_equal = (key_equal_fn),				\
  (t).hp_table_u.key_debug = (key_debug_fn),				\
  (t).hp_table_u.keys = HaResizableAlloc(a),				\
  (t).hp_table_u.hash_codes = HaResizableAlloc(a),			\
  (t).hp_table_v = HaResizableAlloc(a),					\
  (t).hp_table_v = HpTableImplCheckRehash(&(t).hp_table_u,		\
    (char *) (t).hp_table_v)						\
)
#endif

#define HpTableKeyHashFn(t) ((t).hp_table_u.key_hash)
#define HpTableKeyEqualFn(t) ((t).hp_table_u.key_equal)
#define HpTableKeyDebugFn(t) ((t).hp_table_u.key_debug)
#define HpTableArena(t) HaResizableArena((t).hp_table_u.keys)

#define HpTableFree(t)							\
( HaResizableFree((t).hp_table_u.keys),					\
  HaResizableFree((t).hp_table_u.hash_codes),				\
  HaResizableFree((t).hp_table_v) )

/* 3.3 Adding entries to object tables */
#define HpTableAdd(t, key, value)					\
  HpTableAddHashed(t, HpTableKeyHashFn(t)(key), key, value)

#define HpTableAddUnique(t, key, value, other)				\
  HpTableAddUniqueHashed(t, HpTableKeyHashFn(t)(key), key, value, other)

#define HpTableAddUniqueHashed(t, hash_code, key, value, other)		\
(									\
  (t).hp_table_v = HpTableImplCheckRehash(&(t).hp_table_u,		\
    (char *) (t).hp_table_v),						\
  HpTableImplAddUniqueHashed(&(t).hp_table_u, hash_code, (key)) ?	\
    (HpTableReplace(t, (t).hp_table_u.pos, value), true) :		\
    ((other) = HpTableValue(t, (t).hp_table_u.pos), false)		\
)

#define HpTableAddHashed(t, hash_code, key, value)			\
(									\
  (t).hp_table_v = HpTableImplCheckRehash(&(t).hp_table_u,		\
    (char *) (t).hp_table_v),						\
  HpTableImplAddHashed(&(t).hp_table_u, hash_code, (key)),		\
  HpTableReplace(t, (t).hp_table_u.pos, value)				\
)

/* 3.4 Retrieving entries from pointer tables */
#define HpTableContains(t, key, pos)					\
  HpTableImplContainsHashed(&(t).hp_table_u, HpTableKeyHashFn(t)(key),	\
    key, &(pos))

#define HpTableContainsHashed(t, hash_code, key, pos)			\
  HpTableImplContainsHashed(&(t).hp_table_u, (hash_code), (key), &(pos))

#define HpTableContainsNext(t, pos)					\
  HpTableImplContainsNext(&(t).hp_table_u, &(pos))

#define HpTableContainsValue(t, key, value, pos)			\
  HpTableContainsValueHashed(t, HpTableKeyHashFn(t)(key), value, pos)

#define HpTableContainsValueHashed(t, hash_code, value, pos)		\
  for( (pos) = hash_code % HpTableSize(t);				\
       HpTableKey((t),(pos))!=0x0 && HpTableValue((t), (pos))!=(value);	\
       (pos) = ((pos) + 1) % HpTableSize(t) );

#define HpTableRetrieve(t, key, value, pos)				\
  HpTableRetrieveHashed((t), HpTableKeyHashFn(t)(key), (key), (value), (pos))

#define HpTableRetrieveHashed(t, hash_code, key, value, pos)		\
(									\
  HpTableContainsHashed((t), (hash_code), (key), (pos)) ?		\
  ((value) = HpTableValue((t), (pos)), true) : 				\
  ((value) = HpTableValue((t), (pos)), false) 				\
)

#define HpTableRetrieveNext(t, value, pos)				\
(									\
  HpTableContainsNext((t), (pos)) ?					\
  ((value) = HpTableValue((t), (pos)), true) : false			\
)

#define HpTableOccupied(t, pos)	(HpTableKey(t, pos) > (void *) 0x1)
#define HpTableKey(t, pos)	((t).hp_table_u.keys[pos])
#define HpTableValue(t, pos)	((t).hp_table_v[pos])

/* 3.5 Updating and deleting entries */
#define HpTableReplace(t, pos, value)	((t).hp_table_v[pos] = value)
#define HpTableDelete(t, pos) HpTableImplDelete(&(t).hp_table_u, pos)
#define HpTableClear(t) HpTableImplClear(&(t).hp_table_u)

/* 3.6 Traversing object tables */
#define HpTableForEachWithKey(t, key, value, pos)			\
  HpTableForEachWithKeyHashed(t, HpTableKeyHashFn(t)(key), (key),	\
    (value), (pos))

#define HpTableForEachWithKeyHashed(t, hash_code, key, value, pos)	\
  for( HpTableRetrieveHashed((t), (hash_code), (key), (value), (pos));	\
       HpTableOccupied((t), (pos));					\
       HpTableRetrieveNext((t), value, (pos)) )

#define HpTableForEach(t, key, value, pos)				\
  for( (pos) = 0;  (pos) < HpTableSize(t);  ((pos))++ )			\
    if( !HpTableOccupied((t), (pos)) ? false :				\
      ((key)=HpTableKey((t), (pos)), (value)=HpTableValue((t), (pos)), true) )

#define HpTableForEachValue(t, value, pos)				\
  for( (pos) = 0;  (pos) < HpTableSize(t);  ((pos))++ )			\
    if( !HpTableOccupied((t), (pos)) ? false :				\
      ((value)=HpTableValue((t), (pos)), true) )

#define HpTableSize(t) (t).hp_table_u.size

#define HpTableProbeLength(t) HpTableImplProbeLength(&(t).hp_table_u)
#define HpTableDebug(t, i, fp) HpTableImplDebug(&(t).hp_table_u, i, fp)

/* ********************************************************************* */

#define HpGroupInit(t, key_hash_fn, key_equal_fn, key_debug_fn, a)	\
( (t).size = (t).trigger = (t).pos = 0,					\
  (t).value_size = 0,							\
  (t).key_hash = (key_hash_fn),						\
  (t).key_equal = (key_equal_fn),					\
  (t).key_debug = (key_debug_fn),					\
  (t).keys = HaResizableAlloc(a),					\
  (t).hash_codes = HaResizableAlloc(a),					\
  HpTableImplCheckRehashNoValues(&(t))					\
)

#define HpGroupKeyHashFn(t) ((t).key_hash)
#define HpGroupKeyEqualFn(t) ((t).key_equal)
#define HpGroupKeyDebugFn(t) ((t).key_debug)
#define HpGroupArena(t) HaResizableArena((t).keys)

#define HpGroupFree(t) (HaResizableFree((t).keys),			\
  HaResizableFree((t).hash_codes))

/* 3.3 Adding entries to pointer tables */
#define HpGroupAdd(t, key)						\
  HpGroupAddHashed(t, HpGroupKeyHashFn(t)(key), key)

#define HpGroupAddUnique(t, key)					\
  HpGroupAddUniqueHashed(t, HpGroupKeyHashFn(t)(key), key)

#define HpGroupAddUniqueHashed(t, hash_code, key)			\
(									\
  HpTableImplCheckRehashNoValues(&(t)),					\
  HpTableImplAddUniqueHashed(&(t), hash_code, (key))			\
)

#define HpGroupAddHashed(t, hash_code, key)				\
(									\
  HpTableImplCheckRehashNoValues(&(t)),					\
  HpTableImplAddHashed(&(t), hash_code, (key))				\
)

/* 3.4 Retrieving entries from object tables */
#define HpGroupContains(t, key, pos)					\
  HpTableImplContainsHashed(&(t), HpGroupKeyHashFn(t)(key), key, &(pos))

#define HpGroupContainsHashed(t, hash_code, key, pos)			\
  HpTableImplContainsHashed(&(t), (hash_code), (key), &(pos))

#define HpGroupContainsNext(t, pos)					\
  HpTableImplContainsNext(&(t), &(pos))

#define HpGroupOccupied(t, pos)	(HpGroupKey(t, pos) > (char *) 0x1)
#define HpGroupKey(t, pos)	((t).keys[pos])

/* 3.5 Updating and deleting entries */
#define HpGroupDelete(t, pos) HpTableImplDelete(&(t), pos)
#define HpGroupClear(t) HpTableImplClear(&(t))

/* 3.6 Traversing object tables */
#define HpGroupForEachWithKey(t, key, pos)				\
  HpGroupForEachWithKeyHashed(t, HpGroupKeyHashFn(t)(key), (key), (pos))

#define HpGroupForEachWithKeyHashed(t, hash_code, key, pos)		\
  for( HpGroupRetrieveHashed((t), (hash_code), (key), (pos));		\
       HpGroupOccupied((t), (pos));					\
       HpGroupRetrieveNext((t), (pos)) )

#define HpGroupForEach(t, key, pos)					\
  for( (pos) = 0;  (pos) < HpGroupSize(t);  ((pos))++ )			\
    if( !HpGroupOccupied((t), (pos)) ? false :				\
      ((key)=HpGroupKey((t), (pos)), true) )

#define HpGroupSize(t) (t).size

/* these functions should not be called directly */
extern void HpTableImplClear(HP_GROUP *table);
extern void *HpTableImplCheckRehash(HP_GROUP *table, char *hp_table_v);
extern void HpTableImplCheckRehashNoValues(HP_GROUP *table);
/* ***
extern void HpTableImplAdd(HP_GROUP *table, char *key);
extern bool HpTableImplAddUnique(HP_GROUP *table, char *key);
*** */
extern void HpTableImplAddHashed(HP_GROUP *table, int hash_code, void *key);
extern bool HpTableImplAddUniqueHashed(HP_GROUP *table, int hash_code,
  void *key);
extern bool HpTableImplContainsHashed(HP_GROUP *table, int hash_code,
  void *key, int *pos);
extern bool HpTableImplContainsNext(HP_GROUP *table, int *pos);
extern void HpTableImplDelete(HP_GROUP *table, int pos);
extern float HpTableImplProbeLength(HP_GROUP *table);
extern void HpTableImplDebug(HP_GROUP *table, int indent, FILE *fp);

#endif

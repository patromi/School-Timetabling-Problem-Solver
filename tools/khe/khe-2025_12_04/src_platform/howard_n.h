
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
/*  FILE:     howard_n.h                                                     */
/*  PURPOSE:  header file for the Howard Narrow String Library               */
/*                                                                           */
/*****************************************************************************/

#ifndef HOWARD_N_HEADER
#define HOWARD_N_HEADER
#include "howard_a.h"
#include <stdarg.h>

/* 3.7 Narrow strings and symbol tables */
#define HnStringBegin(anc, a)	HaArrayInit(anc, a)
#define HnStringEnd(anc) (HaArrayCount(anc) > 0 && HaArrayLast(anc) == '\0' ? \
  (anc).items : (HaArrayAddLast((anc), '\0'), (anc).items))

extern void HnStringAdd(HA_ARRAY_NCHAR *anc, const char *format, ...);
extern void HnStringVAdd(HA_ARRAY_NCHAR *anc, const char *format,
  va_list args);
extern void HnStringAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,
  const char *format, ...);
extern void HnStringVAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,
  const char *format, va_list args);
extern char *HnStringCopy(char *s, HA_ARENA a);
extern char *HnStringSubstring(char *s, int start, int len, HA_ARENA a);
extern char *HnStringMake(HA_ARENA a, const char *format, ...);
extern char *HnStringVMake(HA_ARENA a, const char *format, va_list args);
extern char *HnStringFromWide(wchar_t *s, HA_ARENA a);
extern wchar_t *HnStringToWide(char *s, HA_ARENA a);

#define HnStringCount(s)	strlen(s)
#define HnStringIsEmpty(s)	((s)[0] == '\0')
#define HnStringEqual(s1, s2)	(strcmp((s1), (s2)) == 0)
extern bool HnStringContains(char *s, char *substr, int *pos);
extern bool HnStringBeginsWith(char *s, char *prefix);
extern bool HnStringEndsWith(char *s, char *suffix);
extern bool HnStringIsWhiteSpaceOnly(char *s);
extern char *HnStringCopyStripped(char *s, HA_ARENA a);

/* Early exit on error */
extern void HnFatal(char *fmt, ...);
extern void HnAbort(char *fmt, ...);
extern void HnAssert(bool cond, char *fmt, ...);

/* 3.2 Defining, initializing, and freeing symbol tables */
typedef struct {
  int		size;		/* current size of key and value arrays      */
  int		trigger;	/* trigger for rehashing                     */
  int		pos;		/* temporary return value (insert and delete)*/
  int		value_size;	/* size of values                            */
  char		**keys;		/* extensible array of keys                  */
} HN_TABLE_U;

#define HN_TABLE(TYPE)							\
  struct {								\
    HN_TABLE_U	hn_table_u;	/* untyped part of table            */	\
    TYPE	*hn_table_v;	/* parallel typed array of values   */	\
  }

#define HnTableInit(t, a)						\
( (t).hn_table_u.size = (t).hn_table_u.trigger = (t).hn_table_u.pos = 0,\
  (t).hn_table_u.value_size = sizeof((t).hn_table_v[0]),		\
  (t).hn_table_u.keys = HaResizableAlloc(a),				\
  (t).hn_table_v = HaResizableAlloc(a),					\
  (t).hn_table_v = HnTableImplCheckRehash(&(t).hn_table_u,		\
    (char *) (t).hn_table_v)						\
)

#define HnTableArena(t) HaResizableArena((t).hn_table_u.keys)

#define HnTableFree(t)							\
( HaResizableFree((t).hn_table_u.keys), HaResizableFree((t).hn_table_v) )

/* 3.3 Adding entries to symbol tables */
#define HnTableAdd(t, key, value)					\
  HnTableAddHashed(t, HnTableHash(key), key, value)

#define HnTableAddUnique(t, key, value, other)				\
  HnTableAddUniqueHashed(t, HnTableHash(key), key, value, other)

#define HnTableAddUniqueHashed(t, hash_code, key, value, other)		\
(									\
  (t).hn_table_v = HnTableImplCheckRehash(&(t).hn_table_u,		\
    (char *) (t).hn_table_v),						\
  HnTableImplAddUniqueHashed(&(t).hn_table_u, hash_code, (key)) ?	\
    (HnTableReplace(t, (t).hn_table_u.pos, value), true) :		\
    ((other) = HnTableValue(t, (t).hn_table_u.pos), false)		\
)

#define HnTableAddHashed(t, hash_code, key, value)			\
(									\
  (t).hn_table_v = HnTableImplCheckRehash(&(t).hn_table_u,		\
    (char *) (t).hn_table_v),						\
  HnTableImplAddHashed(&(t).hn_table_u, hash_code, (key)),		\
  HnTableReplace(t, (t).hn_table_u.pos, value)				\
)

/* 3.4 Retrieving entries from symbol tables */
#define HnTableContains(t, key, pos)					\
  HnTableImplContainsHashed(&(t).hn_table_u, HnTableHash(key), key, &(pos))

#define HnTableContainsHashed(t, hash_code, key, pos)			\
  HnTableImplContainsHashed(&(t).hn_table_u, (hash_code), (key), &(pos))

#define HnTableContainsNext(t, pos)					\
  HnTableImplContainsNext(&(t).hn_table_u, &(pos))

#define HnTableContainsValue(t, key, value, pos)			\
  HnTableContainsValueHashed(t, HnTableHash(key), value, pos)

#define HnTableContainsValueHashed(t, hash_code, value, pos)		\
  for( (pos) = hash_code % HnTableSize(t);				\
       HnTableKey((t),(pos))!=0x0 && HnTableValue((t), (pos))!=(value);	\
       (pos) = ((pos) + 1) % HnTableSize(t) );

extern int HnTableHash(char *key);

#define HnTableRetrieve(t, key, value, pos)				\
  HnTableRetrieveHashed((t), HnTableHash(key), (key), (value), (pos))

#define HnTableRetrieveHashed(t, hash_code, key, value, pos)		\
(									\
  HnTableContainsHashed((t), (hash_code), (key), (pos)) ?		\
  ((value) = HnTableValue((t), (pos)), true) : 				\
  ((value) = HnTableValue((t), (pos)), false) 				\
)

#define HnTableRetrieveNext(t, value, pos)				\
(									\
  HnTableContainsNext((t), (pos)) ?					\
  ((value) = HnTableValue((t), (pos)), true) : false			\
)

#define HnTableOccupied(t, pos)	(HnTableKey(t, pos) > (char *) 0x1)
#define HnTableKey(t, pos)	((t).hn_table_u.keys[pos])
#define HnTableValue(t, pos)	((t).hn_table_v[pos])

/* 3.5 Updating and deleting entries */
#define HnTableReplace(t, pos, value)	((t).hn_table_v[pos] = value)
#define HnTableDelete(t, pos) HnTableImplDelete(&(t).hn_table_u, pos)
#define HnTableClear(t) HnTableImplClear(&(t).hn_table_u)

/* 3.6 Traversing symbol tables */
#define HnTableForEachWithKey(t, key, value, pos)			\
  HnTableForEachWithKeyHashed(t, HnTableHash(key), (key), (value), (pos))

#define HnTableForEachWithKeyHashed(t, hash_code, key, value, pos)	\
  for( HnTableRetrieveHashed((t), (hash_code), (key), (value), (pos));	\
       HnTableOccupied((t), (pos));					\
       HnTableRetrieveNext((t), value, (pos)) )

#define HnTableForEach(t, key, value, pos)				\
  for( (pos) = 0;  (pos) < HnTableSize(t);  ((pos))++ )			\
    if( !HnTableOccupied((t), (pos)) ? false :				\
      ((key)=HnTableKey((t), (pos)), (value)=HnTableValue((t), (pos)), true) )

#define HnTableSize(t) (t).hn_table_u.size

/* these functions should not be called directly */
extern void HnTableImplClear(HN_TABLE_U *table);
extern void *HnTableImplCheckRehash(HN_TABLE_U *table, char *hn_table_v);
/* ***
extern void HnTableImplAdd(HN_TABLE_U *table, char *key);
extern bool HnTableImplAddUnique(HN_TABLE_U *table, char *key);
*** */
extern void HnTableImplAddHashed(HN_TABLE_U *table, int hash_code, char *key);
extern bool HnTableImplAddUniqueHashed(HN_TABLE_U *table, int hash_code,
  char *key);
extern bool HnTableImplContainsHashed(HN_TABLE_U *table, int hash_code,
  char *key, int *pos);
extern bool HnTableImplContainsNext(HN_TABLE_U *table, int *pos);
extern void HnTableImplDelete(HN_TABLE_U *table, int pos);

#endif

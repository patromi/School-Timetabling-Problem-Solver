
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
/*  FILE:         khe_trie.h                                                 */
/*  DESCRIPTION:  Generic trie (header file)                                 */
/*                                                                           */
/*****************************************************************************/
#ifndef KHE_TRIE_HEADER_FILE
#define KHE_TRIE_HEADER_FILE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "howard_a.h"
#include "howard_n.h"

/*****************************************************************************/
/*                                                                           */
/*  KHE_TRIE                                                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_trie_u_rec *KHE_TRIE_U;
typedef HA_ARRAY(KHE_TRIE_U) ARRAY_KHE_TRIE_U;

struct khe_trie_u_rec {
  int			value_index;
  ARRAY_KHE_TRIE_U	children;
};

#define KHE_TRIE(VAL_TYPE)						\
  struct {								\
    KHE_TRIE_U		trie_u;		/* untyped trie */		\
    HA_ARRAY(VAL_TYPE)	values;		/* array of values */		\
    ARRAY_KHE_TRIE_U	trie_u_free_list;				\
  }

#define KheTrieInit(trie, a)						\
(									\
  (trie).trie_u = NULL,							\
  HaArrayInit((trie).values, (a)),					\
  HaArrayInit((trie).trie_u_free_list, (a))				\
)

#define KheTrieClear(trie)						\
(									\
  KheTrieImplClear((trie).trie_u, &((trie).trie_u_free_list)),		\
  (trie).trie_u = NULL,							\
  HaArrayClear((trie).values)						\
)

#define KheTrieArena(trie) HaArrayArena((trie).values)

#define KheTrieAdd(trie, key, value)					\
(									\
  KheTrieImplAdd(&(trie).trie_u, (key), 0, HaArrayCount((trie).values),	\
    KheTrieArena(trie), &((trie).trie_u_free_list)),			\
  HaArrayAddLast((trie).values, (value))				\
)

#define KheTrieRetrieve(trie, key, value, pos)				\
(									\
  KheTrieImplRetrieve((trie).trie_u, (key), 0, &(pos)) ?		\
    ((value) = HaArray((trie).values, (pos)), true) : false		\
)

extern void KheTrieImplClear(KHE_TRIE_U trie_u, ARRAY_KHE_TRIE_U *free_list);
extern void KheTrieImplAdd(KHE_TRIE_U *trie_u, HA_ARRAY_INT *key,
  int key_index, int value_index, HA_ARENA a, ARRAY_KHE_TRIE_U *free_list);
extern bool KheTrieImplRetrieve(KHE_TRIE_U trie_u, HA_ARRAY_INT *key, int index,
  int *value_index);

extern void KheTrieTest(HA_ARENA a);

#endif

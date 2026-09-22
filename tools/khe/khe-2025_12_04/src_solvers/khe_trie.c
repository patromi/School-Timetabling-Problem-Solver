
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
/*  FILE:         khe_trie.c                                                 */
/*  DESCRIPTION:  Generic trie (implementation file)                         */
/*                                                                           */
/*****************************************************************************/

#include "khe_trie.h"

#define DEBUG1 1
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TRIE"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTrieImplClear(KHE_TRIE_U trie_u, ARRAY_KHE_TRIE_U *free_list)    */
/*                                                                           */
/*  Free trie_u, placing its trie_u objects onto free_list.                  */
/*                                                                           */
/*****************************************************************************/

void KheTrieImplClear(KHE_TRIE_U trie_u, ARRAY_KHE_TRIE_U *free_list)
{
  KHE_TRIE_U child_trie_u;  int i;
  if( DEBUG2 )
    fprintf(stderr, "[ KheTrieImplClear(%p, free_list)\n", (void *) trie_u);
  if( trie_u != NULL )
  {
    HaArrayForEach(trie_u->children, child_trie_u, i)
      KheTrieImplClear(child_trie_u, free_list);
    HaArrayAddLast(*free_list, trie_u);
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheTrieImplClear\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTrieImplDebugKeyAndIndex(HA_ARRAY_INT *key, int key_index,       */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of key[key_index ...] onto fp.                               */
/*                                                                           */
/*****************************************************************************/

static void KheTrieImplDebugKeyAndIndex(HA_ARRAY_INT *key, int key_index,
  FILE *fp)
{
  int i;
  fprintf(fp, "[");
  for( i = key_index;  i < HaArrayCount(*key);  i++ )
  {
    if( i > key_index )
      fprintf(fp, ", ");
    fprintf(fp, "%d", HaArray(*key, i));
  }
  fprintf(fp, "]");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTrieImplAdd(KHE_TRIE_U *trie_u, HA_ARRAY_INT *key, int key_index,*/
/*    int value_index, HA_ARENA a, ARRAY_KHE_TRIE_U *free_list)              */
/*                                                                           */
/*  Add (key[key_index ...], value_index) to *trie_u, possibly changing      */
/*  *trie_u from a NULL value to a non-NULL one along the way.               */
/*                                                                           */
/*****************************************************************************/

void KheTrieImplAdd(KHE_TRIE_U *trie_u, HA_ARRAY_INT *key, int key_index,
  int value_index, HA_ARENA a, ARRAY_KHE_TRIE_U *free_list)
{
  int key_item;

  /* you can't insert into an empty trie, so first, if empty, make a node */
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheTrieImplAdd(%p, ", (void *) trie_u);
    KheTrieImplDebugKeyAndIndex(key, key_index, stderr);
    fprintf(stderr, ", %d)\n", value_index);
  }
  if( *trie_u == NULL )
  {
    if( HaArrayCount(*free_list) > 0 )
      *trie_u = HaArrayLastAndDelete(*free_list);
    else
      HaMake(*trie_u, a);
    (*trie_u)->value_index = -1;         /* i.e. no key ends at this node */
    HaArrayInit((*trie_u)->children, a);
  }

  /* now we have a node, so insert into it */
  if( key_index >= HaArrayCount(*key) )
  {
    /* the current node is the one we are looking for, to insert into */
    HnAssert((*trie_u)->value_index == -1,
      "KheUnTypedTrieInsert internal error (key already inserted)");
    (*trie_u)->value_index = value_index;
  }
  else
  {
    /* have to recurse */
    key_item = HaArray(*key, key_index);
    HnAssert(key_item >= 0, "KheTrieImplAdd: key[%d] is negative\n", key_index);
    HaArrayFill((*trie_u)->children, key_item + 1, NULL);
    KheTrieImplAdd(&HaArray((*trie_u)->children, key_item),
      key, key_index + 1, value_index, a, free_list);
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheTrieImplAdd returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTrieImplRetrieve(KHE_TRIE_U trie_u, HA_ARRAY_INT *key,           */
/*    int key_index, int *value_index)                                       */
/*                                                                           */
/*  If there is an entry for khe[key_index ...] in trie_u, set *value_index  */
/*  to its value and return true.  Otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

bool KheTrieImplRetrieve(KHE_TRIE_U trie_u, HA_ARRAY_INT *key,
  int key_index, int *value_index)
{
  int key_item;  bool res;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheTrieImplRetrieve(%p, ", (void *) trie_u);
    KheTrieImplDebugKeyAndIndex(key, key_index, stderr);
    fprintf(stderr, ", -)\n");
  }
  if( trie_u == NULL )
    *value_index = -1, res = false;
  else if( key_index >= HaArrayCount(*key) )
    *value_index = trie_u->value_index, res = (*value_index != -1);
  else
  {
    key_item = HaArray(*key, key_index);
    HnAssert(key_item >= 0, "KheTrieImplRetrieve: key[%d] is negative\n",
      key_index);
    if( key_item >= HaArrayCount(trie_u->children) )
      *value_index = -1, res = false;
    else
      res = KheTrieImplRetrieve(HaArray(trie_u->children, key_item),
	key, key_index + 1, value_index);
  }
  if( DEBUG2 )
  {
    if( res )
      fprintf(stderr, "] KheTrieImplRetrieve returning true (%d)\n",
	*value_index);
    else
      fprintf(stderr, "] KheTrieImplRetrieve returning false\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Test"                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheArrayIntDebug(HA_ARRAY_INT *key, FILE *fp)                       */
/*                                                                           */
/*  Debug print of array of integers *key.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheArrayIntDebug(HA_ARRAY_INT *key, FILE *fp)
{
  int val, i;
  fprintf(fp, "[");
  HaArrayForEach(*key, val, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%d", val);
  }
  fprintf(fp, "]");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheKeyAndValueDebug(HA_ARRAY_INT *key, char *value,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of one (key, value) pair.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheKeyAndValueDebug(HA_ARRAY_INT *key, char *value,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s", indent, "");
  KheArrayIntDebug(key, fp);
  if( value != NULL )
    fprintf(fp, ": \"%s\"\n", value);
  else
    fprintf(fp, ": no value\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTrieTest(HA_ARENA a)                                             */
/*                                                                           */
/*  Test the trie module.                                                    */
/*                                                                           */
/*****************************************************************************/

typedef KHE_TRIE(char *) TRIE_STRING;

void KheTrieTest(HA_ARENA a)
{
  TRIE_STRING trie;  char *str;  int pos;
  HA_ARRAY_INT key1, key2, key3, key4;
  if( DEBUG1 )
    fprintf(stderr, "[ KheTrieTest\n");

  /* make key1 */
  HaArrayInit(key1, a);
  HaArrayAddLast(key1, 3);
  HaArrayAddLast(key1, 3);
  HaArrayAddLast(key1, 2);

  /* make key2 */
  HaArrayInit(key2, a);
  HaArrayAddLast(key2, 3);
  HaArrayAddLast(key2, 2);

  /* make key3 */
  HaArrayInit(key3, a);
  HaArrayAddLast(key3, 3);
  HaArrayAddLast(key3, 3);

  /* make key4 */
  HaArrayInit(key4, a);

  KheTrieInit(trie, a);
  KheTrieAdd(trie, &key1, "key 1");
  KheTrieAdd(trie, &key2, "key 2");
  KheTrieAdd(trie, &key4, "key 4");

  /* try some retrievals */
  if( !KheTrieRetrieve(trie, &key1, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key1, str, 2, stderr);

  if( !KheTrieRetrieve(trie, &key2, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key2, str, 2, stderr);

  if( !KheTrieRetrieve(trie, &key3, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key3, str, 2, stderr);

  if( !KheTrieRetrieve(trie, &key4, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key4, str, 2, stderr);

  /* clear the trie and try again */
  KheTrieClear(trie);
  KheTrieAdd(trie, &key1, "key 1");
  KheTrieAdd(trie, &key2, "key 2");
  KheTrieAdd(trie, &key4, "key 4");
  if( !KheTrieRetrieve(trie, &key1, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key1, str, 2, stderr);
  if( !KheTrieRetrieve(trie, &key2, str, pos) )
    str = NULL;
  KheKeyAndValueDebug(&key2, str, 2, stderr);

  if( DEBUG1 )
    fprintf(stderr, "] KheTrieTest returning\n");
}

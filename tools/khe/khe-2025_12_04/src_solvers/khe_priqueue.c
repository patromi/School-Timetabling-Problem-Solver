
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
/*  FILE:         khe_priqueue.c                                             */
/*  DESCRIPTION:  Generic priority queue                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_priqueue.h"
#include "howard_a.h"
#include "howard_n.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_PRIQUEUE - the priority queue type                                   */
/*                                                                           */
/*****************************************************************************/

struct khe_priqueue_rec {
  /* HA_ARENA 			arena; */
  KHE_PRIQUEUE_KEY_FN		key;		/* current key of entry      */
  KHE_PRIQUEUE_INDEX_GET_FN	index_get;	/* current index of entry    */
  KHE_PRIQUEUE_INDEX_SET_FN	index_set;	/* set current index of entry*/
  HA_ARRAY_VOIDP		entries;	/* entries[0] is NULL        */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_PRIQUEUE KhePriQueueMake(KHE_PRIQUEUE_KEY_FN key,                    */
/*    KHE_PRIQUEUE_INDEX_GET_FN index_get,                                   */
/*    KHE_PRIQUEUE_INDEX_SET_FN index_set, HA_ARENA a)                       */
/*                                                                           */
/*  Make and return a new priority queue with these attributes, in its       */
/*  own arena.                                                               */
/*                                                                           */
/*****************************************************************************/

KHE_PRIQUEUE KhePriQueueMake(KHE_PRIQUEUE_KEY_FN key,
  KHE_PRIQUEUE_INDEX_GET_FN index_get,
  KHE_PRIQUEUE_INDEX_SET_FN index_set, HA_ARENA a)
{
  KHE_PRIQUEUE res;   /* HA_ARENA a; */
  /* a = HaAren aMake(); */
  HaMake(res, a);
  /* res->arena = a; */
  res->key = key;
  res->index_get = index_get;
  res->index_set = index_set;
  HaArrayInit(res->entries, a);
  HaArrayAddLast(res->entries, NULL);  /* entries[0] is always NULL */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueDelete(KHE_PRIQUEUE p)                                   */
/*                                                                           */
/*  Delete p.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePriQueueDelete(KHE_PRIQUEUE p)
{
  HaArenaDe lete(p->arena);
  ** ***
  MArrayFree(p->entries);
  MFree(p);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KhePriQueueEmpty(KHE_PRIQUEUE p)                                    */
/*                                                                           */
/*  Return true if p is empty, else false.                                   */
/*                                                                           */
/*****************************************************************************/

bool KhePriQueueEmpty(KHE_PRIQUEUE p)
{
  return HaArrayCount(p->entries) == 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddLeaf(KHE_PRIQUEUE p, int pos)                                    */
/*                                                                           */
/*  Add a leaf to p at pos.  The entry is already in p, it just needs        */
/*  heapifying.  This function does nothing if the entry is already in       */
/*  the right place with respect to its parent.                              */
/*                                                                           */
/*****************************************************************************/

static void AddLeaf(KHE_PRIQUEUE p, int pos)
{
  void *x, *y;  int j;  int64_t x_key;
  x = HaArray(p->entries, pos);
  x_key = p->key(x);
  j = pos / 2;
  y = HaArray(p->entries, j);
  while( j > 0 && p->key(y) > x_key )
  {
    HaArrayPut(p->entries, pos, y);
    p->index_set(y, pos);
    pos = j;
    j = pos / 2;
    y = HaArray(p->entries, j);
  }
  HaArrayPut(p->entries, pos, x);
  p->index_set(x, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void AddRoot(KHE_PRIQUEUE p, int pos)                                    */
/*                                                                           */
/*  Add a new root to p at pos.  The entry is already in p, it just needs    */
/*  heapifying.  This function does nothing if the entry is already in       */
/*  the right place with respect to its children.                            */
/*                                                                           */
/*****************************************************************************/

static void AddRoot(KHE_PRIQUEUE p, int pos)
{
  void *x, *y;  int j;  int64_t x_key;
  x = HaArray(p->entries, pos);
  x_key = p->key(x);
  j = 2 * pos;
  while( j < HaArrayCount(p->entries) )
  {
    /* find smallest child, y, and its position j */
    if( j < HaArrayCount(p->entries) - 1 &&
        p->key(HaArray(p->entries, j)) > p->key(HaArray(p->entries, j+1)) )
      j++;
    y = HaArray(p->entries, j);

    /* quit now if smallest child is no smaller */
    if( x_key <= p->key(y) )
      break;

    /* move y up and continue down the tree */
    HaArrayPut(p->entries, pos, y);
    p->index_set(y, pos);
    pos = j;
    j = 2 * pos;
  }
  HaArrayPut(p->entries, pos, x);
  p->index_set(x, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueInsert(KHE_PRIQUEUE p, void *entry)                      */
/*                                                                           */
/*  Insert entry into p.                                                     */
/*                                                                           */
/*****************************************************************************/

void KhePriQueueInsert(KHE_PRIQUEUE p, void *entry)
{
  HaArrayAddLast(p->entries, entry);
  AddLeaf(p, HaArrayCount(p->entries) - 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KhePriQueueFindMin(KHE_PRIQUEUE p)                                 */
/*                                                                           */
/*  Assuming p is non-empty, return a minimum entry.                         */
/*                                                                           */
/*****************************************************************************/

void *KhePriQueueFindMin(KHE_PRIQUEUE p)
{
  HnAssert(HaArrayCount(p->entries) > 1, "KhePriQueueFindMin: p is empty");
  return HaArray(p->entries, 1);
}


/*****************************************************************************/
/*                                                                           */
/*  void *KhePriQueueDeleteMin(KHE_PRIQUEUE p)                               */
/*                                                                           */
/*  Delete and return a minimum entry.                                       */
/*                                                                           */
/*****************************************************************************/

void *KhePriQueueDeleteMin(KHE_PRIQUEUE p)
{
  void *res, *x;
  HnAssert(HaArrayCount(p->entries) > 1, "KhePriQueueFindMin: p is empty");
  res = HaArray(p->entries, 1);
  p->index_set(res, 0);
  x = HaArrayLastAndDelete(p->entries);
  if( HaArrayCount(p->entries) > 1 )
  {
    HaArrayPut(p->entries, 1, x);
    AddRoot(p, 1);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueDeleteEntry(KHE_PRIQUEUE p, void *entry)                 */
/*                                                                           */
/*  Delete entry from p; it must be present.                                 */
/*                                                                           */
/*****************************************************************************/

void KhePriQueueDeleteEntry(KHE_PRIQUEUE p, void *entry)
{
  int pos;  void *x;
  pos = p->index_get(entry);
  HnAssert(pos >= 1 && pos < HaArrayCount(p->entries),
    "KhePriQueueDeleteEntry: index returned by index_get out of range");
  HnAssert(HaArray(p->entries, pos) == entry,
    "KhePriQueueDeleteEntry: index returned by index_get is inconsistent");
  if( pos == HaArrayCount(p->entries) - 1 )
  {
    /* entry just happens to be last in the array */
    HaArrayDeleteLast(p->entries);
  }
  else
  {
    /* overwrite entry with last element, and do a key update */
    x = HaArrayLastAndDelete(p->entries);
    HaArrayPut(p->entries, pos, x);
    AddRoot(p, pos);
    AddLeaf(p, p->index_get(x));
  }
  p->index_set(entry, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueClear(KHE_PRIQUEUE p)                                    */
/*                                                                           */
/*  Clear p.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KhePriQueueClear(KHE_PRIQUEUE p)
{
  while( HaArrayCount(p->entries) > 0 )
    KhePriQueueDeleteEntry(p, HaArrayLast(p->entries));
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueNotifyKeyChange(KHE_PRIQUEUE p, void *entry)             */
/*                                                                           */
/*  Accept a notification that the key of *entry has just changed.  It       */
/*  could increase or decrease.                                              */
/*                                                                           */
/*****************************************************************************/

void KhePriQueueNotifyKeyChange(KHE_PRIQUEUE p, void *entry)
{
  int pos;
  pos = p->index_get(entry);
  HnAssert(pos >= 1 && pos < HaArrayCount(p->entries),
    "KhePriQueueNotifyKeyChange: index returned by index_get out of range");
  HnAssert(HaArray(p->entries, pos) == entry,
    "KhePriQueueNotifyKeyChange: index returned by index_get is inconsistent");
  AddRoot(p, pos);
  AddLeaf(p, p->index_get(entry));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "testing".                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PRIQUEUE_TEST_ENTRY - entry type for testing.                        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_priqueue_test_entry_rec {
  int index;
  int64_t key;
} *KHE_PRIQUEUE_TEST_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  int64_t KhePriQueueTestKey(KHE_PRIQUEUE_TEST_ENTRY pte)                  */
/*                                                                           */
/*  Return the key of pte.                                                   */
/*                                                                           */
/*****************************************************************************/

static int64_t KhePriQueueTestKey(KHE_PRIQUEUE_TEST_ENTRY pte)
{
  return pte->key;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePriQueueTestIndex(KHE_PRIQUEUE_TEST_ENTRY pte)                    */
/*                                                                           */
/*  Return the index of pte.                                                 */
/*                                                                           */
/*****************************************************************************/

static int KhePriQueueTestIndex(KHE_PRIQUEUE_TEST_ENTRY pte)
{
  return pte->index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueTestIndexSet(KHE_PRIQUEUE_TEST_ENTRY pte, int index)     */
/*                                                                           */
/*  Set the index of pte.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KhePriQueueTestIndexSet(KHE_PRIQUEUE_TEST_ENTRY pte, int index)
{
  pte->index = index;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PRIQUEUE_TEST_ENTRY KhePriQueueTestEntryMake(int64_t key,            */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a new test entry with this key.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_PRIQUEUE_TEST_ENTRY KhePriQueueTestEntryMake(int64_t key,
  HA_ARENA a)
{
  KHE_PRIQUEUE_TEST_ENTRY res;
  HaMake(res, a);
  res->index = 0;
  res->key = key;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePriQueueTest(FILE *fp)                                           */
/*                                                                           */
/*  Test this module.                                                        */
/*                                                                           */
/*****************************************************************************/

void KhePriQueueTest(FILE *fp)
{
  KHE_PRIQUEUE p;  KHE_PRIQUEUE_TEST_ENTRY pte;  HA_ARENA a;  HA_ARENA_SET as;
  fprintf(fp, "[ KhePriQueueTest()\n");
  as = HaArenaSetMake();
  a = HaArenaMake(as);  /* this arena is used only for testing */
  p = KhePriQueueMake((KHE_PRIQUEUE_KEY_FN) &KhePriQueueTestKey,
    (KHE_PRIQUEUE_INDEX_GET_FN) &KhePriQueueTestIndex,
    (KHE_PRIQUEUE_INDEX_SET_FN) &KhePriQueueTestIndexSet, a);
  KhePriQueueInsert(p, KhePriQueueTestEntryMake(50, a));
  KhePriQueueInsert(p, KhePriQueueTestEntryMake(25, a));
  KhePriQueueInsert(p, KhePriQueueTestEntryMake(60, a));
  KhePriQueueInsert(p, KhePriQueueTestEntryMake(17, a));
  KhePriQueueInsert(p, KhePriQueueTestEntryMake(80, a));
  KhePriQueueInsert(p, (pte = KhePriQueueTestEntryMake(30, a)));
  pte->key = 40;
  KhePriQueueNotifyKeyChange(p, pte);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  del %ld\n",
    ((KHE_PRIQUEUE_TEST_ENTRY) KhePriQueueDeleteMin(p))->key);
  fprintf(fp, "  empty %s\n", KhePriQueueEmpty(p) ? "true" : "false");
  fprintf(fp, "] KhePriQueueTest returning\n");
  /* KhePriQueueDelete(p); */
  HaArenaDelete(a);  /* this arena is used only for testing */
}

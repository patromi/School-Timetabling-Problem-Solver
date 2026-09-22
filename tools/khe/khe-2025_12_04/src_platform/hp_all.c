
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
/*  FILE:     hp_all.c                                                       */
/*  PURPOSE:  implementation file for the Howard Pointer Table Library       */
/*                                                                           */
/*****************************************************************************/
#include "howard_n.h"
#include "howard_p.h"

#define DEBUG1 0
#define DEBUG2 0			/* rehashing */
#define DEBUG3 0			/* rehashing (brief) */
#define DEBUG4 0			/* HpTableImplCheck */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Pointer tables"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HpTableImplCheck(HP_GROUP *table)                                   */
/*                                                                           */
/*  Check consistency of keys with hash codes.                               */
/*                                                                           */
/*****************************************************************************/

static void HpTableImplCheck(HP_GROUP *table, char *where)
{
  int i;
  if( DEBUG4 )
  {
    for( i = 0;  i < table->size;  i++ )
      if( table->keys[i] > (void *) 0x1 )
      {
	/* check keys[i] against hash_codes[i] */
	HnAssert(table->key_hash(table->keys[i]) == table->hash_codes[i],
	  "%s:  key code %d != hash_code %d\n", where,
	  table->key_hash(table->keys[i]), table->hash_codes[i]);
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void HpTableImplDebug(HP_GROUP *table, int indent, FILE *fp)             */
/*                                                                           */
/*  Debug print of *table onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

void HpTableImplDebug(HP_GROUP *table, int indent, FILE *fp)
{
  int i, j;
  fprintf(stderr, "%*s[ HpTable(%p)\n", indent, "", (void *) table);
  for( i = 0;  i < table->size;  i = j )
  {
    fprintf(stderr, "%*s  %d: ", indent, "", i);
    for( j = i + 1;  j < table->size && table->keys[j] == table->keys[i]; j++ );
    if( table->keys[i] == 0x0 )
      fprintf(stderr, "0x0 * %d\n", j - i);
    else if( table->keys[i] == (char *) 0x1 )
      fprintf(stderr, "0x1 * %d\n", j - i);
    else if( table->key_debug == NULL )
      fprintf(stderr, "%p (hash_code %d) * %d\n", table->keys[i],
	table->hash_codes[i], j - i);
    else
    {
      table->key_debug(table->keys[i], fp);
      fprintf(stderr, " (hash_code %d) * %d\n", table->hash_codes[i], j - i);
    }
  }
  fprintf(stderr, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void HpTableImplClear(HP_GROUP *table)                                   */
/*                                                                           */
/*  Clear table, setting the trigger to 3/4 of its capacity.                 */
/*  The values are not cleared, but that does not matter.                    */
/*                                                                           */
/*****************************************************************************/

void HpTableImplClear(HP_GROUP *table)
{
  int i;
  /* invariant might not hold when clearing table, because its contents */
  /* might have already been cleared */
  /* HpTableImplCheck(table, "at start of HpTableImplClear"); */
  for( i = 0;  i < table->size;  i++ )
    table->keys[i] = NULL, table->hash_codes[i] = -1;
  table->trigger = (3 * table->size) / 4;
  table->pos = -1;
  HpTableImplCheck(table, "at end of HpTableImplClear");
}


/*****************************************************************************/
/*                                                                           */
/*  void *HpTableImplCheckRehash(HP_GROUP *table, char *values)              */
/*                                                                           */
/*  Rehash table into just over twice as much space as it occupies now,      */
/*  if the trigger requires it.                                              */
/*                                                                           */
/*  Implementation note.  Since the keys and values need to take up new      */
/*  positions, new space for keys and values is allocated, the old keys and  */
/*  values are copied over, and the old space for keys and values is freed.  */
/*                                                                           */
/*****************************************************************************/

void *HpTableImplCheckRehash(HP_GROUP *table, char *values)
{
  int i, old_size, count;  void *key;  void **old_keys;
  int *old_hash_codes;  char *old_values;  HA_ARENA a;
  HpTableImplCheck(table, "at start of HpTableImplCheckRehash");
  if( table->trigger <= 1 )
  {
    if( DEBUG1 || DEBUG2 )
    {
      fprintf(stderr, "  [ HpTableRehash(table %p)\n", (void *) table);
      HpTableImplDebug(table, 2, stderr);
    }

    /* save old size, keys, hash_codes, and values */
    old_size = table->size;
    old_keys = table->keys;
    old_hash_codes = table->hash_codes;
    old_values = values;

    /* re-initialize table to the next larger size, all clear */
    table->size = (old_size == 0 ? 5 : 2 * old_size + 1);
    table->trigger = (3 * table->size) / 4;
    table->pos = -1;
    a = HaResizableArena(table->keys);
    table->keys = HaResizableAlloc(a);
    table->keys = HaResizableReAlloc(table->keys, table->size * sizeof(char *));
    table->hash_codes = HaResizableAlloc(a);
    table->hash_codes = HaResizableReAlloc(table->hash_codes,
      table->size * sizeof(int));
    values = HaResizableAlloc(a);
    values = HaResizableReAlloc(values, table->size * table->value_size);
    for( i = 0;  i < table->size;  i++ )
      table->keys[i] = NULL, table->hash_codes[i] = -1;

    /* insert the old keys and values into table, then free their arrays */
    count = 0;
    for( i = 0;  i < old_size;  i++ )
    {
      key = old_keys[i];
      if( key > (void *) 0x1 )
      {
	HpTableImplAddHashed(table, old_hash_codes[i], key);
	memcpy(&values[table->pos * table->value_size],
	  &old_values[i * table->value_size], table->value_size);
	count++;
      }
    }
    HaResizableFree(old_keys);
    HaResizableFree(old_hash_codes);
    HaResizableFree(old_values);

    if( DEBUG1 || DEBUG2 )
    {
      fprintf(stderr, "  HpTableRehash ret. (count %d, old size %d, new size "
	"%d, new trigger %d)\n", count, old_size, table->size, table->trigger);
      HpTableImplDebug(table, 4, stderr);
      fprintf(stderr, "  ] HpTableRehash\n");
    }
    if( DEBUG3 )
      fprintf(stderr, "  rehashed\n");
  }
  HpTableImplCheck(table, "at end of HpTableImplCheckRehash");
  return (void *) values;
}


/*****************************************************************************/
/*                                                                           */
/*  void *HpTableImplCheckRehashNoValues(HP_GROUP *table)                    */
/*                                                                           */
/*  Like HpTableImplCheckRehash except there are no values.                  */
/*                                                                           */
/*****************************************************************************/

void HpTableImplCheckRehashNoValues(HP_GROUP *table)
{
  int i, old_size;  void *key;  void **old_keys;  int *old_hash_codes;
  HA_ARENA a;
  HpTableImplCheck(table, "at start of HpTableImplCheckRehashNoValues");
  if( table->trigger <= 1 )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  [ HpTableRehash(table %p)\n", (void *) table);
      HpTableImplDebug(table, 2, stderr);
    }

    /* save old size and keys */
    old_size = table->size;
    old_keys = table->keys;
    old_hash_codes = table->hash_codes;

    /* re-initialize table to the next larger size, all clear */
    table->size = (old_size == 0 ? 5 : 2 * old_size + 1);
    table->trigger = (3 * table->size) / 4;
    table->pos = -1;
    a = HaResizableArena(table->keys);
    table->keys = HaResizableAlloc(a);
    table->keys = HaResizableReAlloc(table->keys, table->size * sizeof(char *));
    table->hash_codes = HaResizableAlloc(a);
    table->hash_codes = HaResizableReAlloc(table->hash_codes,
      table->size * sizeof(int));
    for( i = 0;  i < table->size;  i++ )
      table->keys[i] = NULL, table->hash_codes[i] = -1;

    /* insert the old keys into table, then free their arrays */
    for( i = 0;  i < old_size;  i++ )
    {
      key = old_keys[i];
      if( key > (void *) 0x1 )
	HpTableImplAddHashed(table, old_hash_codes[i], key);
    }
    HaResizableFree(old_keys);
    HaResizableFree(old_hash_codes);

    if( DEBUG1 )
    {
      fwprintf(stderr, L"  at end:\n");
      HpTableImplDebug(table, 2, stderr);
      fprintf(stderr, "  ] HpTableRehash\n");
    }
  }
  HpTableImplCheck(table, "at end of HpTableImplCheckRehashNoValues");
}


/*****************************************************************************/
/*                                                                           */
/*  void HpTableImplAddHashed(HP_GROUP *table, int hash_code, void *key)     */
/*                                                                           */
/*  Add key to table and set table->pos to its position in the keys array,   */
/*  so that the caller can insert the value in a type-safe manner.  Any      */
/*  necessary rehash will have already been done.                            */
/*                                                                           */
/*****************************************************************************/

void HpTableImplAddHashed(HP_GROUP *table, int hash_code, void *key)
{
  int i;
  HpTableImplCheck(table, "at start of HpTableImplAddHashed");
  if( DEBUG1 )
  {
    fprintf(stderr, "[ HpTableImplAdd(%p, %p)\n", (void *) table, key);
    HpTableImplDebug(table, 2, stderr);
  }
  i = hash_code % table->size;
  while( table->keys[i] > (void *) 0x1 )
    i = (i + 1) % table->size;
  if( table->keys[i] == 0x0 )
    --(table->trigger);
  table->keys[i] = key;
  table->hash_codes[i] = hash_code;
  table->pos = i;
  if( DEBUG1 )
  {
    fwprintf(stderr, L"  at end:\n");
    HpTableImplDebug(table, 2, stderr);
    fprintf(stderr, "] HpTableImplAdd returning (pos = %d)\n", i);
  }
  HpTableImplCheck(table, "at end of HpTableImplAddHashed");
}


/*****************************************************************************/
/*                                                                           */
/*  bool HpTableImplAddUniqueHashed(HP_GROUP *table, int hash_code,          */
/*    void *key)                                                             */
/*                                                                           */
/*  If there is currently no entry in the table with this key, insert key    */
/*  into the table and set table->pos to its position in the keys array, so  */
/*  the caller can assign the value in a type-safe manner, and return true.  */
/*                                                                           */
/*  If there is currently an entry in the table with this key, set           */
/*  table->pos to the position of that entry, so that the caller             */
/*  can pass it on to the user in a type-safe manner, and return false.      */
/*                                                                           */
/*  Any necessary rehash will have already been done.                        */
/*                                                                           */
/*****************************************************************************/

bool HpTableImplAddUniqueHashed(HP_GROUP *table, int hash_code, void *key)
{
  int i, insert_pos;
  if( DEBUG1 )
    fprintf(stderr, "[ HpTableImplAddUnique(%p, %p)\n", (void *) table, key);
  HpTableImplCheck(table, "at start of HpTableImplAddUniqueHashed");
  insert_pos = -1;
  i = hash_code % table->size;
#if HP_DEBUG_PROBE_LENGTH
  table->op_count++;
#endif
  while( table->keys[i] != 0x0 )
  {
#if HP_DEBUG_PROBE_LENGTH
    table->op_cost++;
#endif
    if( table->keys[i] == (void *) 0x1 )
    {
      /* not a true key; we may insert here later */
      insert_pos = i;
    }
    else if( hash_code == table->hash_codes[i] &&
      HpGroupKeyEqualFn(*table)(key, table->keys[i]) )
    {
      table->pos = i;
      if( DEBUG1 )
	fprintf(stderr, "] HpTableImplAddUnique ret. false (pos %d)\n", i);
      HpTableImplCheck(table, "at false end of HpTableImplAddUniqueHashed");
      return false;
    }
    i = (i + 1) % table->size;
  }
  if( insert_pos == -1 )
  {
    insert_pos = i;
    --(table->trigger);
  }
  table->keys[insert_pos] = key;
  table->hash_codes[insert_pos] = hash_code;
  table->pos = insert_pos;
  if( DEBUG1 )
    fprintf(stderr, "] HpTableImplAddUnique returning true (pos %d)\n",
      insert_pos);
  HpTableImplCheck(table, "at true end of HpTableImplAddUniqueHashed");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HpTableImplContainsHashed(HP_GROUP *table, int hash_code,           */
/*    void *key, int *pos)                                                   */
/*                                                                           */
/*  Carry out a retrieval of key from table, assuming that hash_code is      */
/*  its hash code.  If there is an entry with this key, return true and set  */
/*  *pos to its index; if not, return false and set *pos to the index of     */
/*  the gap that proves that key is not present.                             */
/*                                                                           */
/*****************************************************************************/

bool HpTableImplContainsHashed(HP_GROUP *table, int hash_code,
  void *key, int *pos)
{
  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ HpTableImplContainsHashed(%p, %d, %p)\n",
      (void *) table, hash_code, key);
  HpTableImplCheck(table, "at start of HpTableImplContainsHashed");
  i = hash_code % table->size;
#if HP_DEBUG_PROBE_LENGTH
  table->op_count++;
#endif
  while( table->keys[i] != 0x0 )
  {
#if HP_DEBUG_PROBE_LENGTH
    table->op_cost++;
#endif
    if( table->keys[i] > (void *) 0x1 && hash_code == table->hash_codes[i] &&
	HpGroupKeyEqualFn(*table)(key, table->keys[i]) )
    {
      if( DEBUG1 )
	fprintf(stderr,
	  "] HpTableImplContainsHashed returning true (pos %d)\n", i);
      *pos = i;
      HpTableImplCheck(table, "at true end of HpTableImplContainsHashed");
      return true;
    }
    i = (i + 1) % table->size;
  }
  if( DEBUG1 )
    fprintf(stderr,
      "] HpTableImplContainsHashed returning false (pos %d)\n", i);
  *pos = i;
  HpTableImplCheck(table, "at false end of HpTableImplContainsHashed");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HpTableImplContainsNext(HP_GROUP *table, int *pos)                  */
/*                                                                           */
/*  It is a precondition of this function that *pos is the index of an       */
/*  entry of table.  Search onwards from that entry for another entry        */
/*  with the same key as this one.  If found, set *pos to the index of       */
/*  that entry and return true; if not found, set *pos to the index of the   */
/*  gap that proves there is no entry, and return false.                     */
/*                                                                           */
/*****************************************************************************/

bool HpTableImplContainsNext(HP_GROUP *table, int *pos)
{
  void *key;   int i, hash_code;
  HpTableImplCheck(table, "at start of HpTableImplContainsNext");
  i = *pos;
  key = table->keys[i];
  hash_code = table->hash_codes[i];
  if( DEBUG1 )
    fprintf(stderr, "[ HpTableImplRetrieveNext(%p, %d holding %p)\n",
      (void *) table, i, key);
  i = (i + 1) % table->size;
#if HP_DEBUG_PROBE_LENGTH
  table->op_count++;
#endif
  while( table->keys[i] != 0x0 )
  {
#if HP_DEBUG_PROBE_LENGTH
    table->op_cost++;
#endif
    if( table->keys[i] > (void *) 0x1 && hash_code == table->hash_codes[i] &&
	HpGroupKeyEqualFn(*table)(key, table->keys[i]) )
    {
      if( DEBUG1 )
	fprintf(stderr,"] HpTableImplRetrieveNext returning true (pos %d)\n",i);
      *pos = i;
      HpTableImplCheck(table, "at true end of HpTableImplContainsNext");
      return true;
    }
    i = (i + 1) % table->size;
  }
  if( DEBUG1 )
    fprintf(stderr, "] HpTableImplRetrieveNext returning false (pos %d)\n", i);
  *pos = i;
  HpTableImplCheck(table, "at false end of HpTableImplContainsNext");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void HpTableImplDelete(HP_GROUP *table, int pos)                         */
/*                                                                           */
/*  Delete the entry at this position.  It must be present.                  */
/*                                                                           */
/*****************************************************************************/

void HpTableImplDelete(HP_GROUP *table, int pos)
{
  HnAssert(table->keys[pos] > (void *) 0x1, "HpTableDelete: no entry at pos");
  if( DEBUG1 )
    fprintf(stderr, "[ HpTableImplDelete(%p, %d %p)\n", (void *) table,
      pos, table->keys[pos]);
  HpTableImplCheck(table, "at start of HpTableImplDelete");
  if( table->keys[(pos + 1) % table->size] == 0x0 )
  {
    /* safe to set this entry to NULL, since nothing follows */
    table->keys[pos] = 0x0;
    (table->trigger)++;
  }
  else
    table->keys[pos] = (void *) 0x1;
  table->hash_codes[pos] = -1;
  HpTableImplCheck(table, "at end of HpTableImplDelete");
}


/*****************************************************************************/
/*                                                                           */
/*  float HpTableImplProbeLength(HP_GROUP *table)                            */
/*                                                                           */
/*  Return the average probe length of table.                                */
/*                                                                           */
/*****************************************************************************/

float HpTableImplProbeLength(HP_GROUP *table)
{
#if HP_DEBUG_PROBE_LENGTH
  if( table->op_count == 0 )
    return -1.0;
  else
    return (float) table->op_cost / (float) table->op_count;
#else
  return -1.0;
#endif
}

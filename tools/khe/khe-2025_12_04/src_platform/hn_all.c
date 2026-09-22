
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
/*  FILE:     hn_all.c                                                       */
/*  PURPOSE:  implementation file for the Howard Narrow String Library       */
/*                                                                           */
/*****************************************************************************/
#include "howard_n.h"
#include <string.h>

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  Submodule "Narrow string construction"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HnStringAdd(HA_ARRAY_NCHAR *anc, const char *format, ...)           */
/*                                                                           */
/*  Like snprintf, except the resulting characters are appended to *anc and  */
/*  this function works out the length by itself.                            */
/*                                                                           */
/*  Implementation note.  Since this function has no idea how many chars     */
/*  will be written, it first tries writing into a stack buffer of moderate  */
/*  size, and then tries a sequence of increasingly large heap buffers.      */
/*                                                                           */
/*  According to "man vsnprintf", vsnprintf returns the number of wide       */
/*  characters which would have been written had space been available,       */
/*  excluding the terminating null wide character, or a negative number      */
/*  when an error occurs.                                                    */
/*                                                                           */
/*****************************************************************************/
#define HN_INIT_LEN 200

void HnStringAdd(HA_ARRAY_NCHAR *anc, const char *format, ...)
{
  va_list args;  char buff[HN_INIT_LEN];  int i, old_len, len;

  /* try writing into buff */
  va_start(args, format);
  len = vsnprintf(buff, HN_INIT_LEN, format, args);
  va_end(args);
  if( len < HN_INIT_LEN )
  {
    for( i = 0;  buff[i] != L'\0';  i++ )
      HaArrayAddLast(*anc, buff[i]);
    return;
  }

  /* now we know how much memory we need, write into *anc */
  old_len = HaArrayCount(*anc);
  HaArrayFill(*anc, old_len + len + 1, '\0');
  va_start(args, format);
  vsnprintf(&HaArray(*anc, old_len), len + 1, format, args);
  va_end(args);
  HaArrayDeleteLast(*anc);
}


/*****************************************************************************/
/*                                                                           */
/*  void HnStringVAdd(HA_ARRAY_NCHAR *anc, const char *format,               */
/*    va_list args)                                                          */
/*                                                                           */
/*  Like HsStringAdd except that the variable arg list comes as a            */
/*  single argument of type va_list.                                         */
/*                                                                           */
/*****************************************************************************/

void HnStringVAdd(HA_ARRAY_NCHAR *anc, const char *format, va_list args)
{
  char buff[HN_INIT_LEN];  int i, old_len, len;

  /* try writing into buff */
  len = vsnprintf(buff, HN_INIT_LEN, format, args);
  if( len < HN_INIT_LEN )
  {
    for( i = 0;  buff[i] != '\0';  i++ )
      HaArrayAddLast(*anc, buff[i]);
    return;
  }

  /* now we know how much memory we need, write into *anc */
  old_len = HaArrayCount(*anc);
  HaArrayFill(*anc, old_len + len + 1, '\0');
  vsnprintf(&HaArray(*anc, old_len), len + 1, format, args);
  HaArrayDeleteLast(*anc);
}


/*****************************************************************************/
/*                                                                           */
/*  void HnStringAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,                   */
/*    const char *format, ...)                                               */
/*                                                                           */
/*  Like HnStringAdd if anc is non-NULL, otherwise write to fp.              */
/*                                                                           */
/*****************************************************************************/

void HnStringAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,
  const char *format, ...)
{
  va_list args;
  va_start(args, format);
  if( anc != NULL )
    HnStringVAdd(anc, format, args);
  else
    vfprintf(fp, format, args);
  va_end(args);
}


/*****************************************************************************/
/*                                                                           */
/*  void HnStringVAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,                  */
/*    const char *format, va_list args)                                      */
/*                                                                           */
/*  Like HnStringVAdd if anc is non-NULL, otherwise write to fp.             */
/*                                                                           */
/*****************************************************************************/

void HnStringVAddOrPrint(HA_ARRAY_NCHAR *anc, FILE *fp,
  const char *format, va_list args)
{
  if( anc != NULL )
    HnStringVAdd(anc, format, args);
  else
    vfprintf(fp, format, args);
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringCopy(char *s, HA_ARENA a)                                  */
/*                                                                           */
/*  Return a copy of s in arena memory.                                      */
/*                                                                           */
/*****************************************************************************/

char *HnStringCopy(char *s, HA_ARENA a)
{
  char *res;
  if( s == NULL )
    return NULL;
  else
  {
    res = (char *) HaAlloc(a, (strlen(s) + 1) * sizeof(char));
    strcpy(res, s);
    return res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringSubstring(char *s, int start, int len, HA_ARENA a)         */
/*                                                                           */
/*  Return the substring of s starting at position start of length len       */
/*  (or less if the string is exhausted first);                              */
/*                                                                           */
/*****************************************************************************/

char *HnStringSubstring(char *s, int start, int len, HA_ARENA a)
{
  char *res;  int s_len, res_len, i;

  /* find res_len, the length of the result string */
  s_len = strlen(s);
  HnAssert(start <= s_len, "HnStringSubstring: start (%d) > wcslen(s) (%d)",
    start, s_len);
  res_len = s_len - start;
  if( len < res_len )
    res_len = len;

  /* build the result in res_len + 1 characters of arena memory */
  res = HaAlloc(a, (res_len + 1) * sizeof(char));
  for( i = 0;  i < res_len;  i++ )
    res[i] = s[start + i];
  res[res_len] = '\0';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringMake(HA_ARENA a, const char *format, ...)                  */
/*                                                                           */
/*  Make a formatted string in heap memory.                                  */
/*                                                                           */
/*****************************************************************************/

char *HnStringMake(HA_ARENA a, const char *format, ...)
{
  va_list args;  char *mem, buff[HN_INIT_LEN];  int len1;

  /* try writing into buff */
  va_start(args, format);
  len1 = vsnprintf(buff, HN_INIT_LEN, format, args);
  va_end(args);
  if( len1 < HN_INIT_LEN )
    return HnStringCopy(buff, a);

  /* now we know how much memory we need, write into mem */
  mem = HaAlloc(a, (len1 + 1) * sizeof(char));
  va_start(args, format);
  vsnprintf(mem, len1 + 1, format, args);
  va_end(args);
  return mem;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringVMake(HA_ARENA a, const char *format, va_list args)        */
/*                                                                           */
/*  Like HnStringMake only with a va_list instead of ...                     */
/*                                                                           */
/*****************************************************************************/

char *HnStringVMake(HA_ARENA a, const char *format, va_list args)
{
  char *mem, buff[HN_INIT_LEN];  int len1;

  /* try writing into buff */
  len1 = vsnprintf(buff, HN_INIT_LEN, format, args);
  if( len1 < HN_INIT_LEN )
    return HnStringCopy(buff, a);

  /* now we know how much memory we need, write into mem */
  mem = HaAlloc(a, (len1 + 1) * sizeof(char));
  vsnprintf(mem, len1 + 1, format, args);
  return mem;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringFromWide(wchar_t *s, HA_ARENA a)                           */
/*                                                                           */
/*  Convert s from narrow to wide.                                           */
/*                                                                           */
/*****************************************************************************/

char *HnStringFromWide(wchar_t *s, HA_ARENA a)
{
  return HnStringMake(a, "%ls", s);
}


/*****************************************************************************/
/*                                                                           */
/*  wchar_t *HnStringToWide(char *s, HA_ARENA a)                             */
/*                                                                           */
/*  Convert s from narrow to wide.                                           */
/*                                                                           */
/*****************************************************************************/

wchar_t *HnStringToWide(char *s, HA_ARENA a)
{
  wchar_t *mem, buff[HN_INIT_LEN];  int len1, len2;
  len1 = swprintf(buff, HN_INIT_LEN, L"%s", s);
  mem = HaAlloc(a, (len1 + 1) * sizeof(wchar_t));
  if( len1 < HN_INIT_LEN )
    return wcscpy(mem, buff);
  len2 = swprintf(mem, len1 + 1, L"%s", s);
  HnAssert(len2 == len1, "HnStringToWide internal error");
  return mem;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Narrow string query"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool HnStringContains(char *s, char *substr, int *pos)                   */
/*                                                                           */
/*  If s contains substr as a substring, return true with *pos set to        */
/*  the index of the start of the first occurrence of substr in s.  If       */
/*  not, return false with *pos untouched.                                   */
/*                                                                           */
/*****************************************************************************/

bool HnStringContains(char *s, char *substr, int *pos)
{
  char *res;
  res = strstr(s, substr);
  if( res == NULL )
    return false;
  else
  {
    *pos = (res - s);
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool HnStringBeginsWith(char *s, char *prefix)                           */
/*                                                                           */
/*  Return true if s begins with prefix.                                     */
/*                                                                           */
/*****************************************************************************/

bool HnStringBeginsWith(char *s, char *prefix)
{
  int i;
  for( i = 0;  prefix[i] != L'\0';  i++ )
    if( s[i] != prefix[i] )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool MStringEndsWith(char *s, char *suffix)                              */
/*                                                                           */
/*  Return true if s ends with suffix.                                       */
/*                                                                           */
/*****************************************************************************/

bool HnStringEndsWith(char *s, char *suffix)
{
  int suffix_len = strlen(suffix);
  int s_len = strlen(s);
  return s_len >= suffix_len && strcmp(&s[s_len - suffix_len], suffix) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HnStringIsWhiteSpaceOnly(char *s)                                   */
/*                                                                           */
/*  Return true if s is NULL or contains white space only.                   */
/*                                                                           */
/*****************************************************************************/
#define is_white(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

bool HnStringIsWhiteSpaceOnly(char *s)
{
  char *p;
  if( s == NULL )
    return true;
  for( p = s;  *p != '\0';  p++ )
    if( !is_white(*p) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *HnStringCopyStripped(char *s, HA_ARENA a)                          */
/*                                                                           */
/*  Return a copy of s with white space stripped from each end.              */
/*                                                                           */
/*****************************************************************************/

char *HnStringCopyStripped(char *s, HA_ARENA a)
{
  HA_ARRAY_NCHAR anc;  char *p, *q, *r;

  /* return "" s is NULL */
  if( s == NULL )
    return "";

  /* find p, the address of the first non-white-space character */
  for( p = s;  *p != '\0';  p++ )
    if( !is_white(*p) )
      break;

  /* return "" if there is no first non-white-space character */
  if( *p == '\0' )
    return "";

  /* find q, the address of the last non-white-space character; it must exist */
  for( q = &s[strlen(s) - 1];  q >= s;  q-- )
    if( !is_white(*q) )
      break;

  /* the result is everything from p to q inclusive */
  HnStringBegin(anc, a);
  for( r = p;  r <= q;  r++ )
    HaArrayAddLast(anc, *r);
  return HnStringEnd(anc);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Abort and assert"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void HnFatal(char *fmt, ...)                                             */
/*                                                                           */
/*  Print an error message on stderr and exit(1).                            */
/*                                                                           */
/*****************************************************************************/

void HnFatal(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}


/*****************************************************************************/
/*                                                                           */
/*  void HnAbort(char *fmt, ...)                                             */
/*                                                                           */
/*  Print an error message on stderr and abort.                              */
/*                                                                           */
/*****************************************************************************/

void HnAbort(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}


/*****************************************************************************/
/*                                                                           */
/*  void HnAssert(bool cond, char *fmt, ...)                                 */
/*                                                                           */
/*  If cond is false, print an error message on stderr and abort.            */
/*                                                                           */
/*****************************************************************************/

void HnAssert(bool cond, char *fmt, ...)
{
  va_list args;
  if( !cond )
  {
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Narrow string symbol tables"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int HnTableHash(char *key)                                               */
/*                                                                           */
/*  Return the hash code of key, before reduction modulo the table size.     */
/*                                                                           */
/*  Implementation note.  This hash function adds the characters together,   */
/*  shifting each subtotal one place to the left.  If the total is negative  */
/*  it is negated to make the result non-negative.                           */
/*                                                                           */
/*****************************************************************************/

int HnTableHash(char *key)
{
  char *p;  int res;
  res = 0;
  for( p = key;  *p != L'\0';  p++ )
    res = (res << 1) + *p;
  if( res < 0 )  res = - res;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void HnTableImplDebug(HN_TABLE_U *table, int indent, FILE *fp)           */
/*                                                                           */
/*  Debug print of *table onto fp with the given indent.                     */
/*                                                                           */
/*****************************************************************************/

static void HnTableImplDebug(HN_TABLE_U *table, int indent, FILE *fp)
{
  int i;
  fwprintf(stderr, L"%*ls[ HnTable(%p)\n", indent, L"", (void *) table);
  for( i = 0;  i < table->size;  i++ )
  {
    fwprintf(stderr, L"%*ls  %d: ", indent, L"", i);
    if( table->keys[i] == 0x0 )
      fwprintf(stderr, L"0x0\n");
    else if( table->keys[i] == (char *) 0x1 )
      fwprintf(stderr, L"0x1\n");
    else
      fwprintf(stderr, L"%s\n", table->keys[i]);
  }
  fwprintf(stderr, L"%*ls]\n", indent, L"");
}


/*****************************************************************************/
/*                                                                           */
/*  void HnTableImplClear(HN_TABLE_U *table)                                 */
/*                                                                           */
/*  Clear table, setting the trigger to 4/5 of its capacity.                 */
/*  The values are not cleared, but that does not matter.                    */
/*                                                                           */
/*****************************************************************************/

void HnTableImplClear(HN_TABLE_U *table)
{
  int i;
  for( i = 0;  i < table->size;  i++ )
    table->keys[i] = NULL;
  table->trigger = (4 * table->size) / 5;
  table->pos = -1;
}


/*****************************************************************************/
/*                                                                           */
/*  void *HnTableImplCheckRehash(HN_TABLE_U *table, char *values)            */
/*                                                                           */
/*  Rehash table into just over twice as much space as it occupies now,      */
/*  if the trigger requires it.                                              */
/*                                                                           */
/*  Implementation note.  Since the keys and values need to take up new      */
/*  positions, new space for keys and values is allocated, the old keys and  */
/*  values are copied over, and the old space for keys and values is freed.  */
/*                                                                           */
/*****************************************************************************/

void *HnTableImplCheckRehash(HN_TABLE_U *table, char *values)
{
  int i, old_size;  char *key;  char **old_keys;  char *old_values;
  HA_ARENA a;
  if( table->trigger <= 1 )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  [ HnTableRehash(table %p)\n", (void *) table);
      HnTableImplDebug(table, 2, stderr);
    }

    /* save old size, keys, and values */
    old_size = table->size;
    old_keys = table->keys;
    old_values = values;

    /* re-initialize table to the next larger size, all clear */
    table->size = (old_size == 0 ? 5 : 2 * old_size + 1);
    table->trigger = (4 * table->size) / 5;
    table->pos = -1;
    a = HaResizableArena(table->keys);
    table->keys = HaResizableAlloc(a);
    table->keys = HaResizableReAlloc(table->keys, table->size * sizeof(char *));
    values = HaResizableAlloc(a);
    values = HaResizableReAlloc(values, table->size * table->value_size);
    /* ***
    table->keys = (char **) call oc(table->size, sizeof(char *));
    values = (char *) call oc(table->size, table->value_size);
    *** */
    for( i = 0;  i < table->size;  i++ )
      table->keys[i] = NULL;

    /* insert the old keys and values into table, then free their arrays */
    for( i = 0;  i < old_size;  i++ )
    {
      key = old_keys[i];
      if( key > (char *) 0x1 )
      {
	HnTableImplAddHashed(table, HnTableHash(key), key);
	memcpy(&values[table->pos * table->value_size],
	  &old_values[i * table->value_size], table->value_size);
      }
    }
    HaResizableFree(old_keys);
    HaResizableFree(old_values);

    if( DEBUG1 )
    {
      fwprintf(stderr, L"  at end:\n");
      HnTableImplDebug(table, 2, stderr);
      fprintf(stderr, "  ] HnTableRehash\n");
    }
  }
  return (void *) values;
}


/*****************************************************************************/
/*                                                                           */
/*  void HnTableImplAddHashed(HN_TABLE_U *table, int hash_code, char *key)   */
/*                                                                           */
/*  Add key to table and set table->pos to its position in the keys array,   */
/*  so that the caller can insert the value in a type-safe manner.  Any      */
/*  necessary rehash with have already been done.                            */
/*                                                                           */
/*****************************************************************************/

void HnTableImplAddHashed(HN_TABLE_U *table, int hash_code, char *key)
{
  int i;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ HnTableImplAdd(%p, \"%s\")\n", (void *) table, key);
    HnTableImplDebug(table, 2, stderr);
  }
  i = hash_code % table->size;
  while( table->keys[i] > (char *) 0x1 )
    i = (i + 1) % table->size;
  if( table->keys[i] == 0x0 )
    --(table->trigger);
  table->keys[i] = key;
  table->pos = i;
  if( DEBUG1 )
  {
    fwprintf(stderr, L"  at end:\n");
    HnTableImplDebug(table, 2, stderr);
    fprintf(stderr, "] HnTableImplAdd returning (pos = %d)\n", i);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool HnTableImplAddUniqueHashed(HN_TABLE_U *table, int hash_code,        */
/*    char *key)                                                             */
/*                                                                           */
/*  If there is currently no entry in the table with this key, insert key    */
/*  into the table and set table->pos to its position in the keys array, so  */
/*  the caller can assign the value in a type-safe manner, and return true.  */
/*                                                                           */
/*  If there is currently an entry in the table with this key, set           */
/*  table->pos to the position of that entry, so that the caller             */
/*  can pass it on to the user in a type-safe manner, and return false.      */
/*                                                                           */
/*  Any necessary rehash with have already been done.                        */
/*                                                                           */
/*****************************************************************************/

bool HnTableImplAddUniqueHashed(HN_TABLE_U *table, int hash_code, char *key)
{
  int i, insert_pos;
  if( DEBUG1 )
    fprintf(stderr, "[ HnTableImplAddUnique(%p, \"%s\")\n",
      (void *) table, key);
  insert_pos = -1;
  i = hash_code % table->size;
  while( table->keys[i] != 0x0 )
  {
    if( table->keys[i] == (char *) 0x1 )
    {
      /* not a true key; we may insert here later */
      insert_pos = i;
    }
    else if( strcmp(key, table->keys[i]) == 0 )
    {
      table->pos = i;
      if( DEBUG1 )
	fprintf(stderr, "] HnTableImplAddUnique ret. false (pos %d)\n", i);
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
  table->pos = insert_pos;
  if( DEBUG1 )
    fprintf(stderr, "] HnTableImplAddUnique returning true (pos %d)\n",
      insert_pos);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HnTableImplContainsHashed(HN_TABLE_U *table, int hash_code,         */
/*    char *key, int *pos)                                                   */
/*                                                                           */
/*  Carry out a retrieval of key from table, assuming that hash_code is      */
/*  its hash code.  If there is an entry with this key, return true and set  */
/*  *pos to its index; if not, return false and set *pos to the index of     */
/*  the gap that proves that key is not present.                             */
/*                                                                           */
/*****************************************************************************/

bool HnTableImplContainsHashed(HN_TABLE_U *table, int hash_code,
  char *key, int *pos)
{
  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ HnTableImplContainsHashed(%p, %d, \"%s\")\n",
      (void *) table, hash_code, key);
  i = hash_code % table->size;
  while( table->keys[i] != 0x0 )
  {
    if( table->keys[i] > (char *) 0x1 && strcmp(key, table->keys[i]) == 0 )
    {
      if( DEBUG1 )
	fprintf(stderr,
	  "] HnTableImplContainsHashed returning true (pos %d)\n", i);
      *pos = i;
      return true;
    }
    i = (i + 1) % table->size;
  }
  if( DEBUG1 )
    fprintf(stderr,
      "] HnTableImplContainsHashed returning false (pos %d)\n", i);
  *pos = i;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool HnTableImplContainsNext(HN_TABLE_U *table, int *pos)                */
/*                                                                           */
/*  It is a precondition of this function that *pos is the index of an       */
/*  entry of table.  Search onwards from that entry for another entry        */
/*  with the same key as this one.  If found, set *pos to the index of       */
/*  that entry and return true; if not found, set *pos to the index of the   */
/*  gap that proves there is no entry, and return false.                     */
/*                                                                           */
/*****************************************************************************/

bool HnTableImplContainsNext(HN_TABLE_U *table, int *pos)
{
  char *key;   int i;
  i = *pos;
  key = table->keys[i];
  if( DEBUG1 )
    fprintf(stderr, "[ HnTableImplRetrieveNext(%p, %d holding \"%s\")\n",
      (void *) table, i, key);
  i = (i + 1) % table->size;
  while( table->keys[i] != 0x0 )
  {
    if( table->keys[i] > (char *) 0x1 && strcmp(key, table->keys[i]) == 0 )
    {
      if( DEBUG1 )
	fprintf(stderr,"] HnTableImplRetrieveNext returning true (pos %d)\n",i);
      *pos = i;
      return true;
    }
    i = (i + 1) % table->size;
  }
  if( DEBUG1 )
    fprintf(stderr, "] HnTableImplRetrieveNext returning false (pos %d)\n", i);
  *pos = i;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void HnTableImplDelete(HN_TABLE_U *table, int pos)                       */
/*                                                                           */
/*  Delete the entry at this position.  It must be present.                  */
/*                                                                           */
/*****************************************************************************/

void HnTableImplDelete(HN_TABLE_U *table, int pos)
{
  HnAssert(table->keys[pos] > (char *) 0x1, "HnTableDelete: no entry at pos");
  if( DEBUG1 )
    fprintf(stderr, "[ HnTableImplDelete(%p, %d \"%s\")\n", (void *) table,
      pos, table->keys[pos]);
  if( table->keys[(pos + 1) % table->size] == 0x0 )
  {
    /* safe to set this entry to NULL, since nothing follows */
    table->keys[pos] = 0x0;
    (table->trigger)++;
  }
  else
    table->keys[pos] = (char *) 0x1;
}

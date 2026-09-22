
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
/*  FILE:         khe_sm_options.c                                           */
/*  DESCRIPTION:  Options for solvers                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS                                                              */
/*                                                                           */
/*****************************************************************************/

typedef HN_TABLE(char *)	TABLE_STRING;
typedef HN_TABLE(void *)	TABLE_OBJECT;

/* ***
typedef struct khe_options_object {
  void				*value;
  KHE_OPTIONS_VALUE_COPY_FN	value_copy_fn;
} *KHE_OPTIONS_OBJECT;
*** */

/* typedef MTABLE(KHE_OPTIONS_OBJECT) TABLE_KHE_OPTIONS_OBJECT; */

/* ***
typedef struct khe_options_time_rec {
  char				*tag;			** consistency check **
  KHE_STATS_TIMER		timer;			** timer             **
  float				time_limit;		** time limit        **
} *KHE_OPTIONS_TIME_LIMIT;

typedef HA_ARRAY(KHE_OPTIONS_TIME_LIMIT) ARRAY_KHE_OPTIONS_TIME_LIMIT;
*** */

struct khe_options_rec {
  HA_ARENA			arena;
  TABLE_STRING			string_table;
  TABLE_OBJECT			object_table;
  KHE_TIMER_SET			timer_set;
  /* ***
  ARRAY_KHE_OPTIONS_TIME_LIMIT	time_limits;
  ARRAY_KHE_OPTIONS_TIME_LIMIT	free_time_limits;
  *** */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS_OBJECT KheOptionsObjectMake(void *value,                     */
/*    KHE_OPTIONS_VALUE_COPY_FN value_copy_fn)                               */
/*                                                                           */
/*  Make a new options object with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_OPTIONS_OBJECT KheOptionsObjectMake(void *value,
  KHE_OPTIONS_VALUE_COPY_FN value_copy_fn)
{
  KHE_OPTIONS_OBJECT res;
  MMake(res);
  res->value = value;
  res->value_copy_fn = value_copy_fn;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsObjectDelete(KHE_OPTIONS_OBJECT oo)                       */
/*                                                                           */
/*  Delete oo.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheOptionsObjectDelete(KHE_OPTIONS_OBJECT oo)
{
  MFree(oo);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS KheOptionsMake(HA_ARENA a)                                   */
/*                                                                           */
/*  Make a new options object with default values for all options.           */
/*                                                                           */
/*****************************************************************************/

KHE_OPTIONS KheOptionsMake(HA_ARENA a)
{
  KHE_OPTIONS res;  /* HA_ARENA a; */
  /* a = HaAre naMake(); */
  HaMake(res, a);
  res->arena = a;
  HnTableInit(res->string_table, a);
  HnTableInit(res->object_table, a);
  res->timer_set = KheTimerSetMake(a);
  /* HaArrayInit(res->time_limits, a); */
  /* HaArrayInit(res->free_time_limits, a); */
  /* res->timer = KheStatsTimerMake(a); */
  /* res->time_limit = -1.0; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  HA_ARENA KheOptionsArena(KHE_OPTIONS options)                            */
/*                                                                           */
/*  Return the arena containing options.                                     */
/*                                                                           */
/*****************************************************************************/

HA_ARENA KheOptionsArena(KHE_OPTIONS options)
{
  return options->arena;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsDelete(KHE_OPTIONS options)                               */
/*                                                                           */
/*  Delete options.                                                          */
/*                                                                           */
/*****************************************************************************/

/* *** will be deleted when its arena is deleted
void KheOptionsDelete(KHE_OPTIONS options)
{
  HaArenaDel ete(options->arena);
  ** ***
  MTableFree(options->string_table);
  MTableFree(options->object_table);
  KheStatsTimerDelete(options->timer);
  MFree(options);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS KheOptionsCopy(KHE_OPTIONS options, HA_ARENA a)              */
/*                                                                           */
/*  Return a copy of options.                                                */
/*                                                                           */
/*****************************************************************************/

KHE_OPTIONS KheOptionsCopy(KHE_OPTIONS options, HA_ARENA a)
{
  KHE_OPTIONS res;  char *key, *value;  int pos;  void *obj;
  res = KheOptionsMake(a);
  HnTableForEach(options->string_table, key, value, pos)
    HnTableAdd(res->string_table, key, value);
  HnTableForEach(options->object_table, key, obj, pos)
    HnTableAdd(res->object_table, key, obj);
  res->timer_set = KheTimerSetCopy(options->timer_set, a);
  /* res->timer = KheStatsTimerMake(res->arena); */  /* don't copy here! */
  /* res->timer = KheStatsTimerCopy(options->timer, res->arena); */
  /* res->time_limit = options->time_limit; */
  return res;
}

/* ***
KHE_OPTIONS KheOptionsCopy(KHE_OPTIONS options)
{
  KHE_OPTIONS res;  char *key, *value;  int pos;  void *v;
  KHE_OPTIONS_OBJECT oo, oo2;
  res = KheOptionsMake();
  MTableForEach(options->string_table, &key, &value, &pos)
    MTableInsert(res->string_table, key, value);
  MTableForEach(options->object_table, &key, &oo, &pos)
  {
    v = (oo->value_copy_fn == NULL || oo->value == NULL ? oo->value :
      oo->value_copy_fn(oo->value));
    oo2 = KheOptionsObjectMake(v, oo->value_copy_fn);
    MTableInsert(res->object_table, key, oo2);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSet(KHE_OPTIONS options, char *key, char *value)          */
/*                                                                           */
/*  Set the value of key to value, replacing any previous value.             */
/*                                                                           */
/*****************************************************************************/

void KheOptionsSet(KHE_OPTIONS options, char *key, char *value)
{
  int pos;
  if( HnTableContains(options->string_table, key, pos) )
    HnTableReplace(options->string_table, pos, value);
  else
    HnTableAdd(options->string_table, key, value);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheOptionsGet(KHE_OPTIONS options, char *key, char *dft)           */
/*                                                                           */
/*  Get the value of key in options, or dft if none.                         */
/*                                                                           */
/*****************************************************************************/

char *KheOptionsGet(KHE_OPTIONS options, char *key, char *dft)
{
  char *value;  int pos;
  if( !HnTableRetrieve(options->string_table, key, value, pos) )
    return dft;
  else
    return value;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSetBool(KHE_OPTIONS options, char *key, bool value)       */
/*                                                                           */
/*  Set the value of key in options to the string equivalent of value.       */
/*                                                                           */
/*****************************************************************************/

void KheOptionsSetBool(KHE_OPTIONS options, char *key, bool value)
{
  KheOptionsSet(options, key, value ? "true" : "false");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsGetBool(KHE_OPTIONS options, char *key, bool dft)         */
/*                                                                           */
/*  Get the value of key in options, assumed to represent a Boolean          */
/*  value, which will be dft if the option is absent or "auto".              */
/*                                                                           */
/*****************************************************************************/

bool KheOptionsGetBool(KHE_OPTIONS options, char *key, bool dft)
{
  char *value;  int pos;
  if( !HnTableRetrieve(options->string_table, key, value, pos) )
    return dft;
  else if( strcmp(value, "false") == 0 )
    return false;
  else if( strcmp(value, "true") == 0 )
    return true;
  else
  {
    HnAbort("KheOptionsGetBool: key \"%s\" has non-bool value \"%s\"",
      key, value);
    return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSetInt(KHE_OPTIONS options, char *key, int value)         */
/*                                                                           */
/*  Set the value of key in options to the string equivalent of value.       */
/*                                                                           */
/*****************************************************************************/

void KheOptionsSetInt(KHE_OPTIONS options, char *key, int value)
{
  static char *values[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  KheOptionsSet(options, key, 0 <= value && value < 10 ? values[value] :
    HnStringMake(options->arena, "%d", value));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOptionsGetInt(KHE_OPTIONS options, char *key, int dft)            */
/*                                                                           */
/*  Get the value of key in options, assumed to represent an integer.        */
/*  Return dft if the option is absent.                                      */
/*                                                                           */
/*****************************************************************************/

int KheOptionsGetInt(KHE_OPTIONS options, char *key, int dft)
{
  char *value;  int pos, res;
  if( !HnTableRetrieve(options->string_table, key, value, pos) )
    return dft;
  else if( sscanf(value, "%d", &res) == 1 )
    return res;
  else
  {
    HnAbort("KheOptionsGetInt: key \"%s\" has non-int value \"%s\"",
      key, value);
    return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSetFloat(KHE_OPTIONS options, char *key, float value)     */
/*                                                                           */
/*  Set the value of key in options to the string equivalent of value.       */
/*                                                                           */
/*****************************************************************************/

void KheOptionsSetFloat(KHE_OPTIONS options, char *key, float value)
{
  KheOptionsSet(options, key, HnStringMake(options->arena, "%f", value));
}


/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsGetFloat(KHE_OPTIONS options, char *key, float dft)      */
/*                                                                           */
/*  Get the value of key in options, assumed to represent a float.           */
/*  Return dft if the option is absent.                                      */
/*                                                                           */
/*****************************************************************************/

float KheOptionsGetFloat(KHE_OPTIONS options, char *key, float dft)
{
  char *value;  int pos;  float res;
  if( !HnTableRetrieve(options->string_table, key, value, pos) )
    return dft;
  else if( sscanf(value, "%f", &res) == 1 )
    return res;
  else
  {
    HnAbort("KheOptionsGetFloat: key \"%s\" has non-float value \"%s\"",
      key, value);
    return 0.0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSetObject(KHE_OPTIONS options, char *key, void *value,    */
/*    KHE_OPTIONS_VALUE_COPY_FN value_copy_fn)                               */
/*                                                                           */
/*  Set the value of key to value, replacing any previous value.             */
/*                                                                           */
/*****************************************************************************/

void KheOptionsSetObject(KHE_OPTIONS options, char *key, void *value)
  /* KHE_OPTIONS_VALUE_COPY_FN value_copy_fn) */
{
  int pos;
  if( HnTableContains(options->object_table, key, pos) )
    HnTableReplace(options->object_table, pos, value);
  else
    HnTableAdd(options->object_table, key, value);
}

/* ***
void KheOptionsSetObject(KHE_OPTIONS options, char *key, void *value,
  KHE_OPTIONS_VALUE_COPY_FN value_copy_fn)
{
  KHE_OPTIONS_OBJECT oo;  int pos;
  if( MTableRetrieve(options->object_table, key, &oo, &pos) )
    oo->value = value, oo->value_copy_fn = value_copy_fn;
  else
  {
    oo = KheOptionsObjectMake(value, value_copy_fn);
    MTableInsert(options->object_table, key, oo);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void *KheOptionsGetObject(KHE_OPTIONS options, char *key, void *dft)     */
/*                                                                           */
/*  Get the value of key in options, or "auto" if none.                      */
/*                                                                           */
/*****************************************************************************/

void *KheOptionsGetObject(KHE_OPTIONS options, char *key, void *dft)
{
  void *value;  int pos;
  if( !HnTableRetrieve(options->object_table, key, value, pos) )
    return dft;
  else
    return value;
}

/* ***
void *KheOptionsGetObject(KHE_OPTIONS options, char *key, void *dft)
{
  KHE_OPTIONS_OBJECT oo;  int pos;
  if( !MTableRetrieve(options->object_table, key, &oo, &pos) )
    return dft;
  else
    return oo->value;
}
*** */



/*****************************************************************************/
/*                                                                           */
/*  Submodule "timer set"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER KheOptionsAddTimer(KHE_OPTIONS options, char *tag,             */
/*    float limit_in_seconds)                                                */
/*                                                                           */
/*  Make a new timer with these attributes, add it to the timer set of       */
/*  options, and return it.                                                  */
/*                                                                           */
/*****************************************************************************/

KHE_TIMER KheOptionsAddTimer(KHE_OPTIONS options, char *tag,
  float limit_in_seconds)
{
  KHE_TIMER res;
  res = KheTimerMake(tag, limit_in_seconds, options->arena);
  KheTimerSetAddTimer(options->timer_set, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsDeleteTimer(KHE_OPTIONS options, KHE_TIMER timer)         */
/*                                                                           */
/*  Delete timer from the timer set of options.  It must be present.         */
/*                                                                           */
/*****************************************************************************/

void KheOptionsDeleteTimer(KHE_OPTIONS options, KHE_TIMER timer)
{
  KheTimerSetDeleteTimer(options->timer_set, timer);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsContainsTimer(KHE_OPTIONS options, char *tag,             */
/*    KHE_TIMER *timer)                                                      */
/*                                                                           */
/*  If the timer set of options contains a timer with the given tag,         */
/*  set *timer to one such timer and return true.  Otherwise return false.   */
/*                                                                           */
/*****************************************************************************/

bool KheOptionsContainsTimer(KHE_OPTIONS options, char *tag, KHE_TIMER *timer)
{
  return KheTimerSetContainsTimer(options->timer_set, tag, timer);
}


/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsRemainingTime(KHE_OPTIONS options)                       */
/*                                                                           */
/*  Return the remaining time of the time set of options.                    */
/*                                                                           */
/*****************************************************************************/

float KheOptionsRemainingTime(KHE_OPTIONS options)
{
  return KheTimerSetRemainingTime(options->timer_set);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsTimeLimitReached(KHE_OPTIONS options)                     */
/*                                                                           */
/*  Return true if a time limit is reached in the timer set of options.      */
/*                                                                           */
/*****************************************************************************/

bool KheOptionsTimeLimitReached(KHE_OPTIONS options)
{
  return KheTimerSetTimeLimitReached(options->timer_set);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOptionsTimeLimitReachedQueryCount(KHE_OPTIONS options)            */
/*                                                                           */
/*  Return the number of calls to KheOptionsTimeLimitReached since this      */
/*  function was last called.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheOptionsTimeLimitReachedQueryCount(KHE_OPTIONS options)
{
  return KheTimeSetTimeLimitReachedQueryCount(options->timer_set);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimeLimitConsistencyBegin(KHE_OPTIONS options,            */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Begin run consistency when solving soln.                                 */
/*                                                                           */
/*****************************************************************************/

void KheOptionsTimeLimitConsistencyBegin(KHE_OPTIONS options, KHE_SOLN soln)
{
  KheTimerSetTimeLimitConsistencyBegin(options->timer_set, soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimeLimitConsistencyEnd(KHE_OPTIONS options,              */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  End run consistency when solving soln.                                   */
/*                                                                           */
/*****************************************************************************/

void KheOptionsTimeLimitConsistencyEnd(KHE_OPTIONS options, KHE_SOLN soln)
{
  KheTimerSetTimeLimitConsistencyEnd(options->timer_set, soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimerSetSaveLog(KHE_OPTIONS options, KHE_SOLN soln)       */
/*                                                                           */
/*  Save the timer set of options.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOptionsTimerSetSaveLog(KHE_OPTIONS options, KHE_SOLN soln)
{
  KheTimerSetSaveLog(options->timer_set, soln);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimerSetDebug(KHE_OPTIONS options, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the the timer set of options onto fp with the given       */
/*  verbosity and indent.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheOptionsTimerSetDebug(KHE_OPTIONS options, int verbosity,
  int indent, FILE *fp)
{
  KheTimerSetDebug(options->timer_set, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "soft time limits (obsolete version"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_OPTIONS_TIME_LIMIT KheOptionsTimeMake(char *tag, float time_limit,   */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make an options time limit object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_OPTIONS_TIME_LIMIT KheOptionsTimeMake(char *tag, float time_limit,
  HA_ARENA a)
{
  KHE_OPTIONS_TIME_LIMIT res;
  HaMake(res, a);
  res->tag = tag;
  res->timer = KheStatsTimerMake(a);
  res->time_limit = time_limit;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimeLimitsDebug(KHE_OPTIONS options, FILE *fp)            */
/*                                                                           */
/*  Debug print of time limits.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheOptionsTimeLimitsDebug(KHE_OPTIONS options, FILE *fp)
{
  int i;  KHE_OPTIONS_TIME_LIMIT tl;  float now;
  if( HaArrayCount(options->time_limits) == 0 )
    fprintf(fp, "-");
  else
  {
    HaArrayForEach(options->time_limits, tl, i)
    {
      if( i > 0 )
	fprintf(fp, " & ");
      now = KheStatsTimerNow(tl->timer);
      fprintf(fp, "%s%c%.1f:%.1f", tl->tag, now >= tl->time_limit ? '!' : ':',
	now, tl->time_limit);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsContainsTimeLimit(KHE_OPTIONS options, char *tag,         */
/*    KHE_OPTIONS_TIME_LIMIT *tl)                                            */
/*                                                                           */
/*  If options contains a time limit with this tag, set *tl to that time     */
/*  limit and return true.  Otherwise set *tl to NULL and return false.      */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheOptionsContainsTimeLimit(KHE_OPTIONS options, char *tag,
  KHE_OPTIONS_TIME_LIMIT *tl)
{
  int i;  KHE_OPTIONS_TIME_LIMIT tl2;
  HaArrayForEach(options->time_limits, tl2, i)
    if( strcmp(tl2->tag, tag) == 0 )
      return *tl = tl2, true;
  return *tl = NULL, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimeLimitBegin(KHE_OPTIONS options, char *tag,            */
/*    float limit_in_secs)                                                   */
/*                                                                           */
/*  Begin a new time limit within options, identified by tag, which          */
/*  records that the given time limit is wanted, starting now.               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOptionsTimeLimitBegin(KHE_OPTIONS options, char *tag,
  float limit_in_secs)
{
  KHE_OPTIONS_TIME_LIMIT tl;
  if( DEBUG1 )
  {
    fprintf(stderr, "  TimeLimitBegin(");
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, ", \"%s\", %.1f) = ", tag, limit_in_secs);
  }
  HnAssert(!KheOptionsContainsTimeLimit(options, tag, &tl),
    "KheOptionsTimeLimitBegin: time limit with tag %s already present", tag);
  if( HaArrayCount(options->free_time_limits) > 0 )
  {
    tl = HaArrayLastAndDelete(options->free_time_limits);
    KheStatsTimerReset(tl->timer);
  }
  else
  {
    HaMake(tl, options->arena);
    tl->timer = KheStatsTimerMake(options->arena);
  }
  tl->tag = tag;
  tl->time_limit = limit_in_secs;
  HaArrayAddLast(options->time_limits, tl);
  if( DEBUG1 )
  {
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, "\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsTimeLimitEnd(KHE_OPTIONS options, char *tag)             */
/*                                                                           */
/*  End the time limit with this tag, also returning the number of           */
/*  seconds since it was begun (like KheOptionsTimeNow).                     */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheOptionsTimeLimitEnd(KHE_OPTIONS options, char *tag)
{
  float res, t2;  KHE_OPTIONS_TIME_LIMIT tl;
  if( DEBUG1 )
  {
    fprintf(stderr, "  TimeLimitEnd(");
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, ", \"%s\") = ", tag);
  }
  if( KheOptionsTimeLimitNow(options, tag, &res) )
  {
    tl = HaArrayLastAndDelete(options->time_limits);  ** must be present **
    HnAssert(strcmp(tl->tag, tag) == 0, "KheOptionsTimeLimitEnd: "
      "time limit with tag \"%s\" not most recent", tag);
    if( DEBUG3 )
    {
      fprintf(stderr, "  time limit %s (%.1f secs) ending after %.1f secs",
	tag, tl->time_limit, res);
      if( KheOptionsTimeLimitNow(options, "global", &t2) )
	fprintf(stderr, " (global %.1f)", t2);
      fprintf(stderr, "\n");
    }
    HaArrayAddLast(options->free_time_limits, tl);
  }
  else
  {
    HnAbort("KheOptionsTimeLimitEnd: no time limit with tag \"%s\"", tag);
    res = 0.0;  ** keep compiler happy **
  }
  if( DEBUG1 )
  {
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, " ret. %.1f\n", res);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsTimeLimitNow(KHE_OPTIONS options, char *tag)             */
/*                                                                           */
/*  Return the number of seconds since the time limit with this tag          */
/*  was begun, or abort if there is no such time limit in options.           */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheOptionsTimeLimitNow(KHE_OPTIONS options, char *tag,
  float *elapsed_time_in_secs)
{
  KHE_OPTIONS_TIME_LIMIT tl;
  if( KheOptionsContainsTimeLimit(options, tag, &tl) )
    return *elapsed_time_in_secs = KheStatsTimerNow(tl->timer), true;
  else
    return *elapsed_time_in_secs = -1.0, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsOneTimeLimitReached(KHE_OPTIONS_TIME_LIMIT tl)            */
/*                                                                           */
/*  Return true if tl has reached its time limit.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheOptionsOneTimeLimitReached(KHE_OPTIONS_TIME_LIMIT tl)
{
  float now;
  if( tl->time_limit == -1.0 )
    return false;
  now = KheStatsTimerNow(tl->timer);
  if( now == -1.0 )
    return false;
  return now >= tl->time_limit;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsTimeLimitReached(KHE_OPTIONS options)                     */
/*                                                                           */
/*  Return true if at least one of the time limits of options has            */
/*  reached its limit.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheOptionsTimeLimitReached(KHE_OPTIONS options)
{
  KHE_OPTIONS_TIME_LIMIT tl;  int i;  bool res;
  res = false;
  HaArrayForEachReverse(options->time_limits, tl, i)
    if( KheOptionsOneTimeLimitReached(tl) )
    {
      res = true;
      break;
    }
  if( DEBUG1 )
  {
    fprintf(stderr, "  TimeLimitReached(");
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, ") = %s\n", res ? "true" : "false");
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsTimeLimitReset(KHE_OPTIONS options, char *tag,            */
/*    float limit_in_secs)                                                   */
/*                                                                           */
/*  Reset the time limit with this tag to start now and have this limit.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOptionsTimeLimitReset(KHE_OPTIONS options, char *tag,
  float limit_in_secs)
{
  KHE_OPTIONS_TIME_LIMIT tl;  float t2;
  if( DEBUG1 )
  {
    fprintf(stderr, "  TimeLimitReset(");
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, ", \"%s\", %.1f) = ", tag, limit_in_secs);
  }
  if( KheOptionsContainsTimeLimit(options, tag, &tl) )
  {
    if( DEBUG3 )
    {
      fprintf(stderr, "  time limit %s (%.1f secs) reset after %.1f secs",
	tag, tl->time_limit, KheStatsTimerNow(tl->timer));
      if( KheOptionsTimeLimitNow(options, "global", &t2) )
	fprintf(stderr, " (global %.1f)", t2);
      fprintf(stderr, "\n");
    }
    KheStatsTimerReset(tl->timer);
    if( limit_in_secs >= 0.0 )
      tl->time_limit = limit_in_secs;
  }
  if( DEBUG1 )
  {
    KheOptionsTimeLimitsDebug(options, stderr);
    fprintf(stderr, "\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "soft time limits" (obsolete version)                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsTimeNow(KHE_OPTIONS options)                             */
/*                                                                           */
/*  Return the wall clock time since options was created.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheOptionsTimeNow(KHE_OPTIONS options)
{
  return KheStatsTimerNow(options->timer);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOptionsSetTimeLimit(KHE_OPTIONS options, float limit_in_secs)    */
/*                                                                           */
/*  Set the time limit to limit_in_secs.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOptionsSetTimeLimit(KHE_OPTIONS options, float limit_in_secs)
{
  options->time_limit = limit_in_secs;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheOptionsTimeLimit(KHE_OPTIONS options)                           */
/*                                                                           */
/*  Return the time limit, or -1.0 if none.                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheOptionsTimeLimit(KHE_OPTIONS options)
{
  return options->time_limit;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionsTimeLimitReached(KHE_OPTIONS options)                     */
/*                                                                           */
/*  Return true if there is a time limit and it has been reached.            */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheOptionsTimeLimitReached(KHE_OPTIONS options)
{
  float now;
  if( options->time_limit == -1 )
  {
    if( DEBUG2 )
      fprintf(stderr, "OptionsTimeLimitReached = false (no time limit)\n");
    return false;
  }
  now = KheOptionsTimeNow(options);
  if( now == -1 )
  {
    if( DEBUG2 )
      fprintf(stderr, "OptionsTimeLimitReached = false (time not known)\n");
    return false;
  }
  if( DEBUG2 )
    fprintf(stderr,
      "OptionsTimeLimitReached = %s (now %.2f mins, limit %.2f mins)\n",
      now >= options->time_limit ? "true" : "false", now / 60.0,
      options->time_limit / 60.0);
  return now >= options->time_limit;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "objects of general interest"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheOptionsFrame(KHE_OPTIONS options, char *key, KHE_SOLN soln) */
/*                                                                           */
/*  Get from options (or make and add to options) a common frame.            */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheOptionsFrame(KHE_OPTIONS options, char *key, KHE_SOLN soln)
{
  KHE_FRAME res;
  res = (KHE_FRAME) KheOptionsGetObject(options, key, NULL);
  if( res == NULL )
  {
    res = KheFrameMakeCommon(KheSolnInstance(soln), options->arena);
    if( res == NULL )
      res = KheFrameMakeSingletons(KheSolnInstance(soln), options->arena);
    KheOptionsSetObject(options, key, (void *) res);
  }
  return res;
}

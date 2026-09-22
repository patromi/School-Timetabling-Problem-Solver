
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
/*  FILE:         khe_sm_timer.c                                             */
/*  DESCRIPTION:  Timers and timer sets                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>
#if KHE_USE_TIMING
#include <time.h>
#include <sys/time.h>
#endif

#define DEBUG1	0
#define DEBUG2	0


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER                                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_timer_rec {
  char				*tag;		/* identifying tag           */
#if KHE_USE_TIMING
  struct timeval		start_time;	/* start time                */
#endif
  float				time_limit;	/* time limit                */
};

typedef HA_ARRAY(KHE_TIMER) ARRAY_KHE_TIMER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSISTENCY_STATE                                                    */
/*                                                                           */
/*  What a timer set is currently doing about time limit consistency:        */
/*                                                                           */
/*    KHE_CONSISTENCY_NONE                                                   */
/*      There has been no call to KheTimerSetTimeLimitConsistencyBegin,      */
/*      so the timer set is using timers to implement time limits, in        */
/*      the usual way.                                                       */
/*                                                                           */
/*    KHE_CONSISTENCY_STARTING                                               */
/*      There has been a call to KheTimerSetTimeLimitConsistencyBegin,       */
/*      but there is no .tlc file, so the timer set is using timers to       */
/*      implement time limits, but it will be writing query counts to        */
/*      a .tlc file so that the next run can use them.                       */
/*                                                                           */
/*    KHE_CONSISTENCY_ACTIVE                                                 */
/*      There has been a call to KheTimerSetTimeLimitConsistencyBegin,       */
/*      and there is a .tlc file, so the timer set is using query            */
/*      counts to determine time limits, not timers.                         */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_CONSISTENCY_NONE,
  KHE_CONSISTENCY_STARTING,
  KHE_CONSISTENCY_ACTIVE
} KHE_CONSISTENCY_STATE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER_SET                                                            */
/*                                                                           */
/*  old_query_counts                                                         */
/*    If state == KHE_CONSISTENCY_ACTIVE, the query counts from a previous   */
/*    run as read from a .tlc file.                                          */
/*                                                                           */
/*  curr_query_counts                                                        */
/*    The query counts from the current run.  These are recorded whatever    */
/*    the state, but only saved to a file if state != KHE_CONSISTENCY_NONE.  */
/*                                                                           */
/*  curr_timer_tags                                                          */
/*    One for each query count except the last, identifying the most         */
/*    recently added timer when the queries were made.  This is a rough      */
/*    but ready way to work out which solvers are making the calls.          */
/*    These values are written to .tlc files, but only as an aid to          */
/*    testing; they could be omitted completely without harm.                */
/*                                                                           */
/*****************************************************************************/

struct khe_timer_set_rec {
  HA_ARENA			arena;
  ARRAY_KHE_TIMER		timers;
  KHE_CONSISTENCY_STATE		state;
  HA_ARRAY_INT			old_query_counts;
  HA_ARRAY_INT			curr_query_counts;
  HA_ARRAY_NSTRING		curr_timer_tags;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheDateToday"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheDateToday(void)                                                 */
/*                                                                           */
/*  Return today's date as a string in static memory.                        */
/*                                                                           */
/*****************************************************************************/

static char *KheMonthName(int tm_mon)
{
  char *months[] = { "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December" };
  HnAssert(tm_mon >= 0 && tm_mon <= 11, "KheMonthName internal error");
  return months[tm_mon];
}

char *KheDateToday(void)
{
#if KHE_USE_TIMING
  static char buff[100];
  time_t t;
  struct tm *time_now;
  time(&t);
  time_now = localtime(&t);
  snprintf(buff, 100, "%d %s %d", time_now->tm_mday,
    KheMonthName(time_now->tm_mon), 1900 + time_now->tm_year);
  return buff;
#else
  return "(no date)";
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "conversion"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  float KheTimeFromString(char *str)                                       */
/*                                                                           */
/*  Convert *str into a number of seconds.  The format is secs, or           */
/*  mins:secs, or hrs:mins:secs.  Alternatively, "-" means KHE_NO_TIME.      */
/*                                                                           */
/*****************************************************************************/

float KheTimeFromString(char *str)
{
  float res;
  if( KheStringContainsTime(str, &res) )
    return res;
  else
  {
    HnAbort("KheTimeFromString:  format error in time limit %s", str);
    return KHE_NO_TIME;  /* keep compiler happy */
  }
  /* ***
  char *p;  float hrs, mins, secs;

  ** "-" special case **
  if( strcmp(str, "-") == 0 )
    return KHE_NO_TIME;

  ** secs **
  p = strstr(str, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%f", &secs) != 1 )
      HnAbort("KheTimeFromString:  format error in time limit %s", str);
    return secs;
  }

  ** mins:secs **
  p = strstr(p + 1, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%f:%f", &mins, &secs) != 2 )
      HnAbort("KheTimeFromString:  format error in time limit %s", str);
    return mins * 60.0 + secs;
  }

  ** hrs:mins:secs **
  if( sscanf(str, "%f:%f:%f", &hrs, &mins, &secs) != 3 )
    HnAbort("KheTimeFromString:  format error in time limit %s", str);
  return hrs * 60.0 * 60.0 + mins * 60.0 + secs;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheStringContainsTime(char *str, float *time)                       */
/*                                                                           */
/*  If str contains a time (possibly -, denoting KHE_NO_TIME), set *time     */
/*  to the time denoted and return true.  Otherwise set *time to             */
/*  KHE_NO_TIME and return false.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheStringContainsTime(char *str, float *time)
{
  char *p;  float hrs, mins, secs;

  /* "-" special case */
  if( strcmp(str, "-") == 0 )
    return *time = KHE_NO_TIME, true;

  /* secs */
  p = strstr(str, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%f", &secs) != 1 )
      return *time = KHE_NO_TIME, false;
    else
      return *time = secs, true;
  }

  /* mins:secs */
  p = strstr(p + 1, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%f:%f", &mins, &secs) != 2 )
      return *time = KHE_NO_TIME, false;
    else
      return *time = mins * 60.0 + secs, true;
  }

  /* hrs:mins:secs */
  if( sscanf(str, "%f:%f:%f", &hrs, &mins, &secs) != 3 )
    return *time = KHE_NO_TIME, false;
  else
    return *time = hrs * 60.0 * 60.0 + mins * 60.0 + secs, true;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimeShow(float secs, char buff[20])                             */
/*                                                                           */
/*  Show secs in the format "XX mins" or "XX secs".                          */
/*                                                                           */
/*****************************************************************************/

char *KheTimeShow(float secs, char buff[20])
{
  if( secs == KHE_NO_TIME )
    snprintf(buff, 20, "(no time)");
  else if( secs > 300.0 )
    snprintf(buff, 20, "%.1f mins", secs / 60.0);
  else
    snprintf(buff, 20, "%.1f secs", secs);
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "timers"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER KheTimerMake(char *tag, float limit_in_seconds, HA_ARENA a)    */
/*                                                                           */
/*  Make and return a new timer in arena a with these attributes.            */
/*                                                                           */
/*****************************************************************************/

KHE_TIMER KheTimerMake(char *tag, float limit_in_seconds, HA_ARENA a)
{
  KHE_TIMER res;
  HaMake(res, a);
  res->tag = tag;
  KheTimerResetStartTime(res);
  res->time_limit = limit_in_seconds;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER KheTimerCopy(KHE_TIMER timer, HA_ARENA a)                      */
/*                                                                           */
/*  Return a copy of timer in a.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_TIMER KheTimerCopy(KHE_TIMER timer, HA_ARENA a)
{
  KHE_TIMER res;
  HaMake(res, a);
  res->tag = timer->tag;
#if KHE_USE_TIMING
  res->start_time = timer->start_time;
#endif
  res->time_limit = timer->time_limit;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheTimerTag(KHE_TIMER timer)                                       */
/*                                                                           */
/*  Return the tag attribute of timer.                                       */
/*                                                                           */
/*****************************************************************************/

char *KheTimerTag(KHE_TIMER timer)
{
  return timer->tag;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTimerTimeLimit(KHE_TIMER timer)                                 */
/*                                                                           */
/*  Return the time limit attribute of timer.                                */
/*                                                                           */
/*****************************************************************************/

float KheTimerTimeLimit(KHE_TIMER timer)
{
  return timer->time_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerResetStartTime(KHE_TIMER timer)                             */
/*                                                                           */
/*  Reset the start time of timer to the current time.                       */
/*                                                                           */
/*****************************************************************************/

void KheTimerResetStartTime(KHE_TIMER timer)
{
#if KHE_USE_TIMING
  gettimeofday(&timer->start_time, NULL);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerResetTimeLimit(KHE_TIMER timer, float limit_in_seconds)     */
/*                                                                           */
/*  Reset the time limit of timer.                                           */
/*                                                                           */
/*****************************************************************************/

void KheTimerResetTimeLimit(KHE_TIMER timer, float limit_in_seconds)
{
  timer->time_limit = limit_in_seconds;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTimeDiff(struct timeval *earlier, struct timeval *later)        */
/*                                                                           */
/*  Return the time difference between earlier and later, in seconds.        */
/*                                                                           */
/*****************************************************************************/

#if KHE_USE_TIMING
static float KheTimeDiff(struct timeval *earlier, struct timeval *later)
{
  return (float) (later->tv_sec - earlier->tv_sec) +
    (float) (later->tv_usec - earlier->tv_usec) / 1000000.0;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  float KheTimerElapsedTime(KHE_TIMER timer)                               */
/*                                                                           */
/*  Return the elapsed time of timer.                                        */
/*                                                                           */
/*****************************************************************************/

float KheTimerElapsedTime(KHE_TIMER timer)
{
#if KHE_USE_TIMING
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return KheTimeDiff(&timer->start_time, &curr_time);
#else
  return KHE_NO_TIME;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTimerRemainingTime(KHE_TIMER timer)                             */
/*                                                                           */
/*  Return the remaining time until timer reaches its time limit, or 0.0     */
/*  if it is already at or over its time limit.                              */
/*                                                                           */
/*****************************************************************************/

float KheTimerRemainingTime(KHE_TIMER timer)
{
  float remaining_time;
#if KHE_USE_TIMING
  if( timer->time_limit == KHE_NO_TIME )
    return KHE_NO_TIME;
  remaining_time = timer->time_limit - KheTimerElapsedTime(timer);
  return remaining_time >= 0.0 ? remaining_time : 0.0;
#else
  return KHE_NO_TIME;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimerReached(KHE_TIMER timer, bool *curr_time_known,             */
/*    struct timeval *curr_time)                                             */
/*                                                                           */
/*  Return true if timer's time limit has been reached; this will always     */
/*  be false if it has no time limit.                                        */
/*                                                                           */
/*  Parameter *curr_time_known is true when *curr_time contains the          */
/*  current time.  It is changed to true by this function if it finds        */
/*  the current time.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTimerReached(KHE_TIMER timer, bool *curr_time_known,
  struct timeval *curr_time)
{
  if( timer->time_limit != KHE_NO_TIME )
  {
    /* get current time, if not done yet */
    if( !*curr_time_known )
    {
      gettimeofday(curr_time, NULL);
      *curr_time_known = true;
    }

    /* work out whether this timer's limit has been reached */
    if( DEBUG2 )
      KheTimerDebug(timer, 2, 2, stderr);
    return KheTimeDiff(&timer->start_time, curr_time) >= timer->time_limit;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimerTimeLimitReached(KHE_TIMER timer)                           */
/*                                                                           */
/*  Return true if timer has a time limit and it has been reached.           */
/*                                                                           */
/*****************************************************************************/

bool KheTimerTimeLimitReached(KHE_TIMER timer)
{
  struct timeval curr_time;  bool curr_time_known;
  curr_time_known = false;
  return KheTimerReached(timer, &curr_time_known, &curr_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerDebug(KHE_TIMER timer, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of timer onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

void KheTimerDebug(KHE_TIMER timer, int verbosity, int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s:%.1f", timer->tag, KheTimerElapsedTime(timer));
  if( timer->time_limit != KHE_NO_TIME )
    fprintf(fp, ":%.1f", timer->time_limit);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "timer sets"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER_SET KheTimerSetMake(bool log, HA_ARENA a)                      */
/*                                                                           */
/*  Make a new, empty timer set.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_TIMER_SET KheTimerSetMake(HA_ARENA a)
{
  KHE_TIMER_SET res;
  HaMake(res, a);
  res->arena = a;
  HaArrayInit(res->timers, a);
  res->state = KHE_CONSISTENCY_NONE;
  HaArrayInit(res->old_query_counts, a);
  HaArrayInit(res->curr_query_counts, a);
  HaArrayAddLast(res->curr_query_counts, 0);
  HaArrayInit(res->curr_timer_tags, a);
  if( DEBUG1 )
  {
    fprintf(stderr, "timer set at start:\n");
    KheTimerSetDebug(res, 2, 2, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIMER_SET KheTimerSetCopy(KHE_TIMER_SET timer_set, HA_ARENA a)       */
/*                                                                           */
/*  Make a copy of timer_set in arena a.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TIMER_SET KheTimerSetCopy(KHE_TIMER_SET timer_set, HA_ARENA a)
{
  KHE_TIMER_SET res;  KHE_TIMER timer;  int i;

  /* make sure there are no query counts around */
  HnAssert(HaArrayCount(timer_set->old_query_counts) == 0,
    "KheTimerSetCopy: old query counts are present");

  /* make a new, empty timer set object and add copies of timer_sets's timers */
  res = KheTimerSetMake(a);
  HaArrayForEach(timer_set->timers, timer, i)
    HaArrayAddLast(timer_set->timers, KheTimerCopy(timer, a));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerSetAddTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer)       */
/*                                                                           */
/*  Add timer to timer_set.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheTimerSetAddTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer)
{
  int pos;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheTimerSetAddTimer(timer_set, timer):\n");
    KheTimerSetDebug(timer_set, 2, 2, stderr);
  }
  HnAssert(!HaArrayContains(timer_set->timers, timer, &pos),
    "KheTimerSetAddTimer: timer_set already contains timer");
  HaArrayAddLast(timer_set->timers, timer);
  if( DEBUG1 )
  {
    KheTimerSetDebug(timer_set, 2, 2, stderr);
    fprintf(stderr, "] KheTimerSetAddTimer returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerSetDeleteTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer)    */
/*                                                                           */
/*  Delete timer from timer_set.                                             */
/*                                                                           */
/*****************************************************************************/

void KheTimerSetDeleteTimer(KHE_TIMER_SET timer_set, KHE_TIMER timer)
{
  int pos;
  if( !HaArrayContains(timer_set->timers, timer, &pos) )
    HnAbort("KheTimerSetDeleteTimer: time_set does not contain timer \"%s\"",
      timer->tag);
  HaArrayDeleteAndShift(timer_set->timers, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimerSetContainsTimer(KHE_TIMER_SET timer_set, char *tag,        */
/*    KHE_TIMER *timer)                                                      */
/*                                                                           */
/*  If timer_set contains a timer with the given tag, set *timer to          */
/*  such a timer and return true.  Otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

bool KheTimerSetContainsTimer(KHE_TIMER_SET timer_set, char *tag,
  KHE_TIMER *timer)
{
  KHE_TIMER tm;  int i;
  HaArrayForEach(timer_set->timers, tm, i)
    if( strcmp(tag, tm->tag) == 0 )
      return *timer = tm, true;
  return *timer = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTimerSetRemainingTime(KHE_TIMER_SET timer_set)                  */
/*                                                                           */
/*  Return the remaining time, or KHE_NO_TIME if unlimited or time limits    */
/*  are irrelevant because time limit consistency is active.                 */
/*                                                                           */
/*****************************************************************************/

float KheTimerSetRemainingTime(KHE_TIMER_SET timer_set)
{
#if KHE_USE_TIMING
  KHE_TIMER timer;  int i;
  float min_remaining_time, remaining_time;
  if( timer_set->state == KHE_CONSISTENCY_ACTIVE )
    return KHE_NO_TIME;
  min_remaining_time = KHE_NO_TIME;
  HaArrayForEach(timer_set->timers, timer, i)
    if( timer->time_limit != KHE_NO_TIME )
    {
      remaining_time = KheTimerRemainingTime(timer);
      if( min_remaining_time == KHE_NO_TIME ||
	  remaining_time < min_remaining_time )
        min_remaining_time = remaining_time;
    }
  return min_remaining_time;
#else
  return KHE_NO_TIME;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimerSetTimeLimitReached(KHE_TIMER_SET timer_set)                */
/*                                                                           */
/*  Return true if any of the time limits of timer_set have been reached.    */
/*                                                                           */
/*  Implementation note.  This does not call KheTimerTimeLimitReached,       */
/*  because it is optimized to call gettimeofday only once.                  */
/*                                                                           */
/*****************************************************************************/

bool KheTimerSetTimeLimitReached(KHE_TIMER_SET timer_set)
{
#if KHE_USE_TIMING
  struct timeval curr_time;  KHE_TIMER timer;  int i, curr_index, curr_count;
  bool curr_time_known;
  switch( timer_set->state )
  {
    case KHE_CONSISTENCY_NONE:

      /* no consistency checking, just use timers */
      curr_time_known = false;
      HaArrayForEach(timer_set->timers, timer, i)
	if( KheTimerReached(timer, &curr_time_known, &curr_time) )
	  return true;
      return false;

    case KHE_CONSISTENCY_STARTING:

      /* use timers, but save query counts for the next run to use */
      curr_index = HaArrayCount(timer_set->curr_query_counts) - 1;
      curr_count = ++HaArray(timer_set->curr_query_counts, curr_index);
      curr_time_known = false;
      HaArrayForEach(timer_set->timers, timer, i)
	if( KheTimerReached(timer, &curr_time_known, &curr_time) )
	{
	  HaArrayAddLast(timer_set->curr_query_counts, 0);
	  timer = HaArrayLast(timer_set->timers);  /* most specific */
	  HaArrayAddLast(timer_set->curr_timer_tags,
	    HnStringCopy(timer->tag, timer_set->arena));
	  return true;
	}
      return false;

    case KHE_CONSISTENCY_ACTIVE:

      /* bypass timers and use old query counts to determine the result; */
      /* but don't save timer tags, there may be no timers at all */
      curr_index = HaArrayCount(timer_set->curr_query_counts) - 1;
      curr_count = ++HaArray(timer_set->curr_query_counts, curr_index);
      HnAssert(curr_index < HaArrayCount(timer_set->old_query_counts),
	"KheTimerSetTimeLimitReached: inconsistency detected "
	"(did the options change from the previous run?)");
      if( curr_count >= HaArray(timer_set->old_query_counts, curr_index) )
      {
	HaArrayAddLast(timer_set->curr_query_counts, 0);
	return true;
      }
      return false;

    default:

      HnAbort("KheTimerSetTimeLimitReached internal error");
      return false;  /* keep compiler happy */
  }
#else
  return false;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerSetTimeLimitConsistencyBegin(KHE_TIMER_SET timer_set,       */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Begin a time limit consistency run.                                      */
/*                                                                           */
/*****************************************************************************/

void KheTimerSetTimeLimitConsistencyBegin(KHE_TIMER_SET timer_set,
  KHE_SOLN soln)
{
#if KHE_USE_TIMING
  char file_name[200], buff[200], header[200];  FILE *fp;  int line_num, val;

  /* ensure not called twice */
  HnAssert(timer_set->state == KHE_CONSISTENCY_NONE,
    "KheTimerSetTimeLimitConsistencyBegin: called twice");

  /* ensure not called too late */
  HnAssert(HaArrayCount(timer_set->curr_query_counts) == 1 &&
    HaArrayFirst(timer_set->curr_query_counts) == 0,
    "KheTimerSetTimeLimitConsistencyBegin: called too late");

  /* construct the file name and open the file for reading */
  snprintf(file_name, 200, "khe_%s_%d.tlc",
    KheInstanceId(KheSolnInstance(soln)), KheSolnDiversifier(soln));
  fp = fopen(file_name, "r");

  /* read old queries (from previous run) if file opened */
  if( fp != NULL )
  {
    /* read the header line of the file and make sure it's OK */
    if( fgets(buff, 200, fp) == NULL || sscanf(buff, "%199s", header) != 1 ||
	strcmp(header, file_name) != 0 )
      HnAbort("KheTimerSetTimeLimitConsistencyBegin:  missing or erroneous "
	"header line in file %s\n", file_name);

    /* read the old query counts into timer_set->old_query_counts */
    line_num = 1;
    while( fgets(buff, 200, fp) != NULL )
    {
      line_num++;
      if( sscanf(buff, "%d", &val) != 1 )
	HnAbort("KheTimerSetTimeLimitConsistencyBegin:  error in file "
	  "%s (line %d)\n", file_name, line_num);
      HaArrayAddLast(timer_set->old_query_counts, val);
    }
    fclose(fp);

    /* record that old queries were found */
    timer_set->state = KHE_CONSISTENCY_ACTIVE;
  }
  else
  {
    /* no file, so just starting time limit consistency on this run */
    timer_set->state = KHE_CONSISTENCY_STARTING;
  }
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerSetTimeLimitConsistencyEnd(KHE_TIMER_SET timer_set,         */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  End a time limit consistency run.  This writes out the current query     */
/*  counts to the .tlc file.                                                 */
/*                                                                           */
/*  Implementation note.  When state == KHE_CONSISTENCY_ACTIVE, a            */
/*  concluding 0 is written to the file that is not written when             */
/*  state == KHE_CONSISTENCY_STARTING.  This is because a time limit         */
/*  is reached at the end of the run in the first case which is not          */
/*  reached in the second case.   This extra 0 is harmless.                  */
/*                                                                           */
/*****************************************************************************/

void KheTimerSetTimeLimitConsistencyEnd(KHE_TIMER_SET timer_set, KHE_SOLN soln)
{
#if KHE_USE_TIMING
  char file_name[200];  FILE *fp;  int val, i;

  /* ensure matching consistency begin */
  HnAssert(timer_set->state != KHE_CONSISTENCY_NONE,
    "KheTimerSetTimeLimitConsistencyEnd: no matching "
    "KheTimerSetTimeLimitConsistencyBegin");

  /* construct the file name and open the file for writing */
  snprintf(file_name, 200, "khe_%s_%d.tlc",
    KheInstanceId(KheSolnInstance(soln)), KheSolnDiversifier(soln));
  fp = fopen(file_name, "w");
  if( fp == NULL )
    HnAbort("KheTimerSetTimeLimitConsistencyEnd: cannot open file %s"
      " for writing\n", file_name);

  /* write the header line and query counts to the file */
  fprintf(fp, "%s\n", file_name);
  HaArrayForEach(timer_set->curr_query_counts, val, i)
  {
    fprintf(fp, "%d", val);
    if( i < HaArrayCount(timer_set->curr_timer_tags) )
      fprintf(fp, " %s", HaArray(timer_set->curr_timer_tags, i));
    fprintf(fp, "\n");
  }
  fclose(fp);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimerSetDebug(KHE_TIMER_SET timer_set, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of time limits.                                              */
/*                                                                           */
/*****************************************************************************/

void KheTimerSetDebug(KHE_TIMER_SET timer_set, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_TIMER timer;
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  if( verbosity >= 2 )
    fprintf(fp, "%p ", (void *) timer_set);
  if( HaArrayCount(timer_set->timers) == 0 )
    fprintf(fp, "-");
  else
  {
    HaArrayForEach(timer_set->timers, timer, i)
    {
      if( i > 0 )
	fprintf(fp, " & ");
      KheTimerDebug(timer, verbosity, -1, fp);
    }
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}

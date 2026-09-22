
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
/*  FILE:         khe_sr_task_finder.c                                       */
/*  DESCRIPTION:  Task finder                                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "limits.h"
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0


/*****************************************************************************/
/*                                                                           */
/*  NEEDS_ASST_TYPE - says whether task should need assignment or not        */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  NEEDS_ASST_FALSE,
  NEEDS_ASST_TRUE,
  NEEDS_ASST_DONT_CARE
} NEEDS_ASST_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL - an interval (private)                                     */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_solvers.h
typedef struct khe_interval_rec {
  int first;
  int last;
} KHE_INTERVAL;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE - the daily schedule of a task                        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_and_time_rec {
  KHE_TASK			task;
  KHE_TIME			time;
} *KHE_TASK_AND_TIME;

typedef HA_ARRAY(KHE_TASK_AND_TIME) ARRAY_KHE_TASK_AND_TIME;

struct khe_daily_schedule_rec {
  KHE_TASK_FINDER		task_finder;
  bool				no_overlap;
  KHE_INTERVAL			interval;
  ARRAY_KHE_TASK_AND_TIME	tasks_and_times;
};

typedef HA_ARRAY(KHE_DAILY_SCHEDULE) ARRAY_KHE_DAILY_SCHEDULE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_FINDER                                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_task_finder_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  KHE_FRAME				days_frame;
  KHE_EVENT_TIMETABLE_MONITOR		etm;
  ARRAY_KHE_DAILY_SCHEDULE		free_daily_schedules;
  ARRAY_KHE_TASK_AND_TIME		free_tasks_and_times;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "intervals" (private)                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheInterval(int first, int last)                            */
/*                                                                           */
/*  Return a new interval with these attributes.                             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static KHE_INTERVAL KheInterval(int first, int last)
{
  KHE_INTERVAL res;
  res.first = first;
  res.last = last;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalLength(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return the length of in.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheIntervalLength(KHE_INTERVAL in)
{
  return in.last - in.first + 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEmpty(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return true if in is empty.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalEmpty(KHE_INTERVAL in)
{
  return in.first > in.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalContains(KHE_INTERVAL in, int index)                     */
/*                                                                           */
/*  Return true if in contains index.                                        */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalContains(KHE_INTERVAL in, int index)
{
  return index >= in.first && index <= in.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)               */
/*                                                                           */
/*  Return true if in1 is a subset of in2.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) ||
    (in2.first <= in1.first && in1.last <= in2.last);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)                */
/*                                                                           */
/*  Return true if in1 and in2 are equal.                                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheIntervalEqual(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  if( KheIntervalEmpty(in1) )
    return KheIntervalEmpty(in2);
  else
    return in1.first == in2.first && in1.last == in2.last;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)             */
/*                                                                           */
/*  Return true if in1 and in2 are disjoint.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) || KheIntervalEmpty(in2) ||
    in1.last < in2.first || in2.last < in1.first;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalUnion(KHE_INTERVAL *in1, KHE_INTERVAL in2)               */
/*                                                                           */
/*  Overwrite *in1 with its union with in2.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_interval.c
static void KheIntervalUnion(KHE_INTERVAL *in1, KHE_INTERVAL in2)
{
  if( KheIntervalEmpty(*in1) )
    *in1 = in2;
  else if( !KheIntervalEmpty(in2) )
  {
    if( in2.first < in1->first ) in1->first = in2.first;
    if( in2.last > in1->last ) in1->last = in2.last;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheTaskFinderIntervalShow(KHE_TASK_FINDER tf, KHE_INTERVAL in)     */
/*                                                                           */
/*  Show in in static memory.                                                */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static char *KheTaskFinderIntervalShow(KHE_TASK_FINDER tf, KHE_INTERVAL in)
{
  static char buff[500];
  if( KheIntervalEmpty(in) )
    sprintf(buff, "-");
  else
    sprintf(buff, "%s-%s",
      KheTimeGroupId(KheFrameTimeGroup(tf->days_frame, in.first)),
      KheTimeGroupId(KheFrameTimeGroup(tf->days_frame, in.last)));
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoInterval(KHE_TASK task, KHE_FRAME frame, KHE_INTERVAL *in) */
/*                                                                           */
/*  Do that part of the work of KheTaskSetFindInterval below that pertains   */
/*  to task and its descendants.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoInterval(KHE_TASK task, KHE_FRAME frame, KHE_INTERVAL *in)
{
  KHE_MEET meet;  KHE_TIME t;  KHE_TASK child_task;  int i, durn, fi, li;

  /* do it for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      if( durn == 1 )
	fi = li = KheFrameTimeIndex(frame, t);
      else
      {
	fi = KheFrameTimeIndex(frame, t);
	li = KheFrameTimeIndex(frame, KheTimeNeighbour(t, durn - 1));
      }
      if( fi < in->first )
	in->first = fi;
      if( li > in->last )
	in->last = li;
    }
  }

  /* do it for the children of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskDoInterval(child_task, frame, in);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskFindInterval(KHE_TASK_FINDER tf, KHE_TASK task)      */
/*                                                                           */
/*  Return the interval covered by task.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheTaskFindInterval(KHE_TASK_FINDER tf, KHE_TASK task)
{
  KHE_INTERVAL res;
  res.first = KheFrameTimeGroupCount(tf->days_frame) - 1;
  res.last = 0;
  KheTaskDoInterval(task, tf->days_frame, &res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskSetFindInterval(KHE_TASK_FINDER tf,  KHE_TASK_SET ts)*/
/*                                                                           */
/*  Return the interval covered by ts.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheTaskSetFindInterval(KHE_TASK_FINDER tf,  KHE_TASK_SET ts)
{
  int i;  KHE_TASK task;  KHE_INTERVAL res;
  res.first = KheFrameTimeGroupCount(tf->days_frame) - 1;
  res.last = 0;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    KheTaskDoInterval(task, tf->days_frame, &res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSatisfiesNeedsAssignment(KHE_TASK task, NEEDS_ASST_TYPE na)  */
/*                                                                           */
/*  Return true if task satisfies na.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSatisfiesNeedsAssignment(KHE_TASK task, NEEDS_ASST_TYPE na)
{
  switch( na )
  {
    case NEEDS_ASST_FALSE:

      return !KheTaskNeedsAssignment(task);

    case NEEDS_ASST_TRUE:

      return KheTaskNeedsAssignment(task);

    case NEEDS_ASST_DONT_CARE:

      return true;

    default:

      HnAbort("KheTaskSatisfiesNeedsAssignment internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskCompatible(KHE_TASK task, KHE_RESOURCE_GROUP rg,             */
/*    int count, KHE_RESOURCE r)                                             */
/*                                                                           */
/*  Return true if task's domain is compatible with rg.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskCompatible(KHE_TASK task, KHE_RESOURCE_GROUP rg,
  int count, KHE_RESOURCE r)
{
  KHE_RESOURCE_GROUP domain;
  domain = KheTaskDomain(task);

  /* check r */
  if( r != NULL && !KheTaskAssignResourceCheck(task, r) )
    return false;

  /* check rg */
  if( rg != NULL && KheResourceGroupIntersectCount(rg, domain) < count )
    return false;

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskIndexInFrameTimeGroup(KHE_TASK task, KHE_FRAME days_frame)    */
/*                                                                           */
/*  Return the index in its days_frame time group of the time that task      */
/*  is running.                                                              */
/*                                                                           */
/*****************************************************************************/

static int KheTaskIndexInFrameTimeGroup(KHE_TASK task, KHE_FRAME days_frame)
{
  KHE_MEET meet;  KHE_TIME t;  KHE_TIME_GROUP tg;  int pos;
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      tg = KheFrameTimeTimeGroup(days_frame, t);
      if( KheTimeGroupContains(tg, t, &pos) )
	return pos;
      HnAbort("KheTaskIndexInFrameTimeGroup internal error");
    }
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGetTask(KHE_TASK_FINDER tf, int day_index,                       */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, KHE_RESOURCE_GROUP *rg, int *tg_start,                 */
/*    bool allow_preassigned, NEEDS_ASST_TYPE na, KHE_INTERVAL disjoint_in,  */
/*    KHE_INTERVAL subset_in, KHE_TASK *task, KHE_INTERVAL *task_in)         */
/*                                                                           */
/*  Try to find a task which satisfies the following conditions.  If         */
/*  successful, set *task to one such task and *task_in to its interval      */
/*  and return true.  If unsuccessful, set *task to NULL and *task_in        */
/*  to an empty interval and return false.  The conditions are:              */
/*                                                                           */
/*    * The task must be a proper root task such that it, or a task          */
/*      assigned to it (directly or indirectly) is running at day_index.     */
/*                                                                           */
/*    * If rtm != NULL, rtm is r's timetable monitor and the task must       */
/*      be assigned r.                                                       */
/*                                                                           */
/*    * If rtm == NULL, so is r and the task must be an unassigned task of   */
/*      type rt which needs assignment.  If *rg != NULL, the task's domain   */
/*      must be compatible with *rg, and it should preferably run at index   */
/*      *tg_start in the times of its day.  If *rg == NULL, KheGetTask       */
/*      resets *rg and *tg_start for next time.  Also, if r != NULL the      */
/*      task must be assignable to r.                                        */
/*                                                                           */
/*    * If allow_preassigned is true, the task may be preassigned.           */
/*                                                                           */
/*    * The task must need assignment or not, depending on na.               */
/*                                                                           */
/*    * The task's interval (including the intervals of all tasks assigned   */
/*      to it, directly or indirectly) must be disjoint from disjoint_in     */
/*      and a subset of subset_in; subset_in must be a legal interval.       */
/*                                                                           */
/*****************************************************************************/

static bool KheGetTask(KHE_TASK_FINDER tf, int day_index,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, KHE_RESOURCE_GROUP *rg, int *tg_start,
  bool allow_preassigned, NEEDS_ASST_TYPE na, KHE_INTERVAL disjoint_in,
  KHE_INTERVAL subset_in, KHE_TASK *task, KHE_INTERVAL *task_in)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, dc;
  KHE_RESOURCE r2;  KHE_MEET meet;
  if( !KheIntervalContains(subset_in, day_index) )
    return false;
  if( KheIntervalContains(disjoint_in, day_index) )
    return false;
  tg = KheFrameTimeGroup(tf->days_frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheGetTask trying %s (rtm %s NULL)\n",
      KheTimeGroupId(tg), rtm != NULL ? "!=" : "==");
  if( rtm != NULL )
  {
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
      for( j = 0;  j < count;  j++ )
      {
	*task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	*task = KheTaskProperRoot(*task);
	if( allow_preassigned || !KheTaskIsPreassigned(*task, &r2) )
	{
	  *task_in = KheTaskFindInterval(tf, *task);
	  if( KheIntervalDisjoint(*task_in, disjoint_in) &&
	      KheIntervalSubset(*task_in, subset_in) &&
              KheTaskSatisfiesNeedsAssignment(*task, na) )
	  {
	    if( DEBUG7 )
	      fprintf(stderr, "  KheGetTask accepting task %s at time %s\n",
		KheTaskId(*task), KheTimeId(t));
	    return true;
	  }
	}
	if( DEBUG7 )
	  fprintf(stderr, "  KheGetTask rejecting task %s at time %s\n",
	    KheTaskId(*task), KheTimeId(t));
      }
    }
  }
  else
  {
    dc = (*rg != NULL ? KheResourceGroupResourceCount(*rg) / 2 : 0);
    count = KheTimeGroupTimeCount(tg);
    for( i = 0;  i < count;  i++ )
    {
      t = KheTimeGroupTime(tg, (i + *tg_start) % count);
      for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(tf->etm, t);  j++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(tf->etm, t, j);
	for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
	{
	  *task = KheMeetTask(meet, k);
	  *task = KheTaskProperRoot(*task);
	  if( KheTaskAsstResource(*task) == NULL &&
	      KheTaskResourceType(*task) == rt &&
	      KheTaskNeedsAssignment(*task) &&
	      KheTaskCompatible(*task, *rg, dc, r) &&
	      (allow_preassigned || !KheTaskIsPreassigned(*task, &r2)) )
	  {
	    *task_in = KheTaskFindInterval(tf, *task);
	    if( KheIntervalDisjoint(*task_in, disjoint_in) &&
		KheIntervalSubset(*task_in, subset_in) )
	    {
	      if( *rg == NULL )
	      {
		*rg = KheTaskDomain(*task);
		*tg_start = KheTaskIndexInFrameTimeGroup(*task, tf->days_frame);
	      }
	      return true;
	    }
	  }
	}
      }
    }
  }
  *task = NULL;
  *task_in = KheIntervalMake(0, -1);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task finders"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_FINDER KheTaskFinderMake(KHE_SOLN soln, KHE_OPTIONS options,    */
/*    KHE_ARENA a)                                                           */
/*                                                                           */
/*  Return a new task finder object, or NULL is there is no frame or etm.    */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_FINDER KheTaskFinderMake(KHE_SOLN soln, KHE_OPTIONS options,
  HA_ARENA a)
{
  KHE_TASK_FINDER res;  KHE_FRAME days_frame;  KHE_EVENT_TIMETABLE_MONITOR etm;

  /* get days_frame and etm and return NULL if can't */
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  if( days_frame == NULL || etm == NULL )
    return NULL;
  if( DEBUG5 )
  {
    fprintf(stderr, "  task finder frame = ");
    KheFrameDebug(days_frame, 2, 2, stderr);
  }

  /* create, initialize, and return the result */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->days_frame = days_frame;
  res->etm = etm;
  HaArrayInit(res->free_daily_schedules, a);
  HaArrayInit(res->free_tasks_and_times, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskFinderLastIndex(KHE_TASK_FINDER tf)                           */
/*                                                                           */
/*  Return the last legal day index.                                         */
/*                                                                           */
/*****************************************************************************/

int KheTaskFinderLastIndex(KHE_TASK_FINDER tf)
{
  return KheFrameTimeGroupCount(tf->days_frame) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheTaskFinderFrame(KHE_TASK_FINDER tf)                         */
/*                                                                           */
/*  Return tf's frame.                                                       */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheTaskFinderFrame(KHE_TASK_FINDER tf)
{
  return tf->days_frame;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskFinderTaskInterval(KHE_TASK_FINDER tf, KHE_TASK task)*/
/*                                                                           */
/*  Return the interval covered by task.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTaskFinderTaskInterval(KHE_TASK_FINDER tf, KHE_TASK task)
{
  return KheTaskFindInterval(tf, task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskFinderTaskSetInterval(KHE_TASK_FINDER tf,            */
/*    KHE_TASK_SET ts)                                                       */
/*                                                                           */
/*  Return the interval covered by ts.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTaskFinderTaskSetInterval(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts)
{
  return KheTaskSetFindInterval(tf, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,          */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Return the interval covered by tg.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg)
{
  KHE_TIME t1, t2;  int count;
  count = KheTimeGroupTimeCount(tg);
  if( count == 0 )
    return KheIntervalMake(1, 0);
  else
  {
    t1 = KheTimeGroupTime(tg, 0);
    t2 = KheTimeGroupTime(tg, count - 1);
    return KheIntervalMake(KheFrameTimeIndex(tf->days_frame, t1),
      KheFrameTimeIndex(tf->days_frame, t2));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "daily schedules"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_TIME KheTaskAndTimeGet(KHE_TASK_FINDER tf)                  */
/*                                                                           */
/*  Get a task and time object, from tf's free list or else from its arena.  */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_TIME KheTaskAndTimeGet(KHE_TASK_FINDER tf)
{
  KHE_TASK_AND_TIME res;
  if( HaArrayCount(tf->free_tasks_and_times) > 0 )
    res = HaArrayLastAndDelete(tf->free_tasks_and_times);
  else
    HaMake(res, tf->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_TIME KheTaskAndTimeMake(KHE_TASK_FINDER tf,                 */
/*    KHE_TASK task, KHE_TIME time)                                          */
/*                                                                           */
/*  Return a new task and time object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_TIME KheTaskAndTimeMake(KHE_TASK_FINDER tf,
  KHE_TASK task, KHE_TIME time)
{
  KHE_TASK_AND_TIME res;
  res = KheTaskAndTimeGet(tf);
  res->task = task;
  res->time = time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAndTimeTypedCmp(KHE_TASK_AND_TIME tt1, KHE_TASK_AND_TIME tt2) */
/*                                                                           */
/*  Typed comparison function for sorting an array of task and time          */
/*  objects by increasing chronological order.                               */
/*                                                                           */
/*****************************************************************************/

static int KheTaskAndTimeTypedCmp(KHE_TASK_AND_TIME tt1, KHE_TASK_AND_TIME tt2)
{
  return KheTimeTypedCmp(tt1->time, tt2->time);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAndTimeCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of task and time        */
/*  objects by increasing chronological order.                               */
/*                                                                           */
/*****************************************************************************/

static int KheTaskAndTimeCmp(const void *t1, const void *t2)
{
  KHE_TASK_AND_TIME tt1, tt2;
  tt1 = * (KHE_TASK_AND_TIME *) t1;
  tt2 = * (KHE_TASK_AND_TIME *) t2;
  return KheTaskAndTimeTypedCmp(tt1, tt2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE KheDailyScheduleGet(KHE_TASK_FINDER tf)               */
/*                                                                           */
/*  Get a daily schedule object, from tf's free list or else from its arena. */
/*                                                                           */
/*****************************************************************************/

static KHE_DAILY_SCHEDULE KheDailyScheduleGet(KHE_TASK_FINDER tf)
{
  KHE_DAILY_SCHEDULE res;
  if( HaArrayCount(tf->free_daily_schedules) > 0 )
  {
    res = HaArrayLastAndDelete(tf->free_daily_schedules);
    HaArrayClear(res->tasks_and_times);
  }
  else
  {
    HaMake(res, tf->arena);
    HaArrayInit(res->tasks_and_times, tf->arena);
    res->task_finder = tf;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDailyScheduleAddTask(KHE_DAILY_SCHEDULE ds, KHE_TASK task)       */
/*                                                                           */
/*  Add task (and all tasks assigned to it, directly or indirectly) to ds.   */
/*                                                                           */
/*****************************************************************************/

static void KheDailyScheduleAddTask(KHE_DAILY_SCHEDULE ds, KHE_TASK task)
{
  KHE_MEET meet;  KHE_TIME t, t2;  KHE_TASK child_task;  int i, durn;

  /* do it for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      for( i = 0;  i < durn;  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	HaArrayAddLast(ds->tasks_and_times,
          KheTaskAndTimeMake(ds->task_finder, task, t2));
      }
    }
  }

  /* do it for the children of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheDailyScheduleAddTask(ds, child_task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDailyScheduleSetFirstAndLastDayIndexes(KHE_DAILY_SCHEDULE ds)    */
/*                                                                           */
/*  Set ds->first_day_index and ds->last_day_index to the indexes of the     */
/*  first and last days covered by ds->tasks_and_times (assumed sorted into  */
/*  chronological order), or to 1 and 0 if ds->tasks_and_times is empty.     */
/*                                                                           */
/*****************************************************************************/

static void KheDailyScheduleSetFirstAndLastDayIndexes(KHE_DAILY_SCHEDULE ds)
{
  KHE_TASK_FINDER tf;  KHE_TASK_AND_TIME tt1, tt2;
  if( HaArrayCount(ds->tasks_and_times) == 0 )
    ds->interval = KheIntervalMake(1, 0);
    /* ds->first_day_index = 1, ds->last_day_index = 0; */
  else
  {
    tf = ds->task_finder;
    tt1 = HaArrayFirst(ds->tasks_and_times);
    /* ds->first_day_index = KheFrameTimeIndex(tf->days_frame, tt->time); */
    tt2 = HaArrayLast(ds->tasks_and_times);
    /* ds->last_day_index = KheFrameTimeIndex(tf->days_frame, tt->time); */
    ds->interval = KheIntervalMake(
      KheFrameTimeIndex(tf->days_frame, tt1->time),
      KheFrameTimeIndex(tf->days_frame, tt2->time));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDailyScheduleRegularize(KHE_DAILY_SCHEDULE ds)                   */
/*                                                                           */
/*  Regularize ds, that is, massage tasks_and_times so that there is one     */
/*  entry for each day.  Also set ds->no_overlap.                            */
/*                                                                           */
/*  Implementation note.  Nearly always, ds will be regularized initially    */
/*  and just needs to be checked.  So we don't worry about the cost of       */
/*  shifting array segments up and down.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheDailyScheduleRegularize(KHE_DAILY_SCHEDULE ds)
{
  int index, i, tt_index;  KHE_TASK_AND_TIME tt;  KHE_TASK_FINDER tf;
  tf = ds->task_finder;
  ds->no_overlap = true;
  index = KheIntervalFirst(ds->interval);
  /* index = ds->first_day_index; */
  HaArrayForEach(ds->tasks_and_times, tt, i)
  {
    /*************************************************************************/
    /*                                                                       */
    /*  Loop invariant:  ds->tasks_and_times[0 .. i-1] has its desired       */
    /*  final value (one time or NULL for each day) for those days, and      */
    /*  index is the index of the next (the ith) day                         */
    /*                                                                       */
    /*************************************************************************/

    tt_index = KheFrameTimeIndex(tf->days_frame, tt->time);
    if( tt_index == index )
    {
      /* tt belongs on the current day, so nothing to do but move on */
      index++;
    }
    else if( tt_index > index )
    {
      /* tt belongs on a later day, so insert a NULL task and time */
      HaArrayAdd(ds->tasks_and_times, i, KheTaskAndTimeMake(tf, NULL, NULL));
      index++;
    }
    else /* tt_index < index */
    {
      /* tt belongs on an earlier day, but too late for that */
      HaArrayDeleteAndShift(ds->tasks_and_times, i);
      i--;
      ds->no_overlap = false;
    }
  }
  HnAssert(index == KheIntervalLast(ds->interval) + 1,
    "KheDailyScheduleRegularize internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE KheTaskFinderTaskDailySchedule(KHE_TASK_FINDER tf,    */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Return a daily schedule for task.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_DAILY_SCHEDULE KheTaskFinderTaskDailySchedule(KHE_TASK_FINDER tf,
  KHE_TASK task)
{
  KHE_DAILY_SCHEDULE res;
  res = KheDailyScheduleGet(tf);
  KheDailyScheduleAddTask(res, task);
  HaArraySort(res->tasks_and_times, &KheTaskAndTimeCmp);
  KheDailyScheduleSetFirstAndLastDayIndexes(res);
  KheDailyScheduleRegularize(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE KheTaskFinderTaskSetDailySchedule(KHE_TASK_FINDER tf, */
/*    KHE_TASK_SET ts)                                                       */
/*                                                                           */
/*  Return a daily schedule for ts.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_DAILY_SCHEDULE KheTaskFinderTaskSetDailySchedule(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts)
{
  KHE_DAILY_SCHEDULE res;  int i;  KHE_TASK task;
  res = KheDailyScheduleGet(tf);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    KheDailyScheduleAddTask(res, task);
  }
  HaArraySort(res->tasks_and_times, &KheTaskAndTimeCmp);
  KheDailyScheduleSetFirstAndLastDayIndexes(res);
  KheDailyScheduleRegularize(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE KheTaskFinderTimeGroupDailySchedule(                  */
/*    KHE_TASK_FINDER tf, KHE_TIME_GROUP tg)                                 */
/*                                                                           */
/*  Return a daily schedule for tg.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_DAILY_SCHEDULE KheTaskFinderTimeGroupDailySchedule(
  KHE_TASK_FINDER tf, KHE_TIME_GROUP tg)
{
  KHE_DAILY_SCHEDULE res;  int i;  KHE_TIME t;
  res = KheDailyScheduleGet(tf);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    HaArrayAddLast(res->tasks_and_times, KheTaskAndTimeMake(tf, NULL, t));
  }
  KheDailyScheduleSetFirstAndLastDayIndexes(res);
  KheDailyScheduleRegularize(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DAILY_SCHEDULE KheTaskFinderNullDailySchedule(KHE_TASK_FINDER tf,    */
/*    KHE_INTERVAL in)                                                       */
/*                                                                           */
/*  Return a null daily schedule for in.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_DAILY_SCHEDULE KheTaskFinderNullDailySchedule(KHE_TASK_FINDER tf,
  KHE_INTERVAL in)
{
  KHE_DAILY_SCHEDULE res;  int index;
  res = KheDailyScheduleGet(tf);
  res->no_overlap = true;
  res->interval = in;
  /*  res->first_day_index = first_day_index; */
  /*  res->last_day_index = last_day_index; */
  for( index = KheIntervalFirst(in);  index <= KheIntervalLast(in);  index++ )
    HaArrayAddLast(res->tasks_and_times, KheTaskAndTimeMake(tf, NULL, NULL));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDailyScheduleDelete(KHE_DAILY_SCHEDULE ds)                       */
/*                                                                           */
/*  Delete ds, that is, add it to the free list in its task finder.          */
/*                                                                           */
/*****************************************************************************/

void KheDailyScheduleDelete(KHE_DAILY_SCHEDULE ds)
{
  KHE_TASK_FINDER tf;  int i;
  tf = ds->task_finder;
  HaArrayAppend(tf->free_tasks_and_times, ds->tasks_and_times, i);
  HaArrayAddLast(tf->free_daily_schedules, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_FINDER KheDailyScheduleTaskFinder(KHE_DAILY_SCHEDULE ds)        */
/*                                                                           */
/*  Return ds's task finder.                                                 */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_FINDER KheDailyScheduleTaskFinder(KHE_DAILY_SCHEDULE ds)
{
  return ds->task_finder;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDailyScheduleNoOverlap(KHE_DAILY_SCHEDULE ds)                    */
/*                                                                           */
/*  Return the no_overlap attribute of ds.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheDailyScheduleNoOverlap(KHE_DAILY_SCHEDULE ds)
{
  return ds->no_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDailyScheduleFirstDayIndex(KHE_DAILY_SCHEDULE ds)                 */
/*                                                                           */
/*  Return the first_day_index attribute of ds.                              */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
int KheDailyScheduleFirstDayIndex(KHE_DAILY_SCHEDULE ds)
{
  return ds->first_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDailyScheduleLastDayIndex(KHE_DAILY_SCHEDULE ds)                  */
/*                                                                           */
/*  Return the last_day_index attribute of ds.                               */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
int KheDailyScheduleLastDayIndex(KHE_DAILY_SCHEDULE ds)
{
  return ds->last_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheDailyScheduleInterval(KHE_DAILY_SCHEDULE ds)             */
/*                                                                           */
/*  Return the interval covered by ds.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheDailyScheduleInterval(KHE_DAILY_SCHEDULE ds)
{
  return ds->interval;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheDailyScheduleTask(KHE_DAILY_SCHEDULE ds, int day_index)      */
/*                                                                           */
/*  Return the task of ds on day_index, or NULL if none.                     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheDailyScheduleTask(KHE_DAILY_SCHEDULE ds, int day_index)
{
  KHE_TASK_AND_TIME tt;
  HnAssert(KheIntervalContains(ds->interval, day_index),
    "KheDailyScheduleTask: day_index (%d) out of range (%s)",
    day_index, KheIntervalShow(ds->interval, NULL));
  tt = HaArray(ds->tasks_and_times, day_index - KheIntervalFirst(ds->interval));
  return tt->task;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheDailyScheduleTime(KHE_DAILY_SCHEDULE ds, int day_index)      */
/*                                                                           */
/*  Return the time of ds on day_index, or NULL if none.                     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheDailyScheduleTime(KHE_DAILY_SCHEDULE ds, int day_index)
{
  KHE_TASK_AND_TIME tt;
  HnAssert(KheIntervalContains(ds->interval, day_index),
    "KheDailyScheduleTime: day_index (%d) out of range (%s)",
    day_index, KheIntervalShow(ds->interval, NULL));
  tt = HaArray(ds->tasks_and_times, day_index - KheIntervalFirst(ds->interval));
  return tt->time;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheTimetableMonitor(KHE_SOLN soln,        */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return r's timetable monitor, or NULL if r is NULL.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_TIMETABLE_MONITOR KheTimetableMonitor(KHE_SOLN soln,
  KHE_RESOURCE r)
{
  return (r == NULL ? NULL : KheResourceTimetableMonitor(soln, r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFindTasksInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,         */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,     */
/*    bool allow_partial, KHE_TASK_SET res_ts, KHE_INTERVAL *res_in)         */
/*                                                                           */
/*  Set res_ts to the tasks in interval in, and set *res_in to the           */
/*  bounding interval of those tasks.                                        */
/*                                                                           */
/*****************************************************************************/

void KheFindTasksInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, KHE_TASK_SET res_ts, KHE_INTERVAL *res_in)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int tg_start, i;  KHE_TASK task;
  KHE_INTERVAL task_in, disjoint_in, subset_in;  KHE_RESOURCE_GROUP rg;
  disjoint_in = KheIntervalMake(1, 0);
  subset_in = allow_partial ?
    KheIntervalMake(0, KheTaskFinderLastIndex(tf)) : in;
  rtm = KheTimetableMonitor(tf->soln, from_r);
  rg = NULL;
  tg_start = 0;
  KheTaskSetClear(res_ts);
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    if( KheGetTask(tf, i, rtm, rt, from_r, &rg, &tg_start, allow_preassigned,
	  NEEDS_ASST_DONT_CARE, disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      disjoint_in = KheIntervalUnion(disjoint_in, task_in);
    }
  }
  *res_in = disjoint_in;
}

/* ***
bool KheFindTasksInInterval(KHE_TASK_FINDER tf, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE from_r, int first_index, int last_index, KHE_TASK_SET res_ts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  struct khe_part_to_rec pt_rec;
  KHE_INTERVAL in = KheIntervalMake(first_index, last_index);
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KhePartToInit(&pt_rec, res_ts);
  HnAbort("KheFindTasksInInterval not re-implemented yet");
  return KheFindTo(tf, in, true, rtm, false, NULL, &pt_rec);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindRunAtIndex(KHE_TASK_FINDER tf, int index,                    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, bool sep_need_asst,            */
/*    KHE_INTERVAL *disjoint_in, KHE_INTERVAL subset_in, KHE_TASK_SET res_ts)*/
/*                                                                           */
/*  If a suitable run overlaps index, set res_ts to it and return true.      */
/*                                                                           */
/*****************************************************************************/

static bool KheFindRunAtIndex(KHE_TASK_FINDER tf, int index,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool allow_preassigned, bool sep_need_asst,
  KHE_INTERVAL *disjoint_in, KHE_INTERVAL subset_in, KHE_TASK_SET res_ts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_TASK task;  KHE_INTERVAL task_in;
  NEEDS_ASST_TYPE na;
  rg = NULL;
  tg_start = 0;
  if( KheGetTask(tf, index, rtm, rt, r, &rg, &tg_start, allow_preassigned,
      NEEDS_ASST_DONT_CARE, *disjoint_in, subset_in, &task, &task_in) )
  {
    /* found a first task; save it, then find the rest of the run and stop */
    na = (!sep_need_asst ? NEEDS_ASST_DONT_CARE :
      KheTaskNeedsAssignment(task) ? NEEDS_ASST_TRUE : NEEDS_ASST_FALSE);
    KheTaskSetAddTask(res_ts, task);
    *disjoint_in = KheIntervalUnion(*disjoint_in, task_in);
    while( KheGetTask(tf, disjoint_in->last + 1, rtm, rt, r, &rg, &tg_start,
	allow_preassigned, na, *disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      *disjoint_in = KheIntervalUnion(*disjoint_in, task_in);
    }
    while( KheGetTask(tf, disjoint_in->first - 1, rtm, rt, r, &rg, &tg_start,
	allow_preassigned, na, *disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      *disjoint_in = KheIntervalUnion(*disjoint_in, task_in);
    }
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceShow(KHE_RESOURCE r)                                    */
/*                                                                           */
/*  Return the Id of r, or "@" if NULL.                                      */
/*                                                                           */
/*****************************************************************************/

static char *KheResourceShow(KHE_RESOURCE r)
{
  return r == NULL ? "@" : KheResourceId(r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindFirstRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,      */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,     */
/*    bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,           */
/*    KHE_INTERVAL *res_in)                                                  */
/*                                                                           */
/*  Find the first (leftmost) run for from_r in in, set res_ts to it, and    */
/*  set *res_in to its bounding interval.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheFindFirstRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,
  KHE_INTERVAL *res_in)
{
  int i;  KHE_INTERVAL disjoint_in, subset_in;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG6 )
    fprintf(stderr, "[ KheFindFirstRunInInterval(tf, %s, %s, %s, %s, %s)\n",
      KheIntervalShow(in, NULL), KheResourceTypeName(rt),
      KheResourceShow(from_r), bool_show(allow_preassigned),
      bool_show(allow_partial));

  /* initialize everything */
  disjoint_in = KheIntervalMake(1, 0);
  subset_in = allow_partial ?
    KheIntervalMake(0, KheTaskFinderLastIndex(tf)) : in;
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KheTaskSetClear(res_ts);

  /* search right from first_index for the first run */
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    if( KheFindRunAtIndex(tf, i, rtm, rt, from_r, allow_preassigned,
	  sep_need_asst, &disjoint_in, subset_in, res_ts) )
      break;
  }

  /* return true if KheTaskSetTaskCount(res_ts) > 0 */
  *res_in = disjoint_in;
  if( DEBUG6 )
  {
    if( KheTaskSetTaskCount(res_ts) > 0 )
      fprintf(stderr, "] KheFindFirstRunInInterval returning true (%s)\n",
	KheIntervalShow(*res_in, tf->days_frame));
    else
      fprintf(stderr, "] KheFindFirstRunInInterval returning false\n");
  }
  return KheTaskSetTaskCount(res_ts) > 0;
}


/* *** old version
bool KheFindTaskRunRight(KHE_TASK_FINDER tf, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE from_r, int first_index,
  bool allow_partial, bool allow_preassigned,
  KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;
  KHE_INTERVAL in;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_PART_FROM pf;
  struct khe_part_from_rec res_pf;  ** bool need_asst; **

  if( DEBUG6 )
    fprintf(stderr, "[ KheFindTaskRunRight(tf, %s, %s, %d, %s, %s, -)\n",
      KheResourceTypeName(rt), KheResourceShow(from_r), first_index,
      bool_show(allow_partial), bool_show(allow_preassigned));
  HnAbort("KheFindTaskRunRight: revised version sti ll to do");

  rtm = KheTimetableMonitor(tf->soln, from_r);
  if( DEBUG6 && rtm != NULL )
    KheResourceTimetableMonitorPrintTimetable(rtm, 10, 2, stderr);
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  in = KheIntervalMake(0, first_index - 1);
  rg = NULL, tg_start = 0;
  pf = &tf->scratch_from;
  for( i = first_index;  i < tg_count;  i++ )
  {
    ** ***
    if( DEBUG6 )
      fprintf(stderr, "  trying day %d:\n", i);
    *** **
    if( KheFindFrom(tf, i, &in, rtm, rt, NULL, true, &rg, &tg_start, pf) )
    {
      ** repeatedly add pf to the run and skip to after it **
      ** need_asst = KhePartFromNeedAsst(pf); **
      KhePartFromInit(&res_pf, res_ts);
      do
      {
	KhePartFromUnion(&res_pf, pf);
        i = in.last + 1;
      } while( KheFindFrom(tf, i, &in, rtm, rt, NULL, true, &rg,&tg_start,pf) );
	** && (!separate_need_asst || KhePartFromNeedAsst(pf)==need_asst) ); **

      ** run ends here **
      *res_first_index = res_pf.in.first;
      *res_last_index = res_pf.in.last;
      HnAssert(*res_first_index >= 0, "KheFindTaskRunRight internal error 1");
      if( DEBUG6 )
	fprintf(stderr, "] KheFindTaskRunRight returning true (%d..%d)\n",
	  *res_first_index, *res_last_index);
      return true;
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheFindTaskRunRight returning false\n");
  return *res_first_index = *res_last_index = -1, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindLastRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,       */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,     */
/*    bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,           */
/*    KHE_INTERVAL *res_in)                                                  */
/*                                                                           */
/*  Find the last (rightmost) run for from_r in in, set res_ts to it, and    */
/*  set *res_in to its bounding interval.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheFindLastRunInInterval(KHE_TASK_FINDER tf, KHE_INTERVAL in,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r, bool allow_preassigned,
  bool allow_partial, bool sep_need_asst, KHE_TASK_SET res_ts,
  KHE_INTERVAL *res_in)
{
  int i;  KHE_INTERVAL disjoint_in, subset_in;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG6 )
    fprintf(stderr, "[ KheFindLastRunInInterval(tf, %s, %s, %s, %s, %s)\n",
      KheIntervalShow(in, NULL), KheResourceTypeName(rt),
      KheResourceShow(from_r), bool_show(allow_preassigned),
      bool_show(allow_partial));

  /* initialize everything */
  disjoint_in = KheIntervalMake(1, 0);
  subset_in = allow_partial ?
    KheIntervalMake(0, KheTaskFinderLastIndex(tf)) : in;
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KheTaskSetClear(res_ts);

  /* search left from last_index for the first run */
  for( i = KheIntervalLast(in);  i >= KheIntervalFirst(in);  i-- )
  {
    if( KheFindRunAtIndex(tf, i, rtm, rt, from_r, allow_preassigned,
	  sep_need_asst, &disjoint_in, subset_in, res_ts) )
      break;
  }

  /* return true if KheTaskSetTaskCount(res_ts) > 0 */
  *res_in = disjoint_in;
  if( DEBUG6 )
  {
    if( KheTaskSetTaskCount(res_ts) > 0 )
      fprintf(stderr, "] KheFindLastRunInInterval returning true (%s)\n",
	KheIntervalShow(*res_in, tf->days_frame));
    else
      fprintf(stderr, "] KheFindLastRunInInterval returning false\n");
  }
  return KheTaskSetTaskCount(res_ts) > 0;
}


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

typedef struct khe_interval_rec {
  int first;
  int last;
} KHE_INTERVAL;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_FROM - what we need to know about what from_r is doing          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_part_from_rec {
  KHE_TASK_SET			ts;
  KHE_INTERVAL			in;
} *KHE_PART_FROM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART_FROM - what we need to know about what from_r is doing         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mpart_from_rec {
  KHE_MTASK_SET			mts;
  KHE_INTERVAL			in;
} *KHE_MPART_FROM;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART_TO - what we need to know about what to_r is doing              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_part_to_rec {
  KHE_TASK_SET			ts;
  int				durn;
  bool				effectively_free;
} *KHE_PART_TO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART_TO - what we need to know about what to_r is doing             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mpart_to_rec {
  KHE_MTASK_SET			mts;
  int				durn;
  bool				effectively_free;
} *KHE_MPART_TO;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART                                                                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_part_rec {
  struct khe_part_from_rec		from;
  struct khe_part_to_rec		to;
  bool					curr_is_swap;
  bool					best_is_swap;
} *KHE_PART;

typedef HA_ARRAY(KHE_PART) ARRAY_KHE_PART;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART                                                                */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mpart_rec {
  struct khe_mpart_from_rec		from;
  struct khe_mpart_to_rec		to;
  bool					curr_is_swap;
  bool					best_is_swap;
} *KHE_MPART;

typedef HA_ARRAY(KHE_MPART) ARRAY_KHE_MPART;


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
  int				first_day_index;
  int				last_day_index;
  ARRAY_KHE_TASK_AND_TIME	tasks_and_times;
};

typedef HA_ARRAY(KHE_DAILY_SCHEDULE) ARRAY_KHE_DAILY_SCHEDULE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_FINDER                                                          */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_WIDENED_TASK_SET) ARRAY_KHE_WIDENED_TASK_SET;
typedef HA_ARRAY(KHE_WIDENED_MTASK_SET) ARRAY_KHE_WIDENED_MTASK_SET;

struct khe_task_finder_rec {
  HA_ARENA				arena;
  KHE_SOLN				soln;
  KHE_FRAME				days_frame;
  KHE_EVENT_TIMETABLE_MONITOR		etm;
  struct khe_part_from_rec		scratch_from;
  struct khe_part_to_rec		scratch_to;
  struct khe_mpart_from_rec		scratch_mfrom;
  struct khe_mpart_to_rec		scratch_mto;
  ARRAY_KHE_WIDENED_TASK_SET		free_widened_task_sets;
  ARRAY_KHE_WIDENED_MTASK_SET		free_widened_mtask_sets;
  ARRAY_KHE_PART			free_parts;
  ARRAY_KHE_MPART			free_mparts;
  ARRAY_KHE_DAILY_SCHEDULE		free_daily_schedules;
  ARRAY_KHE_TASK_AND_TIME		free_tasks_and_times;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_TASK_SET - a widened task set, with a core and wings         */
/*                                                                           */
/*****************************************************************************/

struct khe_widened_task_set_rec {
  KHE_TASK_FINDER			tf;
  KHE_RESOURCE				from_r;
  KHE_RESOURCE_TIMETABLE_MONITOR	from_r_rtm;
  KHE_RESOURCE				to_r;
  KHE_RESOURCE_TIMETABLE_MONITOR	to_r_rtm;
  struct khe_part_rec			core;
  ARRAY_KHE_PART			left_wing;
  ARRAY_KHE_PART			right_wing;
  int					to_r_max_left_wing_count;
  int					to_r_max_right_wing_count;

  /* optimal moves */
  ARRAY_KHE_PART			swappable_wings;
  KHE_COST				best_cost;
  int					best_defects;
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_MTASK_SET - a widened mtask set, with a core and wings       */
/*                                                                           */
/*****************************************************************************/

struct khe_widened_mtask_set_rec {
  KHE_TASK_FINDER			tf;
  KHE_RESOURCE				from_r;
  KHE_RESOURCE_TIMETABLE_MONITOR	from_r_rtm;
  KHE_RESOURCE				to_r;
  KHE_RESOURCE_TIMETABLE_MONITOR	to_r_rtm;
  struct khe_mpart_rec			core;
  ARRAY_KHE_MPART			left_wing;
  ARRAY_KHE_MPART			right_wing;
  int					to_r_max_left_wing_count;
  int					to_r_max_right_wing_count;

  /* optimal moves */
  ARRAY_KHE_MPART			swappable_wings;
  KHE_COST				best_cost;
  int					best_defects;
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

static KHE_INTERVAL KheInterval(int first, int last)
{
  KHE_INTERVAL res;
  res.first = first;
  res.last = last;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntervalLength(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return the length of in.                                                 */
/*                                                                           */
/*****************************************************************************/

static int KheIntervalLength(KHE_INTERVAL in)
{
  return in.last - in.first + 1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalEmpty(KHE_INTERVAL in)                                   */
/*                                                                           */
/*  Return true if in is empty.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalEmpty(KHE_INTERVAL in)
{
  return in.first > in.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalContains(KHE_INTERVAL in, int index)                     */
/*                                                                           */
/*  Return true if in contains index.                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalContains(KHE_INTERVAL in, int index)
{
  return index >= in.first && index <= in.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)               */
/*                                                                           */
/*  Return true if in1 is a subset of in2.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalSubset(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) ||
    (in2.first <= in1.first && in1.last <= in2.last);
}


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

static bool KheIntervalDisjoint(KHE_INTERVAL in1, KHE_INTERVAL in2)
{
  return KheIntervalEmpty(in1) || KheIntervalEmpty(in2) ||
    in1.last < in2.first || in2.last < in1.first;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalUnion(KHE_INTERVAL *in1, KHE_INTERVAL in2)               */
/*                                                                           */
/*  Overwrite *in1 with its union with in2.                                  */
/*                                                                           */
/*****************************************************************************/

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


/*****************************************************************************/
/*                                                                           */
/*  char *KheTaskFinderIntervalShow(KHE_TASK_FINDER tf, KHE_INTERVAL in)     */
/*                                                                           */
/*  Show in in static memory.                                                */
/*                                                                           */
/*****************************************************************************/

#ifdef DEBUG4
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
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoInterval(KHE_TASK task, KHE_FRAME frame, KHE_INTERVAL *in) */
/*                                                                           */
/*  Do that part of the work of KheTaskSetInterval below that pertains to    */
/*  task and its descendants.                                                */
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
/*  KHE_INTERVAL KheTaskInterval(KHE_TASK_FINDER tf, KHE_TASK task)          */
/*                                                                           */
/*  Return the interval covered by task.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheTaskInterval(KHE_TASK_FINDER tf, KHE_TASK task)
{
  KHE_INTERVAL res;
  res.first = KheFrameTimeGroupCount(tf->days_frame) - 1;
  res.last = 0;
  KheTaskDoInterval(task, tf->days_frame, &res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskInterval(KHE_TASK_FINDER tf, KHE_MTASK mt)          */
/*                                                                           */
/*  Return the interval covered by mt.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheMTaskInterval(KHE_TASK_FINDER tf, KHE_MTASK mt)
{
  KHE_INTERVAL res;
  res.first = KheMTaskFirstDayIndex(mt);
  res.last = KheMTaskLastDayIndex(mt);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskSetInterval(KHE_TASK_FINDER tf,  KHE_TASK_SET ts)    */
/*                                                                           */
/*  Return the interval covered by ts.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheTaskSetInterval(KHE_TASK_FINDER tf,  KHE_TASK_SET ts)
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
/*  KHE_INTERVAL KheTaskSetInterval(KHE_TASK_FINDER tf,  KHE_TASK_SET ts)    */
/*                                                                           */
/*  Return the interval covered by ts.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheMTaskSetInterval(KHE_TASK_FINDER tf,  KHE_MTASK_SET mts)
{
  int i, first_day_index, last_day_index;  KHE_MTASK mt;  KHE_INTERVAL res;
  res.first = KheFrameTimeGroupCount(tf->days_frame) - 1;
  res.last = 0;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    first_day_index = KheMTaskFirstDayIndex(mt);
    last_day_index = KheMTaskLastDayIndex(mt);
    if( first_day_index < res.first )
      res.first = first_day_index;
    if( last_day_index < res.last )
      res.last = last_day_index;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoFirstAndLastTimes(KHE_TASK task, int *first_time_index,    */
/*    int *last_time_index)                                                  */
/*                                                                           */
/*  Update *first_time_index and *last_time_index to ensure that that        */
/*  interval covers all the times that task and its descendants are running. */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheTaskDoFirstAndLastTimes(KHE_TASK task, int *first_time_index,
  int *last_time_index)
{
  KHE_MEET meet;  KHE_TIME t;  KHE_TASK child_task;  int i, durn, fi, li;

  ** do it for task itself **
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      fi = KheTimeIndex(t);
      li = fi + durn - 1;
      if( fi < *first_time_index )
	*first_time_index = fi;
      if( li > *last_time_index )
	*last_time_index = li;
    }
  }

  ** do it for the children of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskDoFirstAndLastTimes(child_task, first_time_index, last_time_index);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFirstAndLastTimes(KHE_TASK task, KHE_TIME *first_time,       */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Return the first and last times covered by task.                         */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheTaskFirstAndLastTimes(KHE_TASK task, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  KHE_INSTANCE ins;  int first_time_index, last_time_index;
  ins = KheSolnInstance(KheTaskSoln(task));
  first_time_index = INT_MAX;
  last_time_index = 0;
  KheTaskDoFirstAndLastTimes(task, &first_time_index, &last_time_index);
  if( first_time_index < INT_MAX )
  {
    *first_time = KheInstanceTime(ins, first_time_index);
    *last_time = KheInstanceTime(ins, last_time_index);
  }
  else
  {
    *first_time = NULL;
    *last_time = NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetFirstAndLastTimes(KHE_TASK_SET ts, KHE_TIME *first_time,  */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Set *first_time and *last_time to the first and last times covered       */
/*  by ts, or to NULL if ts does not cover any times.                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheTaskSetFirstAndLastTimes(KHE_TASK_SET ts, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  KHE_INSTANCE ins;  KHE_TASK task;  int i, first_time_index, last_time_index;
  ins = KheSolnInstance(KheTaskSetSoln(ts));
  first_time_index = INT_MAX;
  last_time_index = 0;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    KheTaskDoFirstAndLastTimes(task, &first_time_index, &last_time_index);
  }
  if( first_time_index < INT_MAX )
  {
    *first_time = KheInstanceTime(ins, first_time_index);
    *last_time = KheInstanceTime(ins, last_time_index);
  }
  else
  {
    *first_time = NULL;
    *last_time = NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAddTimes(KHE_TASK task, ARRAY_KHE_TIME *times)               */
/*                                                                           */
/*  Add the times of task and its descendants to *times.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskAddTimes(KHE_TASK task, ARRAY_KHE_TIME *times)
{
  KHE_MEET meet;  KHE_TIME t, t2;  KHE_TASK child_task;  int i, j, durn;

  ** do it for task itself **
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      for( j = 0;  j < durn;  j++ )
      {
	t2 = KheTimeNeighbour(t, j);
	HaArrayAddLast(*times, t2);
      }
    }
  }

  ** do it for the children of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskAddTimes(child_task, times);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "from parts" (private)                                         */
/*                                                                           */
/*****************************************************************************/

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
/*  void KhePartFromInit(KHE_PART_FROM pf, KHE_TASK_SET ts)                  */
/*                                                                           */
/*  Initialize pf to empty, using ts.                                        */
/*                                                                           */
/*****************************************************************************/

static void KhePartFromInit(KHE_PART_FROM pf, KHE_TASK_SET ts)
{
  pf->ts = ts;
  KheTaskSetClear(ts);
  pf->in = KheInterval(1, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromInit(KHE_PART_FROM pf, KHE_TASK_SET ts)                  */
/*                                                                           */
/*  Initialize pf to empty, using ts.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMPartFromInit(KHE_MPART_FROM pf, KHE_MTASK_SET mts)
{
  pf->mts = mts;
  KheMTaskSetClear(mts);
  pf->in = KheInterval(1, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromClear(KHE_PART_FROM pf)                                  */
/*                                                                           */
/*  Clear pf back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KhePartFromClear(KHE_PART_FROM pf)
{
  KheTaskSetClear(pf->ts);
  pf->in = KheInterval(1, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromClear(KHE_PART_FROM pf)                                  */
/*                                                                           */
/*  Clear pf back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheMPartFromClear(KHE_MPART_FROM pf)
{
  KheMTaskSetClear(pf->mts);
  pf->in = KheInterval(1, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromAddTask(KHE_PART_FROM pf, KHE_TASK task,                 */
/*    KHE_INTERVAL task_in)                                                  */
/*                                                                           */
/*  Add task, whose interval is task_in, to pf.                              */
/*                                                                           */
/*****************************************************************************/

static void KhePartFromAddTask(KHE_PART_FROM pf, KHE_TASK task,
  KHE_INTERVAL task_in)
{
  KheTaskSetAddTask(pf->ts, task);
  KheIntervalUnion(&pf->in, task_in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromAddTask(KHE_PART_FROM pf, KHE_TASK task,                 */
/*    KHE_INTERVAL task_in)                                                  */
/*                                                                           */
/*  Add task, whose interval is task_in, to pf.                              */
/*                                                                           */
/*****************************************************************************/

static void KheMPartFromAddMTask(KHE_MPART_FROM pf, KHE_MTASK mt,
  KHE_INTERVAL mt_in)
{
  KheMTaskSetAddMTask(pf->mts, mt);
  KheIntervalUnion(&pf->in, mt_in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromAddTasks(KHE_PART_FROM pf, KHE_TASK_SET ts,              */
/*    KHE_INTERVAL ts_in)                                                    */
/*                                                                           */
/*  Add the tasks of ts to pf.  Together they cover ts_in.                   */
/*                                                                           */
/*****************************************************************************/

static void KhePartFromAddTasks(KHE_PART_FROM pf, KHE_TASK_SET ts,
  KHE_INTERVAL ts_in)
{
  int i;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
    KheTaskSetAddTask(pf->ts, KheTaskSetTask(ts, i));
  KheIntervalUnion(&pf->in, ts_in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromAddTasks(KHE_PART_FROM pf, KHE_TASK_SET ts,              */
/*    KHE_INTERVAL ts_in)                                                    */
/*                                                                           */
/*  Add the tasks of ts to pf.  Together they cover ts_in.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMPartFromAddMTasks(KHE_MPART_FROM pf, KHE_MTASK_SET mts,
  KHE_INTERVAL mts_in)
{
  int i;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
    KheMTaskSetAddMTask(pf->mts, KheMTaskSetMTask(mts, i));
  KheIntervalUnion(&pf->in, mts_in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromUnion(KHE_PART_FROM target_pf, KHE_PART_FROM source_pf)  */
/*                                                                           */
/*  Add source_pf's tasks to target_pf.                                      */
/*                                                                           */
/*****************************************************************************/

static void KhePartFromUnion(KHE_PART_FROM target_pf, KHE_PART_FROM source_pf)
{
  KhePartFromAddTasks(target_pf, source_pf->ts, source_pf->in);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartFromUnion(KHE_PART_FROM target_pf, KHE_PART_FROM source_pf)  */
/*                                                                           */
/*  Add source_pf's tasks to target_pf.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheMPartFromUnion(KHE_MPART_FROM target_pf,
  KHE_MPART_FROM source_pf)
{
  KheMPartFromAddMTasks(target_pf, source_pf->mts, source_pf->in);
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
/*  int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt, KHE_FRAME days_frame)    */
/*                                                                           */
/*  Return the index in its days_frame time group of the time that task      */
/*  is running.                                                              */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt, KHE_FRAME days_frame)
{
  KHE_MEET meet;  KHE_TIME t;  KHE_TIME_GROUP tg;  int pos;
  KHE_COST asst_cost, non_asst_cost;  KHE_TASK task;
  /* what if mt does not have fixed times?  still to do */
  task = KheMTaskTask(mt, 0, &asst_cost, &non_asst_cost);
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
/*  bool KheFindTask(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,        */
/*    int *tg_start, KHE_TASK *task, KHE_INTERVAL *task_in)                  */
/*                                                                           */
/*  Set *task to hold a proper root task running on day_index but not during */
/*  *in, set *task_in to that task's interval, and replace *in by its union  */
/*  with *task_in.  Or if no such task can be found, clear set *task to NULL */
/*  and return false.                                                        */
/*                                                                           */
/*  If allow_preassigned is false, only unpreassigned tasks may be found.    */
/*  If allow_preassigned is true, tasks may be preassigned or unpreassigned. */
/*                                                                           */
/*  If rtm != NULL, the task is to come from rtm and only the requirements   */
/*  just given apply.  If rtm == NULL, the task is to be an unassigned       */
/*  task of type rt.  In that case, the task must need assignment, and if    */
/*  *rg != NULL, the task's domain must be compatible with *rg, and it       */
/*  should preferably run at index *tg_start in the times of its day.        */
/*  If *rg == NULL KheFindTask resets it and *tg_start for next time.        */
/*  Also, if r != NULL the task must be assignable to r.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheFindTask(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,
  int *tg_start, KHE_TASK *task, KHE_INTERVAL *task_in)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, dc;
  KHE_RESOURCE r2;  KHE_MEET meet;
  if( day_index < 0 || day_index >= KheFrameTimeGroupCount(tf->days_frame) )
    return false;
  tg = KheFrameTimeGroup(tf->days_frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheFindTask trying %s (rtm %s NULL)\n",
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
	  *task_in = KheTaskInterval(tf, *task);
	  if( KheIntervalDisjoint(*task_in, *in) )
	  {
	    KheIntervalUnion(in, *task_in);
	    return true;
	  }
	}
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
	      !KheTaskIsPreassigned(*task, &r2) &&
	      KheTaskCompatible(*task, *rg, dc, r) )
	  {
	    *task_in = KheTaskInterval(tf, *task);
	    if( KheIntervalDisjoint(*task_in, *in) )
	    {
	      KheIntervalUnion(in, *task_in);
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
  *task_in = KheInterval(-1, -1);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindTask(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,        */
/*    int *tg_start, KHE_TASK *task, KHE_INTERVAL *task_in)                  */
/*                                                                           */
/*  Set *task to hold a proper root task running on day_index but not during */
/*  *in, set *task_in to that task's interval, and replace *in by its union  */
/*  with *task_in.  Or if no such task can be found, clear set *task to NULL */
/*  and return false.                                                        */
/*                                                                           */
/*  If allow_preassigned is false, only unpreassigned tasks may be found.    */
/*  If allow_preassigned is true, tasks may be preassigned or unpreassigned. */
/*                                                                           */
/*  If rtm != NULL, the task is to come from rtm and only the requirements   */
/*  just given apply.  If rtm == NULL, the task is to be an unassigned       */
/*  task of type rt.  In that case, the task must need assignment, and if    */
/*  *rg != NULL, the task's domain must be compatible with *rg, and it       */
/*  should preferably run at index *tg_start in the times of its day.        */
/*  If *rg == NULL KheFindTask resets it and *tg_start for next time.        */
/*  Also, if r != NULL the task must be assignable to r.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheFindMTask(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,
  int *tg_start, KHE_MTASK *mt, KHE_INTERVAL *mt_in,
  KHE_MTASK_FINDER mtf)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, dc;
  KHE_RESOURCE r2;  KHE_MEET meet;  KHE_TASK task;
  if( day_index < 0 || day_index >= KheFrameTimeGroupCount(tf->days_frame) )
    return false;
  tg = KheFrameTimeGroup(tf->days_frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheFindMTask trying %s (rtm %s NULL)\n",
      KheTimeGroupId(tg), rtm != NULL ? "!=" : "==");
  if( rtm != NULL )
  {
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
      for( j = 0;  j < count;  j++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	/* *task = KheTaskProperRoot(*task); */
	*mt = KheMTaskFinderTaskToMTask(mtf, task);
	if( allow_preassigned || !KheMTaskIsPreassigned(*mt, &r2) )
	{
	  *mt_in = KheMTaskInterval(tf, *mt);
	  if( KheIntervalDisjoint(*mt_in, *in) )
	  {
	    KheIntervalUnion(in, *mt_in);
	    return true;
	  }
	}
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
	  task = KheMeetTask(meet, k);
	  /* *task = KheTaskProperRoot(*task); */
	  *mt = KheMTaskFinderTaskToMTask(mtf, task);
	  if( KheTaskAsstResource(task) == NULL &&
	      KheTaskResourceType(task) == rt &&
	      !KheTaskIsPreassigned(task, &r2) &&
	      KheTaskCompatible(task, *rg, dc, r) )
	  {
	    *mt_in = KheMTaskInterval(tf, *mt);
	    if( KheIntervalDisjoint(*mt_in, *in) )
	    {
	      KheIntervalUnion(in, *mt_in);
	      if( *rg == NULL )
	      {
		*rg = KheMTaskDomain(*mt);
		*tg_start = KheMTaskIndexInFrameTimeGroup(*mt, tf->days_frame);
	      }
	      return true;
	    }
	  }
	}
      }
    }
  }
  *mt = NULL;
  *mt_in = KheInterval(-1, -1);
  return false;
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
/*  bool KheGetTask(KHE_TASK_FINDER tf, int day_index,                       */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, KHE_RESOURCE_GROUP *rg, int *tg_start,                 */
/*    bool ignore_preassigned, NEEDS_ASST_TYPE na, KHE_INTERVAL disjoint_in, */
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
/*    * If ignore_preassigned is true, the task may not be preassigned.      */
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
  bool ignore_preassigned, NEEDS_ASST_TYPE na, KHE_INTERVAL disjoint_in,
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
	if( !ignore_preassigned || !KheTaskIsPreassigned(*task, &r2) )
	{
	  *task_in = KheTaskInterval(tf, *task);
	  if( KheIntervalDisjoint(*task_in, disjoint_in) &&
	      KheIntervalSubset(*task_in, subset_in) &&
              KheTaskSatisfiesNeedsAssignment(*task, na) )
	    return true;
	}
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
	      (!ignore_preassigned || !KheTaskIsPreassigned(*task, &r2)) )
	  {
	    *task_in = KheTaskInterval(tf, *task);
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
  *task_in = KheInterval(0, -1);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindFrom(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,        */
/*    int *tg_start, KHE_PART_FROM pf)                                       */
/*                                                                           */
/*  Set pf to hold a proper root task running on day_index but not during    */
/*  *in, and replace *in by its union with that task's interval.  Or if no   */
/*  such task can be found, clear pf and return false.                       */
/*                                                                           */
/*  If allow_preassigned is false, only unpreassigned tasks may be found.    */
/*  If allow_preassigned is true, tasks may be preassigned or unpreassigned. */
/*                                                                           */
/*  If rtm != NULL, the task is to come from rtm and only the requirements   */
/*  just given apply.  If rtm == NULL, the task is to be an unassigned       */
/*  task of type rt.  In that case, the task must need assignment, and if    */
/*  *rg != NULL, the task's domain must be compatible with *rg, and it       */
/*  should preferably run at index *tg_start in the times of its day.        */
/*  If *rg == NULL KheFindFrom resets it and *tg_start for next time.        */
/*  Also, if r != NULL the task must be assignable to r.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFindFrom(KHE_TASK_FINDER tf, int day_index,
  KHE_INTERVAL *in, KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE r, bool allow_preassigned,
  KHE_RESOURCE_GROUP *rg, int *tg_start, KHE_MPART_FROM pf,
  KHE_MTASK_FINDER mtf)
{
  KHE_MTASK mt;  KHE_INTERVAL mt_in;
  KheMPartFromClear(pf);
  if( KheFindMTask(tf, day_index, in, rtm, rt, r, allow_preassigned, rg,
      tg_start, &mt, &mt_in, mtf) )
  {
    KheMPartFromAddMTask(pf, mt, mt_in);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindFrom(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,        */
/*    int *tg_start, KHE_PART_FROM pf)                                       */
/*                                                                           */
/*  Set pf to hold a proper root task running on day_index but not during    */
/*  *in, and replace *in by its union with that task's interval.  Or if no   */
/*  such task can be found, clear pf and return false.                       */
/*                                                                           */
/*  If allow_preassigned is false, only unpreassigned tasks may be found.    */
/*  If allow_preassigned is true, tasks may be preassigned or unpreassigned. */
/*                                                                           */
/*  If rtm != NULL, the task is to come from rtm and only the requirements   */
/*  just given apply.  If rtm == NULL, the task is to be an unassigned       */
/*  task of type rt.  In that case, the task must need assignment, and if    */
/*  *rg != NULL, the task's domain must be compatible with *rg, and it       */
/*  should preferably run at index *tg_start in the times of its day.        */
/*  If *rg == NULL KheFindFrom resets it and *tg_start for next time.        */
/*  Also, if r != NULL the task must be assignable to r.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheFindFrom(KHE_TASK_FINDER tf, int day_index, KHE_INTERVAL *in,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,
  int *tg_start, KHE_PART_FROM pf)
{
  KHE_TASK task;  KHE_INTERVAL task_in;
  KhePartFromClear(pf);
  if( KheFindTask(tf, day_index, in, rtm, rt, r, allow_preassigned, rg,
      tg_start, &task, &task_in) )
  {
    KhePartFromAddTask(pf, task, task_in);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartFromNeedAsst(KHE_PART_FROM pf)                               */
/*                                                                           */
/*  Return true if the first task of pf needs assignment.                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KhePartFromNeedAsst(KHE_PART_FROM pf)
{
  HnAssert(KheTaskSetTaskCount(pf->ts) > 0,
    "KhePartFromNeedAsst internal error");
  return KheTaskNeedsAssignment(KheTaskSetTask(pf->ts, 0));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "to parts" (private)                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePartToInit(KHE_PART_TO pt, KHE_TASK_SET ts)                      */
/*                                                                           */
/*  Initialize pt to empty, using ts.                                        */
/*                                                                           */
/*****************************************************************************/

static void KhePartToInit(KHE_PART_TO pt, KHE_TASK_SET ts)
{
  pt->ts = ts;
  KheTaskSetClear(ts);
  pt->durn = 0;
  pt->effectively_free = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToInit(KHE_PART_TO pt, KHE_TASK_SET ts)                      */
/*                                                                           */
/*  Initialize pt to empty, using ts.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMPartToInit(KHE_MPART_TO pt, KHE_MTASK_SET mts)
{
  pt->mts = mts;
  KheMTaskSetClear(mts);
  pt->durn = 0;
  pt->effectively_free = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToClear(KHE_PART_TO pt)                                      */
/*                                                                           */
/*  Clear pt back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KhePartToClear(KHE_PART_TO pt)
{
  KheTaskSetClear(pt->ts);
  pt->durn = 0;
  pt->effectively_free = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToClear(KHE_PART_TO pt)                                      */
/*                                                                           */
/*  Clear pt back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheMPartToClear(KHE_MPART_TO pt)
{
  KheMTaskSetClear(pt->mts);
  pt->durn = 0;
  pt->effectively_free = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToAddTask(KHE_PART_TO pt, KHE_TASK task,                     */
/*    int task_durn, bool task_effectively_free)                             */
/*                                                                           */
/*  Add task, with the given duration and effectively_free value, to pt.     */
/*                                                                           */
/*****************************************************************************/

static void KhePartToAddTask(KHE_PART_TO pt, KHE_TASK task,
  int task_durn, bool task_effectively_free)
{
  KheTaskSetAddTask(pt->ts, task);
  pt->durn += task_durn;
  if( !task_effectively_free )
    pt->effectively_free = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToAddTask(KHE_PART_TO pt, KHE_TASK task,                     */
/*    int task_durn, bool task_effectively_free)                             */
/*                                                                           */
/*  Add task, with the given duration and effectively_free value, to pt.     */
/*                                                                           */
/*****************************************************************************/

static void KheMPartToAddMTask(KHE_MPART_TO pt, KHE_MTASK mt,
  int mt_durn, bool mt_effectively_free)
{
  KheMTaskSetAddMTask(pt->mts, mt);
  pt->durn += mt_durn;
  if( !mt_effectively_free )
    pt->effectively_free = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToUnion(KHE_PART_TO target_pt, KHE_PART_TO source_pt)        */
/*                                                                           */
/*  Add source_pt to target_pt.                                              */
/*                                                                           */
/*****************************************************************************/

static void KhePartToUnion(KHE_PART_TO target_pt, KHE_PART_TO source_pt)
{
  int i;  KHE_TASK task;
  for( i = 0;  i < KheTaskSetTaskCount(source_pt->ts);  i++ )
  {
    task = KheTaskSetTask(source_pt->ts, i);
    KheTaskSetAddTask(target_pt->ts, task);
  }
  target_pt->durn += source_pt->durn;
  if( source_pt->effectively_free == false )
    target_pt->effectively_free = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartToUnion(KHE_PART_TO target_pt, KHE_PART_TO source_pt)        */
/*                                                                           */
/*  Add source_pt to target_pt.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheMPartToUnion(KHE_MPART_TO target_pt, KHE_MPART_TO source_pt)
{
  int i;  KHE_MTASK mt;
  for( i = 0;  i < KheMTaskSetMTaskCount(source_pt->mts);  i++ )
  {
    mt = KheMTaskSetMTask(source_pt->mts, i);
    KheMTaskSetAddMTask(target_pt->mts, mt);
  }
  target_pt->durn += source_pt->durn;
  if( source_pt->effectively_free == false )
    target_pt->effectively_free = false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindTo(KHE_TASK_FINDER tf, KHE_INTERVAL in, bool subset,         */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool require_optional,             */
/*    KHE_TASK_SET omit_ts, KHE_PART_TO pt)                                  */
/*                                                                           */
/*  Find all tasks from rtm running within in, and set pt to them; although  */
/*  if omit_ts != NULL, ignore any tasks already present in omit_ts.  Return */
/*  false if any of them are preassigned, or (when subset is true) running   */
/*  outside in, or (when require_optional is true) not optional.             */
/*                                                                           */
/*  If rtm == NULL, pt is cleared and true is returned.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheFindTo(KHE_TASK_FINDER tf, KHE_INTERVAL in, bool subset,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool require_optional,
  KHE_TASK_SET omit_ts, KHE_PART_TO pt)
{
  int day_index, i, j, count, pos;  KHE_TIME_GROUP tg;  KHE_RESOURCE r;
  KHE_TIME t;  KHE_TASK task;  KHE_INTERVAL task_in;  bool optional;
  KhePartToClear(pt);
  if( rtm != NULL )
    for( day_index = in.first;  day_index <= in.last;  day_index++ )
    {
      tg = KheFrameTimeGroup(tf->days_frame, day_index);
      for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
      {
	t = KheTimeGroupTime(tg, i);
	count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
	for( j = 0;  j < count;  j++ )
	{
	  task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	  task = KheTaskProperRoot(task);
	  if( !KheTaskSetContainsTask(pt->ts, task, &pos) &&
	    (omit_ts == NULL || !KheTaskSetContainsTask(omit_ts, task, &pos)) )
	  {
	    if( KheTaskIsPreassigned(task, &r) )
	      return false;
	    task_in = KheTaskInterval(tf, task);
	    if( subset && !KheIntervalSubset(task_in, in) )
	      return false;
	    optional = !KheTaskNeedsAssignment(task);
	    if( require_optional && !optional )
	      return false;
	    KhePartToAddTask(pt, task, KheIntervalLength(task_in), optional);
	  }
	}
      }
    }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindTo(KHE_TASK_FINDER tf, KHE_INTERVAL in, bool subset,         */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool require_optional,             */
/*    KHE_TASK_SET omit_ts, KHE_PART_TO pt)                                  */
/*                                                                           */
/*  Find all tasks from rtm running within in, and set pt to them; although  */
/*  if omit_ts != NULL, ignore any tasks already present in omit_ts.  Return */
/*  false if any of them are preassigned, or (when subset is true) running   */
/*  outside in, or (when require_optional is true) not optional.             */
/*                                                                           */
/*  If rtm == NULL, pt is cleared and true is returned.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFindTo(KHE_TASK_FINDER tf, KHE_INTERVAL in, bool subset,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool require_optional,
  KHE_MTASK_SET omit_mts, KHE_MPART_TO pt, KHE_MTASK_FINDER mtf)
{
  int day_index, i, j, count, pos;  KHE_TIME_GROUP tg;  KHE_RESOURCE r;
  KHE_TIME t;  KHE_TASK task;  KHE_INTERVAL mt_in;  bool optional;
  KHE_MTASK mt;
  KheMPartToClear(pt);
  if( rtm != NULL )
    for( day_index = in.first;  day_index <= in.last;  day_index++ )
    {
      tg = KheFrameTimeGroup(tf->days_frame, day_index);
      for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
      {
	t = KheTimeGroupTime(tg, i);
	count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
	for( j = 0;  j < count;  j++ )
	{
	  task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	  /* task = KheTaskProperRoot(task); */
	  mt = KheMTaskFinderTaskToMTask(mtf, task);
	  if( !KheMTaskSetContainsMTask(pt->mts, mt, &pos) &&
	    (omit_mts == NULL || !KheMTaskSetContainsMTask(omit_mts,mt,&pos)) )
	  {
	    if( KheMTaskIsPreassigned(mt, &r) )
	      return false;
	    mt_in = KheMTaskInterval(tf, mt);
	    if( subset && !KheIntervalSubset(mt_in, in) )
	      return false;
	    optional = !KheTaskNeedsAssignment(task);
	    if( require_optional && !optional )
	      return false;
	    KheMPartToAddMTask(pt, mt, KheIntervalLength(mt_in), optional);
	  }
	}
      }
    }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parts" (private)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePartInit(KHE_PART p, KHE_TASK_SET from_ts, KHE_TASK_SET to_ts)   */
/*                                                                           */
/*  Initialize p to empty, using from_ts and to_ts.                          */
/*                                                                           */
/*****************************************************************************/

static void KhePartInit(KHE_PART p, KHE_TASK_SET from_ts, KHE_TASK_SET to_ts)
{
  KhePartFromInit(&p->from, from_ts);
  KhePartToInit(&p->to, to_ts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartInit(KHE_PART p, KHE_TASK_SET from_ts, KHE_TASK_SET to_ts)   */
/*                                                                           */
/*  Initialize p to empty, using from_ts and to_ts.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMPartInit(KHE_MPART p, KHE_MTASK_SET from_mts,
  KHE_MTASK_SET to_mts)
{
  KheMPartFromInit(&p->from, from_mts);
  KheMPartToInit(&p->to, to_mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartClear(KHE_PART p)                                            */
/*                                                                           */
/*  Clear p back to empty.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KhePartClear(KHE_PART p)
{
  KhePartFromClear(&p->from);
  KhePartToClear(&p->to);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static void KheMPartClear(KHE_MPART p)
{
  KheMPartFromClear(&p->from);
  KheMPartToClear(&p->to);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART KhePartMake(KHE_WIDENED_TASK_SET wts)                           */
/*                                                                           */
/*  Make a new, empty part.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_PART KhePartMake(KHE_WIDENED_TASK_SET wts)
{
  KHE_PART res;  KHE_TASK_FINDER tf;

  /* get a part object from wts's free list or arena */
  tf = wts->tf;
  if( HaArrayCount(tf->free_parts) > 0 )
  {
    res = HaArrayLastAndDelete(tf->free_parts);
    KhePartClear(res);
  }
  else
  {
    HaMake(res, tf->arena);
    KhePartInit(res, KheTaskSetMake(tf->soln), KheTaskSetMake(tf->soln));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PART KhePartMake(KHE_WIDENED_TASK_SET wts)                           */
/*                                                                           */
/*  Make a new, empty part.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_MPART KheMPartMake(KHE_WIDENED_MTASK_SET wmts,
  KHE_MTASK_FINDER mtf)
{
  KHE_MPART res;  KHE_TASK_FINDER tf;

  /* get a part object from wts's free list or arena */
  tf = wmts->tf;
  if( HaArrayCount(tf->free_mparts) > 0 )
  {
    res = HaArrayLastAndDelete(tf->free_mparts);
    KheMPartClear(res);
  }
  else
  {
    HaMake(res, tf->arena);
    KheMPartInit(res, KheMTaskSetMake(mtf),
      KheMTaskSetMake(mtf));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMoveCheck(KHE_PART p, bool force, KHE_RESOURCE to_r)         */
/*                                                                           */
/*  Return true if p can move to to_r, assuming it has been prepared.        */
/*                                                                           */
/*****************************************************************************/

static bool KhePartMoveCheck(KHE_PART p, bool force, KHE_RESOURCE to_r)
{
  return (force || p->to.effectively_free) &&
    KheTaskSetUnAssignResourceCheck(p->to.ts) &&
    KheTaskSetMoveResourceCheck(p->from.ts, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMoveCheck(KHE_PART p, bool force, KHE_RESOURCE to_r)         */
/*                                                                           */
/*  Return true if p can move from from_r to to_r, assuming it has been      */
/*  prepared.                                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartMoveCheck(KHE_MPART p, bool force, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r)
{
  /* *** not sure what this is trying to do, working it out is still to do */
  return (force || p->to.effectively_free) &&
    KheMTaskSetUnAssignResourceCheck(p->to.mts, from_r /* ? */) &&
    KheMTaskSetMoveResourceCheck(p->from.mts, from_r, to_r);
    /* ***
    KheMTaskSetUnAssignResourceCheck(p->to.mts) &&
    KheMTaskSetMoveResourceCheck(p->from.mts, to_r);
    *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMove(KHE_PART p, KHE_RESOURCE to_r,                          */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Move p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KhePartMove(KHE_PART p, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  if( KheTaskSetUnAssignResource(p->to.ts) &&
      KheTaskSetMoveResource(p->from.ts, to_r) )
  {
    len = KheIntervalLength(p->from.in);
    *from_r_durn_change -= len;
    *to_r_durn_change += (len - p->to.durn);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMove(KHE_PART p, KHE_RESOURCE to_r,                          */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Move p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartMove(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  /* *** not sure what this is trying to do, working it out is still to do */
  if( KheMTaskSetUnAssignResource(p->to.mts, from_r /* ? */) &&
      KheMTaskSetMoveResource(p->from.mts, from_r, to_r) )
  {
    len = KheIntervalLength(p->from.in);
    *from_r_durn_change -= len;
    *to_r_durn_change += (len - p->to.durn);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMovePart(KHE_PART p, int first_index, int last_index,        */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Move p[first_index .. last_index] to to_r.                               */
/*                                                                           */
/*****************************************************************************/

static bool KhePartMovePart(KHE_PART p, int first_index, int last_index,
  KHE_RESOURCE to_r)
{
  return KheTaskSetUnAssignResource(p->to.ts) &&
    KheTaskSetPartMoveResource(p->from.ts, first_index, last_index, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartMovePart(KHE_PART p, int first_index, int last_index,        */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Move p[first_index .. last_index] to to_r.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartMovePart(KHE_MPART p, int first_index, int last_index,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  /* *** not sure what this is trying to do, working it out is still to do */
  return KheMTaskSetUnAssignResource(p->to.mts, from_r /* ? */) &&
      KheMTaskSetPartMoveResource(p->from.mts, first_index, last_index,
	from_r, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetFirstTasksEquivalent(KHE_TASK_SET ts1, KHE_TASK_SET ts2)  */
/*                                                                           */
/*  Return true if ts1 and ts2 are non-empty and their first tasks are       */
/*  equivalent.                                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetFirstTasksEquivalent(KHE_TASK_SET ts1, KHE_TASK_SET ts2)
{
  KHE_TASK task1, task2;
  if( KheTaskSetTaskCount(ts1) == 0 || KheTaskSetTaskCount(ts2) == 0 )
    return false;
  task1 = KheTaskSetTask(ts1, 0);
  task2 = KheTaskSetTask(ts2, 0);
  return KheTaskEquivalent(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBlockedByTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)         */
/*                                                                           */
/*  Return true if moving task is blocked because it has a time from tg.     */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskBlockedByTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TIME t, t2;  int i, durn, pos;  KHE_TASK child_task;

  /* do the job for task itself */
  meet = KheTaskMeet(task);
  t = KheMeetAsstTime(meet);
  durn = KheMeetDuration(meet);
  for( i = 0;  i < durn;  i++ )
  {
    t2 = KheTimeNeighbour(t, i);
    if( KheTimeGroupContains(tg, t2, &pos) )
      return true;
  }

  /* do the job for task's children */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskBlockedByTimeGroup(child_task, tg) )
      return true;
  }

  /* no blockage anywhere */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)    */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks have       */
/*  times from time group tg.  Here tg may be NULL, in which case false      */
/*  is returned.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)
{
  int i;  KHE_TASK task;
  if( tg != NULL )
  {
    for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( KheTaskBlockedByTimeGroup(task, tg) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskBlockedByTimeGroup(KHE_MTASK mt, KHE_TIME_GROUP tg)
{
  /* still to do */
  HnAbort("KheMTaskBlockedByTimeGroup still to do");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)    */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks have       */
/*  times from time group tg.  Here tg may be NULL, in which case false      */
/*  is returned.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskSetBlockedByTimeGroup(KHE_MTASK_SET ms, KHE_TIME_GROUP tg)
{
  int i;  KHE_MTASK mt;
  if( tg != NULL )
  {
    for( i = 0;  i < KheMTaskSetMTaskCount(ms);  i++ )
    {
      mt = KheMTaskSetMTask(ms, i);
      if( KheMTaskBlockedByTimeGroup(mt, tg) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBlockedByMonitor(KHE_TASK task, KHE_MONITOR m)               */
/*                                                                           */
/*  Return true if m monitors task.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskBlockedByMonitor(KHE_TASK task, KHE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  int i;  KHE_SOLN soln;  KHE_TASK child_task;

  /* do the job for task itself */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
      if( KheSolnEventResourceMonitor(soln, er, i) == m )
	return true;

  /* do the job for the children of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskBlockedByMonitor(child_task, m) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByMonitor(KHE_TASK_SET ts, KHE_MONITOR m)          */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks are        */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetBlockedByMonitor(KHE_TASK_SET ts, KHE_MONITOR m)
{
  int i;  KHE_TASK task;
  if( m != NULL )
  {
    for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( KheTaskBlockedByMonitor(task, m) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskBlockedByMonitor(KHE_MTASK mt, KHE_MONITOR m)
{
  /* still to do */
  HnAbort("KheMTaskBlockedByMonitor still to do");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByMonitor(KHE_TASK_SET ts, KHE_MONITOR m)          */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks are        */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskSetBlockedByMonitor(KHE_MTASK_SET ms, KHE_MONITOR m)
{
  int i;  KHE_MTASK mt;
  if( m != NULL )
  {
    for( i = 0;  i < KheMTaskSetMTaskCount(ms);  i++ )
    {
      mt = KheMTaskSetMTask(ms, i);
      if( KheMTaskBlockedByMonitor(mt, m) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartCoreSwapCheck(KHE_PART p, KHE_RESOURCE from_r,               */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m)                                                */
/*                                                                           */
/*  Return true if core part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

static bool KhePartCoreSwapCheck(KHE_PART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m)
{
  /* check equivalent tasks, blocking_tg, and blocking_m */
  if( KheTaskSetFirstTasksEquivalent(p->from.ts, p->to.ts) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (equivalent tasks)\n");
    return false;
  }

  if( KheTaskSetBlockedByTimeGroup(p->to.ts, blocking_tg) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (time group)\n");
    return false;
  }

  if( KheTaskSetBlockedByMonitor(p->to.ts, blocking_m) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (monitor)\n");
    return false;
  }

  /* check exact */
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (exact)\n");
    return false;
  }

  /* check move from from_r to to_r */
  if( !KheTaskSetMoveResourceCheck(p->from.ts, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (from_r to to_r)\n");
    return false;
  }

  /* check move from to_r to from_r */
  if( !KheTaskSetMoveResourceCheck(p->to.ts, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheCoreSwapCheck returning false (to_r to from_r)\n");
    return false;
  }

  /* all good */
  if( DEBUG1 )
    fprintf(stderr, "KheCoreSwapCheck returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartCoreSwapCheck(KHE_PART p, KHE_RESOURCE from_r,               */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m)                                                */
/*                                                                           */
/*  Return true if core part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartCoreSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m)
{
  /* check equivalent tasks, blocking_tg, and blocking_m */
  /* *** no longer needed? checking this is still to do
  if( KheTaskSetFirstTasksEquivalent(p->from.ts, p->to.ts) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (equiv tasks)\n");
    return false;
  }
  *** */

  if( KheMTaskSetBlockedByTimeGroup(p->to.mts, blocking_tg) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (time group)\n");
    return false;
  }

  if( KheMTaskSetBlockedByMonitor(p->to.mts, blocking_m) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (monitor)\n");
    return false;
  }

  /* check exact */
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (exact)\n");
    return false;
  }

  /* check move from from_r to to_r */
  if( !KheMTaskSetMoveResourceCheck(p->from.mts, from_r, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck ret false (from_r to to_r)\n");
    return false;
  }

  /* check move from to_r to from_r */
  if( !KheMTaskSetMoveResourceCheck(p->to.mts, to_r, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck ret false (to_r to from_r)\n");
    return false;
  }

  /* all good */
  if( DEBUG1 )
    fprintf(stderr, "KheMPartCoreSwapCheck returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartWingSwapCheck(KHE_PART p, KHE_RESOURCE from_r,               */
/*    KHE_RESOURCE to_r, bool exact)                                         */
/*                                                                           */
/*  Return true if wing part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

static bool KhePartWingSwapCheck(KHE_PART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact)
{
  /* check exact */
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheWingSwapCheck returning false (exact)\n");
    return false;
  }

  /* check move from from_r to to_r */
  if( !KheTaskSetMoveResourceCheck(p->from.ts, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheWingSwapCheck returning false (from_r to to_r)\n");
    return false;
  }

  /* check move from to_r to from_r */
  if( !KheTaskSetMoveResourceCheck(p->to.ts, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheWingSwapCheck returning false (to_r to from_r)\n");
    return false;
  }

  /* all good */
  if( DEBUG1 )
    fprintf(stderr, "KheWingSwapCheck returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartWingSwapCheck(KHE_PART p, KHE_RESOURCE from_r,               */
/*    KHE_RESOURCE to_r, bool exact)                                         */
/*                                                                           */
/*  Return true if wing part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartWingSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact)
{
  /* check exact */
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (exact)\n");
    return false;
  }

  /* check move from from_r to to_r */
  if( !KheMTaskSetMoveResourceCheck(p->from.mts, from_r, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (from_r to to_r)\n");
    return false;
  }

  /* check move from to_r to from_r */
  if( !KheMTaskSetMoveResourceCheck(p->to.mts, to_r, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (to_r to from_r)\n");
    return false;
  }

  /* all good */
  if( DEBUG1 )
    fprintf(stderr, "KheMPartWingSwapCheck returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartSwap(KHE_PART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,     */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Swap p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KhePartSwap(KHE_PART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  if( KheTaskSetMoveResource(p->from.ts, to_r) &&
      KheTaskSetMoveResource(p->to.ts, from_r) )
  {
    len = KheIntervalLength(p->from.in) - p->to.durn;
    *from_r_durn_change -= len;
    *to_r_durn_change += len;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartSwap(KHE_PART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,     */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Swap p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheMPartSwap(KHE_MPART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  if( KheMTaskSetMoveResource(p->from.mts, from_r, to_r) &&
      KheMTaskSetMoveResource(p->to.mts, to_r, from_r) )
  {
    len = KheIntervalLength(p->from.in) - p->to.durn;
    *from_r_durn_change -= len;
    *to_r_durn_change += len;
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartCoreSwap(KHE_PART p, KHE_RESOURCE from_r,                    */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m, int *from_r_durn_change, int *to_r_durn_change)*/
/*                                                                           */
/*  A combination of KhePartCoreSwapCheck and KhePartSwap.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KhePartCoreSwap(KHE_PART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, int *from_r_durn_change, int *to_r_durn_change)
{
  int len;

  ** check equivalent tasks, blocking_tg, and blocking_m **
  if( KheTaskSetFirstTasksEquivalent(p->from.ts, p->to.ts) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (equivalent tasks)\n");
    return false;
  }

  if( KheTaskSetBlockedByTimeGroup(p->to.ts, blocking_tg) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (time group)\n");
    return false;
  }

  if( KheTaskSetBlockedByMonitor(p->to.ts, blocking_m) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (monitor)\n");
    return false;
  }

  ** check exact **
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (exact)\n");
    return false;
  }

  ** check and perform move from from_r to to_r **
  if( !KheTaskSetMoveResource(p->from.ts, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (from_r to to_r)\n");
    return false;
  }

  ** check and perform move from to_r to from_r **
  if( !KheTaskSetMoveResource(p->to.ts, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KhePartCoreSwap returning false (to_r to from_r)\n");
    return false;
  }

  ** all good **
  len = KheIntervalLength(p->from.in) - p->to.durn;
  *from_r_durn_change -= len;
  *to_r_durn_change += len;
  if( DEBUG1 )
    fprintf(stderr, "KhePartCoreSwap returning true\n");
  return true;
}
*** */


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
  KhePartFromInit(&res->scratch_from, KheTaskSetMake(soln));
  KhePartToInit(&res->scratch_to, KheTaskSetMake(soln));
  HaArrayInit(res->free_widened_task_sets, a);
  HaArrayInit(res->free_parts, a);
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
/*  void KheTaskFinderTaskInterval(KHE_TASK_FINDER tf,                       */
/*    KHE_TASK task, int *first_index, int *last_index)                      */
/*                                                                           */
/*  Return the interval covered by task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskFinderTaskInterval(KHE_TASK_FINDER tf,
  KHE_TASK task, int *first_index, int *last_index)
{
  KHE_INTERVAL res;
  res = KheTaskInterval(tf, task);
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTaskSetInterval(KHE_TASK_FINDER tf,                    */
/*    KHE_TASK_SET ts, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Return the interval covered by ts.                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskFinderTaskSetInterval(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts, int *first_index, int *last_index)
{
  KHE_INTERVAL res;
  res = KheTaskSetInterval(tf, ts);
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheTaskFinderMTaskSetInterval(KHE_TASK_FINDER tf,
  KHE_MTASK_SET mts, int *first_index, int *last_index)
{
  KHE_INTERVAL res;
  res = KheMTaskSetInterval(tf, mts);
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,                  */
/*    KHE_TIME_GROUP tg, int *first_index, int *last_index)                  */
/*                                                                           */
/*  Return the interval covered by tg.                                       */
/*                                                                           */
/*****************************************************************************/

void KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index)
{
  KHE_TIME t;  int count;
  count = KheTimeGroupTimeCount(tg);
  if( count == 0 )
    *first_index = 1, *last_index = 0;
  else
  {
    t = KheTimeGroupTime(tg, 0);
    *first_index = KheFrameTimeIndex(tf->days_frame, t);
    t = KheTimeGroupTime(tg, count - 1);
    *last_index = KheFrameTimeIndex(tf->days_frame, t);
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
  KHE_TASK_FINDER tf;  KHE_TASK_AND_TIME tt;
  if( HaArrayCount(ds->tasks_and_times) == 0 )
    ds->first_day_index = 1, ds->last_day_index = 0;
  else
  {
    tf = ds->task_finder;
    tt = HaArrayFirst(ds->tasks_and_times);
    ds->first_day_index = KheFrameTimeIndex(tf->days_frame, tt->time);
    tt = HaArrayLast(ds->tasks_and_times);
    ds->last_day_index = KheFrameTimeIndex(tf->days_frame, tt->time);
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
  index = ds->first_day_index;
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
  HnAssert(index == ds->last_day_index + 1,
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
/*  KHE_DAILY_SCHEDULE KheTaskFinderNullDailySchedule(                       */
/*    KHE_TASK_FINDER tf, int first_day_index, int last_day_index)           */
/*                                                                           */
/*  Return a null daily schedule for first_day_index .. last_day_index.      */
/*                                                                           */
/*****************************************************************************/

KHE_DAILY_SCHEDULE KheTaskFinderNullDailySchedule(
  KHE_TASK_FINDER tf, int first_day_index, int last_day_index)
{
  KHE_DAILY_SCHEDULE res;  int index;
  res = KheDailyScheduleGet(tf);
  res->no_overlap = true;
  res->first_day_index = first_day_index;
  res->last_day_index = last_day_index;
  for( index = first_day_index;  index <= last_day_index;  index++ )
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

int KheDailyScheduleFirstDayIndex(KHE_DAILY_SCHEDULE ds)
{
  return ds->first_day_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDailyScheduleLastDayIndex(KHE_DAILY_SCHEDULE ds)                  */
/*                                                                           */
/*  Return the last_day_index attribute of ds.                               */
/*                                                                           */
/*****************************************************************************/

int KheDailyScheduleLastDayIndex(KHE_DAILY_SCHEDULE ds)
{
  return ds->last_day_index;
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
  HnAssert(ds->first_day_index <= day_index && day_index <= ds->last_day_index,
    "KheDailyScheduleTask: day_index (%d) out of range (%d .. %d)",
    day_index, ds->first_day_index, ds->last_day_index);
  tt = HaArray(ds->tasks_and_times, day_index - ds->first_day_index);
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
  HnAssert(ds->first_day_index <= day_index && day_index <= ds->last_day_index,
    "KheDailyScheduleTime: day_index (%d) out of range (%d .. %d)",
    day_index, ds->first_day_index, ds->last_day_index);
  tt = HaArray(ds->tasks_and_times, day_index - ds->first_day_index);
  return tt->time;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "obsolete"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSetFirstAndLastDayIndex(KHE_TASK_FINDER tf,                      */
/*    ARRAY_KHE_TIME *times_by_day, int *first_index, int *last_index)       */
/*                                                                           */
/*  Set *first_index and *last_index to the indexes of the first and last    */
/*  days covered by *times_of_day (assumed to be sorted into chronological   */
/*  order), or to 1 and 0 if *times_of_day is empty.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheSetFirstAndLastDayIndex(KHE_TASK_FINDER tf,
  ARRAY_KHE_TIME *times_by_day, int *first_index, int *last_index)
{
  KHE_TIME t;
  if( HaArrayCount(*times_by_day) == 0 )
    *first_index = 1, *last_index = 0;
  else
  {
    t = HaArrayFirst(*times_by_day);
    *first_index = KheFrameTimeIndex(tf->days_frame, t);
    t = HaArrayLast(*times_by_day);
    *last_index = KheFrameTimeIndex(tf->days_frame, t);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheCorrelateTimesWithDays(KHE_TASK_FINDER tf,                       */
/*    int first_index, int last_index, ARRAY_KHE_TIME *times_by_day)         */
/*                                                                           */
/*  Correlate the times of *times_by_day with the days of tf, from           */
/*  first_index to last_index.  Return true if no times were deleted.        */
/*                                                                           */
/*  Implementation note.  Nearly always the correlation will be correct      */
/*  initially and just needs to be checked.  So we don't worry about         */
/*  the cost of shifting array segments up and down.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheCorrelateTimesWithDays(KHE_TASK_FINDER tf,
  int first_index, int last_index, ARRAY_KHE_TIME *times_by_day)
{
  int index, i, t_index;  KHE_TIME t;  bool res;
  res = true;
  index = first_index;
  HaArrayForEach(*times_by_day, t, i)
  {
    ***************************************************************************
    **                                                                       **
    **  Loop invariant:  times_by_day[0 .. i-1] has its desired final value  **
    **  (one time or NULL for each day) for those days, and index is the     **
    **  index of the next (the ith) day                                      **
    **                                                                       **
    ***************************************************************************

    t_index = KheFrameTimeIndex(tf->days_frame, t);
    if( t_index == index )
    {
      ** t belongs on the current day, so nothing to do but move on **
      index++;
    }
    else if( t_index > index )
    {
      ** t belongs on a later day, so insert a NULL **
      HaArrayAdd(*times_by_day, i, NULL);
      index++;
    }
    else ** t_index < index **
    {
      ** t belongs on an earlier day, but too late for that **
      HaArrayDeleteAndShift(*times_by_day, i);
      i--;
      res = false;
    }
  }
  HnAssert(index == last_index + 1, "KheCorrelateTimesWithDays internal error");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,               */
/*    KHE_TASK task, int *first_index, int *last_index,                      */
/*    ARRAY_KHE_TIME *times_by_day)                                          */
/*                                                                           */
/*  Find the days that task is running, and its times by day.                */
/*                                                                           */
/*  Return true if all times are included in *times_by_day, because they     */
/*  are all on different days.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK task, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day, ARRAY_KHE_TASK *tasks_by_day)
{
  ** find the times in chronological order; set *first_index and *last_index **
  HaArrayClear(*times_by_day);
  KheTaskAddTimes(task, times_by_day);
  HaArraySort(*times_by_day, &KheTimeCmp);

  ** set *first_index and *last_index, and correlate with days **
  KheSetFirstAndLastDayIndex(tf, times_by_day, first_index, last_index);
  return KheCorrelateTimesWithDays(tf, *first_index, *last_index, times_by_day);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,            */
/*    KHE_TASK_SET ts, int *first_index, int *last_index,                    */
/*    ARRAY_KHE_TIME *times_by_day)                                          */
/*                                                                           */
/*  Find the days that ts is running, and its times by day.                  */
/*                                                                           */
/*  Return true if all times are included in *times_by_day, because they     */
/*  are all on different days.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day)
{
  int i;  KHE_TASK task;

  ** find the times in chronological order **
  HaArrayClear(*times_by_day);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    KheTaskAddTimes(task, times_by_day);
  }
  HaArraySort(*times_by_day, &KheTimeCmp);

  ** set *first_index and *last_index, and correlate with days **
  KheSetFirstAndLastDayIndex(tf, times_by_day, first_index, last_index);
  return KheCorrelateTimesWithDays(tf, *first_index, *last_index, times_by_day);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,          */
/*    KHE_TIME_GROUP tg, int *first_index, int *last_index,                  */
/*    ARRAY_KHE_TIME *times_by_day)                                          */
/*                                                                           */
/*  Find the days that tg is running, and its times by day.                  */
/*                                                                           */
/*  Return true if all times are included in *times_by_day, because they     */
/*  are all on different days.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index,
  ARRAY_KHE_TIME *times_by_day)
{
  int i;  KHE_TIME t;

  ** find the times in chronological order **
  HaArrayClear(*times_by_day);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    HaArrayAddLast(*times_by_day, t);
  }

  ** set *first_index and *last_index, and correlate with days **
  KheSetFirstAndLastDayIndex(tf, times_by_day, first_index, last_index);
  return KheCorrelateTimesWithDays(tf, *first_index, *last_index, times_by_day);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,               */
/*    KHE_TASK task, int *first_index, int *last_index,                      */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Find the first and last day indexes, and the first and last times,       */
/*  that task is running.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** old interface
void KheTaskFinderTaskIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK task, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KheTaskFirstAndLastTimes(task, first_time, last_time);
  if( *first_time == NULL )
    *first_index = 1, *last_index = 0;
  else
  {
    *first_index = KheFrameTimeIndex(tf->days_frame, *first_time);
    *last_index = KheFrameTimeIndex(tf->days_frame, *last_time);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,            */
/*    KHE_TASK_SET ts, int *first_index, int *last_index,                    */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Find the first and last day indexes, and the first and last times,       */
/*  that ts is running.                                                      */
/*                                                                           */
/*****************************************************************************/

/* *** old interface
void KheTaskFinderTaskSetIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TASK_SET ts, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KheTaskSetFirstAndLastTimes(ts, first_time, last_time);
  if( *first_time == NULL )
    *first_index = 1, *last_index = 0;
  else
  {
    *first_index = KheFrameTimeIndex(tf->days_frame, *first_time);
    *last_index = KheFrameTimeIndex(tf->days_frame, *last_time);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,          */
/*    KHE_TIME_GROUP tg, int *first_index, int *last_index,                  */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the interval and times covered by tg.                             */
/*                                                                           */
/*****************************************************************************/

/* *** old interface
void KheTaskFinderTimeGroupIntervalAndTimes(KHE_TASK_FINDER tf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int count;
  count = KheTimeGroupTimeCount(tg);
  if( count == 0 )
    *first_index = 1, *last_index = 0, *first_time = NULL, *last_time = NULL;
  else
  {
    *first_time = KheTimeGroupTime(tg, 0);
    *first_index = KheFrameTimeIndex(tf->days_frame, *first_time);
    *last_time = KheTimeGroupTime(tg, count - 1);
    *last_index = KheFrameTimeIndex(tf->days_frame, *last_time);
  }
}
*** */


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
/*  void KheFindTasksInInterval(KHE_TASK_FINDER tf, int first_index,         */
/*    int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,             */
/*    bool ignore_preassigned, bool ignore_partial,                          */
/*    KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)        */
/*                                                                           */
/*  Find tasks in interval first_index .. last_index.                        */
/*                                                                           */
/*****************************************************************************/

void KheFindTasksInInterval(KHE_TASK_FINDER tf, int first_index,
  int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,
  bool ignore_preassigned, bool ignore_partial,
  KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int tg_start, i;  KHE_TASK task;
  KHE_INTERVAL task_in, disjoint_in, subset_in;  KHE_RESOURCE_GROUP rg;
  disjoint_in = KheInterval(1, 0);
  subset_in = ignore_partial ? KheInterval(first_index, last_index) :
    KheInterval(0, KheTaskFinderLastIndex(tf));
  rtm = KheTimetableMonitor(tf->soln, from_r);
  rg = NULL;
  tg_start = 0;
  KheTaskSetClear(res_ts);
  for( i = first_index;  i <= last_index;  i++ )
  {
    if( KheGetTask(tf, i, rtm, rt, from_r, &rg, &tg_start, ignore_preassigned,
	  NEEDS_ASST_DONT_CARE, disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      KheIntervalUnion(&disjoint_in, task_in);
    }
  }
  *res_first_index = disjoint_in.first;
  *res_last_index = disjoint_in.last;
}

/* ***
bool KheFindTasksInInterval(KHE_TASK_FINDER tf, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE from_r, int first_index, int last_index, KHE_TASK_SET res_ts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  struct khe_part_to_rec pt_rec;
  KHE_INTERVAL in = KheInterval(first_index, last_index);
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KhePartToInit(&pt_rec, res_ts);
  HnAbort("KheFindTasksInInterval not re-implemented yet");
  return KheFindTo(tf, in, true, rtm, false, NULL, &pt_rec);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheFindMTasksInInterval(KHE_TASK_FINDER tf, int first_index,
  int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,
  bool ignore_preassigned, bool ignore_partial,
  KHE_MTASK_SET res_mts, int *res_first_index, int *res_last_index)
{
  /* *** almost finished, I did it by mistake
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int tg_start, i;  KHE_TASK task;
  KHE_INTERVAL task_in, disjoint_in, subset_in;  KHE_RESOURCE_GROUP rg;
  disjoint_in = KheInterval(1, 0);
  subset_in = ignore_partial ? KheInterval(first_index, last_index) :
    KheInterval(0, KheTaskFinderLastIndex(tf));
  rtm = KheTimetableMonitor(tf->soln, from_r);
  rg = NULL;
  tg_start = 0;
  KheMTaskSetClear(res_mts);
  for( i = first_index;  i <= last_index;  i++ )
  {
    if( KheGetMTask(tf, i, rtm, rt, from_r, &rg, &tg_start, ignore_preassigned,
	  NEEDS_ASST_DONT_CARE, disjoint_in, subset_in, &mt, &task_in) )
    {
      KheMTaskSetAddMTask(res_mts, mt);
      KheIntervalUnion(&disjoint_in, task_in);
    }
  }
  *res_first_index = disjoint_in.first;
  *res_last_index = disjoint_in.last;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindRunAtIndex(KHE_TASK_FINDER tf, int index,                    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool ignore_preassigned, bool sep_need_asst,           */
/*    KHE_INTERVAL *disjoint_in, KHE_INTERVAL subset_in, KHE_TASK_SET res_ts)*/
/*                                                                           */
/*  If a suitable run overlaps index, set res_ts to it and return true.      */
/*                                                                           */
/*****************************************************************************/

static bool KheFindRunAtIndex(KHE_TASK_FINDER tf, int index,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool ignore_preassigned, bool sep_need_asst,
  KHE_INTERVAL *disjoint_in, KHE_INTERVAL subset_in, KHE_TASK_SET res_ts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_TASK task;  KHE_INTERVAL task_in;
  NEEDS_ASST_TYPE na;
  rg = NULL;
  tg_start = 0;
  if( KheGetTask(tf, index, rtm, rt, r, &rg, &tg_start, ignore_preassigned,
      NEEDS_ASST_DONT_CARE, *disjoint_in, subset_in, &task, &task_in) )
  {
    /* found a first task; save it, then find the rest of the run and stop */
    na = (!sep_need_asst ? NEEDS_ASST_DONT_CARE :
      KheTaskNeedsAssignment(task) ? NEEDS_ASST_TRUE : NEEDS_ASST_FALSE);
    KheTaskSetAddTask(res_ts, task);
    KheIntervalUnion(disjoint_in, task_in);
    while( KheGetTask(tf, disjoint_in->last + 1, rtm, rt, r, &rg, &tg_start,
	ignore_preassigned, na, *disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      KheIntervalUnion(disjoint_in, task_in);
    }
    while( KheGetTask(tf, disjoint_in->first - 1, rtm, rt, r, &rg, &tg_start,
	ignore_preassigned, na, *disjoint_in, subset_in, &task, &task_in) )
    {
      KheTaskSetAddTask(res_ts, task);
      KheIntervalUnion(disjoint_in, task_in);
    }
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindFirstRunInInterval(KHE_TASK_FINDER tf, int first_index,      */
/*    int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,             */
/*    bool ignore_preassigned, bool ignore_partial, bool sep_need_asst,      */
/*    KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)        */
/*                                                                           */
/*  Find the first (leftmost) run for from_r in first_index .. last_index.   */
/*                                                                           */
/*****************************************************************************/

bool KheFindFirstRunInInterval(KHE_TASK_FINDER tf, int first_index,
  int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,
  bool ignore_preassigned, bool ignore_partial, bool sep_need_asst,
  KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)
{
  int i;  KHE_INTERVAL disjoint_in, subset_in;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG6 )
    fprintf(stderr, "[ KheFindFirstRunInInterval(tf, %d..%d, %s, %s, %s, %s)\n",
      first_index, last_index, KheResourceTypeName(rt), KheResourceShow(from_r),
      bool_show(ignore_preassigned), bool_show(ignore_partial));

  /* initialize everything */
  disjoint_in = KheInterval(1, 0);
  subset_in = ignore_partial ? KheInterval(first_index, last_index) :
    KheInterval(0, KheTaskFinderLastIndex(tf));
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KheTaskSetClear(res_ts);

  /* search right from first_index for the first run */
  for( i = first_index;  i <= last_index;  i++ )
  {
    if( KheFindRunAtIndex(tf, i, rtm, rt, from_r, ignore_preassigned,
	  sep_need_asst, &disjoint_in, subset_in, res_ts) )
      break;
  }

  /* return true if KheTaskSetTaskCount(res_ts) > 0 */
  *res_first_index = disjoint_in.first;
  *res_last_index = disjoint_in.last;
  if( DEBUG6 )
  {
    if( KheTaskSetTaskCount(res_ts) > 0 )
      fprintf(stderr, "] KheFindFirstRunInInterval returning true (%d..%d)\n",
	*res_first_index, *res_last_index);
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
  in = KheInterval(0, first_index - 1);
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
/*  bool KheFindLastRunInInterval(KHE_TASK_FINDER tf, int first_index,       */
/*    int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,             */
/*    bool ignore_preassigned, bool ignore_partial, bool sep_need_asst,      */
/*    KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)        */
/*                                                                           */
/*  Find the last (rightmost) run for from_r in first_index .. last_index.   */
/*                                                                           */
/*****************************************************************************/

bool KheFindLastRunInInterval(KHE_TASK_FINDER tf, int first_index,
  int last_index, KHE_RESOURCE_TYPE rt, KHE_RESOURCE from_r,
  bool ignore_preassigned, bool ignore_partial, bool sep_need_asst,
  KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)
{
  int i;  KHE_INTERVAL disjoint_in, subset_in;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG6 )
    fprintf(stderr, "[ KheFindLastRunInInterval(tf, %d..%d, %s, %s, %s, %s)\n",
      first_index, last_index, KheResourceTypeName(rt), KheResourceShow(from_r),
      bool_show(ignore_preassigned), bool_show(ignore_partial));

  /* initialize everything */
  disjoint_in = KheInterval(1, 0);
  subset_in = ignore_partial ? KheInterval(first_index, last_index) :
    KheInterval(0, KheTaskFinderLastIndex(tf));
  rtm = KheTimetableMonitor(tf->soln, from_r);
  KheTaskSetClear(res_ts);

  /* search left from last_index for the first run */
  for( i = last_index;  i >= first_index;  i-- )
  {
    if( KheFindRunAtIndex(tf, i, rtm, rt, from_r, ignore_preassigned,
	  sep_need_asst, &disjoint_in, subset_in, res_ts) )
      break;
  }

  /* return true if KheTaskSetTaskCount(res_ts) > 0 */
  *res_first_index = disjoint_in.first;
  *res_last_index = disjoint_in.last;
  if( DEBUG6 )
  {
    if( KheTaskSetTaskCount(res_ts) > 0 )
      fprintf(stderr, "] KheFindLastRunInInterval returning true (%d..%d)\n",
	*res_first_index, *res_last_index);
    else
      fprintf(stderr, "] KheFindLastRunInInterval returning false\n");
  }
  return KheTaskSetTaskCount(res_ts) > 0;
}

/* *** old version
bool KheFindTaskRunLeft(KHE_TASK_FINDER tf, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE from_r, int last_index,
  bool allow_partial, bool allow_preassigned,
  KHE_TASK_SET res_ts, int *res_first_index, int *res_last_index)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;
  KHE_INTERVAL in;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_PART_FROM pf;
  struct khe_part_from_rec res_pf;  ** bool need_asst; **
  if( DEBUG6 )
    fprintf(stderr, "[ KheFindTaskRunLeft(tf, %s, %s, %d, %s, %s, -)\n",
      KheResourceTypeName(rt), KheResourceShow(from_r), last_index,
      bool_show(allow_partial), bool_show(allow_preassigned));

  HnAbort("KheFindTaskRunLeft: revised version st ill to do");
  rtm = KheTimetableMonitor(tf->soln, from_r);
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  in = KheInterval(last_index + 1, tg_count - 1);
  rg = NULL, tg_start = 0;
  pf = &tf->scratch_from;
  for( i = last_index;  i >= 0;  i-- )
  {
    if( KheFindFrom(tf, i, &in, rtm, rt, NULL, true, &rg, &tg_start, pf) )
    {
      ** repeatedly add pf to the run and skip to after it **
      ** need_asst = KhePartFromNeedAsst(pf); **
      KhePartFromInit(&res_pf, res_ts);
      do
      {
	KhePartFromUnion(&res_pf, pf);
        i = in.first - 1;
      } while( KheFindFrom(tf, i, &in, rtm, rt, NULL, true,&rg,&tg_start,pf) );
       ** && (!separate_need_asst || KhePartFromNeedAsst(pf) == need_asst) ); **

      ** run ends here **
      *res_first_index = res_pf.in.first;
      *res_last_index = res_pf.in.last;
      if( DEBUG6 )
	fprintf(stderr, "] KheFindTaskRunLeft returning true (%d..%d)\n",
	  *res_first_index, *res_last_index);
      return true;
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheFindTaskRunLeft returning false\n");
  return *res_first_index = *res_last_index = -1, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindTaskRunInitial(KHE_TASK_FINDER tf, KHE_TASK_SET ts,          */
/*    int wanted_durn, KHE_TASK_SET res_ts)                                  */
/*                                                                           */
/*  Find an initial sequence of ts's tasks whose duration equals wanted_durn.*/
/*                                                                           */
/*****************************************************************************/

/* *** correct, probably, but currently undocumented and unused
bool KheFindTaskRunInitial(KHE_TASK_FINDER tf, KHE_TASK_SET ts,
  int wanted_durn, KHE_TASK_SET res_ts)
{
  int durn, i;  KHE_TASK task;
  durn = 0;
  KheTaskSetClear(res_ts);
  for( i = 0;  i < KheTaskSetTaskCount(ts) && durn < wanted_durn;  i++ )
  {
    task = KheTaskSetTask(ts, i);
    durn += KheIntervalLength(KheTaskInterval(tf, task));
    KheTaskSetAddTask(res_ts, task);
  }
  return durn == wanted_durn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindTaskRunFinal(KHE_TASK_FINDER tf, KHE_TASK_SET ts,            */
/*    int wanted_durn, KHE_TASK_SET res_ts)                                  */
/*                                                                           */
/*  Find a final sequence of ts's tasks whose duration equals wanted_durn.   */
/*                                                                           */
/*****************************************************************************/

/* *** correct, probably, but currently undocumented and unused
bool KheFindTaskRunFinal(KHE_TASK_FINDER tf, KHE_TASK_SET ts,
  int wanted_durn, KHE_TASK_SET res_ts)
{
  int durn, i;  KHE_TASK task;
  durn = 0;
  KheTaskSetClear(res_ts);
  for( i = KheTaskSetTaskCount(ts) - 1;  i >= 0 && durn < wanted_durn;  i-- )
  {
    task = KheTaskSetTask(ts, i);
    durn += KheIntervalLength(KheTaskInterval(tf, task));
    KheTaskSetAddTask(res_ts, task);
  }
  return durn == wanted_durn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened task sets"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftWing(KHE_WIDENED_TASK_SET wts,            */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_left_wing_count)                                               */
/*                                                                           */
/*  Build a left wing for wts containing up to max_left_wing_count parts.    */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetBuildLeftWing(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_left_wing_count)
{
  KHE_INTERVAL in;  int i, index;  KHE_PART p;  KHE_PART_FROM pf;
  KHE_TASK_FINDER tf;   KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wts->from_r_rtm;
  tf = wts->tf;
  in = wts->core.from.in;
  pf = &tf->scratch_from;
  for( i = 0;  i < max_left_wing_count;  i++ )
  {
    index = in.first - 1;
    if( !KheFindFrom(wts->tf, index, &in, rtm, rt, NULL, false, rg,tg_start,pf))
      break;
    p = KhePartMake(wts);
    KhePartFromUnion(&p->from, pf);
    HaArrayAddLast(wts->left_wing, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftWing(KHE_WIDENED_TASK_SET wts,            */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_left_wing_count)                                               */
/*                                                                           */
/*  Build a left wing for wts containing up to max_left_wing_count parts.    */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetBuildLeftWing(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_left_wing_count, KHE_MTASK_FINDER mtf)
{
  KHE_INTERVAL in;  int i, index;  KHE_MPART p;  KHE_MPART_FROM pf;
  KHE_TASK_FINDER tf;   KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wmts->from_r_rtm;
  tf = wmts->tf;
  in = wmts->core.from.in;
  pf = &tf->scratch_mfrom;
  for( i = 0;  i < max_left_wing_count;  i++ )
  {
    index = in.first - 1;
    if( !KheMTaskFindFrom(wmts->tf, index, &in, rtm, rt, NULL, false, rg,
	tg_start, pf, mtf) )
      break;
    p = KheMPartMake(wmts, mtf);
    KheMPartFromUnion(&p->from, pf);
    HaArrayAddLast(wmts->left_wing, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildRightWing(KHE_WIDENED_TASK_SET wts,           */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_right_wing_count)                                              */
/*                                                                           */
/*  Build a right wing for wts containing up to max_right_wing_count parts.  */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetBuildRightWing(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_right_wing_count)
{
  KHE_INTERVAL in;  int i, index;  KHE_PART p;  KHE_PART_FROM pf;
  KHE_TASK_FINDER tf;   KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wts->from_r_rtm;
  tf = wts->tf;
  in = wts->core.from.in;
  pf = &tf->scratch_from;
  for( i = 0;  i < max_right_wing_count;  i++ )
  {
    index = in.last + 1;
    if( !KheFindFrom(wts->tf, index, &in, rtm, rt, NULL, false,rg,tg_start,pf) )
      break;
    p = KhePartMake(wts);
    KhePartFromUnion(&p->from, pf);
    HaArrayAddLast(wts->right_wing, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildRightWing(KHE_WIDENED_TASK_SET wts,           */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_right_wing_count)                                              */
/*                                                                           */
/*  Build a right wing for wts containing up to max_right_wing_count parts.  */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetBuildRightWing(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_right_wing_count, KHE_MTASK_FINDER mtf)
{
  KHE_INTERVAL in;  int i, index;  KHE_MPART p;  KHE_MPART_FROM pf;
  KHE_TASK_FINDER tf;   KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wmts->from_r_rtm;
  tf = wmts->tf;
  in = wmts->core.from.in;
  pf = &tf->scratch_mfrom;
  for( i = 0;  i < max_right_wing_count;  i++ )
  {
    index = in.last + 1;
    if( !KheMTaskFindFrom(wmts->tf, index, &in, rtm, rt, NULL, false,
	  rg, tg_start, pf, mtf) )
      break;
    p = KheMPartMake(wmts, mtf);
    KheMPartFromUnion(&p->from, pf);
    HaArrayAddLast(wmts->right_wing, p);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftAndRightWings(KHE_WIDENED_TASK_SET wts,   */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc)  */
/*                                                                           */
/*  Build left and right wings for wts containing up to mwc parts.           */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetBuildLeftAndRightWings(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc)
{
  KHE_INTERVAL in;  KHE_PART_FROM pf;  bool lopen, ropen, try_left;
  KHE_PART p;  KHE_TASK_FINDER tf;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wts->from_r_rtm;
  tf = wts->tf;
  in = wts->core.from.in;
  pf = &tf->scratch_from;
  lopen = true;  ropen = true;
  while( HaArrayCount(wts->left_wing) + HaArrayCount(wts->right_wing) < mwc )
  {
    /* sort out which side to try, or break if neither side is open */
    if( lopen )
    {
      if( ropen )
      {
	/* both sides are open, so try the smallest side */
	try_left = HaArrayCount(wts->left_wing) < HaArrayCount(wts->right_wing);
      }
      else
      {
	/* only the left side is open, so try that */
	try_left = true;
      }
    }
    else
    {
      if( ropen )
      {
	/* only the right side is open, so try that */
	try_left = false;
      }
      else
      {
	/* neither side is open, time to stop */
	break;
      }
    }

    /* try the left or the right side, depending on try_left */
    if( try_left )
    {
      if( KheFindFrom(tf, in.first-1, &in, rtm, rt, NULL, false,rg,tg_start,pf) )
      {
	p = KhePartMake(wts);
	KhePartFromUnion(&p->from, pf);
	HaArrayAddLast(wts->left_wing, p);
      }
      else
	lopen = false;
    }
    else
    {
      if( KheFindFrom(tf, in.last+1, &in, rtm, rt, NULL, false,rg,tg_start,pf) )
      {
	p = KhePartMake(wts);
	KhePartFromUnion(&p->from, pf);
	HaArrayAddLast(wts->right_wing, p);
      }
      else
	ropen = false;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftAndRightWings(KHE_WIDENED_TASK_SET wts,   */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc)  */
/*                                                                           */
/*  Build left and right wings for wts containing up to mwc parts.           */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetBuildLeftAndRightWings(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc,
  KHE_MTASK_FINDER mtf)
{
  KHE_INTERVAL in;  KHE_MPART_FROM pf;  bool lopen, ropen, try_left;
  KHE_MPART p;  KHE_TASK_FINDER tf;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = wmts->from_r_rtm;
  tf = wmts->tf;
  in = wmts->core.from.in;
  pf = &tf->scratch_mfrom;
  lopen = true;  ropen = true;
  while( HaArrayCount(wmts->left_wing) + HaArrayCount(wmts->right_wing) < mwc )
  {
    /* sort out which side to try, or break if neither side is open */
    if( lopen )
    {
      if( ropen )
      {
	/* both sides are open, so try the smallest side */
	try_left =
	  HaArrayCount(wmts->left_wing) < HaArrayCount(wmts->right_wing);
      }
      else
      {
	/* only the left side is open, so try that */
	try_left = true;
      }
    }
    else
    {
      if( ropen )
      {
	/* only the right side is open, so try that */
	try_left = false;
      }
      else
      {
	/* neither side is open, time to stop */
	break;
      }
    }

    /* try the left or the right side, depending on try_left */
    if( try_left )
    {
      if( KheMTaskFindFrom(tf, in.first-1, &in, rtm, rt, NULL, false,
	  rg, tg_start, pf, mtf) )
      {
	p = KheMPartMake(wmts, mtf);
	KheMPartFromUnion(&p->from, pf);
	HaArrayAddLast(wmts->left_wing, p);
      }
      else
	lopen = false;
    }
    else
    {
      if( KheMTaskFindFrom(tf, in.last+1, &in, rtm, rt, NULL, false,
	  rg, tg_start, pf, mtf) )
      {
	p = KheMPartMake(wmts, mtf);
	KheMPartFromUnion(&p->from, pf);
	HaArrayAddLast(wmts->right_wing, p);
      }
      else
	ropen = false;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_TASK_SET KheWidenedTaskSetGet(KHE_TASK_FINDER tf,            */
/*    KHE_RESOURCE from_r, KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm,        */
/*    KHE_RESOURCE to_r, KHE_RESOURCE_TIMETABLE_MONITOR to_r_rtm)            */
/*                                                                           */
/*  Get a widened task set from tf's arena or free list, with all its        */
/*  fields initialized to the values passed, or to null values.              */
/*                                                                           */
/*****************************************************************************/

static KHE_WIDENED_TASK_SET KheWidenedTaskSetGet(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm,
  KHE_RESOURCE to_r, KHE_RESOURCE_TIMETABLE_MONITOR to_r_rtm)
{
  KHE_WIDENED_TASK_SET res;  HA_ARENA a;
  if( HaArrayCount(tf->free_widened_task_sets) > 0 )
  {
    /* get res from tf's free list */
    res = HaArrayLastAndDelete(tf->free_widened_task_sets);
    KhePartClear(&res->core);
    HaArrayClear(res->left_wing);
    HaArrayClear(res->right_wing);
    HaArrayClear(res->swappable_wings);
  }
  else
  {
    /* get res from tf's arena */
    a = tf->arena;
    HaMake(res, a);
    res->tf = tf;
    KhePartInit(&res->core, KheTaskSetMake(tf->soln), KheTaskSetMake(tf->soln));
    HaArrayInit(res->left_wing, a);
    HaArrayInit(res->right_wing, a);
    HaArrayInit(res->swappable_wings, a);
  }
  res->from_r = from_r;
  res->from_r_rtm = from_r_rtm;
  res->to_r = to_r;
  res->to_r_rtm = to_r_rtm;
  res->to_r_max_left_wing_count = 0;
  res->to_r_max_right_wing_count = 0;
  res->best_cost = KheCost(INT_MAX, INT_MAX);
  res->best_defects = INT_MAX;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_TASK_SET KheWidenedTaskSetGet(KHE_TASK_FINDER tf,            */
/*    KHE_RESOURCE from_r, KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm,        */
/*    KHE_RESOURCE to_r, KHE_RESOURCE_TIMETABLE_MONITOR to_r_rtm)            */
/*                                                                           */
/*  Get a widened task set from tf's arena or free list, with all its        */
/*  fields initialized to the values passed, or to null values.              */
/*                                                                           */
/*****************************************************************************/

static KHE_WIDENED_MTASK_SET KheWidenedMTaskSetGet(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm,
  KHE_RESOURCE to_r, KHE_RESOURCE_TIMETABLE_MONITOR to_r_rtm,
  KHE_MTASK_FINDER mtf)
{
  KHE_WIDENED_MTASK_SET res;  HA_ARENA a;
  if( HaArrayCount(tf->free_widened_mtask_sets) > 0 )
  {
    /* get res from tf's free list */
    res = HaArrayLastAndDelete(tf->free_widened_mtask_sets);
    KheMPartClear(&res->core);
    HaArrayClear(res->left_wing);
    HaArrayClear(res->right_wing);
    HaArrayClear(res->swappable_wings);
  }
  else
  {
    /* get res from tf's arena */
    a = tf->arena;
    HaMake(res, a);
    res->tf = tf;
    KheMPartInit(&res->core, KheMTaskSetMake(mtf),
      KheMTaskSetMake(mtf));
    HaArrayInit(res->left_wing, a);
    HaArrayInit(res->right_wing, a);
    HaArrayInit(res->swappable_wings, a);
  }
  res->from_r = from_r;
  res->from_r_rtm = from_r_rtm;
  res->to_r = to_r;
  res->to_r_rtm = to_r_rtm;
  res->to_r_max_left_wing_count = 0;
  res->to_r_max_right_wing_count = 0;
  res->best_cost = KheCost(INT_MAX, INT_MAX);
  res->best_defects = INT_MAX;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetDoMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,    */
/*    KHE_TASK_SET from_r_ts, KHE_WIDENED_TASK_SET *wts,                     */
/*    KHE_RESOURCE_TYPE *rt, KHE_RESOURCE_GROUP *rg, int *tg_start)          */
/*                                                                           */
/*  Do most of the work of building a widened task set; all but the wings.   */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetDoMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_TASK_SET from_r_ts, KHE_WIDENED_TASK_SET *wts, KHE_RESOURCE_TYPE *rt,
  KHE_RESOURCE_GROUP *rg, int *tg_start)
{
  KHE_TASK task;  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm;
  KHE_INTERVAL task_in;  int i;  KHE_RESOURCE r;  KHE_INTERVAL from_r_in;

  /* check the basic conditions for from_r_ts and find its bounding interval */
  if( KheTaskSetTaskCount(from_r_ts) == 0 )
    return *wts = NULL, false;
  from_r_in = KheInterval(1, 0);
  for( i = 0;  i < KheTaskSetTaskCount(from_r_ts);  i++ )
  {
    task = KheTaskSetTask(from_r_ts, i);
    HnAssert(KheTaskAsstResource(task) == from_r,
      "KheWidenedTaskSetMake: from_r_ts has a task not assigned from_r");
    if( KheTaskProperRoot(task) != task || KheTaskIsPreassigned(task, &r) )
      return *wts = NULL, false;
    task_in = KheTaskInterval(tf, task);
    if( !KheIntervalDisjoint(task_in, from_r_in) )
      return *wts = NULL, false;
    KheIntervalUnion(&from_r_in, task_in);
  }

  /* get the first task and its resource type */
  task = KheTaskSetFirst(from_r_ts);
  *rt = KheResourceGroupResourceType(KheTaskDomain(task));

  /* get res, the new widened task set object, and set rg and tg_start */
  if( from_r != NULL )
  {
    from_r_rtm = KheResourceTimetableMonitor(tf->soln, from_r);
    *rg = NULL;
    *tg_start = 0;
  }
  else
  {
    from_r_rtm = NULL;
    *rg = KheTaskDomain(task);
    *tg_start = KheTaskIndexInFrameTimeGroup(task, tf->days_frame);
  }
  *wts = KheWidenedTaskSetGet(tf, from_r, from_r_rtm, (KHE_RESOURCE) 0x1, NULL);
  KhePartFromAddTasks(&(*wts)->core.from, from_r_ts, from_r_in);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetDoMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,    */
/*    KHE_TASK_SET from_r_ts, KHE_WIDENED_TASK_SET *wts,                     */
/*    KHE_RESOURCE_TYPE *rt, KHE_RESOURCE_GROUP *rg, int *tg_start)          */
/*                                                                           */
/*  Do most of the work of building a widened task set; all but the wings.   */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedMTaskSetDoMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_MTASK_SET from_r_mts, KHE_WIDENED_MTASK_SET *wmts,
  KHE_RESOURCE_TYPE *rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  KHE_MTASK_FINDER mtf)
{
  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm;
  KHE_INTERVAL task_in;  int i;  KHE_RESOURCE r;  KHE_INTERVAL from_r_in;
  KHE_MTASK mt;

  /* check the basic conditions for from_r_mts and find its bounding interval */
  if( KheMTaskSetMTaskCount(from_r_mts) == 0 )
    return *wmts = NULL, false;
  from_r_in = KheInterval(1, 0);
  for( i = 0;  i < KheMTaskSetMTaskCount(from_r_mts);  i++ )
  {
    mt = KheMTaskSetMTask(from_r_mts, i);
    /* ***
    HnAssert(KheTaskAsstResource(task) == from_r,
      "KheWidenedTaskSetMake: from_r_ts has a task not assigned from_r");
    *** */
    if( KheMTaskIsPreassigned(mt, &r) )
      return *wmts = NULL, false;
    /* ***
    if( KheTaskProperRoot(task) != task || KheTaskIsPreassigned(task, &r) )
      return *wts = NULL, false;
    *** */
    task_in = KheMTaskInterval(tf, mt);
    if( !KheIntervalDisjoint(task_in, from_r_in) )
      return *wmts = NULL, false;
    KheIntervalUnion(&from_r_in, task_in);
  }

  /* get the first mtask and its resource type */
  mt = KheMTaskSetFirst(from_r_mts);
  *rt = KheResourceGroupResourceType(KheMTaskDomain(mt));

  /* get res, the new widened mtask set object, and set rg and tg_start */
  if( from_r != NULL )
  {
    from_r_rtm = KheResourceTimetableMonitor(tf->soln, from_r);
    *rg = NULL;
    *tg_start = 0;
  }
  else
  {
    from_r_rtm = NULL;
    *rg = KheMTaskDomain(mt);
    *tg_start = KheMTaskIndexInFrameTimeGroup(mt, tf->days_frame);
  }
  *wmts = KheWidenedMTaskSetGet(tf, from_r, from_r_rtm,
    (KHE_RESOURCE) 0x1, NULL, mtf);
  KheMPartFromAddMTasks(&(*wmts)->core.from, from_r_mts, from_r_in);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,      */
/*    KHE_TASK_SET from_r_ts, int max_left_wing_count,                       */
/*    int max_right_wing_count, KHE_WIDENED_TASK_SET *wts)                   */
/*                                                                           */
/*  Check the basic conditions for from_r_ts and make a wts for it.          */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_TASK_SET from_r_ts, int max_left_wing_count,
  int max_right_wing_count, KHE_WIDENED_TASK_SET *wts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  /* make the widened task set object and set its core */
  if( !KheWidenedTaskSetDoMake(tf, from_r, from_r_ts, wts, &rt, &rg,&tg_start) )
    return false;

  /* set the wings */
  KheWidenedTaskSetBuildLeftWing(*wts, rt, &rg, &tg_start, max_left_wing_count);
  KheWidenedTaskSetBuildRightWing(*wts,rt, &rg, &tg_start,max_right_wing_count);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_MTASK_SET from_r_mts, int max_left_wing_count,
  int max_right_wing_count, KHE_WIDENED_MTASK_SET *wmts,
  KHE_MTASK_FINDER mtf)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  /* make the widened mtask set object and set its core */
  if( !KheWidenedMTaskSetDoMake(tf, from_r, from_r_mts, wmts,
	&rt, &rg, &tg_start, mtf) )
    return false;

  /* set the wings */
  KheWidenedMTaskSetBuildLeftWing(*wmts, rt, &rg, &tg_start,
    max_left_wing_count, mtf);
  KheWidenedMTaskSetBuildRightWing(*wmts,rt, &rg, &tg_start,
    max_right_wing_count, mtf);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMakeFlexible(KHE_TASK_FINDER tf,                   */
/*    KHE_RESOURCE from_r, KHE_TASK_SET from_r_ts,                           */
/*    int max_wing_count, KHE_WIDENED_TASK_SET *wts)                         */
/*                                                                           */
/*  Like KheWidenedTaskSetMake except that max_wing_count constrains the     */
/*  total number of tasks in the two wings, taken together.                  */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetMakeFlexible(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_TASK_SET from_r_ts,
  int max_wing_count, KHE_WIDENED_TASK_SET *wts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  /* make the widened task set object and set its core */
  if( !KheWidenedTaskSetDoMake(tf, from_r, from_r_ts, wts, &rt, &rg,&tg_start) )
    return false;

  /* set both wings */
  KheWidenedTaskSetBuildLeftAndRightWings(*wts, rt, &rg, &tg_start,
    max_wing_count);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetMakeFlexible(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_MTASK_SET from_r_mts,
  int max_wing_count, KHE_WIDENED_MTASK_SET *wmts,
  KHE_MTASK_FINDER mtf)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  /* make the widened task set object and set its core */
  if( !KheWidenedMTaskSetDoMake(tf, from_r, from_r_mts, wmts, &rt,
      &rg, &tg_start, mtf) )
    return false;

  /* set both wings */
  KheWidenedMTaskSetBuildLeftAndRightWings(*wmts, rt, &rg, &tg_start,
    max_wing_count, mtf);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetDelete(KHE_WIDENED_TASK_SET wts)                   */
/*                                                                           */
/*  Delete wts by cycling it through a free list in tf.                      */
/*                                                                           */
/*  Implementation note.  The number of wings will change next time, so      */
/*  we move them to a free list too.                                         */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetDelete(KHE_WIDENED_TASK_SET wts)
{
  int i;  KHE_TASK_FINDER tf;
  tf = wts->tf;
  HaArrayAppend(tf->free_parts, wts->left_wing, i);
  HaArrayAppend(tf->free_parts, wts->right_wing, i);
  HaArrayAddLast(tf->free_widened_task_sets, wts);
  /* don't append swappable_wings, they lie in left_wing and right_wing */
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetDelete(KHE_WIDENED_MTASK_SET wmts,
  KHE_MTASK_FINDER mtf)
{
  int i;  KHE_TASK_FINDER tf;
  tf = wmts->tf;
  HaArrayAppend(tf->free_mparts, wmts->left_wing, i);
  HaArrayAppend(tf->free_mparts, wmts->right_wing, i);
  HaArrayAddLast(tf->free_widened_mtask_sets, wmts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetInterval(KHE_WIDENED_TASK_SET wts,                 */
/*    int left_count, int right_count, int *first_index, int *last_index)    */
/*                                                                           */
/*  Return the interval covered by wts's core, its first left_count left     */
/*  wings, and its first right_count right wings.                            */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetInterval(KHE_WIDENED_TASK_SET wts,
  int left_count, int right_count, int *first_index, int *last_index)
{
  KHE_INTERVAL res;  KHE_PART p;  int i;
  res = wts->core.from.in;
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wts->left_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wts->right_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetInterval(KHE_WIDENED_MTASK_SET wmts,
  int left_count, int right_count, int *first_index, int *last_index)
{
  KHE_INTERVAL res;  KHE_MPART p;  int i;
  res = wmts->core.from.in;
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetFullInterval(KHE_WIDENED_TASK_SET wts,             */
/*    int *first_index, int *last_index)                                     */
/*                                                                           */
/*  Return the interval covered by wts's core and all its wings.             */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetFullInterval(KHE_WIDENED_TASK_SET wts,
  int *first_index, int *last_index)
{
  KHE_INTERVAL res;  KHE_PART p;  int i;
  res = wts->core.from.in;
  for( i = 0;  i < HaArrayCount(wts->left_wing);  i++ )
  {
    p = HaArray(wts->left_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  for( i = 0;  i < HaArrayCount(wts->right_wing);  i++ )
  {
    p = HaArray(wts->right_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetFullInterval(KHE_WIDENED_MTASK_SET wmts,
  int *first_index, int *last_index)
{
  KHE_INTERVAL res;  KHE_MPART p;  int i;
  res = wmts->core.from.in;
  for( i = 0;  i < HaArrayCount(wmts->left_wing);  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  for( i = 0;  i < HaArrayCount(wmts->right_wing);  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    KheIntervalUnion(&res, p->from.in);
  }
  *first_index = res.first;
  *last_index = res.last;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMovableWidenedTaskSetRight(KHE_TASK_FINDER tf,               */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,          */
/*    KHE_WIDENED_TASK_SET *res_wts)                                         */
/*                                                                           */
/*  Find the first maximal widened task set at or to the right of            */
/*  days_first_index that can be moved from from_r to to_r.  Core only.      */
/*                                                                           */
/*****************************************************************************/

bool KheFindMovableWidenedTaskSetRight(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,
  KHE_WIDENED_TASK_SET *res_wts)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_TASK_SET res;  KHE_PART_FROM pf;  KHE_PART_TO pt;
  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm, to_r_rtm;  KHE_RESOURCE_TYPE rt;
  bool ff, ft;

  /* boilerplate */
  if( DEBUG3 )
    fprintf(stderr, "[ KheFindMovableWidenedTaskSetRight(tf, %s, %s, %d, -)\n",
      KheResourceShow(from_r), KheResourceShow(to_r), days_first_index);
  HnAssert(from_r != to_r, "KheFindMovableWidenedTaskSetRight: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  from_r_rtm = KheTimetableMonitor(tf->soln, from_r);
  to_r_rtm = KheTimetableMonitor(tf->soln, to_r);
  pf = &tf->scratch_from;
  pt = &tf->scratch_to;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  /* search for a suitable core */
  in = KheInterval(0, days_first_index - 1);
  rg = NULL, tg_start = 0;
  for( i = days_first_index;  i < tg_count;  i++ )
  {
    ff = KheFindFrom(tf, i, &in, from_r_rtm, rt, to_r, false,&rg,&tg_start,pf);
    if( DEBUG3 )
      fprintf(stderr, "  at %d, ff = %s\n", i, bool_show(ff));
    if( ff )
    {
      ft = KheFindTo(tf, pf->in, false, to_r_rtm, true, NULL, pt);
      if( DEBUG3 )
	fprintf(stderr, "  at %d, ft = %s\n", i, bool_show(ff));
      if( ft )
      {
	/* run begins at i; get res */
	res = KheWidenedTaskSetGet(tf, from_r, from_r_rtm, to_r, to_r_rtm);

	/* repeatedly add pf and pt to the run and skip to after it */
	do
	{
	  KhePartFromUnion(&res->core.from, pf);
	  KhePartToUnion(&res->core.to, pt);
	  i = in.last + 1;
	} while( KheFindFrom(tf, i, &in, from_r_rtm, rt, to_r,
	    false, &rg, &tg_start, pf) &&
	    KheFindTo(tf, pf->in, false, to_r_rtm, true, res->core.to.ts, pt) );

	/* run ends here */
	if( DEBUG3 )
	  fprintf(stderr, "] KheFindMovableWidenedTaskSetRight returning true\n");
	return *res_wts = res, true;
      }
    }
  }

  /* if we get here, no luck */
  if( DEBUG3 )
    fprintf(stderr, "] KheFindMovableWidenedTaskSetRight returning false\n");
  return *res_wts = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheFindMovableWidenedMTaskSetRight(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,
  KHE_WIDENED_MTASK_SET *res_wmts, KHE_MTASK_FINDER mtf)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_MTASK_SET res;  KHE_MPART_FROM pf;  KHE_MPART_TO pt;
  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm, to_r_rtm;  KHE_RESOURCE_TYPE rt;
  bool ff, ft;

  /* boilerplate */
  if( DEBUG3 )
    fprintf(stderr, "[ KheFindMovableWidenedMTaskSetRight(tf, %s, %s, %d, -)\n",
      KheResourceShow(from_r), KheResourceShow(to_r), days_first_index);
  HnAssert(from_r != to_r, "KheFindMovableWidenedMTaskSetRight: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  from_r_rtm = KheTimetableMonitor(tf->soln, from_r);
  to_r_rtm = KheTimetableMonitor(tf->soln, to_r);
  pf = &tf->scratch_mfrom;
  pt = &tf->scratch_mto;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  /* search for a suitable core */
  in = KheInterval(0, days_first_index - 1);
  rg = NULL, tg_start = 0;
  for( i = days_first_index;  i < tg_count;  i++ )
  {
    ff = KheMTaskFindFrom(tf, i, &in, from_r_rtm, rt, to_r, false,
      &rg, &tg_start, pf, mtf);
    if( DEBUG3 )
      fprintf(stderr, "  at %d, ff = %s\n", i, bool_show(ff));
    if( ff )
    {
      ft = KheMTaskFindTo(tf, pf->in, false, to_r_rtm, true, NULL, pt,
	mtf);
      if( DEBUG3 )
	fprintf(stderr, "  at %d, ft = %s\n", i, bool_show(ff));
      if( ft )
      {
	/* run begins at i; get res */
	res = KheWidenedMTaskSetGet(tf, from_r, from_r_rtm, to_r, to_r_rtm,
	  mtf);

	/* repeatedly add pf and pt to the run and skip to after it */
	do
	{
	  KheMPartFromUnion(&res->core.from, pf);
	  KheMPartToUnion(&res->core.to, pt);
	  i = in.last + 1;
	} while( KheMTaskFindFrom(tf, i, &in, from_r_rtm, rt, to_r,
	    false, &rg, &tg_start, pf, mtf) &&
	    KheMTaskFindTo(tf, pf->in, false, to_r_rtm, true,
	      res->core.to.mts, pt, mtf) );

	/* run ends here */
	if( DEBUG3 )
	  fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight ret. true\n");
	return *res_wmts = res, true;
      }
    }
  }

  /* if we get here, no luck */
  if( DEBUG3 )
    fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight returning false\n");
  return *res_wmts = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMovableWidenedTaskSetLeft(KHE_TASK_FINDER tf,                */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,           */
/*    KHE_WIDENED_TASK_SET *res_wts)                                         */
/*                                                                           */
/*  Find the first maximal widened task set at or to the left of             */
/*  days_last_index that can be moved from from_r to to_r.  Core only.       */
/*                                                                           */
/*****************************************************************************/

bool KheFindMovableWidenedTaskSetLeft(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,
  KHE_WIDENED_TASK_SET *res_wts)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_TASK_SET res;  KHE_PART_FROM pf;  KHE_PART_TO pt;
  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm, to_r_rtm;  KHE_RESOURCE_TYPE rt;

  /* boilerplate */
  HnAssert(from_r != to_r, "KheFindMovableWidenedTaskSetLeft: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  from_r_rtm = KheTimetableMonitor(tf->soln, from_r);
  to_r_rtm = KheTimetableMonitor(tf->soln, to_r);
  pf = &tf->scratch_from;
  pt = &tf->scratch_to;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  /* search for a suitable core */
  in = KheInterval(days_last_index + 1, tg_count - 1);
  rg = NULL, tg_start = 0;
  for( i = days_last_index;  i >= 0;  i-- )
  {
    if( KheFindFrom(tf, i, &in, from_r_rtm, rt, to_r, false, &rg, &tg_start, pf)
	&& KheFindTo(tf, pf->in, false, to_r_rtm, true, NULL, pt) )
    {
      /* run ends at i; get res */
      res = KheWidenedTaskSetGet(tf, from_r, from_r_rtm, to_r, to_r_rtm);

      /* repeatedly add pf and pt to the run and skip to before it */
      do
      {
	KhePartFromUnion(&res->core.from, pf);
	KhePartToUnion(&res->core.to, pt);
        i = in.first - 1;
      } while( KheFindFrom(tf, i, &in,from_r_rtm,rt,to_r,false,&rg,&tg_start,pf)
	  && KheFindTo(tf, pf->in, false, to_r_rtm, true, res->core.to.ts,pt) );

      /* run ends here */
      return *res_wts = res, true;
    }
  }

  /* if we get here, no luck */
  return *res_wts = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheFindMovableWidenedMTaskSetLeft(KHE_TASK_FINDER tf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,
  KHE_WIDENED_MTASK_SET *res_wmts, KHE_MTASK_FINDER mtf)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_MTASK_SET res;  KHE_MPART_FROM pf;  KHE_MPART_TO pt;
  KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm, to_r_rtm;  KHE_RESOURCE_TYPE rt;

  /* boilerplate */
  HnAssert(from_r != to_r, "KheFindMovableWidenedTaskSetLeft: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(tf->days_frame);
  from_r_rtm = KheTimetableMonitor(tf->soln, from_r);
  to_r_rtm = KheTimetableMonitor(tf->soln, to_r);
  pf = &tf->scratch_mfrom;
  pt = &tf->scratch_mto;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  /* search for a suitable core */
  in = KheInterval(days_last_index + 1, tg_count - 1);
  rg = NULL, tg_start = 0;
  for( i = days_last_index;  i >= 0;  i-- )
  {
    if( KheMTaskFindFrom(tf, i, &in, from_r_rtm, rt, to_r, false, &rg,
	  &tg_start, pf, mtf) &&
	KheMTaskFindTo(tf, pf->in, false, to_r_rtm, true, NULL, pt,
	  mtf) )
    {
      /* run ends at i; get res */
      res = KheWidenedMTaskSetGet(tf, from_r, from_r_rtm, to_r, to_r_rtm,
	mtf);

      /* repeatedly add pf and pt to the run and skip to before it */
      do
      {
	KheMPartFromUnion(&res->core.from, pf);
	KheMPartToUnion(&res->core.to, pt);
        i = in.first - 1;
      } while( KheMTaskFindFrom(tf, i, &in,from_r_rtm, rt, to_r, false,
	    &rg, &tg_start, pf, mtf) &&
	  KheMTaskFindTo(tf, pf->in, false, to_r_rtm, true,
	    res->core.to.mts, pt, mtf) );

      /* run ends here */
      return *res_wmts = res, true;
    }
  }

  /* if we get here, no luck */
  return *res_wmts = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetDoDebug(KHE_TASK_SET ts, int verbosity, FILE *fp)         */
/*                                                                           */
/*  Debug print of ts without enclosing braces, for local use.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSetDoDebug(KHE_TASK_SET ts, int first_index,
  int last_index, int verbosity, FILE *fp)
{
  int i;  KHE_TASK task;
  if( last_index == -1 )
    last_index = KheTaskSetTaskCount(ts) - 1;
  for( i = first_index;  i <= last_index;  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( i > first_index )
      fprintf(fp, ", ");
    KheTaskDebug(task, verbosity, -1, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetDoDebug(KHE_TASK_SET ts, int verbosity, FILE *fp)         */
/*                                                                           */
/*  Debug print of ts without enclosing braces, for local use.               */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetDoDebug(KHE_MTASK_SET mts, int first_index,
  int last_index, int verbosity, FILE *fp)
{
  int i;  KHE_MTASK mt;
  if( last_index == -1 )
    last_index = KheMTaskSetMTaskCount(mts) - 1;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( i > first_index )
      fprintf(fp, ", ");
    KheMTaskDebug(mt, verbosity, -1, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetDoDebug(KHE_WIDENED_TASK_SET wts,                  */
/*    int left_count, int right_count, int first_index, int last_index,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of widened task set wts (but only the first left_count       */
/*  tasks of the left wing, and only the first right_count tasks of the      */
/*  right wing) onto fp with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetDoDebug(KHE_WIDENED_TASK_SET wts, bool opt,
  int left_count, int right_count, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KHE_PART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  if( left_count > HaArrayCount(wts->left_wing) )
    left_count = HaArrayCount(wts->left_wing);
  if( right_count > HaArrayCount(wts->right_wing) )
    right_count = HaArrayCount(wts->right_wing);
  for( i = left_count - 1;  i >= 0;  i-- )
  {
    p = HaArray(wts->left_wing, i);
    if( i < left_count - 1 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheTaskSetDoDebug(p->from.ts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  if( left_count > 0 || right_count > 0 )
  {
    if( left_count > 0 )
      fprintf(fp, " ");
    fprintf(fp, "[");
  }
  KheTaskSetDoDebug(wts->core.from.ts, first_index, last_index, verbosity, fp);
  if( left_count > 0 || right_count > 0 )
  {
    fprintf(fp, "]");
    if( right_count > 0 )
      fprintf(fp, " ");
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wts->right_wing, i);
    if( i > 0 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheTaskSetDoDebug(p->from.ts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetDoDebug(KHE_WIDENED_TASK_SET wts,                  */
/*    int left_count, int right_count, int first_index, int last_index,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of widened task set wts (but only the first left_count       */
/*  tasks of the left wing, and only the first right_count tasks of the      */
/*  right wing) onto fp with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetDoDebug(KHE_WIDENED_MTASK_SET wmts, bool opt,
  int left_count, int right_count, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KHE_MPART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  if( left_count > HaArrayCount(wmts->left_wing) )
    left_count = HaArrayCount(wmts->left_wing);
  if( right_count > HaArrayCount(wmts->right_wing) )
    right_count = HaArrayCount(wmts->right_wing);
  for( i = left_count - 1;  i >= 0;  i-- )
  {
    p = HaArray(wmts->left_wing, i);
    if( i < left_count - 1 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheMTaskSetDoDebug(p->from.mts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  if( left_count > 0 || right_count > 0 )
  {
    if( left_count > 0 )
      fprintf(fp, " ");
    fprintf(fp, "[");
  }
  KheMTaskSetDoDebug(wmts->core.from.mts, first_index, last_index,
    verbosity, fp);
  if( left_count > 0 || right_count > 0 )
  {
    fprintf(fp, "]");
    if( right_count > 0 )
      fprintf(fp, " ");
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( i > 0 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheMTaskSetDoDebug(p->from.mts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetDebug(KHE_WIDENED_TASK_SET wts, int left_count,    */
/*    int right_count, int verbosity, int indent, FILE *fp)                  */
/*                                                                           */
/*  Debug print of widened task set wts (but only the first left_count       */
/*  tasks of the left wing, and only the first right_count tasks of the      */
/*  right wing) onto fp with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetDebug(KHE_WIDENED_TASK_SET wts, int left_count,
  int right_count, int verbosity, int indent, FILE *fp)
{
  KheWidenedTaskSetDoDebug(wts, false, left_count, right_count, 0, -1,
    verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetDebug(KHE_WIDENED_MTASK_SET wmts,
  int left_count, int right_count, int verbosity, int indent, FILE *fp)
{
  KheWidenedMTaskSetDoDebug(wmts, false, left_count, right_count, 0, -1,
    verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened task set moves"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheClearOptimal(KHE_WIDENED_TASK_SET wts)                           */
/*                                                                           */
/*  Make clear that any previously discovered optimal move is out of date.   */
/*                                                                           */
/*****************************************************************************/

static void KheClearOptimal(KHE_WIDENED_TASK_SET wts)
{
  wts->best_defects = INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClearOptimal(KHE_WIDENED_TASK_SET wts)                           */
/*                                                                           */
/*  Make clear that any previously discovered optimal move is out of date.   */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskClearOptimal(KHE_WIDENED_MTASK_SET wmts)
{
  wmts->best_defects = INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHaveOptimal(KHE_WIDENED_TASK_SET wts)                            */
/*                                                                           */
/*  Return true if an optimal result is currently present.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheHaveOptimal(KHE_WIDENED_TASK_SET wts)
{
  return wts->best_defects < INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHaveOptimal(KHE_WIDENED_TASK_SET wts)                            */
/*                                                                           */
/*  Return true if an optimal result is currently present.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskHaveOptimal(KHE_WIDENED_MTASK_SET wmts)
{
  return wmts->best_defects < INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePrepareResource(KHE_WIDENED_TASK_SET wts, KHE_RESOURCE to_r)     */
/*                                                                           */
/*  Get wts ready for operations involving moves from from_r to to_r, or     */
/*  return false if that can't be done.  Do nothing if already set for to_r. */
/*                                                                           */
/*  Implementation note.  When to_r == NULL, KheTimetableMonitor will        */
/*  return NULL, and then KheFindTo will clear pt and return true, so this   */
/*  code does the right thing when to_r == NULL.                             */
/*                                                                           */
/*****************************************************************************/

static bool KhePrepareResource(KHE_WIDENED_TASK_SET wts, KHE_RESOURCE to_r)
{
  int i;  KHE_PART p;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK_FINDER tf;
  if( to_r != wts->to_r )
  {
    /* set wts->to_r */
    wts->to_r = to_r;
    rtm = wts->to_r_rtm = KheTimetableMonitor(wts->tf->soln, to_r);
    tf = wts->tf;

    /* core - fail if it doesn't work */
    if( !KheFindTo(tf, wts->core.from.in, true, rtm, false,NULL,&wts->core.to) )
      return wts->to_r = (KHE_RESOURCE) 0x1, false;
    if( DEBUG4 )
    {
      fprintf(stderr, "  PrepareResource(wts, %s) core %s = ",
	KheResourceShow(to_r), KheTaskFinderIntervalShow(tf,wts->core.from.in));
      KheTaskSetDebug(wts->core.to.ts, 2, 0, stderr);
    }

    /* left wing - stop at first that doesn't work */
    HaArrayForEach(wts->left_wing, p, i)
      if( !KheFindTo(tf, p->from.in, true, rtm, false, NULL, &p->to) )
	break;
    wts->to_r_max_left_wing_count = i;

    /* right wing - stop at first that doesn't work */
    HaArrayForEach(wts->right_wing, p, i)
      if( !KheFindTo(tf, p->from.in, true, rtm, false, NULL, &p->to) )
	break;
    wts->to_r_max_right_wing_count = i;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePrepareResource(KHE_WIDENED_TASK_SET wts, KHE_RESOURCE to_r)     */
/*                                                                           */
/*  Get wts ready for operations involving moves from from_r to to_r, or     */
/*  return false if that can't be done.  Do nothing if already set for to_r. */
/*                                                                           */
/*  Implementation note.  When to_r == NULL, KheTimetableMonitor will        */
/*  return NULL, and then KheFindTo will clear pt and return true, so this   */
/*  code does the right thing when to_r == NULL.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskPrepareResource(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, KHE_MTASK_FINDER mtf)
{
  int i;  KHE_MPART p;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK_FINDER tf;
  if( to_r != wmts->to_r )
  {
    /* set wts->to_r */
    wmts->to_r = to_r;
    rtm = wmts->to_r_rtm = KheTimetableMonitor(wmts->tf->soln, to_r);
    tf = wmts->tf;

    /* core - fail if it doesn't work */
    if( !KheMTaskFindTo(tf, wmts->core.from.in, true, rtm, false,
	  NULL, &wmts->core.to, mtf) )
      return wmts->to_r = (KHE_RESOURCE) 0x1, false;
    if( DEBUG4 )
    {
      fprintf(stderr, "  PrepareResource(wts, %s) core %s = ",
	KheResourceShow(to_r),
	KheTaskFinderIntervalShow(tf, wmts->core.from.in));
      KheMTaskSetDebug(wmts->core.to.mts, 2, 0, stderr);
    }

    /* left wing - stop at first that doesn't work */
    HaArrayForEach(wmts->left_wing, p, i)
      if( !KheMTaskFindTo(tf, p->from.in, true, rtm, false, NULL,
	    &p->to, mtf) )
	break;
    wmts->to_r_max_left_wing_count = i;

    /* right wing - stop at first that doesn't work */
    HaArrayForEach(wmts->right_wing, p, i)
      if( !KheMTaskFindTo(tf, p->from.in, true, rtm, false, NULL,
	    &p->to, mtf) )
	break;
    wmts->to_r_max_right_wing_count = i;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMoveCheck(KHE_WIDENED_TASK_SET wts,                */
/*    KHE_RESOURCE to_r, bool force, int *max_left_count,                    */
/*    int *max_right_count)                                                  */
/*                                                                           */
/*  Return true if the core of wts can move to to_r; set *max_left_count     */
/*  and *max_right_count to the number of left and right wings that can too. */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetMoveCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, bool force, int *max_left_count, int *max_right_count)
{
  int i, j;  KHE_PART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheClearOptimal(wts);
  if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
    return *max_left_count = *max_right_count = -1, false;

  /* core must be movable, else fail */
  if( !KhePartMoveCheck(&wts->core, force, to_r) )
    return *max_left_count = *max_right_count = -1, false;

  /* find i, the number of left wing parts that are movable */
  for( i = 0;  i < wts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wts->left_wing, i);
    if( !KhePartMoveCheck(p, force, to_r) )
      break;
  }

  /* find j, the number of right wing parts that are movable */
  for( j = 0;  j < wts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wts->right_wing, j);
    if( !KhePartMoveCheck(p, force, to_r) )
      break;
  }

  /* all good, set *max_left_count and *max_right_count and return */
  return *max_left_count = i, *max_right_count = j, true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetMoveCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool force,
  int *max_left_count, int *max_right_count, KHE_MTASK_FINDER mtf)
{
  int i, j;  KHE_MPART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r ||
      !KheMTaskPrepareResource(wmts, to_r, mtf) )
    return *max_left_count = *max_right_count = -1, false;

  /* core must be movable, else fail */
  if( !KheMPartMoveCheck(&wmts->core, force, from_r, to_r) )
    return *max_left_count = *max_right_count = -1, false;

  /* find i, the number of left wing parts that are movable */
  for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartMoveCheck(p, force, from_r, to_r) )
      break;
  }

  /* find j, the number of right wing parts that are movable */
  for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wmts->right_wing, j);
    if( !KheMPartMoveCheck(p, force, from_r, to_r) )
      break;
  }

  /* all good, set *max_left_count and *max_right_count and return */
  return *max_left_count = i, *max_right_count = j, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMove(KHE_WIDENED_TASK_SET wts,                     */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Move the core of wts, its first left_count left wings, and its first     */
/*  right_count right wings, to to_r.  If successful return true and set     */
/*  *from_r_durn_change and *to_r_durn_change to the change in duration.     */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetMove(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int i;  KHE_PART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheClearOptimal(wts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
    return false;

  /* check that left_count and right_count are in range */
  HnAssert(0 <= left_count && left_count <= wts->to_r_max_left_wing_count,
    "KheWidenedTaskSetMove: left_count (%d) out of range (0 .. %d)",
    left_count, wts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wts->to_r_max_right_wing_count,
    "KheWidenedTaskSetMove: right_count (%d) out of range (0 .. %d)",
    right_count, wts->to_r_max_right_wing_count);

  /* move the core */
  if( !KhePartMove(&wts->core, to_r, from_r_durn_change, to_r_durn_change) )
    return false;

  /* move the first left_count left wings */
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wts->left_wing, i);
    if( !KhePartMove(p, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  /* move the first right_count right wings */
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wts->right_wing, i);
    if( !KhePartMove(p, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetMove(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int left_count,
  int right_count, int *from_r_durn_change, int *to_r_durn_change,
  KHE_MTASK_FINDER mtf)
{
  int i;  KHE_MPART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheMTaskClearOptimal(wmts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wmts->from_r ||
      !KheMTaskPrepareResource(wmts, to_r, mtf) )
    return false;

  /* check that left_count and right_count are in range */
  HnAssert(0 <= left_count && left_count <= wmts->to_r_max_left_wing_count,
    "KheWidenedMTaskSetMove: left_count (%d) out of range (0 .. %d)",
    left_count, wmts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wmts->to_r_max_right_wing_count,
    "KheWidenedMTaskSetMove: right_count (%d) out of range (0 .. %d)",
    right_count, wmts->to_r_max_right_wing_count);

  /* move the core */
  if( !KheMPartMove(&wmts->core, from_r, to_r, from_r_durn_change,
	to_r_durn_change) )
    return false;

  /* move the first left_count left wings */
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartMove(p, from_r, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  /* move the first right_count right wings */
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( !KheMPartMove(p, from_r, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetToResourceDebug(KHE_WIDENED_TASK_SET wts,          */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the to_r parts of widened task set wts (but only the      */
/*  first left_count tasks of the left wing, and only the first right_count  */
/*  tasks of the right wing) onto fp with the given verbosity and indent.    */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetToResourceDebug(KHE_WIDENED_TASK_SET wts,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  KHE_PART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( to_r == wts->from_r )
    fprintf(fp, "failed (from_r == to_r)");
  else if ( to_r != wts->to_r )
    fprintf(fp, "failed (not prepared for %s)", KheResourceShow(to_r));
  else
  {
    fprintf(fp, "{");
    if( left_count > HaArrayCount(wts->left_wing) )
      left_count = HaArrayCount(wts->left_wing);
    if( right_count > HaArrayCount(wts->right_wing) )
      right_count = HaArrayCount(wts->right_wing);
    for( i = left_count - 1;  i >= 0;  i-- )
    {
      p = HaArray(wts->left_wing, i);
      if( i < left_count - 1 )
	fprintf(fp, ", ");
      if( KheTaskSetTaskCount(p->to.ts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheTaskSetDoDebug(p->to.ts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    if( left_count > 0 || right_count > 0 )
    {
      if( left_count > 0 )
	fprintf(fp, " ");
      fprintf(fp, "[");
    }
    KheTaskSetDoDebug(wts->core.to.ts, 0, -1, verbosity, fp);
    if( left_count > 0 || right_count > 0 )
    {
      fprintf(fp, "]");
      if( right_count > 0 )
	fprintf(fp, " ");
    }
    for( i = 0;  i < right_count;  i++ )
    {
      p = HaArray(wts->right_wing, i);
      if( i > 0 )
	fprintf(fp, ", ");
      if( KheTaskSetTaskCount(p->to.ts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheTaskSetDoDebug(p->to.ts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    fprintf(fp, "}");
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetToResourceDebug(KHE_WIDENED_TASK_SET wts,          */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the to_r parts of widened task set wts (but only the      */
/*  first left_count tasks of the left wing, and only the first right_count  */
/*  tasks of the right wing) onto fp with the given verbosity and indent.    */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetToResourceDebug(KHE_WIDENED_MTASK_SET wmts,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  KHE_MPART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( to_r == wmts->from_r )
    fprintf(fp, "failed (from_r == to_r)");
  else if ( to_r != wmts->to_r )
    fprintf(fp, "failed (not prepared for %s)", KheResourceShow(to_r));
  else
  {
    fprintf(fp, "{");
    if( left_count > HaArrayCount(wmts->left_wing) )
      left_count = HaArrayCount(wmts->left_wing);
    if( right_count > HaArrayCount(wmts->right_wing) )
      right_count = HaArrayCount(wmts->right_wing);
    for( i = left_count - 1;  i >= 0;  i-- )
    {
      p = HaArray(wmts->left_wing, i);
      if( i < left_count - 1 )
	fprintf(fp, ", ");
      if( KheMTaskSetMTaskCount(p->to.mts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheMTaskSetDoDebug(p->to.mts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    if( left_count > 0 || right_count > 0 )
    {
      if( left_count > 0 )
	fprintf(fp, " ");
      fprintf(fp, "[");
    }
    KheMTaskSetDoDebug(wmts->core.to.mts, 0, -1, verbosity, fp);
    if( left_count > 0 || right_count > 0 )
    {
      fprintf(fp, "]");
      if( right_count > 0 )
	fprintf(fp, " ");
    }
    for( i = 0;  i < right_count;  i++ )
    {
      p = HaArray(wmts->right_wing, i);
      if( i > 0 )
	fprintf(fp, ", ");
      if( KheMTaskSetMTaskCount(p->to.mts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheMTaskSetDoDebug(p->to.mts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    fprintf(fp, "}");
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetOpDebug(KHE_WIDENED_TASK_SET wts, char *op,        */
/*    KHE_RESOURCE to_r, int left_count, int right_count, int first_index,   */
/*    int last_index, int verbosity, int indent, FILE *fp)                   */
/*                                                                           */
/*  Debug print of an operation.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedTaskSetOpDebug(KHE_WIDENED_TASK_SET wts, char *op,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int first_index, int last_index, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s ", KheResourceShow(wts->from_r));
  KheWidenedTaskSetDoDebug(wts, opt, left_count, right_count, first_index,
    last_index, verbosity, indent, fp);
  fprintf(fp, " %s ", op);
  fprintf(fp, "%s ", KheResourceShow(to_r));
  KheWidenedTaskSetToResourceDebug(wts, opt, to_r, left_count, right_count,
    verbosity, indent, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetOpDebug(KHE_WIDENED_TASK_SET wts, char *op,        */
/*    KHE_RESOURCE to_r, int left_count, int right_count, int first_index,   */
/*    int last_index, int verbosity, int indent, FILE *fp)                   */
/*                                                                           */
/*  Debug print of an operation.                                             */
/*                                                                           */
/*****************************************************************************/

static void KheWidenedMTaskSetOpDebug(KHE_WIDENED_MTASK_SET wmts, char *op,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int first_index, int last_index, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s ", KheResourceShow(wmts->from_r));
  KheWidenedMTaskSetDoDebug(wmts, opt, left_count, right_count, first_index,
    last_index, verbosity, indent, fp);
  fprintf(fp, " %s ", op);
  fprintf(fp, "%s ", KheResourceShow(to_r));
  KheWidenedMTaskSetToResourceDebug(wmts, opt, to_r, left_count, right_count,
    verbosity, indent, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetMoveDebug(KHE_WIDENED_TASK_SET wts,                */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a move.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetMoveDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheTaskSetTaskCount(wts->core.from.ts);
  KheWidenedTaskSetOpDebug(wts, "--->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetMoveDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "--->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMovePartial(KHE_WIDENED_TASK_SET wts,              */
/*    KHE_RESOURCE to_r, int first_index, int last_index)                    */
/*                                                                           */
/*  Move core[first_index .. last_index] to to_r.                            */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetMovePartial(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int first_index, int last_index)
{
  int count;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheClearOptimal(wts);
  if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
    return false;

  /* check that first_index and last_index are in range */
  count = KheTaskSetTaskCount(wts->core.from.ts);
  HnAssert(0 <= first_index && first_index < count,
    "KheWidenedTaskSetPartMove: first_index (%d) out of range (0 .. %d)",
    first_index, count - 1);
  HnAssert(0 <= last_index && last_index < count,
    "KheWidenedTaskSetPartMove: last_index (%d) out of range (0 .. %d)",
    last_index, count - 1);
  HnAssert(first_index <= last_index,
    "KheWidenedTaskSetPartMove: first_index (%d) greater than last_index (%d)",
    first_index, last_index);

  /* move the core, in part */
  if( !KhePartMovePart(&wts->core, first_index, last_index, to_r) )
    return false;

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetMovePartial(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int first_index, int last_index,
  KHE_MTASK_FINDER mtf)
{
  int count;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r ||
      !KheMTaskPrepareResource(wmts, to_r, mtf) )
    return false;

  /* check that first_index and last_index are in range */
  count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  HnAssert(0 <= first_index && first_index < count,
    "KheWidenedMTaskSetMovePartial: first_index (%d) out of range (0 .. %d)",
    first_index, count - 1);
  HnAssert(0 <= last_index && last_index < count,
    "KheWidenedMTaskSetMovePartial: last_index (%d) out of range (0 .. %d)",
    last_index, count - 1);
  HnAssert(first_index <= last_index,
    "KheWidenedMTaskSetMovePartial: first_index (%d) greater than last_index "
    "(%d)", first_index, last_index);

  /* move the core, in part */
  if( !KheMPartMovePart(&wmts->core, first_index, last_index, from_r, to_r) )
    return false;

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetMovePartialDebug(KHE_WIDENED_TASK_SET wts,         */
/*    KHE_RESOURCE to_r, int first_index, int last_index,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a partial move.                                           */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetMovePartialDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KheWidenedTaskSetOpDebug(wts, "-:->", false, to_r, 0, 0,
    first_index, last_index, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetMovePartialDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KheWidenedMTaskSetOpDebug(wmts, "-:->", false, to_r, 0, 0,
    first_index, last_index, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetFindInitial(KHE_WIDENED_TASK_SET wts,              */
/*    int wanted_durn, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Seach for an initial sequence of wts's core tasks whose total            */
/*  duration is wanted_durn.  Set *first_index and *last_index to the        */
/*  indexes of th first and last tasks in the sequence, and return true      */
/*  true when their total duration equals wanted_durn.                       */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetFindInitial(KHE_WIDENED_TASK_SET wts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i;  KHE_TASK task;  KHE_TASK_SET ts;
  durn = 0;
  ts = wts->core.from.ts;
  for( i = 0;  i < KheTaskSetTaskCount(ts) && durn < wanted_durn;  i++ )
  {
    task = KheTaskSetTask(ts, i);
    durn += KheIntervalLength(KheTaskInterval(wts->tf, task));
  }
  *first_index = 0;
  *last_index = i - 1;
  return durn == wanted_durn;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetFindInitial(KHE_WIDENED_MTASK_SET wmts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i;  KHE_MTASK mt;  KHE_MTASK_SET ms;
  durn = 0;
  ms = wmts->core.from.mts;
  for( i = 0;  i < KheMTaskSetMTaskCount(ms) && durn < wanted_durn;  i++ )
  {
    mt = KheMTaskSetMTask(ms, i);
    durn += KheIntervalLength(KheMTaskInterval(wmts->tf, mt));
  }
  *first_index = 0;
  *last_index = i - 1;
  return durn == wanted_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetFindFinal(KHE_WIDENED_TASK_SET wts,                */
/*    int wanted_durn, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Like KheWidenedTaskSetFindInitial only searching for final tasks.        */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetFindFinal(KHE_WIDENED_TASK_SET wts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i, count;  KHE_TASK task;  KHE_TASK_SET ts;
  durn = 0;
  ts = wts->core.from.ts;
  count = KheTaskSetTaskCount(ts);
  for( i = count - 1;  i >= 0 && durn < wanted_durn;  i-- )
  {
    task = KheTaskSetTask(ts, i);
    durn += KheIntervalLength(KheTaskInterval(wts->tf, task));
  }
  *first_index = i + 1;
  *last_index = count - 1;
  return durn == wanted_durn;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetFindFinal(KHE_WIDENED_MTASK_SET wmts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i, count;  KHE_MTASK mt;  KHE_MTASK_SET ms;
  durn = 0;
  ms = wmts->core.from.mts;
  count = KheMTaskSetMTaskCount(ms);
  for( i = count - 1;  i >= 0 && durn < wanted_durn;  i-- )
  {
    mt = KheMTaskSetMTask(ms, i);
    durn += KheIntervalLength(KheMTaskInterval(wmts->tf, mt));
  }
  *first_index = i + 1;
  *last_index = count - 1;
  return durn == wanted_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened task set swaps"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetSwapCheck(KHE_WIDENED_TASK_SET wts,                */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count)     */
/*                                                                           */
/*  Check whether wts is swappable with to_r.                                */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetSwapCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count)
{
  int i, j;  KHE_PART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheClearOptimal(wts);
  if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedTaskSetSwapCheck(%s) returning false (prep)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  /* check core swappable */
  if( !KhePartCoreSwapCheck(&wts->core, wts->from_r, to_r, exact,
	blocking_tg, blocking_m) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedTaskSetSwapCheck(%s) returning false (core)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  /* find swappable left wings */
  for( i = 0;  i < wts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wts->left_wing, i);
    if( !KhePartWingSwapCheck(p, wts->from_r, to_r, exact) )
      break;
  }

  /* find swappable right wings */
  for( j = 0;  j < wts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wts->right_wing, j);
    if( !KhePartWingSwapCheck(p, wts->from_r, to_r, exact) )
      break;
  }

  /* all good, set *max_left_count and *max_right_count and return */
  if( DEBUG2 )
    fprintf(stderr, "KheWidenedTaskSetSwapCheck(%s) returning true (%d, %d)\n",
      KheResourceShow(to_r), i, j);
  return *max_left_count = i, *max_right_count = j, true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetSwapCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count,
  KHE_MTASK_FINDER mtf)
{
  int i, j;  KHE_MPART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r ||
      !KheMTaskPrepareResource(wmts, to_r, mtf) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) ret false (prep)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  /* check core swappable */
  if( !KheMPartCoreSwapCheck(&wmts->core, wmts->from_r, to_r, exact,
	blocking_tg, blocking_m) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) ret false (core)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  /* find swappable left wings */
  for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartWingSwapCheck(p, wmts->from_r, to_r, exact) )
      break;
  }

  /* find swappable right wings */
  for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wmts->right_wing, j);
    if( !KheMPartWingSwapCheck(p, wmts->from_r, to_r, exact) )
      break;
  }

  /* all good, set *max_left_count and *max_right_count and return */
  if( DEBUG2 )
    fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) returning true (%d, %d)\n",
      KheResourceShow(to_r), i, j);
  return *max_left_count = i, *max_right_count = j, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetSwap(KHE_WIDENED_TASK_SET wts,                     */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Swap wts to to_r.                                                        */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetSwap(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int i;  KHE_PART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheClearOptimal(wts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
    return false;

  /* check that left_count and right_count are in range */
  HnAssert(0 <= left_count && left_count <= wts->to_r_max_left_wing_count,
    "KheWidenedTaskSetSwap: left_count (%d) out of range (0 .. %d)",
    left_count, wts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wts->to_r_max_right_wing_count,
    "KheWidenedTaskSetSwap: right_count (%d) out of range (0 .. %d)",
    right_count, wts->to_r_max_right_wing_count);

  /* swap the core */
  if( !KhePartSwap(&wts->core, wts->from_r, to_r,
	from_r_durn_change, to_r_durn_change) )
    return false;

  /* swap the first left_count left wings */
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wts->left_wing, i);
    if( !KhePartSwap(p, wts->from_r, to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  /* swap the first right_count right wings */
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wts->right_wing, i);
    if( !KhePartSwap(p, wts->from_r, to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetSwap(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change,
  KHE_MTASK_FINDER mtf)
{
  int i;  KHE_MPART p;

  /* give up now if to_r == from_r or can't prepare to_r */
  KheMTaskClearOptimal(wmts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wmts->from_r ||
      !KheMTaskPrepareResource(wmts, to_r, mtf) )
    return false;

  /* check that left_count and right_count are in range */
  HnAssert(0 <= left_count && left_count <= wmts->to_r_max_left_wing_count,
    "KheWidenedMTaskSetSwap: left_count (%d) out of range (0 .. %d)",
    left_count, wmts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wmts->to_r_max_right_wing_count,
    "KheWidenedMTaskSetSwap: right_count (%d) out of range (0 .. %d)",
    right_count, wmts->to_r_max_right_wing_count);

  /* swap the core */
  if( !KheMPartSwap(&wmts->core, wmts->from_r, to_r,
	from_r_durn_change, to_r_durn_change) )
    return false;

  /* swap the first left_count left wings */
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartSwap(p, wmts->from_r, to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  /* swap the first right_count right wings */
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( !KheMPartSwap(p, wmts->from_r, to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetSwapDebug(KHE_WIDENED_TASK_SET wts,                */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a swap.                                                   */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetSwapDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheTaskSetTaskCount(wts->core.from.ts);
  KheWidenedTaskSetOpDebug(wts, "<-->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetSwapDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "<-->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened task sets - optimal moves"                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NewBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,               */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  Return true if curr_cost and curr_defects represent a new best.          */
/*                                                                           */
/*****************************************************************************/

static bool NewBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,
  int curr_defects)
{
  return curr_cost < wts->best_cost ||
    (curr_cost == wts->best_cost && curr_defects < wts->best_defects);
}


/*****************************************************************************/
/*                                                                           */
/*  bool NewBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,               */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  Return true if curr_cost and curr_defects represent a new best.          */
/*                                                                           */
/*****************************************************************************/

static bool NewMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,
  int curr_defects)
{
  return curr_cost < wmts->best_cost ||
    (curr_cost == wmts->best_cost && curr_defects < wmts->best_defects);
}


/*****************************************************************************/
/*                                                                           */
/*  void SetBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,               */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  The current state is a new best.  Remember it.                           */
/*                                                                           */
/*****************************************************************************/

static void SetBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,
  int curr_defects)
{
  int i;  KHE_PART p;

  /* new best wts attributes */
  wts->best_cost = curr_cost;
  wts->best_defects = curr_defects;

  /* new best core and wings attributes */
  wts->core.best_is_swap = wts->core.curr_is_swap;
  HaArrayForEach(wts->swappable_wings, p, i)
    p->best_is_swap = p->curr_is_swap;
}


/*****************************************************************************/
/*                                                                           */
/*  void SetBest(KHE_WIDENED_TASK_SET wts, KHE_COST curr_cost,               */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  The current state is a new best.  Remember it.                           */
/*                                                                           */
/*****************************************************************************/

static void SetMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,
  int curr_defects)
{
  int i;  KHE_MPART p;

  /* new best wts attributes */
  wmts->best_cost = curr_cost;
  wmts->best_defects = curr_defects;

  /* new best core and wings attributes */
  wmts->core.best_is_swap = wmts->core.curr_is_swap;
  HaArrayForEach(wmts->swappable_wings, p, i)
    p->best_is_swap = p->curr_is_swap;
}


/*****************************************************************************/
/*                                                                           */
/*  void RedoBest(KHE_WIDENED_TASK_SET wts, int *from_r_durn_change,         */
/*    int *to_r_durn_change)                                                 */
/*                                                                           */
/*  Redo the previously discovered best.                                     */
/*                                                                           */
/*****************************************************************************/

static void RedoBest(KHE_WIDENED_TASK_SET wts, int *from_r_durn_change,
  int *to_r_durn_change)
{
  int i;  KHE_PART p;

  /* redo the core */
  *from_r_durn_change = *to_r_durn_change = 0;
  if( wts->core.best_is_swap )
    KhePartSwap(&wts->core, wts->from_r, wts->to_r, from_r_durn_change,
      to_r_durn_change);
  else
    KhePartMove(&wts->core, wts->to_r, from_r_durn_change, to_r_durn_change);

  /* redo the wings */
  HaArrayForEach(wts->swappable_wings, p, i)
    if( p->best_is_swap )
      KhePartSwap(p, wts->from_r, wts->to_r, from_r_durn_change,
	  to_r_durn_change);
}


/*****************************************************************************/
/*                                                                           */
/*  void RedoBest(KHE_WIDENED_TASK_SET wts, int *from_r_durn_change,         */
/*    int *to_r_durn_change)                                                 */
/*                                                                           */
/*  Redo the previously discovered best.                                     */
/*                                                                           */
/*****************************************************************************/

static void RedoMTaskBest(KHE_WIDENED_MTASK_SET wmts, int *from_r_durn_change,
  int *to_r_durn_change)
{
  int i;  KHE_MPART p;

  /* redo the core */
  *from_r_durn_change = *to_r_durn_change = 0;
  if( wmts->core.best_is_swap )
    KheMPartSwap(&wmts->core, wmts->from_r, wmts->to_r, from_r_durn_change,
      to_r_durn_change);
  else
    KheMPartMove(&wmts->core, wmts->from_r /* ? still to do */,
      wmts->to_r, from_r_durn_change, to_r_durn_change);

  /* redo the wings */
  HaArrayForEach(wmts->swappable_wings, p, i)
    if( p->best_is_swap )
      KheMPartSwap(p, wmts->from_r, wmts->to_r, from_r_durn_change,
	  to_r_durn_change);
}


/*****************************************************************************/
/*                                                                           */
/*  void DoWingSwaps(KHE_WIDENED_TASK_SET wts, int index)                    */
/*                                                                           */
/*  Do wing swaps from index onwards.  Leave the solution as it was found.   */
/*                                                                           */
/*****************************************************************************/

static void DoWingSwaps(KHE_WIDENED_TASK_SET wts, int index)
{
  KHE_COST curr_cost;  int curr_defects, junk1, junk2;
  KHE_MARK mark;  KHE_PART p;  KHE_SOLN soln;
  soln = wts->tf->soln;
  if( index >= HaArrayCount(wts->swappable_wings) )
  {
    /* base of recursion, check for new best and record if found */
    curr_cost = KheSolnCost(soln);
    curr_defects = KheGroupMonitorDefectCount((KHE_GROUP_MONITOR) soln);
    if( NewBest(wts, curr_cost, curr_defects) )
      SetBest(wts, curr_cost, curr_defects);
  }
  else
  {
    /* try without swapping at index */
    p = HaArray(wts->swappable_wings, index);
    p->curr_is_swap = false;
    DoWingSwaps(wts, index + 1);

    /* try with swapping at index, if p swaps (which it should) */
    p->curr_is_swap = true;
    mark = KheMarkBegin(soln);
    junk1 = junk2 = 0;
    if( KhePartSwap(p, wts->from_r, wts->to_r, &junk1, &junk2) )
      DoWingSwaps(wts, index + 1);
    KheMarkEnd(mark, true);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void DoWingSwaps(KHE_WIDENED_TASK_SET wts, int index)                    */
/*                                                                           */
/*  Do wing swaps from index onwards.  Leave the solution as it was found.   */
/*                                                                           */
/*****************************************************************************/

static void DoMTaskWingSwaps(KHE_WIDENED_MTASK_SET wmts, int index)
{
  KHE_COST curr_cost;  int curr_defects, junk1, junk2;
  KHE_MARK mark;  KHE_MPART p;  KHE_SOLN soln;
  soln = wmts->tf->soln;
  if( index >= HaArrayCount(wmts->swappable_wings) )
  {
    /* base of recursion, check for new best and record if found */
    curr_cost = KheSolnCost(soln);
    curr_defects = KheGroupMonitorDefectCount((KHE_GROUP_MONITOR) soln);
    if( NewMTaskBest(wmts, curr_cost, curr_defects) )
      SetMTaskBest(wmts, curr_cost, curr_defects);
  }
  else
  {
    /* try without swapping at index */
    p = HaArray(wmts->swappable_wings, index);
    p->curr_is_swap = false;
    DoMTaskWingSwaps(wmts, index + 1);

    /* try with swapping at index, if p swaps (which it should) */
    p->curr_is_swap = true;
    mark = KheMarkBegin(soln);
    junk1 = junk2 = 0;
    if( KheMPartSwap(p, wmts->from_r, wmts->to_r, &junk1, &junk2) )
      DoMTaskWingSwaps(wmts, index + 1);
    KheMarkEnd(mark, true);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalMoveCheck(KHE_WIDENED_TASK_SET wts,         */
/*    KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m) */
/*                                                                           */
/*  Check that an optimal move of wts to to_r is possible.                   */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetOptimalMoveCheck(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m)
{
  /* solve from scratch if to_r has changed or no optimal result present */
  if( wts->to_r != to_r || !KheHaveOptimal(wts) )
  {
    /* prepare for to_r; return false if it doesn't work */
    if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
      return false;

    /* try one of the two possibilities for the core */
    if( wts->core.to.effectively_free )
    {
      /* check moving the core; it must be movable, else fail */
      if( !KhePartMoveCheck(&wts->core, false, to_r) )
	return false;
    }
    else
    {
      /* check swapping the core; it must be swappable, else fail */
      if( !KhePartCoreSwapCheck(&wts->core, wts->from_r, to_r, false,
	    blocking_tg, blocking_m) )
	return false;
    }
  }

  /* no problems, optimal move will work */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetOptimalMoveCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, KHE_MTASK_FINDER mtf)
{
  /* solve from scratch if to_r has changed or no optimal result present */
  if( wmts->to_r != to_r || !KheMTaskHaveOptimal(wmts) )
  {
    /* prepare for to_r; return false if it doesn't work */
    if( to_r == wmts->from_r ||
	!KheMTaskPrepareResource(wmts, to_r, mtf) )
      return false;

    /* try one of the two possibilities for the core */
    if( wmts->core.to.effectively_free )
    {
      /* check moving the core; it must be movable, else fail */
      if( !KheMPartMoveCheck(&wmts->core, false, from_r, to_r) )
	return false;
    }
    else
    {
      /* check swapping the core; it must be swappable, else fail */
      if( !KheMPartCoreSwapCheck(&wmts->core, wmts->from_r, to_r, false,
	    blocking_tg, blocking_m) )
	return false;
    }
  }

  /* no problems, optimal move will work */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalMove(KHE_WIDENED_TASK_SET wts,              */
/*    KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change)     */
/*                                                                           */
/*  Make an optimal move of wts to to_r.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheWidenedTaskSetOptimalMove(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change)
{
  int i, j, junk1, junk2;  KHE_PART p;

  /* solve from scratch if to_r has changed or no optimal result present */
  if( wts->to_r != to_r || !KheHaveOptimal(wts) )
  {
    /* initialize solve fields */
    wts->best_cost = KheCost(INT_MAX, INT_MAX);
    wts->best_defects = INT_MAX;

    /* prepare for to_r; return false if it doesn't work */
    *from_r_durn_change = *to_r_durn_change = 0;
    if( to_r == wts->from_r || !KhePrepareResource(wts, to_r) )
      return false;

    /* Set wts->swappable_wings to all swappable left and right wings */
    HaArrayClear(wts->swappable_wings);
    for( i = 0;  i < wts->to_r_max_left_wing_count;  i++ )
    {
      p = HaArray(wts->left_wing, i);
      if( KhePartWingSwapCheck(p, wts->from_r, to_r, false) )
	HaArrayAddLast(wts->swappable_wings, p);
    }
    for( j = 0;  j < wts->to_r_max_right_wing_count;  j++ )
    {
      p = HaArray(wts->right_wing, j);
      if( KhePartWingSwapCheck(p, wts->from_r, to_r, false) )
	HaArrayAddLast(wts->swappable_wings, p);
    }

    /* try one of the two possibilities for the core */
    if( wts->core.to.effectively_free )
    {
      /* more the core; it must be movable, else fail */
      if( !KhePartMove(&wts->core, to_r, &junk1, &junk2) )
	return false;
      wts->core.curr_is_swap = false;
    }
    else
    {
      /* swap the core; it must be swappable, else fail */
      if( !KhePartSwap(&wts->core, wts->from_r, to_r, &junk1, &junk2) )
	return false;
      wts->core.curr_is_swap = true;
    }

    /* recursively try all swaps and non-swaps in the wings */
    DoWingSwaps(wts, 0);
  }

  /* apply best solution, or do nothing if none */
  if( KheHaveOptimal(wts) )
  {
    /* there is a best solution; apply it and return */
    RedoBest(wts, from_r_durn_change, to_r_durn_change);
    return true;
  }
  else
  {
    /* no best solution, false */
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

bool KheWidenedMTaskSetOptimalMove(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int *from_r_durn_change,
  int *to_r_durn_change, KHE_MTASK_FINDER mtf)
{
  int i, j, junk1, junk2;  KHE_MPART p;

  /* solve from scratch if to_r has changed or no optimal result present */
  if( wmts->to_r != to_r || !KheMTaskHaveOptimal(wmts) )
  {
    /* initialize solve fields */
    wmts->best_cost = KheCost(INT_MAX, INT_MAX);
    wmts->best_defects = INT_MAX;

    /* prepare for to_r; return false if it doesn't work */
    *from_r_durn_change = *to_r_durn_change = 0;
    if( to_r == wmts->from_r ||
	!KheMTaskPrepareResource(wmts, to_r, mtf) )
      return false;

    /* Set wmts->swappable_wings to all swappable left and right wings */
    HaArrayClear(wmts->swappable_wings);
    for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
    {
      p = HaArray(wmts->left_wing, i);
      if( KheMPartWingSwapCheck(p, wmts->from_r, to_r, false) )
	HaArrayAddLast(wmts->swappable_wings, p);
    }
    for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
    {
      p = HaArray(wmts->right_wing, j);
      if( KheMPartWingSwapCheck(p, wmts->from_r, to_r, false) )
	HaArrayAddLast(wmts->swappable_wings, p);
    }

    /* try one of the two possibilities for the core */
    if( wmts->core.to.effectively_free )
    {
      /* more the core; it must be movable, else fail */
      if( !KheMPartMove(&wmts->core, from_r, to_r, &junk1, &junk2) )
	return false;
      wmts->core.curr_is_swap = false;
    }
    else
    {
      /* swap the core; it must be swappable, else fail */
      if( !KheMPartSwap(&wmts->core, wmts->from_r, to_r, &junk1, &junk2) )
	return false;
      wmts->core.curr_is_swap = true;
    }

    /* recursively try all swaps and non-swaps in the wings */
    DoMTaskWingSwaps(wmts, 0);
  }

  /* apply best solution, or do nothing if none */
  if( KheMTaskHaveOptimal(wmts) )
  {
    /* there is a best solution; apply it and return */
    RedoMTaskBest(wmts, from_r_durn_change, to_r_durn_change);
    return true;
  }
  else
  {
    /* no best solution, false */
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetOptimalMoveDebug(KHE_WIDENED_TASK_SET wts,         */
/*    KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp)                */
/*                                                                           */
/*  Debug print of an optimal move.                                          */
/*                                                                           */
/*****************************************************************************/

void KheWidenedTaskSetOptimalMoveDebug(KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp)
{
  int count = KheTaskSetTaskCount(wts->core.from.ts);
  KheWidenedTaskSetOpDebug(wts, "<-opt->", true, to_r,
    wts->to_r_max_left_wing_count, wts->to_r_max_right_wing_count,
    0, count - 1, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

void KheWidenedMTaskSetOptimalMoveDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "<-opt->", true, to_r,
    wmts->to_r_max_left_wing_count, wmts->to_r_max_right_wing_count,
    0, count - 1, verbosity, indent, fp);
}

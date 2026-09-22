
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
/*  FILE:         khe_sr_weekend_grouping.c                                  */
/*  DESCRIPTION:  Weekend grouping                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_mmatch.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0
#define DEBUG4	0	/* display final groups */
#define DEBUG6	0
#define DEBUG7	0
#define DEBUG8	0
#define DEBUG9	0
#define DEBUG10	0
#define DEBUG11	0
#define DEBUG12	0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_WEEKEND_TASK - a weekend task                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_weekend_day_rec *KHE_WEEKEND_DAY;

typedef struct khe_weekend_task_rec {
  KHE_WEEKEND_DAY			day;
  KHE_TASK				task;
  int					offset_in_day;
  KHE_COST				non_asst_cost;
  KHE_COST				asst_cost;
  KHE_MMATCH_NODE			match_node;
} *KHE_WEEKEND_TASK;

typedef HA_ARRAY(KHE_WEEKEND_TASK) ARRAY_KHE_WEEKEND_TASK;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_WEEKEND_DAY - a weekend day                                     */
/*                                                                           */
/*****************************************************************************/

struct khe_weekend_day_rec {
  KHE_TIME_GROUP			time_group;
  int					index_in_frame;
  bool					second_day;
  ARRAY_KHE_WEEKEND_TASK		required_tasks;
  ARRAY_KHE_WEEKEND_TASK		optional_tasks;
};

typedef HA_ARRAY(KHE_WEEKEND_DAY) ARRAY_KHE_WEEKEND_DAY;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_WEEKEND - a weekend with its constraint class and days          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_weekend_rec {
  KHE_CONSTRAINT_CLASS			class;
  KHE_WEEKEND_DAY			day1;
  KHE_WEEKEND_DAY			day2;
} *KHE_WEEKEND;

typedef HA_ARRAY(KHE_WEEKEND) ARRAY_KHE_WEEKEND;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_SOLVER - a solver for balancing complete weekends            */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(KHE_TIME_SET) ARRAY_KHE_TIME_SET;

typedef struct khe_weekend_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_TASK_GROUPER		task_grouper;
  KHE_SOLN_ADJUSTER		soln_adjuster;
  KHE_RESOURCE_TYPE		resource_type;
  /* KHE_BALANCE_SOLVER		balance_solver; */
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_FRAME			days_frame;
  KHE_CONSTRAINT_CLASS_FINDER	weekends_ccf;
  KHE_CONSTRAINT_CLASS_FINDER	consec_shifts_ccf;
  ARRAY_KHE_WEEKEND		weekends;
  KHE_MMATCH			match;
  ARRAY_KHE_TASK		scratch_tasks;
  ARRAY_KHE_TIME_SET		time_set_free_list;
} *KHE_WEEKEND_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WEEKEND_TASK"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTaskOffsetInDay(KHE_TASK task, KHE_FRAME frame)                   */
/*                                                                           */
/*  Return the offset of task in its day, or 0 if task has no offset.        */
/*                                                                           */
/*  Implementation note.  This does not do a good job on grouped tasks.      */
/*  But that may not matter very much.                                       */
/*                                                                           */
/*****************************************************************************/

static int KheTaskOffsetInDay(KHE_TASK task, KHE_FRAME frame)
{
  KHE_MEET meet;  KHE_TIME t;
  meet = KheTaskMeet(task);
  if( meet == NULL )
    return 0;
  t = KheMeetAsstTime(meet);
  if( t == NULL )
    return 0;
  return KheFrameTimeOffset(frame, t);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_TASK KheWeekendTaskMake(KHE_TASK task, HA_ARENA a)           */
/*                                                                           */
/*  Make a weekend task object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_TASK KheWeekendTaskMake(KHE_TASK task,
  KHE_WEEKEND_DAY day, KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_TASK res;
  HaMake(res, ws->arena);
  res->task = task;
  res->day = day;
  res->offset_in_day = KheTaskOffsetInDay(task, ws->days_frame);
  KheTaskNonAsstAndAsstCost(task, &res->non_asst_cost, &res->asst_cost);
  res->match_node = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWeekendTaskTriggersFixing(KHE_WEEKEND_TASK wtask,                */
/*    KHE_COST optional_cost, KHE_COST over_cost, KHE_COST constraint_cost)  */
/*                                                                           */
/*  Return true if the presence of required task wtask triggers fixing       */
/*  of the optional tasks on the other weekend day.                          */
/*                                                                           */
/*  For the derivation of this formula, see the Guide.  We've omitted the    */
/*  initial solution cost since it occurs in all three of c1, c2, and c3.    */
/*                                                                           */
/*****************************************************************************/

static bool KheWeekendTaskTriggersFixing(KHE_WEEKEND_TASK wtask,
  KHE_COST optional_cost, KHE_COST over_cost, KHE_COST constraint_cost)
{
  KHE_COST c1, c2, c3;

  /* if over_cost <= 0 we don't want any fixing */
  if( DEBUG9 )
    fprintf(stderr, "[ KheWeekendTaskTriggersFixing(wtask, opt %.5f, "
      "over %.5f, constraint %.5f)\n", KheCostShow(optional_cost),
      KheCostShow(over_cost), KheCostShow(constraint_cost));
  if( over_cost <= 0 )
  {
    if( DEBUG9 )
      fprintf(stderr, "] KheWeekendTaskTriggersFixing ret. false (over)\n");
    return false;
  }

  /* otherwise apply the formula from the Guide */
  c1 = wtask->asst_cost - wtask->non_asst_cost + over_cost + constraint_cost;
  c2 = 0;
  c3 = wtask->asst_cost - wtask->non_asst_cost + optional_cost + 2 * over_cost;
  if( DEBUG9 )
  {
    fprintf(stderr, "  c1 = %.5f, c2 = %.5f, c3 = %.5f\n",
      KheCostShow(c1), KheCostShow(c2), KheCostShow(c3));
    fprintf(stderr, "] KheWeekendTaskTriggersFixing returning %s || %s\n",
      bool_show(c3 > c1), bool_show(c3 > c2));
  }
  return c3 > c1 || c3 > c2;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendTaskDebug(KHE_WEEKEND_TASK wtask, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of wtask onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendTaskDebug(KHE_WEEKEND_TASK wtask, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheTaskDebug(wtask->task, verbosity, -1, fp);
  fprintf(fp, " [non_asst_cost %.5f, asst_cost %.5f]",
    KheCostShow(wtask->non_asst_cost), KheCostShow(wtask->asst_cost));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendTaskDebugFn(void *value, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Version of KheWeekendTaskDebug to pass to matching.                      */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendTaskDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KheWeekendTaskDebug((KHE_WEEKEND_TASK) value, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WEEKEND_DAY"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_DAY KheWeekendDayMake(KHE_TIME_GROUP tg, bool second_day,    */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Make a new weekend day object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_DAY KheWeekendDayMake(KHE_TIME_GROUP tg, bool second_day,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_DAY res;
  HaMake(res, ws->arena);
  res->time_group = tg;
  res->index_in_frame = KheFrameTimeIndex(ws->days_frame,
    KheTimeGroupTime(tg, 0));
  res->second_day = second_day;
  HaArrayInit(res->required_tasks, ws->arena);
  HaArrayInit(res->optional_tasks, ws->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayAddTask(KHE_WEEKEND_DAY wday, KHE_WEEKEND_TASK wtask)  */
/*                                                                           */
/*  Add wtask to wday.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayAddTask(KHE_WEEKEND_DAY wday, KHE_WEEKEND_TASK wtask)
{
  if( wtask->non_asst_cost > wtask->asst_cost )
    HaArrayAddLast(wday->required_tasks, wtask);
  else
    HaArrayAddLast(wday->optional_tasks, wtask);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)                      */
/*                                                                           */
/*  Typed comparison function for bringing tasks together.                   */
/*                                                                           */
/*****************************************************************************/

static int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Untyped comparison function for bringing tasks together.                 */
/*                                                                           */
/*****************************************************************************/

static int KheTaskCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  return KheTaskTypedCmp(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayAddTasks(KHE_WEEKEND_DAY wday, KHE_WEEKEND_SOLVER ws)  */
/*                                                                           */
/*  Add its tasks to wday.  These are all proper root tasks whose first      */
/*  or last day is wday.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayAddTasks(KHE_WEEKEND_DAY wday, KHE_WEEKEND_SOLVER ws)
{
  KHE_TIME t;  int i, j, k;  KHE_MEET meet;  KHE_TASK task;
  KHE_WEEKEND_TASK wtask;  KHE_INTERVAL in;

  /* add to ws->scratch_tasks the tasks of type rt that start/end on wday */
  HaArrayClear(ws->scratch_tasks);
  for( i = 0;  i < KheTimeGroupTimeCount(wday->time_group);  i++ )
  {
    t = KheTimeGroupTime(wday->time_group, i);
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(ws->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(ws->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	if( KheTaskResourceType(task) == ws->resource_type )
        {
	  /* task needs to end on the first day or start on the second day */
	  task = KheTaskProperRoot(task);
	  in = KheTaskInterval(task, ws->days_frame);
	  if( wday->second_day ? (in.first == wday->index_in_frame) :
	      (in.last == wday->index_in_frame) )
	    HaArrayAddLast(ws->scratch_tasks, task);
	}
      }
    }
  }

  /* uniqueify the tasks */
  HaArraySortUnique(ws->scratch_tasks, &KheTaskCmp);

  /* add one weekend task object for each task */
  HaArrayForEach(ws->scratch_tasks, task, i)
  {
    wtask = KheWeekendTaskMake(task, wday, ws);
    KheWeekendDayAddTask(wday, wtask);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWeekendDayDominates(KHE_WEEKEND_DAY wd1, KHE_WEEKEND_DAY wd2)    */
/*                                                                           */
/*  Return true if wd1 dominates wd2 (if it has more required tasks).        */
/*                                                                           */
/*****************************************************************************/

static bool KheWeekendDayDominates(KHE_WEEKEND_DAY wd1, KHE_WEEKEND_DAY wd2)
{
  return HaArrayCount(wd1->required_tasks) > HaArrayCount(wd2->required_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheWeekendDayMinOptionalAsstCost(KHE_WEEKEND_DAY wd)            */
/*                                                                           */
/*  Return the minimum, over all optional tasks t on wd, of the asst         */
/*  cost of t.  It is an error if there are no optional tasks.               */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheWeekendDayMinOptionalAsstCost(KHE_WEEKEND_DAY wd)
{
  KHE_COST res;  KHE_WEEKEND_TASK wtask;  int i;
  HnAssert(HaArrayCount(wd->optional_tasks) > 0,
    "KheWeekendDayMinOptionalAsstCost internal error");
  wtask = HaArrayFirst(wd->optional_tasks);
  res = wtask->asst_cost;
  for( i = 1;  i < HaArrayCount(wd->optional_tasks);  i++ )
  {
    wtask = HaArray(wd->optional_tasks, i);
    if( wtask->asst_cost < res )
      res = wtask->asst_cost;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayFixUnassignedOptionalTasks(KHE_WEEKEND_DAY wd,         */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Fix the assignments of the optional tasks of wd.                         */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayFixUnassignedOptionalTasks(KHE_WEEKEND_DAY wd,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_TASK wtask;  int i;
  if( DEBUG10 )
    fprintf(stderr, "[ KheWeekendDayFixUnassignedOptionalTasks(%s, ws)\n",
      KheTimeGroupId(wd->time_group));
  HaArrayForEach(wd->optional_tasks, wtask, i)
    if( KheTaskAsstResource(wtask->task) == NULL )
    {
      if( DEBUG10 )
	fprintf(stderr, "  fixing unassigned optional task %s\n",
	  KheTaskId(wtask->task));
      KheSolnAdjusterTaskAssignFix(ws->soln_adjuster, wtask->task);
    }
  if( DEBUG10 )
    fprintf(stderr, "] KheWeekendDayFixUnassignedOptionalTasks returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEdgeCost3(KHE_WEEKEND_SOLVER ws, KHE_WEEKEND_TASK wtask1,         */
/*    KHE_WEEKEND_TASK wtask2)                                               */
/*                                                                           */
/*  Return the third component of the edge cost.  This combines the c2       */
/*  and c3 of the documentation into a single integer.                       */
/*                                                                           */
/*****************************************************************************/

static int KheEdgeCost3(KHE_WEEKEND_SOLVER ws, KHE_WEEKEND_TASK wtask1,
  KHE_WEEKEND_TASK wtask2)
{
  int c2, c3;

  /* find c2, the difference in offset */
  c2 = wtask1->offset_in_day - wtask2->offset_in_day;
  /* ***
  c2 = KheTaskOffsetInDay(wtask1->task, ws->days_frame) -
	KheTaskOffsetInDay(wtask2->task, ws->days_frame);
  *** */
  if( c2 < 0 ) c2 = - c2;

  /* find c3, the size of the symmetric difference of the tasks' domains */
  c3 = KheResourceGroupSymmetricDifferenceCount(KheTaskDomain(wtask1->task),
    KheTaskDomain(wtask2->task));

  /* return a suitable combination of c2 and c3 */
  return c2 * KheResourceTypeResourceCount(ws->resource_type) + c3;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEdgeIsWanted(KHE_WEEKEND_SOLVER ws, KHE_WEEKEND_TASK wtask1,     */
/*    KHE_WEEKEND_TASK wtask2, int *c1, int *c2, int *c3)                    */
/*                                                                           */
/*  If an edge is wanted between wtask1 and wtask2, return true and set      */
/*  (*c1, *c2, *c3) to its cost.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheEdgeIsWanted(KHE_WEEKEND_SOLVER ws, KHE_WEEKEND_TASK wtask1,
  KHE_WEEKEND_TASK wtask2, int *c1, int *c2, int *c3)
{
  KHE_SOLN soln;  KHE_COST cost;

  /* return false if the task grouper disallows the edge */
  KheTaskGrouperClear(ws->task_grouper);
  KheTaskGrouperAddTask(ws->task_grouper, wtask1->task);
  if( !KheTaskGrouperAddTask(ws->task_grouper, wtask2->task) )
    return *c1 = *c2 = *c3 = 0, false;

  /* (c1, c2) is cost after assignment, c3 is difference in offset and domain */
  soln = KheTaskSoln(wtask1->task);
  cost = KheSolnCost(soln) + KheTaskGrouperCost(ws->task_grouper)
    + wtask1->asst_cost - wtask1->non_asst_cost
    + wtask2->asst_cost - wtask2->non_asst_cost;
  *c1 = KheHardCost(cost);
  *c2 = KheSoftCost(cost);
  *c3 = KheEdgeCost3(ws, wtask1, wtask2);
  /* ***
  *c3 = KheTaskOffsetInDay(wtask1->task, ws->days_frame) -
	KheTaskOffsetInDay(wtask2->task, ws->days_frame);
  if( *c3 < 0 ) *c3 = - *c3;
  *** */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SET KheTaskTimeSetMake(KHE_TASK task, KHE_WEEKEND_SOLVER ws)    */
/*                                                                           */
/*  Return a time set containing the times of task.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_SET KheTaskTimeSetMake(KHE_TASK task, KHE_WEEKEND_SOLVER ws)
{
  KHE_TIME_SET res;
  if( HaArrayCount(ws->time_set_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(ws->time_set_free_list);
    KheTimeSetClear(res);
  }
  else
    res = KheTimeSetMake(KheSolnInstance(ws->soln), ws->arena);
  KheTimeSetAddTaskTimes(res, task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskTimeSetDelete(KHE_TIME_SET ts, KHE_WEEKEND_SOLVER ws)        */
/*                                                                           */
/*  Delete a time set made by ws.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskTimeSetDelete(KHE_TIME_SET ts, KHE_WEEKEND_SOLVER ws)
{
  HaArrayAddLast(ws->time_set_free_list, ts);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassMatchesTimeSet(KHE_WEEKEND_SOLVER ws,             */
/*    KHE_CONSTRAINT_CLASS cc, KHE_TIME_SET ts, KHE_INTERVAL *in)            */
/*                                                                           */
/*  If ts matches cc, return true with *in set to its interval of days.      */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassMatchesTimeSet(KHE_WEEKEND_SOLVER ws,
  KHE_CONSTRAINT_CLASS cc, KHE_TIME_SET ts, KHE_INTERVAL *in)
{
  KHE_TIME time;  int i, index, pos;  KHE_POLARITY po;  KHE_TIME_GROUP tg;
  *in = KheIntervalMake(1, 0);
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
  {
    time = KheTimeSetTime(ts, i);
    index = KheFrameTimeIndex(ws->days_frame, time);
    tg = KheConstraintClassTimeGroup(cc, index, &po);
    if( !KheTimeGroupContains(tg, time, &pos) )
      return false;
    if( KheIntervalContains(*in, index) )
      return false;
    *in = KheIntervalUnion(*in, KheIntervalMake(index, index));
  }
  return KheIntervalLength(*in) == KheTimeSetTimeCount(ts);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWeekendConstraintMatches(KHE_WEEKEND_SOLVER ws,                  */
/*    KHE_WEEKEND_TASK wtask, KHE_CONSTRAINT_CLASS *cc, KHE_INTERVAL *in)    */
/*                                                                           */
/*  Find a constraint class that matches the times of wtask.  If found,      */
/*  return the class and wtask's interval of days.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheWeekendConstraintMatches(KHE_WEEKEND_SOLVER ws,
  KHE_WEEKEND_TASK wtask, KHE_CONSTRAINT_CLASS *cc, KHE_INTERVAL *in)
{
  int i;  KHE_TIME_SET ts;
  ts = KheTaskTimeSetMake(wtask->task, ws);
  for( i=0; i < KheConstraintClassFinderClassCount(ws->consec_shifts_ccf); i++ )
  {
    *cc = KheConstraintClassFinderClass(ws->consec_shifts_ccf, i);
    if( KheConstraintClassMatchesTimeSet(ws, *cc, ts, in) )
    {
      KheTaskTimeSetDelete(ts, ws);
      return true;
    }
  }
  KheTaskTimeSetDelete(ts, ws);
  return *cc = NULL, *in = KheIntervalMake(1, 0), false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheConstraintClassTimeOnDay(KHE_CONSTRAINT_CLASS cc, int index) */
/*                                                                           */
/*  Return the sole time monitored by cc on the day with the given index.    */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheConstraintClassTimeOnDay(KHE_CONSTRAINT_CLASS cc, int index)
{
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  HnAssert(0 <= index && index < KheConstraintClassTimeGroupCount(cc),
    "KheConstraintClassTimeOnDay internal error (index %d out of range)",index);
  tg = KheConstraintClassTimeGroup(cc, index, &po);
  HnAssert(KheTimeGroupTimeCount(tg) == 1,
    "KheConstraintClassTimeOnDay internal error (tg)");
  return KheTimeGroupTime(tg, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindBestTask(KHE_WEEKEND_SOLVER ws, KHE_TIME time,               */
/*    KHE_RESOURCE_GROUP init_domain, KHE_TASK *best_task)                   */
/*                                                                           */
/*  Find the best suitable task running at the given time with the           */
/*  given domain (or as close to that domain as possible).                   */
/*                                                                           */
/*****************************************************************************/

static bool KheFindBestTask(KHE_WEEKEND_SOLVER ws, KHE_TIME time,
  KHE_RESOURCE_GROUP init_domain, KHE_TASK *best_task)
{
  int i, j, domain_diff, best_domain_diff;  KHE_RESOURCE_GROUP task_domain;
  KHE_MEET meet;  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;

  best_domain_diff = INT_MAX;
  *best_task = NULL;
  for( i = 0; i < KheEventTimetableMonitorTimeMeetCount(ws->etm, time); i++ )
  {
    meet = KheEventTimetableMonitorTimeMeet(ws->etm, time, i);
    for( j = 0;  j < KheMeetTaskCount(meet);  j++ )
    {
      task = KheTaskProperRoot(KheMeetTask(meet, j));
      if( KheTaskResourceType(task) == ws->resource_type &&
	  KheTaskAsstResource(task) == NULL )
      {
	KheTaskNonAsstAndAsstCost(task, &non_asst_cost, &asst_cost);
	if( non_asst_cost > asst_cost &&
	    KheTaskGrouperAddTaskCheck(ws->task_grouper, task) )
	{
	  if( DEBUG12 )
	    fprintf(stderr, "    examining eligible task %s\n",
	      KheTaskId(task));
	  task_domain = KheTaskDomain(task);
	  domain_diff = KheResourceGroupSymmetricDifferenceCount(
	    task_domain, init_domain);
	  if( domain_diff < best_domain_diff )
	  {
	    *best_task = task;
	    best_domain_diff = domain_diff;
	    if( DEBUG12 )
	      fprintf(stderr, "    new best task %s (%s)\n",
		KheTaskId(task), KheResourceGroupId(task_domain));
	  }
	}
      }
    }
  }
  return (*best_task != NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendExtendBack(KHE_WEEKEND_TASK wtask1,                       */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Extend the current grouping backwards from wtask1 to satisfy a           */
/*  constraint on consecutive shifts of the same type.                       */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendExtendBack(KHE_WEEKEND_TASK wtask1,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_INTERVAL in;  KHE_CONSTRAINT_CLASS cc;  int min_limit;
  KHE_TIME time;  KHE_TASK best_task;  KHE_RESOURCE_GROUP init_domain;

  /* find a constraint class that covers the times of wtask1 */
  if( KheWeekendConstraintMatches(ws, wtask1, &cc, &in) )
  {
    if( DEBUG12 )
    {
      fprintf(stderr, "  wtask1 %s matches constraint class (interval %s):\n",
	KheTaskId(wtask1->task), KheIntervalShow(in, ws->days_frame));
      KheConstraintClassDebug(cc, 2, 2, stderr);
    }

    /* add tasks to cover min_limit */
    min_limit = KheConstraintClassMinimum(cc);
    init_domain = KheTaskDomain(wtask1->task);
    while( KheIntervalLength(in) < min_limit && KheIntervalFirst(in) > 0 )
    {
      /* find the time to cover next, and the best task at that time */
      time = KheConstraintClassTimeOnDay(cc, KheIntervalFirst(in) - 1);
      if( DEBUG12 )
	fprintf(stderr, "  examining predecessor time %s\n", KheTimeId(time));
      if( KheFindBestTask(ws, time, init_domain, &best_task) )
      {
	/* use this task */
	if( DEBUG12 )
	  fprintf(stderr, "    adding best task %s\n", KheTaskId(best_task));
	if( !KheTaskGrouperAddTask(ws->task_grouper, best_task) )
	  HnAbort("KheWeekendExtendBack internal error");
	in = KheIntervalUnion(in, KheTaskInterval(best_task, ws->days_frame));
      }
      else
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendExtendForward(KHE_WEEKEND_TASK wtask2,                    */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Extend the current grouping forwards from wtask2 to satisfy a           */
/*  constraint on consecutive shifts of the same type.                       */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendExtendForward(KHE_WEEKEND_TASK wtask2,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_INTERVAL in;  KHE_CONSTRAINT_CLASS cc;  int min_limit;
  KHE_TIME time;  KHE_TASK best_task;  KHE_RESOURCE_GROUP init_domain;

  /* find a constraint class that covers the times of wtask1 */
  if( KheWeekendConstraintMatches(ws, wtask2, &cc, &in) )
  {
    if( DEBUG12 )
    {
      fprintf(stderr, "  wtask2 %s matches constraint class (interval %s):\n",
	KheTaskId(wtask2->task), KheIntervalShow(in, ws->days_frame));
      KheConstraintClassDebug(cc, 2, 2, stderr);
    }

    /* add tasks to cover min_limit */
    min_limit = KheConstraintClassMinimum(cc);
    init_domain = KheTaskDomain(wtask2->task);
    while( KheIntervalLength(in) < min_limit &&
      KheIntervalLast(in) < KheConstraintClassTimeGroupCount(cc) - 1 )
    {
      /* find the time to cover next, and the best task at that time */
      time = KheConstraintClassTimeOnDay(cc, KheIntervalLast(in) + 1);
      if( DEBUG12 )
	fprintf(stderr, "  examining successor time %s\n", KheTimeId(time));
      if( KheFindBestTask(ws, time, init_domain, &best_task) )
      {
	/* use this task */
	if( DEBUG12 )
	  fprintf(stderr, "    adding best task %s\n", KheTaskId(best_task));
	if( !KheTaskGrouperAddTask(ws->task_grouper, best_task) )
	  HnAbort("KheWeekendExtendForward internal error");
	in = KheIntervalUnion(in, KheTaskInterval(best_task, ws->days_frame));
      }
      else
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverMakeGroup(KHE_WEEKEND_SOLVER ws,                    */
/*    KHE_WEEKEND_TASK wtask1, KHE_WEEKEND_TASK wtask2)                      */
/*                                                                           */
/*  Make a group from wtask1 and wtask2.  At a minimum this will include     */
/*  just these two tasks; but it they have different offsets in their        */
/*  days, more tasks may be added.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverMakeGroup(KHE_WEEKEND_SOLVER ws,
  KHE_WEEKEND_TASK wtask1, KHE_WEEKEND_TASK wtask2)
{
  KheTaskGrouperClear(ws->task_grouper);
  KheTaskGrouperAddTask(ws->task_grouper, wtask1->task);
  if( !KheTaskGrouperAddTask(ws->task_grouper, wtask2->task) )
    HnAbort("KheWeekendDayMatchAndFixRequiredTasks internal error");
  if( wtask1->offset_in_day != wtask2->offset_in_day )
  {
    if( DEBUG12 )
      fprintf(stderr, "  looking to extend %s and %s\n",
	KheTaskId(wtask1->task), KheTaskId(wtask2->task));
    if( wtask1->day->second_day )
    {
      KheWeekendExtendBack(wtask2, ws);
      KheWeekendExtendForward(wtask1, ws);
    }
    else
    {
      KheWeekendExtendBack(wtask1, ws);
      KheWeekendExtendForward(wtask2, ws);
    }
  }
  if( DEBUG4 )
  {
    fprintf(stderr, "  weekend grouping ");
    KheTaskGrouperDebug(ws->task_grouper, 1, 0, stderr);
  }
  KheTaskGrouperMakeGroup(ws->task_grouper, ws->soln_adjuster);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayMatchAndFixRequiredTasks(KHE_WEEKEND_DAY wd1,          */
/*    KHE_WEEKEND_DAY wd2, bool fix_optionals, KHE_WEEKEND_SOLVER ws)        */
/*                                                                           */
/*  Match the required tasks on these two days, then make groups from the    */
/*  matched pairs.  If fix_optionals is true, also fix any tasks from wd1    */
/*  that fail to match.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayMatchAndFixRequiredTasks(KHE_WEEKEND_DAY wd1,
  KHE_WEEKEND_DAY wd2, bool have_optionals, KHE_COST optional_cost,
  KHE_COST over_cost, KHE_COST constraint_cost, bool *fix_optionals,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_TASK wtask1, wtask2;  int i, j, mult, c1, c2, c3;
  KHE_MMATCH_NODE sn;

  /* make demand nodes from the required tasks of wd1 */
  HaArrayForEach(wd1->required_tasks, wtask1, i)
    wtask1->match_node = KheMMatchDemandNodeMake(ws->match, 1, (void *) wtask1);

  /* make supply nodes from the required tasks of wd2 */
  HaArrayForEach(wd2->required_tasks, wtask2, j)
    wtask2->match_node = KheMMatchSupplyNodeMake(ws->match, 1, (void *) wtask2);

  /* make edges */
  if( DEBUG6 )
    fprintf(stderr, "  [ graph (wd1 %d required, wd2 %d required):\n",
      HaArrayCount(wd1->required_tasks), HaArrayCount(wd2->required_tasks));
  HaArrayForEach(wd1->required_tasks, wtask1, i)
  {
    if( DEBUG6 )
      KheTaskDebug(wtask1->task, 3, 6, stderr);
    HaArrayForEach(wd2->required_tasks, wtask2, j)
      if( KheEdgeIsWanted(ws, wtask1, wtask2, &c1, &c2, &c3) )
      {
	KheMMatchAddEdge(wtask1->match_node, wtask2->match_node, 1, c1, c2, c3);
	if( DEBUG6 )
	{
	  fprintf(stderr, "        --(cost %.5f, offset %d, domain %d)-> ",
	    KheCostShow(KheCost(c1, c2)),
	    c3 / KheResourceTypeResourceCount(ws->resource_type),
	    c3 % KheResourceTypeResourceCount(ws->resource_type));
	  KheTaskDebug(wtask2->task, 3, 0, stderr);
	}
      }
  }
  if( DEBUG6 )
    fprintf(stderr, "  ]\n");

  /* solve */
  KheMMatchSolve(ws->match);

  /* group the matched demand nodes and check the unmatched demand nodes */
  *fix_optionals = false;
  if( DEBUG7 )
    fprintf(stderr, "[ KheWeekendDayMatchAndFixRequiredTasks:\n");
  HaArrayForEach(wd1->required_tasks, wtask1, i)
  {
    if( KheMMatchResultEdge(wtask1->match_node, &sn, &mult, &c1, &c2, &c3) )
    {
      /* wtask1 matched, so group it with the task it matched with */
      /* we made sure previously that the grouping works */
      wtask2 = (KHE_WEEKEND_TASK) KheMMatchSupplyNodeBack(sn);
      KheWeekendSolverMakeGroup(ws, wtask1, wtask2);
      /* *** moved to KheWeekendSolverMakeGroup
      KheTaskGrouperClear(ws->task_grouper);
      KheTaskGrouperAddTask(ws->task_grouper, wtask1->task);
      wtask2 = (KHE_WEEKEND_TASK) KheMMatchSupplyNodeBack(sn);
      if( !KheTaskGrouperAddTask(ws->task_grouper, wtask2->task) )
	HnAbort("KheWeekendDayMatchAndFixRequiredTasks internal error");
      KheTaskGrouperMakeGroup(ws->task_grouper, ws->soln_adjuster);
      if( DEBUG4 )
	fprintf(stderr, "  weekend grouping grouping %s and %s\n",
	  KheTaskId(wtask1->task), KheTaskId(wtask2->task));
      *** */
    }
    else if( have_optionals && !*fix_optionals &&
	KheWeekendTaskTriggersFixing(wtask1, optional_cost, over_cost,
	  constraint_cost) )
    {
      /* wtask1 did not match and it triggers fixing */
      *fix_optionals = true;
    }
  }

  /* clear the matching and return */
  KheMMatchClear(ws->match);
  HaArrayForEach(wd1->required_tasks, wtask1, i)
    wtask1->match_node = NULL;
  HaArrayForEach(wd2->required_tasks, wtask2, j)
    wtask2->match_node = NULL;
  if( DEBUG7 )
    fprintf(stderr, "] KheWeekendDayMatchAndFixRequiredTasks returning"
      " (*fix_optionals = %s)\n", bool_show(*fix_optionals));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayHeaderDebug(KHE_WEEKEND_DAY wday, FILE *fp)            */
/*                                                                           */
/*  Print the header of the debug print of wday onto fp.                     */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayHeaderDebug(KHE_WEEKEND_DAY wday, FILE *fp)
{
  fprintf(fp, "WeekendDay(%s, %d required, %d unrequired)",
    KheTimeGroupId(wday->time_group), HaArrayCount(wday->required_tasks),
    HaArrayCount(wday->optional_tasks));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayDebug(KHE_WEEKEND_DAY wday, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of wday onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayDebug(KHE_WEEKEND_DAY wday, int verbosity,
  int indent, FILE *fp)
{
  KHE_WEEKEND_TASK wtask;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheWeekendDayHeaderDebug(wday, fp);
    fprintf(fp, "\n");
    fprintf(fp, "%*s  required:\n", indent, "");
    HaArrayForEach(wday->required_tasks, wtask, i)
      KheWeekendTaskDebug(wtask, verbosity, indent + 4, fp);
    fprintf(fp, "%*s  unrequired:\n", indent, "");
    HaArrayForEach(wday->optional_tasks, wtask, i)
      KheWeekendTaskDebug(wtask, verbosity, indent + 4, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheWeekendDayHeaderDebug(wday, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WEEKEND"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND KheWeekendMake(KHE_CONSTRAINT_CLASS cc,                      */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Make a weekend object for constraint class cc.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND KheWeekendMake(KHE_CONSTRAINT_CLASS cc,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND res;  KHE_TIME_GROUP tg1, tg2, tmp_tg;
  KHE_POLARITY po;  KHE_TIME time1, time2;

  HaMake(res, ws->arena);
  res->class = cc;

  /* get the two time groups */
  tg1 = KheConstraintClassTimeGroup(cc, 0, &po);
  tg2 = KheConstraintClassTimeGroup(cc, 1, &po);

  /* sort the two time groups into chronological order */
  time1 = KheTimeGroupTime(tg1, 0);
  time2 = KheTimeGroupTime(tg2, 0);
  if( KheTimeIndex(time1) > KheTimeIndex(time2) )
    tmp_tg = tg1, tg1 = tg2, tg2 = tmp_tg;

  /* add the two days, but with no tasks initially */
  res->day1 = KheWeekendDayMake(tg1, false, ws);
  res->day2 = KheWeekendDayMake(tg2, true, ws);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassAddDays(KHE_CONSTRAINT_CLASS cc,                  */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Add the days to cc, including adding the tasks to the days.              */
/*                                                                           */
/*  Implementation note.  This could be done at any time, but we delay it    */
/*  until a call to KheConstraintClassCoversAllResources has verified that   */
/*  we need it.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendAddTasks(KHE_WEEKEND wk, KHE_WEEKEND_SOLVER ws)
{
  KheWeekendDayAddTasks(wk->day1, ws);
  KheWeekendDayAddTasks(wk->day2, ws);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDoSolve(KHE_WEEKEND wk, KHE_WEEKEND_DAY wd1,              */
/*    KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)                            */
/*                                                                           */
/*  Solve weekend (wd1, wd2), assuming that wd1 has more required tasks      */
/*  than wd2 has.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDoSolve(KHE_WEEKEND wk, KHE_WEEKEND_DAY wd1,
  KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)
{
  KHE_COST optional_cost, over_cost, constraint_cost;
  bool have_optionals, fix_optionals;
  int demand, supply;  KHE_COST task_cost, resource_cost;
  if( HaArrayCount(wd2->optional_tasks) > 0 )
  {
    optional_cost = KheWeekendDayMinOptionalAsstCost(wd2);
    have_optionals = true;
  }
  else
  {
    optional_cost = -1;
    have_optionals = false;
  }
  /* over_cost = KheBalanceSolverMarginalCost(ws->balance_solver); */
  KheResourceDemandExceedsSupply(ws->soln, ws->resource_type,
    &demand, &supply, &task_cost, &resource_cost);
  over_cost = resource_cost;
  constraint_cost = KheConstraintClassCombinedWeight(wk->class);
  KheWeekendDayMatchAndFixRequiredTasks(wd1, wd2, have_optionals,
    optional_cost, over_cost, constraint_cost, &fix_optionals, ws);
  if( fix_optionals )
    KheWeekendDayFixUnassignedOptionalTasks(wd2, ws);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolve(KHE_WEEKEND wk, KHE_WEEKEND_SOLVER ws)              */
/*                                                                           */
/*  Solve wk.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheWeekendDebug(KHE_WEEKEND wk, int verbosity, int indent,
  FILE *fp);

static void KheWeekendSolve(KHE_WEEKEND wk, KHE_WEEKEND_SOLVER ws)
{
  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheWeekendSolve(wk, ws)\n");
    KheWeekendDebug(wk, 2, 2, stderr);
  }
  if( KheWeekendDayDominates(wk->day1, wk->day2) )
    KheWeekendDoSolve(wk, wk->day1, wk->day2, ws);
  else
    KheWeekendDoSolve(wk, wk->day2, wk->day1, ws);
  if( DEBUG3 )
    fprintf(stderr, "] KheWeekendSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDebug(KHE_WEEKEND wk, int verbosity, int indent,          */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of wk onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDebug(KHE_WEEKEND wk, int verbosity, int indent,
  FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Weekend\n", indent, "");
    KheConstraintClassDebug(wk->class, verbosity, indent + 2, fp);
    KheWeekendDayDebug(wk->day1, verbosity, indent + 2, fp);
    KheWeekendDayDebug(wk->day2, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WEEKEND_SOLVER"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverDebugFn(void *value, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function which prints the solver, for passing to an mmatch.        */
/*                                                                           */
/*****************************************************************************/
static void KheWeekendSolverDebug(KHE_WEEKEND_SOLVER ws, int verbosity,
  int indent, FILE *fp);

static void KheWeekendSolverDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KheWeekendSolverDebug((KHE_WEEKEND_SOLVER) value, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)                    */
/*                                                                           */
/*  Cost debug function.                                                     */
/*                                                                           */
/*****************************************************************************/

static void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%.5f:%d", KheCostShow(KheCost(c1, c2)), c3);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_SOLVER KheWeekendSolverMake(KHE_SOLN soln,                   */
/*    KHE_SOLN_ADJUSTER sa, KHE_RESOURCE_TYPE rt,                            */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_FRAME frame, HA_ARENA a)          */
/*                                                                           */
/*  Make a weekend solver object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_SOLVER KheWeekendSolverMake(KHE_SOLN soln,
  KHE_SOLN_ADJUSTER sa, KHE_RESOURCE_TYPE rt,
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_FRAME frame,
  KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a)
{
  KHE_WEEKEND_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->task_grouper = KheTaskGrouperMake(soln, frame, tgdf, a);
  res->soln_adjuster = sa;
  res->resource_type = rt;
  /* res->balance_solver = KheBalanceSolverMake(soln, rt, frame, a); */
  res->etm = etm;
  res->days_frame = frame;
  res->weekends_ccf = KheConstraintClassFinderMake(rt, frame, a);
  res->consec_shifts_ccf = KheConstraintClassFinderMake(rt, frame, a);
  HaArrayInit(res->weekends, a);
  res->match = KheMMatchMake((void *) res, &KheWeekendSolverDebugFn,
    &KheWeekendTaskDebugFn, &KheWeekendTaskDebugFn, &KheCostDebugFn, a);
  HaArrayInit(res->scratch_tasks, a);
  HaArrayInit(res->time_set_free_list, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupIsDaySubset(KHE_TIME_GROUP tg, KHE_FRAME days_frame,    */
/*    int *day_index)                                                        */
/*                                                                           */
/*  If tg is a subset of one of the days of days_frame, set *day_index       */
/*  to the index of that day and return true.  Otherwise set *day_index      */
/*  to -1 and return false.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheTimeGroupIsDaySubset(KHE_TIME_GROUP tg, KHE_FRAME days_frame,
  int *day_index)
{
  KHE_TIME time;  int index;  KHE_TIME_GROUP frame_tg;

  ** get the first time of tg, or return false if none **
  if( KheTimeGroupTimeCount(tg) == 0 )
    return *day_index = -1, false;
  time = KheTimeGroupTime(tg, 0);

  ** get the index of the day containing time, and the time group there **
  index = KheFrameTimeIndex(days_frame, time);
  frame_tg = KheFrameTimeGroup(days_frame, index);

  ** return true or false depending on whether tg is a subset of frame_tg **
  if( KheTimeGroupSubset(tg, frame_tg) )
    return *day_index = index, true;
  else
    return *day_index = -1, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool DayIndexesAdjacent(int day_index1, int day_index2)                  */
/*                                                                           */
/*  Return true if day_index1 and day_index2 are adjacent.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool DayIndexesAdjacent(int day_index1, int day_index2)
{
  int diff;
  diff = day_index1 - day_index2;
  return diff == 1 || diff == -1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetIsCompleteWeekend(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,                    */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if cbtc plus offset defines a complete weekend constraint.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheConstraintPlusOffsetIsCompleteWeekend(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,
  KHE_FRAME days_frame)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;  int day_index1, day_index2;

  ** must have exactly two time groups, both positive **
  if( KheClusterBusyTimesConstraintTimeGroupCount(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllPositive(cbtc) )
    return false;

  ** must have min limit 2, max limit 2, and allow-zero flag true **
  if( KheClusterBusyTimesConstraintMinimum(cbtc) != 2 ||
      KheClusterBusyTimesConstraintMaximum(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllowZero(cbtc) )
    return false;

  ** the time groups must be subsets of adjacent days of the common frame **
  tg1 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 0, offset, &po1);
  tg2 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 1, offset, &po2);
  return KheTimeGroupIsDaySubset(tg1, days_frame, &day_index1) &&
    KheTimeGroupIsDaySubset(tg2, days_frame, &day_index2) &&
    DayIndexesAdjacent(day_index1, day_index2);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverAddConstraintClasses(KHE_WEEKEND_SOLVER ws)         */
/*                                                                           */
/*  Add constraint classes to ws.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sr_constraint_classes.c
static void KheWeekendSolverAddConstraintClasses(KHE_WEEKEND_SOLVER ws)
{
  KHE_INSTANCE ins;  int i, j, offset, count;  KHE_RESOURCE_TYPE rt;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  ins = KheSolnInstance(ws->soln);
  rt = ws->resource_type;
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > 0 )
    {
      cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
      if( KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc, rt) > 0 )
      {
	count = KheClusterBusyTimesConstraintAppliesToOffsetCount(cbtc);
	for( j = 0;  j < count;  j++ )
	{
	  offset = KheClusterBusyTimesConstraintAppliesToOffset(cbtc, j);
	  if( KheConstraintPlusOffsetIsCompleteWeekend(cbtc, offset,
		ws->days_frame) )
	  {
	    ** have complete weekends constraint plus offset, add to ws **
	    KheConstraintClassFinderAddConstraint(ws->weekends_ccf, c, offset);
	  }
	}
      }
    }
  }
  if( DEBUG8 )
    KheConstraintClassFinderDebug(ws->weekends_ccf, 2, 2, stderr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverAddWeekends(KHE_WEEKEND_SOLVER ws)                  */
/*                                                                           */
/*  Add weekend objects to ws, one for each constraint class that covers     */
/*  all resources.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverAddWeekends(KHE_WEEKEND_SOLVER ws)
{
  int i;  KHE_CONSTRAINT_CLASS cc;  KHE_WEEKEND wk;
  for( i = 0;  i < KheConstraintClassFinderClassCount(ws->weekends_ccf);  i++ )
  {
    cc = KheConstraintClassFinderClass(ws->weekends_ccf, i);
    if( KheConstraintClassCoversResourceType(cc) )
    {
      wk = KheWeekendMake(cc, ws);
      HaArrayAddLast(ws->weekends, wk);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverHeaderDebug(KHE_WEEKEND_SOLVER ws, FILE *fp)        */
/*                                                                           */
/*  Debug print of the header part of the debug of ws onto fp.               */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverHeaderDebug(KHE_WEEKEND_SOLVER ws, FILE *fp)
{
  fprintf(fp, "WeekendSolver(%s, %s)", KheInstanceId(KheSolnInstance(ws->soln)),
    KheResourceTypeId(ws->resource_type));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverDebug(KHE_WEEKEND_SOLVER ws, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of ws with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverDebug(KHE_WEEKEND_SOLVER ws, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheWeekendSolverHeaderDebug(ws, fp);
    fprintf(fp, "\n");
    KheConstraintClassFinderDebug(ws->weekends_ccf, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheWeekendSolverHeaderDebug(ws, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)                             */
/*                                                                           */
/*  Balance the weekends of sa's solution.                                   */
/*                                                                           */
/*****************************************************************************/

void KheWeekendGrouping(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_SOLN_ADJUSTER sa)
{
  HA_ARENA a;  KHE_FRAME frame;  KHE_WEEKEND wk;
  KHE_WEEKEND_SOLVER ws;  KHE_EVENT_TIMETABLE_MONITOR etm;
  int i, j, count, offset;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;

  /* make a weekend solver */
  if( DEBUG1 )
    fprintf(stderr, "[ KheWeekendGrouping(%s, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
  a = KheSolnArenaBegin(soln);
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  frame = KheOptionsFrame(options, "gs_common_frame", soln);
  ws = KheWeekendSolverMake(soln, sa, rt, etm, frame, tgdf, a);

  /* find the weekend constraint classes */
  KheConstraintClassFinderAddCompleteWeekendsConstraints(ws->weekends_ccf,
    false);
  /* KheWeekendSolverAddConstraintClasses(ws); */

  /* find the consecutive shifts constraint classes */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG &&
	KheConstraintCombinedWeight(c) > 0 )
    {
      laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
      if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic, rt) > 0 &&
	  KheLimitActiveIntervalsConstraintTimeGroupCount(laic) > 0 &&
	  KheLimitActiveIntervalsConstraintMinimum(laic) >= 2 &&
	  KheLimitActiveIntervalsConstraintAllPositive(laic) )
      {
	count = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(laic);
	for( j = 0;  j < count;  j++ )
	{
	  offset = KheLimitActiveIntervalsConstraintAppliesToOffset(laic, j);
	  if( KheConstraintTimeGroupsSubsetFrame(c, offset, ws->days_frame) &&
	      KheConstraintTimeGroupsAllSingletons(c) )
	    KheConstraintClassFinderAddConstraint(ws->consec_shifts_ccf,
	      c, offset);
	}
      }
    }
  }
  if( DEBUG11 )
  {
    fprintf(stderr, "  KheWeekendGrouping consec_shifts_ccf:\n");
    KheConstraintClassFinderDebug(ws->consec_shifts_ccf, 2, 2, stderr);
  }

  /* make a weekend for each constraint class that covers all resources */
  KheWeekendSolverAddWeekends(ws);

  /* do the job for each weekend */
  HaArrayForEach(ws->weekends, wk, i)
  {
    KheWeekendAddTasks(wk, ws);
    KheWeekendSolve(wk, ws);
  }
  if( DEBUG2 )
    KheWeekendSolverDebug(ws, 2, 2, stderr);

  /* clean up and exit */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheWeekendGrouping returning\n");
}

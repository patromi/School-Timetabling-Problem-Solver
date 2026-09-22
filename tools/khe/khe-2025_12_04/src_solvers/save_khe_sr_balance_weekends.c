
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
/*  FILE:         khe_sm_balance_wekends.c                                   */
/*  DESCRIPTION:  Balance weekends                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_mmatch.h"

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0
#define DEBUG4	0
#define DEBUG5	0
#define DEBUG6	0

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_PLUS_OFFSET - a cluster busy times constraint and offset  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_constraint_plus_offset_rec {
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT	constraint;
  int					offset;
} *KHE_CONSTRAINT_PLUS_OFFSET;

typedef HA_ARRAY(KHE_CONSTRAINT_PLUS_OFFSET) ARRAY_KHE_CONSTRAINT_PLUS_OFFSET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_TASK - a weekend task with its non-assignment and asst cost  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_weekend_task_rec {
  KHE_TASK				task;
  KHE_COST				non_asst_cost;
  KHE_COST				asst_cost;
  KHE_MMATCH_NODE			match_node;
} *KHE_WEEKEND_TASK;

typedef HA_ARRAY(KHE_WEEKEND_TASK) ARRAY_KHE_WEEKEND_TASK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_DAY - a weekend day and associated tasks                     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_weekend_day_rec {
  KHE_TIME_GROUP			time_group;
  ARRAY_KHE_WEEKEND_TASK		required_tasks;
  ARRAY_KHE_WEEKEND_TASK		optional_tasks;
} *KHE_WEEKEND_DAY;

typedef HA_ARRAY(KHE_WEEKEND_DAY) ARRAY_KHE_WEEKEND_DAY;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS - complete weekends constraints with equal tg's.    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_constraint_class_rec {
  ARRAY_KHE_CONSTRAINT_PLUS_OFFSET	constraints;
  KHE_COST				min_weight;
  ARRAY_KHE_WEEKEND_DAY			days;
} *KHE_CONSTRAINT_CLASS;

typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_METHOD - the method to use to correct unbalanced weekends            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_METHOD_NONE,
  KHE_METHOD_FIX_OPTIONAL,
  KHE_METHOD_FIX_REQUIRED
} KHE_METHOD;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_SOLVER - a solver for balancing complete weekends            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_weekend_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_SOLN_ADJUSTER		soln_adjuster;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_BALANCE_SOLVER		balance_solver;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_FRAME			days_frame;
  KHE_METHOD			method;
  /* bool			no_undo; */
  ARRAY_KHE_CONSTRAINT_CLASS	constraint_classes;
  HA_ARRAY_BOOL			unique_resource_present;
  int				unique_resource_count;
  KHE_MMATCH			match;
} *KHE_WEEKEND_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSTRAINT_PLUS_OFFSET"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(                  */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset,                    */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Make a new constraint plus offset object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_PLUS_OFFSET KheConstraintPlusOffsetMake(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset, KHE_WEEKEND_SOLVER ws)
{
  KHE_CONSTRAINT_PLUS_OFFSET res;
  HaMake(res, ws->arena);
  res->constraint = cbtc;
  res->offset = offset;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetIsCompleteWeekend(                           */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset)                    */
/*                                                                           */
/*  Return true if cbtc plus offset defines a complete weekend constraint.   */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetIsCompleteWeekend(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc, int offset)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;

  /* must have exactly two time groups, both positive */
  if( KheClusterBusyTimesConstraintTimeGroupCount(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllPositive(cbtc) )
    return false;

  /* must have min limit 2, max limit 2, and allow-zero flag true */
  if( KheClusterBusyTimesConstraintMinimum(cbtc) != 2 ||
      KheClusterBusyTimesConstraintMaximum(cbtc) != 2 ||
      !KheClusterBusyTimesConstraintAllowZero(cbtc) )
    return false;

  /* the time groups must be disjoint */
  tg1 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 0, offset, &po1);
  tg2 = KheClusterBusyTimesConstraintTimeGroup(cbtc, 1, offset, &po2);
  if( !KheTimeGroupDisjoint(tg1, tg2) )
    return false;

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintPlusOffsetEqualTimeGroups(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co1,                                        */
/*    KHE_CONSTRAINT_PLUS_OFFSET co2)                                        */
/*                                                                           */
/*  Return true if these two constraints have equal time groups.  We ignore  */
/*  the possibility that their order could be different.                     */
/*                                                                           */
/*  Implementation note.  These constraints will have already been checked   */
/*  to some extent, including checking that they have two time groups each   */
/*  and that their time groups are positive.  We don't check that again.     */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintPlusOffsetEqualTimeGroups(
  KHE_CONSTRAINT_PLUS_OFFSET co1,
  KHE_CONSTRAINT_PLUS_OFFSET co2)
{
  int i;  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  for(i=0; i<KheClusterBusyTimesConstraintTimeGroupCount(co1->constraint); i++)
  {
    tg1 = KheClusterBusyTimesConstraintTimeGroup(co1->constraint, i,
      co1->offset, &po1);
    tg2 = KheClusterBusyTimesConstraintTimeGroup(co2->constraint, i,
      co2->offset, &po2);
    if( !KheTimeGroupEqual(tg1, tg2) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of co onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintPlusOffsetDebug(KHE_CONSTRAINT_PLUS_OFFSET co,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s|%d", KheConstraintId((KHE_CONSTRAINT) co->constraint),
    co->offset);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WEEKEND_TASK"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEKEND_TASK KheWeekendTaskMake(KHE_TASK task, HA_ARENA a)           */
/*                                                                           */
/*  Make a weekend task object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_TASK KheWeekendTaskMake(KHE_TASK task, HA_ARENA a)
{
  KHE_WEEKEND_TASK res;
  HaMake(res, a);
  res->task = task;
  KheTaskNonAsstAndAsstCost(task, &res->non_asst_cost, &res->asst_cost);
  res->match_node = NULL;
  return res;
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
/*  KHE_WEEKEND_DAY KheWeekendDayMake(KHE_TIME_GROUP tg, HA_ARENA a)         */
/*                                                                           */
/*  Make a new weekend day object with these attributes.                     */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_DAY KheWeekendDayMake(KHE_TIME_GROUP tg, HA_ARENA a)
{
  KHE_WEEKEND_DAY res;
  HaMake(res, a);
  res->time_group = tg;
  HaArrayInit(res->required_tasks, a);
  HaArrayInit(res->optional_tasks, a);
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
  if( wtask->non_asst_cost > 0 )
    HaArrayAddLast(wday->required_tasks, wtask);
  else
    HaArrayAddLast(wday->optional_tasks, wtask);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayAddTasks(KHE_WEEKEND_DAY wday, KHE_WEEKEND_SOLVER ws)  */
/*                                                                           */
/*  Add its tasks to wday.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayAddTasks(KHE_WEEKEND_DAY wday, KHE_WEEKEND_SOLVER ws)
{
  KHE_TIME t;  int i, j, k;  KHE_MEET meet;  KHE_TASK task;
  KHE_WEEKEND_TASK wtask;
  for( i = 0;  i < KheTimeGroupTimeCount(wday->time_group);  i++ )
  {
    t = KheTimeGroupTime(wday->time_group, i);
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(ws->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(ws->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	task = KheTaskProperRoot(task);
	if( KheTaskResourceType(task) == ws->resource_type &&
	    KheTaskTotalDuration(task) == 1 )
        {
	  wtask = KheWeekendTaskMake(task, ws->arena);
          KheWeekendDayAddTask(wday, wtask);
	}
      }
    }
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
/*  bool KheWeekendDayHasOptionalTasks(KHE_WEEKEND_DAY wd)                   */
/*                                                                           */
/*  Return true if wd has at least one optional task.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheWeekendDayHasOptionalTasks(KHE_WEEKEND_DAY wd)
{
  return HaArrayCount(wd->optional_tasks) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWeekendDayHasAssignedOptionalTasks(KHE_WEEKEND_DAY wd)           */
/*                                                                           */
/*  Return true if any of the optional tasks of wd are assigned.             */
/*                                                                           */
/*****************************************************************************/

static bool KheWeekendDayHasAssignedOptionalTasks(KHE_WEEKEND_DAY wd)
{
  KHE_WEEKEND_TASK wtask;  int i;
  HaArrayForEach(wd->optional_tasks, wtask, i)
    if( KheTaskAsstResource(wtask->task) != NULL )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheWeekendDayMinRequiredNonAsstCost(KHE_WEEKEND_DAY wd)         */
/*                                                                           */
/*  Return the minimum, over all required tasks t on wd, of the non-asst     */
/*  cost of t.  It is an error if there are no required tasks.               */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheWeekendDayMinRequiredNonAsstCost(KHE_WEEKEND_DAY wd)
{
  KHE_COST res;  KHE_WEEKEND_TASK wtask;  int i;
  HnAssert(HaArrayCount(wd->required_tasks) > 0,
    "KheWeekendDayMinRequiredNonAsstCost internal error");
  wtask = HaArrayFirst(wd->required_tasks);
  res = wtask->non_asst_cost;
  for( i = 1;  i < HaArrayCount(wd->required_tasks);  i++ )
  {
    wtask = HaArray(wd->required_tasks, i);
    if( wtask->non_asst_cost < res )
      res = wtask->non_asst_cost;
  }
  return res;
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
/*  void KheWeekendDayFixOptionalTasks(KHE_WEEKEND_DAY wd,                   */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Fix the assignments of the optional tasks of wd.                         */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayFixOptionalTasks(KHE_WEEKEND_DAY wd,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_TASK wtask;  int i;
  HaArrayForEach(wd->optional_tasks, wtask, i)
    KheSolnAdjusterTaskEnsureFixed(ws->soln_adjuster, wtask->task);
  /* ***
  if( ws->soln_adjuster != NULL )
    HaArrayForEach(wd->optional_tasks, wtask, i)
      KheSolnAdjusterTaskEnsureFixed(ws->soln_adjuster, wtask->task);
  else
    HaArrayForEach(wd->optional_tasks, wtask, i)
      KheTaskAssignFix(wtask->task);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskOffsetInFrame(KHE_TASK task, KHE_FRAME frame)                 */
/*                                                                           */
/*  Return the offset of task in frame, or 0 if task has no offset.          */
/*                                                                           */
/*  Implementation note.  This does not do a good job on grouped tasks.      */
/*  But that may not matter very much.                                       */
/*                                                                           */
/*****************************************************************************/

static int KheTaskOffsetInFrame(KHE_TASK task, KHE_FRAME frame)
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
  KHE_RESOURCE_GROUP domain1, domain2;  KHE_SOLN soln;  KHE_COST cost;

  /* no edge if domains do not intersect */
  domain1 = KheTaskDomain(wtask1->task);
  domain2 = KheTaskDomain(wtask2->task);
  if( KheResourceGroupIntersectCount(domain1, domain2) == 0 )
    return *c1 = *c2 = *c3 = 0, false;

  /* (c1, c2) is cost after assignment, c3 is difference in offset */
  soln = KheTaskSoln(wtask1->task);
  cost = KheSolnCost(soln) - wtask1->non_asst_cost - wtask2->non_asst_cost;
  *c1 = KheHardCost(cost);
  *c2 = KheSoftCost(cost);
  *c3 = KheTaskOffsetInFrame(wtask1->task, ws->days_frame) -
	  KheTaskOffsetInFrame(wtask2->task, ws->days_frame);
  if( *c3 < 0 ) *c3 = - *c3;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendDayFixRequiredTasks(KHE_WEEKEND_DAY wd1,                  */
/*    KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)                            */
/*                                                                           */
/*  Match the required tasks on these two days and fix any from wd1 that     */
/*  fail to match.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendDayFixRequiredTasks(KHE_WEEKEND_DAY wd1,
  KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)
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
	  fprintf(stderr, "        --(%.5f, %d)-> ",
	    KheCostShow(KheCost(c1, c2)), c3);
	  KheTaskDebug(wtask2->task, 3, 0, stderr);
	}
      }
  }
  if( DEBUG6 )
    fprintf(stderr, "  ]\n");

  /* solve */
  KheMMatchSolve(ws->match);

  /* fix the unmatched demand nodes from wd1 */
  HaArrayForEach(wd1->required_tasks, wtask1, i)
    if( !KheMMatchResultEdge(wtask1->match_node, &sn, &mult, &c1, &c2, &c3) )
    {
      /* wtask1 did not match, so fix it */
      if( DEBUG5 )
	fprintf(stderr, "  fixing required task %s\n", KheTaskId(wtask1->task));
      KheSolnAdjusterTaskEnsureFixed(ws->soln_adjuster, wtask1->task);
      /* ***
      if( ws->soln_adjuster != NULL )
	KheSolnAdjusterTaskEnsureFixed(ws->soln_adjuster, wtask1->task);
      else
	KheTaskAssignFix(wtask1->task);
      *** */
    }

  /* clear the matching */
  KheMMatchClear(ws->match);
  HaArrayForEach(wd1->required_tasks, wtask1, i)
    wtask1->match_node = NULL;
  HaArrayForEach(wd2->required_tasks, wtask2, j)
    wtask2->match_node = NULL;
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
/*  Submodule "KHE_CONSTRAINT_CLASS"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheConstraintPlusOffsetWeight(KHE_CONSTRAINT_PLUS_OFFSET co)    */
/*                                                                           */
/*  Return the combined weight of co's constraint.                           */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheConstraintPlusOffsetWeight(KHE_CONSTRAINT_PLUS_OFFSET co)
{
  return KheConstraintCombinedWeight((KHE_CONSTRAINT) co->constraint);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSTRAINT_CLASS KheConstraintClassMake(                             */
/*    KHE_CONSTRAINT_PLUS_OFFSET co, KHE_WEEKEND_SOLVER ws)                  */
/*                                                                           */
/*  Make a new constraint class object containing co initially.              */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSTRAINT_CLASS KheConstraintClassMake(
  KHE_CONSTRAINT_PLUS_OFFSET co, KHE_WEEKEND_SOLVER ws)
{
  KHE_CONSTRAINT_CLASS res;
  HaMake(res, ws->arena);
  HaArrayInit(res->constraints, ws->arena);
  HaArrayAddLast(res->constraints, co);
  res->min_weight = KheConstraintPlusOffsetWeight(co);
  HaArrayInit(res->days, ws->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassAcceptsConstraint(KHE_CONSTRAINT_CLASS cc,        */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Return true if co is compatible with cc.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintClassAcceptsConstraint(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_PLUS_OFFSET co)
{
  KHE_CONSTRAINT_PLUS_OFFSET co2;
  co2 = HaArrayFirst(cc->constraints);
  return KheConstraintPlusOffsetEqualTimeGroups(co2, co);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassAddConstraint(KHE_CONSTRAINT_CLASS cc,            */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Add co to cc.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassAddConstraint(KHE_CONSTRAINT_CLASS cc,
  KHE_CONSTRAINT_PLUS_OFFSET co)
{
  KHE_COST co_weight;
  HaArrayAddLast(cc->constraints, co);
  co_weight = KheConstraintPlusOffsetWeight(co);
  if( co_weight < cc->min_weight )
    cc->min_weight = co_weight;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintClassCoversAllResources(KHE_CONSTRAINT_CLASS cc,       */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Return true if the constraints of class cc apply to all resources of     */
/*  type ws->resource_type.                                                  */
/*                                                                           */
/*****************************************************************************/
static void KheWeekendSolverUniqueResourceCountBegin(KHE_WEEKEND_SOLVER ws);
static void KheWeekendSolverUniqueResourceAdd(KHE_WEEKEND_SOLVER ws,
  KHE_RESOURCE r);
static int KheWeekendSolverUniqueResourceCountEnd(KHE_WEEKEND_SOLVER ws);

static bool KheConstraintClassCoversAllResources(KHE_CONSTRAINT_CLASS cc,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  KHE_RESOURCE_TYPE rt;  int i, j, k, count;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;

  /* special case for when the first constraint covers all resources */
  /* *** no, ResourceOfType does not count unique resources
  co = HaArrayFirst(cc->constraints);
  if( KheClusterBusyTimesConstraintResourceOfTypeCount(co->constraint, rt)
	== KheResourceTypeResourceCount(rt) )
    return true;
  *** */

  /* general case */
  rt = ws->resource_type;
  KheWeekendSolverUniqueResourceCountBegin(ws);
  HaArrayForEach(cc->constraints, co, i)
  {
    count = KheClusterBusyTimesConstraintResourceGroupCount(co->constraint);
    for( j = 0;  j < count;  j++ )
    {
      rg = KheClusterBusyTimesConstraintResourceGroup(co->constraint, j);
      if( KheResourceGroupResourceType(rg) == rt )
	for( k = 0;  k < KheResourceGroupResourceCount(rg);  k++ )
	{
	  r = KheResourceGroupResource(rg, k);
	  KheWeekendSolverUniqueResourceAdd(ws, r);
	}
    }
    count = KheClusterBusyTimesConstraintResourceCount(co->constraint);
    for( j = 0;  j < count;  j++ )
    {
      r = KheClusterBusyTimesConstraintResource(co->constraint, j);
      if( KheResourceResourceType(r) == rt )
	KheWeekendSolverUniqueResourceAdd(ws, r);
    }
  }
  count = KheWeekendSolverUniqueResourceCountEnd(ws);
  return count == KheResourceTypeResourceCount(rt);
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

static void KheConstraintClassAddDays(KHE_CONSTRAINT_CLASS cc,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;  int i;  KHE_WEEKEND_DAY wday;
  co = HaArrayFirst(cc->constraints);
  cbtc = co->constraint;
  for( i = 0;  i < KheClusterBusyTimesConstraintTimeGroupCount(cbtc);  i++ )
  {
    tg = KheClusterBusyTimesConstraintTimeGroup(co->constraint, i,
      co->offset, &po);
    wday = KheWeekendDayMake(tg, ws->arena);
    KheWeekendDayAddTasks(wday, ws);
    HaArrayAddLast(cc->days, wday);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassDoSolve(KHE_CONSTRAINT_CLASS cc,                  */
/*    KHE_WEEKEND_DAY wd1, KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)       */
/*                                                                           */
/*  Solve weekend (wd1, wd2), assuming that wd1 has more required tasks      */
/*  than wd2 has.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassDoSolve(KHE_CONSTRAINT_CLASS cc,
  KHE_WEEKEND_DAY wd1, KHE_WEEKEND_DAY wd2, KHE_WEEKEND_SOLVER ws)
{
  KHE_COST min_non_asst_cost, min_asst_cost, soln_cost, c, c1, c2, c3;
  if( KheWeekendDayHasOptionalTasks(wd2) &&
      !KheWeekendDayHasAssignedOptionalTasks(wd2) )
  {
    min_non_asst_cost = KheWeekendDayMinRequiredNonAsstCost(wd1);
    min_asst_cost = KheWeekendDayMinOptionalAsstCost(wd2);
    soln_cost = KheSolnCost(ws->soln);
    c = KheBalanceSolverMarginalCost(ws->balance_solver);
    c1 = soln_cost - min_non_asst_cost + c + cc->min_weight;
    c2 = soln_cost;
    c3 = soln_cost - min_non_asst_cost + 2*c + min_asst_cost;
    if( DEBUG5 )
      fprintf(stderr, "  KheBalanceWeekends: c1 = %.5f, c2 = %.5f, "
	"c3 = %.5f, so %s\n", KheCostShow(c1), KheCostShow(c2),
	KheCostShow(c3), (c3 > c1 || c3 > c2 ? "fixing" : "not fixing"));
      /* ***
      fprintf(stderr, "  KheBalanceWeekends: c1 = %.5f, c2 = %.5f, "
	"c3 = %.5f, so %s %s\n", KheCostShow(c1), KheCostShow(c2),
	KheCostShow(c3), (c3 > c1 || c3 > c2 ? "fixing" : "not fixing"),
	KheTimeGroupId(wd2->time_group));
      *** */
    if( c3 > c1 || c3 > c2 ) switch( ws->method )
    {
      case KHE_METHOD_FIX_OPTIONAL:

	KheWeekendDayFixOptionalTasks(wd2, ws);
	break;

      case KHE_METHOD_FIX_REQUIRED:

	KheWeekendDayFixRequiredTasks(wd1, wd2, ws);
	break;

      default:

	HnAbort("KheConstraintClassDoSolve internal error");
	break;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,                    */
/*    KHE_WEEKEND_SOLVER ws)                                                 */
/*                                                                           */
/*  Solve cc.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp);

static void KheConstraintClassSolve(KHE_CONSTRAINT_CLASS cc,
  KHE_WEEKEND_SOLVER ws)
{
  KHE_WEEKEND_DAY wd1, wd2;

  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheConstraintClassSolve(cc, ws)\n");
    KheConstraintClassDebug(cc, 2, 2, stderr);
  }

  wd1 = HaArrayFirst(cc->days);
  wd2 = HaArrayLast(cc->days);
  if( KheWeekendDayDominates(wd1, wd2) )
    KheConstraintClassDoSolve(cc, wd1, wd2, ws);
  else if( KheWeekendDayDominates(wd2, wd1) )
    KheConstraintClassDoSolve(cc, wd2, wd1, ws);

  if( DEBUG3 )
    fprintf(stderr, "] KheConstraintClassSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassHeaderDebug(KHE_CONSTRAINT_CLASS cc, FILE *fp)    */
/*                                                                           */
/*  Print the header part of cc's debug print onto fp.                       */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassHeaderDebug(KHE_CONSTRAINT_CLASS cc, FILE *fp)
{
  fprintf(fp, "ConstraintClass(%d constraints, min_weight %.5f)",
    HaArrayCount(cc->constraints), KheCostShow(cc->min_weight));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug printf of cc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheConstraintClassDebug(KHE_CONSTRAINT_CLASS cc,
  int verbosity, int indent, FILE *fp)
{
  KHE_CONSTRAINT_PLUS_OFFSET co;  int i;  KHE_WEEKEND_DAY wday;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheConstraintClassHeaderDebug(cc, fp);
    fprintf(fp, "\n");
    HaArrayForEach(cc->constraints, co, i)
      KheConstraintPlusOffsetDebug(co, 2, indent + 2, fp);
    HaArrayForEach(cc->days, wday, i)
      KheWeekendDayDebug(wday, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheConstraintClassHeaderDebug(cc, fp);
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
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_FRAME frame,                      */
/*    KHE_BALANCE_SOLVER bs, KHE_METHOD method, bool no_undo, HA_ARENA a)    */
/*                                                                           */
/*  Make a weekend solver object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEKEND_SOLVER KheWeekendSolverMake(KHE_SOLN soln,
  KHE_SOLN_ADJUSTER sa, KHE_RESOURCE_TYPE rt,
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_FRAME frame,
  KHE_BALANCE_SOLVER bs, KHE_METHOD method,
  /* bool no_undo, */  HA_ARENA a)
{
  KHE_WEEKEND_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->soln_adjuster = sa;
  res->resource_type = rt;
  res->balance_solver = bs;
  res->etm = etm;
  res->days_frame = frame;
  res->method = method;
  /* res->no_undo = no_undo; */
  HaArrayInit(res->constraint_classes, a);
  HaArrayInit(res->unique_resource_present, a);
  res->unique_resource_count = 0;
  if( method == KHE_METHOD_FIX_REQUIRED )
    res->match = KheMMatchMake((void *) res, &KheWeekendSolverDebugFn,
      &KheWeekendTaskDebugFn, &KheWeekendTaskDebugFn, &KheCostDebugFn, a);
  else
    res->match = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverAddConstraintPlusOffset(KHE_WEEKEND_SOLVER ws,      */
/*    KHE_CONSTRAINT_PLUS_OFFSET co)                                         */
/*                                                                           */
/*  Add co to ws.                                                            */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverAddConstraintPlusOffset(KHE_WEEKEND_SOLVER ws,
  KHE_CONSTRAINT_PLUS_OFFSET co)
{
  KHE_CONSTRAINT_CLASS cc;  int i;

  /* try to add co to an existing constraint class */
  HaArrayForEach(ws->constraint_classes, cc, i)
    if( KheConstraintClassAcceptsConstraint(cc, co) )
    {
      KheConstraintClassAddConstraint(cc, co);
      return;
    }

  /* no luck, so make a new constraint class for co */
  cc = KheConstraintClassMake(co, ws);
  HaArrayAddLast(ws->constraint_classes, cc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverAddConstraintClasses(KHE_WEEKEND_SOLVER ws)         */
/*                                                                           */
/*  Add constraint classes to ws.                                            */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverAddConstraintClasses(KHE_WEEKEND_SOLVER ws)
{
  KHE_INSTANCE ins;  int i, j, offset, count;  KHE_CONSTRAINT_PLUS_OFFSET co;
  KHE_CONSTRAINT c;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_RESOURCE_TYPE rt;
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
	  if( KheConstraintPlusOffsetIsCompleteWeekend(cbtc, offset) )
	  {
	    /* have complete weekends constraint plus offset, add to ws */
	    co = KheConstraintPlusOffsetMake(cbtc, offset, ws);
	    KheWeekendSolverAddConstraintPlusOffset(ws, co);
	  }
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverUniqueResourceCountBegin(KHE_WEEKEND_SOLVER ws)     */
/*                                                                           */
/*  Begin unique resource counting.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverUniqueResourceCountBegin(KHE_WEEKEND_SOLVER ws)
{
  HaArrayClear(ws->unique_resource_present);
  ws->unique_resource_count = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekendSolverUniqueResourceAdd(KHE_WEEKEND_SOLVER ws,            */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Report that r is present.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheWeekendSolverUniqueResourceAdd(KHE_WEEKEND_SOLVER ws,
  KHE_RESOURCE r)
{
  int index;
  index = KheResourceResourceTypeIndex(r);
  HaArrayFill(ws->unique_resource_present, index + 1, false);
  if( !HaArray(ws->unique_resource_present, index) )
  {
    HaArrayPut(ws->unique_resource_present, index, true);
    ws->unique_resource_count++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheWeekendSolverUniqueResourceCountEnd(KHE_WEEKEND_SOLVER ws)        */
/*                                                                           */
/*  End unique resource counting and return the number of unique resources.  */
/*                                                                           */
/*****************************************************************************/

static int KheWeekendSolverUniqueResourceCountEnd(KHE_WEEKEND_SOLVER ws)
{
  return ws->unique_resource_count;
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
  KHE_CONSTRAINT_CLASS cc;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheWeekendSolverHeaderDebug(ws, fp);
    fprintf(fp, "\n");
    HaArrayForEach(ws->constraint_classes, cc, i)
      KheConstraintClassDebug(cc, verbosity, indent + 2, fp);
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
/*  KHE_METHOD KheGetMethod(KHE_OPTIONS options)                             */
/*                                                                           */
/*  Get the method option from options.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_METHOD KheGetMethod(KHE_OPTIONS options)
{
  char *str;
  str = KheOptionsGet(options, "rs_balance_weekends_method", "fix_required");
  if( strcmp(str, "none") == 0 )
    return KHE_METHOD_NONE;
  else if( strcmp(str, "fix_optional") == 0 )
    return KHE_METHOD_FIX_OPTIONAL;
  else if( strcmp(str, "fix_required") == 0 )
    return KHE_METHOD_FIX_REQUIRED;
  else
  {
    HnAbort("KheBalanceWeekends:  invalid value \"%s\" of option "
      "rs_balance_weekends_method", str);
    return KHE_METHOD_NONE;   /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBalanceWeekends(KHE_SOLN soln, KHE_OPTIONS options,              */
/*    KHE_RESOURCE_TYPE rt, KHE_SOLN_ADJUSTER sa)                            */
/*                                                                           */
/*  Balance the weekends of sa's solution.                                   */
/*                                                                           */
/*****************************************************************************/

void KheBalanceWeekends(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE_TYPE rt, KHE_SOLN_ADJUSTER sa)
{
  KHE_BALANCE_SOLVER bs;  HA_ARENA a;  KHE_FRAME frame;
  KHE_WEEKEND_SOLVER ws;  KHE_CONSTRAINT_CLASS cc;  int i;
  KHE_EVENT_TIMETABLE_MONITOR etm;  /* bool no_undo; */  KHE_METHOD method;

  /* return immediately if rs_balance_weekends_method is "none" */
  method = KheGetMethod(options);
  if( method == KHE_METHOD_NONE )
    return;

  /* boilerplate */
  if( DEBUG1 )
    fprintf(stderr, "[ KheBalanceWeekends(%s, %s)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
  a = KheSolnArenaBegin(soln);
  frame = KheOptionsFrame(options, "gs_common_frame", soln);
  bs = KheBalanceSolverMake(soln, rt, frame, a);

  if( KheBalanceSolverTotalSupply(bs) < KheBalanceSolverTotalDemand(bs) )
  {
    /* shortage of workload, so we need to balance weekends */
    /* make a weekend solver and add constraint classes to it */
    etm = (KHE_EVENT_TIMETABLE_MONITOR)
      KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
    /* ***
    no_undo = KheOptionsGetBool(options, "rs_balance_weekends_no_undo", false);
    *** */
    ws = KheWeekendSolverMake(soln, sa, rt, etm, frame, bs, method,
      /* no_undo, */ a);
    KheWeekendSolverAddConstraintClasses(ws);

    /* do the job for each constraint class */
    HaArrayForEach(ws->constraint_classes, cc, i)
      if( KheConstraintClassCoversAllResources(cc, ws) )
      {
	KheConstraintClassAddDays(cc, ws);
	KheConstraintClassSolve(cc, ws);
      }
    if( DEBUG2 )
      KheWeekendSolverDebug(ws, 2, 2, stderr);
  }

  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheBalanceWeekends returning\n");
}

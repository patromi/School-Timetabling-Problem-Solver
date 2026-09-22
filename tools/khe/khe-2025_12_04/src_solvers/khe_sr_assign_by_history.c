
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
/*  FILE:         khe_sr_assign_by_history.c                                 */
/*  DESCRIPTION:  Assignment by history                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#include "khe_mmatch.h"
#define bool_show(x) ((x) ? "true" : "false")

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0
#define DEBUG4	0
#define DEBUG5	0	/* assignments (only) */
#define DEBUG6	0	/* edges (in detail) */
#define DEBUG7	0	/* busy days limit */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_NODE - one task with derived information                        */
/*                                                                           */
/*  This type implements the "admissible task" of the documentation.         */
/*  It is specific to one stage of one history instance.                     */
/*                                                                           */
/*****************************************************************************/
#define KHE_TASK_NODE_MAGIC 3028777

typedef struct khe_task_node_rec {
  int				magic;			/* checking          */
  KHE_TASK			task;			/* proper root task  */
  KHE_INTERVAL			interval;		/* task's interval   */
  KHE_COST			his_non_asst_cost;	/* cost of non-aast  */
  KHE_COST			his_asst_cost;		/* cost of asst      */
  KHE_MMATCH_NODE		his_match_node; 	/* match node        */
} *KHE_TASK_NODE;

typedef HA_ARRAY(KHE_TASK_NODE) ARRAY_KHE_TASK_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_STATE - the current state of an admissible resource         */
/*                                                                           */
/*    KHE_RESOURCE_STATE_FINISHED                                            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_RESOURCE_STATE_INITIAL,		/* before anything done              */
  KHE_RESOURCE_STATE_FINISHED,		/* nothing more to do with this one  */
  KHE_RESOURCE_STATE_ACTIVE_UNASSIGNED,	/* active, unassigned on current day */
  KHE_RESOURCE_STATE_ACTIVE_ASSIGNED	/* active, assigned on current day   */
} KHE_RESOURCE_STATE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE - information about one resource                       */
/*                                                                           */
/*  This node is specific to one history instance.  It is re-used by each    */
/*  stage of that history instance wfor which this is an active resource.    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_node_rec {
  KHE_RESOURCE			resource;
  int				history;
  int				minimum;
  int				maximum;
  int				busy_days_limit;
  /* KHE_INTERVAL		assigned_interval; */	/* grows each stage */
  KHE_COST			his_non_asst_cost;	/* cost of non-asst */
  KHE_COST			his_asst_cost;		/* cost of asst     */
  KHE_MMATCH_NODE		his_match_node;		/* match node       */
  KHE_RESOURCE_STATE		state;			/* current state    */
} *KHE_RESOURCE_NODE;

typedef HA_ARRAY(KHE_RESOURCE_NODE) ARRAY_KHE_RESOURCE_NODE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HISTORY_INSTANCE_STAGE                                          */
/*                                                                           */
/*  One stage of one history instance, defined by a history instance plus    */
/*  a index into the current frame.                                          */
/*                                                                           */
/*****************************************************************************/
typedef struct khe_history_instance_rec *KHE_HISTORY_INSTANCE;

typedef struct khe_history_instance_stage_rec {
  KHE_HISTORY_INSTANCE		hi;
  int				tgi;
  KHE_TIME_GROUP		day_tg;
  KHE_TIME_GROUP		class_tg;
} KHE_HISTORY_INSTANCE_STAGE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HISTORY_INSTANCE - one instance of the problem we're solving    */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_CONSTRAINT_CLASS) ARRAY_KHE_CONSTRAINT_CLASS;

struct khe_history_instance_rec {
  KHE_CONSTRAINT_CLASS		class;
  ARRAY_KHE_RESOURCE_NODE	resource_nodes;
  ARRAY_KHE_CONSTRAINT_CLASS	busy_days_classes;
};

typedef HA_ARRAY(KHE_HISTORY_INSTANCE) ARRAY_KHE_HISTORY_INSTANCE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HISTORY_SOLVER                                                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_history_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  KHE_SOLN_ADJUSTER		soln_adjuster;
  KHE_CONSTRAINT_CLASS_FINDER	admissible_ccf;
  KHE_CONSTRAINT_CLASS_FINDER	busy_days_ccf;
  ARRAY_KHE_HISTORY_INSTANCE	history_instances;
  ARRAY_KHE_RESOURCE_NODE	his_resource_nodes;	/* one stage         */
  ARRAY_KHE_TASK_NODE		his_task_nodes;		/* one stage         */
  KHE_MMATCH			his_match;		/* one stage         */
  HA_ARRAY_INT			tmp_indexes;		/* scratch array     */
  KHE_TASK_SET			tmp_task_set;		/* scratch task set  */
  int				assts_made;
} *KHE_HISTORY_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_NODE"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_NODE KheTaskNodeMakeAndInitForSolve(KHE_TASK task,              */
/*    KHE_INTERVAL in, KHE_HISTORY_SOLVER hs)                                */
/*                                                                           */
/*  Return a new task node containing task, whose interval is in.  The       */
/*  node is initialized for solving (i.e. res->his_match_node is set).       */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_NODE KheTaskNodeMakeAndInitForSolve(KHE_TASK task,
  KHE_INTERVAL in, KHE_HISTORY_SOLVER hs)
{
  KHE_TASK_NODE res;
  HaMake(res, hs->arena);
  res->magic = KHE_TASK_NODE_MAGIC;
  res->task = task;
  res->interval = in;
  KheTaskNonAsstAndAsstCost(task, &res->his_non_asst_cost, &res->his_asst_cost);
  res->his_match_node = KheMMatchSupplyNodeMake(hs->his_match, 1, (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoAssignResource(KHE_TASK task, KHE_RESOURCE r,              */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Assign r to task.  This must succeed.                                    */
/*                                                                           */
/*  The documentation says that if hs->soln_adjuster != NULL, the            */
/*  assignments are made and fixed, and the fixes (but not the assignments)  */
/*  are added to hs->soln_adjuster so that they can be removed later.        */
/*  If hs->soln_adjuster == NULL, the assignments are made but not fixed.    */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoAssignResource(KHE_TASK task, KHE_RESOURCE r,
  KHE_HISTORY_SOLVER hs)
{
  if( DEBUG5 )
  {
    fprintf(stderr, "  KheAssignByHistory assigning%s %s to ",
      hs->soln_adjuster != NULL ? "+fixing" : "", KheResourceId(r));
    KheTaskDebug(task, 2, 0, stderr);
  }
  if( !KheTaskAssignResource(task, r) )
    HnAbort("KheTaskNodeAssignResource internal error");
  if( hs->soln_adjuster != NULL )
    KheSolnAdjusterTaskAssignFix(hs->soln_adjuster, task);
  hs->assts_made++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp) */
/*                                                                           */
/*  Debug print of the header of tn in frame onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebugHeader(KHE_TASK_NODE tn, KHE_FRAME frame, FILE *fp)
{
  fprintf(fp, "TaskNode(");
  KheTaskDebug(tn->task, 1, -1, fp);
  fprintf(fp, ", in %s, non_asst %.5f, asst %.5f)",
    KheIntervalShow(tn->interval, frame),
    KheCostShow(tn->his_non_asst_cost), KheCostShow(tn->his_asst_cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebug(KHE_TASK_NODE tn, KHE_FRAME frame,                 */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tn in frame onto fp with the given verbosity and indent.  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebug(KHE_TASK_NODE tn, KHE_FRAME frame,
  int verbosity, int indent, FILE *fp)
{
  if( indent < 0 )
    KheTaskNodeDebugHeader(tn, frame, fp);
  else if( verbosity <= 1 )
  {
    fprintf(fp, "%*s", indent, "");
    KheTaskNodeDebugHeader(tn, frame, fp);
    fprintf(fp, "\n");
  }
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheTaskNodeDebugHeader(tn, frame, fp);
    fprintf(fp, " ]\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNodeDebugFn(void *value, int verbosity,                      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Task node debug function for passing to the mmatch module.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskNodeDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_NODE tn;
  tn = (KHE_TASK_NODE) value;
  KheTaskNodeDebug(tn, NULL, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_RESOURCE_NODE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r,                    */
/*    int history, int minimum, HA_ARENA a)                                  */
/*                                                                           */
/*  Make a resource node with these attributes.  This is specific to one     */
/*  history instance.                                                        */
/*                                                                           */
/*****************************************************************************/
static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp);

static KHE_RESOURCE_NODE KheResourceNodeMake(KHE_RESOURCE r,
  int history, int minimum, int maximum, HA_ARENA a)
{
  KHE_RESOURCE_NODE res;
  HaMake(res, a);
  res->resource = r;
  res->history = history;
  res->minimum = minimum;
  res->maximum = maximum;
  res->busy_days_limit = INT_MAX;
  /* res->assigned_interval = KheIntervalMake(1, 0); */
  res->his_non_asst_cost = 0;
  res->his_asst_cost = 0;
  res->his_match_node = NULL;
  res->state = KHE_RESOURCE_STATE_INITIAL;
  if( DEBUG3 )
  {
    fprintf(stderr, "  KheResourceNodeMake returning ");
    KheResourceNodeDebug(res, 2, 0, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of rn onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDebug(KHE_RESOURCE_NODE rn, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ %s: history %d, minimum %d", KheResourceId(rn->resource),
    rn->history, rn->minimum);
  if( rn->busy_days_limit != INT_MAX )
    fprintf(fp, ", busy_days_limit %d", rn->busy_days_limit);
  fprintf(fp, " ]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceNodeDebugFn(void *value, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Resource node debug function for passing to the mmatch module.           */
/*                                                                           */
/*****************************************************************************/

static void KheResourceNodeDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_RESOURCE_NODE rn;
  rn = (KHE_RESOURCE_NODE) value;
  KheResourceNodeDebug(rn, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HISTORY_INSTANCE_STAGE"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_HISTORY_INSTANCE_STAGE KheHistoryInstanceStageMake(                  */
/*    KHE_HISTORY_INSTANCE hi, int tgi, KHE_HISTORY_SOLVER hs)               */
/*                                                                           */
/*  Make an history instance stage object from hi and tgi.  There is no      */
/*  memory allocation.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_HISTORY_INSTANCE_STAGE KheHistoryInstanceStageMake(
  KHE_HISTORY_INSTANCE hi, int tgi, KHE_HISTORY_SOLVER hs)
{
  KHE_HISTORY_INSTANCE_STAGE res;  KHE_POLARITY po;
  res.hi = hi;
  res.tgi = tgi;
  res.day_tg = KheFrameTimeGroup(hs->days_frame, tgi);
  res.class_tg = KheConstraintClassTimeGroup(hi->class, tgi, &po);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCheckTimesAndSetIndexes(KHE_TASK task, KHE_CONSTRAINT_CLASS cc,  */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Helper function for KheHistoryInstanceStageAcceptsTask below.  This      */
/*  checks that every time that task is running is monitored by cc, and      */
/*  it builds hs->tmp_indexes, an array of indexes of the days that task     */
/*  is running.                                                              */
/*                                                                           */
/*  Implementation note.  Variable index is clearly set to an index          */
/*  into hs->days_frame.  But by condition (5) of the admissibility          */
/*  conditions for constraints, this is also an index into the sequence      */
/*  of time groups of cc.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheCheckTimesAndSetIndexes(KHE_TASK task, KHE_CONSTRAINT_CLASS cc,
  KHE_HISTORY_SOLVER hs)
{
  KHE_TASK child_task;  int i, index, pos;  KHE_MEET meet;
  KHE_TIME meet_t, t;  KHE_TIME_GROUP tg;  KHE_POLARITY po;

  /* do the job for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    meet_t = KheMeetAsstTime(meet);
    if( meet_t != NULL )
      for( i = 0;  i < KheMeetDuration(meet);  i++ )
      {
	t = KheTimeNeighbour(meet_t, i);
	index = KheFrameTimeIndex(hs->days_frame, t);
	tg = KheConstraintClassTimeGroup(cc, index, &po);
	if( !KheTimeGroupContains(tg, t, &pos) )
	  return false;  /* (f) */
	HaArrayAddLast(hs->tmp_indexes, index);
      }
  }

  /* do the job for task's children */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheCheckTimesAndSetIndexes(child_task, cc, hs) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIntCmp(const void *t1, const void *t2)                            */
/*                                                                           */
/*  Comparison function for sorting an array of integers into increasing     */
/*  order.                                                                   */
/*                                                                           */
/*****************************************************************************/

static int KheIntCmp(const void *t1, const void *t2)
{
  int i1, i2;
  i1 = * (int *) t1;
  i2 = * (int *) t2;
  return i1 - i2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHistoryInstanceStageAcceptsTask(KHE_HISTORY_INSTANCE_STAGE his,  */
/*    KHE_TASK task, KHE_HISTORY_SOLVER hs, KHE_INTERVAL *in)                */
/*                                                                           */
/*  If his accepts task, set *in to task's interval and return true.         */
/*                                                                           */
/*  Implementation note.  Conditions (b) and (c) are satisfied by the        */
/*  time that this function is called.  This function checks the rest.       */
/*                                                                           */
/*****************************************************************************/

static bool KheHistoryInstanceStageAcceptsTask(KHE_HISTORY_INSTANCE_STAGE his,
  KHE_TASK task, KHE_HISTORY_SOLVER hs, KHE_INTERVAL *in)
{
  int init_count, count, first, last;
  *in = KheIntervalMake(1, 0);
  if( KheTaskResourceType(task) != hs->resource_type )
    return false;  /* (a) */
  if( KheTaskAsstResource(task) != NULL )
    return false;  /* (c) */
  HaArrayClear(hs->tmp_indexes);
  if( !KheCheckTimesAndSetIndexes(task, his.hi->class, hs) )
    return false;  /* (f) */
  init_count = HaArrayCount(hs->tmp_indexes);
  HaArraySortUnique(hs->tmp_indexes, &KheIntCmp);
  count = HaArrayCount(hs->tmp_indexes);
  if( count != init_count )
    return false;  /* (e) */
  HnAssert(count > 0, "KheHistoryInstanceStageAcceptsTask internal error");
  first = HaArrayFirst(hs->tmp_indexes);
  if( first != his.tgi )
    return false;  /* (h) */
  last = HaArrayLast(hs->tmp_indexes);
  if( last - first + 1 != count )
    return false;  /* (g) */
  *in = KheIntervalMake(first, last);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceStageMakeAndInitTasksForSolve(                    */
/*    KHE_HISTORY_INSTANCE_STAGE his, KHE_HISTORY_SOLVER hs)                 */
/*                                                                           */
/*  Add new task nodes to *nodes for the tasks of his.  Each task must       */
/*  be admissible, which (as the documentation says) means this:             */
/*                                                                           */
/*  (a) task has resource type hs->resource_type.                            */
/*                                                                           */
/*  (b) task is a proper root task.                                          */
/*                                                                           */
/*  (c) task is not assigned a resource.                                     */
/*                                                                           */
/*  (d) task is running at at least one time.                                */
/*                                                                           */
/*  (e) the times task is running include at most one time from each day.    */
/*                                                                           */
/*  (f) every time that task is running is monitored by his's constraints.   */
/*                                                                           */
/*  (g) the days of the times that task is running are consecutive.          */
/*                                                                           */
/*  (h) task's first time lies in his's time group.                          */
/*                                                                           */
/*  Condition (f) allows us to find the tasks we need by searching all       */
/*  tasks in all meets running at any time of tg.                            */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryInstanceStageMakeAndInitTasksForSolve(
  KHE_HISTORY_INSTANCE_STAGE his, KHE_HISTORY_SOLVER hs)
{
  int i, j, k;  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;
  KHE_TASK_NODE tn;  KHE_INTERVAL in;
  HaArrayClear(hs->his_task_nodes);
  for( i = 0;  i < KheTimeGroupTimeCount(his.class_tg);  i++ )
  {
    t = KheTimeGroupTime(his.class_tg, i);  /* (d) */
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(hs->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(hs->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	task = KheTaskProperRoot(task);  /* (b) */
	if( KheHistoryInstanceStageAcceptsTask(his, task, hs, &in) )
	{
	  /* add task to hs->his_task_nodes */
	  tn = KheTaskNodeMakeAndInitForSolve(task, in, hs);
	  HaArrayAddLast(hs->his_task_nodes, tn);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskDayTimesAreAlsoClassTimes(KHE_TASK task,                     */
/*    KHE_TIME_GROUP day_tg, KHE_TIME_GROUP class_tg, KHE_TIME *time)        */
/*                                                                           */
/*  If the times of task which lie in day_tg all also lie in class_tg,       */
/*  return true, otherwise return false.                                     */
/*                                                                           */
/*  This function will return true when none of the times of task lie in     */
/*  day_tg.  However, this vacuous case does not arise in the context in     */
/*  which this function is called below.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskDayTimesAreAlsoClassTimes(KHE_TASK task,
  KHE_TIME_GROUP day_tg, KHE_TIME_GROUP class_tg)
{
  KHE_MEET meet;  int i, pos;  KHE_TASK child_task;
  KHE_TIME starting_time, time;

  /* do it for task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    starting_time = KheMeetAsstTime(meet);
    if( starting_time != NULL )
    {
      for( i = 0;  i < KheMeetDuration(meet);  i++ )
      {
	time = KheTimeNeighbour(starting_time, i);
	if( KheTimeGroupContains(day_tg, time, &pos) &&
	    !KheTimeGroupContains(class_tg, time, &pos) )
	  return false;
      }
    }
  }

  /* do it for the child tasks of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheTaskDayTimesAreAlsoClassTimes(child_task, day_tg, class_tg) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_STATE KheHistoryInstanceStageResourceState(                 */
/*    KHE_HISTORY_INSTANCE_STAGE his, KHE_RESOURCE_NODE rn,                  */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Set rn->state appropriately for his, and also return it.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_STATE KheHistoryInstanceStageResourceState(
  KHE_HISTORY_INSTANCE_STAGE his, KHE_RESOURCE_NODE rn,
  KHE_HISTORY_SOLVER hs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK task;

  if( rn->state == KHE_RESOURCE_STATE_FINISHED )		/* 2 */
  {
    /* once you're finished, you remain finished */
  }
  else if( rn->history + his.tgi >= rn->minimum )		/* 3 */
  {
    /* rn has reached its target */
    rn->state = KHE_RESOURCE_STATE_FINISHED;
  }
  else if( his.tgi >= rn->busy_days_limit )			/* 4 */
  {
    /* rn would exceed its busy days limit if it continued */
    rn->state = KHE_RESOURCE_STATE_FINISHED;
  }
  else
  {
    rtm = KheResourceTimetableMonitor(hs->soln, rn->resource);
    KheTaskSetClear(hs->tmp_task_set);
    KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm,
      his.day_tg, true, hs->tmp_task_set);
    if( KheTaskSetTaskCount(hs->tmp_task_set) == 0 )
    {
      /* r is active and unassigned and needs to go into the matching */
      rn->state = KHE_RESOURCE_STATE_ACTIVE_UNASSIGNED;		/* 5 */
    }
    else if( KheTaskSetTaskCount(hs->tmp_task_set) >= 2 )
    {
      /* r is anomalous and we don't want anything to do with it */
      rn->state = KHE_RESOURCE_STATE_FINISHED;
    }
    else
    {
      /* r is active and assigned, helpfully or unhelpfully */
      task = KheTaskSetTask(hs->tmp_task_set, 0);
      if( KheTaskDayTimesAreAlsoClassTimes(task, his.day_tg, his.class_tg) )
      {
	/* resource is assigned a helpful task */
	rn->state = KHE_RESOURCE_STATE_ACTIVE_ASSIGNED;		/* 5 */
      }
      else
      {
	/* resource is assigned an unhelpful task */
	rn->state = KHE_RESOURCE_STATE_FINISHED;
      }
    }
  }

  return rn->state;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceStageAddResource(KHE_HISTORY_INSTANCE_STAGE his,  */
/*    KHE_RESOURCE_NODE rn, KHE_HISTORY_SOLVER hs)                           */
/*                                                                           */
/*  Add rn to his.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryInstanceStageAddResource(KHE_HISTORY_INSTANCE_STAGE his,
  KHE_RESOURCE_NODE rn, KHE_HISTORY_SOLVER hs)
{
  rn->his_match_node = KheMMatchDemandNodeMake(hs->his_match, 1,(void *)rn);
  rn->his_asst_cost = 0;
  rn->his_non_asst_cost = KheConstraintClassResourceDeterminantToCost(
    his.hi->class, rn->resource, rn->history + his.tgi, false);
  HaArrayAddLast(hs->his_resource_nodes, rn);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHistoryInstanceStageInitResourcesForSolve(                       */
/*    KHE_HISTORY_INSTANCE_STAGE his, KHE_HISTORY_SOLVER hs)                 */
/*                                                                           */
/*  Initialize the resource nodes for solving his.  Return true if there     */
/*  are any active resources, assigned or unassigned.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheHistoryInstanceStageInitResourcesForSolve(
  KHE_HISTORY_INSTANCE_STAGE his, KHE_HISTORY_SOLVER hs)
{
  KHE_RESOURCE_NODE rn;  int i;  bool res;
  res = false;
  HaArrayClear(hs->his_resource_nodes);
  HaArrayForEach(his.hi->resource_nodes, rn, i)  /* 1 */
  {
    rn->his_match_node = NULL;
    switch( KheHistoryInstanceStageResourceState(his, rn, hs) )
    {
      case KHE_RESOURCE_STATE_FINISHED:

	/* finished with rn, so skip it */
	break;

      case KHE_RESOURCE_STATE_ACTIVE_UNASSIGNED:

	/* active and unassigned, so add it to the matching */
	KheHistoryInstanceStageAddResource(his, rn, hs);
	res = true;
	break;

      case KHE_RESOURCE_STATE_ACTIVE_ASSIGNED:

	/* active but already assigned, so not part of the matching */
	res = true;
	break;

      case KHE_RESOURCE_STATE_INITIAL:
      default:

	HnAbort("KheHistoryInstanceStageInitResourcesForSolve internal error");
	break;

    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEdgeIsWanted(KHE_HISTORY_SOLVER hs,                              */
/*    KHE_HISTORY_INSTANCE_STAGE his, KHE_RESOURCE_NODE rn,                  */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm,                                    */
/*    KHE_TASK_NODE tn, int *c1, int *c2, int *c3)                           */
/*                                                                           */
/*  If an edge is wanted between rn and tn, return true and set *c1, *c2,    */
/*  and *c3 to a suitable cost vector.  Otherwise return false.              */
/*                                                                           */
/*****************************************************************************/

static bool KheEdgeIsWanted(KHE_HISTORY_SOLVER hs,
  KHE_HISTORY_INSTANCE_STAGE his, KHE_RESOURCE_NODE rn,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_TASK_NODE tn, int *c1, int *c2, int *c3)
{
  /* (1) return false if r is not assignable to t */
  if( DEBUG6 )
    fprintf(stderr, "edge from %s to %s: ", KheResourceId(rn->resource),
      KheTaskId(tn->task));
  HnAssert(KheTaskAsstResource(tn->task) == NULL,
    "KheEdgeIsWanted internal error (task is assigned)");
  /* ***
  r = KheTaskAsstResource(tn->task);
  if( r != NULL )
    ok1 = (r == rn->resource);
  else
  *** */
  if( !KheTaskAssignResourceCheck(tn->task, rn->resource) )
  {
    if( DEBUG6 )
      fprintf(stderr, "false (%s not assignable to %s)\n",
	KheResourceId(rn->resource), KheTaskId(tn->task));
    return *c1 = *c2 = *c3 = -1, false;
  }

  /* (2) return false if r is not unassigned on the days when t is running */
  if( !KheResourceTimetableMonitorFreeForTask(rtm, tn->task,
      hs->days_frame, NULL, false) )
  {
    if( DEBUG6 )
      fprintf(stderr, "false (%s not free on the days of %s)\n",
	KheResourceId(rn->resource), KheTaskId(tn->task));
    return *c1 = *c2 = *c3 = -1, false;
  }

  /* (3) return false if task takes the resource over its max limit */
  if( rn->history + his.tgi + KheIntervalLength(tn->interval) > rn->maximum )
  {
    if( DEBUG6 )
      fprintf(stderr, "false (%s over max limit if assigned to %s)\n",
	KheResourceId(rn->resource), KheTaskId(tn->task));
    return *c1 = *c2 = *c3 = -1, false;
  }

  /* (4) return false if task takes the resource over its busy days limit */
  if( his.tgi + KheIntervalLength(tn->interval) > rn->busy_days_limit )
  {
    if( DEBUG6 )
      fprintf(stderr, "false (%s over busy days limit if assigned to %s)\n",
	KheResourceId(rn->resource), KheTaskId(tn->task));
    return *c1 = *c2 = *c3 = -1, false;
  }

  /* all good, so set the costs and return true */
  if( DEBUG6 )
    fprintf(stderr, "true (rn asst_cost %.5f, rn non_asst_cost %.5f, "
      "tn asst_cost %.5f, tn non_asst_cost %.5f)\n",
      KheCostShow(rn->his_asst_cost), KheCostShow(rn->his_non_asst_cost),
      KheCostShow(tn->his_asst_cost), KheCostShow(tn->his_non_asst_cost));
  *c1 = 1;  /* so no edges have negative cost */
  *c2 = KheHardCost(rn->his_asst_cost) - KheHardCost(rn->his_non_asst_cost)
      + KheHardCost(tn->his_asst_cost) - KheHardCost(tn->his_non_asst_cost);
  *c3 = KheSoftCost(rn->his_asst_cost) - KheSoftCost(rn->his_non_asst_cost)
      + KheSoftCost(tn->his_asst_cost) - KheSoftCost(tn->his_non_asst_cost);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHistoryInstanceStageMakeAndSolve(                                */
/*    KHE_HISTORY_INSTANCE hi, int tgi, KHE_HISTORY_SOLVER hs)               */
/*                                                                           */
/*  Make and solve a history instance stage for hi and tgi.  Return          */
/*  true if any resources were active; false means it's time to stop.        */
/*                                                                           */
/*****************************************************************************/

static bool KheHistoryInstanceStageMakeAndSolve(
  KHE_HISTORY_INSTANCE hi, int tgi, KHE_HISTORY_SOLVER hs)
{
  KHE_HISTORY_INSTANCE_STAGE his;
  KHE_RESOURCE_NODE rn;  KHE_TASK_NODE tn;  int i, j, mult, c1, c2, c3;
  KHE_MMATCH_NODE sn;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;

  /* make a history instance stage object for hi and tgi */
  his = KheHistoryInstanceStageMake(hi, tgi, hs);
  if( DEBUG4 )
    fprintf(stderr, "  %d resources, time group %d: %s\n",
      HaArrayCount(hi->resource_nodes), tgi, KheTimeGroupId(his.class_tg));

  /* clear the match */
  KheMMatchClear(hs->his_match);

  /* find the resources; return now if there are no active resources */
  /* NB only unassigned active resources enter the matching */
  if( !KheHistoryInstanceStageInitResourcesForSolve(his, hs) )
    return false;

  /* make and add the supply (task) nodes */
  KheHistoryInstanceStageMakeAndInitTasksForSolve(his, hs);

  /* add the edges */
  HaArrayForEach(hs->his_resource_nodes, rn, i)
  {
    rtm = KheResourceTimetableMonitor(hs->soln, rn->resource);
    HaArrayForEach(hs->his_task_nodes, tn, j)
      if( KheEdgeIsWanted(hs, his, rn, rtm, tn, &c1, &c2, &c3) )
	KheMMatchAddEdge(rn->his_match_node, tn->his_match_node, 1, c1, c2, c3);
  }

  /* solve */
  KheMMatchSolve(hs->his_match);

  /* make one assignment for each edge of the maximum matching */
  /* and kill off any resources that failed to match */
  HaArrayForEach(hs->his_resource_nodes, rn, j)
    if( KheMMatchResultEdge(rn->his_match_node, &sn, &mult, &c1, &c2, &c3) )
    {
      /* assign rn->resource to tn->task */
      tn = (KHE_TASK_NODE) KheMMatchSupplyNodeBack(sn);
      HnAssert(tn->magic == KHE_TASK_NODE_MAGIC,
	"KheHistoryInstanceStageSolve internal error");
      KheTaskDoAssignResource(tn->task, rn->resource, hs);
      /* KheResourceNodeAssignTask(rn, tn, more_to_do); */
    }
    else
      rn->state = KHE_RESOURCE_STATE_FINISHED;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HISTORY_INSTANCE"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_HISTORY_INSTANCE KheHistoryInstanceMake(KHE_CONSTRAINT_CLASS cc,     */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make one instance of the assign by history problem, based on constraint  */
/*  class cc.                                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_HISTORY_INSTANCE KheHistoryInstanceMake(KHE_CONSTRAINT_CLASS cc,
  HA_ARENA a)
{
  KHE_HISTORY_INSTANCE res;
  HaMake(res, a);
  res->class = cc;
  HaArrayInit(res->resource_nodes, a);
  HaArrayInit(res->busy_days_classes, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHistoryInstanceAcceptsBusyDaysClass(KHE_HISTORY_INSTANCE hi,     */
/*    KHE_CONSTRAINT_CLASS busy_days_cc)                                     */
/*                                                                           */
/*  Return true if hi is linkable to busy_days_cc:  if hi's class's time     */
/*  groups are subsets of the corresponding time groups of busy_days_cc,     */
/*  and its weight is larger.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheHistoryInstanceAcceptsBusyDaysClass(KHE_HISTORY_INSTANCE hi,
  KHE_CONSTRAINT_CLASS busy_days_cc)
{
  int tg_count1, tg_count2, i;  KHE_TIME_GROUP tg1, tg2;
  KHE_POLARITY po1, po2;  KHE_COST weight1, weight2;

  /* check time groups */
  tg_count1 = KheConstraintClassTimeGroupCount(hi->class);
  tg_count2 = KheConstraintClassTimeGroupCount(busy_days_cc);
  HnAssert(tg_count1 == tg_count2,
    "KheHistoryInstanceAcceptsBusyDaysClass internal error");
  for( i = 0;  i < tg_count1;  i++ )
  {
    tg1 = KheConstraintClassTimeGroup(hi->class, i, &po1);
    tg2 = KheConstraintClassTimeGroup(busy_days_cc, i, &po2);
    if( !KheTimeGroupSubset(tg1, tg2) )
      return false;
  }

  /* check weight */
  weight1 = KheConstraintClassCombinedWeight(hi->class);
  weight2 = KheConstraintClassCombinedWeight(busy_days_cc);
  if( weight2 <= weight1 )
    return false;

  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceAddBusyDaysClass(KHE_HISTORY_INSTANCE hi,         */
/*    KHE_CONSTRAINT_CLASS busy_days_cc)                                     */
/*                                                                           */
/*  Add busy_days_cc to hi.                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryInstanceAddBusyDaysClass(KHE_HISTORY_INSTANCE hi,
  KHE_CONSTRAINT_CLASS busy_days_cc)
{
  HaArrayAddLast(hi->busy_days_classes, busy_days_cc);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHistoryInstanceAcceptsResource(KHE_HISTORY_INSTANCE hi,          */
/*    KHE_RESOURCE r, int *history, int *minimum)                            */
/*                                                                           */
/*  If hi accepts r (if r is admissible), set *history, *minimum, and        */
/*  *maximum, and return true.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheHistoryInstanceAcceptsResource(KHE_HISTORY_INSTANCE hi,
  KHE_RESOURCE r, int *history, int *minimum, int *maximum)
{
  *history = KheConstraintClassResourceHistory(hi->class, r);
  *minimum = KheConstraintClassResourceMinimum(hi->class, r);
  *maximum = KheConstraintClassResourceMaximum(hi->class, r);
  return 0 < *history && *history < *minimum;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceAddResource(KHE_HISTORY_INSTANCE hi,              */
/*    KHE_RESOURCE r, int history, HA_ARENA a)                               */
/*                                                                           */
/*  Add a resource node for r (with the given history) to hi.  Also          */
/*  initialize rn's busy days limit.                                         */
/*                                                                           */
/*  Implementation note:  hi->busy_days_classes contains only classes        */
/*  whose weight exceeds the weight of hi->class, as can be seen from        */
/*  KheHistoryInstanceAcceptsBusyDaysClass above.  So the busy days limit    */
/*  always applies, although there could be no busy days classes.            */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryInstanceAddResource(KHE_HISTORY_INSTANCE hi,
  KHE_RESOURCE r, int history, int minimum, int maximum, HA_ARENA a)
{
  int i, lim;  KHE_CONSTRAINT_CLASS cc;
  KHE_RESOURCE_NODE rn;

  /* add a resource node to hi */
  rn = KheResourceNodeMake(r, history, minimum, maximum, a);
  HaArrayAddLast(hi->resource_nodes, rn);

  /* work out rn's busy days limit */
  HaArrayForEach(hi->busy_days_classes, cc, i)
  {
    lim = KheConstraintClassResourceMaximumMinusHistory(cc, r);
    if( lim < rn->busy_days_limit )
      rn->busy_days_limit = lim;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceSolve(KHE_HISTORY_INSTANCE hi,                    */
/*    KHE_HISTORY_SOLVER hs)                                                 */
/*                                                                           */
/*  Solve hi.                                                                */
/*                                                                           */
/*****************************************************************************/
static void KheHistoryInstanceDebug(KHE_HISTORY_INSTANCE hi,
  int verbosity, int indent, FILE *fp);

static void KheHistoryInstanceSolve(KHE_HISTORY_INSTANCE hi,
  KHE_HISTORY_SOLVER hs)
{
  int tg_count, tgi;
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheHistoryInstanceSolve(hi, hs); cc =\n");
    KheHistoryInstanceDebug(hi, 2, 2, stderr);
  }
  tg_count = KheConstraintClassTimeGroupCount(hi->class);
  for( tgi = 0;  tgi < tg_count;  tgi++ )
    if( !KheHistoryInstanceStageMakeAndSolve(hi, tgi, hs) )
      break;
  if( DEBUG4 )
    fprintf(stderr, "] KheHistoryInstanceSolve returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistoryInstanceDebug(KHE_HISTORY_INSTANCE hi,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of hi onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHistoryInstanceDebug(KHE_HISTORY_INSTANCE hi,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_RESOURCE_NODE rn;  KHE_CONSTRAINT_CLASS busy_days_cc;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ HistoryInstance\n", indent, "");
    KheConstraintClassDebug(hi->class, verbosity, indent + 2, fp);
    HaArrayForEach(hi->resource_nodes, rn, i)
      KheResourceNodeDebug(rn, verbosity, indent + 2, fp);
    if( HaArrayCount(hi->busy_days_classes) > 0 )
    {
      fprintf(fp, "%*s  busy days classes:\n", indent, "");
      HaArrayForEach(hi->busy_days_classes, busy_days_cc, i)
        KheConstraintClassDebug(busy_days_cc, 1, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HISTORY_SOLVER"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)                    */
/*                                                                           */
/*  Debug function for printing an edge cost.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCostDebugFn(int c1, int c2, int c3, FILE *fp)
{
  fprintf(fp, "%d:%d:%d", c1, c2, c3);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_HISTORY_SOLVER KheHistorySolverMake(KHE_SOLN soln,                   */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa,       */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Make a new history grouper object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/
static void KheHistorySolverDebugFn(void *value, int verbosity,
  int indent, FILE *fp);

static KHE_HISTORY_SOLVER KheHistorySolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa,
  HA_ARENA a)
{
  KHE_HISTORY_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  res->etm = (KHE_EVENT_TIMETABLE_MONITOR) KheOptionsGetObject(options,
    "gs_event_timetable_monitor", NULL);
  res->soln_adjuster = sa;
  res->admissible_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  res->busy_days_ccf = KheConstraintClassFinderMake(rt, res->days_frame, a);
  HaArrayInit(res->history_instances, a);
  HaArrayInit(res->his_resource_nodes, a);
  HaArrayInit(res->his_task_nodes, a);
  res->his_match = KheMMatchMake((void *) res, &KheHistorySolverDebugFn,
    &KheResourceNodeDebugFn, &KheTaskNodeDebugFn, &KheCostDebugFn, a);
  HaArrayInit(res->tmp_indexes, a);
  res->tmp_task_set = KheTaskSetMake(soln);
  res->assts_made = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistorySolverDebug(KHE_HISTORY_SOLVER hs, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of hs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHistorySolverDebug(KHE_HISTORY_SOLVER hs, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s[ HistorySolver(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(hs->soln)),
    KheResourceTypeId(hs->resource_type));
  fprintf(fp, "%*s  admissible classes:\n", indent, "");
  KheConstraintClassFinderDebug(hs->admissible_ccf, verbosity, indent + 2, fp);
  fprintf(fp, "%*s  busy days classes:\n", indent, "");
  KheConstraintClassFinderDebug(hs->busy_days_ccf, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHistorySolverDebugFn(void *value, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for passing to the mmatch module.                         */
/*                                                                           */
/*****************************************************************************/

static void KheHistorySolverDebugFn(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_HISTORY_SOLVER hs;
  hs = (KHE_HISTORY_SOLVER) value;
  KheHistorySolverDebug(hs, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheAssignByHistory"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAssignByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,              */
/*    KHE_OPTIONS options, KHE_TASK_SET r_ts)                                */
/*                                                                           */
/*  Group tasks of type rt and assign them resources of type rt to handle    */
/*  history.  Return the number of groups made.                              */
/*                                                                           */
/*  Implementation note.  We add resource nodes to the history instances     */
/*  all at once, before doing any solving.  This is to ensure that each      */
/*  resource gets added to at most one history instance.                     */
/*                                                                           */
/*****************************************************************************/

int KheAssignByHistory(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  int count, offset, res, i, j, history, minimum, maximum;
  HA_ARENA a;  KHE_RESOURCE r;  KHE_CONSTRAINT_CLASS cc, busy_days_cc;
  KHE_INSTANCE ins;  KHE_CONSTRAINT c;  KHE_HISTORY_INSTANCE hi;
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;  KHE_HISTORY_SOLVER hs;

  /* make a history solver object */
  if( DEBUG1 )
    fprintf(stderr, "[ KheAssignByHistory(%s, %s, -, -)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));
  a = KheSolnArenaBegin(soln);
  hs = KheHistorySolverMake(soln, rt, options, sa, a);

  /* build constraint classes (both admissible classes and busy days classes) */
  ins = KheSolnInstance(hs->soln);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) != KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG ||
	KheConstraintCombinedWeight(c) == 0 )
      continue;								/* 1 */
    laic = (KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c;
    if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(laic, rt) > 0 &&
	KheLimitActiveIntervalsConstraintTimeGroupCount(laic) > 0 &&	/* 2 */
	KheLimitActiveIntervalsConstraintAllPositive(laic) )		/* 3 */
    {
      count = KheLimitActiveIntervalsConstraintAppliesToOffsetCount(laic);
      for( j = 0;  j < count;  j++ )
      {
	offset = KheLimitActiveIntervalsConstraintAppliesToOffset(laic, j);
	if( KheConstraintTimeGroupsSubsetFrame(c, offset,hs->days_frame) ) /*5*/
	{
	  if( KheConstraintTimeGroupsAllSingletons(c) )			/* 4 */
	    KheConstraintClassFinderAddConstraint(hs->admissible_ccf, c,offset);
	  else
	    KheConstraintClassFinderAddConstraint(hs->busy_days_ccf, c, offset);
	}
      }
    }
  }

  /* build one history instance for each admissible class */
  for( i = 0; i < KheConstraintClassFinderClassCount(hs->admissible_ccf); i++ )
  {
    /* build history instance hi */
    cc = KheConstraintClassFinderClass(hs->admissible_ccf, i);
    hi = KheHistoryInstanceMake(cc, a);
    HaArrayAddLast(hs->history_instances, hi);

    /* add busy days classes to hi */
    for( j = 0; j < KheConstraintClassFinderClassCount(hs->busy_days_ccf); j++ )
    {
      busy_days_cc = KheConstraintClassFinderClass(hs->busy_days_ccf, j);
      if( KheHistoryInstanceAcceptsBusyDaysClass(hi, busy_days_cc) )
	KheHistoryInstanceAddBusyDaysClass(hi, busy_days_cc);
    }
  }

  /* make and add resource nodes to the history instances; */
  /* each resource gets added to at most one history instance */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    HaArrayForEach(hs->history_instances, hi, j)
      if( KheHistoryInstanceAcceptsResource(hi, r, &history,&minimum,&maximum) )
      {
        KheHistoryInstanceAddResource(hi, r, history, minimum, maximum, a);
	break;
      }
  }
  if( DEBUG2 )
    KheHistorySolverDebug(hs, 2, 2, stderr);

  /* solve each history instance */
  HaArrayForEach(hs->history_instances, hi, j)
    KheHistoryInstanceSolve(hi, hs);

  /* all done */
  KheTaskSetDelete(hs->tmp_task_set);
  res = hs->assts_made;
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheAssignByHistory returning %d\n", res);
  return res;
}

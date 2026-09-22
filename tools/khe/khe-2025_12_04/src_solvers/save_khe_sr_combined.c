
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
/*  FILE:         khe_sr_combined.c                                          */
/*  DESCRIPTION:  Combined resource solvers                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1	0
#define DEBUG2	0		/* KheCombinedResourceAssign */
#define DEBUG3	0		/* KheSolverRun */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "checks"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDebugTimetable(KHE_SOLN soln, char *header)                      */
/*                                                                           */
/*  If DEBUG7, print the timetable of resource DEBUG7_ID, plus header.       */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheDebugTimetable(KHE_SOLN soln, char *header)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_INSTANCE ins;  KHE_RESOURCE r;
  if( DEBUG7 )
  {
    ins = KheSolnInstance(soln);
    if( KheInstanceRetrieveResource(ins, DEBUG7_ID, &r) )
    {
      fprintf(stderr, "  [ %s timetable %s\n", KheResourceId(r), header);
      rtm = KheResourceTimetableMonitor(soln, r);
      KheResourceTimetableMonitorPrintTimetable(rtm, 10, 4, stderr);
      fprintf(stderr, "  ]\n");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCheckTasks(KHE_SOLN soln, char *label)                       */
/*                                                                           */
/*  Check soln for inconsistent tasks; if found, print an error message      */
/*  including label.                                                         */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheSolnCheckTasks(KHE_SOLN soln, char *label)
{
  int i;  KHE_TASK task;  KHE_RESOURCE ass_r, pre_r;  KHE_EVENT e;
  char *role;  KHE_EVENT_RESOURCE er;  KHE_INSTANCE ins;
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);

    ** only interested in tasks derived from event resources **
    er = KheTaskEventResource(task);
    if( er == NULL )
      continue;

    ** get assigned and preassigned resources and check that they have Ids **
    role = KheEventResourceRole(er);
    e = KheEventResourceEvent(er);
    ass_r = KheTaskAsstResource(task);
    pre_r = KheEventResourcePreassignedResource(er);
    ins = KheEventInstance(e);
    HnAssert(ass_r == NULL || KheResourceId(ass_r) != NULL,
      "KheSolnCheckTasks %s: in instance %s, resource without Id assigned to "
      "event %s (role %s)", label, KheInstanceId(ins), KheEventId(e),
      role == NULL ? "(none)" : role);
    HnAssert(pre_r == NULL || KheResourceId(pre_r) != NULL,
      "KheSolnCheckTasks %s: in instance %s, resource without Id preassigned to"
      " event %s (role %s)", label, KheInstanceId(ins), KheEventId(e),
      role == NULL ? "(none)" : role);

    if( pre_r != NULL )
    {
      ** if preassigned, check that the assigned resource is equal to it **
      HnAssert(ass_r != NULL,
	"KheSolnCheckTasks %s: in event %s of instance %s, event resource with "
	"preassigned resource %s has task with missing resource assignment",
	label, KheEventId(e), KheInstanceId(ins), KheResourceId(pre_r));
      HnAssert(ass_r == pre_r,
	"KheSolnCheckTasks %s: in event %s of instance %s, event resource with "
	"preassigned resource %s has task with inconsistent assignment %s",
	label, KheEventId(e), KheInstanceId(ins), KheResourceId(pre_r),
	KheResourceId(ass_r));
    }
    else
    {
      ** if unpreassigned, must have a role **
      HnAssert(role != NULL, "KheSolnCheckTasks %s: in event %s of instance %s, "
	"unpreassigned event resource has no role", label, KheEventId(e),
	KheInstanceId(ins));
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheNoClashesCheck(KHE_SOLN soln, KHE_RESOURCE r)                    */
/*                                                                           */
/*  Make sure that r has no clashes.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn to save the cost of RTMClashingTimeCount
static void KheNoClashesCheck(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( r != NULL )
  {
    rtm = KheResourceTimetableMonitor(soln, r);
    HnAssert(KheResourceTimetableMonitorClashingTimeCount(rtm) == 0,
      "KheNoClashesCheck failed on resource %s", KheResourceId(r));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnCheckForClashes(KHE_SOLN soln)                               */
/*                                                                           */
/*  Check for clashes.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn to save the cost of RTMClashingTimeCount
static void KheSolnCheckForClashes(KHE_SOLN soln, char *message)
{
  int i;  KHE_RESOURCE r;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_INSTANCE ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    if( DEBUG8_ID != NULL && strcmp(KheResourceId(r), DEBUG8_ID) == 0 )
    {
      fprintf(stderr, "timetable for %s %s:\n", KheResourceId(r), message);
      rtm = KheResourceTimetableMonitor(soln, r);
      KheResourceTimetableMonitorPrintTimetable(rtm, 10, 0, stderr);
    }
    KheNoClashesCheck(soln, r);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "trying unassignments"                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool TryUnassign(KHE_TASK_SET ts, int first_index, int last_index)       */
/*                                                                           */
/*  Try unassigning tasks ts[first_index .. last_index].                     */
/*                                                                           */
/*****************************************************************************/

static bool TryUnassign(KHE_SOLN soln, KHE_TASK_SET ts, int first_index,
  int last_index)
{
  KHE_MARK mark;  int i;  bool success;  KHE_TASK task;  KHE_COST init_cost;
  KHE_RESOURCE r;

  /* try the unassignments, using a mark to undo them if unsuccessful */
  mark = KheMarkBegin(soln);
  success = true;
  init_cost = KheSolnCost(soln);
  for( i = first_index;  success && i <= last_index;  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( !KheTaskIsPreassigned(task, &r) )
      success = KheTaskUnAssign(task);
  }
  success = success && KheSolnCost(soln) < init_cost;
  KheMarkEnd(mark, !success);

  /* debug print if successful, and return */
  if( DEBUG1 && success )
  {
    fprintf(stderr, "  KheSolnTryTaskUnAssignments unassign ");
    for( i = first_index;  i <= last_index;  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( i > first_index )
	fprintf(stderr, ", ");
      KheTaskDebug(task, 1, -1, stderr);
    }
    fprintf(stderr, " success: %.5f -> %.5f\n", KheCostShow(init_cost),
      KheCostShow(KheSolnCost(soln)));
  }
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, KHE_OPTIONS options)     */
/*                                                                           */
/*  Try unassigning each proper root task of soln, to see if that improves   */
/*  the cost.  Return true if any unassignments were kept.                   */
/*                                                                           */
/*****************************************************************************/

bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, KHE_OPTIONS options)
{
  HA_ARENA a;  int i, j, unassign, max_unassign, /*, x1, x2, */ ts_count;
  KHE_TASK_SET ts;  KHE_INSTANCE ins;  KHE_RESOURCE r;  KHE_TASK_FINDER tf;
  KHE_INTERVAL in;  bool res;
  a = KheSolnArenaBegin(soln);
  max_unassign = KheOptionsGetInt(options, "rs_max_unassign", 1);
  if( DEBUG1 )
    fprintf(stderr, "[ KheSolnTryTaskUnAssignments(soln, max_unassign %d)\n",
      max_unassign);
  tf = KheTaskFinderMake(soln, options, a);
  res = false;
  ins = KheSolnInstance(soln);
  ts = KheTaskSetMake(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    KheFindTasksInInterval(tf, KheIntervalMake(0, KheTaskFinderLastIndex(tf)),
      KheResourceResourceType(r), r, true, false, ts, &in);
    ts_count = KheTaskSetTaskCount(ts);
    for( unassign = 1;  unassign <= max_unassign;  unassign++ )
    {
      for( j = 0;  j <= ts_count - unassign;  j++ )
	if( TryUnassign(soln, ts, j, j + unassign - 1) )
	  res = true;
    }
  }
  KheTaskSetDelete(ts);
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheSolnTryTaskUnAssignments returning %s\n",
      res ? "true" : "false");
  return res;
}


/* *** old version that does not consult rs_max_unassign option
bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, KHE_OPTIONS options)
{
  KHE_TASK task, target_task;  int i, j;  KHE_COST init_cost;  bool ok, res;
  KHE_TRACE trace;  KHE_MONITOR m;  KHE_RESOURCE r;
  if( DEBUG1 )
    fprintf(stderr, "[ KheSolnTryTaskUnAssignments(soln)\n");
  res = false;
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskProperRoot(task) == task )
    {
      target_task = KheTaskAsst(task);
      init_cost = KheSolnCost(soln);
      if( target_task != NULL && !KheTaskIsPreassigned(task, NULL) )
      {
	if( DEBUG10 )
	{
	  r = KheTaskAsstResource(task);
	  trace = KheTraceMake((KHE_GROUP_MONITOR) soln);
	  KheTraceBegin(trace);
	  ok = KheTaskUnAssign(task);
	  KheTraceEnd(trace);
	  fprintf(stderr, "  unassign %s from ",
	    r == NULL ? "?" : KheResourceId(r));
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, ":  %.5f -> %.5f\n",
	    KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
	  for( j = 0;  j < KheTraceMonitorCount(trace);  j++ )
	  {
	    m = KheTraceMonitor(trace, j);
	    fprintf(stderr, "      from %.5f to ",
	      KheCostShow(KheTraceMonitorInitCost(trace, j)));
	    KheMonitorDebug(m, 2, 0, stderr);
	  }
	  KheTraceDelete(trace);
	}
	else
	  ok = KheTaskUnAssign(task);
	if( ok )
	{
	  if( KheSolnCost(soln) < init_cost )
	  {
	    res = true;
	    if( DEBUG1 )
	    {
	      fprintf(stderr, "  %.5f -> %.5f for unassigning task ",
		KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
	      KheTaskDebug(task, 2, 0, stderr);
	    }
	  }
	  else
	    KheTaskAssign(task, target_task);
	}
      }
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheSolnTryTaskUnAssignments returning %s\n",
      res ? "true" : "false");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "putting it all together"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheTaskingLabel(KHE_TASKING tasking)                               */
/*                                                                           */
/*  Return a string description of the scope of tasking.                     */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static char *KheTaskingLabel(KHE_TASKING tasking)
{
  return KheTaskingResourceType(tasking) == NULL ? "All Resources" :
    KheResourceTypeId(KheTaskingResourceType(tasking)) == NULL ? "?" :
    KheResourceTypeId(KheTaskingResourceType(tasking));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDebugStage(KHE_TASKING tasking, char *stage, int verbosity)      */
/*                                                                           */
/*  Print a debug message showing the stage, and the solution cost.          */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheDebugStage(KHE_TASKING tasking, char *stage, int verbosity)
{
  fprintf(stderr, "  %s:\n", stage);
  KheSolnDebug(KheTaskingSoln(tasking), verbosity, 2, stderr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceDebugMonitor(KHE_OPTIONS options, char *intro,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug the nominated monitor.                                             */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static void KheResourceDebugMonitor(KHE_OPTIONS options, char *intro,
  int indent, FILE *fp)
{
  KHE_MONITOR m;
  m = (KHE_MONITOR) KheOptionsGetObject(options, "gs_debug_monitor", NULL);
  if( m != NULL )
  {
    fprintf(fp, "%*s%s: ", indent, "", intro);
    KheMonitorDebug(m, 1, 0, fp);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, char *fn_name)       */
/*                                                                           */
/*  Brief report of what is starting, for debugging.                         */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheSolveDebug
static void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, char *fn_name)
{
  KHE_INSTANCE ins;  KHE_TIMER timer;  char buff[20];
  if( DEBUG5 )
  {
    ins = KheSolnInstance(soln);
    fprintf(stderr, "%s div %d", KheInstanceId(ins), KheSolnD iversifier(soln));
    if( KheOptionsContainsTimer(options, "global", &timer) )
      fprintf(stderr, " after %s",
	KheTimeShow(KheTimerElapsedTime(timer), buff));
    fprintf(stderr, " calling %s\n", fn_name);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourcesGlobalCheck(KHE_SOLN soln, char *pos)             */
/*                                                                           */
/*  Global check of soln.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignResourcesGlobalCheck(KHE_SOLN soln, char *pos)
{
  KHE_MONITOR m;
  if( DEBUG7 )
  {
    if( !KheSolnRetrieveMonitor(soln, DEBUG7_ID, &m) )
      HnAbort("DebugCheck (khe_sr_combined.c) internal error 1");
    HnAssert(KheMonitorTag(m) == KHE_LIMIT_RESOURCES_MONITOR_TAG,
      "DebugCheck (khe_sr_combined.c) internal error 2");
    KheLimitResourcesMonitorCheck((KHE_LIMIT_RESOURCES_MONITOR) m, pos);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingRepairResources(KHE_TASKING tasking, KHE_OPTIONS options, */
/*    float time_limit_multiplier, int variant)                              */
/*                                                                           */
/*  Repair the assignments of tasking using all the tricks available.        */
/*  (Not forgetting that each repair algorithm can be turned off by          */
/*  an option.)  Return true if any improvement.                             */
/*                                                                           */
/*  The time limit (if present) is multiplied by time_limit_multiplier.      */
/*                                                                           */
/*  Parameter variant is passed to resource rematch to make it vary          */
/*  its behaviour on different calls on this function.  Actually it          */
/*  just causes it to sort things in reverse order, no big deal.             */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn in favour of the rs option
static bool KheTaskingRepairResources(KHE_TASKING tasking, KHE_OPTIONS options,
  float time_limit_multiplier, int variant)
{
  KHE_COST init_cost;  KHE_SOLN soln;  KHE_INSTANCE ins;
  KHE_RESOURCE_TYPE rt;
  float repair_time_limit;  KHE_TIMER repair_timer, rematch_timer;
  soln = KheTaskingSoln(tasking);
  if( DEBUG2 )
    KheDebugStage(tasking, "at start of KheTaskingRepairResources", 2);
  ** ***
  if( DEBUG8 )
    KheSolnCheckForClashes(soln, "at start of KheTaskingRepairResources");
  *** **

  ** set the repair and rematch timers **
  repair_time_limit = time_limit_multiplier * KheTimeFromString(
    KheOptionsGet(options, "rs_repair_time_limit", "-"));
  if( repair_time_limit >= 0.0 )
  {
    repair_timer = KheOptionsAddTimer(options, "repair", repair_time_limit);
    rematch_timer = KheOptionsAddTimer(options, "rematch",
      repair_time_limit / 2.0);
  }
  else
    repair_timer = rematch_timer = NULL;

  ** boilerplate **
  init_cost = KheSolnCost(soln);
  ins = KheSolnInstance(soln);
  rt = KheTaskingResourceType(tasking);

  ** rematch **
  if( DEBUG5 )
    KheSolveDebug(soln, options, "KheResourceRematch:");
  if( !KheOptionsGetBool(options, "rs_repair_rematch_off", false) )
    KheResourceRematch(soln, KheResourceTypeFullResourceGroup(rt), options,
      variant);
  ** ***
  if( DEBUG8 )
    KheSolnCheckForClashes(soln, "after KheResourceRematch");
  *** **
  if( rematch_timer != NULL )
    KheOptionsDeleteTimer(options, rematch_timer);
  if( DEBUG4 )
    KheSolnCheckTasks(KheTaskingSoln(tasking), "after KheResourceRematch");

  ** ejection chains **
  if( DEBUG5 )
    KheSolveDebug(soln, options, "KheEjectionChainRepairResources:");
  if( !KheOptionsGetBool(options, "rs_repair_ejection_off", false) )
    KheEjectionChainRepairResources(tasking, options);
  if( DEBUG4 )
    KheSolnCheckTasks(KheTaskingSoln(tasking),
      "after KheEjectionChainRepairResources");

  ** reassignment **
  ** KheDynamicResourceVLSNSolve(soln, rt, options); **

  if( KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE )
  {
    ** KheReassignRepair **
    if( DEBUG5 )
      KheSolveDebug(soln, options, "KheReassignRepair:");
    KheReassign1Repair(soln, rt, options);
    if( DEBUG4 )
      KheSolnCheckTasks(soln, "after KheReassignRepair");

    ** KheReassign2Repair **
    if( DEBUG5 )
      KheSolveDebug(soln, options, "KheReassign2Repair:");
    KheReassign2Repair(soln, rt, options);
    if( DEBUG4 )
      KheSolnCheckTasks(soln, "after KheReassign2Repair");

    ** KheReassign3Repair **
    if( DEBUG5 )
      KheSolveDebug(soln, options, "KheReassign3Repair:");
    KheReassign3Repair(soln, rt, options);
    if( DEBUG4 )
      KheSolnCheckTasks(soln, "after KheReassign3Repair");
  }

  ** wrapup **
  if( repair_timer != NULL )
    KheOptionsDeleteTimer(options, repair_timer);
  if( DEBUG6 && KheSolnCost(soln) < init_cost )
    fprintf(stderr, "KheTaskingRepairResources improvement (%.5f -> %.5f)\n",
      KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
  return KheSolnCost(soln) < init_cost;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDoTimeSweep(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,                 */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out a time sweep assignment, along with a few other odd things.    */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheDoTimeSweep(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  KHE_FRAME frame;
  frame = KheOptionsFrame(options, "gs_common_frame", soln);
  if( KheResourceTypeLimitResourcesCount(rt) > 0 )
    KheOptionsSetBool(options, "rs_rematch_matching_off", true);
  if( DEBUG5 )
    KheSolveDebug(soln, options, "KheSolnAssignRequestedResources:");
  if( DEBUG8 )
    KheFrameAssertNoClashes(frame);
  KheSolnAssignRequestedResources(soln, rt, options);
  KheDebugTimetable(soln, "after KheSolnAssignRequestedResources");
  if( DEBUG8 )
    KheFrameAssertNoClashes(frame);
  if( DEBUG4 )
    KheSolnCheckTasks(soln, "after Stage1 requested");
  if( DEBUG5 )
    KheSolveDebug(soln, options, "KheTimeSweepAssignResources:");
  KheTimeSweepAssignResources(soln, KheResourceTypeFullResourceGroup(rt),
    options);
  KheDebugTimetable(soln, "after KheTimeSweepAssignResources");
  if( DEBUG8 )
    KheFrameAssertNoClashes(frame);
  if( DEBUG4 )
    KheSolnCheckTasks(soln, "after Stage1 time sweep");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingAssignResourcesStage1(KHE_TASKING tasking,                */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign resources to the leader tasks of tasking (Stage 1):  assign       */
/*  most tasks preserving the resource assignment invariant.                 */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn in favour of the rs option
bool KheTaskingAssignResourcesStage1(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;  char *resource_assign;
  KHE_SOLN soln;  KHE_TASK_SET group_by_rc_ts, group_by_resource_ts;
  char *rs_multiplier;  char mult_str[101];  int mult_val;  KHE_RESOURCE r;
  bool rs_cluster_minimum;  KHE_CLUSTER_MINIMUM_SOLVER cms;  HA_ARENA a;
  ** KHE_DYNAMIC_RESOURCE_SOLVER drs; **
  if( DEBUG1 )
    fprintf(stderr, "[ KheTaskingAssignResourcesStage1(%s)\n",
      KheTaskingLabel(tasking));
  soln = KheTaskingSoln(tasking);
  rt = KheTaskingResourceType(tasking);
  KheDebugTimetable(soln, "at the start of Stage 1");
  if( DEBUG4 )
    KheSolnCheckTasks(soln, "at start of Stage1");
  ** ***
  if( DEBUG8 )
    KheSolnCheckForClashes(soln, "at start of Stage1");
  *** **

  ** maintain the resource assignment invariant **
  KheOptionsSetBool(options, "rs_invariant", true);

  ** rs_multiplier option **
  rs_multiplier = KheOptionsGet(options, "rs_multiplier", NULL);
  if( rs_multiplier != NULL )
  {
    if( sscanf(rs_multiplier, "%d:%100s", &mult_val, mult_str) != 2 )
      HnAbort("syntax error in rs_multiplier value \"%s\"", rs_multiplier);
    KheSetMonitorMultipliers(soln, mult_str, mult_val);
  }

  ** rs_cluster_minimum option **
  rs_cluster_minimum = KheOptionsGet(options, "rs_cluster_minimum", false);
  if( rs_cluster_minimum && rt != NULL )
  {
    a = KheSolnArenaBegin(soln);
    cms = KheClusterMinimumSolverMake(a);
    KheClusterMinimumSolverSolve(cms, soln, options, rt);
    KheClusterMinimumSolverSetBegin(cms);
    KheClusterMinimumSolverSetMulti(cms, KheResourceTypeFullResourceGroup(rt));
    KheClusterMinimumSolverSetEnd(cms, false);
    KheSolnArenaEnd(soln, a);
  }

  ** grouping by resource constraints **
  if( DEBUG5 )
    KheSolveDebug(soln, options, "before KheTaskingGroupByResourceConstraints");
  ** DebugStart(soln, options, "KheTaskingGroupByResourceConstraints"); **
  group_by_rc_ts = KheTaskSetMake(soln);
  KheGroupByResourceConstraints(soln, KheTaskingResourceType(tasking),
    options, group_by_rc_ts);
  ** ***
  if( DEBUG8 )
    KheSolnCheckForClashes(soln, "after KheGroupByResourceConstraints");
  *** **

  ** enforcing work patterns **
  ** *** incomplete and not used
  if( DEBUG5 )
    KheSolveDebug(soln, options, "before KheEnforceWorkPatterns");
  KheEnforceWorkPatterns(soln, KheTaskingResourceType(tasking), options);
  *** **

  ** consec solver test **
  ** ***
  if( DEBUG10 )
  {
    KHE_FRAME frame;  KHE_CONSEC_SOLVER cs;
    frame = KheOptionsFrame(options, "gs_common_frame", soln);
    cs = KheConsecSolverMake(soln, frame);
    KheConsecSolverDebug(cs, 2, 2, stderr);
  }
  *** **

  ** sort out the resource_assign options **
  ins = KheSolnInstance(soln);
  resource_assign = KheOptionsGet(options, "rs_constructor", "auto");
  if( strcmp(resource_assign, "auto") == 0 )
  {
    if( KheInstanceModel(ins) == KHE_MODEL_EMPLOYEE_SCHEDULE )
      resource_assign = "time_sweep";
    else if( rt != NULL && KheResourceTypeAvoidSplitAssignmentsCount(rt) == 0 )
      resource_assign = "most_constrained";
    else
      resource_assign = "resource_packing";
  }

  ** construction **
  if( DEBUG5 )
    KheSolveDebug(soln, options, "before %s:", resource_assign);
  ** DebugStart(soln, options, resource_assign); **
  if( strcmp(resource_assign, "none") == 0 )
  {
    ** do nothing **
  }
  else if( strcmp(resource_assign, "most_constrained") == 0 )
    KheMostConstrainedFirstAssignResources(tasking, options);
  else if( strcmp(resource_assign, "resource_packing") == 0 )
    KheResourcePackAssignResources(tasking, options);
  else if( strcmp(resource_assign, "time_sweep") == 0 )
  {
    KheDoTimeSweep(soln, rt, options);
    ** ***
    KheDynamicResourceVLSNTest(soln, rt, options);
    KheDynamicResourceVLSNSolve(soln, rt, options);
    *** **
  }
  ** ***
  else if( strcmp(resource_assign, "consec_packing") == 0 )
    KheResourcePackConsecutive(tasking, options);
  *** **
  else if( strcmp(resource_assign, "requested_only") == 0 )
    KheSolnAssignRequestedResources(soln, rt, options);
  else if( strcmp(resource_assign, "single_test") == 0 )
  {
    if( KheResourceTypeResourceCount(rt) > 0 )
    {
      r = KheResourceTypeResource(rt, 0);
      KheSingleResourceSolverTest(soln, options, r);
    }
    KheOptionsSetBool(options, "rs_repair_off", true);
    return true;
  }
  else if( strcmp(resource_assign, "dynamic_test") == 0 )
  {
    ** drs = ** KheDynamicResourceSolverMake(soln, rt, options);
    return true;
  }
  else
    HnAbort("KheTaskingAssignResourcesStage1: "
      "invalid resource_assign option \"%s\"", resource_assign);
  ** ***
  if( DEBUG8 )
    KheSolnCheckForClashes(soln, "after Stage 1 construction");
  *** **
  if( DEBUG4 )
    KheSolnCheckTasks(KheTaskingSoln(tasking), "after Stage1 construction");

  ** repair **
  if( !KheOptionsGetBool(options, "rs_repair_off", false) &&
      !KheOptionsGetBool(options, "rs_repair1_off", false) )
  {
    ** repair without grouping by resource **
    if( DEBUG3 )
      KheResourceDebugMonitor(options, "before Stage1 repair", 2, stderr);
    KheTaskingRepairResources(tasking, options, 1.0, 0);

    ** optional repair with grouping by resource **
    if( KheOptionsGetBool(options, "rs_group_by_resource", false) )
    {
      group_by_resource_ts = KheTaskSetMake(soln);
      if( KheTaskingGroupByResource(tasking, options, group_by_resource_ts) )
	KheTaskingRepairResources(tasking, options, 0.5, 1);
      KheTaskSetUnGroup(group_by_resource_ts);
      KheTaskSetDelete(group_by_resource_ts);
    }
  }

  ** ungroup the grouping by resource constraints **
  if( !KheOptionsGetBool(options, "rs_ungroup_off", false) )
  {
    KheTaskSetUnGroup(group_by_rc_ts);
    KheTaskSetDelete(group_by_rc_ts);
  }

  ** undo the rs_multiplier option **
  if( rs_multiplier != NULL )
    KheSetMonitorMultipliers(soln, mult_str, 1);

  KheDebugTimetable(soln, "at the end of Stage 1");
  if( DEBUG1 )
  {
    KheDebugStage(tasking, "at end of Stage1", 2);
    fprintf(stderr, "] KheTaskingAssignResourcesStage1 returning\n");
  }
  if( DEBUG4 )
    KheSolnCheckTasks(KheTaskingSoln(tasking), "at end of Stage1");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingAssignResourcesStage2(KHE_TASKING tasking,                */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign resources to the leader tasks of tasking (Stage 2):  find split   */
/*  assignments, still preserving the resource assignment invariant.  This   */
/*  stage does nothing if there are no avoid split assignments constraints.  */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn in favour of the rs option
bool KheTaskingAssignResourcesStage2(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KHE_RESOURCE_TYPE rt;  KHE_SOLN soln;

  rt = KheTaskingResourceType(tasking);
  if( KheResourceTypeAvoidSplitAssignmentsCount(rt) > 0 &&
      !KheOptionsGetBool(options, "rs_repair_off", false) &&
      !KheOptionsGetBool(options, "rs_repair2_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr, "[ KheTaskingAssignResourcesStage2(%s)\n",
	KheTaskingLabel(tasking));
    soln = KheTaskingSoln(tasking);
    KheDebugTimetable(soln, "at the start of Stage 2");
    KheFindSplitResourceAssignments(tasking, options);
    KheTaskingAllowSplitAssignments(tasking, false);
    if( DEBUG3 )
      KheResourceDebugMonitor(options, "monitor before Stage2 repair",2,stderr);
    KheTaskingRepairResources(tasking, options, 0.5, 0);
    if( DEBUG1 )
    {
      KheDebugStage(tasking, "at end of Stage2", 2);
      fprintf(stderr, "] KheTaskingAssignResourcesStage2 returning\n");
    }
  }
  if( DEBUG4 )
    KheSolnCheckTasks(soln, "at end of Stage2");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingAssignResourcesStage3(KHE_TASKING tasking,                */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Assign resources to the leader tasks of tasking (Stage 2):  last-ditch   */
/*  assignments, without the resource assignment invariant.                  */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn in favour of the rs option
bool KheTaskingAssignResourcesStage3(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KHE_SOLN soln;
  if( !KheOptionsGetBool(options, "rs_repair_off", false) &&
      !KheOptionsGetBool(options, "rs_repair3_off", false) )
  {
    if( DEBUG1 )
      fprintf(stderr, "[ KheTaskingAssignResourcesStage3(%s)\n",
	KheTaskingLabel(tasking));
    soln = KheTaskingSoln(tasking);
    KheDebugTimetable(soln, "at the start of Stage 3");
    ** ***
    if( DEBUG8 )
      KheSolnCheckForClashes(soln, "at start of Stage3");
    *** **
    KheOptionsSetBool(options, "rs_invariant", false);
    ** ***
    KheTaskingEnlargeDomains(tasking, false);
    *** **
    if( DEBUG3 )
      KheResourceDebugMonitor(options, "before Stage3 repair", 2, stderr);
    ** KheOptionsSetBool(options, "es_fresh_visits", true); **
    KheTaskingRepairResources(tasking, options, 0.5, 1);
    ** ***
    if( DEBUG4 )
      KheSolnCheckTasks(soln, "second repair in Stage3");
    KheTaskingRepairResources(tasking, options, 0.5, 1);
    *** **
    KheDebugTimetable(soln, "at the end of Stage 3");
    if( DEBUG4 )
      KheSolnCheckTasks(soln, "at end of Stage3");
    if( DEBUG1 )
    {
      KheDebugStage(tasking, "at end of Stage3", 2);
      fprintf(stderr, "] KheTaskingAssignResourcesStage3 returning\n");
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingAssignResources(KHE_TASKING tasking, KHE_OPTIONS options) */
/*                                                                           */
/*  Assign resources to the leader tasks of tasking.                         */
/*                                                                           */
/*****************************************************************************/

/* *** not used; was not in khe.h
bool KheTaskingAssignResources(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KheTaskingAssignResourcesStage1(tasking, options);
  KheTaskingAssignResourcesStage2(tasking, options);
  KheTaskingAssignResourcesStage3(tasking, options);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheCombinedResourceAssign"                                    */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SOLVER_RT,
  KHE_SOLVER_RTC,	/* converted version of RT, done when reading */
  KHE_SOLVER_RDO,
  KHE_SOLVER_RSM,
  KHE_SOLVER_RCM,
  KHE_SOLVER_RGC,
  KHE_SOLVER_RGR,
  KHE_SOLVER_RED,
  KHE_SOLVER_RIN,
  KHE_SOLVER_RRQ,
  KHE_SOLVER_RMC,
  KHE_SOLVER_RPK,
  KHE_SOLVER_RTS,
  KHE_SOLVER_RCX,
  KHE_SOLVER_RFS,
  KHE_SOLVER_RDV,
  KHE_SOLVER_RDT,
  KHE_SOLVER_RRM,
  KHE_SOLVER_REC,
  KHE_SOLVER_RRS,
  KHE_SOLVER_RRP
} KHE_SOLVER_ID;

typedef struct khe_solver_item_rec *KHE_SOLVER_ITEM;
typedef HA_ARRAY(KHE_SOLVER_ITEM) ARRAY_KHE_SOLVER_ITEM;

typedef struct khe_solver_rec *KHE_SOLVER;
typedef HA_ARRAY(KHE_SOLVER) ARRAY_KHE_SOLVER;

struct khe_solver_item_rec {
  int			time_weight;
  KHE_SOLVER_ID		id;
  KHE_SOLVER		solver;
  KHE_TASKING		rtc_tasking;   /* used only by RTC type */
};

struct khe_solver_rec {
  ARRAY_KHE_SOLVER_ITEM	items;
};


/*****************************************************************************/
/*                                                                           */
/*  bool KheNextToken(char **str, char *delimiter, char **val)               */
/*                                                                           */
/*  Search *str for the next token, defined to be everything up to but       */
/*  not including delimiter, or if that is not found, then end of string.    */
/*  If there is a token present, set *val to it (this will be just the       */
/*  initial value of *str), update *str to point to the start of the next    */
/*  token, and return true.  Otherwise set *val to NULL and return false.    */
/*                                                                           */
/*  An empty token followed by a delimiter is returned in *val.  An          */
/*  empty token at the end of the string is ignored.                         */
/*                                                                           */
/*  And yes, I know that strtok_r does essentially the same thing.           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheNextToken(char **str, char *delimiter, char **val)
{
  char *tmp;
  if( **str == '\0' )
  {
    ** we're already at end of string, so no token; return false **
    *val = NULL;
    return false;
  }
  else
  {
    *val = *str;
    tmp = strstr(*str, delimiter);
    if( tmp != NULL )
    {
      ** token is followed by delimiter **
      *tmp = '\0';
      *str = tmp + strlen(delimiter);
    }
    else
    {
      ** token is followed by end of string **
      *str += strlen(*str);
    }
    return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSkipWhiteSpace(char **str)                                       */
/*                                                                           */
/*  Move *str past any initial white space.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheSkipWhiteSpace(char **str)
{
  char ch;
  ch = **str;
  while( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' )
  {
    *str += 1;
    ch = **str;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSkipChar(char **str, char ch)                                    */
/*                                                                           */
/*  Skip ch and any following white space.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheSkipChar(char **str, char ch)
{
  if( **str != ch )
    HnAbort("expected but did not find `%c` in rs option string", ch);
  *str += 1;
  KheSkipWhiteSpace(str);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseChar(char **str, char *ch)                                  */
/*                                                                           */
/*  Parse one character.                                                     */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheParseChar(char **str, char *ch)
{
  if( **str == '\0' )
    return false;
  *ch = **str;
  *str += 1;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseId(char **str, char buff[20])                               */
/*                                                                           */
/*  Parse an identifier and any following white space.                       */
/*                                                                           */
/*****************************************************************************/

#define is_letter(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

static bool KheParseId(char **str, char buff[20])
{
  int i;
  i = 0;
  while( is_letter(**str) && i < 20 )
  {
    buff[i++] = **str;
    *str += 1;
  }
  buff[i++] = '\0';
  KheSkipWhiteSpace(str);
  return i >= 2;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverIdParse(char **str, KHE_SOLVER_ID *res)                    */
/*                                                                           */
/*  Parse a solver id and any following white space.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheSolverIdParse(char **str, KHE_SOLVER_ID *res)
{
  char buff[20];
  if( !KheParseId(str, buff) )
    return *res = KHE_SOLVER_RT, false;
  else if( strcmp(buff, "rt") == 0 )
    return *res = KHE_SOLVER_RT, true;
  else if( strcmp(buff, "rdo") == 0 )
    return *res = KHE_SOLVER_RDO, true;
  else if( strcmp(buff, "rsm") == 0 )
    return *res = KHE_SOLVER_RSM, true;
  else if( strcmp(buff, "rcm") == 0 )
    return *res = KHE_SOLVER_RCM, true;
  else if( strcmp(buff, "rgc") == 0 )
    return *res = KHE_SOLVER_RGC, true;
  else if( strcmp(buff, "rgr") == 0 )
    return *res = KHE_SOLVER_RGR, true;
  else if( strcmp(buff, "red") == 0 )
    return *res = KHE_SOLVER_RED, true;
  else if( strcmp(buff, "rin") == 0 )
    return *res = KHE_SOLVER_RIN, true;
  else if( strcmp(buff, "rrq") == 0 )
    return *res = KHE_SOLVER_RRQ, true;
  else if( strcmp(buff, "rmc") == 0 )
    return *res = KHE_SOLVER_RMC, true;
  else if( strcmp(buff, "rpk") == 0 )
    return *res = KHE_SOLVER_RPK, true;
  else if( strcmp(buff, "rts") == 0 )
    return *res = KHE_SOLVER_RTS, true;
  else if( strcmp(buff, "rcx") == 0 )
    return *res = KHE_SOLVER_RCX, true;
  else if( strcmp(buff, "rfs") == 0 )
    return *res = KHE_SOLVER_RFS, true;
  else if( strcmp(buff, "rdv") == 0 )
    return *res = KHE_SOLVER_RDV, true;
  else if( strcmp(buff, "rdt") == 0 )
    return *res = KHE_SOLVER_RDT, true;
  else if( strcmp(buff, "rrm") == 0 )
    return *res = KHE_SOLVER_RRM, true;
  else if( strcmp(buff, "rec") == 0 )
    return *res = KHE_SOLVER_REC, true;
  else if( strcmp(buff, "rrs") == 0 )
    return *res = KHE_SOLVER_RRS, true;
  else if( strcmp(buff, "rrp") == 0 )
    return *res = KHE_SOLVER_RRP, true;
  else
    return *res = KHE_SOLVER_RT, false;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSolverIdShow(KHE_SOLVER_ID id)                                  */
/*                                                                           */
/*  Return a string representation of id.                                    */
/*                                                                           */
/*****************************************************************************/

static char *KheSolverIdShow(KHE_SOLVER_ID id)
{
  switch( id )
  {
    case KHE_SOLVER_RT:		return "rt";
    case KHE_SOLVER_RTC:	return "rtc";
    case KHE_SOLVER_RDO:	return "rdo";
    case KHE_SOLVER_RSM:	return "rsm";
    case KHE_SOLVER_RCM:	return "rcm";
    case KHE_SOLVER_RGC:	return "rgc";
    case KHE_SOLVER_RGR:	return "rgr";
    case KHE_SOLVER_RED:	return "red";
    case KHE_SOLVER_RIN:	return "rin";
    case KHE_SOLVER_RRQ:	return "rrq";
    case KHE_SOLVER_RMC:	return "rmc";
    case KHE_SOLVER_RPK:	return "rpk";
    case KHE_SOLVER_RTS:	return "rts";
    case KHE_SOLVER_RCX:	return "rcx";
    case KHE_SOLVER_RFS:	return "rfs";
    case KHE_SOLVER_RDV:	return "rdv";
    case KHE_SOLVER_RDT:	return "rdt";
    case KHE_SOLVER_RRM:	return "rrm";
    case KHE_SOLVER_REC:	return "rec";
    case KHE_SOLVER_RRS:	return "rrs";
    case KHE_SOLVER_RRP:	return "rrp";
    default:			return "??";
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheParseInt(char **str, int *val)                                   */
/*                                                                           */
/*  Parse an integer and any following white space.                          */
/*                                                                           */
/*****************************************************************************/

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static void KheParseInt(char **str, int *val)
{
  sscanf(*str, "%d", val);
  while( is_digit(**str) )
    *str += 1;
  KheSkipWhiteSpace(str);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER KheSolverMake(HA_ARENA a)                                     */
/*                                                                           */
/*  Make and return a new solver.                                            */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER KheSolverMake(HA_ARENA a)
{
  KHE_SOLVER res;
  HaMake(res, a);
  HaArrayInit(res->items, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLVER_ITEM KheSolverItemMake(int time_weight, KHE_SOLVER_ID id,     */
/*    KHE_SOLVER solver, KHE_TASKING rtc_tasking, HA_ARENA a)                */
/*                                                                           */
/*  Make and return a new solver item.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLVER_ITEM KheSolverItemMake(int time_weight, KHE_SOLVER_ID id,
  KHE_SOLVER solver, KHE_TASKING rtc_tasking, HA_ARENA a)
{
  KHE_SOLVER_ITEM res;
  HaMake(res, a);
  res->time_weight = time_weight;
  res->id = id;
  res->solver = solver;
  res->rtc_tasking = rtc_tasking;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSolverItemTimeWeightCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Comparison function for sorting solver items by increasing time weight.  */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static int KheSolverItemTimeWeightCmp(const void *t1, const void *t2)
{
  KHE_SOLVER_ITEM si1 = * (KHE_SOLVER_ITEM *) t1;
  KHE_SOLVER_ITEM si2 = * (KHE_SOLVER_ITEM *) t2;
  return si1->time_weight - si2->time_weight;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseItem(char **str, KHE_SOLVER_ITEM *res)                      */
/*                                                                           */
/*  Parse one item and any following white space, according to grammar       */
/*                                                                           */
/*    <item>    ::=  [ <int> ":" ] <id> [ "(" <solver> ")" ]                 */
/*                                                                           */
/*****************************************************************************/
static bool KheParseSolver(char **str, KHE_SOLN soln, HA_ARENA a,
  KHE_SOLVER *res);

static bool KheParseItem(char **str, KHE_SOLN soln,
  HA_ARENA a, KHE_SOLVER_ITEM *res)
{
  int i, time_weight;  KHE_SOLVER_ID id;  KHE_SOLVER solver, rdo_solver;
  KHE_TASKING rtc_tasking;  KHE_SOLVER_ITEM item;  KHE_RESOURCE_TYPE rt;

  /* [ <int> : ] */
  if( is_digit(**str) )
  {
    KheParseInt(str, &time_weight);
    KheSkipChar(str, ':');
  }
  else
    time_weight = 1;

  /* id */
  if( !KheSolverIdParse(str, &id) )
    return false;

  /* [ "(" <solver> ")" ] */
  if( **str == '(' )
  {
    KheSkipChar(str, '(');
    if( !KheParseSolver(str, soln, a, &solver) )
      return false;
    KheSkipChar(str, ')');
  }
  else
    solver = NULL;

  /* create result and return true */
  if( id == KHE_SOLVER_RT )
  {
    /* convert rt(<solver>) into rdo(rtc(<solver>), ..., rtc(<solver>)) */
    rdo_solver = KheSolverMake(a);
    for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
    {
      rtc_tasking = KheSolnTasking(soln, i);
      rt = KheTaskingResourceType(rtc_tasking);
      if( KheResourceTypeResourceCount(rt) > 0 )
      {
	item = KheSolverItemMake(KheResourceTypeResourceCount(rt),
	  KHE_SOLVER_RTC, solver, rtc_tasking, a);
	HaArrayAddLast(rdo_solver->items, item);
      }
      /* HaArraySort(rdo_solver->items, &KheSolverItemTimeWeightCmp); */
    }
    *res = KheSolverItemMake(time_weight, KHE_SOLVER_RDO, rdo_solver, NULL, a);
  }
  else
    *res = KheSolverItemMake(time_weight, id, solver, NULL, a);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheParseSolver(char **str, HA_ARENA a, KHE_SOLVER *res)             */
/*                                                                           */
/*  Parse one <solver> and any following white space, according to grammar   */
/*                                                                           */
/*    <solver>  ::=  <item> { "," <item> }                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheParseSolver(char **str, KHE_SOLN soln, HA_ARENA a,
  KHE_SOLVER *res)
{
  KHE_SOLVER_ITEM item;
  *res = KheSolverMake(a);
  KheSkipWhiteSpace(str);
  if( !KheParseItem(str, soln, a, &item) )
    return false;
  HaArrayAddLast((*res)->items, item);
  while( **str == ',' )
  {
    KheSkipChar(str, ',');
    if( !KheParseItem(str, soln, a, &item) )
      return false;
    HaArrayAddLast((*res)->items, item);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheRemainingTimeWeight(KHE_SOLVER solver, int i)                     */
/*                                                                           */
/*  Return the remaining time weight for items from i inclusive to the end.  */
/*                                                                           */
/*****************************************************************************/

static int KheRemainingTimeWeight(KHE_SOLVER solver, int i)
{
  int j, res;  KHE_SOLVER_ITEM item;
  res = 0;
  for( j = i;  j < HaArrayCount(solver->items);  j++ )
  {
    item = HaArray(solver->items, j);
    res += item->time_weight;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverItemRun(KHE_SOLVER_ITEM item, KHE_RESOURCE_TYPE rt,        */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)    */
/*                                                                           */
/*  Run item.                                                                */
/*                                                                           */
/*****************************************************************************/
static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth);

static bool KheSolverItemRun(KHE_SOLVER_ITEM item, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)
{
  bool res;  KHE_TASK_SET task_set;
  char *rs_multiplier;  char mult_str[101];  int mult_val;
  KHE_CLUSTER_MINIMUM_SOLVER cms;  HA_ARENA a;
  switch( item->id )
  {
    case KHE_SOLVER_RT:

      HnAbort("KheSolverItemRun: unexpected type KHE_SOLVER_RT");
      return false;

    case KHE_SOLVER_RTC:

      tasking = item->rtc_tasking;
      rt = KheTaskingResourceType(tasking);
      return KheSolverRun(item->solver, rt, tasking, soln, options, depth);

    case KHE_SOLVER_RDO:

      return KheSolverRun(item->solver, rt, tasking, soln, options, depth);

    case KHE_SOLVER_RSM:

      rs_multiplier = KheOptionsGet(options, "rs_multiplier", NULL);
      HnAssert(rs_multiplier != NULL, "rsm item in rs option has no "
	"corresponding rs_multiplier option");
      if( sscanf(rs_multiplier, "%d:%100s", &mult_val, mult_str) != 2 )
	HnAbort("syntax error in rs_multiplier value \"%s\"", rs_multiplier);
      KheSetMonitorMultipliers(soln, mult_str, mult_val);
      res = KheSolverRun(item->solver, rt, tasking, soln, options, depth);
      KheSetMonitorMultipliers(soln, mult_str, 1);
      return res;

    case KHE_SOLVER_RCM:

      HnAssert(rt != NULL, "rcm item in rs option is outside rt()");
      a = KheSolnArenaBegin(soln);
      cms = KheClusterMinimumSolverMake(a);
      KheClusterMinimumSolverSolve(cms, soln, options, rt);
      KheClusterMinimumSolverSetBegin(cms);
      KheClusterMinimumSolverSetMulti(cms,
	KheResourceTypeFullResourceGroup(rt));
      KheClusterMinimumSolverSetEnd(cms, false);
      KheSolnArenaEnd(soln, a);
      return true;

    case KHE_SOLVER_RGC:

      task_set = KheTaskSetMake(soln);
      KheGroupByResourceConstraints(soln, rt, options, task_set);
      res = KheSolverRun(item->solver, rt, tasking, soln, options, depth);
      KheTaskSetUnGroup(task_set);
      KheTaskSetDelete(task_set);
      return res;

    case KHE_SOLVER_RGR:

      task_set = KheTaskSetMake(soln);
      if( KheTaskingGroupByResource(tasking, options, task_set) )
      {
	res = KheSolverRun(item->solver, rt, tasking, soln, options, depth);
	/* KheTaskingRepairResources(tasking, options, 0.5, 1); */
	KheTaskSetUnGroup(task_set);
	KheTaskSetDelete(task_set);
      }
      else
	res = false;
      return res;

    case KHE_SOLVER_RED:

      KheTaskingEnlargeDomains(tasking, false);
      return true;

    case KHE_SOLVER_RIN:

      KheOptionsSetBool(options, "rs_invariant", true);
      res = KheSolverRun(item->solver, rt, tasking, soln, options, depth);
      KheOptionsSetBool(options, "rs_invariant", false);
      return res;

    case KHE_SOLVER_RRQ:

      return KheSolnAssignRequestedResources(soln, rt, options);

    case KHE_SOLVER_RMC:

      return KheMostConstrainedFirstAssignResources(tasking, options);

    case KHE_SOLVER_RPK:

      return KheResourcePackAssignResources(tasking, options);

    case KHE_SOLVER_RTS:

      return KheTimeSweepAssignResources(soln,
	KheResourceTypeFullResourceGroup(rt), options);

    case KHE_SOLVER_RCX:

      if( KheInstanceModel(KheSolnInstance(soln))==KHE_MODEL_EMPLOYEE_SCHEDULE )
	return KheTimeSweepAssignResources(soln,
	  KheResourceTypeFullResourceGroup(rt), options);
      else if( rt != NULL && KheResourceTypeAvoidSplitAssignmentsCount(rt)==0 )
	return KheMostConstrainedFirstAssignResources(tasking, options);
      else
	return KheResourcePackAssignResources(tasking, options);

    case KHE_SOLVER_RFS:

      if( KheResourceTypeAvoidSplitAssignmentsCount(rt) > 0 )
      {
	res = KheFindSplitResourceAssignments(tasking, options);
	KheTaskingAllowSplitAssignments(tasking, false);
	return res;
      }
      else
	return true;

    case KHE_SOLVER_RDV:

      return KheDynamicResourceVLSNSolve(soln, rt, options);

    case KHE_SOLVER_RDT:

      KheDynamicResourceVLSNTest(soln, rt, options);
      return false;

    case KHE_SOLVER_RRM:

      return KheResourceRematch(soln, KheResourceTypeFullResourceGroup(rt),
        options, 0);

    case KHE_SOLVER_REC:

      return KheEjectionChainRepairResources(tasking, options);

    case KHE_SOLVER_RRS:

      return KheResourcePairRepair(tasking, options);

    case KHE_SOLVER_RRP:

      return KheReassign1Repair(soln, rt, options);

    default:

      HnAbort("KheSolverItemRun: internal error (id %d)", item->id);
      return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolverRun(KHE_SOLVER solver, KHE_RESOURCE_TYPE rt,               */
/*    KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)    */
/*                                                                           */
/*  Run solver on these attributes.                                          */
/*                                                                           */
/*****************************************************************************/
static void KheSolverDebug(KHE_SOLVER solver, FILE *fp);
static void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp);

static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)
{
  KHE_SOLVER_ITEM item;  int i, rem_time_weight;  float rem_time, avail_time;
  char buff[20], buff2[20];  bool with_timer, res;  KHE_TIMER timer;
  KHE_COST cost;

  /* quit now if there is no solver */
  if( s == NULL )
    return false;

  /* decide whether a timer is needed or not */
  with_timer = (KheOptionsRemainingTime(options) != KHE_NO_TIME &&
    HaArrayCount(s->items) >= 2);

  /* run each item with a non-zero time weight */
  timer = NULL;  avail_time = -1.0;  /* these two to keep compiler happy */
  res = false;
  HaArrayForEach(s->items, item, i)
    if( item->time_weight > 0 )
    {
      if( with_timer )
      {
	rem_time = KheOptionsRemainingTime(options);
	if( rem_time == 0.0 )
	  break;
	rem_time_weight = KheRemainingTimeWeight(s, i);
	avail_time = (rem_time * item->time_weight) / rem_time_weight;
	sprintf(buff, "cr%d", depth);
	timer = KheOptionsAddTimer(options, buff, avail_time);
      }
      if( DEBUG3 )
      {
	fprintf(stderr, "%*s[ ", depth * 2, "");
        KheSolverItemDebug(item, stderr);
	if( with_timer )
	  fprintf(stderr, ", %s available", KheTimeShow(avail_time, buff2));
	fprintf(stderr, "\n");
      }
      cost = KheSolnCost(soln);
      if( KheSolverItemRun(item, rt, tasking, soln, options, depth + 1) )
	res = true;
      if( DEBUG3 )
      {
	fprintf(stderr, "%*s] %s end", depth*2, "", KheSolverIdShow(item->id));
	if( with_timer )
	  fprintf(stderr, ", %s used",
	    KheTimeShow(KheTimerElapsedTime(timer), buff2));
	if( KheSolnCost(soln) < cost && item->id >= KHE_SOLVER_RRQ )
	  fprintf(stderr, " (new best, %.5f < %.5f)",
	    KheCostShow(KheSolnCost(soln)), KheCostShow(cost));
	fprintf(stderr, "\n");
      }
      if( with_timer )
	KheOptionsDeleteTimer(options, timer);
    }
  return res;
}


/* *** old version
static bool KheSolverRun(KHE_SOLVER s, KHE_RESOURCE_TYPE rt,
  KHE_TASKING tasking, KHE_SOLN soln, KHE_OPTIONS options, int depth)
{
  KHE_SOLVER_ITEM item;  int i, rem_time_weight;  float rem_time, avail_time;
  char buff[20], buff2[20];  bool res;  KHE_TIMER timer;  KHE_COST cost;

  ** quit now if there is no solver **
  if( s == NULL )
    return false;

  ** ***
  if( DEBUG3 )
  {
    fprintf(stderr, "%*s[ KheSolverRun(", depth * 2, "");
    KheSolverDebug(s, stderr);
    fprintf(stderr, ", %s)\n", rt == NULL ? "-" : KheResourceTypeId(rt));
  }
  *** **

  ** two cases:  with a time limit, and without a time limit **
  rem_time = KheOptionsRemainingTime(options);
  res = false;
  if( rem_time != KHE_NO_TIME && HaArrayCount(s->items) >= 2 )
  {
    ** run each item with its own time limit **
    HaArrayForEach(s->items, item, i)
      if( item->time_weight > 0 )
      {
	rem_time = KheOptionsRemainingTime(options);
	if( rem_time == 0.0 )
	  break;
	rem_time_weight = KheRemainingTimeWeight(s, i);
	avail_time = (rem_time * item->time_weight) / rem_time_weight;
	sprintf(buff, "cr%d", depth);
	timer = KheOptionsAddTimer(options, buff, avail_time);
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s[ %s: KheSolverItemRun(", depth * 2, "",
	    KheTimeShow(avail_time, buff2));
	  KheSolverItemDebug(item, stderr);
	  fprintf(stderr, ")\n");
	}
	cost = KheSolnCost(soln);
	if( KheSolverItemRun(item, rt, tasking, soln, options, depth + 1) )
	  res = true;
	if( DEBUG3 )
	{
	  if( KheSolnCost(soln) < cost && item->id >= KHE_SOLVER_RRQ )
	    fprintf(stderr, "%*s] (new best, %.5f < %.5f)\n", depth * 2, "",
	      KheCostShow(KheSolnCost(soln)), KheCostShow(cost));
	  else
	    fprintf(stderr, "%*s]\n", depth * 2, "");
	}
	KheOptionsDeleteTimer(options, timer);
      }
  }
  else
  {
    ** run each item without any additional time limit **
    HaArrayForEach(s->items, item, i)
      if( item->time_weight > 0 )
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s[ KheSolverItemRun(", depth * 2, "");
	  KheSolverItemDebug(item, stderr);
	  fprintf(stderr, ")\n");
	}
	if( KheSolverItemRun(item, rt, tasking, soln, options, depth + 1) )
	  res = true;
	if( DEBUG3 )
	  fprintf(stderr, "%*s]\n", depth * 2, "");
      }
  }

  ** ***
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", depth * 2, "");
  *** **
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)                  */
/*                                                                           */
/*  Debug print of item onto fp, in the same format as it was read in.       */
/*                                                                           */
/*****************************************************************************/

static void KheSolverItemDebug(KHE_SOLVER_ITEM item, FILE *fp)
{
  KHE_RESOURCE_TYPE rt;
  fprintf(fp, "%d: %s", item->time_weight, KheSolverIdShow(item->id));
  if( item->rtc_tasking != NULL )
  {
    rt = KheTaskingResourceType(item->rtc_tasking);
    fprintf(fp, " %s", KheResourceTypeId(rt));
  }
  if( item->solver != NULL )
  {
    fprintf(fp, "(");
    KheSolverDebug(item->solver, fp);
    fprintf(fp, ")");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolverDebug(KHE_SOLVER solver, FILE *fp)                         */
/*                                                                           */
/*  Debug print of solver onto fp, in the same format as it was read in.     */
/*                                                                           */
/*****************************************************************************/

static void KheSolverDebug(KHE_SOLVER solver, FILE *fp)
{
  KHE_SOLVER_ITEM item;  int i;
  HaArrayForEach(solver->items, item, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheSolverItemDebug(item, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options)       */
/*                                                                           */
/*  Combined resource assignment directed by the rs option.                  */
/*                                                                           */
/*****************************************************************************/

bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options)
{
  char *rs, *rs2, *str;  KHE_SOLVER solver;  bool res;  HA_ARENA a;
  float rs_time_limit;  KHE_TIMER timer;

  /* sort out the time limit and timer */
  str = KheOptionsGet(options, "rs_time_limit", "-");
  rs_time_limit = KheTimeFromString(str);
  if( rs_time_limit != KHE_NO_TIME )
    timer = KheOptionsAddTimer(options, "cr0", rs_time_limit);
  else
    timer = NULL;

  /* parse the rs option and check it for problems */
  rs = rs2 = KheOptionsGet(options, "rs",
    "1:rt(rin(rrq, rgc(rcx))),"
    "2:rt(rin(rgc(rrm, rec, rdv))),"
    "1:rt(red, rrm, rec, rdv)");
  /* HnAssert(strcmp(rs, "auto") != 0, "KheCombinedResourceAssign: rs=auto"); */
  a = KheSolnArenaBegin(soln);
  if( !KheParseSolver(&rs2, soln, a, &solver) )
    HnAbort("KheCombinedResourceAssign: syntax error near pos %d in option"
      " rs=\"%s\"", rs2 - rs, rs);
  /* *** this doesn't work now that I'm converting rt to rdo/rtc
  HaArrayForEach(solver->items, item, i)
    HnAssert(item->id == KHE_SOLVER_RT, "KheCombinedResourceAssign: top-level"
      "item is not rt in option rs=\"%s\"", rs);
  *** */
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombinedResourceAssign(soln of %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)));
    fprintf(stderr, "  rs=\"");
    KheSolverDebug(solver, stderr);
    fprintf(stderr, "\"\n");
    fprintf(stderr, "  rs_time_limit=\"%s\"\n", str);
  }

  /* run */
  res = KheSolverRun(solver, NULL, NULL, soln, options, 1);

  /* tidy up and exit */
  KheSolnArenaEnd(soln, a);
  if( timer != NULL )
    KheOptionsDeleteTimer(options, timer);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombinedResourceAssign returning %s\n",
      bool_show(res));
  return res;
}

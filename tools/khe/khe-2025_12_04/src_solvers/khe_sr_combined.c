
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
/*  bool TryUnassign(KHE_SOLN soln, bool strict, KHE_TASK_SET ts,            */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Try unassigning tasks ts[first_index .. last_index].                     */
/*                                                                           */
/*****************************************************************************/

static bool TryUnassign(KHE_SOLN soln, bool strict, KHE_TASK_SET ts,
  int first_index, int last_index)
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
  success = success &&
    (strict ? KheSolnCost(soln) < init_cost : KheSolnCost(soln) <= init_cost);
  if( DEBUG1 )
  {
    fprintf(stderr, "    unassigning ");
    for( i = first_index;  i <= last_index;  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( i > first_index )
	fprintf(stderr, ", ");
      KheTaskDebug(task, 1, -1, stderr);
    }
    fprintf(stderr, ": %s %s %.5f -> %.5f\n", strict ? "strict" : "non-strict",
      success ? "success" : "fail", KheCostShow(init_cost),
      KheCostShow(KheSolnCost(soln)));
  }
  KheMarkEnd(mark, !success);
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, bool strict,             */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Try unassigning each proper root task of soln, to see if that improves   */
/*  the cost.  Return true if any unassignments were kept.                   */
/*                                                                           */
/*  If strict is true, all unassignments that decrease the cost of soln are  */
/*  kept.  If strict is false, all unassignments that do not increase the    */
/*  cost of soln are kept.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, bool strict,
  KHE_OPTIONS options)
{
  int i, j, unassign, max_unassign, ts_count;  bool res;
  KHE_TASK_SET ts;  KHE_INSTANCE ins;  KHE_RESOURCE r;
  KHE_TIME_GROUP full_tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  max_unassign = KheOptionsGetInt(options, "rs_max_unassign", 1);
  if( DEBUG1 )
    fprintf(stderr,
      "[ KheSolnTryTaskUnAssignments(soln, %s, max_unassign %d)\n",
      bool_show(strict), max_unassign);
  res = false;
  ins = KheSolnInstance(soln);
  full_tg = KheInstanceFullTimeGroup(ins);
  ts = KheTaskSetMake(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    rtm = KheResourceTimetableMonitor(soln, r);
    KheTaskSetClear(ts);
    KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm,
      full_tg, false, ts);
    if( DEBUG1 )
    {
      fprintf(stderr, "  %s: ", KheResourceId(r));
      KheTaskSetDebug(ts, 2, 0, stderr);
    }
    ts_count = KheTaskSetTaskCount(ts);
    for( unassign = 1;  unassign <= max_unassign;  unassign++ )
    {
      for( j = 0;  j <= ts_count - unassign;  j++ )
	if( TryUnassign(soln, strict, ts, j, j + unassign - 1) )
	  res = true;
    }
  }
  KheTaskSetDelete(ts);
  if( DEBUG1 )
    fprintf(stderr, "] KheSolnTryTaskUnAssignments returning %s\n",
      res ? "true" : "false");
  return res;
}


/* *** old version that calls KheFindTasksInInterval
bool KheSolnTryTaskUnAssignments(KHE_SOLN soln, bool strict,
  KHE_OPTIONS options)
{
  HA_ARENA a;  int i, j, unassign, max_unassign, **, x1, x2, ** ts_count;
  KHE_TASK_SET ts;  KHE_INSTANCE ins;  KHE_RESOURCE r;  KHE_TASK_FINDER tf;
  KHE_INTERVAL in;  bool res;
  max_unassign = KheOptionsGetInt(options, "rs_max_unassign", 1);
  if( DEBUG1 )
    fprintf(stderr,
      "[ KheSolnTryTaskUnAssignments(soln, %s, max_unassign %d)\n",
      bool_show(strict), max_unassign);
  res = false;
  a = KheSolnArenaBegin(soln);
  tf = KheTaskFinderMake(soln, options, a);
  if( tf != NULL )
  {
    ins = KheSolnInstance(soln);
    ts = KheTaskSetMake(soln);
    for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
    {
      r = KheInstanceResource(ins, i);
      KheFindTasksInInterval(tf, KheIntervalMake(0, KheTaskFinderLastIndex(tf)),
	KheResourceResourceType(r), r, true, false, ts, &in);
      if( DEBUG1 )
      {
	fprintf(stderr, "  %s: ", KheResourceId(r));
	KheTaskSetDebug(ts, 2, 0, stderr);
      }
      ts_count = KheTaskSetTaskCount(ts);
      for( unassign = 1;  unassign <= max_unassign;  unassign++ )
      {
	for( j = 0;  j <= ts_count - unassign;  j++ )
	  if( TryUnassign(soln, strict, ts, j, j + unassign - 1) )
	    res = true;
      }
    }
    KheTaskSetDelete(ts);
  }
  KheSolnArenaEnd(soln, a);
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
/*  bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options)       */
/*                                                                           */
/*  Combined resource assignment directed by the rs option.                  */
/*                                                                           */
/*****************************************************************************/

bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options)
{
  char *rs, *str;  bool res;
  float rs_time_limit;  KHE_TIMER rs_timer;

  /* sort out the time limit and rs_timer */
  str = KheOptionsGet(options, "rs_time_limit", "-");
  rs_time_limit = KheTimeFromString(str);
  if( rs_time_limit != KHE_NO_TIME )
    rs_timer = KheOptionsAddTimer(options, "rs_time_limit", rs_time_limit);
  else
    rs_timer = NULL;

  /* parse and run the rs option */
  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "30: rin!do rwp rcm @rps{rcg rig rwg rah} (rcx!rds, rfs, rrm, rec),"
      "10: (rrm, rec),"
      "10: (red, rrm, rdv, rec),"
      "1: re2"
    ")"
  );

  /* *** old values
  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "3: rin!do rwp rcm rah rcg rig (rcx!rds, rfs, rrm, rec),"
      "1: (rrm, rec),"
      "1: (red, rrm, rec, rdv)"
    ")"
  );

  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "3: rin!do rrd rwp rcm rah rcg rig (rcx!rds, rfs, rrm, rec),"
      "1: (rrm, rec),"
      "1: (red, rrm, rec, rdv)"
    ")"
  );

  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "3: rin!do rrd rwp rcm rah rcg rwg rig (rcx!rds, rfs, rrm, rec),"
      "1: (rrm, rec),"
      "1: (red, rrm, rec, rdv)"
    ")"
  );

  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt "
      "rin!do rrd rwp rcm rah rcg rwg rig (rcx!rds, rfs, rrm, rec)"
  );

  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "1: rin!do rrd rwp rcm rwg rah rcg rig rcx!rds,"
      "2: rrd rwp rcm rwg (rfs, rrm, rec, rdv),"
      "1: (red, rrm, rec, rdv),"
      "1: (rrm, rec, rdv)"
    ")"
  );

  rs = KheOptionsGet(options, "rs",
    "gts!do rem rt"
    "("
      "3: rin!do rrd rwp rcm rwg rah rcg rig"
      "("
	  "1: rcx!rts,"
	  "2: (rfs, rrm, rec, rdv)"
      "),"
      "1: (red, rrm, rec, rdv)"
    ")"
  );
  *** */

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombinedResourceAssign(soln of %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)));
    fprintf(stderr, "  rs=\"%s\"\n", rs);
    fprintf(stderr, "  rs_time_limit=\"%s\"\n", str);
  }
  res = KheDoItYourselfSolverParseAndRun(soln, NULL, options, "rs", rs);

  /* tidy up and exit */
  if( rs_timer != NULL )
    KheOptionsDeleteTimer(options, rs_timer);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombinedResourceAssign returning %s\n",
      bool_show(res));
  return res;
}


/* *** old version from before solver was made a separate module
bool KheCombinedResourceAssign(KHE_SOLN soln, KHE_OPTIONS options)
{
  char *rs, *rs2, *str;  KHE_SOLVER solver;  bool res;  HA_ARENA a;
  float rs_time_limit;  KHE_TIMER timer;

  ** sort out the time limit and timer **
  str = KheOptionsGet(options, "rs_time_limit", "-");
  rs_time_limit = KheTimeFromString(str);
  if( rs_time_limit != KHE_NO_TIME )
    timer = KheOptionsAddTimer(options, "cr0", rs_time_limit);
  else
    timer = NULL;

  ** parse the rs option and check it for problems **
  rs = rs2 = KheOptionsGet(options, "rs",
    "1:rt(rin(rrq, rgc(rcx))),"
    "2:rt(rin(rgc(rrm, rec, rdv))),"
    "1:rt(red, rrm, rec, rdv)");
  ** HnAssert(strcmp(rs, "auto") != 0, "KheCombinedResourceAssign: rs=auto"); **
  a = KheSolnArenaBegin(soln);
  if( !KheParseSolver(&rs2, soln, a, &solver) )
    HnAbort("KheCombinedResourceAssign: syntax error near pos %d in option"
      " rs=\"%s\"", rs2 - rs, rs);
  ** *** this doesn't work now that I'm converting rt to rdo/rtc
  HaArrayForEach(solver->items, item, i)
    HnAssert(item->id == KHE_SOLVER_RT, "KheCombinedResourceAssign: top-level"
      "item is not rt in option rs=\"%s\"", rs);
  *** **
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombinedResourceAssign(soln of %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)));
    fprintf(stderr, "  rs=\"");
    KheSolverDebug(solver, stderr);
    fprintf(stderr, "\"\n");
    fprintf(stderr, "  rs_time_limit=\"%s\"\n", str);
  }

  ** run **
  res = KheSolve rRun(solver, NULL, NULL, soln, options, 1);

  ** tidy up and exit **
  KheSolnArenaEnd(soln, a);
  if( timer != NULL )
    KheOptionsDeleteTimer(options, timer);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombinedResourceAssign returning %s\n",
      bool_show(res));
  return res;
}
*** */


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
/*  FILE:         khe_sm_general_solve.c                                     */
/*  DESCRIPTION:  KheGeneralSolve2020().                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
/* ***
#define DEBUG4 1		** malloc_stats **
#include <malloc.h>
*** */

/*****************************************************************************/
/*                                                                           */
/*  void KheDebugStage(KHE_SOLN soln, KHE_OPTIONS options,                   */
/*    char *stage, int verbosity)                                            */
/*                                                                           */
/*  Print a debug message showing the stage, how much time has been spent    */
/*  so far, and the solution cost.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheDebugStage(KHE_SOLN soln, KHE_OPTIONS options,
  char *stage, int verbosity)
{
  KHE_TIMER timer;
  float secs ** , kbytes **;  char time_buff[100] ** , mem_buff[100] **
  HnAssert(soln != NULL, "KheDebugStage internal error");

  ** sort out time usage **
  if( !KheOptionsContainsTimer(options, "gs_time_limit", &timer) )
    sprintf(time_buff, "no time limit");
  else
  {
    secs = KheTimerElapsedTime(timer);
    if( secs > 300.0 )
      sprintf(time_buff, "%.2f mins", secs / 60.0);
    else
      sprintf(time_buff, "%.2f secs", secs);
  }

  ** sort out memory usage **
  ** ***
  kbytes = KheStatsMemory();
  if( kbytes > 1024.0 )
    sprintf(mem_buff, "%.2fMB", kbytes / 1024.0);
  else
    sprintf(mem_buff, "%.2fKB", kbytes);
  *** **

  ** print the header and soln stuff **
  fprintf(stderr, "  KheGeneralSolve2020 %s (%s):\n", stage, time_buff);
  KheSolnDebug(soln, verbosity, 2, stderr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource outcomes"                                            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
typedef struct khe_resource_outcome_rec {
  KHE_RESOURCE		resource;
  KHE_COST		cost;
} RESOURCE_OUTCOME;

typedef HA_ARRAY(RESOURCE_OUTCOME) ARRAY_RESOURCE_OUTCOME;
*** */


/*****************************************************************************/
/*                                                                           */
/*  int ResourceOutcomeCmp(const void *t1, const void *t2)                   */
/*                                                                           */
/*  Comparison function for sorting resource outcomes by decreasing cost.    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int ResourceOutcomeCmp(const void *t1, const void *t2)
{
  RESOURCE_OUTCOME rc1 = * (RESOURCE_OUTCOME *) t1;
  RESOURCE_OUTCOME rc2 = * (RESOURCE_OUTCOME *) t2;
  return KheCostCmp(rc2.cost, rc1.cost);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDebugPoorResources(KHE_SOLN soln, int k,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the worst k resources.                                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheSolnDebugPoorResources(KHE_SOLN soln, int k,
  int indent, FILE *fp)
{
  HA_ARENA a;  ARRAY_RESOURCE_OUTCOME outcomes;  KHE_INSTANCE ins;
  RESOURCE_OUTCOME oc;  int i, j;  KHE_MONITOR m;

  ** build resource outcome array **
  a = KheSolnArenaBegin(soln);
  ins = KheSolnInstance(soln);
  HaArrayInit(outcomes, a);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    oc.resource = KheInstanceResource(ins, i);
    oc.cost = KheSolnResourceCost(soln, oc.resource);
    HaArrayAddLast(outcomes, oc);
  }

  ** sort the array **
  HaArraySort(outcomes, &ResourceOutcomeCmp);

  ** print the resources **
  fprintf(stderr, "%*s[ resources of %s:\n", indent, "", KheInstanceId(ins));
  HaArrayForEach(outcomes, oc, i)
  {
    fprintf(stderr, "%*s%s: %.5f\n", indent + 2, "", KheResourceId(oc.resource),
      KheCostShow(oc.cost));
    if( i < k )
    {
      for( j = 0;  j < KheSolnResourceMonitorCount(soln, oc.resource);  j++ )
      {
	m = KheSolnResourceMonitor(soln, oc.resource, j);
	if( KheMonitorCost(m) > 0 )
	  KheMonitorDebug(m, 1, indent + 4, fp);
      }
    }
  }
  fprintf(stderr, "%*s]\n", indent, "");

  ** recycle memory **
  KheSolnArenaEnd(soln, a);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheInstanceContainsSoftSplitConstraint(KHE_INSTANCE ins)            */
/*                                                                           */
/*  Return true if ins contains at least one soft split events or            */
/*  distribute split events constraint.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheInstanceContainsSoftSplitConstraint(KHE_INSTANCE ins)
{
  int i;  KHE_CONSTRAINT c;
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( (KheConstraintTag(c) == KHE_SPLIT_EVENTS_CONSTRAINT_TAG ||
        KheConstraintTag(c) == KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT_TAG)
	&& !KheConstraintRequired(c) && KheConstraintWeight(c) > 0 )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceNameToResource(KHE_INSTANCE ins, char *r_name)   */
/*                                                                           */
/*  Convert r_name into the resource it names.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_RESOURCE KheResourceNameToResource(KHE_INSTANCE ins, char *r_name)
{
  KHE_RESOURCE r;
  if( !KheInstanceRetrieveResource(ins, r_name, &r) )
    HnAbort("KheTryDoubleMove: resource name %s unknown", r_name);
  return r;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupNameToFrameIndex(KHE_INSTANCE ins, KHE_FRAME frame,      */
/*    char *tg_name)                                                         */
/*                                                                           */
/*  Return the index in frame of the time group with name tg_name.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheTimeGroupNameToFrameIndex(KHE_INSTANCE ins, KHE_FRAME frame,
  char *tg_name)
{
  KHE_TIME_GROUP tg, res;  KHE_TIME t;  int index;
  if( !KheInstanceRetrieveTimeGroup(ins, tg_name, &tg) )
    HnAbort("KheTryDoubleMove:  time group name %s unknown", tg_name);
  if( KheTimeGroupTimeCount(tg) == 0 )
    HnAbort("KheTryDoubleMove: time group %s is empty", tg_name);
  t = KheTimeGroupTime(tg, 0);
  index = KheFrameTimeIndex(frame, t);
  res = KheFrameTimeGroup(frame, index);
  if( !KheTimeGroupEqual(res, tg) )
    HnAbort("KheTryDoubleMove: time group %s not in frame", tg_name);
  return index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTryDoubleMove(KHE_SOLN soln, KHE_OPTIONS options,                */
/*    char *r1_name, char *first_index1_name, char *last_index1_name,        */
/*    char *r2_name, char *first_index2_name, char *last_index2_name)        */
/*                                                                           */
/*  Try a double move.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTryDoubleMove(KHE_SOLN soln, KHE_OPTIONS options,
  char *r1_name, char *first_index1_name, char *last_index1_name,
  char *r2_name, char *first_index2_name, char *last_index2_name)
{
  KHE_INSTANCE ins;  int first_index1, last_index1, first_index2, last_index2;
  KHE_RESOURCE r1, r2;  KHE_FRAME frame;  KHE_TASK_SET ts1, ts2;  HA_ARENA a;
  KHE_TASK_FINDER tf;  bool success;  int init_count, i;  KHE_COST init_cost;
  KHE_MARK mark;  KHE_TRACE trace;  KHE_MONITOR d;

  if( DEBUG7 )
  {
    ** convert resource names into resources **
    a = KheSolnArenaBegin(soln);
    frame = KheOptionsFrame(options, "gs_common_frame", soln);
    ins = KheSolnInstance(soln);
    trace = KheTraceMake((KHE_GROUP_MONITOR) soln);
    r1 = KheResourceNameToResource(ins, r1_name);
    r2 = KheResourceNameToResource(ins, r2_name);
    if( r1 == r2 )
      HnAbort("KheTryDoubleMove: resource %s given twice", r1_name);
    if( KheResourceResourceType(r1) != KheResourceResourceType(r2) )
      HnAbort("KheTryDoubleMove: resources %s and %s have different types",
	r1_name, r2_name);

    ** convert time group names for first interval into frame indexes **
    first_index1 = KheTimeGroupNameToFrameIndex(ins, frame, first_index1_name);
    last_index1 =  KheTimeGroupNameToFrameIndex(ins, frame, last_index1_name);
    if( first_index1 > last_index1 )
      HnAbort("KheTryDoubleMove: first interval [%s, %s] is empty",
        first_index1_name, last_index1_name);

    ** convert time group names for second interval into frame indexes **
    first_index2 = KheTimeGroupNameToFrameIndex(ins, frame, first_index2_name);
    last_index2 =  KheTimeGroupNameToFrameIndex(ins, frame, last_index2_name);
    if( first_index2 > last_index2 )
      HnAbort("KheTryDoubleMove: second interval [%s, %s] is empty",
        first_index2_name, last_index2_name);

    ** get the task sets and carry out the double moves **
    tf = KheTaskFinde rMake(soln, options, a);
    ts1 = KheTaskSetMake(soln);
    ts2 = KheTaskSetMake(soln);
    if( KheFindT asksInInterval(tf, r1, first_index1, last_index1, ts1) &&
        KheFindTask sInInterval(tf, r2, first_index2, last_index2, ts2) )
    {
      init_cost = KheSolnCost(soln);
      KheAtomicOperationBegin(soln, &mark, &init_count, false);
      KheTraceBegin(trace);
      success = KheTaskSetMoveResource(ts1, r2) &&
	KheTaskSetMoveResource(ts2, r1);
      KheTraceEnd(trace);
      fprintf(stderr, "  KheTryDoubleMove %s[%s, %s] <-> %s[%s, %s]:"
	" %.5f -> %.5f\n", r1_name, first_index1_name, last_index1_name,
	r2_name, first_index2_name, last_index2_name,
	KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
      for( i = 0;  i < KheTraceMonitorCount(trace);  i++ )
      {
	d = KheTraceMonitor(trace, i);
	fprintf(stderr, "    monitor %.5f -> %.5f: ",
	  KheCostShow(KheTraceMonitorInitCost(trace, i)),
	  KheCostShow(KheMonitorCost(d)));
	KheMonitorDebug(d, 1, 0, stderr);
      }
      if( KheAtomicOperationEnd(soln, &mark, &init_count, false,
	    success && KheSolnCost(soln) < init_cost) )
      {
	fprintf(stderr, "  KheTryDoubleMove %s[%s, %s] <-> %s[%s, %s] success:"
	  " %.5f -> %.5f\n", r1_name, first_index1_name, last_index1_name,
	  r2_name, first_index2_name, last_index2_name,
	  KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
      }
      else
      {
	fprintf(stderr, "  KheTryDoubleMove %s[%s, %s] <-> %s[%s, %s] fail\n",
	  r1_name, first_index1_name, last_index1_name,
	  r2_name, first_index2_name, last_index2_name);
      }
    }
    else
    {
      fprintf(stderr, "  KheTryDoubleMove %s[%s, %s] <-> %s[%s, %s] skipped\n",
	r1_name, first_index1_name, last_index1_name,
	r2_name, first_index2_name, last_index2_name);
    }
     KheSolnArenaEnd(soln, a);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheConvertTimeLimit(char *str)                                     */
/*                                                                           */
/*  Convert *str into a number of seconds, taking the format to be           */
/*                                                                           */
/*      secs    or    mins:secs    or    hrs:mins:secs                       */
/*                                                                           */
/*****************************************************************************/

/* ** moved to KheTimeFromString in khe_sm_timer.c
float KheConve rtTimeLimit(char *str)
{
  char *p;  int hrs, mins, secs;

  ** "-" special case **
  if( strcmp(str, "-") == 0 )
    return -1;

  ** secs **
  p = strstr(str, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%d", &secs) != 1 )
      HnAbort("KheConvertTimeLimit:  format error in time limit %s", str);
    return (float) secs;
  }

  ** mins:secs **
  p = strstr(p + 1, ":");
  if( p == NULL )
  {
    if( sscanf(str, "%d:%d", &mins, &secs) != 2 )
      HnAbort("KheConvertTimeLimit:  format error in time limit %s", str);
    return (float) (mins * 60 + secs);
  }

  ** hrs:mins:secs **
  if( sscanf(str, "%d:%d:%d", &hrs, &mins, &secs) != 3 )
    HnAbort("KheConvertTimeLimit:  format error in time limit %s", str);
  return (float) (hrs * 60 * 60 + mins * 60 + secs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void HandleDebugRTM(KHE_SOLN soln, char *debug_rtm)                      */
/*                                                                           */
/*  Handle a debug_rtm option.                                               */
/*                                                                           */
/*****************************************************************************/

static void HandleDebugRTM(KHE_SOLN soln, char *debug_rtm)
{
  KHE_INSTANCE ins;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;
  char r_name[101], tg_name[101];  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( sscanf(debug_rtm, "%100[^:]:%100s", r_name, tg_name) != 2 )
    HnAbort("KheGeneralSolve2020: syntax error in gs_debug_rtm option \"%s\"",
      debug_rtm);
  ins = KheSolnInstance(soln);
  if( !KheInstanceRetrieveResource(ins, r_name, &r) )
    HnAbort("KheGeneralSolve2020: unknown resource \"%s\" in gs_debug_rtm "
      "option \"%s\"", r_name, debug_rtm);
  if( !KheInstanceRetrieveTimeGroup(ins, tg_name, &tg) )
    HnAbort("KheGeneralSolve2020: unknown time group \"%s\" in gs_debug_rtm "
      "option \"%s\"", tg_name, debug_rtm);
  rtm = KheResourceTimetableMonitor(soln, r);
  KheResourceTimetableMonitorSetDebug(rtm, tg, true);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheGeneralSolve2024(KHE_SOLN soln, KHE_OPTIONS options)         */
/*                                                                           */
/*  Solve soln, assuming that it has just emerged from KheSolnMake.          */
/*                                                                           */
/****************************************************************************/
/* extern void KheEjectionChainNodeGroupForTimingTest(KHE_NODE parent_node); */

/* *** replaced by do-it-yourself version below
KHE_SOLN KheGeneralSolve2024(KHE_SOLN soln, KHE_OPTIONS options)
{
  int i, diversifier;  KHE_EVENT junk;  KHE_NODE cycle_node;  float time_limit;
  char *monitor_id, *debug_rtm;  KHE_MONITOR m;  KHE_TIMER global_timer;
  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_INSTANCE ins;
  bool matching_off, monitor_evenness, time_assignment_only, unas sign_off;
  bool propagate_unavailable_times_off;  KHE_GROUP_MONITOR low_cost_gm;
  ** char *rs;  KHE_TASKING tasking; **
  ** KHE_RESOURCE_TYPE rt; **


  *****************************************************************************
  **                                                                         **
  **  initialization phase                                                   **
  **                                                                         **
  *****************************************************************************

  ** set soft time limit **
  time_limit = KheTimeFromString(KheOptionsGet(options, "gs_time_limit", "-"));
  global_timer = KheOptionsAddTimer(options, "gs_time_limit", time_limit);

  ins = KheSolnInstance(soln);
  matching_off = KheOptionsGetBool(options, "gs_matching_ off", false);
  monitor_evenness = KheOptionsGetBool(options, "gs_monitor_evenness", false);
  propagate_unavailable_times_off = KheOptionsGetBool(options,
    "gs_propagate_unavailable_times_off", false);
  time_assignment_only = KheOptionsGetBool(options, "gs_time_assignment_only",
    false);
  unas sign_off = KheOptionsGetBool(options, "gs_unassi gnment_off", false);
  diversifier = KheOptionsGetInt(options, "gs_diversifier", -1);
  if( diversifier != -1 )
    KheSolnSetDiversifier(soln, diversifier);
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheGeneralSolve2024(%p %s) div %d\n", (void *) soln,
      KheInstanceId(ins) == NULL ? "-" : KheInstanceId(ins),
      KheSolnDiversifier(soln));
    ** KheDebugStage(soln, options, "at start", 2); **
    KheSolveDebug(soln, options, "at start");
  }

  ** set debug_monitor option if debug_monitor_id is present **
  monitor_id = KheOptionsGet(options, "gs_debug_monitor_id", NULL);
  if( monitor_id != NULL )
  {
    if( DEBUG3 )
      fprintf(stderr, "  KheGeneralSolve2020 found debug_monitor_id %s\n",
	monitor_id);
    if( KheSolnRetrieveMonitor(soln, monitor_id, &m) )
    {
      if( DEBUG3 )
      {
	fprintf(stderr, "  KheGeneralSolve2020 setting debug_monitor:\n");
	KheMonitorDebug(m, 1, 2, stderr);
      }
      KheOptionsSetObject(options, "gs_debug_monitor", (void *) m);
    }
    else
      HnAbort("KheGeneralSolve2020: cannot find gs_debug_monitor_id \"%s\"\n",
	monitor_id);
  }

  ** handle gs_debug_rtm option **
  debug_rtm = KheOptionsGet(options, "gs_debug_rtm", NULL);
  if( debug_rtm != NULL )
    HandleDebugRTM(soln, debug_rtm);

  ** detach low-cost monitors if requested **
  low_cost_gm = NULL;
  if( KheOptionsGetBool(options, "gs_hard_constraints_only", false) )
  {
    low_cost_gm = KheGroupMonitorMake(soln, 55, "low-cost");
    KheDetachLowCostMonitors(soln, KheCost(1, 0), low_cost_gm);
  }


  *****************************************************************************
  **                                                                         **
  **  time structural phase                                                  **
  **                                                                         **
  *****************************************************************************

  ** split initial cycle meet and make complete representation **
  KheSolnSplitCycleMeet(soln);
  if( DEBUG1 )
    KheSolveDebug(soln, options, "before KheSolnMakeCompleteRepresentation");
  if( !KheSolnMakeCompleteRepresentation(soln, &junk) )
    HnAbort("KheGeneralSolve2020:  !KheSolnMakeCompleteRepresentation");

  ** build layer tree **
  cycle_node = KheLayerTreeMake(soln);
  if( DEBUG2 )
  {
    fprintf(stderr, "  cycle node after KheLayerTreeMake:\n");
    KheNodeDebug(cycle_node, 4, 2, stderr);
  }
  KheOptionsSetBool(options, "es_split_moves",
    KheInstanceContainsSoftSplitConstraint(ins));

  ** +++ gs starts here +++ **
  ** optionally install the global tixel matching **
  if( !matching_off )
  {
    if( DEBUG1 )
      KheSolveDebug(soln, options, "before KheS olnMatchingBegin");
    KheSoln MatchingBegin(soln);
    KheSolnMatchingSetWeight(soln, KheCost(1, 0));
    KheSolnMatchingAddAllWorkloadRequirements(soln);
    KheSolnMatchingAttachAllOrdinaryDemandMonitors(soln);
    if( DEBUG2 )
    {
      ** KheDebugStage(soln, options, "after installing the matching", 2); **
      KheSolveDebug(soln, options, "after installing the matching");
      if( KheSolnMatchingDefectCount(soln) > 0 )
	KheGroupMonitorDefectDebug((KHE_GROUP_MONITOR) soln, 2, 2, stderr);
    }
  }

  ** optionally install evenness monitors **
  if( monitor_evenness )
  {
    KheSolnEvennessBegin(soln);
    KheSolnSetAllEvennessMonitorWeights(soln, KheCost(0, 5));
    KheSolnAttachAllEvennessMonitors(soln);
  }

  ** optionally propagate resource monitor adjustments **
  if( !propagate_unavailable_times_off )
    KhePropagateUnavailableTimes(soln, NULL);

  ** build task tree, including assigning preassigned resources **
  KheOptionsSetBool(options, "rs_invariant", true);
  KheTaskTreeMake(soln, options);

  ** find time-equivalent events and resources **
  ** done on first use now KheTimeEquivSolve(time_eq uiv, soln); **

  ** coordinate layers and build runarounds **
  if( DEBUG1 )
    KheSolveDebug(soln, options, "before KheCoordinateLayers");
  KheCoordinateLayers(cycle_node, true);
  if( DEBUG2 )
  {
    fprintf(stderr, "  cycle node after KheCoordinateLayers:\n");
    KheNodeDebug(cycle_node, 4, 2, stderr);
  }
  if( DEBUG1 )
    KheSolveDebug(soln, options, "before KheBuildRunarounds");
  KheBuildRunarounds(cycle_node, &KheNodeSimple AssignTimes, options,
    &KheRunaroundNodeAssignTimes, options);
  if( DEBUG2 )
  {
    fprintf(stderr, "  cycle node after KheBuildRunarounds:\n");
    KheNodeDebug(cycle_node, 4, 2, stderr);
  }


  *****************************************************************************
  **                                                                         **
  **  time assignment phase                                                  **
  **                                                                         **
  *****************************************************************************

  ** assign to nodes below the cycle node **
  if( DEBUG1 )
    KheSolveDebug(soln, options, "before %d KheNodeRecur siveAssignTimes",
      KheNodeChildCount(cycle_node));
  for( i = 0;  i < KheNodeChildCount(cycle_node);  i++ )
    KheNodeRecursiveAssignT imes(KheNodeChild(cycle_node, i),
      &KheRunaroundNodeAssignTimes, options);

  ** assign to the cycle node **
  if( DEBUG1 )
    KheSolveDebug(soln, options, "before KheCycleNodeAssignTimes");
  KheCycleNodeAssignTimes(cycle_node, options);
    ** KheDebugStage(soln, options, "after time assignment", 2); **
  if( time_assignment_only )
  {
    if( low_cost_gm != NULL )
      KheAttachLowCostMonitors(low_cost_gm);
    KheSolnSetRunningTime(soln, KheTimerElapsedTime(global_timer));
    KheOptionsDeleteTimer(options, global_timer);
    if( DEBUG1 )
      fprintf(stderr, "] KheGeneralSolve2020 returning after time asst\n");
    return soln;
  }


  *****************************************************************************
  **                                                                         **
  **  resource assignment phases                                             **
  **                                                                         **
  *****************************************************************************

  ** end the inclusion of demand cost in the solution cost **
  KheDisconnectAllDemandMonitors(soln, NULL);

  ** add an event timetable monitor to options **
  e tm = KheEventTimetableMonitorMake(soln, KheInstanceFullEventGroup(ins));
  KheMonitorAttachToSoln((KHE_MONITOR) etm);
  KheOptionsSetObject(options, "gs_event_timetable_monitor", (void *) etm);

  ** ***
  rs = KheOptionsGet(options, "r s", "auto");
  if( strcmp(rs, "auto") == 0 )
  {
    ** assign taskings (parts 1 and 2) **
    for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
    {
      tasking = KheSolnTasking(soln, i);
      if( DEBUG1 )
	KheSolveDebug(soln, options, "before %s assignment (stages 1 and 2)",
	  KheResourceTypeName(KheTaskingResourceType(tasking)));
      KheTaskingAssignResourcesStage1(tasking, options);
      KheTaskingAssignResourcesStage2(tasking, options);
    }

    ** assign taskings (part 3) **
    for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
    {
      tasking = KheSolnTasking(soln, i);
      if( DEBUG1 )
	KheSolveDebug(soln, options, "before %s assignment (part 3)",
	  KheResourceTypeName(KheTaskingResourceType(tasking)));
      KheTaskingAssignResourcesStage3(tasking, options);
    }

    ** try a tree search wherever there are still problems **
    ** ***
    KheTreeSearchRepa irTimes(soln, NULL, true);
    *** **
  }
  else
  *** **
  KheCombinedResourceAssign(soln, options);


  *****************************************************************************
  **                                                                         **
  **  cleanup phase                                                          **
  **                                                                         **
  *****************************************************************************

  ** ensure that the solution cost is the official cost **
  if( low_cost_gm != NULL )
    KheAttachLowCostMonitors(low_cost_gm);
  KheSolnEnsureOfficialCost(soln);

  ** merge meets (requires split events monitors, hence this placement) **
  KheMergeMeets(soln);

  ** try task and meet unassignments **
  if( !unas sign_off )
  {
    KheSolnTryTaskUnAssignments(soln, options);
    KheSolnTryMeetUnAssignments(soln);
  }

  ** last-ditch check of swap possibilities (NB requires etm) **
  ** ***
  if( DEBUG6 )
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      KheResourcePairAllSwapRepair(soln, rt, options);
    }
  KheTryDoubleMove(soln, options,
    "TR_27", "4Sat", "4Sun", "TR_26", "1Sat", "2Mon");
  *** **

  ** remove the event timetable monitor from options and delete it **
  HnAssert(KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL)
    == (void *) etm, "KheGeneralSolve2020 internal error (etm)");
  KheOptionsSetObject(options, "gs_event_timetable_monitor", NULL);
  ** KheEventTimetableMonitorDelete(etm); **

  ** wrapup **
  if( DEBUG5 )
    KheSolnDebugPoorResources(soln, 8, 2, stderr);
  if( DEBUG1 )
  {
    KheSolveDebug(soln, options, "at end");
    if( DEBUG4 )
    {
      KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_RESOURCE r;
      for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
      {
	r = KheInstanceResource(ins, i);
	fprintf(stderr, "\n  Timetable for resource %s\n", KheResourceId(r));
	rtm = KheResourceTimetableMonitor(soln, r);
	KheResourceTimetableMonitorPrintTimetable(rtm, 6, 2, stderr);
      }
    }
    fprintf(stderr, "] KheGeneralSolve2020 returning\n");
  }
  KheSolnSetRunningTime(soln, KheTimerElapsedTime(global_timer));
  KheOptionsDeleteTimer(options, global_timer);
  return soln;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheGeneralSolve2024(KHE_SOLN soln, KHE_OPTIONS options)         */
/*                                                                           */
/*  Solve soln, assuming that it has just emerged from KheSolnMake.          */
/*                                                                           */
/****************************************************************************/
/* extern void KheEjectionChainNodeGroupForTimingTest(KHE_NODE parent_node); */

KHE_SOLN KheGeneralSolve2024(KHE_SOLN soln, KHE_OPTIONS options)
{
  int diversifier, i;  KHE_EVENT junk;  KHE_NODE cycle_node;  float time_limit;
  char *monitor_id, *debug_rtm, *gs;  KHE_MONITOR m;  KHE_TIMER gs_timer;
  KHE_INSTANCE ins;  bool unassign_off, time_limit_consistency;
  KHE_RESOURCE_TYPE rt;


  /***************************************************************************/
  /*                                                                         */
  /*  handle options                                                         */
  /*                                                                         */
  /***************************************************************************/

  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGeneralSolve2024(soln of %s, options)\n",
      KheInstanceId(ins));

  /* gs_diversifier */
  diversifier = KheOptionsGetInt(options, "gs_diversifier", -1);
  if( diversifier != -1 )
    KheSolnSetDiversifier(soln, diversifier);

  /* gs_time_limit */
  time_limit = KheTimeFromString(KheOptionsGet(options, "gs_time_limit", "-"));
  gs_timer = KheOptionsAddTimer(options, "gs_time_limit", time_limit);

  /* gs_time_limit_consistency */
  time_limit_consistency = KheOptionsGetBool(options,
    "gs_time_limit_consistency", false);

  /* gs_unassignment_off */
  unassign_off = KheOptionsGetBool(options, "gs_unassignment_off", false);

  /* gs_debug_monitor_id and gs_debug_monitor */
  monitor_id = KheOptionsGet(options, "gs_debug_monitor_id", NULL);
  if( monitor_id != NULL )
  {
    if( KheSolnRetrieveMonitor(soln, monitor_id, &m) )
      KheOptionsSetObject(options, "gs_debug_monitor", (void *) m);
    else
      HnAbort("KheGeneralSolve2020: cannot find gs_debug_monitor_id \"%s\"\n",
	monitor_id);
  }

  /* gs_debug_rtm */
  debug_rtm = KheOptionsGet(options, "gs_debug_rtm", NULL);
  if( debug_rtm != NULL )
    HandleDebugRTM(soln, debug_rtm);


  /***************************************************************************/
  /*                                                                         */
  /*  functions more or less unavoidable when starting a solve               */
  /*                                                                         */
  /***************************************************************************/

  /* begin time limit consistency, if requested */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 KheOptionsTimeLimitConsistency:\n");
  if( time_limit_consistency )
    KheOptionsTimeLimitConsistencyBegin(options, soln);

  /* split initial cycle meet */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 KheSolnSplitCycleMeet:\n");
  KheSolnSplitCycleMeet(soln);

  /* make complete representation */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 MakeCompleteRepresentation:\n");
  if( !KheSolnMakeCompleteRepresentation(soln, &junk) )
    HnAbort("KheGeneralSolve2020:  !KheSolnMakeCompleteRepresentation");

  /* layer tree */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 KheLayerTreeMake:\n");
  cycle_node = KheLayerTreeMake(soln);

  /* es_split_moves */
  KheOptionsSetBool(options, "es_split_moves",
    KheInstanceContainsSoftSplitConstraint(ins));

  /* build task tree, including assigning preassigned resources */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 KheTaskTreeMake:\n");
  KheOptionsSetBool(options, "rs_invariant", true);
  KheTaskTreeMake(soln, NULL, options);
  KheOptionsSetBool(options, "rs_invariant", false);


  /***************************************************************************/
  /*                                                                         */
  /*  do-it-yourself part                                                    */
  /*                                                                         */
  /***************************************************************************/

  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 do-it-yourself part\n");
  gs = KheOptionsGet(options, "gs", "(gpu, ts, rs)");
  KheDoItYourselfSolverParseAndRun(soln, cycle_node, options, "gs", gs);


  /***************************************************************************/
  /*                                                                         */
  /*  functions more or less unavoidable when ending a solve                 */
  /*                                                                         */
  /***************************************************************************/

  /* ensure that the solution cost is the official cost */
  if( DEBUG1 )
    fprintf(stderr, "  KheGeneralSolve2024 ending solve part\n");
  KheSolnEnsureOfficialCost(soln);

  /* merge meets (requires split events monitors, hence this placement) */
  KheMergeMeets(soln);

  /* try task and meet unassignments */
  if( !unassign_off )
  {
    if( DEBUG8 )
      fprintf(stderr, "  KheGeneralSolve2024 calling "
	"KheSolnTryTaskUnAssignments (etc.)\n");
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      KheEjectionChainRepairResources(soln, rt, options, "2+");
    }
    /* KheSolnTryTaskUnAssignments(soln, false, options); */
    KheSolnTryMeetUnAssignments(soln);
  }

  /* end time limit consistency, if requested */
  if( time_limit_consistency )
    KheOptionsTimeLimitConsistencyEnd(options, soln);

  /* set running time and delete timer */
  KheSolnSetRunningTime(soln, KheTimerElapsedTime(gs_timer));
  KheOptionsDeleteTimer(options, gs_timer);
  if( DEBUG1 )
    fprintf(stderr, "] KheGeneralSolve2024 returning soln\n");
  return soln;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolveDebug(KHE_SOLN soln, KHE_OPTIONS options, int indent,       */
/*    FILE *fp, char *fmt, ...)                                              */
/*                                                                           */
/*  Debug print of the solve.                                                */
/*                                                                           */
/*****************************************************************************/

void KheSolveDebug(KHE_SOLN soln, KHE_OPTIONS options, char *fmt, ...)
{
  KHE_INSTANCE ins;  KHE_TIMER timer;  char buff[20];  va_list args;
  KHE_MONITOR m;
  ins = KheSolnInstance(soln);
  fprintf(stderr, "%*s%s#%d: %.5f", 2, "", KheInstanceId(ins),
    KheSolnDiversifier(soln), KheCostShow(KheSolnCost(soln)));
  if( KheOptionsContainsTimer(options, "gs_time_limit", &timer) )
    fprintf(stderr, ", %9s", KheTimeShow(KheTimerElapsedTime(timer), buff));
  if( fmt != NULL )
  {
    fprintf(stderr, " ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
  }
  fprintf(stderr, "\n");
  m = (KHE_MONITOR) KheOptionsGetObject(options, "gs_debug_monitor", NULL);
  if( m != NULL )
    KheMonitorDebug(m, 2, 4, stderr);
}

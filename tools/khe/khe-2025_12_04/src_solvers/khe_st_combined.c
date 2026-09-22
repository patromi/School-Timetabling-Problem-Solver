
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
/*  FILE:         khe_st_combined.c                                          */
/*  DESCRIPTION:  Combined time solvers                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheCycleNodeAssignTimes"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCycleNodeAssignTimes(KHE_NODE cycle_node, KHE_OPTIONS options)   */
/*                                                                           */
/*  Assign times to the meets of all the proper descendants of cycle_node,   */
/*  assumed to be the cycle node, using the best available algorithm.        */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheCombinedTimeAssign below
bool KheCycleNodeAssignTimes(KHE_NODE cycle_node, KHE_OPTIONS options)
{
  int i;  KHE_SOLN soln;  bool res;
  KHE_MEET_BOUND_GROUP cluster_mbg;  KHE_TASK_BOUND_GROUP tighten_tbg;
  bool cluster_meet_domains, tighten_domains_off, node_repair_off;
  KHE_TIMER node_repair_timer;  float node_repair_time_limit;
  ** int save_ejector_limit_defects; **
  ** bool cluster_and_limit = true; **
  ** bool save_ejector_use_fuzzy_moves; **

  if( DEBUG3 )
  {
    fprintf(stderr, "[ KheCycleNodeAssignTimes(Node %d)\n",
      KheNodeSolnIndex(cycle_node));
    KheNodeDebug(cycle_node, 4, 2, stderr);
  }

  ** assign preassigned meets **
  soln = KheNodeSoln(cycle_node);
  KheNodePreassignedAssignTimes(cycle_node, options);

  if( !KheInstanceAllEventsHavePreassignedTimes(KheSolnInstance(soln)) )
  {
    ** get options used by this solver **
    cluster_meet_domains = KheOptionsGetBool(options, "ts_cluster_meet_domains",
      false);
    tighten_domains_off = KheOptionsGetBool(options, "ts_tighten_domains_off",
      false);
    node_repair_off = KheOptionsGetBool(options, "ts_node_repair_off", false);
    node_repair_time_limit = KheTimeFromString(KheOptionsGet(options,
	"ts_node_repair_time_limit", "-"));

    ** tighten resource domains **
    if( DEBUG3 )
      fprintf(stderr, "  KheCycleNodeAssignTimes tightening domains:\n");
    if( !tighten_domains_off )
    {
      tighten_tbg = KheTaskBound GroupMake(soln);
      for( i = 0;  i < KheSolnTaskingCount(soln);  i++ )
	KheTaskingTightenToPartition(KheSolnTasking(soln, i),
	  tighten_tbg, options);
    }
    else
      tighten_tbg = NULL;  ** keep compiler happy **

    ** cluster meet domains **
    if( DEBUG3 )
      fprintf(stderr, "  KheCycleNodeAssignTimes clustering meet domains\n");
    if( cluster_meet_domains )
    {
      cluster_mbg = KheMeetBoundGroupMake(soln);
      KheSolnAddUnavailableBounds(soln, KheCost(0, 0), cluster_mbg);
      KheSolnClusterAndLimitMeetDomains(soln, 0, KheCost(1, 0), 1.8,
	cluster_mbg, options);
      ** ***
      if( cluster_and_limit )
	KheSolnClusterAndLimitMeetDomains(soln, 0, 0, 0.8, cluster_mbg);
      else
	KheSolnClusterMeetDomains(soln, KheCost(0, 0), cluster_mbg);
      *** **
    }
    else
      cluster_mbg = NULL;  ** keep compiler happy **

    ** time assignment and repair **
    if( DEBUG3 )
      fprintf(stderr,
	"  KheCycleNodeAssignTimes calling KheNodeLayeredAssignTimes\n");
    ** ***
    save_ejector_limit_defects = KheOptionsEjectorLimitDefects(options);
    if( save_ejector_limit_defects > 50 )
      KheOptionsSetEjectorLimitDefects(options, 50);
    KheOptionsSetEjectorLimitDefects(options, save_ejector_limit_defects);
    *** **
    KheNodeLayeredAssignTimes(cycle_node, options ** was false, true **);
    if( !node_repair_off )
    {
      if( DEBUG3 )
	fprintf(stderr,
	 "  KheCycleNodeAssignTimes calling KheEjectionChainNodeRepairTimes\n");
      node_repair_timer = (node_repair_time_limit < 0.0 ? NULL :
	KheOptionsAddTimer(options, "node_repair", node_repair_time_limit));
      KheEjectionChainNodeRepairTimes(cycle_node, options);
      if( node_repair_timer != NULL )
	KheOptionsDeleteTimer(options, node_repair_timer);
    }
    if( DEBUG3 )
      fprintf(stderr,
	"  KheCycleNodeAssignTimes flattening and deleting zones\n");
    KheNodeFlatten(cycle_node);
    KheNodeDeleteZones(cycle_node);
    if( cluster_meet_domains )
      KheMeetBoundGroupDelete(cluster_mbg);
    if( !node_repair_off )
    {
      if( DEBUG3 )
	fprintf(stderr,
	 "  KheCycleNodeAssignTimes calling KheEjectionChainNodeRepairTimes\n");
      node_repair_timer = (node_repair_time_limit < 0.0 ? NULL :
	KheOptionsAddTimer(options, "node_repair", node_repair_time_limit));
      KheEjectionChainNodeRepairTimes(cycle_node, options);
      if( node_repair_timer != NULL )
	KheOptionsDeleteTimer(options, node_repair_timer);
      ** ***
      save_ejector_use_fuzzy_moves = KheOptionsEjectorUseFuzzyMoves(options);
      KheOptionsSetEjectorUseFuzzyMoves(options, true);
      KheEjectionChainNodeRepairTimes(cycle_node, options);
      KheOptionsSetEjectorUseFuzzyMoves(options, save_ejector_use_fuzzy_moves);
      *** **
    }

    ** undo resource domain tightening **
    if( !tighten_domains_off )
    {
      if( DEBUG3 )
	fprintf(stderr, "  demand defects before untightening: %d\n",
	  KheSolnMatchingDefectCount(soln));
      KheTaskBoundGroupDelete(tighten_tbg);
      if( DEBUG3 )
	fprintf(stderr, "  demand defects after untightening: %d\n",
	  KheSolnMatchingDefectCount(soln));
    }
  }

  ** all done **
  res = KheNodeAllChildMeetsAssigned(cycle_node);
  if( DEBUG3 )
    fprintf(stderr, "] KheCycleNodeAssignTimes returning %s\n",
      res ? "true" : "false");
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombinedTimeAssign(KHE_NODE cycle_node, KHE_OPTIONS options)     */
/*                                                                           */
/*  Assign times to the meets of all the proper descendants of cycle_node,   */
/*  assumed to be the cycle node, do-it-yourself solver ts.                  */
/*                                                                           */
/*****************************************************************************/
#define bool_show(x) ((x) ? "true" : "false")

bool KheCombinedTimeAssign(KHE_NODE cycle_node, KHE_OPTIONS options)
{
  char *ts, *str;  bool res;  KHE_SOLN soln;
  float ts_time_limit;  KHE_TIMER ts_timer;

  /* sort out the time limit and ts_timer */
  soln = KheNodeSoln(cycle_node);
  str = KheOptionsGet(options, "ts_time_limit", "-");
  ts_time_limit = KheTimeFromString(str);
  if( ts_time_limit != KHE_NO_TIME )
    ts_timer = KheOptionsAddTimer(options, "ts_time_limit", ts_time_limit);
  else
    ts_timer = NULL;

  /* parse and run the ts option */
  ts = KheOptionsGet(options, "ts",
      "gti(tcl, tbr, trt, tpa, tnp ttp(tnl, tec, tnf, tdz, tec))");
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombinedTimeAssign(soln of %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)));
    fprintf(stderr, "  ts=\"%s\"\n", ts);
    fprintf(stderr, "  ts_time_limit=\"%s\"\n", str);
  }
  res = KheDoItYourselfSolverParseAndRun(soln, cycle_node, options, "ts", ts);

  /* tidy up and exit */
  if( ts_timer != NULL )
    KheOptionsDeleteTimer(options, ts_timer);
  if( DEBUG2 )
    fprintf(stderr, "] KheCombinedTimeAssign returning %s\n",
      bool_show(res));
  return res;
}

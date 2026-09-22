
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
/*  it under the terms of the GNU Generar Public License as published by     */
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
/*  FILE:         khe_sr_resource_matching.c                                 */
/*  DESCRIPTION:  Resource matching                                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_mmatch.h"
#include <limits.h>

#define bool_show(a) ((a) ? "true" : "false")

#define DEBUG1 0	/* preparation: find and group assignable tasks      */
#define DEBUG2 0	/* preparation: find task profiles and merge nodes   */
#define DEBUG3 0	/* preparation: add preferences for limit resources  */
#define DEBUG4 0	/* initial/final cost problem                        */
#define DEBUG5 0	/* KheAddLimitResourcesMonitorPreferences            */
#define DEBUG6 0	/* individual solves                                 */
#define DEBUG7 0	/* graph construction                                */
#define DEBUG8 0	/* KheDemandNodeSplit                                */
#define DEBUG9 0	/* tracing during graph construction                 */
#define DEBUG10 0	/* individual solves (basic)                         */
#define DEBUG11 0	/* individual solves (basic)                         */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SUPPLY_NODE                                                          */
/*                                                                           */
/*  One supply node sn of the graph.                                         */
/*                                                                           */
/*  If sn->resource != NULL, then sn represents assigning sn->resource to    */
/*  one of the tasks of some (any) demand node.  The multiplicity of sn is   */
/*  1, because assigning sn->resource to more than one node would cause a    */
/*  clash.                                                                   */
/*                                                                           */
/*  If sn->resource == NULL, then sn represents not assigning a resource     */
/*  to one of the tasks of some (any) demand node.  The multiplicity of sn   */
/*  is effectively inifinite, because assigning nothing can be done many     */
/*  times.  However, the MMatch module does not accept INT_MAX, so what we   */
/*  actually use is the total multiplicity of the demand nodes.              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_supply_node_rec {
  KHE_RESOURCE_MATCHING_SOLVER	solver;		/* enclosing solver          */
  KHE_RESOURCE			resource;	/* the resource (optional)   */
  /* KHE_GROUP_MONITOR	group_monitor; */	/* group monitor for resource*/
  int				max_busy;	/* part of adjust1           */
  int				curr_busy;	/* part of adjust1           */
  /* int			curr_at_max; */	/* reset for each match      */
  bool				curr_was_used;	/* part of adjust3           */
  int				curr_run_len;	/* part of adjust3           */
  int				curr_offset;	/* part of adjust4           */
  KHE_MMATCH_NODE		mmatch_node;	/* supply node in mmatch     */
} *KHE_SUPPLY_NODE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFERENCE                                                           */
/*                                                                           */
/*  A set of preferred resources:                                            */
/*                                                                           */
/*    resource_group                                                         */
/*      The preferred resources.  As a special case, NULL means all          */
/*      resources of the relevant type.                                      */
/*                                                                           */
/*    include_r0                                                             */
/*      If this flag is true, non-assignment (r0 in the documentation) is    */
/*      also a preferred "resource".                                         */
/*                                                                           */
/*    monitor_tag                                                            */
/*      The kind of monitor that this preference is derived from.  We need   */
/*      to remember this because only preferences derived from limit         */
/*      resources monitors affect edge weights.                              */
/*                                                                           */
/*    cost                                                                   */
/*      The cost of not satisfying this preference, equal to the weight of   */
/*      the constraint times the duration of the affected task(s).           */
/*                                                                           */
/*  Preferences have two uses:  to help decide whether two tasks are         */
/*  equivalent, and for adjusting edge weights during matching.              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_preference_rec {
  KHE_RESOURCE_GROUP	resource_group;
  bool			include_r0;
  KHE_MONITOR_TAG	monitor_tag;
  KHE_COST		cost;
} *KHE_PREFERENCE;

typedef HA_ARRAY(KHE_PREFERENCE) ARRAY_KHE_PREFERENCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_PROFILE                                                         */
/*                                                                           */
/*  The profile of a task, including all tasks assigned to it, directly      */
/*  or indirectly.  This holds everything that affects solution cost, so     */
/*  that, when two tasks have the same profile, assigning either of them     */
/*  to the same resource (any resource) has the same effect.                 */
/*                                                                           */
/*    time_set                                                               */
/*      The set of times they occupy                                         */
/*                                                                           */
/*    duration                                                               */
/*      Their total duration (equal to the cardinality of time_set)          */
/*                                                                           */
/*    workload                                                               */
/*      Their total workload                                                 */
/*                                                                           */
/*    preferences                                                            */
/*      A set of preferences giving the effect of an assignment to task      */
/*                                                                           */
/*    intersect                                                              */
/*      When non-NULL, this field holds the intersection of the resource     */
/*      groups of the preferences, including NULL when include_r0 is set     */
/*      in all of them.  It is only constructed when necessary.              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_profile_rec {
  KHE_TIME_SET		time_set;		/* times occupied by task    */
  int			duration;		/* total duration of task    */
  float			workload;		/* total workload of task    */
  ARRAY_KHE_PREFERENCE	preferences;		/* derived from monitors     */
  KHE_RESOURCE_SET	intersect;		/* of preferred resources    */
} *KHE_TASK_PROFILE;

typedef HA_ARRAY(KHE_TASK_PROFILE) ARRAY_KHE_TASK_PROFILE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_DEMAND_NODE                                                          */
/*                                                                           */
/*  One demand node of the graph, representing one or more equivalent tasks  */
/*  (tasks with equal profiles) included in the matching.                    */
/*                                                                           */
/*    demand_set                                                             */
/*      The enclosing demand set                                             */
/*                                                                           */
/*    tasks                                                                  */
/*      One or more assignable tasks, with equal profiles                    */
/*                                                                           */
/*    profile                                                                */
/*      The profile shared by the tasks                                      */
/*                                                                           */
/*    incompatibility                                                        */
/*      Only defined while handling a limit resources monitor m.  This       */
/*      is the cardinality of the symmetric difference of m's resource       */
/*      group and the intersection of the profile's resource groups.         */
/*                                                                           */
/*    mmatch_node                                                            */
/*      Only defined while matching.  The corresponding mmatch node.         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

typedef struct khe_demand_node_rec {
  KHE_RESOURCE_MATCHING_DEMAND_SET demand_set;	/* enclosing demand set      */
  ARRAY_KHE_TASK	tasks;			/* the tasks (same profile)  */
  KHE_TASK_PROFILE	profile;		/* profile of any one task   */
  float			incompatibility;	/* size of symmetric diff.   */
  int			offset_in_day;		/* offset of task in day     */
  KHE_MMATCH_NODE	mmatch_node;		/* demand node in mmatch     */
} *KHE_DEMAND_NODE;


/*****************************************************************************/
/*                                                                           */
/*   KHE_LIMIT_MONITOR - a limit workload monitor and associated task count  */
/*                                                                           */
/*****************************************************************************/

/* *** found a much better way to do this
typedef struct khe_limit_monitor_rec {
  KHE_LIMIT_RESOURCES_MONITOR	monitor;		** the monitor       **
  int				uncovered_durn;		** tasks covering it **
} *KHE_LIMIT_MONITOR;

typedef HA_ARRAY(KHE_LIMIT_MONITOR) ARRAY_KHE_LIMIT_MONITOR;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_MATCHING_DEMAND_SET                                         */
/*                                                                           */
/*  A set of demand nodes to be solved, with associated attributes.  Most    */
/*  of the fields are boilerplate or concerned with preparing the demand     */
/*  set for solving.  When solving, the following fields are used:           */
/*                                                                           */
/*    task_grouper                                                           */
/*      This object remembers which tasks have to be grouped under leader    */
/*      tasks because they are assigned the same resource initially.         */
/*                                                                           */
/*    demand_nodes                                                           */
/*      The demand nodes.  Each holds one or more equivalent tasks.          */
/*                                                                           */
/*    monitors                                                               */
/*      The affected limit resources monitors.  These have to be detached    */
/*      while the solve is running; they are being adjusted for.             */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_DEMAND_NODE) ARRAY_KHE_DEMAND_NODE;
/* typedef HA_ARRAY(KHE_TIME) ARRAY_KHE_TIME; defined in khe_solvers.h */
typedef HA_ARRAY(KHE_LIMIT_RESOURCES_MONITOR) ARRAY_KHE_LIMIT_RESOURCES_MONITOR;

struct khe_resource_matching_demand_set_rec {
  KHE_RESOURCE_MATCHING_SOLVER	solver;			/* solver            */
  KHE_TIME_SET			time_set;		/* times             */
  /* bool			preserve_existing; */	/* preserve existing */
  bool				ready_for_solving;	/* ready for solving */
  KHE_SIMPLE_GROUPER		simple_grouper;	/* groups tasks      */
  KHE_SOLN_ADJUSTER		soln_adjuster;		/* with task grouper */
  /* KHE_TASK_RESOURCE_GROUPER	task_grouper; */	/* groups tasks      */
  ARRAY_KHE_TASK		assignable_tasks;	/* assignable tasks  */
  ARRAY_KHE_DEMAND_NODE		demand_nodes;		/* the demand nodes  */
  /* ARRAY_KHE_LIMIT_MONITOR	limit_monitors;	*/	/* limit monitors    */
  ARRAY_KHE_LIMIT_RESOURCES_MONITOR monitors;		/* monitor the tasks */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_MATCHING_SOLVER                                             */
/*                                                                           */
/*  A resource matching solver.                                              */
/*                                                                           */
/*  The supply_nodes array holds all the supply nodes, one for each of the   */
/*  resources of the resource group, plus one representing non-assignment.   */
/*  These are fixed throughout the lifetime of the solver.                   */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_SUPPLY_NODE) ARRAY_KHE_SUPPLY_NODE;

struct khe_resource_matching_solver_rec {

  /* fields with a well-defined value for the lifetime of the solver */
  HA_ARENA			arena;			/* arena             */
  KHE_SOLN			soln;			/* the solution      */
  KHE_RESOURCE_GROUP		resource_group;		/* resources affected*/
  bool				max_busy_set;		/* max_busy is set   */
  KHE_MMATCH			mmatch;			/* match object      */
  ARRAY_KHE_SUPPLY_NODE		supply_nodes;		/* supply nodes      */
  ARRAY_KHE_PREFERENCE		free_preferences;	/* free preferences  */
  ARRAY_KHE_TASK_PROFILE	free_task_profiles;	/* free task profiles*/
  ARRAY_KHE_DEMAND_NODE		free_demand_nodes;	/* free demand nodes */
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET free_demand_sets;
  int				prev_last_time_index;	/* from prev solve   */

  /* fields with a well-defined value during a single solve */
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET demand_sets;	/* current solve     */
  /* KHE_RESOURCE_MATCHING_DEMAND_SET demand_set; */	/* demand set        */
  KHE_OPTIONS			options;		/* options           */
  KHE_FRAME			common_frame;		/* common frame      */
  KHE_TASK_GROUP_DOMAIN_FINDER	domain_finder;		/* domain finder     */
  bool				resource_invariant;	/* preserve invt     */
  bool				edge_adjust1_off;	/* don't adjust edges*/
  bool				edge_adjust2_off;	/* don't adjust edges*/
  bool				edge_adjust3_off;	/* don't adjust edges*/
  bool				edge_adjust4_off;	/* don't adjust edges*/

  /* combinatorial assignment */
  ARRAY_KHE_TASK		tasks;			/* tasks assigned    */
  ARRAY_KHE_TASK		best_tasks;		/* best assigned     */
  KHE_COST			best_cost;		/* best cost         */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit monitors"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_MONITOR KheLimitMonitorMake(KHE_LIMIT_RESOURCES_MONITOR lrm,   */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Make a limit monitor object holding lrm, add it to rmds, and return it.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_LIMIT_MONITOR KheLimitMonitorMake(KHE_LIMIT_RESOURCES_MONITOR lrm,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  HA_ARENA a;  KHE_LIMIT_MONITOR res;
  int minimum, maximum, active_durn;
  a = rmds->solver->arena;
  HaMake(res, a);
  res->monitor = lrm;
  KheLimitResourcesMonitorActiveDuration(lrm, &minimum, &maximum, &active_durn);
  res->uncovered_durn = minimum - active_durn;
  HaArrayAddLast(rmds->limit_monitors, res);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_MONITOR KheLimitMonitorFind(KHE_LIMIT_RESOURCES_MONITOR lrm,   */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Return a limit monitor of rmds containing lrm.  If there is not one      */
/*  already, make it.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_LIMIT_MONITOR KheLimitMonitorFind(KHE_LIMIT_RESOURCES_MONITOR lrm,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_LIMIT_MONITOR lm;  int i;
  HaArrayForEach(rmds->limit_monitors, lm, i)
    if( lm->monitor == lrm )
      return lm;
  return KheLimitMonitorMake(lrm, rmds);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAsstNeededToSatisfyMonitors(KHE_TASK task,                   */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Return true if, in order to satisfy its monitorss, task should be        */
/*  assigned.  We assume that it is not assigned already.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheTaskAsstNeededToSatisfyMonitors(KHE_TASK task,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  int i;  KHE_TASK child_task;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  KHE_SOLN soln;  KHE_CONSTRAINT c;  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_LIMIT_MONITOR lm;
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    soln = rmds->solver->soln;
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      c = KheMonitor Constraint(m);
      if( KheConstr aintCombinedWeight(c) > 0 && KheMonitorAttachedToSoln(m) )
	switch( KheMonitorTag(m) )
	{
	  case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	    ** task is subject to an assign resource constraint **
	    return true;

	  case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	    lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	    lm = KheLimitMonitorFind(lrm, rmds);
	    if( lm->uncovered_durn > 0 )
	      return true;
	    break;

	  default:

	    ** other monitors are irrelevant **
	    break;
	}
    }
  }

  ** do the same job for the tasks assigned to task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskAsstNeededToSatisfyMonitors(child_task, rmds) )
      return true;
  }

  ** not needed **
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAddToLimitMonitors(KHE_TASK task,                            */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  We're adding task to rmds.  Update the covers of the limit resources     */
/*  monitors appropriately.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskAddToLimitMonitors(KHE_TASK task,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  int i, durn;  KHE_TASK child_task;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  KHE_SOLN soln;  KHE_CONSTRAINT c;  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_LIMIT_MONITOR lm;
  er = KheTaskEventResource(task);
  durn = KheTaskDuration(task);
  if( er != NULL )
  {
    soln = rmds->solver->soln;
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( KheMonitorTag(m) == KHE_LIMIT_RESOURCES_MONITOR_TAG &&
          KheMonitorAttachedToSoln(m) )
      {
	c = KheMonitorCons traint(m);
	if( KheConstrai ntCombinedWeight(c) > 0 )
	{
	  lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	  lm = KheLimitMonitorFind(lrm, rmds);
          lm->uncovered_durn -= durn;
	}
      }
    }
  }

  ** do the same job for the tasks assigned to task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskAddToLimitMonitors(child_task, rmds);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "supply nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SUPPLY_NODE KheSupplyNodeMake(KHE_RESOURCE_MATCHING_SOLVER rms,      */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Make a new supply node and add it to rms->supply_nodes.  If r != NULL,   */
/*  the node represents assigning r, otherwise it represents non-assignment. */
/*                                                                           */
/*****************************************************************************/

static KHE_SUPPLY_NODE KheSupplyNodeMake(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE r)
{
  KHE_SUPPLY_NODE res;
  HaMake(res, rms->arena);
  res->solver = rms;
  res->resource = r;
  /* res->group_monitor = NULL; */
  res->max_busy = 0;  /* will be revised later if required */
  res->curr_busy = 0;    /* reset for each match  */
  /* res->curr_at_max = 0; */  /* reset for each match  */
  res->curr_was_used = false;	/* reset for each match  */
  res->curr_run_len = 0;	/* reset for each match  */
  res->curr_offset = 0;		/* reset for each match  */
  res->mmatch_node = NULL;
  HaArrayAddLast(rms->supply_nodes, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSupplyNodeMultiplicity(KHE_SUPPLY_NODE sn)                        */
/*                                                                           */
/*  Return the multiplicity of sn.  If it is a resource node, this is        */
/*  just 1.  Otherwise it is the total multiplicity of all demand nodes.     */
/*                                                                           */
/*****************************************************************************/
static int KheDemandNodeMultiplicity(KHE_DEMAND_NODE dn);

static int KheSupplyNodeMultiplicity(KHE_SUPPLY_NODE sn)
{
  int res, i;  KHE_DEMAND_NODE dn;  KHE_RESOURCE_MATCHING_DEMAND_SET rmds;
  if( sn->resource != NULL )
    return 1;
  else
  {
    rmds = HaArrayFirst(sn->solver->demand_sets);
    res = 0;
    HaArrayForEach(rmds->demand_nodes, dn, i)
      res += KheDemandNodeMultiplicity(dn);
    return res;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeAddMMatchNode(KHE_SUPPLY_NODE sn, KHE_MMATCH m)        */
/*                                                                           */
/*  Add a mmatch node corresponding to sn to m.                              */
/*                                                                           */
/*****************************************************************************/

static void KheSupplyNodeAddMMatchNode(KHE_SUPPLY_NODE sn, KHE_MMATCH m)
{
  sn->mmatch_node = KheMMatchSupplyNodeMake(m, KheSupplyNodeMultiplicity(sn),
    (void *) sn);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOldResourceBusyTimes(KHE_SOLN soln, KHE_RESOURCE r)               */
/*                                                                           */
/*  Return the number of times that r is currently busy in soln.             */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheOldResourceBusyTimes(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(soln, r);
  return KheResourceTimeta bleMonitorBusyTimes(rtm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeDebug(KHE_SUPPLY_NODE sn, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing a supply node.                               */
/*                                                                           */
/*****************************************************************************/

static void KheSupplyNodeDebug(KHE_SUPPLY_NODE sn, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( sn->resource == NULL )
    fprintf(fp, "noassign");
  else
  {
    fprintf(fp, "%s", KheResourceId(sn->resource));
    if( !sn->solver->edge_adjust1_off )
      fprintf(fp, ":mb%dcb%d", sn->max_busy, sn->curr_busy);
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSupplyNodeUnTypedDebug(void *value, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing a supply node.                               */
/*                                                                           */
/*****************************************************************************/

static void KheSupplyNodeUnTypedDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  KheSupplyNodeDebug((KHE_SUPPLY_NODE) value, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preferences"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFERENCE KhePreferenceMake(KHE_RESOURCE_MATCHING_SOLVER rms,       */
/*    KHE_RESOURCE_GROUP rg, bool include_r0, KHE_MONITOR_TAG monitor_tag,   */
/*    KHE_COST cost)                                                         */
/*                                                                           */
/*  Make an preference object with these attributes.  Memory for it          */
/*  comes from a free list in rms, or else from rms's arena.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_PREFERENCE KhePreferenceMake(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE_GROUP rg, bool include_r0, KHE_MONITOR_TAG monitor_tag,
  KHE_COST cost)
{
  KHE_PREFERENCE res;
  if( HaArrayCount(rms->free_preferences) > 0 )
    res = HaArrayLastAndDelete(rms->free_preferences);
  else
    HaMake(res, rms->arena);
  res->resource_group = rg;
  res->include_r0 = include_r0;
  res->monitor_tag = monitor_tag;
  res->cost = cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFERENCE KhePreferenceCopy(KHE_PREFERENCE pref,                    */
/*    KHE_RESOURCE_MATCHING_SOLVER rms)                                      */
/*                                                                           */
/*  Return a copy of pref.                                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_PREFERENCE KhePreferenceCopy(KHE_PREFERENCE pref,
  KHE_RESOURCE_MATCHING_SOLVER rms)
{
  return KhePreferenceMake(rms, pref->resource_group, pref->include_r0,
    pref->monitor_tag, pref->cost);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOptionalResourceGroupEqual(KHE_RESOURCE_GROUP rg1,               */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Return true if rg1 and rg2 are either both NULL or both non-NULL and     */
/*  equal.                                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheOptionalResourceGroupEqual(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2)
{
  if( rg1 == NULL )
    return rg2 == NULL;
  else if( rg2 == NULL )
    return false;
  else
    return KheResourceGroupEqual(rg1, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferenceMergeable(KHE_PREFERENCE pref, KHE_RESOURCE_GROUP rg,  */
/*    bool include_r0, KHE_MONITOR_TAG monitor_tag)                          */
/*                                                                           */
/*  If pref can merge with a new preference with these attributes, return    */
/*  true, otherwise return false.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferenceMergeable(KHE_PREFERENCE pref, KHE_RESOURCE_GROUP rg,
  bool include_r0, KHE_MONITOR_TAG monitor_tag)
{
  return pref->monitor_tag == monitor_tag && pref->include_r0 == include_r0
    && KheOptionalResourceGroupEqual(pref->resource_group, rg);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupOptionalCmp(KHE_RESOURCE_GROUP rg1,                  */
/*    KHE_RESOURCE_GROUP rg2)                                                */
/*                                                                           */
/*  Comparison function for comparing two optional resource groups.          */
/*                                                                           */
/*****************************************************************************/

static int KheResourceGroupOptionalCmp(KHE_RESOURCE_GROUP rg1,
  KHE_RESOURCE_GROUP rg2)
{
  if( rg1 == NULL )
    return rg2 == NULL ? 0 : -1;
  else if( rg2 == NULL )
    return 1;
  else
    return KheResourceGroupTypedCmp(rg1, rg2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferenceTypedCmp(KHE_PREFERENCE pref1, KHE_PREFERENCE pref2)    */
/*                                                                           */
/*  Typed comparison function for comparing preferences.                     */
/*                                                                           */
/*****************************************************************************/

static int KhePreferenceTypedCmp(KHE_PREFERENCE pref1, KHE_PREFERENCE pref2)
{
  int cmp;
  cmp = (int) pref1->monitor_tag - (int) pref2->monitor_tag;
  if( cmp != 0 ) return cmp;
  cmp = KheCostCmp(pref1->cost, pref2->cost);
  if( cmp != 0 ) return cmp;
  cmp = (int) pref1->include_r0 - (int) pref2->include_r0;
  if( cmp != 0 ) return cmp;
  return KheResourceGroupOptionalCmp(pref1->resource_group,
    pref2->resource_group);
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferenceCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of preferences into a           */
/*  canonical order.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KhePreferenceCmp(const void *t1, const void *t2)
{
  KHE_PREFERENCE pref1 = * (KHE_PREFERENCE *) t1;
  KHE_PREFERENCE pref2 = * (KHE_PREFERENCE *) t2;
  return KhePreferenceTypedCmp(pref1, pref2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferenceDebug(KHE_PREFERENCE pref, int verbosity,              */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of pref onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KhePreferenceDebug(KHE_PREFERENCE pref, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "(");
  if( pref->resource_group == NULL )
    fprintf(fp, "(all)");
  else
  {
    KheResourceGroupDebug(pref->resource_group, 1, -1, fp);
    fprintf(fp, " (%d resources)",
      KheResourceGroupResourceCount(pref->resource_group));
  }
  if( pref->include_r0 )
    fprintf(fp, " + r0");
  fprintf(fp, ", %.5f)", KheCostShow(pref->cost));
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task monitored duration" (stored in visit num)                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskClearMonitoredDurn(KHE_TASK task)                            */
/*                                                                           */
/*  Clear the monitored duration of task back to 0.                          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskClearMonitoredDurn(KHE_TASK task)
{
  KheTaskSetVisitNum(task, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskAddMonitoredDurn(KHE_TASK task, int durn)                    */
/*                                                                           */
/*  Add durn to task's monitored duration.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTaskAddMonitoredDurn(KHE_TASK task, int durn)
{
  KheTaskSetVisitNum(task, KheTaskVisitNum(task) + durn);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskMonitoredDurn(KHE_TASK task)                                  */
/*                                                                           */
/*  Return the monitored duration of task.                                   */
/*                                                                           */
/*****************************************************************************/

static int KheTaskMonitoredDurn(KHE_TASK task)
{
  return KheTaskVisitNum(task);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task profiles"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_PROFILE KheTaskProfileMake(KHE_RESOURCE_MATCHING_SOLVER rms)    */
/*                                                                           */
/*  Make an empty task profile object.  Get the object either from the free  */
/*  list in rms (in which case its time set can also be re-used), or else    */
/*  from rms's arena.                                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_PROFILE KheTaskProfileMake(KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_TASK_PROFILE res;  KHE_INSTANCE ins;
  if( HaArrayCount(rms->free_task_profiles) > 0 )
  {
    res = HaArrayLastAndDelete(rms->free_task_profiles);
    KheTimeSetClear(res->time_set);
    HaArrayClear(res->preferences);
  }
  else
  {
    HaMake(res, rms->arena);
    ins = KheSolnInstance(rms->soln);
    res->time_set = KheTimeSetMake(ins, rms->arena);
    HaArrayInit(res->preferences, rms->arena);
  }
  res->duration = -1;
  res->workload = -1.0;
  res->intersect = NULL;
  /* res->intersect_r0 = false; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_PROFILE KheTaskProfileCopy(KHE_TASK_PROFILE tp,                 */
/*    KHE_RESOURCE_MATCHING_SOLVER rms)                                      */
/*                                                                           */
/*  Return a copy of tp.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_PROFILE KheTaskProfileCopy(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_TASK_PROFILE res;  KHE_PREFERENCE pref;  int i;
  res = KheTaskProfileMake(rms);
  KheTimeSetCopyElements(res->time_set, tp->time_set);
  res->duration = tp->duration;
  res->workload = tp->workload;
  HaArrayForEach(tp->preferences, pref, i)
    HaArrayAddLast(res->preferences, KhePreferenceCopy(pref, rms));
  res->intersect = (tp->intersect == NULL ? NULL :
    KheResourceSetCopy(tp->intersect, rms->arena));
  /* res->intersect_r0 = tp->intersect_r0; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskProfileDelete(KHE_TASK_PROFILE tp,                           */
/*    KHE_RESOURCE_MATCHING_SOLVER rms)                                      */
/*                                                                           */
/*  Free tp.  Place its preferences onto the free list in rms, but do not    */
/*  do anything of that kind with its time set, because it will re-use its   */
/*  time set if it is itself re-used.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheTaskProfileDelete(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_MATCHING_SOLVER rms)
{
  int i;
  HaArrayAppend(rms->free_preferences, tp->preferences, i);
  HaArrayAddLast(rms->free_task_profiles, tp);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskProfileAddPreference(KHE_TASK_PROFILE tp,                    */
/*    KHE_RESOURCE_GROUP rg, bool include_r0, KHE_MONITOR_TAG monitor_tag,   */
/*    KHE_COST cost, KHE_RESOURCE_MATCHING_SOLVER rms)                       */
/*                                                                           */
/*  Add a new preference with these attributes to tp, or possibly merge      */
/*  it into an existing preference.                                          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskProfileAddPreference(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_GROUP rg, bool include_r0, KHE_MONITOR_TAG monitor_tag,
  KHE_COST cost, KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_PREFERENCE pref;  int i;
  HaArrayForEach(tp->preferences, pref, i)
    if( KhePreferenceMergeable(pref, rg, include_r0, monitor_tag) )
    {
      pref->cost += cost;
      return;
    }
  pref = KhePreferenceMake(rms, rg, include_r0, monitor_tag, cost);
  HaArrayAddLast(tp->preferences, pref);
  if( tp->intersect != NULL )
  {
    /* update tp->intersect, if present */
    if( rg != NULL )
      KheResourceSetIntersectGroup(tp->intersect, rg);
    if( !include_r0 )
      KheResourceSetDeleteResource(tp->intersect, NULL);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskProfileShowsNodeIsOptional(KHE_TASK_PROFILE tp)              */
/*                                                                           */
/*  Return true if assigning the tasks of a node containing tp is optional,  */
/*  because tp's preferences all contain r0.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheTaskProfileShowsNodeIsOptional(KHE_TASK_PROFILE tp)
{
  KHE_PREFERENCE pref;  int i;
  HaArrayForEach(tp->preferences, pref, i)
    if( !pref->include_r0 )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskProfileReduceToLimitResourcesOnly(KHE_TASK_PROFILE tp,       */
/*    KHE_RESOURCE_MATCHING_SOLVER rms)                                      */
/*                                                                           */
/*  Remove all preferences from tp except those derived from limit           */
/*  resources monitors.  Add the deleted preferences to rms's free list.     */
/*                                                                           */
/*****************************************************************************/

static void KheTaskProfileReduceToLimitResourcesOnly(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_PREFERENCE pref;  int i;
  HaArrayForEach(tp->preferences, pref, i)
    if( pref->monitor_tag != KHE_LIMIT_RESOURCES_MONITOR_TAG )
    {
      HaArrayAddLast(rms->free_preferences, pref);
      HaArrayDeleteAndPlug(tp->preferences, i);
      i--;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskProfileHandleMonitors(KHE_TASK_PROFILE tp, KHE_TASK task,    */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Visit task, which is an affected task, and its descendants, adding       */
/*  preferences for the assign resource and prefer resources monitors to     */
/*  tp.  Also add limit resources monitors to the list in rmds.              */
/*                                                                           */
/*****************************************************************************/

static void KheTaskProfileHandleMonitors(KHE_TASK_PROFILE tp, KHE_TASK task,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  int i, durn;  KHE_TASK child_task;  KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  KHE_SOLN soln;  KHE_RESOURCE_MATCHING_SOLVER rms;
  KHE_COST weight;  KHE_RESOURCE_GROUP rg;  KHE_PREFER_RESOURCES_MONITOR prm;

  /* handle task's monitors */
  er = KheTaskEventResource(task);
  if( er != NULL )
  {
    rms = rmds->solver;
    soln = rms->soln;
    durn = KheTaskDuration(task);
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, i);
      if( KheMonitorAttachedToSoln(m) )
      {
	weight = KheMonitorCombinedWeight(m);
	if( weight > 0 ) switch( KheMonitorTag(m) )
	{
	  case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	    KheTaskProfileAddPreference(tp, NULL /* means all */, false,
	      KHE_ASSIGN_RESOURCE_MONITOR_TAG, weight * durn, rms);
	    break;

	  case KHE_PREFER_RESOURCES_MONITOR_TAG:

	    prm = (KHE_PREFER_RESOURCES_MONITOR) m;
	    rg = KhePreferResourcesMonitorDomain(prm);
	    KheTaskProfileAddPreference(tp, rg, true,
	      KHE_PREFER_RESOURCES_MONITOR_TAG, weight * durn, rms);
	    break;

	  case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	    /* save these for later */
	    HaArrayAddLast(rmds->monitors, (KHE_LIMIT_RESOURCES_MONITOR) m);
	    break;

	  default:

	    /* do nothing */
	    break;
	}
      }
    }
  }

  /* do the same job for the tasks assigned to task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskProfileHandleMonitors(tp, child_task, rmds);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_PROFILE KheTaskProfileSetup(KHE_TASK task,                      */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Set up a new task profile based on task, including adding and sorting    */
/*  its preferences.  Add any limit resources monitors encountered along     */
/*  the way to the list in rmds->solver.                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_PROFILE KheTaskProfileSetup(KHE_TASK task,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_TASK_PROFILE res;
  res = KheTaskProfileMake(rmds->solver);
  KheTimeSetAddTaskTimes(res->time_set, task);
  res->duration = KheTimeSetTimeCount(res->time_set);
  res->workload = KheTaskTotalWorkload(task);
  KheTaskProfileHandleMonitors(res, task, rmds);
  HaArraySort(res->preferences, &KhePreferenceCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskProfileResourceGroupCount(KHE_TASK_PROFILE tp,                */
/*    KHE_RESOURCE_GROUP groups[10], bool *intersect_r0)                     */
/*                                                                           */
/*  Return the number of non-NULL resource groups in tp's preferences, and   */
/*  set groups[] to the first 10 of those resource groups.  Also set         */
/*  *intersect_r0 to true if r0 is present in the intersection.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheTaskProfileResourceGroupCount(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_GROUP groups[10], bool *intersect_r0)
{
  int res, i;  KHE_PREFERENCE pref;
  res = 0;
  *intersect_r0 = true;
  HaArrayForEach(tp->preferences, pref, i)
  {
    if( pref->resource_group != NULL && res < 5 )
      groups[res++] = pref->resource_group;
    if( !pref->include_r0 )
      *intersect_r0 = false;
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskProfileIncompatibility(KHE_TASK_PROFILE tp,                   */
/*    KHE_RESOURCE_GROUP rg, KHE_RESOURCE_MATCHING_SOLVER rms)               */
/*                                                                           */
/*  Return the incompatibility of tp for rg:  the cardinality of the         */
/*  symmetric difference of rg and the intersection of the resource          */
/*  sets of tp's preferences.                                                */
/*                                                                           */
/*  NB this function is always derived from a limit resources constraint,    */
/*  and hence rg never contains r0.                                          */
/*                                                                           */
/*  This function is optimized to avoid set operations as far as possible.   */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  DC_NO_GROUPS,
  DC_ONE_GROUP,
  DC_MULTI_GROUPS
} DC_STATE;

static float KheTaskProfileIncompatibility(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_GROUP rg, KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_RESOURCE_TYPE rt;  DC_STATE state;  KHE_PREFERENCE pref;
  int i, old_count, new_count;  bool include_r0;
  KHE_RESOURCE_GROUP sole_rg;
  if( tp->intersect != NULL )
  {
    /* tp->intersect is present; it holds the intersection,   */
    /* including a NULL entry if the intersection includes r0 */
    old_count = KheResourceSetResourceCount(tp->intersect);
    new_count = KheResourceSetIntersectCountGroup(tp->intersect, rg);
    /* ***
    return KheResourceSetSymmetricDifferenceCountGroup(tp->intersect, rg);
    *** */
  }
  else
  {
    /* find the intersection of the existing preferences' resource    */
    /* groups, possibly creating tp->intersect along the way.  Even   */
    /* if r0 has to be included, it is not added to tp->intersect     */
    /* during the loop; it is kept in include_r0 and added afterwards */
    rt = KheResourceGroupResourceType(rms->resource_group);
    state = DC_NO_GROUPS;
    sole_rg = NULL;
    include_r0 = true;
    HaArrayForEach(tp->preferences, pref, i)
    {
      if( !pref->include_r0 )
	include_r0 = false;
      if( pref->resource_group != NULL ) switch( state )
      {
	case DC_NO_GROUPS:

	  sole_rg = pref->resource_group;
	  state = DC_ONE_GROUP;
	  break;

	case DC_ONE_GROUP:

	  tp->intersect = KheResourceSetMake(rt, rms->arena);
	  KheResourceSetUnionGroup(tp->intersect, sole_rg);
	  KheResourceSetIntersectGroup(tp->intersect, pref->resource_group);
	  sole_rg = NULL;
	  state = DC_MULTI_GROUPS;
	  break;

	case DC_MULTI_GROUPS:

	  KheResourceSetIntersectGroup(tp->intersect, pref->resource_group);
	  break;

	default:

	  HnAbort("KheTaskProfileIncompatibility internal error");
	  break;
      }
    }
    switch( state )
    {
      case DC_NO_GROUPS:

	/* no resource groups, intersection is all resources of the type */
	old_count = KheResourceTypeResourceCount(rt) + (int) include_r0;
	new_count = KheResourceGroupResourceCount(rg);
	/* ***
	return (KheResourceTypeResourceCount(rt) + intersect_r0) -
	  KheResourceGroupResourceCount(rg);
	*** */
	break;

      case DC_ONE_GROUP:

	/* one resource group, have to intersect with that */
	old_count = KheResourceGroupResourceCount(sole_rg) + (int) include_r0;
	new_count = KheResourceGroupIntersectCount(sole_rg, rg);
	/* ***
	return KheResourceGroupSymmetricDifferenceCount(rg, sole_rg) +
	  intersect_r0;
	*** */
	break;

      case DC_MULTI_GROUPS:

	/* two or more resource groups, so tp->intersect is set; use it */
	if( include_r0 )
	  KheResourceSetAddResource(tp->intersect, NULL);
	old_count = KheResourceSetResourceCount(tp->intersect);
	new_count = KheResourceSetIntersectCountGroup(tp->intersect, rg);
	/* ***
	return KheResourceSetSymmetricDifferenceCountGroup(tp->intersect, rg);
	*** */
	break;

      default:

	HnAbort("KheTaskProfileIncompatibility internal error");
	old_count = 0;  /* keep compiler happy */
	new_count = 0;  /* keep compiler happy */
	/* ***
	return 0;  ** keep compiler happy **
	*** */
	break;
    }
  }
  return 1.0 - (old_count == 0 ? 0.0 : (float) new_count / (float) old_count);
}

/* *** old version that calls KheTaskProfileResourceGroupCount
static int KheTaskProfileIncompatibility(KHE_TASK_PROFILE tp,
  KHE_RESOURCE_GROUP rg, KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_RESOURCE_TYPE rt;
  KHE_RESOURCE_GROUP groups[10];  int i, count;  bool intersect_r0;
  if( tp->intersect != NULL )
  {
    ** tp->intersect is present, meaning that it holds the intersection **
    return KheResourceSetSymmetricDifferenceCountGroup(tp->intersect, rg);
      ** + (int) tp->intersect_r0; **
  }
  else
  {
    rt = KheResourceGroupResourceType(rms->resource_group);
    count = KheTaskProfileResourceGroupCount(tp, groups, &intersect_r0);
    if( count == 0 )
    {
      ** no resource groups, intersection is all resources of the type **
      ** including r0 **
      return (KheResourceTypeResourceCount(rt) + 1) -
	KheResourceGroupResourceCount(rg);
    }
    else if( count == 1 )
    {
      ** one resource group, have to intersect with that **
      return KheResourceGroupSymmetricDifferenceCount(rg, groups[0]) +
	(int) intersect_r0;
    }
    else
    {
      ** two or more resource groups, need to find intersection and use it **
      tp->intersect = KheResourceSetMake(rt, rms->arena);
      KheResourceSetUnionGroup(tp->intersect, groups[0]);
      for( i = 1;  i < count;  i++ )
	KheResourceSetIntersectGroup(tp->intersect, groups[i]);
      if( intersect_r0 )
	KheResourceSetAddResource(tp->intersect, NULL);
      return KheResourceSetSymmetricDifferenceCountGroup(tp->intersect, rg);
        ** + (int) intersect_r0; **
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int FloatTypedCmp(float f1, float f2)                                    */
/*                                                                           */
/*  Comparison function for comparing two floats.                            */
/*                                                                           */
/*****************************************************************************/

static int FloatTypedCmp(float f1, float f2)
{
  return f1 < f2 ? -1 : f1 == f2 ? 0 : 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskProfileTypedCmp(KHE_TASK_PROFILE tp1, KHE_TASK_PROFILE tp2)   */
/*                                                                           */
/*  Typed comparison function for sorting task profiles, assuming that       */
/*  their preferences have been sorted.                                      */
/*                                                                           */
/*****************************************************************************/

static int KheTaskProfileTypedCmp(KHE_TASK_PROFILE tp1, KHE_TASK_PROFILE tp2)
{
  int i, cmp;  KHE_PREFERENCE pref1, pref2;
  cmp = KheTimeSetTypedCmp(tp1->time_set, tp2->time_set);
  if( cmp != 0 ) return cmp;
  cmp = tp1->duration - tp2->duration;
  if( cmp != 0 ) return cmp;
  cmp = FloatTypedCmp(tp1->workload, tp2->workload);
  if( cmp != 0 ) return cmp;
  cmp = HaArrayCount(tp1->preferences) - HaArrayCount(tp2->preferences);
  if( cmp != 0 ) return cmp;
  for( i = 0;  i < HaArrayCount(tp1->preferences);  i++ )
  {
    pref1 = HaArray(tp1->preferences, i);
    pref2 = HaArray(tp2->preferences, i);
    cmp = KhePreferenceTypedCmp(pref1, pref2);
    if( cmp != 0 ) return cmp;
  }
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskProfileDebug(KHE_TASK_PROFILE tp, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of tp onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheTaskProfileDebug(KHE_TASK_PROFILE tp, int verbosity,
  int indent, FILE *fp)
{
  KHE_PREFERENCE pref;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Task Profile(", indent, "");
    KheTimeSetDebug(tp->time_set, verbosity, -1, fp);
    fprintf(fp, " durn %d, workload %.1f)\n", tp->duration, tp->workload);
    HaArrayForEach(tp->preferences, pref, i)
      KhePreferenceDebug(pref, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand nodes"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DEMAND_NODE KheDemandNodeMake(KHE_RESOURCE_MATCHING_DEMAND_SET rmds) */
/*                                                                           */
/*  Make a new demand node, initially containing no tasks, and add it to     */
/*  rmds->demand_nodes.                                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_DEMAND_NODE KheDemandNodeMake(KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_DEMAND_NODE res;  KHE_RESOURCE_MATCHING_SOLVER rms;  HA_ARENA a;
  rms = rmds->solver;
  a = rms->arena;
  if( HaArrayCount(rms->free_demand_nodes) > 0 )
  {
    res = HaArrayLastAndDelete(rms->free_demand_nodes);
    HaArrayClear(res->tasks);
  }
  else
  {
    HaMake(res, a);
    HaArrayInit(res->tasks, a);
  }
  res->demand_set = rmds;
  res->profile = NULL;
  res->offset_in_day = 0;
  res->incompatibility = 0.0;
  res->mmatch_node = NULL;
  HaArrayAddLast(rmds->demand_nodes, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskOffsetInDay(KHE_TASK task, KHE_FRAME frame)                   */
/*                                                                           */
/*  Return the offset in its day of task.  This more or less corresponds     */
/*  with the shift type.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheTaskOffsetInDay(KHE_TASK task, KHE_FRAME frame)
{
  KHE_MEET meet;  KHE_TIME t; /* , t2;  int index;  KHE_TIME_GROUP tg; */
  meet = KheTaskMeet(task);
  if( meet == NULL )
    return 0;
  t = KheMeetAsstTime(meet);
  if( t == NULL )
    return 0;
  else
    return KheFrameTimeOffset(frame, t);
  /* ***
  index = KheFrameTimeIndex(frame, t);
  tg = KheFrameTimeGroup(frame, index);
  if( KheTimeGroupTimeCount(tg) == 0 )
    return 0;
  t2 = KheTimeGroupTime(tg, 0);
  return KheTimeIndex(t) - KheTimeIndex(t2);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeAddTask(KHE_DEMAND_NODE dn, KHE_TASK task)             */
/*                                                                           */
/*  Add task to dn.  Also set offset_in_day if this is the first task.       */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeAddTask(KHE_DEMAND_NODE dn, KHE_TASK task)
{
  KHE_FRAME frame;  int pos;
  if( HaArrayCount(dn->tasks) == 0 )
  {
    frame = dn->demand_set->solver->common_frame;
    dn->offset_in_day = KheTaskOffsetInDay(task, frame);
  }
  HnAssert(!HaArrayContains(dn->tasks, task, &pos),
    "KheDemandNodeAddTask internal error");
  HaArrayAddLast(dn->tasks, task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DEMAND_NODE KheDemandNodeCopy(KHE_DEMAND_NODE dn)                    */
/*                                                                           */
/*  Return a copy of dn, including its profile but not its tasks.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_DEMAND_NODE KheDemandNodeCopy(KHE_DEMAND_NODE dn)
{
  KHE_DEMAND_NODE res;  KHE_RESOURCE_MATCHING_SOLVER rms;
  rms = dn->demand_set->solver;
  res = KheDemandNodeMake(dn->demand_set);
  res->profile = KheTaskProfileCopy(dn->profile, rms);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeMultiplicity(KHE_DEMAND_NODE dn)                        */
/*                                                                           */
/*  Return the multiplicity of dn.                                           */
/*                                                                           */
/*****************************************************************************/

static int KheDemandNodeMultiplicity(KHE_DEMAND_NODE dn)
{
  return HaArrayCount(dn->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeAddMMatchNode(KHE_DEMAND_NODE dn, KHE_MMATCH m)        */
/*                                                                           */
/*  Add a mmatch node corresponding to dn to m.                              */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeAddMMatchNode(KHE_DEMAND_NODE dn, KHE_MMATCH m)
{
  dn->mmatch_node = KheMMatchDemandNodeMake(m, KheDemandNodeMultiplicity(dn),
    (void *) dn);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandNodeAssignOneTask(KHE_DEMAND_NODE dn, KHE_TASK task,       */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Try to assign r to task (a task from dn), optionally preserving the      */
/*  resource assignment invariant.  Return true if task is assigned r        */
/*  afterwards.                                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandNodeAssignOneTask(KHE_DEMAND_NODE dn, KHE_TASK task,
  KHE_RESOURCE r)
{
  KHE_RESOURCE_MATCHING_SOLVER rms;  int dc;  bool all_assigned;
  rms = dn->demand_set->solver;
  dc = KheSolnMatchingDefectCount(rms->soln);
  all_assigned = KheEjectingTaskMoveFrame(task, r, false, rms->common_frame);
  return all_assigned &&
    (!rms->resource_invariant || KheSolnMatchingDefectCount(rms->soln) <= dc);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeUnAssignTasks(KHE_DEMAND_NODE dn)                      */
/*                                                                           */
/*  Unassign the assigned tasks of dn.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeUnAssignTasks(KHE_DEMAND_NODE dn)
{
  int i;  KHE_TASK task;
  HaArrayForEach(dn->tasks, task, i)
    if( KheTaskAsstResource(task) != NULL )
      KheTaskUnAssign(task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeDelete(KHE_DEMAND_NODE dn)                             */
/*                                                                           */
/*  Delete dn, placing its bits and pieces onto free lists in rms.           */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeDelete(KHE_DEMAND_NODE dn)
{
  KHE_RESOURCE_MATCHING_SOLVER rms;
  rms = dn->demand_set->solver;
  KheTaskProfileDelete(dn->profile, rms);
  HaArrayAddLast(rms->free_demand_nodes, dn);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeTaskProfileTypedCmp(KHE_DEMAND_NODE dn1,                */
/*    KHE_DEMAND_NODE dn2)                                                   */
/*                                                                           */
/*  Comparison function for sorting demand nodes to bring nodes with         */
/*  equal task profiles together.                                            */
/*                                                                           */
/*****************************************************************************/

static int KheDemandNodeTaskProfileTypedCmp(KHE_DEMAND_NODE dn1,
  KHE_DEMAND_NODE dn2)
{
  return KheTaskProfileTypedCmp(dn1->profile, dn2->profile);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeTaskProfileCmp(const void *t1, const void *t2)          */
/*                                                                           */
/*  Comparison function for sorting demand nodes to bring nodes with         */
/*  equal task profiles together.                                            */
/*                                                                           */
/*****************************************************************************/

static int KheDemandNodeTaskProfileCmp(const void *t1, const void *t2)
{
  KHE_DEMAND_NODE dn1 = * (KHE_DEMAND_NODE *) t1;
  KHE_DEMAND_NODE dn2 = * (KHE_DEMAND_NODE *) t2;
  return KheTaskProfileTypedCmp(dn1->profile, dn2->profile);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeIncreasingIncompatibilityCmp(const void *t1,            */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Comparison function for sorting an array of demand nodes by increasing   */
/*  incompatibility.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheDemandNodeIncreasingIncompatibilityCmp(const void *t1,
  const void *t2)
{
  float cmp;
  KHE_DEMAND_NODE dn1 = * (KHE_DEMAND_NODE *) t1;
  KHE_DEMAND_NODE dn2 = * (KHE_DEMAND_NODE *) t2;
  cmp = dn1->incompatibility - dn2->incompatibility;
  return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeDebug(KHE_DEMAND_NODE dn, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing a demand node.                               */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeDebug(KHE_DEMAND_NODE dn, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK task;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Demand Node {", indent, "");
    HaArrayForEach(dn->tasks, task, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheTaskDebug(task, 2, -1, fp);
    }
    fprintf(fp, "}\n");
    if( verbosity >= 2 && dn->profile != NULL )
      KheTaskProfileDebug(dn->profile, 2, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    fprintf(fp, "Demand Node {");
    HaArrayForEach(dn->tasks, task, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheTaskDebug(task, 2, -1, fp);
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeUnTypedDebug(void *value, int verbosity,               */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing a demand node.                               */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeUnTypedDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  KheDemandNodeDebug((KHE_DEMAND_NODE) value, verbosity, indent, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "demand sets - construction"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_MATCHING_DEMAND_SET KheResourceMatchingDemandSetMake(       */
/*    KHE_RESOURCE_MATCHING_SOLVER rms, bool preserve_existing)              */
/*                                                                           */
/*  Make and return a new demand set for rms, initially with no times.       */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_MATCHING_DEMAND_SET KheResourceMatchingDemandSetMake(
  KHE_RESOURCE_MATCHING_SOLVER rms /* , bool preserve_existing */)
{
  KHE_RESOURCE_MATCHING_DEMAND_SET res;  KHE_RESOURCE_TYPE rt;
  if( HaArrayCount(rms->free_demand_sets) > 0 )
  {
    res = HaArrayLastAndDelete(rms->free_demand_sets);
    KheTimeSetClear(res->time_set);
    KheSimpleGrouperClear(res->simple_grouper);
    KheSolnAdjusterClear(res->soln_adjuster);
    /* KheTaskResourceGrouperClear(res->task_grouper); */
    HaArrayClear(res->assignable_tasks);
    HaArrayClear(res->demand_nodes);
    HaArrayClear(res->monitors);
  }
  else
  {
    HaMake(res, rms->arena);
    rt = KheResourceGroupResourceType(rms->resource_group);
    res->time_set = KheTimeSetMake(KheResourceTypeInstance(rt), rms->arena);
    res->simple_grouper = KheSimpleGrouperMake(rms->soln,
      rms->common_frame, rms->domain_finder, rms->arena);
    res->soln_adjuster = KheSolnAdjusterMake(rms->soln, rms->arena);
    /* res->task_grouper = KheTaskResourceGrouperMake(rt, rms->arena); */
    HaArrayInit(res->assignable_tasks, rms->arena);
    HaArrayInit(res->demand_nodes, rms->arena);
    HaArrayInit(res->monitors, rms->arena);
  }
  res->solver = rms;
  /* res->preserve_existing = preserve_existing; */
  res->ready_for_solving = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetAddTime(                                */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME t)                     */
/*                                                                           */
/*  Add time t to rmds.                                                      */
/*                                                                           */
/*****************************************************************************/

void KheResourceMatchingDemandSetAddTime(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME t)
{
  if( rmds->ready_for_solving )
    KheResourceMatchingDemandSetClear(rmds);
  KheTimeSetAddTime(rmds->time_set, t);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetAddTimeGroup(                           */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME_GROUP tg)              */
/*                                                                           */
/*  Add the times of tg to rmds.                                             */
/*                                                                           */
/*****************************************************************************/

void KheResourceMatchingDemandSetAddTimeGroup(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_TIME_GROUP tg)
{
  if( rmds->ready_for_solving )
    KheResourceMatchingDemandSetClear(rmds);
  KheTimeSetAddTimeGroup(rmds->time_set, tg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetAddFrame(                               */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_FRAME frame)                */
/*                                                                           */
/*  Add the times of the time groups of frame to rmds.                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceMatchingDemandSetAddFrame(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_FRAME frame)
{
  int i;  KHE_TIME_GROUP tg;
  if( rmds->ready_for_solving )
    KheResourceMatchingDemandSetClear(rmds);
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    KheResourceMatchingDemandSetAddTimeGroup(rmds, tg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetDelete(                                 */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Delete rmds.                                                             */
/*                                                                           */
/*****************************************************************************/

void KheResourceMatchingDemandSetDelete(KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KheResourceMatchingDemandSetClear(rmds);
  HaArrayAddLast(rmds->solver->free_demand_sets, rmds);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetClear(                                  */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Clear rmds back to its state when it was first made.                     */
/*                                                                           */
/*****************************************************************************/

void KheResourceMatchingDemandSetClear(KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  int i;  KHE_DEMAND_NODE dn;
  KheTimeSetClear(rmds->time_set);
  rmds->ready_for_solving = false;
  KheSimpleGrouperClear(rmds->simple_grouper);
  KheSolnAdjusterClear(rmds->soln_adjuster);
  /* KheTaskResourceGrouperClear(rmds->task_grouper); */
  HaArrayClear(rmds->assignable_tasks);
  HaArrayForEach(rmds->demand_nodes, dn, i)
    KheDemandNodeDelete(dn);
  HaArrayClear(rmds->demand_nodes);
  HaArrayClear(rmds->monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preparation: find and group assignable tasks"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDebugTask1(KHE_TASK task, bool preserve_existing)                */
/*                                                                           */
/*  Debug task, showing the stuff relevant to whether it gets matched.       */
/*                                                                           */
/*****************************************************************************/

static void KheDebugTask1(KHE_TASK task /* , bool preserve_existing */)
{
  KHE_RESOURCE r;
  fprintf(stderr, "    examining task ");
  KheTaskDebug(task, 1, 0, stderr);
  task = KheTaskProperRoot(task);
  fprintf(stderr, "      proper root is ");
  KheTaskDebug(task, 1, -1, stderr);
  if( KheTaskIsPreassigned(task, &r) )
    fprintf(stderr, ", preassigned %s", KheResourceId(r));
  if( KheTaskAssignIsFixed(task) )
    fprintf(stderr, ", fixed");
  if( KheTaskAsst(task) != NULL )
  {
    fprintf(stderr, ", assigned to ");
    KheTaskDebug(KheTaskAsst(task), 1, -1, stderr);
  }
  fprintf(stderr, "\n");
  /* ***
  fprintf(stderr, "      condition1: %s && %s && %s\n",
    bool_show(!KheTaskIsPreassigned(task, &r)),
    bool_show(!KheTaskAssignIsFixed(task)),
    bool_show(!preserve_existing || KheTaskAsst(task) == NULL));
  *** */
  fprintf(stderr, "      condition1: %s && %s\n",
    bool_show(!KheTaskIsPreassigned(task, &r)),
    bool_show(!KheTaskAssignIsFixed(task)));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Comparison function that sorts tasks into an arbitrary fixed order.      */
/*                                                                           */
/*****************************************************************************/

static int KheTaskCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFindAndGroupAssignableTasks(                                     */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, bool nocost_off)                */
/*                                                                           */
/*  The first step in preparing demand nodes:  find and group the            */
/*  assignable nodes.                                                        */
/*                                                                           */
/*  If nocost_off is true, do not limit to tasks for which non-assignment    */
/*  has a cost.                                                              */
/*                                                                           */
/*****************************************************************************/

static void KheFindAndGroupAssignableTasks(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds /* , bool nocost_off */)
{
  KHE_TASK task, leader_task;  KHE_RESOURCE_MATCHING_SOLVER rms;
  KHE_DEMAND_NODE dn; KHE_EVENT_TIMETABLE_MONITOR etm;  int i, j, k;
  KHE_RESOURCE r;  KHE_TIME t;  KHE_MEET meet;  KHE_RESOURCE_TYPE rt;
  bool rg_is_all;

  /* boilerplate */
  if( DEBUG1 )
    fprintf(stderr, "[ KheFindAndGroupAssignableTasks(rmds)\n");
  rms = rmds->solver;
  etm = (KHE_EVENT_TIMETABLE_MONITOR) KheOptionsGetObject(rms->options,
    "gs_event_timetable_monitor", NULL);
  rt = KheResourceGroupResourceType(rms->resource_group);
  rg_is_all = (KheResourceTypeResourceCount(rt) ==
    KheResourceGroupResourceCount(rms->resource_group));

  /* find all the tasks that could possibly be assignable  */
  HaArrayClear(rmds->assignable_tasks);
  for( i = 0;  i < KheTimeSetTimeCount(rmds->time_set);  i++ )
  {
    t = KheTimeSetTime(rmds->time_set, i);
    if( DEBUG1 )
      fprintf(stderr, "  examining time %s\n", KheTimeId(t));
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(etm, t, j);  /* 3 */
      if( DEBUG1 )
      {
	fprintf(stderr, "  examining meet ");
	KheMeetDebug(meet, 1, -1, stderr);
	fprintf(stderr, " at time %s\n", KheTimeId(t));
      }
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	if( KheTaskResourceType(task) == rt )  /* 1 */
	{
	  task = KheTaskProperRoot(task);  /* 2 */
	  if( DEBUG1 )
	    KheDebugTask1(task /* , rmds->preserve_existing */);
	  if( !KheTaskIsPreassigned(task, &r)  /* 4 */
	      && !KheTaskAssignIsFixed(task) ) /* 5 */
	      /* && (!rmds->preserve_existing || KheTaskAsst(task)==NULL) ) 3 */
	  {
	    r = KheTaskAsstResource(task);
	    if( r == NULL || rg_is_all ||
		KheResourceGroupContains(rms->resource_group, r) )
	    {
	      if( DEBUG1 )
	      {
		fprintf(stderr, "      adding assignable task ");
		KheTaskDebug(task, 2, 0, stderr);
	      }
	      HaArrayAddLast(rmds->assignable_tasks, task);
	    }
	  }
	}
      }
    }
  }

  /* sort and uniqueify the possibly assignable tasks */
  HaArraySortUnique(rmds->assignable_tasks, &KheTaskCmp);

  /* group tasks assigned the same resource */
  KheSimpleGrouperClear(rmds->simple_grouper);
  KheSolnAdjusterClear(rmds->soln_adjuster);
  HaArrayForEach(rmds->assignable_tasks, task, i)
    KheSimpleGrouperAddTask(rmds->simple_grouper, task);
  KheSimpleGrouperMakeGroups(rmds->simple_grouper,
    KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE, rmds->soln_adjuster);

  /* make one demand node for each leader task in the simple grouper */
  for( i = 0; i < KheSimpleGrouperGroupCount(rmds->simple_grouper); i++ )
  {
    leader_task = KheSimpleGrouperGroup(rmds->simple_grouper, i);
    if( DEBUG1 )
    {
      fprintf(stderr, "      adding leader task (assigned %s) ",
	KheTaskAsstResource(leader_task) == NULL ? "-" :
	KheResourceId(KheTaskAsstResource(leader_task)));
      KheTaskDebug(leader_task, 2, 0, stderr);
    }
    HnAssert(KheTaskProperRoot(leader_task) == leader_task,
      "KheFindAndGroupAssignableTasks internal error 1");
    dn = KheDemandNodeMake(rmds);
    KheDemandNodeAddTask(dn, leader_task);
  }

  /* *** old code ***
  ** make one demand node for each truly assignable task **
  ** NB task grouping must come after uniqueifying **
  HaArrayForEach(rmds->assignable_tasks, task, i)
  {
    ** ***
    if( DEBUG1 )
      fprintf(stderr, "      condition3: %s\n",
	bool_show(KheTaskNonAssignmentHasCost(task, false)));
    *** **
    ** ***
    if( nocost_off || KheTaskNonAssignmentHasCost(task, false) )  ** 7 **
    *** **
    ** if( KheTaskAsstNeededToSatisfyMonitors(task, rmds) ) **
    {
      r = KheTaskAsstResource(task);
      if( r == NULL || KheTaskResourceGrouperAddTask(rmds->task_grouper, task) )
      {
	if( DEBUG1 )
	{
	  fprintf(stderr, "      adding task ");
	  KheTaskDebug(task, 2, 0, stderr);
	}
	dn = KheDemandNodeMake(rmds);
	HnAssert(KheTaskProperRoot(task) == task,
	  "KheFindAndGroupAssignableTasks internal error 1");
        KheDemandNodeAddTask(dn, task);
        ** HaArrayAddLast(dn->tasks, task); **
        ** KheTaskAddToLimitMonitors(task, rmds); **
      }
    }
  }
  KheTaskResourceGrouperGroup(rmds->task_grouper);
  *** */

  if( DEBUG1 )
  {
    fprintf(stderr, "  KheFindAndGroupAssignableTasks final demand nodes:\n");
    fprintf(stderr, "  %d demand nodes initially:\n",
      HaArrayCount(rmds->demand_nodes));
    HaArrayForEach(rmds->demand_nodes, dn, i)
      KheDemandNodeDebug(dn, 2, 4, stderr);
    fprintf(stderr, "] KheFindAndGroupAssignableTasks returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preparation: find task profiles and merge equivalent nodes"   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandNodeContainsDuplicate(KHE_DEMAND_NODE dn)                  */
/*                                                                           */
/*  Return true if dn contains a duplicate task.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandNodeContainsDuplicate(KHE_DEMAND_NODE dn)
{
  int i, j;  KHE_TASK taski, taskj;
  HaArrayForEach(dn->tasks, taski, i)
    for( j = i + 1;  j < HaArrayCount(dn->tasks);  j++ )
    {
      taskj = HaArray(dn->tasks, j);
      if( taski == taskj )
	return true;
    }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFindTaskProfilesAndMergeEquivalentNodes(                         */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  The second step in preparing demand nodes:  find task profiles and       */
/*  merge nodes containing equivalent tasks.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheFindTaskProfilesAndMergeEquivalentNodes(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_DEMAND_NODE dn, dni, dnj;  int i, j, k;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheFindTaskProfilesAndMergeEquivalentNodes(rmds)\n");
    fprintf(stderr, "  %d demand nodes initially:\n",
      HaArrayCount(rmds->demand_nodes));
    HaArrayForEach(rmds->demand_nodes, dn, i)
      KheDemandNodeDebug(dn, 2, 4, stderr);
  }

  /* set up the profiles in the demand nodes */
  /* NB profile setup must come after task grouping */
  HaArrayForEach(rmds->demand_nodes, dn, i)
    dn->profile = KheTaskProfileSetup(HaArrayFirst(dn->tasks), rmds);

  /* sort the demand nodes by profile and merge equivalent ones */
  if( HaArrayCount(rmds->demand_nodes) >= 1 )
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  %d demand nodes just before sorting and merging:\n",
	HaArrayCount(rmds->demand_nodes));
      HaArrayForEach(rmds->demand_nodes, dn, i)
	KheDemandNodeDebug(dn, 2, 4, stderr);
    }
    HaArraySort(rmds->demand_nodes, &KheDemandNodeTaskProfileCmp);
    i = 0;
    dni = HaArray(rmds->demand_nodes, i);
    for( j = 1;  j < HaArrayCount(rmds->demand_nodes);  j++ )
    {
      /* either merge dnj into dni or make dnj be the new dni */
      dnj = HaArray(rmds->demand_nodes, j);
      if( KheDemandNodeTaskProfileTypedCmp(dni, dnj) == 0 )
      {
	HaArrayAppend(dni->tasks, dnj->tasks, k);
        HnAssert(!KheDemandNodeContainsDuplicate(dni),
	  "KheFindTaskProfilesAndMergeEquivalentNodes internal error 1");
	KheDemandNodeDelete(dnj);
      }
      else
      {
	i++;
	HaArrayPut(rmds->demand_nodes, i, dnj);
	dni = dnj;
      }
    }
    HaArrayDeleteLastSlice(rmds->demand_nodes,
      HaArrayCount(rmds->demand_nodes) - i - 1);
    if( DEBUG2 )
    {
      fprintf(stderr, "  %d demand nodes after sorting and merging:\n",
	HaArrayCount(rmds->demand_nodes));
      HaArrayForEach(rmds->demand_nodes, dn, i)
	KheDemandNodeDebug(dn, 2, 4, stderr);
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheFindTaskProfilesAndMergeEquivalentNodes returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "preparation: add preferences for limit resources monitors"    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskSetBackPointers(KHE_TASK task, void *val)                    */
/*                                                                           */
/*  Set to val the back pointers in task and in all tasks assigned directly  */
/*  or indirectly to task.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTaskSetBackPointers(KHE_TASK task, void *val)
{
  int i;  KHE_TASK child_task;
  KheTaskSetBack(task, val);
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskSetBackPointers(child_task, val);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorIncreasingSolnIndexCmp(const void *t1, const void *t2)     */
/*                                                                           */
/*  Comparison function for sorting an array of monitors by increasing       */
/*  solution index.                                                          */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorIncreasingSolnIndexCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDemandNodeSplit(KHE_DEMAND_NODE dn, int *target_durn,            */
/*    KHE_RESOURCE_GROUP rg, bool include_r0, KHE_COST weight)               */
/*                                                                           */
/*  Split dn as required into nodes containing tasks whose monitored         */
/*  duration is non-zero and totals at most *target_durn, which is then      */
/*  updated to the reduced value.  Each node has tasks of equal monitored    */
/*  duration, and its profile gains a preference based on that monitored     */
/*  duration, rg, include_r0, and weight.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheDemandNodeSplit(KHE_DEMAND_NODE dn, int *target_durn,
  KHE_RESOURCE_GROUP rg, bool include_r0, KHE_COST weight)
{
  KHE_TASK task;  KHE_DEMAND_NODE new_dn;  KHE_RESOURCE_MATCHING_SOLVER rms;
  int i, j, included_md, md, included_tasks;  bool more_use_of_node_possible;
  if( DEBUG8 )
  {
    fprintf(stderr, "[ KheDemandNodeSplit(dn, &%d, rg, %s, %.5f)\n",
      *target_durn, bool_show(include_r0), KheCostShow(weight));
    KheDemandNodeDebug(dn, 4, 2, stderr);
  }

  rms = dn->demand_set->solver;
  do
  {
    /* find the number of included tasks, and their shared monitored duration */
    included_tasks = 0;
    included_md = -1;	/* undefined until the first included task is found */
    HaArrayForEach(dn->tasks, task, i)
    {
      md = KheTaskMonitoredDurn(task);
      if( md == 0 )
      {
	/* exclude task (zero monitored duration) */
      }
      else if( included_tasks == 0 )
      {
	if( md <= *target_durn )
	{
	  /* include task; it's the first, so set included_md */
	  included_tasks++;
	  included_md = md;
	}
	else
	{
	  /* exclude task (monitored duration too large) */
	}
      }
      else if( md != included_md )
      {
	/* exclude task (different monitored durn) */
      }
      else if( md + included_tasks * included_md > *target_durn )
      {
	/* exclude task and beyond (would exceed *target_durn) */
	break;
      }
      else
      {
	/* include task */
	included_tasks++;
      }
    }

    /* either use all of the node, or use none of it, or split it */
    if( included_tasks == HaArrayCount(dn->tasks) )
    {
      /* use all of the node; add preference to dn and stop */
      if( DEBUG8 )
	fprintf(stderr, "  all tasks included\n");
      KheTaskProfileAddPreference(dn->profile, rg, include_r0,
	KHE_LIMIT_RESOURCES_MONITOR_TAG, weight * included_md, rms);
      *target_durn -= included_tasks * included_md;
      more_use_of_node_possible = false;
    }
    else if( included_tasks == 0 )
    {
      /* use none of the node; just stop */
      if( DEBUG8 )
	fprintf(stderr, "  no tasks included\n");
      more_use_of_node_possible = false;
    }
    else
    {
      /* split the node; the new node contains those tasks with monitored */
      /* durn included_md whose index is less than i, and a new preference */
      if( DEBUG8 )
	fprintf(stderr, "  new node required\n");
      new_dn = KheDemandNodeMake(dn->demand_set);
      new_dn->profile = KheTaskProfileCopy(dn->profile, rms);
      for( j = 0;  j < i;  j++ )
      {
	task = HaArray(dn->tasks, j);
	if( KheTaskMonitoredDurn(task) == included_md )
	{
	  KheDemandNodeAddTask(new_dn, task);
	  /* HaArrayAddLast(new_dn->tasks, task); */
	  HaArrayDeleteAndShift(dn->tasks, j);
	  j--, i--;
	}
      }
      KheTaskProfileAddPreference(new_dn->profile, rg, include_r0,
	KHE_LIMIT_RESOURCES_MONITOR_TAG, weight * included_md, rms);
      *target_durn -= included_tasks * included_md;
      more_use_of_node_possible = (*target_durn > 0);
    }
  } while( more_use_of_node_possible );
  if( DEBUG8 )
    fprintf(stderr, "] KheDemandNodeSplit returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddLimitResourcesMonitorPreferences(                             */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_LIMIT_RESOURCES_MONITOR lrm)*/
/*                                                                           */
/*  Add preferences to the demand nodes of rmds for lrm.                     */
/*                                                                           */
/*  Implementation note.  Adding a preference may require a node to be       */
/*  split.  This raises the question of where the new node goes in the       */
/*  sequence, and whether that position will confuse later operations.       */
/*                                                                           */
/*  After handling of lrm is complete, the nodes will be re-sorted for       */
/*  the next monitor, so only the current function could be confused.        */
/*  We place new nodes at the obvious point (the end), but we handle U       */
/*  first.  The new nodes all have preferences for U, so they are of no      */
/*  interest to L, so having them at the end is fine.                        */
/*                                                                           */
/*****************************************************************************/

static void KheAddLimitResourcesMonitorPreferences(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  int N, L, U, i, j, egi, durn;  KHE_DEMAND_NODE dn;
  KHE_EVENT_RESOURCE er; KHE_LIMIT_RESOURCES_CONSTRAINT c; KHE_SOLN soln;
  KHE_TASK task, root_task;  KHE_RESOURCE r;  KHE_COST weight;
  KHE_RESOURCE_GROUP domain, neg_domain;  KHE_RESOURCE_MATCHING_SOLVER rms;

  /* initialize N, L, U, and the monitored durations */
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheAddLimitResourcesMonitorPreferences(");
    KheTimeSetDebug(rmds->time_set, 2, -1, stderr);
    fprintf(stderr, ", lrm), lrm =\n");
    KheMonitorDebug((KHE_MONITOR) lrm, 2, 2, stderr);
  }
  rms = rmds->solver;
  soln = rms->soln;
  c = KheLimitResourcesMonitorConstraint(lrm);
  weight = KheMonitorCombinedWeight((KHE_MONITOR) lrm);
  domain = KheLimitResourcesConstraintDomain(c);
  neg_domain = KheLimitResourcesConstraintDomainComplement(c);
  N = 0;
  L = KheLimitResourcesConstraintMinimum(c);
  U = KheLimitResourcesConstraintMaximum(c);
  HaArrayForEach(rmds->demand_nodes, dn, i)
    HaArrayForEach(dn->tasks, task, j)
      KheTaskClearMonitoredDurn(task);

  /* traverse the tasks monitored by lrm */
  if( DEBUG5 )
    fprintf(stderr, "  traversing tasks monitored by lrm\n");
  egi = KheLimitResourcesMonitorEventGroupIndex(lrm);
  for( i = 0;  i < KheLimitResourcesConstraintEventResourceCount(c, egi);  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheEventResourceTask(soln, er, j);
      durn = KheTaskDuration(task);
      root_task = (KHE_TASK) KheTaskBack(task);
      if( root_task != NULL )
      {
	N += durn;
        KheTaskAddMonitoredDurn(root_task, durn);
      }
      else
      {
	r = KheTaskAsstResource(task);
	if( r == NULL )
	  L -= durn;
	else if( KheResourceGroupContains(domain, r) )
          L -= durn, U -= durn;
      }
    }
  }

  /* tidy up L and U */
  if( L < 0 )  L = 0;
  if( L > N )  L = N;
  if( U < 0 )  U = 0;
  if( U > N )  U = N;

  /* find demand node incompatibilities; sort by increasing incompatibility */
  HaArrayForEach(rmds->demand_nodes, dn, i)
    dn->incompatibility = KheTaskProfileIncompatibility(dn->profile,
      domain, rms);
  HaArraySort(rmds->demand_nodes, &KheDemandNodeIncreasingIncompatibilityCmp);
  if( DEBUG5 )
  {
    fprintf(stderr, "  demand node incompatibilities (after sorting):\n");
    HaArrayForEach(rmds->demand_nodes, dn, i)
    {
      fprintf(stderr, "    incompatibility %.1f:\n", dn->incompatibility);
      KheDemandNodeDebug(dn, 2, 4, stderr);
    }
  }

  /* find preferences for U; see implementation note for why U first */
  if( DEBUG5 )
    fprintf(stderr, "  finding preferences for U (%d)\n", U);
  durn = N - U;
  HaArrayForEachReverse(rmds->demand_nodes, dn, i)
  {
    if( durn <= 0 )
      break;
    KheDemandNodeSplit(dn, &durn, neg_domain, true, weight);
  }

  /* find preferences for L */
  if( DEBUG5 )
    fprintf(stderr, "  finding preferences for L (%d)\n", L);
  durn = L;
  HaArrayForEach(rmds->demand_nodes, dn, i)
  {
    if( durn <= 0 )
      break;
    KheDemandNodeSplit(dn, &durn, domain, false, weight);
  }

  /* clear the monitored durations */
  if( DEBUG5 )
    fprintf(stderr, "  clearing monitored durations\n");
  HaArrayForEach(rmds->demand_nodes, dn, i)
    HaArrayForEach(dn->tasks, task, j)
      KheTaskClearMonitoredDurn(task);
  if( DEBUG5 )
  {
    fprintf(stderr, "  final demand nodes:\n");
    HaArrayForEach(rmds->demand_nodes, dn, i)
      KheDemandNodeDebug(dn, 2, 2, stderr);
    fprintf(stderr, "] KheAddLimitResourcesMonitorPreferences returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddPreferencesForLimitResourcesMonitors(                         */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  The third step in preparing demand nodes:  add preferences for limit     */
/*  resources monitors.                                                      */
/*                                                                           */
/*****************************************************************************/

static void KheAddPreferencesForLimitResourcesMonitors(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_TASK task;  KHE_DEMAND_NODE dn;  int i, j;
  KHE_LIMIT_RESOURCES_MONITOR lrm;

  if( HaArrayCount(rmds->monitors) > 0 )
  {
    /* set the back pointer of each affected task to its assignable task */
    HaArrayForEach(rmds->demand_nodes, dn, i)
      HaArrayForEach(dn->tasks, task, j)
	KheTaskSetBackPointers(task, (void *) task);

    /* uniqueify the limit resources monitors and add preferences for them */
    HaArraySortUnique(rmds->monitors, &KheMonitorIncreasingSolnIndexCmp);
    if( DEBUG3 )
      fprintf(stderr, "  %d limit resources monitors after uniqueifying\n",
	HaArrayCount(rmds->monitors));
    HaArrayForEach(rmds->monitors, lrm, i)
    {
      if( DEBUG3 )
      {
	fprintf(stderr, "  [ pref ");
	KheMonitorDebug((KHE_MONITOR) lrm, 2, 0, stderr);
      }
      KheAddLimitResourcesMonitorPreferences(rmds, lrm);
      if( DEBUG3 )
	fprintf(stderr, "  ]\n");
    }

    /* set the back pointer of each affected task to NULL */
    HaArrayForEach(rmds->demand_nodes, dn, i)
      HaArrayForEach(dn->tasks, task, j)
	KheTaskSetBackPointers(task, NULL);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDeleteOptionalNodes(KHE_RESOURCE_MATCHING_DEMAND_SET rmds)       */
/*                                                                           */
/*  Delete nodes whose tasks might as well remain unassigned.                */
/*                                                                           */
/*****************************************************************************/

static void KheDeleteOptionalNodes(KHE_RESOURCE_MATCHING_DEMAND_SET rmds)
{
  KHE_DEMAND_NODE dn;  int i;
  HaArrayForEach(rmds->demand_nodes, dn, i)
    if( KheTaskProfileShowsNodeIsOptional(dn->profile) )
    {
      /* remove dn from rmds->demand_nodes and delete it */
      HaArrayDeleteAndPlug(rmds->demand_nodes, i);
      i--;
      KheDemandNodeDelete(dn);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingDemandSetPrepareForSolving(                      */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds)                                 */
/*                                                                           */
/*  Make rmds ready for solving.  This is done only once, the first time     */
/*  rmds is solved, unless rmds is cleared or more times are added.          */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMatchingDemandSetPrepareForSolving(
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds /* , bool nocost_off */)
{
  KHE_DEMAND_NODE dn;  int i;
  if( DEBUG1 || DEBUG2 || DEBUG3 )
  {
    fprintf(stderr, "[ KheResourceMatchingDemandSetPrepareForSolving(");
    KheTimeSetDebug(rmds->time_set, 2, -1, stderr);
    fprintf(stderr, "):\n");
  }
  HnAssert(KheTimeSetTimeCount(rmds->time_set) > 0,
    "KheResourceMatchingSolverSolve: no times");
  KheFindAndGroupAssignableTasks(rmds /* , nocost_off */);
  KheFindTaskProfilesAndMergeEquivalentNodes(rmds);
  KheAddPreferencesForLimitResourcesMonitors(rmds);
  KheDeleteOptionalNodes(rmds);
  HaArrayForEach(rmds->demand_nodes, dn, i)
    KheTaskProfileReduceToLimitResourcesOnly(dn->profile, rmds->solver);
  /* KheTaskResourceGrouperUnGroup(rmds->task_grouper); */
  KheSolnAdjusterUndo(rmds->soln_adjuster);
  if( DEBUG1 || DEBUG2 || DEBUG3 )
  {
    fprintf(stderr, "  final demand nodes ready for solving:\n");
    HaArrayForEach(rmds->demand_nodes, dn, i)
      KheDemandNodeDebug(dn, 2, 2, stderr);
    fprintf(stderr,
      "] KheResourceMatchingDemandSetPrepareForSolving (%d demands)\n",
      HaArrayCount(rmds->demand_nodes));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "solver construction and solving"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingSolverBackDebug(void *value, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug function for printing the back value of a time sweep mmatch.       */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMatchingSolverBackDebug(void *value, int verbosity,
  int indent, FILE *fp)
{
  KHE_RESOURCE_MATCHING_SOLVER rms;  KHE_RESOURCE_MATCHING_DEMAND_SET rmds;
  rms = (KHE_RESOURCE_MATCHING_SOLVER) value;
  rmds = HaArrayFirst(rms->demand_sets);
  KheTimeSetDebug(rmds->time_set, 2, -1, fp);
  if( HaArrayCount(rms->demand_sets) > 1 )
  {
    fprintf(fp, " with lookahead to ");
    rmds = HaArrayLast(rms->demand_sets);
    KheTimeSetDebug(rmds->time_set, 2, -1, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingCostDebug(void *back, int64_t cost, FILE *fp)    */
/*                                                                           */
/*  Debug function for displaying cost.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMatchingCostDebug(int c1, int c2, int c3, FILE *fp)
{
  if( c2 > 99999 )
    fprintf(fp, "%d,%d", c1, c2);
  else
    fprintf(fp, "%.5f", KheCostShow(KheCost(c1, c2)));
  if( c3 != 0 )
    fprintf(fp, "|%d", c3);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingEdgeDebug(KHE_DEMAND_NODE dn,                    */
/*    KHE_SUPPLY_NODE sn, int c1, int c2, int c3, int indent, FILE *fp)      */
/*                                                                           */
/*  Debug print of an edge from dn to sn with cost (c1, c2, c3).             */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMatchingEdgeDebug(KHE_DEMAND_NODE dn,
  KHE_SUPPLY_NODE sn, int mult, int c1, int c2, int c3, int indent, FILE *fp)
{
  KHE_RESOURCE_MATCHING_SOLVER rms;  KHE_TASK task;  int i;
  rms = dn->demand_set->solver;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  KheDemandNodeDebug(dn, 1, -1, fp);
  if( HaArrayCount(rms->demand_sets) > 1 )
  {
    fprintf(fp, " (");
    HaArrayForEach(rms->best_tasks, task, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      if( task == NULL )
	fprintf(fp, "_");
      else
	KheTaskDebug(task, 2, -1, fp);
    }
    fprintf(fp, ")");
  }
  if( mult != INT_MAX )
    fprintf(fp, " %d--", mult);
  else
    fprintf(fp, " --");
  KheResourceMatchingCostDebug(c1, c2, c3, fp);
  fprintf(fp, "-> ");
  KheSupplyNodeDebug(sn, 1, -1, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheExtra(KHE_TASK task, KHE_RESOURCE r)                              */
/*                                                                           */
/*  A bit extra, based on the diversifier and the task and resource          */
/*  indexes, just to shake things up a bit.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** this just seems to make things slightly worse
static int KheExtra(KHE_TASK task, KHE_RESOURCE r, int range)
{
int diversifier;
diversifier = KheSolnDiversifier(KheTaskSoln(task));
return (diversifier * 5 + KheTaskSolnIndex(task) * 7 +
  KheResourceInstanceIndex(r) * 13) % range;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceAtMaxCount(KHE_SOLN soln, KHE_RESOURCE r)                 */
/*                                                                           */
/*  Return the total at max count of r in soln, but only if there are        */
/*  not too many monitors to hunt through.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheResourceAtMaxCount(KHE_SOLN soln, KHE_RESOURCE r)
{
int i, res, count;  KHE_MONITOR m;
res = 0;
count = KheSolnResourceMonitorCount(soln, r);
if( count < 300 )
  for( i = 0;  i < count;  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
      res += KheClusterBusyTimesMonitorAtMaxLimitCount(
	(KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
    else if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
      res += KheLimitActiveIntervalsMonitorAtMaxLimitCount(
	(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
  }
return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceAtMaxCount(KHE_RESOURCE r, KHE_SOLN soln,                 */
/*    KHE_DEMAND_NODE dn)                                                    */
/*                                                                           */
/*  Return the at_max value of r at any time covered by dn, or 0 if none.    */
/*                                                                           */
/*****************************************************************************/

static int KheResourceAtMaxCount(KHE_RESOURCE r, KHE_SOLN soln,
  KHE_DEMAND_NODE dn)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_SET ts;  KHE_TIME t;
  ts = dn->profile->time_set;
  if( KheTimeSetTimeCount(ts) > 0 )
  {
    t = KheTimeSetTime(ts, 0);
    rtm = KheResourceTimetableMonitor(soln, r);
    return KheResourceTimetableMonitorAtMaxLimitCount(rtm, t);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDoCombinatorialAsst(KHE_RESOURCE_MATCHING_SOLVER rms,            */
/*    KHE_RESOURCE r, int curr_index)                                        */
/*                                                                           */
/*  Do combinatorial assignment from curr_index onwards.                     */
/*                                                                           */
/*****************************************************************************/

static void KheDoCombinatorialAsst(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE r, int curr_index)
{
  KHE_RESOURCE_MATCHING_DEMAND_SET demand_set;  KHE_COST cost;
  KHE_DEMAND_NODE dn;  int i;  KHE_TASK task;
  if( curr_index >= HaArrayCount(rms->demand_sets) )
  {
    /* one combination completed, save if best */
    cost = KheSolnCost(rms->soln);
    if( cost < rms->best_cost )
    {
      rms->best_cost = cost;
      HaArrayClear(rms->best_tasks);
      HaArrayAppend(rms->best_tasks, rms->tasks, i);
    }
  }
  else
  {
    /* start with non-assignment */
    HaArrayAddLast(rms->tasks, NULL);
    KheDoCombinatorialAsst(rms, r, curr_index + 1);
    HaArrayDeleteLast(rms->tasks);

    /* carry on with all distinct assignments */
    demand_set = HaArray(rms->demand_sets, curr_index);
    HaArrayForEach(demand_set->demand_nodes, dn, i)
    {
      task = HaArrayFirst(dn->tasks);
      if( KheTaskAssignResource(task, r) )
      {
	HaArrayAddLast(rms->tasks, task);
        KheDoCombinatorialAsst(rms, r, curr_index + 1);
	HaArrayDeleteLast(rms->tasks);
	KheTaskUnAssignResource(task);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheCostWithLookahead(KHE_RESOURCE_MATCHING_SOLVER rms,          */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the minimum possible solution cost after assigning r to           */
/*  one task (or nothing) from each of the lookahead task sets.              */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheCostWithLookahead(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE r)
{
  rms->best_cost = KheCost(INT_MAX, INT_MAX);
  HaArrayClear(rms->tasks);
  HaArrayClear(rms->best_tasks);
  KheDoCombinatorialAsst(rms, r, 1);
  return rms->best_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEdge(KHE_RESOURCE_MATCHING_SOLVER rms, KHE_DEMAND_NODE dn,       */
/*    KHE_SUPPLY_NODE sn, int *c1, int *c2, int *c3)                         */
/*                                                                           */
/*  If there needs to be an edge between demand node dn and supply node sn,  */
/*  return true with (*c1, *c2, *c3) set to its cost, else return false.     */
/*                                                                           */
/*****************************************************************************/
#define max(a, b) ((a) > (b) ? (a) : (b))

static bool KheEdge(KHE_RESOURCE_MATCHING_SOLVER rms, KHE_DEMAND_NODE dn,
KHE_SUPPLY_NODE sn, int *c1, int *c2, int *c3)
{
  KHE_COST cost;  KHE_PREFERENCE pref;
  int i, part_count, busy_times, at_max_before, at_max_after;
  HaArrayClear(rms->best_tasks);
  if( sn->resource == NULL )
  {
    /* leave dn unassigned, whatever it is */
    cost = KheSolnCost(rms->soln);
    *c1 = KheHardCost(cost);
    *c2 = KheSoftCost(cost);
    *c3 = 0;
    HaArrayForEach(dn->profile->preferences, pref, i)
      if( !pref->include_r0 )
	*c1 += KheHardCost(pref->cost), *c2 += KheSoftCost(pref->cost);
    return true;
  }
  else
  {
    if( !rms->edge_adjust2_off )
      at_max_before = KheResourceAtMaxCount(sn->resource, rms->soln, dn);
    else
      at_max_before = 0;  /* keep compiler happy (it's really undefined) */
    if( KheDemandNodeAssignOneTask(dn, HaArrayFirst(dn->tasks), sn->resource) )
    {
      /* ***
      at_max_before = rms->edge_adjust1_off ? 0 :
	KheResourceAtMaxCount(rms->soln, sn->resource);
      *** */

      /* we've assigned one or all of the tasks of dn to sn->resource */
      /* cost = KheSolnCost(rms->soln); */
      cost = KheCostWithLookahead(rms, sn->resource);
      *c1 = KheHardCost(cost);
      *c2 = KheSoftCost(cost);
      *c3 = 0;
      if( !rms->edge_adjust1_off )
      {
	part_count = KheFrameTimeGroupCount(rms->common_frame);
	busy_times = sn->curr_busy + dn->profile->duration;
	*c3 += (part_count - sn->max_busy + busy_times) * 2;
      }
      if( !rms->edge_adjust2_off )
      {
	at_max_after = KheResourceAtMaxCount(sn->resource, rms->soln, dn);
	if( at_max_after > at_max_before )
	*c3 += at_max_after - at_max_before;
      }
      if( !rms->edge_adjust3_off )
      {
	*c3 += sn->curr_run_len;
      }
      if( !rms->edge_adjust4_off )
      {
	if( dn->offset_in_day != sn->curr_offset )
	  *c3 += 1;
      }
      /* ***
      else
      {
	at_max_after = KheResourceAtMaxCount(sn->resource, rms->soln, dn);
	*c3 = (part_count - sn->max_busy + busy_times) * 2 +
	  max(0, at_max_after - at_max_before) + sn->curr_run_len;
      }
      *** */
      HaArrayForEach(dn->profile->preferences, pref, i)
      {
	if( !KheResourceGroupContains(pref->resource_group, sn->resource) )
	  *c1 += KheHardCost(pref->cost), *c2 += KheSoftCost(pref->cost);
      }
      return true;
    }
    else
      return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMatchingEdgeFn(KHE_DEMAND_NODE dn,                       */
/*    KHE_SUPPLY_NODE sn, int64_t *cost)                                     */
/*                                                                           */
/*  If there should be an edge between dn and sn in the bipartite graph,     */
/*  return true and set (*c1, *c2, *c3) to its cost.  Otherwise return       */
/*  false and leave (*c1, *c2, *c3) unchanged.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceMatchingEdgeFn(KHE_DEMAND_NODE dn,
  KHE_SUPPLY_NODE sn, int *c1, int *c2, int *c3)
{
  bool res;  int mult;  KHE_MARK mark;  KHE_RESOURCE_MATCHING_SOLVER rms;

  /* boilerplate */
  HnAssert(c1 != NULL, "KheResourceMatchingEdgeFn internal error 1");
  HnAssert(sn != NULL, "KheResourceMatchingEdgeFn internal error 2");
  rms = dn->demand_set->solver;

  mark = KheMarkBegin(rms->soln);
  if( DEBUG9 )
  {
    KHE_TRACE trace;  KHE_MONITOR m;  int i;
    trace = KheTraceMake((KHE_GROUP_MONITOR) rms->soln);
    KheTraceBegin(trace);
    res = KheEdge(rms, dn, sn, c1, c2, c3);
    KheTraceEnd(trace);
    fprintf(stderr, "    edge ");
    if( res )
    {
      mult = (sn->resource != NULL ? 1 : KheDemandNodeMultiplicity(dn));
      KheResourceMatchingEdgeDebug(dn, sn, mult, *c1, *c2, *c3, -1, stderr);
      fprintf(stderr, ":\n    [\n");
      for( i = 0;  i < KheTraceMonitorCount(trace);  i++ )
      {
	m = KheTraceMonitor(trace, i);
	fprintf(stderr, "      %.5f -> %.5f ",
	  KheCostShow(KheTraceMonitorInitCost(trace, i)),
	  KheCostShow(KheMonitorCost(m)));
	KheMonitorDebug(m, 2, 0, stderr);
      }
      KheTraceDelete(trace);
      fprintf(stderr, "    ]\n");
    }
    else
    {
      KheDemandNodeDebug(dn, 1, -1, stderr);
      fprintf(stderr, " -> ");
      KheSupplyNodeDebug(sn, 1, -1, stderr);
      fprintf(stderr, " not present\n");
    }
  }
  else
    res = KheEdge(rms, dn, sn, c1, c2, c3);
  KheMarkEnd(mark, true);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_MATCHING_SOLVER KheResourceMatchingSolverMake(KHE_SOLN soln,*/
/*    KHE_RESOURCE_GROUP rg, HA_ARENA a)                                     */
/*                                                                           */
/*  Make a resource matching solver with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_MATCHING_SOLVER KheResourceMatchingSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_GROUP rg, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_RESOURCE_MATCHING_SOLVER res;  int i, count, extra, diversifier;
  KHE_RESOURCE r;

  /* fields with a well-defined value for the lifetime of the solver */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_group = rg;
  res->max_busy_set = false;
  res->mmatch = KheMMatchMake((void *) res, &KheResourceMatchingSolverBackDebug,
    &KheDemandNodeUnTypedDebug, &KheSupplyNodeUnTypedDebug,
    &KheResourceMatchingCostDebug, a);
  HaArrayInit(res->supply_nodes, a);
  HaArrayInit(res->free_preferences, a);
  HaArrayInit(res->free_task_profiles, a);
  HaArrayInit(res->free_demand_nodes, a);
  HaArrayInit(res->free_demand_sets, a);
  res->prev_last_time_index = -2;

  /* fields with a well-defined value during a single solve */
  HaArrayInit(res->demand_sets, a);
  /* res->demand_set = NULL; */
  res->options = NULL;
  res->common_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  res->domain_finder = tgdf;
  /* res->domain_finder = KheTaskGroup DomainFinderMake(soln, a); */
  res->resource_invariant = false;
  /* res->matching_off = false; */
  res->edge_adjust1_off = false;
  res->edge_adjust2_off = false;
  res->edge_adjust3_off = false;
  res->edge_adjust4_off = false;

  /* combinatorial assignment */
  HaArrayInit(res->tasks, a);
  HaArrayInit(res->best_tasks, a);
  res->best_cost = 0;

  /* initialize supply nodes (the same for all solves) */
  count = KheResourceGroupResourceCount(rg);
  diversifier = KheSolnDiversifier(soln);
  extra = 5 * diversifier;
  /* ***
  for( i = 0;  i < diversifier;  i++ )
    for( j = i;  j < count;  j += diversifier )
    {
      r = KheResourceGroupResource(rg, (j + extra) % count);
      KheSupplyNodeMake(res, r);
    }
  *** */
  if( KheSolnDiversifier(soln) % 3 == 0 )
  {
    for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
    {
      r = KheResourceGroupResource(rg, (i + extra) % count);
      KheSupplyNodeMake(res, r);
    }
  }
  else if( KheSolnDiversifier(soln) % 3 == 1 )
  {
    for( i = 0;  i < KheResourceGroupResourceCount(rg);  i += 2 )
    {
      r = KheResourceGroupResource(rg, (i + extra) % count);
      KheSupplyNodeMake(res, r);
    }
    for( i = 1;  i < KheResourceGroupResourceCount(rg);  i += 2 )
    {
      r = KheResourceGroupResource(rg, (i + extra) % count);
      KheSupplyNodeMake(res, r);
    }
  }
  else
  {
    for( i = KheResourceGroupResourceCount(rg) - 1;  i >= 0;  i-- )
    {
      r = KheResourceGroupResource(rg, (i + extra) % count);
      KheSupplyNodeMake(res, r);
    }
  }
  KheSupplyNodeMake(res, NULL);  /* denotes non-assignment of any demand node */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingSolverDelete(KHE_RESOURCE_MATCHING_SOLVER rms)   */
/*                                                                           */
/*  Delete rms.  Actually all that can be deleted here are the group         */
/*  monitors; the rest get deleted when the enclosing arena is deleted.      */
/*                                                                           */
/*****************************************************************************/

/* *** these group monitors are no longer used
void KheResourceMatchingSolverDelete(KHE_RESOURCE_MATCHING_SOLVER rms)
{
  KHE_SUPPLY_NODE sn;  int i;
  HaArrayForEach(rms->supply_nodes, sn, i)
    if( sn->group_monitor != NULL )
    {
      KheGroupMonitorDelete(sn->group_monitor);
      sn->group_monitor = NULL;
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingSolverDelete(KHE_RESOURCE_MATCHING_SOLVER rms)   */
/*                                                                           */
/*  Delete rms.                                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheResourceMatchingSolverDelete(KHE_RESOURCE_MATCHING_SOLVER rms)
{
KheSolnArenaEnd(rms->soln, rms->arena);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDemandNodeIncreasingDomainSizeCmp(const void *t1, const void *t2) */
/*                                                                           */
/*  Comparison function for sorting an array of demand nodes into increasing */
/*  order of the size of the domain of their first task.                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheDemandNodeIncreasingDomainSizeCmp(const void *t1, const void *t2)
{
  KHE_DEMAND_NODE dn1 = * (KHE_DEMAND_NODE *) t1;
  KHE_DEMAND_NODE dn2 = * (KHE_DEMAND_NODE *) t2;
  KHE_TASK task1 = HaArrayFirst(dn1->tasks);
  KHE_TASK task2 = HaArrayFirst(dn2->tasks);
  int size1 = KheResourceGroupResourceCount(KheTaskDomain(task1));
  int size2 = KheResourceGroupResourceCount(KheTaskDomain(task2));
  return size1 - size2;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMonitoredByAssignOrLimitResourceMonitors(KHE_TASK task)      */
/*                                                                           */
/*  Return true if task is monitored by at least one unsatisfied assign      */
/*  resource or limit resources monitor.                                     */
/*                                                                           */
/*****************************************************************************/

/* *** moved into the platform under the name KheTaskNonA ssignmentHasCost
 ** NB this version has at least two bugs:  testing MonitorCost instead
 ** of ConstraintWeight, and not testing descendant tasks as well as task
static bool KheTaskMonitoredByAssignOrLimitResourceMonitors(KHE_TASK task)
{
KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  int i;  KHE_MONITOR m;
KHE_MONITOR_TAG tag;

** check the task itself **
soln = KheTaskSoln(task);
er = KheTaskEventResource(task);
if( er != NULL )
  for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(soln, er, i);
    tag = KheMonitorTag(m);
    if( tag == KHE_ASSIGN_RESOURCE_MONITOR_TAG ||
	tag == KHE_LIMIT_RESOURCES_MONITOR_TAG )
    {
      if( KheMonitorCost(m) > 0 )
	return true;
    }
  }

** check the tasks assigned to task **
return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandNodeNonAssignmentHasCost(KHE_DEMAND_NODE dn)               */
/*                                                                           */
/*  Return true if not assigning the tasks of dn would have a cost.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheDemandNodeNonAssignmentHasCost(KHE_DEMAND_NODE dn)
{
  KHE_TASK task;  int i;
  HaArrayForEach(dn->tasks, task, i)
    if( KheTaskNonAss ignmentHasCost(task) )
      return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SUPPLY_NODE KheFindBestSupplyNode(KHE_RESOURCE_MATCHING_SOLVER rms,  */
/*    KHE_DEMAND_NODE dn)                                                    */
/*                                                                           */
/*  Return the best supply node to assign to dn.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_SUPPLY_NODE KheFindBestSupplyNode(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_DEMAND_NODE dn)
{
  KHE_SUPPLY_NODE sn, best_sn;  int i, c1, c2, c3, c4;
  best_sn = NULL;
  HaArrayForEach(rms->supply_nodes, sn, i)
  {
    ** assign a cost to sn **
    if( KheResourceMatchingEdgeFn(dn, sn, &c1, &c2, &c3) )
    {
      c4 = KheSolnDiversifier(rms->soln) * 5;
      if( sn->resource != NULL )
	c4 += KheResourceResourceTypeIndex(sn->resource) + 1;
      sn->c1 = c1, sn->c2 = c2, sn->c3 = c3, sn->c4 = c4;
    }
    else
      sn->c1 = sn->c2 = sn->c3 = sn->c4 = INT_MAX;

    ** compare sn with best_sn **
    if( best_sn == NULL || KheSupplyNodeLT(sn, best_sn) )
      best_sn = sn;
  }
  return best_sn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingMakeBestAsst(KHE_RESOURCE_MATCHING_SOLVER rms,   */
/*    KHE_DEMAND_NODE dn)                                                    */
/*                                                                           */
/*  Make the best assignment to dn.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceMatchingMakeBestAsst(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_DEMAND_NODE dn)
{
  int i, j;  KHE_SUPPLY_NODE sn;
  KHE_TASK task; KHE_MONITOR m;  KHE_EVENT_RESOURCE er;
  KHE_COST cost;  KHE_MARK mark;

  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheResourceMatchingMakeBestAsst(rms, ");
    KheDemandNodeDebug(dn, 1, -1, stderr);
    fprintf(stderr, "), initial cost %.5f\n",
      KheCostShow(KheSolnCost(rms->soln)));

    ** find the relevant monitors **
    HaArrayForEach(dn->tasks, task, i)
    {
      er = KheTaskEventResource(task);
      if( er != NULL )
	for( j = 0;  j < KheSolnEventResourceMonitorCount(rms->soln, er);  j++ )
	{
	  m = KheSolnEventResourceMonitor(rms->soln, er, j);
	  KheMonitorDebug(m, 2, 2, stderr);
	}
    }
  }

  ** only do anything if assignment is needed **
  if( KheDemandNodeNonAssignmentHasCost(dn) )
  {
    if( dn->grouped )
    {
      ** assign one resource to all tasks, or to none **
      sn = KheFindBestSupplyNode(rms, dn);
      if( sn->resource != NULL )
      {
	cost = KheSolnCost(rms->soln);
	mark = KheMarkBegin(rms->soln);
	KheDemandNodeAssignAllTasks(dn, sn->resource);
	KheMarkEnd(mark, KheSolnCost(rms->soln) >= cost);
      }
    }
    else
    {
      ** one assignment to make for each task; we visit the tasks in **
      ** reverse order so that the first remains free for testing with **
      HaArrayForEachReverse(dn->tasks, task, i)
      {
	sn = KheFindBestSupplyNode(rms, dn);
	if( sn->resource != NULL )
	{
	  cost = KheSolnCost(rms->soln);
	  mark = KheMarkBegin(rms->soln);
	  KheDemandNod eAssignOneTask(dn, task, sn->resource);
	  KheMarkEnd(mark, KheSolnCost(rms->soln) >= cost);
	}
      }
    }
    }
    if( DEBUG4 )
      fprintf(stderr,
	"] KheResourceMatchingMakeBestAsst returning, final cost %.5f\n",
	KheCostShow(KheSolnCost(rms->soln)));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMatchingMakeBestAsst(KHE_RESOURCE_MATCHING_SOLVER rms,   */
/*    KHE_DEMAND_NODE dn)                                                    */
/*                                                                           */
/*  Make the best assignment to dn.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceMatchingMakeBestAsstOld(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_DEMAND_NODE dn)
{
  int c1, c2, c3, c4, i, j, ti, si;  KHE_SUPPLY_NODE sn;  bool assigned;
  KHE_TASK task; KHE_MONITOR m;  KHE_EVENT_RESOURCE er;

  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheResourceMatchingMakeBestAsst(rms, ");
    KheDemandNodeDebug(dn, 1, -1, stderr);
    fprintf(stderr, "), initial cost %.5f\n",
      KheCostShow(KheSolnCost(rms->soln)));

    ** find the relevant monitors **
    HaArrayForEach(dn->tasks, task, i)
    {
      er = KheTaskEventResource(task);
      if( er != NULL )
	for( j = 0;  j < KheSolnEventResourceMonitorCount(rms->soln, er);  j++ )
	{
	  m = KheSolnEventResourceMonitor(rms->soln, er, j);
	  KheMonitorDebug(m, 2, 2, stderr);
	}
    }
  }

  ** only do anything if assignment is needed **
  if( KheDemandNodeNonAssignmentHasCost(dn) )
  {
    ** for each supply node sn, store the cost of edge (dn, sn) in sn **
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( KheResourceMatchingEdgeFn(dn, sn, &c1, &c2, &c3) )
      {
	c4 = KheSolnDiversifier(rms->soln) * 5;
	if( sn->resource != NULL )
	  c4 += KheResourceResourceTypeIndex(sn->resource) + 1;
	sn->c1 = c1, sn->c2 = c2, sn->c3 = c3, sn->c4 = c4;
      }
      else
	sn->c1 = sn->c2 = sn->c3 = sn->c4 = INT_MAX;

    ** sort the supply nodes by increasing cost **
    HaArraySort(rms->supply_nodes, &KheSupplyNodeIncreasingCostCmp);
    if( DEBUG4 )
    {
      fprintf(stderr, "  edges sorted by increasing cost:\n");
      HaArrayForEach(rms->supply_nodes, sn, si)
	KheResourceMatchingEdgeDebug(dn, sn, 1, sn->c1, sn->c2, sn->c3,
	  2, stderr);
    }

    ** assign the tasks of dn using the best of them **
    if( DEBUG4 )
      fprintf(stderr, "  best edges:\n");
    if( dn->grouped )
    {
      sn = HaArrayFirst(rms->supply_nodes);
      if( sn->resource != NULL )
        KheDemandNodeAssignAllTasks(dn, sn->resource);
    }
    else
    {
      ti = si = 0;
     while(ti < HaArrayCount(dn->tasks) && si < HaArrayCount(rms->supply_nodes))
      {
	task = HaArray(dn->tasks, ti);
	sn = HaArray(rms->supply_nodes, si);
	if( sn->resource != NULL )
	{
	  ** assign sn->resource to task **
          assigned = KheDemandNod eAssignOneTask(dn, task, sn->resource);
	  if( DEBUG4 )
	  {
	    fprintf(stderr, "  %s: ", assigned ? "assigned" : "not assigned");
            KheResourceMatchingEdgeDebug(dn, sn, 1, sn->c1, sn->c2, sn->c3,
	      0, stderr);
	  }
	  ti++;
	  si++;
	}
	else
	{
	  ** leave all remaining tasks unassigned **
	  if( DEBUG4 )
            KheResourceMatchingEdgeDebug(dn, sn, 1, sn->c1, sn->c2, sn->c3,
	      2, stderr);
	  ti = HaArrayCount(dn->tasks);
	  si = HaArrayCount(rms->supply_nodes);
	}
      }
    }
  }
  else
  {
    if( DEBUG4 )
      fprintf(stderr, "  non-assignment has no cost\n");
  }
  if( DEBUG4 )
    fprintf(stderr,
      "] KheResourceMatchingMakeBestAsst returning, final cost %.5f\n",
      KheCostShow(KheSolnCost(rms->soln)));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnActiveDefectsDebug(KHE_SOLN soln, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of defective limit active intervals monitors.                */
/*                                                                           */
/*****************************************************************************/

/* *** good but currently unused
static void KheSolnActiveDefectsDebug(KHE_SOLN soln, int verbosity,
  int indent, FILE *fp)
{
  KHE_MONITOR m;  int i;
  fprintf(stderr, "%*s[ limit active intervals defects:\n", indent, "");
  for( i = 0;  i < KheSolnDefectCount(soln);  i++ )
  {
    m = KheSolnDefect(soln, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
      KheMonitorDebug(m, verbosity, indent + 2, fp);
  }
  fprintf(stderr, "%*s]\n", indent, "");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnDefectsDebug(KHE_SOLN soln, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of defective monitors.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheSolnDefectsDebug(KHE_SOLN soln, int verbosity,
  int indent, FILE *fp)
{
  KHE_MONITOR m;  int i;
  fprintf(stderr, "%*s[ all defects:\n", indent, "");
  for( i = 0;  i < KheSolnDefectCount(soln);  i++ )
  {
    m = KheSolnDefect(soln, i);
    KheMonitorDebug(m, verbosity, indent + 2, fp);
  }
  fprintf(stderr, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, char *fn_name)       */
/*                                                                           */
/*  Brief report of what is starting, for debugging.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, char *fn_name)
{
  KHE_INSTANCE ins;  KHE_TIMER timer;  char buff[20];
  if( DEBUG6 )
  {
    ins = KheSolnInstance(soln);
    fprintf(stderr, "%s div %d", KheInstanceId(ins), KheSolnDiversifier(soln));
    if( KheOptionsContainsTimer(options, "global", &timer) )
      fprintf(stderr, " after %s",
	KheTimeShow(KheTimerElapsedTime(timer), buff));
    fprintf(stderr, " ResourceMatching %s\n", fn_name);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMatchingSolve(KHE_RESOURCE_MATCHING_SOLVER rms,          */
/*    bool edge_adjust1_off, bool edge_adjust2_off, bool edge_adjust3_off,   */
/*    bool edge_adjust4_off, bool nocost_off, KHE_OPTIONS options)           */
/*                                                                           */
/*  Do the actual solve.  The demand sets are already loaded into rms.       */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceMatchingSolve(KHE_RESOURCE_MATCHING_SOLVER rms,
  bool edge_adjust1_off, bool edge_adjust2_off, bool edge_adjust3_off,
  bool edge_adjust4_off, /* bool nocost_off, */ bool ejection_off,
  KHE_OPTIONS options)
{
  int i, j, c1, c2, c3, mult, ti, first_time_index, ts_count;  KHE_TASK task;
  KHE_SOLN soln;  KHE_SUPPLY_NODE sn;  /* KHE_FRAME_WORKLOAD fw; */
  KHE_DEMAND_NODE dn;  KHE_COST init_cost, solve_cost, tmp_cost, final_cost;
  KHE_MMATCH_NODE msn;  KHE_MARK init_mark;
  /* KHE_EVENT_TIMETABLE_MONITOR etm; */
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_GROUP_MONITOR gm;
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, rmds2;  bool consec;

  /* decide whether this run is immediately after the previous run */
  rmds = HaArrayFirst(rms->demand_sets);
  first_time_index = KheTimeIndex(KheTimeSetTime(rmds->time_set, 0));
  consec = (rms->prev_last_time_index + 1 == first_time_index);
  ts_count = KheTimeSetTimeCount(rmds->time_set);
  rms->prev_last_time_index =
    KheTimeIndex(KheTimeSetTime(rmds->time_set, ts_count - 1));

  if( DEBUG6 || DEBUG10 )
  {
    fprintf(stderr, "[ %sKheResourceMatchingSolve(",
      consec ? "consecutive " : "");
    KheResourceGroupDebug(rms->resource_group, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTimeSetDebug(rmds->time_set, 1, -1, stderr);
    if( HaArrayCount(rms->demand_sets) > 1 )
    {
      fprintf(stderr, " lookahead ");
      rmds2 = HaArrayLast(rms->demand_sets);
      KheTimeSetDebug(rmds2->time_set, 2, -1, stderr);
    }
    fprintf(stderr, ", %s, %s, %s, %s, opts)\n", bool_show(edge_adjust1_off),
      bool_show(edge_adjust2_off), bool_show(edge_adjust3_off),
      bool_show(edge_adjust4_off));
  }

  /* turn off adjust3 and adjust4 if not consecutive */
  if( !consec )
  {
    edge_adjust3_off = edge_adjust4_off = true;
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( sn->resource != NULL )
      {
        sn->curr_was_used = false;
        sn->curr_run_len = 0;
        sn->curr_offset = -1;
      }
  }

  /* set fields with a well-defined value during a single solve */
  rms->options = options;
  /* ***
  rms->common_frame = KheOptionsFrame(options, "gs_common_frame", rms->soln);
  *** */
  rms->resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  rms->edge_adjust1_off = edge_adjust1_off;
  rms->edge_adjust2_off = edge_adjust2_off;
  rms->edge_adjust3_off = edge_adjust3_off;

  /* prepare demand sets for solving, if not already done */
  soln = rms->soln;
  HaArrayForEach(rms->demand_sets, rmds2, i)
    if( !rmds2->ready_for_solving )
    {
      if( DEBUG6 )
	KheSolveDebug(soln, options, "rmatch preparing rmds2 for solving");
      KheResourceMatchingDemandSetPrepareForSolving(rmds2 /* , nocost_off */);
      rmds2->ready_for_solving = true;
    }

  /* if no assignments to make, return; if there are no demand nodes, then */
  /* the multiplicity of the supply node representing non-assignment is 0, */
  /* which will fail in the mmatch module, so we need this special case    */
  if( HaArrayCount(rmds->demand_nodes) == 0 )
  {
    if( DEBUG6 || DEBUG10 )
      fprintf(stderr, "] KheResourceMatchingrSolve returning false"
	" (no demands)\n");
    return false;
  }

  /* add max busy times to resource supply nodes, if required */
  if( !rms->edge_adjust1_off && !rms->max_busy_set )
  {
    if( DEBUG6 )
      KheSolveDebug(soln, options, "rmatch adding max_busy to supply nodes");
    /* ***
    etm = (KHE_EVENT_TIMETABLE_MONITOR)
      KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
    fw = KheFrameWorkload Make(rms->common_frame,
      KheResourceGroupResourceType(rms->resource_group), etm);
    ** */
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( sn->resource != NULL )
      {
	if( !KheResourceMaxBusyTimes(soln, sn->resource, &sn->max_busy) )
	  sn->max_busy = KheInstanceTimeCount(KheSolnInstance(soln));
	/* ***
	sn->max_busy = KheFrameReso urceMaxBusyTimes(rms->common_frame,
	  fw, sn->resource);
	*** */
      }
    rms->max_busy_set = true;
    /* KheFrameWorkloadDelete(fw); */
  }

  /* initialize adjustment fields in the supply nodes */
  if( !rms->edge_adjust1_off )
  {
    if( DEBUG6 )
      KheSolveDebug(soln, options, "rmatch adding curr_busy to supply nodes");
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( sn->resource != NULL )
	sn->curr_busy = KheResourceBusyTimes(soln, sn->resource);
	/* ***
	sn->curr_busy = KheOldReso urceBusyTimes(soln, sn->resource);
	*** */
  }
  if( !rms->edge_adjust3_off && !rms->edge_adjust4_off )
  {
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( sn->resource != NULL )
	sn->curr_was_used = false;
  }

  /* changes start here */
  init_cost = KheSolnCost(soln);
  init_mark = KheMarkBegin(soln);
  if( DEBUG4 )
  {
    fprintf(stderr, "  KheResourceMatchingSolve initial solution:\n");
    KheSolnDebug(soln, 2, 2, stderr);
    KheSolnDefectsDebug(soln, 2, 2, stderr);
  }

  /* carry out any grouping, and detach affected limit resources monitors */
  KheSolnAdjusterRedo(rmds->soln_adjuster);
  /* KheTaskResourceGrouperGroup(rmds->task_grouper); */
  HaArrayForEach(rmds->monitors, lrm, i)
    KheMonitorDetachFromSoln((KHE_MONITOR) lrm);

  /* unassign the tasks in the demand nodes */
  HaArrayForEach(rmds->demand_nodes, dn, i)
    KheDemandNodeUnAssignTasks(dn);

  /* build the mmatch and add supply and demand nodes */
  if( DEBUG6 )
    KheSolveDebug(soln, options, "rmatch graph build (demand %d, supply %d)",
      HaArrayCount(rmds->demand_nodes), HaArrayCount(rms->supply_nodes));
  if( DEBUG7 )
    fprintf(stderr, "    graph supply nodes:\n");
  HaArrayForEach(rms->supply_nodes, sn, i)
  {
    KheSupplyNodeAddMMatchNode(sn, rms->mmatch);
    if( DEBUG7 )
      KheSupplyNodeDebug(sn, 1, 6, stderr);
  }
  if( DEBUG7 )
    fprintf(stderr, "    graph demand nodes:\n");
  HaArrayForEach(rmds->demand_nodes, dn, i)
  {
    KheDemandNodeAddMMatchNode(dn, rms->mmatch);
    if( DEBUG7 )
      KheDemandNodeDebug(dn, 1, 6, stderr);
  }
  if( DEBUG7 )
  {
    c1 = KheHardCost(init_cost);
    c2 = KheSoftCost(init_cost);
    c3 = 0;
    fprintf(stderr, "    graph edges (initial cost ");
    KheResourceMatchingCostDebug(c1, c2, c3, stderr);
    fprintf(stderr, "):\n");
  }
  HaArrayForEach(rmds->demand_nodes, dn, i)
    HaArrayForEach(rms->supply_nodes, sn, j)
      if( KheResourceMatchingEdgeFn(dn, sn, &c1, &c2, &c3) )
      {
	mult = (sn->resource != NULL ? 1 : KheDemandNodeMultiplicity(dn));
	KheMMatchAddEdge(dn->mmatch_node, sn->mmatch_node, mult, c1, c2, c3);
	if( DEBUG7 && !DEBUG9 )
	  KheResourceMatchingEdgeDebug(dn, sn, mult, c1, c2, c3, 6, stderr);
      }

  /* solve, extract the matching edges and assign them */
  if( DEBUG6 )
    KheSolveDebug(soln, options, "rmatch solving the matching graph");
  KheMMatchSolve(rms->mmatch);
  if( DEBUG7 )
  {
    fprintf(stderr, "    MMatch after solving:\n");
    KheMMatchDebug(rms->mmatch, 4, 4, stderr);
    fprintf(stderr, "    matching edges:\n");
  }
  if( DEBUG6 )
    KheSolveDebug(soln, options, "rmatch making assignments");
  HaArrayForEach(rmds->demand_nodes, dn, i)
  {
    ti = 0;
    while( KheMMatchResultEdge(dn->mmatch_node, &msn, &mult, &c1, &c2, &c3) )
    {
      sn = (KHE_SUPPLY_NODE) KheMMatchSupplyNodeBack(msn);
      if( DEBUG7 )
	KheResourceMatchingEdgeDebug(dn, sn, mult, c1, c2, c3, 6, stderr);
      for( j = 0;  j < mult;  j++ )
      {
	HnAssert(ti < HaArrayCount(dn->tasks),
	  "KheResourceMatchingSolve internal error 2");
	task = HaArray(dn->tasks, ti);
	ti++;
	if( DEBUG7 )
	{
	  fprintf(stderr, "        ");
	  KheTaskDebug(task, 1, -1, stderr);
	  fprintf(stderr, " := ");
	  KheSupplyNodeDebug(sn, 1, 0, stderr);
	}
	if( sn->resource != NULL )
	{
	  KheDemandNodeAssignOneTask(dn, task, sn->resource);
	  sn->curr_was_used = true;
	  sn->curr_offset = dn->offset_in_day;
	}
      }
    }
    HnAssert(ti == HaArrayCount(dn->tasks),
      "KheResourceMatchingSolve internal error 3");
  }

  /* clear out the mmatch */
  if( DEBUG6 )
    KheSolveDebug(soln, options, "rmatch clearing the mmatch");
  KheMMatchClear(rms->mmatch);

  /* set up run length for next time */
  if( !rms->edge_adjust3_off )
    HaArrayForEach(rms->supply_nodes, sn, i)
      if( sn->resource != NULL )
	sn->curr_run_len = (sn->curr_was_used ? sn->curr_run_len + 1 : 0);

  /* ungroup, and reattach affected limit resources monitors */
  tmp_cost = KheSolnCost(soln);
  /* KheTaskResourceGrouperUnGroup(rmds->task_grouper); */
  KheSolnAdjusterUndo(rmds->soln_adjuster);
  HaArrayForEach(rmds->monitors, lrm, i)
    KheMonitorAttachToSoln((KHE_MONITOR) lrm);
  if( DEBUG6 )
    KheSolveDebug(soln, options, "rmatch ungroup/reattach (%.5f -> %.5f)",
      KheCostShow(tmp_cost), KheCostShow(KheSolnCost(soln)));
  /* *** not such a great idea
  if( DEBUG11 )
    KheSolnMatchingMarkCheck(soln);
  *** */

  if( !ejection_off )
  {
    gm = NULL;
    HaArrayForEach(rmds->monitors, lrm, i)
      if( KheMonitorCost((KHE_MONITOR) lrm) > 0 )
      {
	if( gm == NULL )
	  gm = KheGroupMonitorMake(soln, KHE_SUBTAG_RESOURCE_REPAIR,
	    "ResourceRepairGroupMonitor");
	KheGroupMonitorAddChildMonitor(gm, (KHE_MONITOR) lrm);
      }
    /* *** a failed experiment
    if( gm == NULL )
      gm = (KHE_GROUP_MONITOR) soln;
    *** */
    if( gm != NULL )
    {
      if( DEBUG6 )
      {
	KHE_MONITOR m;
	KheSolveDebug(soln, options, "rmatch ejection chain repair:");
	for( i = 0;  i < KheGroupMonitorChildMonitorCount(gm);  i++ )
	{
	  m = KheGroupMonitorChildMonitor(gm, i);
	  KheMonitorDebug(m, 2, 2, stderr);
	}
      }
      tmp_cost = KheSolnCost(soln);
      KheEjectionChainRepairInitialResourceAssignment(gm, options);
      /* if( gm != (KHE_GROUP_MONITOR) soln ) */
      KheGroupMonitorDelete(gm);
      if( DEBUG6 )
	KheSolveDebug(soln, options,
	  "rmatch ejection chain repair end (%.5f -> %.5f)",
	  KheCostShow(tmp_cost), KheCostShow(KheSolnCost(soln)));
    }
  }

  /* if new solution is not better, return to original solution */
  if( DEBUG6 )
    KheSolveDebug(soln, options, "before KheSolnCost");
  solve_cost = KheSolnCost(soln);
  if( DEBUG6 )
    KheSolveDebug(soln, options, "before KheMarkEnd");
  KheMarkEnd(init_mark, solve_cost >= init_cost);
  final_cost = KheSolnCost(soln);
  if( DEBUG4 )
  {
    fprintf(stderr, "  KheResourceMatchingSolve final solution:\n");
    KheSolnDebug(soln, 2, 2, stderr);
    KheSolnDefectsDebug(soln, 2, 2, stderr);
  }
  HnAssert(final_cost <= init_cost,
    "KheResourceMatchingSolve internal error 4 (init %.5f, final %.5f)",
    KheCostShow(init_cost), KheCostShow(final_cost));
  if( DEBUG6 || DEBUG10 )
  {
    if( solve_cost < init_cost )
      fprintf(stderr, "] KheResourceMatchingSolve returning success: "
	"%.5f -> %.5f\n", KheCostShow(init_cost), KheCostShow(solve_cost));
    else
      fprintf(stderr, "] KheResourceMatchingSolve returning failure: "
	"%.5f (solve cost was %.5f)\n", KheCostShow(init_cost),
	KheCostShow(solve_cost));
  }
  return final_cost < init_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMatchingSolverSolve(KHE_RESOURCE_MATCHING_SOLVER rms,    */
/*    KHE_RESOURCE_MATCHING_DEMAND_SET rmds, bool edge_adjust1_off,          */
/*    bool edge_adjust2_off, bool edge_adjust3_off, bool edge_adjust4_off,   */
/*    bool nocost_off, KHE_OPTIONS options)                                  */
/*                                                                           */
/*  Do the actual solve.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMatchingSolverSolve(KHE_RESOURCE_MATCHING_SOLVER rms,
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds, bool edge_adjust1_off,
  bool edge_adjust2_off, bool edge_adjust3_off, bool edge_adjust4_off,
  bool ejection_off, /* bool nocost_off, */ KHE_OPTIONS options)
{
  HaArrayClear(rms->demand_sets);
  HaArrayAddLast(rms->demand_sets, rmds);
  return KheResourceMatchingSolve(rms, edge_adjust1_off, edge_adjust2_off,
    edge_adjust3_off, edge_adjust4_off, ejection_off, /*nocost_off,*/ options);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMatchingSolverSolveWithLookahead(                        */
/*    KHE_RESOURCE_MATCHING_SOLVER rms,                                      */
/*    ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET *rmds_array,                    */
/*    int first_index, int last_index, bool edge_adjust1_off,                */
/*    bool edge_adjust2_off, bool edge_adjust3_off,                          */
/*    bool edge_adjust4_off, bool nocost_off, KHE_OPTIONS options)           */
/*                                                                           */
/*  Like KheResourceMatchingSolverSolve only with lookahead.                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceMatchingSolverSolveWithLookahead(
  KHE_RESOURCE_MATCHING_SOLVER rms,
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET *rmds_array,
  int first_index, int last_index, bool edge_adjust1_off,
  bool edge_adjust2_off, bool edge_adjust3_off, bool edge_adjust4_off,
  bool ejection_off, /* bool nocost_off, */ KHE_OPTIONS options)
{
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds;  int i;
  HaArrayClear(rms->demand_sets);
  for( i = first_index;  i <= last_index;  i++ )
  {
    rmds = HaArray(*rmds_array, i);
    HaArrayAddLast(rms->demand_sets, rmds);
  }
  return KheResourceMatchingSolve(rms, edge_adjust1_off, edge_adjust2_off,
    edge_adjust3_off, edge_adjust4_off, ejection_off, /*nocost_off,*/ options);
}

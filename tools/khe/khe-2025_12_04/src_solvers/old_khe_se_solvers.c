
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
/*  FILE:         khe_se_solvers.c                                           */
/*  DESCRIPTION:  Ejection chain solvers                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define IGNORE_NOCOST true
#define RANDOMIZED    false	/* try just one randomly chosen repair */

#define DEBUG1 0	/* main calls */
#define DEBUG2 0	/* initial and final defects */
#define DEBUG3 0	/* main augment functions */

#define DEBUG4 0	/* augment calls */
#define DEBUG5 0	/* repairs tried */
#define DEBUG6 0	/* demand repairs */
#define DEBUG7 0	/* cluster repairs */
#define DEBUG8 0	/* complex idle times repairs */
#define DEBUG9 0	/* complex idle times recursion */
#define DEBUG10 0	/* split repairs */

#define DEBUG11 0
#define DEBUG12 0
#define DEBUG13 0
#define DEBUG14 0
#define DEBUG15 0	/* avoid split assignments repairs */
#define DEBUG16 0	/* limit idle meet bound repairs */
#define DEBUG17 0	/* extra limit busy times repairs */
#define DEBUG18 0	/* new part of OverloadAugment */
#define DEBUG19 0	/* NurseOver */
#define DEBUG20 0	/* NurseOver */
#define DEBUG21 0	/* KheFrameMove */
#define DEBUG22 0	/* KheTaskSetRemovesUnderload */
#define DEBUG23 0	/* KheWorkloadLimitsAreTight */
#define DEBUG24 0	/* KheEjectionChainRepairInitialResourceAssignment */
#define DEBUG26 0	/* NoClashes check */
#define DEBUG27 0	/* visiting */
#define DEBUG28 0	/* cluster overload augment */
#define DEBUG29 0	/* KheResourceOverloadMultiRepair */
#define DEBUG30 0
#define DEBUG31 0
#define DEBUG32 0
#define DEBUG33 0	/* double moves */
#define DEBUG34 0	/* swap to end repair */
#define DEBUG35 1

#define MAX_MOVE 3	/* max number of days to left and right to move */

#define MAX_LIT_REPAIRS 1000	/* max number of complex limit idle repairs */

#define WITH_DEMAND_NODE_SWAPS true
#define WITH_SPREAD_EVENTS_NODE_SWAPS false
#define WITH_PREFER_TIMES_NODE_SWAPS false
#define WITH_OLD_AVOID_SPLIT_REPAIR 0
#define WITH_OLD_SPREAD_REPAIR 0

#define WITH_OLD_LIMIT_IDLE_REPAIR 1
#define WITH_NEW_LIMIT_IDLE_REPAIR_OVERLAP 0
#define WITH_NEW_LIMIT_IDLE_REPAIR_EXACT 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "augment options"                                              */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_OPTIONS_KEMPE_FALSE,
  KHE_OPTIONS_KEMPE_LARGE_LAYERS,
  KHE_OPTIONS_KEMPE_TRUE
} KHE_OPTIONS_KEMPE;

typedef HA_ARRAY(KHE_MEET) ARRAY_KHE_MEET;
typedef HA_ARRAY(KHE_TIME_GROUP) ARRAY_KHE_TIME_GROUP;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_TYPE - augment types (i.e. defect types)                     */
/*                                                                           */
/*  The order here follows the order of monitor types used in the Guide.     */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_AUGMENT_ORDINARY_DEMAND,
  KHE_AUGMENT_WORKLOAD_DEMAND,
  KHE_AUGMENT_SPLIT_EVENTS,
  KHE_AUGMENT_ASSIGN_TIME,
  KHE_AUGMENT_PREFER_TIMES,
  KHE_AUGMENT_SPREAD_EVENTS,
  KHE_AUGMENT_LINK_EVENTS,
  KHE_AUGMENT_ORDER_EVENTS,
  KHE_AUGMENT_ASSIGN_RESOURCE,
  KHE_AUGMENT_PREFER_RESOURCES,
  KHE_AUGMENT_AVOID_SPLIT_ASSIGNMENTS,
  KHE_AUGMENT_LIMIT_RESOURCES,
  KHE_AUGMENT_AVOID_CLASHES,
  KHE_AUGMENT_AVOID_UNAVAILABLE_TIMES,
  KHE_AUGMENT_LIMIT_IDLE_TIMES,
  KHE_AUGMENT_CLUSTER_BUSY_TIMES,
  KHE_AUGMENT_LIMIT_BUSY_TIMES,
  KHE_AUGMENT_LIMIT_WORKLOAD,
  KHE_AUGMENT_LIMIT_ACTIVE_INTERVALS
} KHE_AUGMENT_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_REPAIR_TYPE - repair types (i.e. repair operations)                  */
/*                                                                           */
/*  There doesn't seem to be any natural order for repair types.  These      */
/*  ones originally appeared in the order they appear in the text of my      */
/*  PATAT14 paper, but they have been fiddled with since then.               */
/*                                                                           */
/*  It's not clear that these categories are particularly useful.  In some   */
/*  cases they don't distinguish move from assignment and unassignment.      */
/*  In other cases the same repair is called from different contexts, and    */
/*  it would be better to measure its effectiveness in each context.         */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* repairs applied to meets */
  KHE_REPAIR_MEET_MOVE_UNCHECKED,
  KHE_REPAIR_MEET_MOVE_CHECKED,
  KHE_REPAIR_MEET_MOVE_EJECTING,
  KHE_REPAIR_MEET_MOVE_KEMPE,

  /* old meet move types, mostly obsolete */
  KHE_REPAIR_FUZZY_MEET_MOVE,

  /* repairs applied to tasks and task sets */
  KHE_REPAIR_TASK_DOUBLE_MOVE,
  KHE_REPAIR_TASK_SET_MOVE,
  KHE_REPAIR_TASK_SET_DOUBLE_MOVE,
  KHE_REPAIR_TASK_SET_REPLACE,
  KHE_REPAIR_TASK_SET_DOUBLE_REPLACE,
  KHE_REPAIR_RESOURCE_MOVE,

  /* combined operations */
  KHE_REPAIR_NODE_MEET_SWAP,
  KHE_REPAIR_MEET_SPLIT,
  KHE_REPAIR_SPLIT_MOVE,
  KHE_REPAIR_MERGE_MOVE,
  KHE_REPAIR_SPLIT_TASKS_UNASSIGN,
  KHE_REPAIR_MEET_BOUND,
  KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN,
  KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE,
  KHE_REPAIR_COMPLEX_IDLE_MOVE
} KHE_REPAIR_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_OPTIONS - augment options                                    */
/*                                                                           */
/*****************************************************************************/

struct khe_augment_options_rec {

  /* time repair options */
  bool				vizier_node;
  bool				layer_repair_long;
  bool				nodes_before_meets;
  KHE_OPTIONS_KEMPE		use_kempe_moves;
  bool				use_fuzzy_moves;
  bool				no_ejecting_moves;

  /* resource repair options */
  bool				widening_off;
  bool				reversing_off;
  bool				balancing_off;
  int				widening_max;
  int				balancing_max;
  bool 				full_widening_on;
  bool				optimal_on;
  int				optimal_width;

  /* options that are set by functions, not by the user */
  bool				repair_times;
  bool				use_split_moves;
  KHE_KEMPE_STATS		kempe_stats;
  KHE_NODE			limit_node;
  bool				repair_resources;
  KHE_FRAME			frame;
  KHE_EVENT_TIMETABLE_MONITOR	event_timetable_monitor;
  KHE_TASK_FINDER		task_finder;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "augment options"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectorRepairTimes(KHE_EJECTOR ej)                               */
/*  KHE_NODE KheEjectorLimitNode(KHE_EJECTOR ej)                             */
/*  bool KheEjectorRepairResources(KHE_EJECTOR ej)                           */
/*  bool KheEjectorNodesBeforeMeets(KHE_EJECTOR ej)                          */
/*  bool KheEjectorUseKempeMoves(KHE_EJECTOR ej)                             */
/*  bool KheEjectorBasicNotEjecting(KHE_EJECTOR ej)                          */
/*  bool KheEjectorUseFuzzyMoves(KHE_EJECTOR ej)                             */
/*  bool KheEjectorUseSplitMoves(KHE_EJECTOR ej)                             */
/*  bool KheEjectorWideningOff(KHE_EJECTOR ej)                               */
/*  bool KheEjectorReversingOff(KHE_EJECTOR ej)                              */
/*  bool KheEjectorBalancingOff(KHE_EJECTOR ej)                              */
/*  bool KheEjectorNoCostOff(KHE_EJECTOR ej)                                 */
/*                                                                           */
/*  Return the indicated options.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by augment options
static bool KheEjectorRepairTimes(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_repair_times", false);
}

static KHE_NODE KheEjectorLimitNode(KHE_EJECTOR ej)
{
  return (KHE_NODE) KheOptionsGetObject(KheEjectorOptions(ej),
    "es_limit_node", NULL);
}

static bool KheEjectorRepairResources(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_repair_resources", false);
}

static bool KheEjectorNodesBeforeMeets(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_nodes_before_meets",
    false);
}

static KHE_OPTIONS_KEMPE KheEjectorUseKempeMoves(KHE_EJECTOR ej)
{
  char *opt = KheOptionsGet(KheEjectorOptions(ej), "es_kempe_moves",
    "large_layers");
  if( strcmp(opt, "false") == 0 )
    return KHE_OPTIONS_KEMPE_FALSE;
  else if( strcmp(opt, "large_layers") == 0 )
    return KHE_OPTIONS_KEMPE_LARGE_LAYERS;
  else if( strcmp(opt, "true") == 0 )
    return KHE_OPTIONS_KEMPE_TRUE;
  else
  {
    HnAbort("KheEjector: kempe_moves option has invalid value \"%s\"", opt);
    return KHE_OPTIONS_KEMPE_FALSE;
  }
}

static bool KheEjectorBasicNotEjecting(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_no_ejecting_moves",false);
}

static bool KheEjectorUseFuzzyMoves(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_fuzzy_moves", false);
}

static bool KheEjectorUseSplitMoves(KHE_EJECTOR ej)
{
  return KheOptionsGetBool(KheEjectorOptions(ej), "es_split_moves", false);
}

static KHE_KEMPE_STATS KheEjectorKempeStats(KHE_EJECTOR ej)
{
  return KheKempeStatsOption(KheEjectorOptions(ej), "ts_kempe_stats");
}

typedef struct khe_expansion_options_rec {
  bool	widening_off;
  int	widening_max;
  bool	reversing_off;
  bool	balancing_off;
  int	balancing_max;
  bool  full_widening_on;
  bool	optimal_on;
  int	optimal_width;
} *KHE_EXPANSION_OPTIONS;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_OPTIONS KheAugmentOptionsMake(KHE_SOLN soln,                 */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Create and return a new augment options object based on soln and         */
/*  options, and stored in arena a.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_AUGMENT_OPTIONS KheAugmentOptionsMake(KHE_SOLN soln,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_AUGMENT_OPTIONS res;  char *opt;
  HaMake(res, a);

  /* time repair options */
  res->vizier_node = KheOptionsGetBool(options, "es_vizier_node", false);
  res->layer_repair_long = KheOptionsGetBool(options, "es_layer_repair_long",
    false);
  res->nodes_before_meets =
    KheOptionsGetBool(options, "es_nodes_before_meets", false);
  opt = KheOptionsGet(options, "es_kempe_moves", "large_layers");
  if( strcmp(opt, "false") == 0 )
    res->use_kempe_moves = KHE_OPTIONS_KEMPE_FALSE;
  else if( strcmp(opt, "large_layers") == 0 )
    res->use_kempe_moves = KHE_OPTIONS_KEMPE_LARGE_LAYERS;
  else if( strcmp(opt, "true") == 0 )
    res->use_kempe_moves = KHE_OPTIONS_KEMPE_TRUE;
  else
  {
    HnAbort("KheAugmentOptionsMake: es_kempe_moves option has invalid value"
      " \"%s\"", opt);
    return KHE_OPTIONS_KEMPE_FALSE;
  }
  res->use_fuzzy_moves =
    KheOptionsGetBool(options, "es_fuzzy_moves", false);
  res->no_ejecting_moves =
    KheOptionsGetBool(options, "es_no_ejecting_moves",false);

  /* resource repair options */
  res->widening_off = KheOptionsGetBool(options, "es_widening_off", false);
  if( res->widening_off )
    res->widening_max = 0;
  else
    res->widening_max = KheOptionsGetInt(options, "es_widening_max", 4);
  res->reversing_off = KheOptionsGetBool(options, "es_reversing_off", false);
  res->balancing_off = KheOptionsGetBool(options, "es_balancing_off", false);
  if( res->balancing_off )
    res->balancing_max = 0;
  else
    res->balancing_max = KheOptionsGetInt(options, "es_balancing_max", 12);
  res->full_widening_on = KheOptionsGetBool(options, "es_full_widening_on",
    false);
  res->optimal_on = KheOptionsGetBool(options, "es_optimal_on", false);
  res->optimal_width = KheOptionsGetInt(options, "es_optimal_width", 6);

  /* options that are set by functions, not by the user */
  res->repair_times =
    KheOptionsGetBool(options, "es_repair_times", false);
  res->use_split_moves =
    KheOptionsGetBool(options, "es_split_moves", false);
  res->kempe_stats =
    KheKempeStatsOption(options, "ts_kempe_stats");
  res->limit_node =
    (KHE_NODE) KheOptionsGetObject(options, "es_limit_node",NULL);
  res->repair_resources =
    KheOptionsGetBool(options, "es_repair_resources", false);
  res->frame = KheOptionsFrame(options, "gs_common_frame", soln);
  res->event_timetable_monitor =
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  res->task_finder =
    KheTaskFinderMake(soln, options, a);

  return res;
}


/* *** replaced by KheAugmentOptionsMake
static void KheEjectorExpansionOptions(KHE_EJECTOR ej, KHE_EXPANSION_OPTIONS eo)
{
  KHE_OPTIONS options;
  options = KheEjectorOptions(ej);
  eo->widening_off = KheOptionsGetBool(options, "es_widening_off", false);
  if( eo->widening_off )
    eo->widening_max = 0;
  else
    eo->widening_max = KheOptionsGetInt(options, "es_widening_max", 4);
  eo->reversing_off = KheOptionsGetBool(options, "es_reversing_off", false);
  eo->balancing_off = KheOptionsGetBool(options, "es_balancing_off", false);
  if( eo->balancing_off )
    eo->balancing_max = 0;
  else
    eo->balancing_max = KheOptionsGetInt(options, "es_balancing_max", 12);
  eo->full_widening_on = KheOptionsGetBool(options, "es_full_widening_on",
    false);
  eo->optimal_on = KheOptionsGetBool(options, "es_optimal_on", false);
  eo->optimal_width = KheOptionsGetInt(options, "es_optimal_width", 6);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "clash checking"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheNoClashesCheck(KHE_EJECTOR ej, KHE_RESOURCE r)                   */
/*                                                                           */
/*  Make sure that r has no clashes.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheNoClashesCheck(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r)
{
  if( DEBUG26 && r != NULL )
    KheFrameResourceAssertNoClashes(ao->frame, r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClashes(KHE_EJECTOR ej, KHE_RESOURCE r)                          */
/*                                                                           */
/*  Return true if there are clashes.                                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheClashes(KHE_EJECTOR ej, KHE_RESOURCE r)
{
  return DEBUG26 && r != NULL &&
    KheFrameResourceHasClashes(KheEjectorFrame(ej), r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource iterator"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SE_RESOURCE_SET - a set of resources, defined very flexibly          */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_se_resource_set_type {
  KHE_RESOURCE_GROUP		include_rg;
  KHE_RESOURCE_GROUP		exclude_rg;
  int				curr_index;
  int				stop_index;
  int				random_offset;
} *KHE_SE_RESOURCE_SET;


/*****************************************************************************/
/*                                                                           */
/*  void KheSeResourceSetInit(KHE_SE_RESOURCE_SET srs,                       */
/*    KHE_RESOURCE_GROUP include_rg, KHE_RESOURCE_GROUP exclude_rg,          */
/*    bool include_nonassign)                                                */
/*                                                                           */
/*  Initialize a resource set object.  It will iterate over the resources    */
/*  of include_rg, skipping the resources of exclude_rg if non-NULL.  If     */
/*  include_nonassign is true, non-assignment in the form of a NULL          */
/*  resource will be included in the iteration.  It comes first if present.  */
/*                                                                           */
/*****************************************************************************/

static void KheSeResourceSetInit(KHE_SE_RESOURCE_SET srs,
  KHE_RESOURCE_GROUP include_rg, KHE_RESOURCE_GROUP exclude_rg,
  bool include_nonassign, int random_offset)
{
  srs->include_rg = include_rg;
  srs->exclude_rg = exclude_rg;
  srs->curr_index = include_nonassign ? -1 : 0;
  srs->stop_index = KheResourceGroupResourceCount(include_rg);
  srs->random_offset = random_offset;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSeResourceSetNext(KHE_SE_RESOURCE_SET srs, KHE_RESOURCE *r)      */
/*                                                                           */
/*  Return the next resource if there is one, or 0 otherwise.                */
/*                                                                           */
/*****************************************************************************/

static bool KheSeResourceSetNext(KHE_SE_RESOURCE_SET srs, KHE_RESOURCE *r)
{
  KHE_RESOURCE r2;
  if( srs->curr_index == -1 )
  {
    srs->curr_index++;
    return *r = NULL, true;
  }
  else
  {
    while( srs->curr_index < srs->stop_index )
    {
      r2 = KheResourceGroupResource(srs->include_rg,
	(srs->curr_index + srs->random_offset) % srs->stop_index);
      srs->curr_index++;
      if( srs->exclude_rg == NULL ||
	  !KheResourceGroupContains(srs->exclude_rg, r2) )
	return *r = r2, true;
    }
    return *r = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheForEachResource - iterator over resources                              */
/*                                                                           */
/*****************************************************************************/

#define KheForEachResource(srs_rec, irg, erg, in, random_offset, r)	\
for( KheSeResourceSetInit(&(srs_rec), irg, erg, in, random_offset);	\
     KheSeResourceSetNext(&(srs_rec), &r); )


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task and task set visiting and equivalence"                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskEquiv(KHE_TASK task1, KHE_TASK task2)                        */
/*                                                                           */
/*  Return true if task1 and task2 are equivalent and unassigned.            */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskEquiv(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskAsstResource(task1) == KheTaskAsstResource(task2) &&
    KheTaskEquivalent(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet bound repairs"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMeetBoundOnSuccess(void *on_success_val)                         */
/*                                                                           */
/*  On-success function used by meet bound repairs.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMeetBoundOnSuccess(KHE_MEET_BOUND mb)
{
  bool success;
  if( DEBUG13 )
    fprintf(stderr, "[ KheMeetBoundOnSuccess\n");
  success = KheMeetBoundDelete(mb);
  if( DEBUG13 )
    fprintf(stderr, "] KheMeetBoundOnSuccess(success = %s)\n",
      success ? "true" : "false");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetBoundRepair(KHE_EJECTOR ej, KHE_RESOURCE r,                  */
/*    KHE_TIME_GROUP include_tg)                                             */
/*                                                                           */
/*  Reduce the domains of the meets currently assigned r, to contain a       */
/*  subset of the times of include_tg.  Meets currently assigned outside     */
/*  include_tg are unassigned first.  Return true if all successful.         */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetBoundRepair(KHE_EJECTOR ej, KHE_RESOURCE r,
  KHE_TIME_GROUP include_tg)
{
  int i, junk, durn;  KHE_TASK task;  KHE_TIME time;  KHE_MEET meet;
  KHE_MEET_BOUND mb;  KHE_SOLN soln;  bool success;
  if( DEBUG13 )
  {
    fprintf(stderr, "[ KheMeetBoundRepair(ej, %s, ", KheResourceId(r));
    KheTimeGroupDebug(include_tg, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* unassign r's meets outside include_tg; add time bounds to all r's meets */
  soln = KheEjectorSoln(ej);
  KheEjectorRepairBegin(ej);
  mb = KheMeetBoundMake(soln, true, include_tg);
  for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
  {
    task = KheResourceAssignedTask(soln, r, i);
    meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    if( meet == NULL || KheMeetIsPreassigned(meet, &time) )
    {
      if( DEBUG13 )
	fprintf(stderr, "] KheMeetBoundRepair returning false (no unfixed)");
      return KheEjectorRepairEnd(ej, KHE_REPAIR_MEET_BOUND, false);
    }
    durn = KheMeetDuration(meet);
    time = KheMeetAsstTime(meet);
    if( time != NULL && KheTimeGroupOverlap(include_tg, time, durn) != durn )
    {
      if( DEBUG13 )
      {
	fprintf(stderr, " unassigning ");
	KheMeetDebug(meet, 1, 0, stderr);
      }
      if( !KheMeetUnAssign(meet) )
      {
	if( DEBUG13 )
	{
	  fprintf(stderr, "] KheMeetBoundRepair returning false");
	  fprintf(stderr, " cannot unassign ");
	  KheMeetDebug(meet, 1, 0, stderr);
	}
	return KheEjectorRepairEnd(ej, KHE_REPAIR_MEET_BOUND, false);
      }
    }
    if( !KheMeetAddMeetBound(meet, mb) )
    {
      if( DEBUG13 )
      {
	fprintf(stderr, "] KheMeetBoundRepair returning false");
	fprintf(stderr, " cannot bound ");
	KheMeetDebug(meet, 1, 0, stderr);
      }
      return KheEjectorRepairEnd(ej, KHE_REPAIR_MEET_BOUND, false);
    }
  }
  success = KheEjectorRepairEnd(ej, KHE_REPAIR_MEET_BOUND, true);
  if( success )
    KheMeetBoundOnSuccess(mb);
  if( DEBUG13 )
    fprintf(stderr, "] KheMeetBoundRepair returning %s\n",
      success ? "true" : "False");
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meet move repairs"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TRY_MEET_MOVE_FN                                                     */
/*                                                                           */
/*  Functions of this type are called to decide whether to try meet moves    */
/*  of meet (or of an ancestor of meet) which give meet this starting time.  */
/*                                                                           */
/*  Parameter impl gives access to other information whose type is known     */
/*  to the function, which it can use in making its decision.                */
/*                                                                           */
/*****************************************************************************/

typedef bool (*KHE_TRY_MEET_MOVE_FN)(KHE_MEET meet, KHE_TIME time, void *impl);


/*****************************************************************************/
/*                                                                           */
/*  KHE_REPAIR_TYPE KheMoveTypeToMeetRepairType(KHE_MOVE_TYPE mt)            */
/*                                                                           */
/*  Convert move type mt into the corresponding task repair type.            */
/*                                                                           */
/*****************************************************************************/

static KHE_REPAIR_TYPE KheMoveTypeToMeetRepairType(KHE_MOVE_TYPE mt)
{
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:	return KHE_REPAIR_MEET_MOVE_UNCHECKED;
    case KHE_MOVE_CHECKED:	return KHE_REPAIR_MEET_MOVE_CHECKED;
    case KHE_MOVE_EJECTING:	return KHE_REPAIR_MEET_MOVE_EJECTING;
    case KHE_MOVE_KEMPE:	return KHE_REPAIR_MEET_MOVE_KEMPE;

    default:
      HnAbort("KheMoveTypeToMeetRepairType internal error");
      return KHE_REPAIR_MEET_MOVE_UNCHECKED;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,                     */
/*    KHE_MEET target_meet, int offset, KHE_MOVE_TYPE mt,                    */
/*    bool preserve_regularity, bool *basic, KHE_KEMPE_STATS kempe_stats)    */
/*                                                                           */
/*  Try a typed meet move of meet to target_meet at offset.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,
  KHE_MEET target_meet, int offset, KHE_MOVE_TYPE mt,
  bool preserve_regularity, bool *basic, KHE_KEMPE_STATS kempe_stats)
{
  bool success;  int d;
  KheEjectorRepairBegin(ej);
  success = KheTypedMeetMove(meet, target_meet, offset, mt,
    preserve_regularity, &d, basic, kempe_stats);
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cMeetMove(", success ? '+' : '-');
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(target_meet, 1, -1, stderr);
    fprintf(stderr, ", %d, %s, %s)", offset, KheMoveTypeShow(mt),
      preserve_regularity ? "true":"false");
  }
  return KheEjectorRepairEnd(ej, KheMoveTypeToMeetRepairType(mt), success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFuzzyMeetMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,               */
/*    KHE_MEET target_meet, int offset)                                      */
/*                                                                           */
/*  Try a fuzzy move (KHE_REPAIR_FUZZY_MEET_MOVE) of meet to target_meet     */
/*  at offset.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheFuzzyMeetMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,
  KHE_MEET target_meet, int offset)
{
  bool success;
  HnAssert(KheMeetAsst(meet) != NULL, "KheFuzzyMeetMoveRepair internal error");
  KheEjectorRepairBegin(ej);
  success = KheFuzzyMeetMove(meet, target_meet, offset, 4, 2, 12);
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cFuzzyMeetMove(", success ? '+' : '-');
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(target_meet, 1, -1, stderr);
    fprintf(stderr, ", %d)", offset);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_FUZZY_MEET_MOVE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeMeetSwapRepair(KHE_EJECTOR ej, KHE_NODE node1,KHE_NODE node2)*/
/*                                                                           */
/*  Try a node swap (repair KHE_REPAIR_NODE_MEET_SWAP) of node1 and node2.   */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeMeetSwapRepair(KHE_EJECTOR ej, KHE_NODE node1,KHE_NODE node2)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheNodeMeetSwap(node1, node2);
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cNodeMeetSwap(", success ? '+' : '-');
    KheNodeDebug(node1, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheNodeDebug(node2, 1, -1, stderr);
    fprintf(stderr, ")");
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_NODE_MEET_SWAP, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeSwapToSimilarNodeMultiRepair(KHE_EJECTOR ej, KHE_NODE node)  */
/*                                                                           */
/*  Try swapping node with other nodes that it shares a layer with.          */
/*  The node is known to be not visited.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeSwapToSimilarNodeMultiRepair(KHE_EJECTOR ej, KHE_NODE node)
{
  KHE_LAYER layer;  KHE_NODE node2;  int i, index, random_offset;
  if( KheNodeParentLayerCount(node) > 0 && !KheNodeVisited(node, 0) )
  {
    KheNodeVisit(node);
    layer = KheNodeParentLayer(node, 0);
    random_offset = 13 * KheEjectorCurrAugmentCount(ej) +
      31 * KheSolnDiversifier(KheEjectorSoln(ej));
    for( i = 0;  i < KheLayerChildNodeCount(layer);  i++ )
    {
      index = (random_offset + i) % KheLayerChildNodeCount(layer);
      node2 = KheLayerChildNode(layer, index);
      if( node2 != node && KheNodeSameParentLayers(node, node2) &&
	  KheNodeMeetSwapRepair(ej, node, node2) )
	return true;
      if( KheEjectorCurrMayRevisit(ej) )
	KheNodeUnVisit(node);
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetSplitRepair(KHE_EJECTOR ej, KHE_MEET meet, int duration1)    */
/*                                                                           */
/*  Try a meet split (KHE_REPAIR_MEET_SPLIT) of meet.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetSplitRepair(KHE_EJECTOR ej, KHE_MEET meet, int duration1)
{
  bool success;  KHE_MEET meet2;
  KheEjectorRepairBegin(ej);
  success = KheMeetSplit(meet, duration1, true, &meet, &meet2);
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cMeetSplit(", success ? '+' : '-');
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %d)", duration1);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_MEET_SPLIT, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitMoveMultiRepair(KHE_EJECTOR ej, KHE_MEET meet,              */
/*    KHE_MEET target_meet, int offset)                                      */
/*                                                                           */
/*  Try two split moves (KHE_REPAIR_SPLIT_MOVE) of meet to target_meet       */
/*  at offset.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitMoveMultiRepair(KHE_EJECTOR ej, KHE_MEET meet,
  KHE_MEET target_meet, int offset)
{
  bool success, /* split_success, */ basic;  int d, durn1;  KHE_MEET meet2;
  HnAssert(KheMeetAsst(meet) != NULL, "KheSplitMoveMultiRepair internal error");
  if( KheMeetDuration(meet) >= 2 )
  {
    /* try moving the first half of the split */
    durn1 = 1 + KheEjectorCurrAugmentCount(ej) % (KheMeetDuration(meet)-1);
    if( DEBUG11 )
    {
      fprintf(stderr, "[ KheSplitMoveMultiRepair splitting ");
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, " at %d\n", durn1);
    }
    KheEjectorRepairBegin(ej);
    success = KheMeetSplit(meet, durn1, true, &meet, &meet2) &&
      KheTypedMeetMove(meet, target_meet, offset, KHE_MOVE_KEMPE,
	false, &d, &basic, NULL);
    if( DEBUG11 || KheEjectorCurrDebug(ej) )
    {
      fprintf(stderr, "%cSplitMoveA(", success ? '+' : '-');
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, ", ");
      KheMeetDebug(target_meet, 1, -1, stderr);
      fprintf(stderr, ", %d)", offset);
    }
    /* ***
    if( DEBUG11 )
      fprintf(stderr, "  1 split_success: %s, success %s\n",
	split_success ? "true" : "false", success ? "true" : "false");
    *** */
    if( KheEjectorRepairEnd(ej, KHE_REPAIR_SPLIT_MOVE, success) )
    {
      if( DEBUG11 )
	fprintf(stderr, "] KheSplitMoveMultiRepair returning true\n");
      return true;
    }

    /* try moving the second half of the split */
    KheEjectorRepairBegin(ej);
    success = KheMeetSplit(meet, durn1, true, &meet, &meet2) &&
      KheTypedMeetMove(meet2, target_meet, offset, KHE_MOVE_KEMPE,
	false, &d, &basic, NULL);
    if( DEBUG11 || KheEjectorCurrDebug(ej) )
    {
      fprintf(stderr, "%cSplitMoveB(", success ? '+' : '-');
      KheMeetDebug(meet, 1, -1, stderr);
      fprintf(stderr, ", ");
      KheMeetDebug(target_meet, 1, -1, stderr);
      fprintf(stderr, ", %d)", offset);
    }
    /* ***
    if( DEBUG11 )
      fprintf(stderr, "  2 split_success: %s, success %s\n",
	split_success ? "true" : "false", success ? "true" : "false");
    *** */
    if( KheEjectorRepairEnd(ej, KHE_REPAIR_SPLIT_MOVE, success) )
    {
      if( DEBUG11 )
	fprintf(stderr, "] KheSplitMoveMultiRepair returning true\n");
      return true;
    }
    if( DEBUG11 )
      fprintf(stderr, "] KheSplitMoveMultiRepair returning false\n");
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMergeMoveRepair(KHE_EJECTOR ej, KHE_MEET meet1, KHE_MEET meet2,  */
/*    bool after)                                                            */
/*                                                                           */
/*  Try one merge move (KHE_REPAIR_MERGE_MOVE) which moves meet1 to          */
/*  before or after meet2, depending on the after parameter.  Both meets     */
/*  are currently assigned.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMergeMoveRepair(KHE_EJECTOR ej, KHE_MEET meet1, KHE_MEET meet2,
  bool after)
{
  bool success, b;  int d, offset;
  KheEjectorRepairBegin(ej);
  offset = (after ? KheMeetAsstOffset(meet2) + KheMeetDuration(meet2) :
    KheMeetAsstOffset(meet2) - KheMeetDuration(meet1));
  success = KheTypedMeetMove(meet1, KheMeetAsst(meet2), offset, KHE_MOVE_KEMPE,
      false, &d, &b, NULL) && KheMeetMerge(meet1, meet2, true, &meet1);
  if( DEBUG10 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cMergeMove(", success ? '+' : '-');
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", (meet2), %s)", after ? "after" : "before");
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_MERGE_MOVE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeIsVizier(KHE_NODE node)                                      */
/*                                                                           */
/*  Return true if node is a vizier node.  At present we are deciding        */
/*  this by seeing whether it has a parent and is its parent's sole child.   */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeIsVizier(KHE_NODE node)
{
  KHE_NODE parent_node;
  parent_node = KheNodeParent(node);
  return parent_node != NULL && KheNodeChildCount(parent_node) == 1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNodeIsSuitable(KHE_NODE node, bool try_vizier,                   */
/*    KHE_NODE limit_node)                                                   */
/*                                                                           */
/*  Return true if it is acceptable to change the assignments of the         */
/*  meets of node.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheNodeIsSuitable(KHE_NODE node, bool try_vizier,
  KHE_NODE limit_node)
{
  /* not acceptable if there is no parent node */
  if( KheNodeParent(node) == NULL )
    return false;

  /* not acceptable if not trying viziers and this is a vizier */
  if( !try_vizier && KheNodeIsVizier(node) )
    return false;

  /* not acceptable if there is a node limit and we have reached it */
  if( node == limit_node )
    return false;

  /* otherwise OK */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryKempe(KHE_MEET meet, KHE_OPTIONS_KEMPE kempe)                 */
/*                                                                           */
/*  Decide whether to try Kempe moves on meet.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheTryKempe(KHE_MEET meet, KHE_OPTIONS_KEMPE kempe)
{
  int i, layer_durn, max_layer_durn, cycle_durn;  KHE_NODE node;
  switch( kempe )
  {
    case KHE_OPTIONS_KEMPE_FALSE:

      return false;

    case KHE_OPTIONS_KEMPE_LARGE_LAYERS:

      node = KheMeetNode(meet);
      if( node != NULL && KheNodeParentLayerCount(node) > 0 )
      {
	max_layer_durn = 0;
	for( i = 0;  i < KheNodeParentLayerCount(node);  i++ )
	{
	  layer_durn = KheLayerDuration(KheNodeParentLayer(node, i));
	  if( layer_durn > max_layer_durn )
	    max_layer_durn = layer_durn;
	}
	cycle_durn = KheInstanceTimeCount(KheSolnInstance(KheMeetSoln(meet)));
	return 10 * max_layer_durn >= 8 * cycle_durn;
      }
      else
	return true;
      break;

    case KHE_OPTIONS_KEMPE_TRUE:

      return true;

    default:

      HnAbort("KheTryKempe internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMultiRepair(KHE_EJECTOR ej, KHE_MEET meet,                   */
/*    bool try_vizier, KHE_TRY_MEET_MOVE_FN try_meet_move_fn, void *impl,    */
/*    KHE_OPTIONS_KEMPE try_kempe, bool try_ejecting, bool try_basic,        */
/*    bool try_node_swaps)                                                   */
/*                                                                           */
/*  This wonderfully general multi-repair function tries to change the       */
/*  assignment of meet, or one of its ancestors, using meet assignments      */
/*  and moves of various kinds, and node swaps.  The other parameters        */
/*  offer detailed control over exactly what is tried, as follows:           */
/*                                                                           */
/*  try_vizier                                                               */
/*    If true, try assignments and moves of meets in vizier nodes as well    */
/*    as in other nodes.                                                     */
/*                                                                           */
/*  try_meet_move_fn                                                         */
/*    If this function returns true, try meet assignments and moves on       */
/*    meet, and on meet's proper ancestors if ancestors is true.   Exactly   */
/*    which meet assignments and moves depends on the next 3 parameters.     */
/*    If the function is NULL, no time is needed and we try everything.      */
/*                                                                           */
/*  try_kempe                                                                */
/*    If KHE_OPTIONS_KEMPE_FALSE, don't try Kempe moves.                     */
/*    If KHE_OPTIONS_KEMPE_LARGE_LAYERS, try Kempe moves when the meet lies  */
/*    in a layer of relatively large duration.                               */
/*    If KHE_OPTIONS_KEMPE_TRUE, try Kempe moves.                            */
/*                                                                           */
/*  try_ejecting                                                             */
/*    If true, try ejecting assignments or moves (depending on whether meet  */
/*    is unassigned or assigned) when try_meet_move_fn returns true.         */
/*                                                                           */
/*  try_basic                                                                */
/*    If true, try basic assignments or moves (depending on whether meet     */
/*    is unassigned or assigned) when try_meet_move_fn returns true.         */
/*                                                                           */
/*  try_node_swaps                                                           */
/*    If true, try node swaps on meet's node (if any), and on meet's         */
/*    proper ancestors' nodes (if any) if ancestors is true.                 */
/*                                                                           */
/*  Kempe meet moves are always tried before basic and ejecting moves,       */
/*  and basic and ejecting moves are skipped if a corresponding Kempe        */
/*  move was tried and indicated (via its "basic" parameter) that it         */
/*  did what a basic or ejecting move would do.                              */
/*                                                                           */
/*  Obsolete (?):  This function also consults the time_limit_node option    */
/*  to ensure that any meets whose assignments it changes lie in the limit.  */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MEET meet, bool try_vizier, KHE_TRY_MEET_MOVE_FN try_meet_move_fn,
  void *impl, KHE_OPTIONS_KEMPE try_kempe, bool try_ejecting,
  bool try_basic, bool try_node_swaps)
{
  int base, i, max_offset, offset, offs, index, random_offset;
  KHE_MEET anc_meet, target_meet;
  KHE_NODE node, limit_node, parent_node;  KHE_TIME time;
  bool basic, try_meet_moves;
  /* bool nodes_before_meets = KheEjectorNodesBeforeMeets(ej); */
  /* bool try_fuzzy_moves = KheEjectorUseFuzzyMoves(ej); */
  /* bool try_split_moves = KheEjectorUseSplitMoves(ej); */
  /* KHE_KEMPE_STATS kempe_stats = KheEjectorKempeStats(ej); */
  if( DEBUG4 )
  {
    fprintf(stderr, "%*s[ KheMeetMultiRepair(",
      KheEjectorCurrDebugIndent(ej) + 2, "");
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* follow the chain upwards to the first ancestor lying in a node */
  base = 0;
  anc_meet = meet;
  while( anc_meet != NULL && KheMeetNode(anc_meet) == NULL )
  {
    base += KheMeetAsstOffset(anc_meet);
    anc_meet = KheMeetAsst(anc_meet);
  }
  if( anc_meet == NULL )
  {
    if( DEBUG4 )
      fprintf(stderr,"%*s] KheMeetMultiRepair returning false (no movable "
        "ancestor)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
    return false;
  }

  /* make sure anc_meet's node is in scope */
  node = KheMeetNode(anc_meet);
  /* limit_node = KheEjectorLimitNode(ej); */
  limit_node = ao->limit_node;
  if( limit_node != NULL && !KheNodeIsProperDescendant(node, limit_node) )
  {
    if( DEBUG4 )
      fprintf(stderr,"%*s] KheMeetMultiRepair ret false (not in node scope)\n",
	KheEjectorCurrDebugIndent(ej) + 2, "");
    return false;
  }

  /* try repairs at each ancestor of meet lying in a suitable node */
  try_meet_moves = (try_kempe != KHE_OPTIONS_KEMPE_FALSE) ||
    try_ejecting || try_basic;
  while( anc_meet != NULL )
  {
    node = KheMeetNode(anc_meet);
    if( node != NULL && KheNodeIsSuitable(node, try_vizier, limit_node) )
    {
      /* if nodes before meets, try node swaps of anc_meet's node */
      if( ao->nodes_before_meets && try_node_swaps &&
	  KheNodeSwapToSimilarNodeMultiRepair(ej, node) )
      {
	if( DEBUG4 )
	  fprintf(stderr, "%*s] KheMeetMultiRepair ret true (node)\n",
	    KheEjectorCurrDebugIndent(ej) + 2, "");
	return true;
      }

      /* try meet moves of anc_meet */
      if( try_meet_moves && !KheMeetVisited(anc_meet, 0) )
      {
	KheMeetVisit(anc_meet);
	parent_node = KheNodeParent(node);
	random_offset = 23 * KheEjectorCurrAugmentCount(ej) +
	  47 * KheSolnDiversifier(KheEjectorSoln(ej));
	for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
	{
	  index = (i + random_offset) % KheNodeMeetCount(parent_node);
	  target_meet = KheNodeMeet(parent_node, index);
	  time = KheMeetAsstTime(target_meet);
	  if( try_meet_move_fn == NULL || time != NULL )
	  {
	    max_offset = KheMeetDuration(target_meet)-KheMeetDuration(anc_meet);
	    for( offset = 0;  offset <= max_offset;  offset++ )
	    {
	      offs = (offset + random_offset) % (max_offset + 1);
	      if( try_meet_move_fn == NULL || try_meet_move_fn(meet,
		    KheTimeNeighbour(time, base + offs), impl) )
	      {
		if( KheMeetAsst(anc_meet) == NULL )
		{
		  /* meet assignments (not moves) are allowed; try ejecting */
		  basic = false;
		  if( try_ejecting && KheMeetMoveRepair(ej, anc_meet,
		      target_meet, offs, KHE_MOVE_EJECTING, true, &basic,NULL) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,
			"%*s] KheMeetMultiRepair ret true (ejecting asst)\n",
			KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }

		  /* try basic, unless already tried by ejecting */
		  if( !basic && try_basic && KheMeetMoveRepair(ej, anc_meet,
		      target_meet, offs, KHE_MOVE_UNCHECKED, true,&basic,NULL) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,
			"%*s] KheMeetMultiRepair ret true (basic asst)\n",
			KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }
		}
		else
		{
		  /* meet moves (not assignments) are allowed; try kempe */
		  basic = false;
		  if( KheTryKempe(anc_meet, try_kempe) &&
		      KheMeetMoveRepair(ej, anc_meet, target_meet,
			offs, KHE_MOVE_KEMPE, true, &basic, ao->kempe_stats) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr, "%*s] KheMeetMultiRepair returning true "
			"(kempe)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }

		  /* try ejecting, unless already tried by kempe */
		  if( !basic && try_ejecting &&
		      KheMeetMoveRepair(ej, anc_meet, target_meet, offs,
			KHE_MOVE_EJECTING, true, &basic, NULL) )
		  {
		    if( DEBUG4 )
		     fprintf(stderr, "%*s] KheMeetMultiRepair returning true "
			"(ejecting)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }

		  /* try basic, unless already tried by kempe or ejecting */
		  if( !basic && try_basic && KheMeetMoveRepair(ej, anc_meet,
		      target_meet, offs, KHE_MOVE_UNCHECKED, true,&basic,NULL) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(basic)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }

		  /* try fuzzy, if allowed and depth is 1 */
		  if( ao->use_fuzzy_moves && KheEjectorCurrLength(ej) == 1 &&
		      KheFuzzyMeetMoveRepair(ej, meet, target_meet, offs) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(fuzzy)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }

		  /* try split, if allowed and depth is 1 */
		  if( ao->use_split_moves && KheEjectorCurrLength(ej) == 1 &&
		      KheSplitMoveMultiRepair(ej, meet, target_meet, offs) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(split)\n", KheEjectorCurrDebugIndent(ej) + 2, "");
		    return true;
		  }
		}
	      }
	    }
	  }
	}
	if( KheEjectorCurrMayRevisit(ej) )
	  KheMeetUnVisit(anc_meet);
      }

      /* if nodes after meets, try node swaps of anc_meet's node */
      if( !ao->nodes_before_meets && try_node_swaps &&
	  KheNodeSwapToSimilarNodeMultiRepair(ej, node) )
      {
	if( DEBUG4 )
	  fprintf(stderr, "%*s] KheMeetMultiRepair returning true (node)\n",
	    KheEjectorCurrDebugIndent(ej) + 2, "");
	return true;
      }
    }
    base += KheMeetAsstOffset(anc_meet);
    anc_meet = KheMeetAsst(anc_meet);
  }
  if( DEBUG4 )
    fprintf(stderr,"%*s] KheMeetMultiRepair returning false\n",
      KheEjectorCurrDebugIndent(ej) + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened task set move and double move repairs and augments"   */
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
/*  bool KheWidenedTaskSetMoveRepair(KHE_EJECTOR ej,                         */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int before, int after, int *from_r_durn_change, int *to_r_durn_change) */
/*                                                                           */
/*  Try a repair that moves wts from from_r to to_r.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetMoveRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetMove(wts, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetMove(", success ? '+' : '-');
    KheWidenedTaskSetMoveDebug(wts, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalMoveRepair(KHE_EJECTOR ej,                  */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Like KheWidenedTaskSetMoveRepair, except moving wts optimally.           */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetOptimalMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetOptimalMove(wts, to_r,
    from_r_durn_change, to_r_durn_change);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetOptimalMove(", success ? '+' : '-');
    KheWidenedTaskSetOptimalMoveDebug(wts, to_r, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetDoubleMoveRepair(KHE_EJECTOR ej,                   */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r,                                */
/*    KHE_WIDENED_TASK_SET wts1, int before, int after,                      */
/*    KHE_WIDENED_TASK_SET wts2, int first_index, int last_index)            */
/*                                                                           */
/*  Try a repair that moves wts1 with wings before and after from from_r to  */
/*  to_r, and wts2's core (first_index to last_index) from to_r to from_r.   */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetDoubleMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, int before, int after,
  KHE_WIDENED_TASK_SET wts2, int first_index, int last_index)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetMove(wts1, to_r, before, after,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedTaskSetMovePartial(wts2, from_r, first_index, last_index);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetDoubleMovePartial(", success ? '+' : '-');
    KheWidenedTaskSetMoveDebug(wts1, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedTaskSetMovePartialDebug(wts2, from_r, first_index, last_index,
      2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalDoubleMoveRepair(KHE_EJECTOR ej,            */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_WIDENED_TASK_SET wts1,     */
/*    KHE_WIDENED_TASK_SET wts2, int first_index, int last_index)            */
/*                                                                           */
/*  Like KheWidenedTaskSetDoubleMoveRepair, except that the move of          */
/*  wts1 from from_r to to_r is optimal.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetOptimalDoubleMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, KHE_WIDENED_TASK_SET wts2,
  int first_index, int last_index)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetOptimalMove(wts1, to_r,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedTaskSetMovePartial(wts2, from_r, first_index, last_index);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetOptimalDoubleMovePartial(",
      success ? '+' : '-');
    KheWidenedTaskSetOptimalMoveDebug(wts1, to_r, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedTaskSetMovePartialDebug(wts2, from_r, first_index, last_index,
      2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetDoubleMoveWholeRepair(KHE_EJECTOR ej,              */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r,                                */
/*    KHE_WIDENED_TASK_SET wts1, int before, int after,                      */
/*    KHE_WIDENED_TASK_SET wts2)                                             */
/*                                                                           */
/*  Try a repair that moves wts1 with wings before and after from from_r to  */
/*  to_r, and wts2's core (first_index to last_index) from to_r to from_r.   */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetDoubleMoveWholeRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, int before, int after,
  KHE_WIDENED_TASK_SET wts2)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetMove(wts1, to_r, before, after,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedTaskSetMove(wts2, from_r, 0, 0,
      &from_r_durn_change, &to_r_durn_change);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetDoubleMoveWhole(", success ? '+' : '-');
    KheWidenedTaskSetMoveDebug(wts1, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedTaskSetMoveDebug(wts2, from_r, 0, 0, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalDoubleMoveWholeRepair(KHE_EJECTOR ej,       */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_WIDENED_TASK_SET wts1,     */
/*    KHE_WIDENED_TASK_SET wts2)                                             */
/*                                                                           */
/*  Like KheWidenedTaskSetDoubleMoveWholeRepair, except that the move of     */
/*  wts1 from from_r to to_r is optimal.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetOptimalDoubleMoveWholeRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, KHE_WIDENED_TASK_SET wts2)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetOptimalMove(wts1, to_r,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedTaskSetMove(wts2, from_r, 0, 0,
      &from_r_durn_change, &to_r_durn_change);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetOptimalDoubleMoveWhole(",
      success ? '+' : '-');
    KheWidenedTaskSetOptimalMoveDebug(wts1, to_r, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedTaskSetMoveDebug(wts2, from_r, 0, 0, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetSwapRepair(KHE_EJECTOR ej,                         */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int before, int after, int *from_r_durn_change, int *to_r_durn_change) */
/*                                                                           */
/*  Try a repair that swaps wts between from_r and to_r.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetSwap(wts, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( DEBUG18 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedTaskSetSwap(", success ? '+' : '-');
    KheWidenedTaskSetSwapDebug(wts, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryRuns(KHE_EJECTOR ej, KHE_WIDENED_TASK_SET wts1,               */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int before, int after,         */
/*    KHE_WIDENED_TASK_SET wts2, int wanted_durn, int *attempts,             */
/*    int *first_index, int *last_index)                                     */
/*                                                                           */
/*  Helper function that tries one or two double move repairs, depending     */
/*  on how the duration of wts2 compares with wanted_durn.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTryRuns(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, int before, int after,
  KHE_WIDENED_TASK_SET wts2, int wanted_durn,
  int *attempts, int *first_index, int *last_index)
{
  int actual_durn, fi, li;
  KheWidenedTaskSetInterval(wts2, 0, 0, first_index, last_index);
  actual_durn = *last_index - *first_index + 1;
  if( actual_durn > wanted_durn )
  {
    if( RANDOMIZED )
    {
      int random_offset = 13 * KheEjectorCurrAugmentCount(ej) +
	31 * KheSolnDiversifier(KheEjectorSoln(ej));
      if( random_offset % 2 == 0 )
      {
	/* try left end */
	if( KheWidenedTaskSetFindInitial(wts2, wanted_durn, &fi, &li) &&
	    KheWidenedTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wts1,
	      before, after, wts2, fi, li) )
	{
	  KheWidenedTaskSetDelete(wts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedTaskSetDelete(wts2);
	return false;
      }
      else
      {
	/* try right end */
	if( KheWidenedTaskSetFindFinal(wts2, wanted_durn, &fi, &li) &&
	    KheWidenedTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wts1,
	      before, after, wts2, fi, li) )
	{
	  KheWidenedTaskSetDelete(wts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedTaskSetDelete(wts2);
	return false;
      }
    }
    else
    {
      /* try each end of wts2 */
      if( KheWidenedTaskSetFindInitial(wts2, wanted_durn, &fi, &li) &&
	  KheWidenedTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wts1,
	    before, after, wts2, fi, li) )
      {
	KheWidenedTaskSetDelete(wts2);
	return true;
      }
      if( KheWidenedTaskSetFindFinal(wts2, wanted_durn, &fi, &li) &&
	  KheWidenedTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wts1,
	    before, after, wts2, fi, li) )
      {
	KheWidenedTaskSetDelete(wts2);
	return true;
      }
      *attempts += 2;
    }
  }
  else if( actual_durn == wanted_durn )
  {
    /* try whole of wts2 */
    if( KheWidenedTaskSetDoubleMoveWholeRepair(ej, ao, from_r, to_r, wts1,
	before, after, wts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedTaskSetDelete(wts2);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryOptimalRuns(KHE_EJECTOR ej, KHE_RESOURCE from_r,              */
/*    KHE_RESOURCE to_r, KHE_WIDENED_TASK_SET wts1,KHE_WIDENED_TASK_SET wts2,*/
/*    int wanted_durn, int *attempts, int *first_index, int *last_index)     */
/*                                                                           */
/*  Like KheTryRuns except making an optimal move of wts1.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTryOptimalRuns(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_TASK_SET wts1, KHE_WIDENED_TASK_SET wts2,
  int wanted_durn, int *attempts, int *first_index, int *last_index)
{
  int actual_durn, fi, li;
  KheWidenedTaskSetInterval(wts2, 0, 0, first_index, last_index);
  actual_durn = *last_index - *first_index + 1;
  if( actual_durn > wanted_durn )
  {
    if( RANDOMIZED )
    {
      int random_offset = 13 * KheEjectorCurrAugmentCount(ej) +
	31 * KheSolnDiversifier(KheEjectorSoln(ej));
      if( random_offset % 2 == 0 )
      {
	/* try left end */
	if( KheWidenedTaskSetFindInitial(wts2, wanted_durn, &fi, &li) &&
	    KheWidenedTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	      to_r, wts1, wts2, fi, li) )
	{
	  KheWidenedTaskSetDelete(wts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedTaskSetDelete(wts2);
	return false;
      }
      else
      {
	/* try right end */
	if( KheWidenedTaskSetFindFinal(wts2, wanted_durn, &fi, &li) &&
	    KheWidenedTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	      to_r, wts1, wts2, fi, li) )
	{
	  KheWidenedTaskSetDelete(wts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedTaskSetDelete(wts2);
	return false;
      }
    }
    else
    {
      /* try each end of wts2 */
      if( KheWidenedTaskSetFindInitial(wts2, wanted_durn, &fi, &li) &&
	  KheWidenedTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	    to_r, wts1, wts2, fi, li) )
      {
	KheWidenedTaskSetDelete(wts2);
	return true;
      }
      if( KheWidenedTaskSetFindFinal(wts2, wanted_durn, &fi, &li) &&
	  KheWidenedTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	    to_r, wts1, wts2, fi, li) )
      {
	KheWidenedTaskSetDelete(wts2);
	return true;
      }
      *attempts += 2;
    }
  }
  else if( actual_durn == wanted_durn )
  {
    /* try whole of wts2 */
    if( KheWidenedTaskSetOptimalDoubleMoveWholeRepair(ej, ao, from_r, to_r,
	  wts1, wts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedTaskSetDelete(wts2);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMoveAndDoubleMovesOptimized(KHE_EJECTOR ej,        */
/*    KHE_RESOURCE_TYPE rt, KHE_TASK_FINDER tf, KHE_WIDENED_TASK_SET wts,    */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_EXPANSION_OPTIONS eo)      */
/*                                                                           */
/*  Try repairs that move wts's core, the first before elements of its left  */
/*  wing, and the first after elements of its right wing.  The first repair  */
/*  moves these tasks to to_r; the rest are double moves which also move     */
/*  tasks the other way, so as not to change the overall workload of to_r.   */
/*                                                                           */
/*  If eo->balancing_off is true, or to_r is NULL, don't do double moves.    */
/*  We make up to eo->balancing_max attempts to repair using double moves,   */
/*  although we may overrun this limit by up to one attempt.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetMoveAndDoubleMovesOptimized(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;
  int prev_first_index, foll_last_index, index;
  bool prev_active, foll_active;  KHE_WIDENED_TASK_SET wts2;

  /* try an optimal move */
  if( KheWidenedTaskSetOptimalMoveRepair(ej, ao, wts, from_r, to_r,
        &from_r_durn_change, &to_r_durn_change) )
    return true;

  /* return if not trying double moves, or no change in to_r's durn */
  if( ao->balancing_off || ao->balancing_max == 0 || /* to_r == NULL || */
      to_r_durn_change == 0 )
    return false;

  if( RANDOMIZED )
  {
    int random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
      53 * KheSolnDiversifier(KheEjectorSoln(ej));
    if( random_offset % 2 == 0 )
      return false;
  }

  /* boilerplate */
  KheWidenedTaskSetFullInterval(wts, &prev_first_index, &foll_last_index);

  /* debug print of balancing */
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  /* make up to eo->balancing_max attempts at a balanced double move */
  attempts = 0;
  prev_active = foll_active = true;
  while( prev_active || foll_active )
  {
    if( foll_active )
    {
      foll_active = KheFindMovableWidenedTaskSetRight(ao->task_finder, to_r,
	from_r, foll_last_index + 1, &wts2);
      if( foll_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found right run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedTaskSetDebug(wts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryOptimalRuns(ej, ao, from_r, to_r, wts, wts2, to_r_durn_change,
	      &attempts, &index, &foll_last_index) )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] balancing success\n",
	      KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
	if( RANDOMIZED )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
	  return false;
	}
	if( attempts >= ao->balancing_max )  break;
      }
    }
    if( prev_active )
    {
      prev_active = KheFindMovableWidenedTaskSetLeft(ao->task_finder,
	to_r, from_r, prev_first_index - 1, &wts2);
      if( prev_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found left run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedTaskSetDebug(wts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryOptimalRuns(ej, ao, from_r, to_r, wts, wts2,
	      to_r_durn_change, &attempts, &prev_first_index, &index) )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] balancing success\n",
	      KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
	if( RANDOMIZED )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
	  return false;
	}
	if( attempts >= ao->balancing_max )  break;
      }
    }
  }
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMoveAndDoubleMoves(KHE_EJECTOR ej,                 */
/*    KHE_RESOURCE_TYPE rt, KHE_TASK_FINDER tf, KHE_WIDENED_TASK_SET wts,    */
/*    int before, int after, KHE_RESOURCE from_r, KHE_RESOURCE to_r,         */
/*    KHE_EXPANSION_OPTIONS eo)                                              */
/*                                                                           */
/*  Try repairs that move wts's core, the first before elements of its left  */
/*  wing, and the first after elements of its right wing.  The first repair  */
/*  moves these tasks to to_r; the rest are double moves which also move     */
/*  tasks the other way, so as not to change the overall workload of to_r.   */
/*                                                                           */
/*  If eo->balancing_off is true, or to_r is NULL, don't do double moves.    */
/*  We make up to eo->balancing_max attempts to repair using double moves,   */
/*  although we may overrun this limit by up to one attempt.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheWidenedTaskSetMoveAndDoubleMoves(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_TASK_SET wts,
  int before, int after, KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;
  int prev_first_index, foll_last_index, index;
  bool prev_active, foll_active;  KHE_WIDENED_TASK_SET wts2;

  /* try a move */
  if( KheWidenedTaskSetMoveRepair(ej, ao, wts, from_r, to_r, before, after,
	&from_r_durn_change, &to_r_durn_change) )
    return true;
  if( RANDOMIZED )
  {
    int random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
      53 * KheSolnDiversifier(KheEjectorSoln(ej));
    if( random_offset % 2 == 0 )
      return false;
  }

  /* return if not trying double moves, or no change in to_r's durn */
  if( ao->balancing_off || ao->balancing_max == 0 || /* to_r == NULL || */
      to_r_durn_change == 0 )
    return false;

  /* boilerplate */
  KheWidenedTaskSetInterval(wts, before, after, &prev_first_index,
    &foll_last_index);

  /* debug print of balancing */
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (max %d, durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", ao->balancing_max, to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  /* make up to eo->balancing_max attempts at a balanced double move */
  attempts = 0;
  prev_active = foll_active = true;
  while( prev_active || foll_active )
  {
    if( foll_active )
    {
      foll_active = KheFindMovableWidenedTaskSetRight(ao->task_finder,
	to_r, from_r, foll_last_index + 1, &wts2);
      if( foll_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found right run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedTaskSetDebug(wts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryRuns(ej, ao, from_r, to_r, wts, before, after, wts2,
	      to_r_durn_change, &attempts, &index, &foll_last_index) )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] balancing success\n",
	      KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
	if( RANDOMIZED )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
	  return false;
	}
	if( attempts >= ao->balancing_max )  break;
      }
    }
    if( prev_active )
    {
      prev_active = KheFindMovableWidenedTaskSetLeft(ao->task_finder,
	to_r, from_r, prev_first_index - 1, &wts2);
      if( prev_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found left run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedTaskSetDebug(wts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryRuns(ej, ao, from_r, to_r, wts, before, after, wts2,
	      to_r_durn_change, &attempts, &prev_first_index, &index) )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] balancing success\n",
	      KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
	if( RANDOMIZED )
	{
	  if( DEBUG33 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
	  return false;
	}
	if( attempts >= ao->balancing_max )  break;
      }
    }
  }
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceBusyOnDayOfTime(KHE_RESOURCE_TIMETABLE_MONITOR rtm,      */
/*    KHE_FRAME days_frame, KHE_TIME t, KHE_TASK *task)                      */
/*                                                                           */
/*  If rtm's resource is busy on the day containing time t, return true      */
/*  and set *task to the proper root of a task that proves this fact.        */
/*  Otherwise return false and set *task to NULL.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceBusyOnDayOfTime(KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_FRAME days_frame, KHE_TIME t, KHE_TASK *task)
{
  int i, index;  KHE_TIME t2;  KHE_TIME_GROUP tg;
  index = KheFrameTimeIndex(days_frame, t);
  tg = KheFrameTimeGroup(days_frame, index);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t2 = KheTimeGroupTime(tg, i);
    if( KheResourceTimetableMonitorTimeTaskCount(rtm, t2) > 0 )
    {
      *task = KheResourceTimetableMonitorTimeTask(rtm, t2, 0);
      return *task = KheTaskProperRoot(*task), true;
    }
  }
  return *task = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetMoveMultiRepair(KHE_EJECTOR ej, KHE_TASK_SET from_r_ts,   */
/*    KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg, bool allow_unassign, */
/*    KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m,                    */
/*    KHE_EXPANSION_OPTIONS eo)                                              */
/*                                                                           */
/*  Try repairs that move the tasks of from_r_ts from whatever they are      */
/*  assigned to now to a resource to_r, which must lie in rg and not in      */
/*  not_rg (if not_rg != NULL).  If allow_unassign is true, the tasks        */
/*  may be unassigned as well as moved to a resource from rg.                */
/*                                                                           */
/*  Apply widening unless eo->widening_off is true, reversing unless         */
/*  eo->reversing_off is true, and balancing unless eo->balancing_off is     */
/*  true.  If balancing, make at most eo->balancing_max attempts.            */
/*                                                                           */
/*  If blocking_tg != NULL, block reversals that move tasks to blocking_tg.  */
/*  If blocking_m != NULL, block reversals that move tasks monitored by      */
/*  blocking_m.                                                              */
/*                                                                           */
/*  If the ejector allows meet moves, try them too, but without widening,    */
/*  reversing, or balancing.                                                 */
/*                                                                           */
/*  This function uses widened task sets, and the repairs it tries are       */
/*  widened task set move and swap operations.  There is also a double       */
/*  move operation which consists of two widened task set moves.             */
/*                                                                           */
/*****************************************************************************/
static bool KheMeetMoveAndTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_TASK task,
  KHE_RESOURCE r);

static bool KheTaskSetMoveMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK_SET from_r_ts, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m)
{
  struct khe_se_resource_set_type srs_rec;
  KHE_RESOURCE from_r, to_r;  KHE_TASK task;  KHE_SOLN soln;
  int from_r_ts_count, from_r_durn_change, to_r_durn_change;
  int random_offset1, random_offset2, random_offset3;
  int max_extra, max_before, max_after, before, after, lim_after;
  int first_index, last_index;  KHE_RESOURCE_TYPE rt;  KHE_WIDENED_TASK_SET wts;

  /* get tf and exit if not available */
  /* tf = KheEjectorTaskFinder(ej); */
  if( ao->task_finder == NULL )
    return false;
  soln = KheEjectorSoln(ej);

  /* make sure from_r_ts is non-empty and get from_r from its first task */
  from_r_ts_count = KheTaskSetTaskCount(from_r_ts);
  HnAssert(from_r_ts_count > 0, "KheTaskSetMoveMultiRepair internal error 1");
  task = KheTaskSetTask(from_r_ts, 0);
  rt = KheTaskResourceType(task);
  from_r = KheTaskAsstResource(task);

  if( ao->optimal_on )
  {
    /* make the widened task set */
    if( !KheWidenedTaskSetMakeFlexible(ao->task_finder, from_r, from_r_ts,
	  ao->optimal_width, &wts) )
      return false;
    max_extra = 0;  /* actually unused */
  }
  else
  {
    /* get max_extra */
    if( ao->widening_off )
    {
      max_extra = 0;
    }
    else
    {
      int from_r_ts_durn;
      KheTaskFinderTaskSetInterval(ao->task_finder, from_r_ts, &first_index,
	&last_index);
      from_r_ts_durn = last_index - first_index + 1;
      max_extra = max(ao->widening_max, from_r_ts_durn) - from_r_ts_durn;
    }

    /* make the widened task set */
    if( !KheWidenedTaskSetMake(ao->task_finder, from_r, from_r_ts,
	max_extra, max_extra, &wts) )
      return false;

  }

  /* for each resource to_r in rg, not in not_rg, plus possibly unassignment */
  if( DEBUG30 && KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%*sKheTaskSetMoveMultiRepair trying %sresource group:\n",
      KheEjectorCurrDebugIndent(ej), "", allow_unassign ? "unassign and " : "");
    KheResourceGroupDebug(rg, 2, KheEjectorCurrDebugIndent(ej), stderr);
    if( not_rg != NULL )
    {
      fprintf(stderr, "%*somitting resource group:\n",
	KheEjectorCurrDebugIndent(ej), "");
      KheResourceGroupDebug(not_rg, 2, KheEjectorCurrDebugIndent(ej), stderr);
    }
  }
  random_offset1 = 11 * KheEjectorCurrAugmentCount(ej) +
    53 * KheSolnDiversifier(soln);
  random_offset2 = 7 * KheEjectorCurrAugmentCount(ej) +
    37 * KheSolnDiversifier(soln);
  random_offset3 = 73 * KheEjectorCurrAugmentCount(ej) +
    5 * KheSolnDiversifier(soln);
  KheForEachResource(srs_rec, rg, not_rg, allow_unassign, random_offset1, to_r)
  {
    if( DEBUG30 && KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*sKheTaskSetMoveMultiRepair trying resource %s\n",
	KheEjectorCurrDebugIndent(ej), "", KheResourceShow(to_r));
    if( to_r != from_r )
    {
      if( ao->optimal_on )
      {
	if( KheWidenedTaskSetOptimalMoveCheck(wts, to_r, blocking_tg,blocking_m)
	    && KheWidenedTaskSetMoveAndDoubleMovesOptimized(ej, ao, rt, wts,
	      from_r, to_r) )
	{
	  KheWidenedTaskSetDelete(wts);
	  return true;
	}
      }
      else
      if( KheWidenedTaskSetMoveCheck(wts, to_r, false, &max_before,&max_after) )
      {
	/* try moving each possible number of tasks before and after */
	/* if( random_offset % 2 == 0 ) */
	if( true )
	{
	  /* shortest intervals first */
	  for( before = 0;  before <= max_before;  before++ )
	  {
	    lim_after = min(max_after, max_extra - before);
	    for( after = 0;  after <= lim_after;  after++ )
	    {
	      if( KheWidenedTaskSetMoveAndDoubleMoves(ej, ao, rt, wts,
		    (before + random_offset2) % (max_before + 1),
		    (after + random_offset3) % (lim_after + 1), from_r, to_r) )
	      {
		KheWidenedTaskSetDelete(wts);
		return true;
	      }
	      if( RANDOMIZED )
	      {
		KheWidenedTaskSetDelete(wts);
		return false;
	      }
	    }
	  }
	}
	else
	{
	  /* longest intervals first */
	  for( before = max_before;  before >= 0;  before-- )
	  {
	    lim_after = min(max_after, max_extra - before);
	    for( after = lim_after;  after >= 0;  after-- )
	    {
	      if( KheWidenedTaskSetMoveAndDoubleMoves(ej, ao, rt, wts,
		    (before + random_offset2) % (max_before + 1),
		    (after + random_offset3) % (lim_after + 1), from_r, to_r) )
	      {
		KheWidenedTaskSetDelete(wts);
		return true;
	      }
	      if( RANDOMIZED )
	      {
		KheWidenedTaskSetDelete(wts);
		return false;
	      }
	    }
	  }
	}
      }
      else if( !ao->reversing_off && KheWidenedTaskSetSwapCheck(wts, to_r,
	  true, blocking_tg, blocking_m, &max_before, &max_after) )
      {
	/* try exact swapping */
	for( before = 0;  before <= max_before;  before++ )
	{
	  lim_after = min(max_after, max_extra - before);
	  for( after = 0;  after <= lim_after;  after++ )
	  {
	    if( KheWidenedTaskSetSwapRepair(ej, ao, wts, from_r, to_r, before,
		  after, &from_r_durn_change, &to_r_durn_change) )
	    {
	      KheWidenedTaskSetDelete(wts);
	      return true;
	    }
	    if( RANDOMIZED )
	    {
	      KheWidenedTaskSetDelete(wts);
	      return false;
	    }
	  }
	}
      }
      /* *** forced moves - omit
      else if( KheWidenedTaskSetMoveCheck(wts, to_r, true,
	    &max_before, &max_after) )
      {
	** try forced moves, but don't widen them **
	if( KheWidenedTas kSetMoveAndDoubleMoves(ej, rt, tf, wts,
	      0, 0, from_r, to_r, eo) )
	{
	  KheWidenedTaskSetDelete(wts);
	  return true;
	}
      }
      *** */
	/* *** widened version
	for( before = 0;  before <= max_before;  before++ )
	  for( after=0; after<=max_after && before+after<=max_extra; after++ )
	    if( KheWidenedTas kSetMoveAndDoubleMoves(ej, rt, tf, wts,
		  before, after, from_r, to_r, eo, ts1, ts2) )
	    {
	      KheWidenedTaskSetDelete(wts);
	      KheTaskSetDelete(ts1);
	      KheTaskSetDelete(ts2);
	      return true;
	    }
	*** */
      if( RANDOMIZED )
      {
	KheWidenedTaskSetDelete(wts);
	return false;
      }
    }
  }

  /* delete the widened task set and scratch task sets we made earlier */
  KheWidenedTaskSetDelete(wts);

  /* try meet plus task moves, if permitted and just one task */
  /* if( KheEjectorRepairTimes(ej) && from_r_ts_count == 1 ) */
  if( ao->repair_times && from_r_ts_count == 1 )
  {
    random_offset1 = 29 * KheEjectorCurrAugmentCount(ej) +
      59 * KheSolnDiversifier(soln);
    KheForEachResource(srs_rec, rg, not_rg, allow_unassign,random_offset1,to_r)
      if( to_r != from_r )
      {
	if( KheMeetMoveAndTaskMoveMultiRepair(ej, ao, KHE_MOVE_KEMPE,
	      task, to_r) )
	  return true;
      }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetSwapToEndRepair(KHE_EJECTOR ej, KHE_TASK_SET ts,          */
/*    KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg)                      */
/*                                                                           */
/*  Repair function which swaps the timetables of r1 and r2, where r1        */
/*  is currently assigned ts and r2 is a resource of rg but not not_rg,      */
/*  from ts back to the start of the cycle, and from ts forward to the       */
/*  end of the cycle.                                                        */
/*                                                                           */
/*  Do nothing if ts is currently unassigned.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetSwapToEndRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK_SET ts, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  struct khe_se_resource_set_type srs_rec;  KHE_SOLN soln;
  KHE_RESOURCE r1, r2;  KHE_RESOURCE_TYPE rt;  KHE_WIDENED_TASK_SET wts;
  KHE_TASK_SET ts1;  int offset, ts_first, ts_last, x1, x2, final_index;

  /* make sure that ts is not empty; do nothing if it's unassigned */
  HnAssert(KheTaskSetTaskCount(ts) > 0,
    "KheTaskSetSwapToEndRepair internal error 1");
  r1 = KheTaskAsstResource(KheTaskSetTask(ts, 0));

  if( r1 != NULL )
  {
    /* boilerplate */
    if( DEBUG34 )
    {
      fprintf(stderr, "[ KheTaskSetSwapToEndRepair(ej, ");
      KheTaskSetDebug(ts, 2, -1, stderr);
      fprintf(stderr, " (assigned %s), ", KheResourceId(r1));
      KheResourceGroupDebug(rg, 1, -1, stderr);
      fprintf(stderr, ", ");
      if( not_rg != NULL )
	KheResourceGroupDebug(not_rg, 1, -1, stderr);
      else
	fprintf(stderr, "-");
      fprintf(stderr, ")\n");
    }
    rt = KheResourceResourceType(r1);
    /* tf = ao->task_finder; */
    /* tf = KheEjectorTaskFinder(ej); */
    final_index = KheTaskFinderLastIndex(ao->task_finder);
    soln = KheEjectorSoln(ej);
    ts1 = KheTaskSetMake(soln);
    KheTaskFinderTaskSetInterval(ao->task_finder, ts, &ts_first, &ts_last);

    /* get the tasks for r1 from the start to ts_last */
    KheFindTasksInInterval(ao->task_finder, 0, ts_last, rt, r1, false,
      false, ts1, &x1, &x2);
    if( KheWidenedTaskSetMake(ao->task_finder, r1, ts1, 0, 0, &wts) )
    {
      /* try swapping r1 and r2's timetables from start to ts_last */
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      KheForEachResource(srs_rec, rg, not_rg, false, offset, r2)
      {
	/* get the tasks for r2 from the start to ts, and try the move */
	if( r2 != r1 && KheWidenedTaskSetSwapCheck(wts, r2, false,
	    blocking_tg, blocking_d, &x1, &x2) &&
            KheWidenedTaskSetSwapRepair(ej, ao, wts, r1, r2, 0, 0, &x1, &x2) )
	{
	  KheTaskSetDelete(ts1);
	  if( DEBUG34 )
	    fprintf(stderr, "] KheTaskSetSwapToEndRepair returning true\n");
	  return true;
	}
      }
      KheWidenedTaskSetDelete(wts);
    }

    /* get the tasks for r1 from ts_first to the end */
    KheFindTasksInInterval(ao->task_finder, ts_first, final_index, rt,
      r1, false, false, ts1, &x1, &x2);
    if( KheWidenedTaskSetMake(ao->task_finder, r1, ts1, 0, 0, &wts) )
    {
      /* try swapping r1 and r2's timetables from ts_first to the end */
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      KheForEachResource(srs_rec, rg, not_rg, false, offset, r2)
      {
	/* get the tasks for r2 from ts to the end, and try the move */
	if( r2 != r1 && KheWidenedTaskSetSwapCheck(wts, r2, false,
	    blocking_tg, blocking_d, &x1, &x2) &&
            KheWidenedTaskSetSwapRepair(ej, ao, wts, r1, r2, 0, 0, &x1, &x2) )
	{
	  KheTaskSetDelete(ts1);
	  if( DEBUG34 )
	    fprintf(stderr, "] KheTaskSetSwapToEndRepair returning true\n");
	  return true;
	}
      }
      KheWidenedTaskSetDelete(wts);
    }

    /* no luck; tidy up and exit */
    KheTaskSetDelete(ts1);
  }
  if( DEBUG34 )
    fprintf(stderr, "] KheTaskSetSwapToEndRepair returning false\n");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetMoveAugment(KHE_EJECTOR ej, KHE_TASK_SET ts,              */
/*    KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg, bool allow_unassign, */
/*    KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d,                    */
/*    KHE_EXPANSION_OPTIONS eo)                                              */
/*                                                                           */
/*  Augment function which moves the tasks of ts from the resource they      */
/*  are all assigned to now to a different resource lying in rg, not         */
/*  lying in not_rg (if non-NULL).  The new resource may be NULL if          */
/*  allow_unassign is true.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSetMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK_SET ts, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  if( KheTaskSetTaskCount(ts) > 0 /* && !KheTaskSetAnyVisited(ts, 0) */ )
  {
    /* KheTaskSetVisitEquivalent(ts); */
    if( KheTaskSetMoveMultiRepair(ej, ao, ts, rg, not_rg, allow_unassign,
	  blocking_tg, blocking_d) )
      return true;
    if( KheEjectorCurrLength(ej) == 1 && ao->full_widening_on &&
	KheTaskSetSwapToEndRepair(ej, ao, ts, rg, not_rg, allow_unassign,
	  blocking_tg, blocking_d) )
      return true;
    /* ***
    if( KheEjectorCurrMayRevisit(ej) )
      KheTaskSetUnVisitEquivalent(ts);
    *** */
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskMoveAugment(KHE_EJECTOR ej, KHE_TASK task,                   */
/*    KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg, bool allow_unassign, */
/*    KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d,                    */
/*    KHE_EXPANSION_OPTIONS eo, KHE_TASK_SET scratch_ts)                     */
/*                                                                           */
/*  Like KheTaskSetMoveAugment but for a single task.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK task, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_d, KHE_TASK_SET scratch_ts)
{
  HnAssert(task == KheTaskProperRoot(task),
    "KheTaskMoveAugment internal error");
  KheTaskSetClear(scratch_ts);
  KheTaskSetAddTask(scratch_ts, task);
  if( KheTaskSetMoveMultiRepair(ej, ao, scratch_ts, rg, not_rg, allow_unassign,
	blocking_tg, blocking_d) )
    return true;
  if( KheEjectorCurrLength(ej) == 1 && ao->full_widening_on &&
      KheTaskSetSwapToEndRepair(ej, ao, scratch_ts, rg, not_rg, allow_unassign,
	blocking_tg, blocking_d) )
    return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "combined meet and task move and swap repairs and augments"    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveAndTaskMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,         */
/*    KHE_MEET target_meet, int offset, KHE_MOVE_TYPE meet_mt,               */
/*    KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE task_mt)                  */
/*                                                                           */
/*  Carry out a combined meet move and task move augment.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetMoveAndTaskMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MEET meet,
  KHE_MEET target_meet, int offset, KHE_MOVE_TYPE meet_mt,
  KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE task_mt)
{
  bool success, basic, move;  int d;  KHE_RESOURCE r2;  KHE_FRAME frame;
  if( KheTaskIsPreassigned(task, &r2) )
    return false;
  HnAssert(KheMeetAsst(meet) != NULL,
    "KheMeetMoveAndTaskMoveRepair internal error 1");
  KheEjectorRepairBegin(ej);
  move = (KheTaskAsst(task) != NULL);
  /* frame = KheEjectorFrame(ej); */
  frame = ao->frame;
  success = (move ? KheTaskUnAssign(task) : true) &&
    KheTypedMeetMove(meet, target_meet, offset, meet_mt, false, &d,&basic,NULL)
    && KheTypedTaskMoveFrame(task, r, task_mt, frame);
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cMeetMoveAndTaskMove(",
      success ? '+' : '-');
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(target_meet, 1, -1, stderr);
    fprintf(stderr, ", %d, meet_mt %s", offset, KheMoveTypeShow(meet_mt));
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %s, task_mt %s)", KheResourceId(r),
      KheMoveTypeShow(task_mt));
  }
  return KheEjectorRepairEnd(ej,
    move ? KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE :
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN, success);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveAndTaskMoveMultiRepair(KHE_EJECTOR ej,                   */
/*    KHE_MOVE_TYPE meet_mt, KHE_TASK task, KHE_RESOURCE r,                  */
/*    KHE_MOVE_TYPE task_mt)                                                 */
/*                                                                           */
/*  Try a move of type meet_mt of task's meet to somewhere else, and at the  */
/*  same time a move of type task_mt of task to r.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetMoveAndTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_TASK task,
  KHE_RESOURCE r)
{
  int max_offset, offset, base, i, junk, index, random_offset;
  KHE_MEET anc_meet, target_meet;  KHE_NODE node, parent_node;
  if( DEBUG5 )
  {
    fprintf(stderr, "%*s[ KheMeetMoveAndTaskMoveMultiRepair(",
      KheEjectorCurrDebugIndent(ej) + 2, "");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheResourceId(r));
  }
  anc_meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
  base = 0;
  while( anc_meet != NULL )
  {
    node = KheMeetNode(anc_meet);
    if( node != NULL && KheNodeParent(node) != NULL && !KheNodeIsVizier(node) )
    {
      parent_node = KheNodeParent(node);
      random_offset = 3 * KheEjectorCurrAugmentCount(ej) +
        7 * KheSolnDiversifier(KheEjectorSoln(ej));
      for( i = 0;  i < KheNodeMeetCount(parent_node);  i++ )
      {
	index = (random_offset + i ) % KheNodeMeetCount(parent_node);
	target_meet = KheNodeMeet(parent_node, index);
	max_offset = KheMeetDuration(target_meet)-KheMeetDuration(anc_meet);
	for( offset = 0;  offset <= max_offset;  offset++ )
	{
	  if( KheMeetMoveAndTaskMoveRepair(ej, ao, anc_meet, target_meet,
		offset, meet_mt, task, r, KHE_MOVE_EJECTING) )
	  {
	    if( DEBUG5 )
	      fprintf(stderr, "%*s] KheMeetMoveAndTaskMoveMultiRepair ret true\n",
		KheEjectorCurrDebugIndent(ej) + 2, "");
	    return true;
	  }
	}
      }
    }
    base += KheMeetAsstOffset(anc_meet);
    anc_meet = KheMeetAsst(anc_meet);
  }
  if( DEBUG5 )
    fprintf(stderr, "%*s] KheMeetMoveAndTaskMoveMultiRepair ret false\n",
      KheEjectorCurrDebugIndent(ej) + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource overload repairs"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDecreaseOverlapMeetMoveFn(KHE_MEET meet,                         */
/*    KHE_TIME time, void *impl)                                             */
/*                                                                           */
/*  Return true when moving meet to time would decrease the overlap with     */
/*  a time group, passed in impl.  This function assumes that meet is        */
/*  currently assigned a time.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheDecreaseOverlapMeetMoveFn(KHE_MEET meet,
  KHE_TIME time, void *impl)
{
  KHE_TIME_GROUP tg;  int curr_overlap, new_overlap, durn;
  tg = (KHE_TIME_GROUP) impl;
  durn = KheMeetDuration(meet);
  curr_overlap = KheTimeGroupOverlap(tg, KheMeetAsstTime(meet), durn);
  new_overlap = KheTimeGroupOverlap(tg, time, durn);
  return new_overlap < curr_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceOverloadMultiRepair(KHE_EJECTOR ej, KHE_RESOURCE r,      */
/*    KHE_TIME_GROUP tg, bool require_zero)                                  */
/*                                                                           */
/*  Carry out repairs which try to decrease the number of times that r is    */
/*  busy within time group tg.  If require_zero is true, only try repairs    */
/*  that will reduce this number to zero.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceOverloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg, bool require_zero)
{
  KHE_MEET meet;  KHE_TASK task;  KHE_RESOURCE_GROUP r_rg, domain;
  int i, tg_time_count, all_time_count, random_offset, ts_count;
  bool not_ejecting;  KHE_SOLN soln;  KHE_TASK_SET ts, scratch_ts;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  /* struct khe_expansion_options_rec eor; */

  /* find the set of tasks running during tg that are assigned r */
  soln = KheEjectorSoln(ej);
  r_rg = KheResourceSingletonResourceGroup(r);
  rtm = KheResourceTimetableMonitor(soln, r);
  ts = KheTaskSetMake(soln);
  KheResourceTimetableMonitorAddProperRootTasks(rtm, tg, false, ts);
  ts_count = KheTaskSetTaskCount(ts);
  if( DEBUG29 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheResourceOverloadMultiRepair(ej, %s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r),
      KheTimeGroupId(tg), require_zero ? "true" : "false");
  if( ts_count == 0 )
  {
    KheTaskSetDelete(ts);
    if( DEBUG29 && KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning false "
	"(ts)\n", KheEjectorCurrDebugIndent(ej), "");
    return false;
  }

  /* get expansion options */
  /* KheEjectorExpansionOptions(ej, &eor); */

  /* different repairs depending on whether require_zero is true */
  if( require_zero )
  {
    /* if( KheEjectorRepairResources(ej) && */
    if( ao->repair_resources &&
      !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(r)) )
    {
      /* require_zero, so try to move the whole task set */
      domain = KheTaskDomain(KheTaskSetTask(ts, 0));
      if( KheTaskSetMoveAugment(ej, ao, ts, domain, r_rg, true, tg, NULL) )
      {
	if( DEBUG29 && KheEjectorCurrDebug(ej) )
	  fprintf(stderr,"%*s] KheResourceOverloadMultiRepair returning true "
	    "(d)\n", KheEjectorCurrDebugIndent(ej), "");
	KheTaskSetDelete(ts);
	return true;
      }
    }
  }
  else
  {
    /* try ejecting task moves of individual tasks in ts away from r */
    /* if( KheEjectorRepairResources(ej) && */
    if( ao->repair_resources &&
	!KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(r)) )
    {
      random_offset = 19 * KheEjectorCurrAugmentCount(ej) +
	23 * KheSolnDiversifier(soln);
      scratch_ts = KheTaskSetMake(soln);
      for( i = 0;  i < ts_count;  i++ )
      {
	task = KheTaskSetTask(ts, (i + random_offset) % ts_count);
	domain = KheTaskDomain(task);
	if( KheTaskMoveAugment(ej, ao, task, domain, r_rg, true, tg, NULL,
	      scratch_ts) )
	{
	  if( DEBUG24 )
	    fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning "
	      "true (a)\n", KheEjectorCurrDebugIndent(ej), "");
	  KheTaskSetDelete(scratch_ts);
	  KheTaskSetDelete(ts);
	  return true;
	}
	if( RANDOMIZED )
	{
	  KheTaskSetDelete(scratch_ts);
	  KheTaskSetDelete(ts);
	  return false;
	}
      }
      KheTaskSetDelete(scratch_ts);
    }

    /* try ejecting meet moves of meets in tg away from tg */
    /* if( KheEjectorRepairTimes(ej) ) */
    if( ao->repair_times )
    {
      tg_time_count = KheTimeGroupTimeCount(tg);
      all_time_count = KheInstanceTimeCount(KheSolnInstance(soln));
      if( tg_time_count < all_time_count )  /* saves time with workload c's */
      {
	/* not_ejecting = KheEjectorBasicNotEjecting(ej); */
	not_ejecting = ao->no_ejecting_moves;
	random_offset = 17 * KheEjectorCurrAugmentCount(ej) +
	  43 * KheSolnDiversifier(soln);
	for( i = 0;  i < ts_count;  i++ )
	{
	  task = KheTaskSetTask(ts, (i + random_offset) % ts_count);
	  meet = KheTaskMeet(task);
	  if( KheMeetMultiRepair(ej, ao, meet, true,
	      &KheDecreaseOverlapMeetMoveFn, (void *) tg,
	      KHE_OPTIONS_KEMPE_FALSE, !not_ejecting, not_ejecting, false) )
	  {
	    if( DEBUG29 && KheEjectorCurrDebug(ej) )
	      fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning "
	        "true (c)\n", KheEjectorCurrDebugIndent(ej), "");
	    KheTaskSetDelete(ts);
	    return true;
	  }
	  if( RANDOMIZED )
	  {
	    KheTaskSetDelete(ts);
	    return false;
	  }
	}
      }
    }
  }
  if( DEBUG29 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning false\n",
      KheEjectorCurrDebugIndent(ej), "");
  KheTaskSetDelete(ts);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource underload repairs"                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheSolnComplementTimeGroup(KHE_SOLN soln,                 */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Return a time group that is the cycle minus tg.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheSolnComplementTimeGroup(KHE_SOLN soln,
  KHE_TIME_GROUP tg)
{
  KheSolnTimeGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  KheSolnTimeGroupDifference(soln, tg);
  return KheSolnTimeGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIncreaseOverlapMeetMoveFn(KHE_MEET meet,                         */
/*    KHE_TIME time, void *impl)                                             */
/*                                                                           */
/*  Return true when moving meet to time would increase the overlap with     */
/*  a time group, passed in impl.  This function assumes that meet is        */
/*  currently assigned a time.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheIncreaseOverlapMeetMoveFn(KHE_MEET meet,
  KHE_TIME time, void *impl)
{
  KHE_TIME_GROUP tg;  int curr_overlap, new_overlap, durn;
  tg = (KHE_TIME_GROUP) impl;
  durn = KheMeetDuration(meet);
  curr_overlap = KheTimeGroupOverlap(tg, KheMeetAsstTime(meet), durn);
  new_overlap = KheTimeGroupOverlap(tg, time, durn);
  return new_overlap > curr_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOneMeet(KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)   */
/*                                                                           */
/*  Return true if rtm contains exactly one meet overlapping tg.             */
/*                                                                           */
/*****************************************************************************/

static bool KheOneMeet(KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)
{
  int i, j, count;  KHE_TIME t;  KHE_MEET first_meet, meet;
  first_meet = NULL;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
    for( j = 0;  j < count;  j++ )
    {
      meet = KheTaskMeet(KheResourceTimetableMonitorTimeTask(rtm, t, j));
      if( first_meet == NULL )
	first_meet = meet;
      else if( meet != first_meet )
	return false;
    }
  }
  return first_meet != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskOverlapsTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)          */
/*                                                                           */
/*  Return true if task overlaps tg.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskOverlapsTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TIME t, t2;  int i, pos;
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      for( i = 0;  i < KheMeetDuration(meet);  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	if( KheTimeGroupContains(tg, t2, &pos) )
	  return true;
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGainTaskMultiRepair(KHE_EJECTOR ej, KHE_RESOURCE r,      */
/*    KHE_TIME_GROUP tg, bool force, KHE_TASK_SET scratch_ts)                */
/*                                                                           */
/*  Carry out augments which move a task to r within time group tg.  If      */
/*  force is true, do this even at times when r is already busy.             */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceGainTaskMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool force, KHE_TASK_SET scratch_ts)
{
  int i, j, k, index, random_offset, tg_time_count;  KHE_RESOURCE_TYPE rt;
  KHE_RESOURCE_GROUP r_rg;  KHE_TIME t;  KHE_TASK r_task, task, prev_task;
  KHE_MEET meet;  KHE_RESOURCE r2;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_FRAME days_frame;
  /* struct khe_expansion_options_rec eor; */  KHE_SOLN soln;

  /* boilerplate */
  soln = KheEjectorSoln(ej);
  /* KheEjectorExpansionOptions(ej, &eor); */
  r_rg = KheResourceSingletonResourceGroup(r);
  rt = KheResourceResourceType(r);
  rtm = KheResourceTimetableMonitor(soln, r);
  etm = ao->event_timetable_monitor;
  /* etm = KheEjectorEventTimetableMonitor(ej); */
  days_frame = ao->frame;
  /* days_frame = KheEjectorFrame(ej); */

  /* try each time of tg */
  tg_time_count = KheTimeGroupTimeCount(tg);
  random_offset = 37 * KheEjectorCurrAugmentCount(ej) +
    5 * KheSolnDiversifier(KheEjectorSoln(ej));;
  prev_task = NULL;
  for( i = 0;  i < tg_time_count;  i++ )
  {
    index = (random_offset + i) % tg_time_count;
    t = KheTimeGroupTime(tg, index);

    /* skip t if r is already busy on its day with a task overlapping tg */
    if( !force )
    {
      KheResourceBusyOnDayOfTime(rtm, days_frame, t, &r_task);
      if( r_task != NULL && (KheTaskIsPreassigned(r_task, &r2) ||
	  KheTaskOverlapsTimeGroup(r_task, tg)) )
	continue;
    }

    /* try to move a task to r at t */
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	if( KheTaskResourceType(task) == rt )
	{
	  task = KheTaskProperRoot(task);
	  if( KheTaskMoveResourceCheck(task, r) &&
	      (prev_task == NULL || !KheTaskEquiv(prev_task, task)) )
	  {
	    if( KheTaskMoveAugment(ej, ao, task, r_rg, NULL, false, NULL,
		  NULL, scratch_ts) )
	      return true;
	    prev_task = task;
	  }
	}
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceUnderloadMultiRepair(KHE_EJECTOR ej, KHE_RESOURCE r,     */
/*    KHE_TIME_GROUP tg, bool allow_zero)                                    */
/*                                                                           */
/*  Carry out augments which try to either increase or (if allow_zero is     */
/*  true) reduce to zero the number of times that r is busy within tg.       */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Here tg_index is the index of tg in d, if an index is needed to          */
/*  identify its position, and -1 otherwise.                                 */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Parameter d is the monitor whose augment function called this function.  */
/*  It was traditionally used for debugging only, but now it is being used   */
/*  to determine how many tasks should be assigned within one repair.        */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceUnderloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg, bool allow_zero)
{
  int i, tg_time_count, all_time_count, durn, random_offset, index;
  bool not_ejecting;  KHE_MEET meet;  KHE_TIME t;  KHE_SOLN soln;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK_SET scratch_ts;
  soln = KheEjectorSoln(ej);
  rtm = KheResourceTimetableMonitor(soln, r);
  tg_time_count = KheTimeGroupTimeCount(tg);

  /* if( KheEjectorRepairResources(ej) && */
  if( ao->repair_resources &&
      !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(r)) )
  {
    /* try clearing out tg altogether, if allow_zero */
    if( allow_zero && KheResourceOverloadMultiRepair(ej, ao, r, tg, true) )
      return true;

    /* for each time t of tg, try to make r busy at t */
    scratch_ts = KheTaskSetMake(soln);
    if( KheResourceGainTaskMultiRepair(ej, ao, r, tg, false, scratch_ts) )
    {
      KheTaskSetDelete(scratch_ts);
      return true;
    }
    KheTaskSetDelete(scratch_ts);
  }

  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    all_time_count = KheInstanceTimeCount(KheSolnInstance(soln));
    if( tg_time_count < all_time_count )
    {
      /* try ejecting meet moves into tg */
      /* NB finding tasks to move into tg, so might as well search whole list */
      random_offset = 7 * KheEjectorCurrAugmentCount(ej) +
	23 * KheSolnDiversifier(KheEjectorSoln(ej));
      /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
      not_ejecting = ao->no_ejecting_moves;
      for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
      {
	index = (random_offset + i) % KheResourceAssignedTaskCount(soln, r);
	meet = KheTaskMeet(KheResourceAssignedTask(soln, r, index));
	t = KheMeetAsstTime(meet);
	durn = KheMeetDuration(meet);
	if( t != NULL && KheTimeGroupOverlap(tg, t, durn) < durn &&
	  KheMeetMultiRepair(ej, ao, meet, false, &KheIncreaseOverlapMeetMoveFn,
	    (void *) tg, KHE_OPTIONS_KEMPE_TRUE, !not_ejecting, not_ejecting,
	    false) )
	  return true;
      }

      /* try clearing out tg altogether, using a cluster-like repair */
      if( allow_zero && (KheEjectorCurrLength(ej) == 1 || KheOneMeet(rtm, tg)) )
      {
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewGlobalVisit(soln);
	if( KheMeetBoundRepair(ej, r, KheSolnComplementTimeGroup(soln, tg)) )
	  return true;
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOverUnderMultiRepair(KHE_EJECTOR ej, KHE_RESOURCE r,             */
/*    KHE_TIME_GROUP tg, bool over, bool require_zero, bool allow_zero)      */
/*                                                                           */
/*  Call KheResourceOverloadMultiRepair(ej, r, tg, require_zero) if over is  */
/*  true, and KheResourceUnderloadMultiRepair(ej, r, tg, allow_zero) if      */
/*  over is false.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheOverUnderMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r, KHE_TIME_GROUP tg, bool over, bool require_zero,
  bool allow_zero)
{
  if( over )
    return KheResourceOverloadMultiRepair(ej, ao, r, tg, require_zero);
  else
    return KheResourceUnderloadMultiRepair(ej, ao, r, tg, allow_zero);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "split events augment functions"                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheSplitMonitorEvent(KHE_MONITOR d)                            */
/*                                                                           */
/*  Return the event monitored by split events or distribute split events    */
/*  monitor d.                                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_EVENT KheSplitMonitorEvent(KHE_MONITOR d)
{
  switch( KheMonitorTag(d) )
  {
    case KHE_SPLIT_EVENTS_MONITOR_TAG:

      return KheSplitEventsMonitorEvent((KHE_SPLIT_EVENTS_MONITOR) d);

    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      return KheDistributeSplitEventsMonitorEvent(
	(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR) d);

    default:

      HnAbort("KheSplitMonitorEvent internal error");
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnEventMeetsVisited(KHE_SOLN soln, KHE_EVENT e, int slack)     */
/*                                                                           */
/*  Like KheMeetVisited(meet, slack) only applied to all the meets of soln   */
/*  derived from e; it returns true if any of them are visited.              */
/*                                                                           */
/*****************************************************************************/

static bool KheSolnEventMeetsVisited(KHE_SOLN soln, KHE_EVENT e, int slack)
{
  int i;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    if( KheMeetVisited(KheEventMeet(soln, e, i), slack) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventMeetsVisit(KHE_SOLN soln, KHE_EVENT e)                  */
/*                                                                           */
/*  KheMeetVisit(meet) applied to all the meets of soln derived from e.      */
/*                                                                           */
/*****************************************************************************/

static void KheSolnEventMeetsVisit(KHE_SOLN soln, KHE_EVENT e)
{
  int i;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    KheMeetVisit(KheEventMeet(soln, e, i));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSolnEventMeetsUnVisit(KHE_SOLN soln, KHE_EVENT e)                */
/*                                                                           */
/*  KheMeetUnVisit(meet) applied to all the meets of soln derived from e.    */
/*                                                                           */
/*****************************************************************************/

static void KheSolnEventMeetsUnVisit(KHE_SOLN soln, KHE_EVENT e)
{
  int i;
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    KheMeetUnVisit(KheEventMeet(soln, e, i));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitEventsAugment(KHE_EJECTOR ej, KHE_MONITOR d)                */
/*                                                                           */
/*  Augment function for individual split events and distribute split        */
/*  events defects.                                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitEventsAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_SPLIT_ANALYSER sa;  int i, j, k, merged_durn, split1_durn, split2_durn;
  KHE_SOLN soln;  KHE_EVENT e;  KHE_MEET meet1, meet2;
  if( DEBUG4 )
  {
    fprintf(stderr, "%*s[ KheSplitEventsAugment(",
      KheEjectorCurrDebugIndent(ej) + 2, "");
    KheMonitorDebug(d, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  HnAssert(KheMonitorTag(d) == KHE_SPLIT_EVENTS_MONITOR_TAG ||
    KheMonitorTag(d) == KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG,
    "KheSplitEventsAugment internal error 1");
  /* if( KheEjectorUseSplitMoves(ej) ) */
  if( ao->use_split_moves )
  {
    soln = KheMonitorSoln(d);
    e = KheSplitMonitorEvent(d);
    if( !KheSolnEventMeetsVisited(soln, e, 0) )
    {
      /* get split and merge suggestions */
      sa = KheSplitAnalyserOption(KheEjectorOptions(ej), "ts_split_analyser",
	soln);
      /* ***
      sa = (KHE_SPLIT_ANALYSER) KheOptionsGetObject(KheEjectorOptions(ej),
	"ts_split_analyser", NULL);
      *** */
      /* sa = KheOptionsStructuralSplitAnalyser(KheEjectorOptions(ej)); */
      KheSolnEventMeetsVisit(soln, e);
      KheSplitAnalyserAnalyse(sa, e);
      if( DEBUG10 )
	KheSplitAnalyserDebug(sa, 1, KheEjectorCurrDebugIndent(ej) + 2, stderr);

      /* carry out sa's split suggestions */
      for( i = 0;  i < KheSplitAnalyserSplitSuggestionCount(sa);  i++ )
      {
	KheSplitAnalyserSplitSuggestion(sa, i, &merged_durn, &split1_durn);
	for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
	{
	  meet1 = KheEventMeet(soln, e, j);
	  if( KheMeetAsst(meet1)!=NULL && KheMeetDuration(meet1)==merged_durn )
	  {
            if( KheMeetSplitRepair(ej, meet1, split1_durn) )
	    {
	      if( DEBUG4 )
		fprintf(stderr, "%*s] KheSplitEventsAugment ret true (a)\n",
		  KheEjectorCurrDebugIndent(ej) + 2, "");
	      return true;
	    }
	  }
	}
      }

      /* carry out sa's merge suggestions */
      for( i = 0;  i < KheSplitAnalyserMergeSuggestionCount(sa);  i++ )
      {
	KheSplitAnalyserMergeSuggestion(sa, i, &split1_durn, &split2_durn);
	for( j = 0;  j < KheEventMeetCount(soln, e);  j++ )
	{
	  meet1 = KheEventMeet(soln, e, j);
	  if( KheMeetAsst(meet1)!=NULL && KheMeetDuration(meet1)==split1_durn )
	  {
	    for( k = j + 1;  k < KheEventMeetCount(soln, e);  k++ )
	    {
	      meet2 = KheEventMeet(soln, e, k);
	      if( KheMeetAsst(meet2) != NULL &&
		  KheMeetDuration(meet2) == split2_durn )
	      {
		if( KheMergeMoveRepair(ej, meet1, meet2, false) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej) + 2, "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet1, meet2, true) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej) + 2, "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet2, meet1, false) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej) + 2, "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet2, meet1, true) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej) + 2, "");
		  return true;
		}
	      }
	    }
	  }
	}
      }
      if( KheEjectorCurrMayRevisit(ej) )
	KheSolnEventMeetsUnVisit(soln, e);
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "%*s] KheSplitEventsAugment ret false\n",
      KheEjectorCurrDebugIndent(ej) + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitEventsGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)           */
/*                                                                           */
/*  Augment function for groups of split events and distribute split events  */
/*  defects.                                                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheSplitEventsGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheSplitEventsGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheSplitEventsGroupAugment internal error 2");
  return KheSplitEventsAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assign time monitor augment functions"                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDebugAugmentBegin(bool debug, KHE_EJECTOR ej, KHE_MONITOR d,     */
/*    char *str)                                                             */
/*  void KheDebugAugmentEnd(bool debug, KHE_EJECTOR ej, KHE_MONITOR d,       */
/*    char *str, bool res)                                                   */
/*                                                                           */
/*  Debug functions to call at the start and end of each augment function.   */
/*                                                                           */
/*****************************************************************************/

static void KheDebugAugmentBegin(bool debug, KHE_EJECTOR ej, KHE_MONITOR d,
  char *str)
{
  if( debug )
  {
    fprintf(stderr, "%*s[ %s(", KheEjectorCurrDebugIndent(ej) + 2, "", str);
    KheMonitorDebug(d, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
}

static void KheDebugAugmentEnd(bool debug, KHE_EJECTOR ej, KHE_MONITOR d,
  char *str, bool res)
{
  if( debug )
    fprintf(stderr, "%*s] %s ret %s\n", KheEjectorCurrDebugIndent(ej) + 2, "",
      str, res ? "true" : "false");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheInDomainMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)     */
/*                                                                           */
/*  Return true when assigning or moving meet to time would place its        */
/*  starting time in a given time group, passed in impl.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheInDomainMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)
{
  int pos;
  return KheTimeGroupContains((KHE_TIME_GROUP) impl, time, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheNotInDomainMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)  */
/*                                                                           */
/*  Return true when assigning or moving meet to time would not place its    */
/*  starting time in a given time group, passed in impl.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheNotInDomainMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)
{
  int pos;
  return !KheTimeGroupContains((KHE_TIME_GROUP) impl, time, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignTimeAugment(KHE_EJECTOR ej, KHE_MONITOR d)                 */
/*                                                                           */
/*  Augment function for individual assign time defects.                     */
/*                                                                           */
/*  If KheEjectorRepairTimes(ej), then for each monitored unassigned meet,   */
/*  try all ejecting meet moves to a parent meet and offset that would       */
/*  assign the unassigned meet within its domain.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheAssignTimeAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_SOLN soln;  KHE_EVENT e;  KHE_MEET meet;  int i;
  bool not_ejecting;  KHE_ASSIGN_TIME_MONITOR atm;
  KheDebugAugmentBegin(DEBUG4, ej, d, "KheAssignTimeAugment");
  HnAssert(KheMonitorTag(d) == KHE_ASSIGN_TIME_MONITOR_TAG,
    "KheAssignTimeAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    atm = (KHE_ASSIGN_TIME_MONITOR) d;
    soln = KheEjectorSoln(ej);
    e = KheAssignTimeMonitorEvent(atm);
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    {
      meet = KheEventMeet(soln, e, i);
      if( KheMeetAsstTime(meet) == NULL )
      {
	/* ***
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewG lobalVisit(soln);
	*** */
	if( KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
	  (void *) KheMeetDomain(meet), KHE_OPTIONS_KEMPE_FALSE,
	  !not_ejecting, not_ejecting, false) )
	{
	  KheDebugAugmentEnd(DEBUG4, ej, d, "KheAssignTimeAugment", true);
	  return true;
	}
      }
    }
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheAssignTimeAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignTimeGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)            */
/*                                                                           */
/*  Augment function for groups of assign time defects.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheAssignTimeGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheAssignTimeGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheAssignTimeGroupAugment internal error 2");
  return KheAssignTimeAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "prefer times monitor augment functions"                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)                */
/*                                                                           */
/*  Augment function for individual prefer times defects.                    */
/*                                                                           */
/*  If KheEjectorRepairTimes(ej), then for each monitored meet with an       */
/*  unpreferred assignment, try all meet moves to a parent meet and offset   */
/*  that give the meet a preferred assignment.                               */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_SOLN soln;  KHE_EVENT e;  KHE_MEET meet;  int i, durn, pos;
  KHE_PREFER_TIMES_CONSTRAINT ptc;  KHE_TIME_GROUP domain;  KHE_TIME t;
  bool not_ejecting;  KHE_OPTIONS_KEMPE kempe;  KHE_PREFER_TIMES_MONITOR ptm;
  KheDebugAugmentBegin(DEBUG4, ej, d, "KhePreferTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_PREFER_TIMES_MONITOR_TAG,
    "KhePreferTimesAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    soln = KheEjectorSoln(ej);
    ptm = (KHE_PREFER_TIMES_MONITOR) d;
    e = KhePreferTimesMonitorEvent(ptm);
    ptc = KhePreferTimesMonitorConstraint(ptm);
    domain = KhePreferTimesConstraintDomain(ptc);
    durn = KhePreferTimesConstraintDuration(ptc);
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    /* kempe = KheEjectorUseKempeMoves(ej); */
    kempe = ao->use_kempe_moves;
    for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
    {
      meet = KheEventMeet(soln, e, i);
      t = KheMeetAsstTime(meet);
      if( (durn == KHE_ANY_DURATION || KheMeetDuration(meet) == durn) &&
	  t != NULL && !KheTimeGroupContains(domain, t, &pos) )
      {
	/* ***
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewG lobalVisit(soln);
	*** */
	if( KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
	    (void *) domain, kempe, !not_ejecting, not_ejecting,
	    WITH_PREFER_TIMES_NODE_SWAPS) )
	{
	  KheDebugAugmentEnd(DEBUG4, ej, d, "KhePreferTimesAugment", true);
	  return true;
	}
      }
    }
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KhePreferTimesAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)           */
/*                                                                           */
/*  Augment function for groups of prefer times defects.                     */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KhePreferTimesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KhePreferTimesGroupAugment internal error 2");
  return KhePreferTimesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "spread events monitor augment functions"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheSpreadEventsAugment(KHE_EJECTOR ej, KHE_MONITOR d)               */
/*                                                                           */
/*  Augment function for individual spread events defects.                   */
/*                                                                           */
/*  If KheEjectorRepairTimes(ej), try all meet moves of the relevant         */
/*  meets from outside each day to inside, or vice versa, depending on       */
/*  whether the problem is too few meets or too many.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheSpreadEventsAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_SOLN soln;  KHE_EVENT_GROUP eg;  KHE_EVENT e;  KHE_MEET meet;
  KHE_SPREAD_EVENTS_MONITOR sem;  KHE_TIME time;  KHE_TIME_GROUP tg;
  int i, j, k, minimum, maximum, inc, index, random_offset, pos;
  bool not_ejecting;  KHE_OPTIONS_KEMPE kempe;
  KheDebugAugmentBegin(DEBUG4, ej, d, "KheSpreadEventsAugment");
  HnAssert(KheMonitorTag(d) == KHE_SPREAD_EVENTS_MONITOR_TAG,
    "KheSpreadEventsAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    /* kempe = KheEjectorUseKempeMoves(ej); */
    kempe = ao->use_kempe_moves;
    soln = KheEjectorSoln(ej);
    sem = (KHE_SPREAD_EVENTS_MONITOR) d;
    eg = KheSpreadEventsMonitorEventGroup(sem);
    random_offset = 19 * KheEjectorCurrAugmentCount(ej) +
      43 * KheSolnDiversifier(KheEjectorSoln(ej));
    for( i = 0;  i < KheSpreadEventsMonitorTimeGroupCount(sem);  i++ )
    {
      KheSpreadEventsMonitorTimeGroup(sem, i, &tg, &minimum, &maximum, &inc);
      if( inc < minimum )
      {
	/* try all meet moves from outside tg to inside tg */
	for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
	{
	  index = (j + random_offset) % KheEventGroupEventCount(eg);
	  e = KheEventGroupEvent(eg, index);
	  for( k = 0;  k < KheEventMeetCount(soln, e);  k++ )
	  {
	    index = (k + random_offset) % KheEventMeetCount(soln, e);
	    meet = KheEventMeet(soln, e, index);
	    time = KheMeetAsstTime(meet);
	    if( time != NULL && !KheTimeGroupContains(tg, time, &pos) &&
	        KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
		  (void *) tg, kempe, !not_ejecting, not_ejecting,
		  WITH_SPREAD_EVENTS_NODE_SWAPS) )
	    {
	      KheDebugAugmentEnd(DEBUG4, ej, d, "KheSpreadEventsAugment (a)",
		true);
	      return true;
	    }
	  }
	}
      }
      else if( inc > maximum )
      {
	/* try all meet moves from inside tg to outside tg */
	for( j = 0;  j < KheEventGroupEventCount(eg);  j++ )
	{
	  index = (random_offset + j) % KheEventGroupEventCount(eg);
	  e = KheEventGroupEvent(eg, index);
	  for( k = 0;  k < KheEventMeetCount(soln, e);  k++ )
	  {
            index = (k + random_offset) % KheEventMeetCount(soln, e);
	    meet = KheEventMeet(soln, e, index);
	    time = KheMeetAsstTime(meet);
	    if( time != NULL && KheTimeGroupContains(tg, time, &pos) &&
	        KheMeetMultiRepair(ej, ao, meet, true,
		  &KheNotInDomainMeetMoveFn, (void *) tg, kempe,
		  !not_ejecting, not_ejecting, WITH_SPREAD_EVENTS_NODE_SWAPS) )
	    {
	      KheDebugAugmentEnd(DEBUG4, ej, d, "KheSpreadEventsAugment (b)",
		true);
	      return true;
	    }
	  }
	}
      }
    }
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheSpreadEventsAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSpreadEventsGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)          */
/*                                                                           */
/*  Augment function for groups of spread events defects.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheSpreadEventsGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheSpreadEventsGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheSpreadEventsGroupAugment internal error 2");
  return KheSpreadEventsAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "order events monitor augment functions"                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsAugment(KHE_EJECTOR ej, KHE_MONITOR d)                */
/*                                                                           */
/*  Augment function for individual order events defects.                    */
/*                                                                           */
/*  Not implemented yet.                                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheOrderEventsAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KheDebugAugmentBegin(DEBUG4, ej, d, "KheOrderEventsAugment");
  HnAssert(KheMonitorTag(d) == KHE_ORDER_EVENTS_MONITOR_TAG,
    "KheOrderEventsAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    /* still to do */
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheOrderEventsAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)           */
/*                                                                           */
/*  Augment function for groups of order events defects.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheOrderEventsGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheOrderEventsGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheOrderEventsGroupAugment internal error 2");
  return KheOrderEventsAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assign resource monitor augment functions"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSatisfies(KHE_TASK task, KHE_RESOURCE_GROUP rg,              */
/*    KHE_RESOURCE_GROUP not_rg, bool allow_unassign)                        */
/*                                                                           */
/*  Return true if task is currently assigned a resource from rg but         */
/*  not from not_rg, or is unassigned if allow_unassign is true.             */
/*  When rg and not_rg are NULL, no condition is imposed by them.            */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSatisfies(KHE_TASK task, KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP not_rg, bool allow_unassign)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task);
  if( r == NULL )
   return allow_unassign;
  else
    return (rg == NULL || KheResourceGroupContains(rg, r)) &&
    (not_rg == NULL || !KheResourceGroupContains(not_rg, r));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceMoveMultiRepair(KHE_EJECTOR ej,                     */
/*    KHE_AUGMENT_OPTIONS ao, KHE_EVENT_RESOURCE er, KHE_RESOURCE_GROUP rg,  */
/*    KHE_RESOURCE_GROUP not_rg, bool allow_unassign, KHE_MONITOR d,         */
/*    KHE_TASK *prev_task, KHE_TASK_SET scratch_ts)                          */
/*                                                                           */
/*  Try to repair defect d, by ensuring that the tasks of event resource er  */
/*  are assigned resources from rg (which must be present) but not not_rg    */
/*  (if present), or are unassigned if allow_unassign is true.               */
/*                                                                           */
/*  Here *prev_task is the task that this was tried on previously, or        */
/*  NULL if none.  This is used to avoid trying the same augment on          */
/*  equivalent tasks, and reassigned by this operation.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_EVENT_RESOURCE er, KHE_RESOURCE_GROUP rg,
  KHE_RESOURCE_GROUP not_rg, bool allow_unassign, KHE_MONITOR d,
  KHE_TASK *prev_task, KHE_TASK_SET scratch_ts)
{
  int i, count, random_offset;  KHE_SOLN soln;  KHE_TASK task;  KHE_RESOURCE r;
  KHE_TIME t;  KHE_TIME_GROUP t_tg;  /* struct khe_expansion_options_rec eor; */
  soln = KheEjectorSoln(ej);
  HnAssert(rg != NULL, "KheEventResourceMoveMultiRepair internal error 1");
  /* KheEjectorExpansionOptions(ej, &eor); */
  /* nocost_off = KheEjectorNoCostOff(ej); */
  random_offset = 23 * KheEjectorCurrAugmentCount(ej) +
    29 * KheSolnDiversifier(KheEjectorSoln(ej));
  count = KheEventResourceTaskCount(soln, er);
  for( i = 0;  i < count;  i++ )
  {
    /* here task is one of the tasks being monitored */
    task = KheEventResourceTask(soln, er, (i + random_offset) % count);
    task = KheTaskProperRoot(task);
    if( (*prev_task == NULL || !KheTaskEquiv(*prev_task, task)) &&
        !KheTaskSatisfies(task, rg, not_rg, allow_unassign) )
    {
      /* try moving the task to a resource in its domain */
      if( KheTaskMoveAugment(ej, ao, task, rg, not_rg, allow_unassign, NULL, d,
	    scratch_ts) )
	return true;

      /* if the domain is empty, try a double move of another task */
      r = KheTaskAsstResource(task);
      t = KheMeetAsstTime(KheTaskMeet(task));
      if( allow_unassign && t != NULL && r != NULL &&
	  KheResourceGroupResourceCount(rg) == 0 )
      {
	t_tg = KheTimeSingletonTimeGroup(t);
	if( KheResourceGainTaskMultiRepair(ej, ao, r, t_tg, true, scratch_ts) )
	  return true;
      }
      if( RANDOMIZED )
	return false;
      *prev_task = task;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignResourceAugment(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment function for individual assign resource defects.                 */
/*                                                                           */
/*  This uses calls to KheTaskMoveAugment to try to move each monitored      */
/*  unassigned task to any resource in that task's domain.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheAssignResourceAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_RESOURCE_GROUP domain;
  KHE_TASK pt;  KHE_EVENT_RESOURCE er;  KHE_TASK_SET scratch_ts;
  KheDebugAugmentBegin(DEBUG3, ej, d, "KheAssignResourceAugment");
  HnAssert(KheMonitorTag(d) == KHE_ASSIGN_RESOURCE_MONITOR_TAG,
    "KheAssignResourceAugment internal error 1");
  /* if( KheEjectorRepairResources(ej) ) */
  if( ao->repair_resources )
  {
    arm = (KHE_ASSIGN_RESOURCE_MONITOR) d;
    er = KheAssignResourceMonitorEventResource(arm);
    domain = KheEventResourceHardDomain(er);
    scratch_ts = KheTaskSetMake(KheEjectorSoln(ej));
    pt = NULL;
    if( KheEventResourceMoveMultiRepair(ej, ao, er, domain, NULL, false, d, &pt,
	  scratch_ts) )
    {
      KheTaskSetDelete(scratch_ts);
      return true;
    }
    KheTaskSetDelete(scratch_ts);
  }
  KheDebugAugmentEnd(DEBUG3, ej, d, "KheAssignResourceAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignResourceGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)        */
/*                                                                           */
/*  Augment function for groups of assign resource defects.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheAssignResourceGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheAssignResourceGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheAssignResourceGroupAugment internal error 2");
  return KheAssignResourceAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "prefer resources monitor augment functions"                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesAugment(KHE_EJECTOR ej, KHE_MONITOR d)            */
/*                                                                           */
/*  Augment function for individual prefer resources defects.                */
/*                                                                           */
/*  This uses calls to KheTaskMoveAugment to try to move each monitored      */
/*  task assigned an unpreferred resource to a preferred resource.  There    */
/*  is also another kind of repair, which moves the unpreferred resource     */
/*  to elsewhere in the same meet.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferResourcesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_PREFER_RESOURCES_CONSTRAINT prc;  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_EVENT_RESOURCE er;  KHE_TASK pt;  KHE_RESOURCE_GROUP domain;
  KHE_TASK_SET scratch_ts;
  KheDebugAugmentBegin(DEBUG3, ej, d, "KhePreferResourcesAugment");
  HnAssert(KheMonitorTag(d) == KHE_PREFER_RESOURCES_MONITOR_TAG,
    "KhePreferResourcesAugment internal error 1");
  /* if( KheEjectorRepairResources(ej) ) */
  if( ao->repair_resources )
  {
    prm = (KHE_PREFER_RESOURCES_MONITOR) d;
    er = KhePreferResourcesMonitorEventResource(prm);
    prc = KhePreferResourcesMonitorConstraint(prm);
    domain = KhePreferResourcesConstraintDomain(prc);
    scratch_ts = KheTaskSetMake(KheEjectorSoln(ej));
    pt = NULL;
    if( KheEventResourceMoveMultiRepair(ej, ao, er, domain, NULL, true, d, &pt,
	  scratch_ts) )
    {
      KheTaskSetDelete(scratch_ts);
      return true;
    }
    KheTaskSetDelete(scratch_ts);
  }
  KheDebugAugmentEnd(DEBUG3, ej, d, "KhePreferResourcesAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferResourcesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)       */
/*                                                                           */
/*  Augment function for groups of prefer resources defects.                 */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferResourcesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KhePreferResourcesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KhePreferResourcesGroupAugment internal error 2");
  return KhePreferResourcesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "avoid split assignments monitor augment functions"            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEject ingTaskSetMoveRepair(KHE_EJECTOR ej,                       */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE from_r1,        */
/*    KHE_RESOURCE from_r2, KHE_RESOURCE r)                                  */
/*                                                                           */
/*  Try an ejecting task-set move (KHE_REPAIR_TASK_SET_MOVE) to              */
/*  r of those tasks monitored by asam which are currently assigned to       */
/*  from_r1 and from_r2.                                                     */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static bool KheEject ingTaskSetMoveRepair(KHE_EJECTOR ej,
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE from_r1,
  KHE_RESOURCE from_r2, KHE_RESOURCE r)
{
  KHE_RESOURCE ar;  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;
  KHE_TASK task;  int egi, count, j, k, durn;  KHE_TIME_PARTITION tp;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  bool success;
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  soln = KheEjectorSoln(ej);
  KheEjecto rRepairBegin(ej);
  success = true;
  durn = 0;
  tp = KheEjectorTimePartition(ej);
  for( j = 0;  success && j < count;  j++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, j);
    for( k = 0;  success && k < KheEventResourceTaskCount(soln, er);  k++ )
    {
      task = KheEventResourceTask(soln, er, k);
      ar = KheTaskAsstResource(task);
      if( (ar == from_r1 || ar == from_r2) && ar != r )
      {
	success = success && KheEjectingTaskMove(task, r, tp);
	durn += KheTaskDuration(task);
      }
    }
  }
  return KheEjector RepairEnd(ej, KHE_REPAIR_TASK_SET_MOVE, success);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheSplitTasksUnassignResourceGroup(                   */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE r)              */
/*                                                                           */
/*  Return a resource group consisting of the resources currently assigned   */
/*  to the tasks monitored by asam, excluding r.                             */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
#else
static KHE_RESOURCE_GROUP KheSplitTasksUnassignResourceGroup(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE r)
{
  KHE_SOLN soln;  int i;  KHE_RESOURCE r2;
  soln = KheMonitorSoln((KHE_MONITOR) asam);
  KheSolnResourceGroupBegin(soln, KheResourceResourceType(r));
  for( i = 0;  i < KheAvoidSplitAssignmentsMonitorResourceCount(asam);  i++ )
  {
    r2 = KheAvoidSplitAssignmentsMonitorResource(asam, i);
    if( r2 != r )
      KheSolnResourceGroupAddResource(soln, r2);
  }
  return KheSolnResourceGroupEnd(soln);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitTasksUnassignOnSuccess(void *on_success_val)                */
/*                                                                           */
/*  On-success function used when repairing avoid split assignments defects. */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
#else
static void KheSplitTasksUnassignOnSuccess(KHE_TASK_BOUND tb)
{
  bool success;
  if( DEBUG13 )
    fprintf(stderr, "[ KheSplitTasksUnassignOnSuccess\n");
  success = KheTaskBoundDelete(tb);
  if( DEBUG13 )
    fprintf(stderr, "] KheSplitTasksUnassignOnSuccess(success = %s)\n",
      success ? "true" : "false");
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitTasksUnassignRepair(KHE_EJECTOR ej,                         */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE r)              */
/*                                                                           */
/*  Repair which tries unassigning all tasks monitored by asam that are      */
/*  assigned r, and reducing the domains of all tasks monitored by asam      */
/*  to exclude r.                                                            */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
#else
static bool KheSplitTasksUnassignRepair(KHE_EJECTOR ej,
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_RESOURCE r)
{
  KHE_SOLN soln;  KHE_RESOURCE_GROUP avoid_splits_rg;  KHE_TASK task;
  int egi, count, i, j;  KHE_TASK_BOUND tb;  bool success;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  KHE_EVENT_RESOURCE er;
  KheEjectorRepairBegin(ej);
  soln = KheEjectorSoln(ej);
  avoid_splits_rg = KheSplitTasksUnassignResourceGroup(asam, r);
  tb = KheTaskBoundMake(soln, avoid_splits_rg);
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheEventResourceTask(soln, er, j);
      if( KheTaskAsstResource(task) == r && !KheTaskUnAssign(task) )
	return KheEjectorRepairEnd(ej, KHE_REPAIR_SPLIT_TASKS_UNASSIGN, false);
      if( !KheTaskAddTaskBound(task, tb) )
	return KheEjectorRepairEnd(ej, KHE_REPAIR_SPLIT_TASKS_UNASSIGN, false);
    }
  }
  success = KheEjectorRepairEnd(ej, KHE_REPAIR_SPLIT_TASKS_UNASSIGN, true);
  if( success )
    KheSplitTasksUnassignOnSuccess(tb);
  return success;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignedTasksVisited(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam,   */
/*    int slack)                                                             */
/*                                                                           */
/*  Like KheTaskVisited(task, slack) only applied to all the assigned        */
/*  tasks monitored by asam; it returns true if any of them are visited.     */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static bool KheAssignedTasksVisited(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam,
  int slack)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  int egi, count, i, j;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TASK task;
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  soln = KheMonitorSoln((KHE_MONITOR) asam);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, j));
      if( task == NULL )
	return false;
      if( KheTaskAsstResource(task) != NULL && KheTaskVisited(task, slack) )
	return true;
    }
  }
  return false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignedTasksVisit(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)     */
/*                                                                           */
/*  Like KheTaskVisit(task) only applied to all the assigned tasks           */
/*  monitored by asam.                                                       */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static void KheAssignedTasksVisit(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  int egi, count, i, j;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TASK task;
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  soln = KheMonitorSoln((KHE_MONITOR) asam);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, j));
      if( task != NULL && KheTaskAsstResource(task) != NULL )
	KheTaskVisit(task);
    }
  }
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignedTasksUnVisit(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)   */
/*                                                                           */
/*  Like KheTaskUnVisit(task) only applied to all the assigned tasks         */
/*  monitored by asam.                                                       */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static void KheAssignedTasksUnVisit(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  int egi, count, i, j;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TASK task;
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  soln = KheMonitorSoln((KHE_MONITOR) asam);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, j));
      if( task != NULL && KheTaskAsstResource(task) != NULL )
	KheTaskUnVisit(task);
    }
  }
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheAnyTaskAssignedToResource(KHE_RESOURCE r,                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)                              */
/*                                                                           */
/*  Return any task monitored by asam assigned r.  There must be one.        */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static KHE_TASK KheAnyTaskAssignedToResource(KHE_RESOURCE r,
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;  int egi, count, i, j;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TASK task;
  c = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, egi);
  soln = KheMonitorSoln((KHE_MONITOR) asam);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, egi, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, j));
      if( task != NULL && KheTaskAsstResource(task) == r )
	return task;
    }
  }
  HnAbort("KheAnyTaskAssignedToResource internal error");
  return NULL;  /* keep compiler happy */
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsAugment(KHE_EJECTOR ej, KHE_MONITOR d)      */
/*                                                                           */
/*  Augment function for individual avoid split assignments defects.         */
/*  There is no group version of this function.                              */
/*                                                                           */
/*  If KheEjectorRepairResources(ej), then for each pair of resources        */
/*  assigned to monitored tasks, and for each resource in the domain of      */
/*  one of those tasks, try a set of ejecting task moves of those tasks to   */
/*  that resource.                                                           */
/*                                                                           */
/*  Also, if KheEjectorRepairTimes(ej) as well, then for each resource       */
/*  assigned to exactly one of the monitored tasks, try moving that          */
/*  task's meet to a different time, and simultaneously moving the task      */
/*  to one of the other resources assigned to a monitored task.              */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_AVOID_SPLIT_REPAIR
static bool KheAvoidSplitAssignmentsAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPIONS ao, KHE_MONITOR d)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;  KHE_TASK task;
  int i, j, k;  KHE_RESOURCE r1, r2, r;  KHE_RESOURCE_GROUP rg;
  KheDebugAugmentBegin(DEBUG5, ej, d, "KheAvoidSplitAssignmentsAugment");
  if( KheEjectorRepairResources(ej) )
  {
    asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) d;
    if( !KheAssignedTasksVisited(asam, 0) )
    {
      KheAssignedTasksVisit(asam);

      /* try ejecting task-set moves */
      for( i = 0; i < KheAvoidSplitAssignmentsMonitorResourceCount(asam); i++ )
      {
	r1 = KheAvoidSplitAssignmentsMonitorResource(asam, i);
	rg = KheTaskDomain(KheAnyTaskAssignedToResource(r1, asam));
	for( j=i+1; j<KheAvoidSplitAssignmentsMonitorResourceCount(asam); j++ )
	{
	  r2 = KheAvoidSplitAssignmentsMonitorResource(asam, j);
	  for( k = 0;  k < KheResourceGroupResourceCount(rg);  k++ )
	  {
	    r = KheResourceGroupResource(rg, k);
	    if( DEBUG12 )
	      fprintf(stderr, "  trying %s\n", KheResourceShow(r));
	    if( KheEjectingTaskSe tMoveRepair(ej, asam, r1, r2, r) )
	    {
	      KheDebugAugmentEnd(DEBUG5, ej, d,
		"KheAvoidSplitAssignmentsAugment", true);
	      return true;
	    }
	  }
	}
      }

      /* try meet/task moves */
      if( KheEjectorRepairTimes(ej) )
      for( i = 0; i < KheAvoidSplitAssignmentsMonitorResourceCount(asam); i++ )
      {
	if( KheAvoidSplitAssignmentsMonitorResourceMultiplicity(asam, i) == 1 )
	{
	  /* r1 occurs in only one task, try moving it to any other resource */
	  r1 = KheAvoidSplitAssignmentsMonitorResource(asam, i);
	  task = KheAnyTaskAssignedToResource(r1, asam);
	  HnAssert(task!=NULL, "KheAvoidSplitAssignmentsAugment internal error");
	  for( j=0; j<KheAvoidSplitAssignmentsMonitorResourceCount(asam); j++ )
	  {
	    r2 = KheAvoidSplitAssignmentsMonitorResource(asam, j);
	    if(r2!=r1 && KheKem peMeetMoveAndTaskMoveAugment(ej,
		  task, r2, KHE_MOVE_EJECTING))
	    {
	      KheDebugAugmentEnd(DEBUG5, ej, d,
		"KheAvoidSplitAssignmentsAugment", true);
	      return true;
	    }
	  }
	}
      }

      if( KheEjectorCurrMayRevisit(ej) )
	KheAssignedTasksUnVisit(asam);
    }
  }
  KheDebugAugmentEnd(DEBUG5, ej, d, "KheAvoidSplitAssignmentsAugment", false);
  return false;
}
#else
static bool KheAvoidSplitAssignmentsAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;  int i;  KHE_RESOURCE r;
  /* bool save_ejector_repair_times; */
  KheDebugAugmentBegin(DEBUG15, ej, d, "KheAvoidSplitAssignmentsAugment");
  /* if( KheEjectorRepairResources(ej) ) */
  if( ao->repair_resources )
  {
    asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) d;
    /* *** interesting idea, didn't help much
    save_ejector_repair_times = KheEjectorRepairTimes(ej);
    if( KheEjectorCurrLength(ej) == 1 )
      KheOptionsSetEjectorRepairTimes(KheEjectorOptions(ej), true);
    *** */
    for( i = 0;  i < KheAvoidSplitAssignmentsMonitorResourceCount(asam);  i++ )
    {
      r = KheAvoidSplitAssignmentsMonitorResource(asam, i);
      if( KheEjectorCurrLength(ej) == 1 ||
	  KheAvoidSplitAssignmentsMonitorResourceMultiplicity(asam, i) == 1 )
      {
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewGlobalVisit(KheEjectorSoln(ej));
	if( KheSplitTasksUnassignRepair(ej, asam, r) )
	{
	  if( DEBUG15 )
	  {
	    fprintf(stderr,
	      "%*s] KheAvoidSplitAssignmentsAugment ret true%s:\n",
	      KheEjectorCurrDebugIndent(ej) + 2, "",
	      KheEjectorCurrLength(ej) == 1 ? " 1" : "");
	      KheMonitorDebug(d, 1, KheEjectorCurrDebugIndent(ej) + 4, stderr);
	  }
	  /* ***
	  KheOptionsSetEjectorRepairTimes(KheEjectorOptions(ej),
	    save_ejector_repair_times);
	  *** */
	  return true;
	}
      }
    }
    /* ***
    KheOptionsSetEjectorRepairTimes(KheEjectorOptions(ej),
      save_ejector_repair_times);
    *** */
  }
  KheDebugAugmentEnd(DEBUG15, ej, d, "KheAvoidSplitAssignmentsAugment", false);
  return false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit resources monitor augment functions"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesAugment(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment function for individual limit resources defects.                 */
/*                                                                           */
/*  If the defect is an underload, use KheTaskMoveAugment to try to move     */
/*  each task which is either unassigned or contains an unpreferred          */
/*  resource to a resource in the domain of the constraint.  Unassigned      */
/*  tasks are tried first.                                                   */
/*                                                                           */
/*  If the defect is an overload, use KheTaskMoveAugment to try to move      */
/*  each task which is currently assigned a resource from the domain to a    */
/*  resource from outside the domain (including unassignment).               */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT c;  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_TASK pt;  int eg_index, i, count, maximum, minimum, active_durn;
  KHE_RESOURCE_GROUP domain, rg;  KHE_EVENT_RESOURCE er;
  KHE_TASK_SET scratch_ts;
  KheDebugAugmentBegin(DEBUG15, ej, d, "KheLimitResourcesAugment");
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_RESOURCES_MONITOR_TAG,
    "KheLimitResourcesAugment internal error 1");
  lrm = (KHE_LIMIT_RESOURCES_MONITOR) d;
  c = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  count = KheLimitResourcesConstraintEventResourceCount(c, eg_index);
  domain = KheLimitResourcesConstraintDomain(c);
  /* if( KheEjectorRepairResources(ej) ) */
  if( ao->repair_resources )
  {
    KheLimitResourcesMonitorActiveDuration(lrm, &minimum,&maximum,&active_durn);
    if( active_durn < minimum )
    {
      /* too few resources from c's domain */
      scratch_ts = KheTaskSetMake(KheEjectorSoln(ej));
      pt = NULL;
      for( i = 0;  i < count;  i++ )
      {
	er = KheLimitResourcesConstraintEventResource(c, eg_index, i);
        if( KheEventResourceMoveMultiRepair(ej, ao, er, domain, NULL, false,
	      d, &pt, scratch_ts) )
	{
	  KheTaskSetDelete(scratch_ts);
	  KheDebugAugmentEnd(DEBUG15, ej, d, "KheLimitResourcesAugment",true);
	  return true;
	}
      }
      KheTaskSetDelete(scratch_ts);
    }
    else if( active_durn > maximum )
    {
      /* too many resources from c's domain */
      scratch_ts = KheTaskSetMake(KheEjectorSoln(ej));
      pt = NULL;
      for( i = 0;  i < count;  i++ )
      {
	er = KheLimitResourcesConstraintEventResource(c, eg_index, i);
	rg = KheEventResourceHardDomain(er);
        if( KheEventResourceMoveMultiRepair(ej, ao, er, rg, domain, true,
	      d, &pt, scratch_ts) )
	{
	  KheTaskSetDelete(scratch_ts);
	  KheDebugAugmentEnd(DEBUG15, ej, d, "KheLimitResourcesAugment", true);
	  return true;
	}
      }
      KheTaskSetDelete(scratch_ts);
    }
    else
      HnAbort("KheLimitResourcesAugment internal error 2");
  }
  KheDebugAugmentEnd(DEBUG15, ej, d, "KheLimitResourcesAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)        */
/*                                                                           */
/*  Augment function for groups of limit resources defects.                  */
/*                                                                           */
/*  These are not necessarily equivalent so we search for one with           */
/*  non-zero cost and repair that.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;  KHE_MONITOR m;  int i;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheLimitResourcesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheLimitResourcesGroupAugment internal error 2");
  for( i = 0;  i < KheGroupMonitorChildMonitorCount(gm);  i++ )
  {
    m = KheGroupMonitorChildMonitor(gm, i);
    if( KheMonitorCost(m) > 0 && KheLimitResourcesAugment(ej, ao, m) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "avoid clashes augment functions"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesAugment(KHE_EJECTOR ej, KHE_MONITOR d)               */
/*                                                                           */
/*  Augment function for individual avoid clashes defects.                   */
/*                                                                           */
/*  The current plan is to detach all avoid clashes monitors during          */
/*  repair, since their work is done (and done better) by demand monitors.   */
/*  So this function aborts if it is called.                                 */
/*                                                                           */
/*  Old specification                                                        */
/*  -----------------                                                        */
/*  For each clashing task, try moving it to any other resource in its       */
/*  domain.  This function expects to be called only during resource repair, */
/*  because during time repair avoid clashes monitors are usually detached.  */
/*  Even during resource repair it would probably be better to ignore these  */
/*  defects, since they are basically unrepairable then anyway.              */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidClashesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KheDebugAugmentBegin(DEBUG3, ej, d, "KheAvoidClashesAugment");
  HnAbort("KheAvoidClashesAugment: unexpected call");
  KheDebugAugmentEnd(DEBUG3, ej, d, "KheAvoidClashesAugment", false);
  return false;  /* keep compiler happy */

  /* boilerplate */
  /* ***
  HnAssert(KheMonitorTag(d) == KHE_AVOID_CLASHES_MONITOR_TAG,
    "KheAvoidClashesAugment internal error 1");
  soln = KheEjectorSoln(ej);
  acm = (KHE_AVOID_CLASHES_MONITOR) d;
  r = KheAvoidClashesMonitorResource(acm);
  tm = KheResourceTimetableMonitor(soln, r);

  ** find each clashing task and reassign it in all possible ways **
  for( i = 0;  i < KheTimetableMonitorClashingTimeCount(tm);  i++ )
  {
    t = KheTimetableMonitorClashingTime(tm, i);
    for( j = 0;  j < KheTimetableMonitorTimeMeetCount(tm, t);  j++ )
    {
      meet = KheTimetableMonitorTimeMeet(tm, t, j);
      if(KheMeetEjectingTaskMoveAugment(ej, meet, r) )
      {
	KheDebugAugmentEnd(DEBUG3, ej, d, "KheAvoidClashesAugment", true);
	return;
      }
    }
  }

  ** wrapup **
  KheDebugAugmentEnd(DEBUG3, ej, d, "KheAvoidClashesAugment", false);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)          */
/*                                                                           */
/*  Augment function for groups of avoid clashes defects.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidClashesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheAvoidClashesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheAvoidClashesGroupAugment internal error 2");
  return KheAvoidClashesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "avoid unavailable times monitor augment functions"            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)      */
/*                                                                           */
/*  Augment function for individual avoid unavailable times defects.         */
/*                                                                           */
/*  Try repairs documented in the header of KheResourceOverloadMultiRepair,  */
/*  which in this case amount to task moves away from d's resource and       */
/*  meet moves away from unavailable times.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidUnavailableTimesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;  KHE_TIME_GROUP domain;
  KHE_RESOURCE r; KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;  bool res;
  KheDebugAugmentBegin(DEBUG4, ej, d, "KheAvoidUnavailableTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
    "KheAvoidUnavailableTimesAugment internal error 1");
  autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) d;
  autc = KheAvoidUnavailableTimesMonitorConstraint(autm);
  domain = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);
  r = KheAvoidUnavailableTimesMonitorResource(autm);
  res = KheResourceOverloadMultiRepair(ej, ao, r, domain, false);
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheAvoidUnavailableTimesAugment", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidUnavailableTimesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d) */
/*                                                                           */
/*  Augment function for groups of avoid unavailable times defects.          */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidUnavailableTimesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheAvoidUnavailableTimesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheAvoidUnavailableTimesGroupAugment internal error 2");
  return KheAvoidUnavailableTimesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit idle times monitor augment functions (simple)"          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimeWillBeBusy(KHE_TIME time,                           */
/*    KHE_TIMETABLE_MONITOR tm, KHE_MEET meet, KHE_TIME new_time)            */
/*                                                                           */
/*  Return true if time will be busy after meet moves to new_time.           */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
static bool KheLimitIdleTimeWillBeBusy(KHE_TIME time,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MEET meet, KHE_TIME new_time)
{
  int i;  KHE_MEET meet2;

  /* time will be busy if there is already some other meet at time */
  for( i = 0;  i < KheResourceTimetableMonitorTimeTaskCount(rtm, time);  i++ )
  {
    meet2 = KheTaskMeet(KheResourceTimetableMonitorTimeTask(rtm, time, i));
    if( meet2 != meet )
      return true;
  }

  /* alternatively, time will be busy if meet will overlap time */
  return KheTimeIntervalsOverlap(new_time, KheMeetDuration(meet), time, 1) > 0;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitNewIdle(KHE_TIME_GROUP tg, KHE_TIMETABLE_MONITOR tm,         */
/*    KHE_MEET meet, KHE_TIME new_time)                                      */
/*                                                                           */
/*  Return the number of idle times there will be in tg after meet           */
/*  moves from where it is now to new_time.                                  */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
static int KheLimitNewIdle(KHE_TIME_GROUP tg,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MEET meet, KHE_TIME new_time)
{
  KHE_TIME time, first_busy, last_busy;  int i, busy_count;

  /* find the number of and first and last busy times after meet moves */
  first_busy = last_busy = NULL;
  busy_count = 0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    if( KheLimitIdleTimeWillBeBusy(time, rtm, meet, new_time) )
    {
      busy_count++;
      if( first_busy == NULL )
	first_busy = time;
      last_busy = time;
    }
  }

  /* use the usual formula to calculate the number of idle times */
  return busy_count == 0 ? 0 :
    (KheTimeIndex(last_busy) - KheTimeIndex(first_busy) + 1) - busy_count;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)    */
/*                                                                           */
/*  Return true if moving meet from where it is now to new_time would        */
/*  decrease the number of idle times, according to litm (passed in impl).   */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
static bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)
{
  int tg_durn, durn, i, delta, busy_count, idle_count, count;
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_TIME times[2];
  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TIME tg_time, old_time;
  litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) impl;
  rtm = KheResourceTimetableMonitor(KheMeetSoln(meet),
    KheLimitIdleTimesMonitorResource(litm));
  durn = KheMeetDuration(meet);
  old_time = KheMeetAsstTime(meet);
  delta = 0;
  for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupCount(litm);  i++ )
  {
    tg = KheLimitIdleTimesMonitorTimeGroupState(litm, i, &busy_count,
      &idle_count, times, &count);
    tg_time = KheTimeGroupTime(tg, 0);
    tg_durn = KheTimeGroupTimeCount(tg);
    if( KheTimeIntervalsOverlap(old_time, durn, tg_time, tg_durn) ||
	KheTimeIntervalsOverlap(time, durn, tg_time, tg_durn) )
      delta += KheLimitNewIdle(tg, rtm, meet, time) - idle_count;
  }
  return delta < 0;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit idle times monitor augment functions (complex)"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIT_SOLVER - solver for complex limit idle times repairs             */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_lit_solver_rec {
  HA_ARENA			arena;		/* arena                     */
  KHE_EJECTOR			ejector;	/* the ejector               */
  KHE_LIMIT_IDLE_TIMES_MONITOR	litm;		/* the defect                */
  KHE_TIME_GROUP		tg;		/* defective time group      */
  KHE_RESOURCE			resource;	/* litm's resource           */
  KHE_SOLN			soln;		/* litm's soln               */
  ARRAY_KHE_MEET		meets;		/* the meets of tgm          */
  int				total_durn;	/* total duration of meets   */
  int				asst_count;	/* number of meets assigned  */
  int				asst_problems;	/* number of bad assignments */
  int				repair_count;	/* number repairs            */
  KHE_TIME			curr_start_time; /* start time, currently    */
} *KHE_LIT_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIT_SOLVER KheLitSolverMake(KHE_EJECTOR ej,                          */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR litm, KHE_TIME_GROUP_MONITOR tgm)         */
/*                                                                           */
/*  Make and return a new lit solver with these attributes.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_LIT_SOLVER KheLitSolverMake(KHE_EJECTOR ej,
  KHE_LIMIT_IDLE_TIMES_MONITOR litm, KHE_TIME_GROUP tg)
{
  KHE_LIT_SOLVER res;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_SOLN soln;
  KHE_TIME time;  int i, j, pos;  KHE_MEET meet;  HA_ARENA a;

  /* make the basic object */
  soln = KheEjectorSoln(ej);
  a = KheSolnArenaBegin(soln, false);
  /* a = HaAre naMake(); */
  HaMake(res, a);
  res->arena = a;
  res->ejector = ej;
  res->litm = litm;
  res->tg = tg;
  res->resource = KheLimitIdleTimesMonitorResource(litm);
  res->soln = soln;
  HaArrayInit(res->meets, a);
  res->total_durn = 0;
  res->asst_count = 0;
  res->asst_problems = 0;
  res->repair_count = 0;
  res->curr_start_time = NULL;

  /* add the meets of tgm */
  rtm = KheResourceTimetableMonitor(res->soln, res->resource);
  for( i = 0;  i < KheTimeGroupTimeCount(res->tg);  i++ )
  {
    time = KheTimeGroupTime(res->tg, i);
    for( j = 0;  j < KheResourceTimetableMonitorTimeTaskCount(rtm, time);  j++ )
    {
      meet = KheTaskMeet(KheResourceTimetableMonitorTimeTask(rtm, time, j));
      if( !HaArrayContains(res->meets, meet, &pos) )
      {
	HaArrayAddLast(res->meets, meet);
	res->total_durn += KheMeetDuration(meet);
      }
    }
  }

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLitSolverDelete(KHE_LIT_SOLVER lits)                             */
/*                                                                           */
/*  Delete lits.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheLitSolverDelete(KHE_LIT_SOLVER lits)
{
  KheSolnArenaEnd(lits->soln, lits->arena);
  /* HaArenaD elete(lits->arena); */
  /* ***
  MArrayFree(lits->meets);
  MFree(lits);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLitSolverAsstIsOpen(KHE_LIT_SOLVER lits,                         */
/*    KHE_MEET meet, KHE_TIME time, int pos)                                 */
/*                                                                           */
/*  Return true if the assignment of meet to time is open.                   */
/*                                                                           */
/*****************************************************************************/
#define KheIndent 2*(KheEjectorCurrLength(lits->ejector) + pos + 1)

/*****************************************************************************/
/*                                                                           */
/*  bool KheLitSolverSolve(KHE_LIT_SOLVER lits, int pos, int prev_durn)      */
/*                                                                           */
/*  Solve lits from position pos onwards; prev_durn is the total duration    */
/*  previous to pos.                                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheLitSolverSolve(KHE_LIT_SOLVER lits, int pos, int prev_durn)
{
  KHE_TIME time;  int i, save_problems;  KHE_MEET meet, tmp;  bool success;
  if( DEBUG9 )
    fprintf(stderr, "%*s[ KheLitSolverSolve(lits, pos %d, prev_durn %d)\n",
      KheIndent, "", pos, prev_durn);
  if( pos >= HaArrayCount(lits->meets) )
  {
    /* carry out the repair now; the ejector will undo it */
    lits->repair_count++;
    KheEjectorRepairBegin(lits->ejector);
    HaArrayForEach(lits->meets, meet, i)
      KheMeetUnAssignTime(meet);
    time = lits->curr_start_time;
    success = true;
    HaArrayForEach(lits->meets, meet, i)
    {
      HnAssert(time != NULL, "KheLitSolverSolve internal error 1");
      success = KheMeetAssignTime(meet, time);
      if( !success )
	break;
      time = KheTimeHasNeighbour(time, KheMeetDuration(meet)) ?
	KheTimeNeighbour(time, KheMeetDuration(meet)) : NULL;
    }
    success = KheEjectorRepairEnd(lits->ejector, KHE_REPAIR_COMPLEX_IDLE_MOVE,
      success);
    if( DEBUG9 )
      fprintf(stderr, "%*s] KheLitSolverSolve returning %s (repair)\n",
	KheIndent, "", success ? "true" : "false");
    return success;
  }
  else if( lits->repair_count < MAX_LIT_REPAIRS )
  {
    /* try every meet from pos onwards in position pos */
    for( i = pos;  i < HaArrayCount(lits->meets);  i++ )
    {
      HaArraySwap(lits->meets, pos, i, tmp);
      meet = HaArray(lits->meets, pos);
      time = KheTimeNeighbour(lits->curr_start_time, prev_durn);
      save_problems = lits->asst_problems;
      if( /* KheLitSolverAsstIsOpen(lits, meet, time, pos) && */
	  KheLitSolverSolve(lits, pos+1, prev_durn + KheMeetDuration(meet)) )
      {
	if( DEBUG9 )
	  fprintf(stderr, "%*s] KheLitSolverSolve returning true\n",
	    KheIndent, "");
	return true;
      }
      lits->asst_problems = save_problems;
      HaArraySwap(lits->meets, pos, i, tmp);
    }
    if( DEBUG9 )
      fprintf(stderr, "%*s] KheLitSolverSolve returning false\n",
	KheIndent, "");
    return false;
  }
  else
  {
    /* just try one repair, since limit is reached */
    meet = HaArray(lits->meets, pos);
    time = KheTimeNeighbour(lits->curr_start_time, prev_durn);
    save_problems = lits->asst_problems;
    if( /* KheLitSolverAsstIsOpen(lits, meet, time, pos) && */
	KheLitSolverSolve(lits, pos+1, prev_durn + KheMeetDuration(meet)) )
    {
      if( DEBUG9 )
	fprintf(stderr, "%*s] KheLitSolverSolve returning true (single)\n",
	  KheIndent, "");
      return true;
    }
    lits->asst_problems = save_problems;
    if( DEBUG9 )
      fprintf(stderr, "%*s] KheLitSolverSolve returning false (single)\n",
	KheIndent, "");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleComplexAugment(KHE_EJECTOR ej,                          */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR litm, KHE_TIME_GROUP_MONITOR tgm)         */
/*                                                                           */
/*  Augment function for limit idle times defects which tries complex        */
/*  reassignments of the meets assigned to the times of tg, which is         */
/*  known to contain at least one idle time.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitIdleComplexAugment(KHE_EJECTOR ej,
  KHE_LIMIT_IDLE_TIMES_MONITOR litm, KHE_TIME_GROUP tg)
{
  KHE_LIT_SOLVER lits;  int stop_index, i;
  lits = KheLitSolverMake(ej, litm, tg);
  if( DEBUG8 )
    fprintf(stderr, "%*s[ KheLimitIdleComplexAugment(ej, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(lits->resource),
      KheTimeGroupId(lits->tg));
  stop_index = KheTimeGroupTimeCount(lits->tg) - lits->total_durn;
  for( i = 0;  i < stop_index;  i++ )
  {
    lits->curr_start_time = KheTimeGroupTime(lits->tg, i);
    if( DEBUG8 )
      fprintf(stderr, "%*s  starting time %s:\n", KheEjectorCurrDebugIndent(ej), "",
	KheTimeId(lits->curr_start_time));
    if( KheLitSolverSolve(lits, 0, 0) )
    {
      KheLitSolverDelete(lits);
      if( DEBUG8 )
	fprintf(stderr,
	  "%*s] KheLimitIdleComplexAugment returning true (%d repairs)\n",
	  KheEjectorCurrDebugIndent(ej), "", lits->repair_count);
      return true;
    }
  }
  KheLitSolverDelete(lits);
  if( DEBUG8 )
    fprintf(stderr,
      "%*s] KheLimitIdleComplexAugment returning false (%d repairs)\n",
      KheEjectorCurrDebugIndent(ej), "", lits->repair_count);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)    */
/*                                                                           */
/*  Return true if moving meet from where it is now to time would cause      */
/*  it to begin at an idle time of litm (passed in impl).                    */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
#else
#if WITH_NEW_LIMIT_IDLE_REPAIR_EXACT
static bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_TIME_GROUP_MONITOR tgm;
  int durn, xdurn, i, count;  KHE_TIME times[2], xt;
  KHE_TIMETABLE_MONITOR tm;
  litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) impl;
  tm = KheResourceTimetableMonitor(KheMeetSoln(meet),
    KheLimitIdleTimesMonitorResource(litm));
  if( KheTimetableMonitorTimeMeetCount(tm, time) > 0 )
    return false;
  durn = KheMeetDuration(meet);
  for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupMonitorCount(litm);  i++ )
  {
    tgm = KheLimitIdleTimesMonitorTimeGroupMonitor(litm, i);
    if( KheTimeGroupMonitorIdleCount(tgm) > 0 )
    {
      /* tgm has an idle time to overlap with */
      KheTimeGroupMonitorFirstAndLastBusyTimes(tgm, times, &count);
      HnAssert(count == 2, "KheLimitIdleMeetMoveFn internal error");
      xt = KheTimeNeighbour(times[0], 1);
      xdurn = KheTimeIndex(times[1]) - KheTimeIndex(times[0]) - 1;
      if( KheTimeIntervalsOverlap(time, 1, xt, xdurn) > 0 )
	return true;
    }
  }
  return false;
}
#endif
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)    */
/*                                                                           */
/*  Return true if moving meet from where it is now to time would cause      */
/*  it to overlap an idle time of litm (passed in impl).                     */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
#else
#if WITH_NEW_LIMIT_IDLE_REPAIR_OVERLAP
static bool KheLimitIdleMeetMoveFn(KHE_MEET meet, KHE_TIME time, void *impl)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_TIME_GROUP_MONITOR tgm;
  int durn, xdurn, ydurn, i, count, ti;  KHE_TIME times[2], xt, yt, time2;
  KHE_TIMETABLE_MONITOR tm;
  litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) impl;
  tm = KheResourceTimetableMonitor(KheMeetSoln(meet),
    KheLimitIdleTimesMonitorResource(litm));
  durn = KheMeetDuration(meet);
  for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupMonitorCount(litm);  i++ )
  {
    tgm = KheLimitIdleTimesMonitorTimeGroupMonitor(litm, i);
    if( KheTimeGroupMonitorIdleCount(tgm) > 0 )
    {
      /* tgm has an idle time to overlap with */
      KheTimeGroupMonitorFirstAndLastBusyTimes(tgm, times, &count);
      HnAssert(count == 2, "KheLimitIdleMeetMoveFn internal error");
      xt = KheTimeNeighbour(times[0], 1);
      xdurn = KheTimeIndex(times[1]) - KheTimeIndex(times[0]) - 1;
      if( KheTimeIntervalsOverlapInterval(time, durn, xt, xdurn, &yt, &ydurn) )
	for( ti = 0;  ti < ydurn;  ti++ )
	{
	  time2 = KheTimeNeighbour(yt, ti);
	  if( KheTimetableMonitorTimeMeetCount(tm, time2) == 0 )
	    return true;
	}
    }
  }
  return false;
}
#endif
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment function for individual limit idle times defects.                */
/*                                                                           */
/*  This function tries each ejecting meet of a meet assigned the monitor's  */
/*  resource to a time that causes it to overlap with an idle time.          */
/*                                                                           */
/*****************************************************************************/
#if WITH_OLD_LIMIT_IDLE_REPAIR
#else
static void KheLimitIdleTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_MEET meet;  KHE_RESOURCE r;
  KHE_OPTIONS_KEMPE kempe;  bool not_ejecting;
  int i, index, random_offset, junk, count;
  KHE_SOLN soln;  KHE_TASK task;  KHE_TIME_GROUP_MONITOR tgm;
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT litc;
  KHE_TIME extreme_busy_times[2]; int extreme_busy_times_count;

  KheDebugAugmentBegin(DEBUG4, ej, d, "KheLimitIdleTimesAugment");
  if( KheEjectorRepairTimes(ej) )
  {
    /* simple repairs */
    soln = KheEjectorSoln(ej);
    litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) d;
    /* ejecting = KheEjectorEjectingNotBasic(ej); */
    not_ejecting = ao->no_ejecting_moves;
    kempe = KheEjectorUseKempeMoves(ej);
    random_offset = 3 * KheEjectorCurrAugmentCount(ej) +
      29 * KheSolnDiversifier(KheEjectorSoln(ej));
    r = KheLimitIdleTimesMonitorResource(litm);
    for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
    {
      index = (random_offset + i) % KheResourceAssignedTaskCount(soln, r);
      task = KheResourceAssignedTask(soln, r, index);
      meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
      if( meet != NULL && KheMeetMultiRepair(ej, meet, true,
	    &KheLimitIdleMeetMoveFn, (void *) litm, kempe, !not_ejecting,
	    not_ejecting, false) )
      {
	KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment", true);
	return true;
      }
    }

    /* complex repairs, only tried at depth 1 (currently not at all) */
    if( KheEjectorCurrLength(ej) == 1 && false )
    {
      litc = KheLimitIdleTimesMonitorConstraint(litm);
      count = KheLimitIdleTimesConstraintTimeGroupCount(litc);
      for( i = 0;  i < count;  i++ )
      {
        index = (random_offset + i) % count;
	tg = KheLimitIdleTimesConstraintTimeGroup(litc, index);
	KheLimitIdleTimesMonitorTimeGroupState(litm, index, &busy_count,
	  &idle_count, extreme_busy_times, &extreme_busy_times_count);
	if( idle_count > 0 )
	{
	  /* ***
	  KheSolnNewG lobalVisit(KheMonitorSoln(d));
	  *** */
	  if( KheLimitIdleComplexAugment(ej, litm, tg) )
	  {
	    KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment (c)",
	      true);
	    return true;
	  }
	}
      }
    }
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment", false);
  return false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleMeetBoundAugment(KHE_EJECTOR ej, KHE_MEET last_meet,    */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR litm, KHE_TIME_GROUP_MONITOR tgm)         */
/*                                                                           */
/*  Try unassigning last_meet and reducing the domains of all litm's meets.  */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
static bool KheLimitIdleMeetBoundAugment(KHE_EJECTOR ej,
  /* KHE_MEET last_meet, */ KHE_LIMIT_IDLE_TIMES_MONITOR litm, int index)
{
  KHE_SOLN soln;  int i, j, count, num, junk, busy_count, idle_count;
  KHE_MEET meet;  KHE_TASK task;  KHE_RESOURCE r;
  KHE_TIME times[2];  KHE_TIME_GROUP tg, include_tg;  bool success;
  r = KheLimitIdleTimesMonitorResource(litm);
  soln = KheEjectorSoln(ej);
  if( DEBUG16 )
  {
    fprintf(stderr, "[ KheLimitIdleMeetBoundAugment(ej, ");
    /* KheMeetDebug(last_meet, 1, -1, stderr); */
    fprintf(stderr, ", litm(%s), %d)\n", KheResourceId(r), index);
    for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
    {
      task = KheResourceAssignedTask(soln, r, i);
      meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
      if( meet != NULL )
	KheMeetDebug(meet, 2, 2, stderr);
    }
  }

  /* build a time group containing all times not mentioned in litm */
  KheSolnTimeGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupCount(litm);  i++ )
  {
    tg = KheLimitIdleTimesMonitorTimeGroup(litm, i);
    KheSolnTimeGroupDifference(soln, tg);
  }

  /* add the busy spans of litm's time groups, except the last time of the */
  /* index'th time group */
  for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupCount(litm);  i++ )
  {
    tg = KheLimitIdleTimesMonitorTimeGroupState(litm, i, &busy_count,
      &idle_count, times, &count);
    if( count >= 1 )
    {
      if( DEBUG16 )
      {
	fprintf(stderr, "  within ");
	KheTimeGroupDebug(tg, 1, -1, stderr);
	fprintf(stderr, " busy span %s .. %s\n", KheTimeId(times[0]),
	  KheTimeId(times[count - 1]));
      }
      num = KheTimeIndex(times[count-1]) - KheTimeIndex(times[0]) + 1;
      if( i == index )
	num--;
      for( j = 0;  j < num;  j++ )
        KheSolnTimeGroupAddTime(soln, KheTimeNeighbour(times[0], j));
    }
  }
  include_tg = KheSolnTimeGroupEnd(soln);
  if( DEBUG16 )
  {
    fprintf(stderr, "  include_tg: ");
    KheTimeGroupDebug(include_tg, 1, 0, stderr);
  }

  /* apply the meet bound repair */
  success = KheMeetBoundRepair(ej, r, include_tg);
  if( DEBUG16 )
    fprintf(stderr, "] KheLimitIdleMeetBoundAugment ret %s (end)\n",
      success ? "true" : "false");
  return success;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupMonitorHasSoleLastMeet(KHE_TIME_GROUP_MONITOR tgm,      */
/*    KHE_MEET *last_meet)                                                   */
/*                                                                           */
/*  If the last busy time of tgm is occupied by a single meet, set           */
/*  *last_meet to that meet and return true.  Otherwise return false.        */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupHasSoleLastMeet(KHE_LIMIT_IDLE_TIMES_MONITOR litm,
  KHE_TIME_GROUP tg, KHE_TIME last_busy_time, KHE_MEET *last_meet)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_RESOURCE r;
  r = KheLimitIdleTimesMonitorResource(litm);
  rtm = KheResourceTimetableMonitor(KheMonitorSoln((KHE_MONITOR) litm), r);
  if( KheResourceTimetableMonitorTimeTaskCount(rtm, last_busy_time) == 1 )
  {
    *last_meet =
      KheTaskMeet(KheResourceTimetableMonitorTimeTask(rtm, last_busy_time, 0));
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d) old version */
/*                                                                           */
/*  Augment function for individual limit idle times defects.                */
/*                                                                           */
/*  This function tries each meet move whose initial meet move moves a       */
/*  meet from the start or end of a day (actually, a time group of the       */
/*  monitor) in any way that reduces the number of idle times.               */
/*                                                                           */
/*****************************************************************************/

#if WITH_OLD_LIMIT_IDLE_REPAIR
static bool KheLimitIdleTimesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR litm;  KHE_MEET meet, last_meet;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  bool not_ejecting;  KHE_TIME_GROUP tg;
  KHE_TIME time;  int i, j, k, index, random_offset, durn;
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT litc;  int tg_count;
  int busy_count, idle_count;
  KHE_TIME times[2]; int times_count;

  KheDebugAugmentBegin(DEBUG4, ej, d, "KheLimitIdleTimesAugment");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    /* simple repairs */
    litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) d;
    litc = KheLimitIdleTimesMonitorConstraint(litm);
    tg_count = KheLimitIdleTimesConstraintTimeGroupCount(litc);
    rtm = KheResourceTimetableMonitor(KheEjectorSoln(ej),
      KheLimitIdleTimesMonitorResource(litm));
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    random_offset = 7 * KheEjectorCurrAugmentCount(ej) +
      59 * KheSolnDiversifier(KheEjectorSoln(ej));
    for( durn = 1;  durn <= 4;  durn++ )
    {
      for( i = 0;  i < tg_count;  i++ )
      {
	index = (random_offset + i) % tg_count;
	KheLimitIdleTimesMonitorTimeGroupState(litm, index, &busy_count,
	  &idle_count, times, &times_count);
	/* ***
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewG lobalVisit(KheMonitorSoln(d));
	*** */
	for( j = 0;  j < times_count;  j++ )
	{
	  time = times[j];
	  for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, time);  k++ )
	  {
	    meet = KheTaskMeet(KheResourceTimetableMonitorTimeTask(rtm,time,k));
	    if( KheMeetDuration(meet) == durn ||
		(durn == 4 && KheMeetDuration(meet) >= durn) )
	    {
	      if( KheMeetMultiRepair(ej, ao, meet, true,
		  &KheLimitIdleMeetMoveFn, (void *) litm,
		  KHE_OPTIONS_KEMPE_TRUE, !not_ejecting, not_ejecting, false) )
	      {
		KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment",
		  true);
		return true;
	      }
	    }
	  }
	}
      }
    }

    /* meet bound repairs, only tried at depth 1 (currently not at all) */
    if( KheEjectorCurrLength(ej) == 1 && false )
      for( i = 0;  i < tg_count;  i++ )
      {
        index = (random_offset + i) % tg_count;
	tg = KheLimitIdleTimesMonitorTimeGroupState(litm, index, &busy_count,
	  &idle_count, times, &times_count);
	if( idle_count > 0 && KheTimeGroupHasSoleLastMeet(litm, tg,
	      times[times_count - 1], &last_meet)
	    /* KheTimeGroupMonitorHasSoleLastMeet(tgm, &last_meet) */ )
	{
	  KheSolnNewGlobalVisit(KheEjectorSoln(ej));
	  if( KheLimitIdleMeetBoundAugment(ej, /* last_meet, */ litm, index) )
	  {
	    KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment (d)",
	      true);
	    return true;
	  }
	}
      }

    /* complex repairs, only tried at depth 1 (currently not at all) */
    if( KheEjectorCurrLength(ej) == 1 && false )
      for( i = 0;  i < tg_count;  i++ )
      {
        index = (random_offset + i) % tg_count;
	tg = KheLimitIdleTimesConstraintTimeGroup(litc, index);
	KheLimitIdleTimesMonitorTimeGroupState(litm, index, &busy_count,
	  &idle_count, times, &times_count);
	if( idle_count > 0 )
	{
	  /* ***
	  KheSolnNewGl obalVisit(KheEjectorSoln(ej));
	  *** */
	  if( KheLimitIdleComplexAugment(ej, litm, tg) )
	  {
	    KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment (c)",
	      true);
	    return true;
	  }
	}
      }
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitIdleTimesAugment", false);
  return false;
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)        */
/*                                                                           */
/*  Augment function for groups of limit idle times defects.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitIdleTimesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheLimitIdleTimesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheLimitIdleTimesGroupAugment internal error 2");
  return KheLimitIdleTimesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cluster busy times monitor augment functions"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterUnderloadMultiRepair(KHE_EJECTOR ej,                      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,           */
/*    int active_count, bool allow_zero)                                     */
/*                                                                           */
/*  Repair an underload defect in cbtm: make an inactive time group active.  */
/*  Here active_count is the number of active time groups.                   */
/*  Or if allow_zero is true, also try reducing to zero.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterUnderloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  int active_count, bool allow_zero)
{
  int i, ix, random_offset, count, busy;  KHE_RESOURCE r;  KHE_TIME_GROUP tg;
  KHE_POLARITY po;
  r = KheClusterBusyTimesMonitorResource(cbtm);
  if( DEBUG7 )
    fprintf(stderr, "%*s[ KheClusterUnderloadMultiRepair(ej, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r));
  random_offset = 17 * KheEjectorCurrAugmentCount(ej) +
    19 * KheSolnDiversifier(KheEjectorSoln(ej));
  count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    if( !KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po,&busy) )
    {
      if( DEBUG7 )
      {
	fprintf(stderr, "  trying time group ");
	KheTimeGroupDebug(tg, 1, 0, stderr);
      }
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_NEGATIVE, true,false) )
      {
	if( DEBUG7 )
	  fprintf(stderr, "%*s] KheClusterUnderloadMultiRepair ret true\n",
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
    }
    else if( allow_zero && active_count == 1 )
    {
      /* try to reduce the number of active time groups to zero */
      if( DEBUG7 )
      {
	fprintf(stderr, "  trying allow_zero time group ");
	KheTimeGroupDebug(tg, 1, 0, stderr);
      }
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE, true,false) )
      {
	if( DEBUG7 )
	  fprintf(stderr, "%*s] KheClusterUnderloadMultiRepair ret true\n",
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
    }
    if( RANDOMIZED )
      return false;
  }
  if( DEBUG7 )
    fprintf(stderr, "%*s] KheClusterUnderloadMultiRepair ret false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheClusterMeetsUnassignTimeGroup(                         */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_TIME_GROUP tg)                */
/*                                                                           */
/*  Return a time group that is the cycle minus every time group of cbtm     */
/*  that has no meets, and minus tg as well.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP KheClusterMeetsUnassignTimeGroup(
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_TIME_GROUP tg)
{
  KHE_SOLN soln;  int i, tg_count, busy_count;  KHE_POLARITY po;
  KHE_TIME_GROUP tg2;
  soln = KheMonitorSoln((KHE_MONITOR) cbtm);
  KheSolnTimeGroupBegin(soln);
  KheSolnTimeGroupUnion(soln, KheInstanceFullTimeGroup(KheSolnInstance(soln)));
  tg_count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  for( i = 0;  i < tg_count;  i++ )
  {
    KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i, &tg2, &po,&busy_count);
    if( busy_count == 0 )
      KheSolnTimeGroupDifference(soln, tg2);
  }
  KheSolnTimeGroupDifference(soln, tg);
  return KheSolnTimeGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterOverloadMultiRepair(KHE_EJECTOR ej,                       */
/*    KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)           */
/*                                                                           */
/*  Repair an overload defect in cbtm, by making an active time group        */
/*  inactive.                                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterOverloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)
{
  int i, busy, ix, tg_count, count, random_offset;
  KHE_TIME_GROUP tg, include_tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_POLARITY po;  KHE_RESOURCE r;  KHE_SOLN soln;
  if( DEBUG28 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheClusterOverloadMultiRepair(times %s, depth %d)\n",
      KheEjectorCurrDebugIndent(ej), "",
      ao->repair_times ? "true" : "false", KheEjectorCurrLength(ej));
  r = KheClusterBusyTimesMonitorResource(cbtm);

  /* underload/overload repair similar to repairing underloads */
  random_offset = 83 * KheEjectorCurrAugmentCount(ej) +
    7 * KheSolnDiversifier(KheEjectorSoln(ej));
  count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po, &busy) )
    {
      /* this time group is active */
      if( DEBUG28 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s  KheClusterOverloadMultiRepair active %s\n",
	  KheEjectorCurrDebugIndent(ej), "", KheTimeGroupId(tg));
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE, true,false) )
      {
	if( DEBUG28 && KheEjectorCurrDebug(ej) )
	  fprintf(stderr, "%*s] KheClusterOverloadMultiRepair ret true\n",
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
      if( RANDOMIZED )
	return false;
    }
  }

  /* if meet moves are allowed, try a meet bound repair as last resort */
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    soln = KheEjectorSoln(ej);
    rtm = KheResourceTimetableMonitor(soln, r);
    tg_count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
    random_offset = 5 * KheEjectorCurrAugmentCount(ej) +
      13 * KheSolnDiversifier(KheEjectorSoln(ej));
    for( i = 0;  i < tg_count;  i++ )
    {
      ix = (random_offset + i) % tg_count;
      KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po, &busy);
      if( po == KHE_POSITIVE && busy > 0 )
      {
	if( KheEjectorCurrLength(ej) == 1 || KheOneMeet(rtm, tg) )
	{
	  if( KheEjectorCurrLength(ej) == 1 )
	    KheSolnNewGlobalVisit(soln);
	  include_tg = KheClusterMeetsUnassignTimeGroup(cbtm, tg);
	  if( KheMeetBoundRepair(ej, r, include_tg) )
	  {
	    if( DEBUG28 && KheEjectorCurrDebug(ej) )
	      fprintf(stderr, "%*s] KheClusterOverloadMultiRepair ret true "
		"(1)\n", KheEjectorCurrDebugIndent(ej), "");
	    return true;
	  }
	  if( RANDOMIZED )
	    return false;
	}
      }
    }
  }
  if( DEBUG28 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheClusterOverloadMultiRepair ret false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)           */
/*                                                                           */
/*  Augment function for individual cluster busy times defects.              */
/*                                                                           */
/*  Use KheOverUnderMultiRepair to make inactive time groups active (if      */
/*  there is an underload) or active time groups inactive (if there is an    */
/*  overload).  If repairing times, also try a more complex ejection         */
/*  tree repair as a last resort.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  int count, ind_count, minimum, maximum;  bool allow_zero, success;
  KheDebugAugmentBegin(DEBUG14, ej, d, "KheClusterBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    "KheClusterBusyTimesAugment internal error 1");
  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) d;
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &count,
    &ind_count, &minimum, &maximum, &allow_zero);
  success = false;
  if( count < minimum )
    success = KheClusterUnderloadMultiRepair(ej, ao, cbtm, count, allow_zero);
  else if( count > maximum )
    success = KheClusterOverloadMultiRepair(ej, ao, cbtm);
  else
    HnAbort("KheClusterBusyTimesAugment internal error 2");
  KheDebugAugmentEnd(DEBUG14, ej, d, "KheClusterBusyTimesAugment", success);
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)      */
/*                                                                           */
/*  Augment function for groups of cluster busy times defects.               */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterBusyTimesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheClusterBusyTimesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheClusterBusyTimesGroupAugment internal error 2");
  return KheClusterBusyTimesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit busy times monitor augment functions"                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment function for individual limit busy times defects.                */
/*                                                                           */
/*  Use KheOverUnderMultiRepair to increase or decrease the number of busy   */
/*  times in each time group as required.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitBusyTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;  KHE_TIME_GROUP tg;  KHE_RESOURCE r;
  bool allow_zero;
  int i, ix, count, random_offset, busy_count, minimum, maximum;
  KheDebugAugmentBegin(DEBUG4, ej, d, "KheLimitBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG,
    "KheLimitBusyTimesAugment internal error 1");
  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) d;
  r = KheLimitBusyTimesMonitorResource(lbtm);
  count = KheLimitBusyTimesMonitorDefectiveTimeGroupCount(lbtm);
  random_offset = 47 * KheEjectorCurrAugmentCount(ej) +
    53 * KheSolnDiversifier(KheEjectorSoln(ej));
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    KheLimitBusyTimesMonitorDefectiveTimeGroup(lbtm, ix, &tg, &busy_count,
      &minimum, &maximum, &allow_zero);
    if( KheOverUnderMultiRepair(ej, ao, r, tg, busy_count > maximum, false,
	  allow_zero) )
    {
      KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitBusyTimesAugment", true);
      return true;
    }
    if( RANDOMIZED )
      return false;
  }
  KheDebugAugmentEnd(DEBUG4, ej, d, "KheLimitBusyTimesAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)        */
/*                                                                           */
/*  Augment function for groups of limit busy times defects.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitBusyTimesGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheLimitBusyTimesGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheLimitBusyTimesGroupAugment internal error 2");
  return KheLimitBusyTimesAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit workload monitor augment functions"                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadAugment(KHE_EJECTOR ej, KHE_MONITOR d)              */
/*                                                                           */
/*  Augment function for individual limit workload defects.                  */
/*                                                                           */
/*  Use KheOverUnderMultiRepair to increase or decrease the number of busy   */
/*  times in each time group as required.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitWorkloadAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_LIMIT_WORKLOAD_MONITOR lwm;  KHE_RESOURCE r;
  int i, ix, count, random_offset, minimum, maximum;
  KHE_TIME_GROUP tg;  float workload;  bool allow_zero;
  KheDebugAugmentBegin(DEBUG3, ej, d, "KheLimitWorkloadAugment");
  lwm = (KHE_LIMIT_WORKLOAD_MONITOR) d;
  r = KheLimitWorkloadMonitorResource(lwm);
  tg = KheInstanceFullTimeGroup(KheResourceInstance(r));
  count = KheLimitWorkloadMonitorDefectiveTimeGroupCount(lwm);
  random_offset = 23 * KheEjectorCurrAugmentCount(ej) +
    73 * KheSolnDiversifier(KheEjectorSoln(ej));
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    KheLimitWorkloadMonitorDefectiveTimeGroup(lwm, ix, &tg, &workload,
      &minimum, &maximum, &allow_zero);
    if( KheOverUnderMultiRepair(ej, ao, r, tg, workload > maximum,false,false) )
    {
      KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitWorkloadAugment (overload)",
	true);
      return true;
    }
    if( RANDOMIZED )
      return false;
  }
  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitWorkloadAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)         */
/*                                                                           */
/*  Augment function for groups of limit workload defects.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitWorkloadGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheLimitWorkloadGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheLimitWorkloadGroupAugment internal error 2");
  return KheLimitWorkloadAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "limit active intervals monitor augment functions"             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsAugment(KHE_EJECTOR ej, KHE_MONITOR d)       */
/*                                                                           */
/*  Augment function for individual limit active intervals defects.          */
/*                                                                           */
/*  For each defective sequence, if it is too long, use                      */
/*  KheOverUnderMultiRepair to try to make one of its time groups inactive,  */
/*  preferably the first or last.  If it is too short, first use             */
/*  KheOverUnderMultiRepair to try to remove it altogether, then use         */
/*  KheOverUnderMultiRepair to try to lengthen it at each end.               */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitActiveIntervalsAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_POLARITY po;  KHE_TIME_GROUP tg;  int busy;  KHE_RESOURCE r;
  int min_lim, len, j;
  int i, history, first_index, last_index, history_after, random_offset, count;
  bool too_long;
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
    "KheLimitActiveIntervalsAugment internal error 1");
  KheDebugAugmentBegin(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment");
  laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) d;
  random_offset = 89 * KheEjectorCurrAugmentCount(ej) +
    29 * KheSolnDiversifier(KheEjectorSoln(ej));
  count = KheLimitActiveIntervalsMonitorDefectiveIntervalCount(laim);
  r = KheLimitActiveIntervalsMonitorResource(laim);
  for( i = 0;  i < count;  i++ )
  {
    KheLimitActiveIntervalsMonitorDefectiveInterval(laim,
      (random_offset + i) % count, &history, &first_index,
      &last_index, &history_after, &too_long);
    if( last_index < 0 )
      continue;
    if( too_long )
    {
      /* try to make the first time group inactive */
      KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	&tg, &po, &busy);
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE, true,false) )
      {
	KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	  true);
	return true;
      }

      /* try to make the last time group inactive, if it's not the first */
      if( last_index != first_index )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_POSITIVE,true,false) )
	{
	  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	  return true;
	}
      }

      /* as a last resort, try the time groups in between */
      for( j = first_index + 1;  j < last_index;  j++ )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, j,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_POSITIVE,true,false) )
	{
	  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	  return true;
	}
      }
    }
    else
    {
      /* if len <= 2, try to remove the interval altogether */
      /* NB repair will be enhanced if sequence length is 2 */
      min_lim = KheLimitActiveIntervalsMonitorMinimum(laim);
      len = history + (last_index - first_index + 1) + history_after;
      HnAssert(min_lim>len,"KheLimitActiveIntervalsAugment internal error 3");
      if( 1 <= len && len <= 2 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_POSITIVE,true,false) )
	{
	  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	  return true;
	}
      }

      /* try to make the interval longer, by extending at the left end */
      if( first_index > 0 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index - 1,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_NEGATIVE,true,false) )
	{
	  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	  return true;
	}
      }

      /* try to make the interval longer, by extending at the right end */
      if( last_index < KheLimitActiveIntervalsMonitorTimeGroupCount(laim) - 1 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index + 1,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_NEGATIVE,true,false) )
	{
	  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	  return true;
	}
      }
    }
    if( RANDOMIZED )
      return false;
  }
  KheDebugAugmentEnd(DEBUG3, ej, d, "KheLimitActiveIntervalsAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitActiveIntervalsGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)  */
/*                                                                           */
/*  Augment function for groups of limit active intervals defects.           */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitActiveIntervalsGroupAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheLimitActiveIntervalsGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheLimitActiveIntervalsGroupAugment internal error 2");
  return KheLimitActiveIntervalsAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ordinary and workload demand monitor augment functions"       */
/*                                                                           */
/*  Implementation note.  Ordinary and workload demand augments are          */
/*  treated the same, because for them it is the set of competitors which    */
/*  is the true defect, and it can have demand monitors of both kinds.       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandDefectIsWorkloadOverload(KHE_MONITOR d, KHE_RESOURCE *r,   */
/*    KHE_TIME_GROUP *tg)                                                    */
/*                                                                           */
/*  If defect d has a workload demand defect competitor, set *r to the       */
/*  originating monitor's resource and *tg to its time group, and return     */
/*  true.  Otherwise return false.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandDefectIsWorkloadOverload(KHE_MONITOR d, KHE_RESOURCE *r,
  KHE_TIME_GROUP *tg)
{
  int i;  KHE_MONITOR m;  KHE_WORKLOAD_DEMAND_MONITOR wdm;  KHE_SOLN soln;
  soln = KheMonitorSoln(d);
  KheSolnMatchingSetCompetitors(soln, d);
  for( i = 0;  i < KheSolnMatchingCompetitorCount(soln);  i++ )
  {
    m = KheSolnMatchingCompetitor(soln, i);
    if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    {
      wdm = (KHE_WORKLOAD_DEMAND_MONITOR) m;
      *r = KheWorkloadDemandMonitorResource(wdm);
      *tg = KheWorkloadDemandMonitorTimeGroup(wdm);
      return true;
    }
  }
  *r = NULL;
  *tg = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandAugment(KHE_EJECTOR ej, KHE_MONITOR d)                     */
/*                                                                           */
/*  Augment function for individual ordinary or workload demand defects.     */
/*                                                                           */
/*  If the defect has a workload demand defect competitor, try repairs       */
/*  documented in the header of KheResourceLoadDefectAugment, which in this  */
/*  case means meet moves out of the domain of the workload monitor's        */
/*  originating monitor.                                                     */
/*                                                                           */
/*  If the defect has no workload demand defect competitor, try moving       */
/*  each of clashing meets away.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_TIME_GROUP tg;  KHE_MONITOR m;  KHE_MEET meet;  bool not_ejecting;
  KHE_OPTIONS_KEMPE kempe;  int i;  KHE_SOLN soln;  KHE_RESOURCE r;
  ARRAY_KHE_MEET meets;  KHE_ORDINARY_DEMAND_MONITOR odm;

  /* boilerplate */
  KheDebugAugmentBegin(DEBUG6, ej, d, "KheDemandAugment");
  HnAssert(KheMonitorTag(d) == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
          KheMonitorTag(d) == KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    "KheDemandAugment internal error 1");
  soln = KheEjectorSoln(ej);
  /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
  not_ejecting = ao->no_ejecting_moves;
  /* kempe = KheEjectorUseKempeMoves(ej); */
  kempe = ao->use_kempe_moves;

  if( KheDemandDefectIsWorkloadOverload(d, &r, &tg) )
  {
    /* defect is workload overload of r in tg, so handle appropriately */
    if( KheResourceOverloadMultiRepair(ej, ao, r, tg, false) )
    {
      if( DEBUG6 )
	fprintf(stderr, "%*s] KheDemandAugment ret true: %.5f\n",
	  KheEjectorCurrDebugIndent(ej) + 2, "", KheCostShow(KheSolnCost(soln)));
      return true;
    }
  }
  /* else if( KheEjectorRepairTimes(ej) ) */
  else if( ao->repair_times )
  {
    /* defect is ordinary overload; get meets */
    /* KheDemandDefectIsWorkloadOverload calls KheSolnMatchingSetCompetitors */
    HaArrayInit(meets, KheEjectorArena(ej));
    for( i = 0;  i < KheSolnMatchingCompetitorCount(soln);  i++ )
    {
      m = KheSolnMatchingCompetitor(soln, i);
      HnAssert(KheMonitorTag(m) == KHE_ORDINARY_DEMAND_MONITOR_TAG,
	"KheDemandAugment internal error 1");
      odm = (KHE_ORDINARY_DEMAND_MONITOR) m;
      meet = KheTaskMeet(KheOrdinaryDemandMonitorTask(odm));
      HaArrayAddLast(meets, meet);
    }
    if( DEBUG6 )
      fprintf(stderr, "  KheDemandAugment: %d meets\n", HaArrayCount(meets));

    /* move meets */
    /* MArraySort(meets, &KheMeetIncreasingDemandCmp); */
    HaArrayForEach(meets, meet, i)
    {
      /* this call to KheMeetMultiRepair excludes moves at vizier nodes */
      if( KheMeetMultiRepair(ej, ao, meet, false, NULL, NULL, kempe,
	  !not_ejecting, not_ejecting, WITH_DEMAND_NODE_SWAPS) )
      {
	if( DEBUG6 )
	  fprintf(stderr, "%*s] KheDemandAugment ret true: %.5f\n",
	    KheEjectorCurrDebugIndent(ej) + 2, "",
	    KheCostShow(KheSolnCost(soln)));
	HaArrayFree(meets);
	return true;
      }
    }
    HaArrayFree(meets);
  }

  /* no luck */
  KheDebugAugmentEnd(DEBUG6, ej, d, "KheDemandAugment", false);
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDemandGroupAugment(KHE_EJECTOR ej, KHE_MONITOR d)                */
/*                                                                           */
/*  Augment function for groups of ordinary or workload demand defects.      */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandGroupAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_GROUP_MONITOR gm;
  HnAssert(KheMonitorTag(d) == KHE_GROUP_MONITOR_TAG,
    "KheDemandGroupAugment internal error 1");
  gm = (KHE_GROUP_MONITOR) d;
  HnAssert(KheGroupMonitorDefectCount(gm) > 0,
    "KheDemandGroupAugment internal error 2");
  return KheDemandAugment(ej, ao, KheGroupMonitorDefect(gm, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR KheEjectionChainEjectorMake(KHE_OPTIONS options)             */
/*                                                                           */
/*  Make an ejector object.                                                  */
/*                                                                           */
/*****************************************************************************/

KHE_EJECTOR KheEjectionChainEjectorMake(KHE_OPTIONS options, HA_ARENA a)
{
  KHE_EJECTOR res;  /* char *active_augment; */
  /* KHE_ACTIVE_AUGMENT_TYPE active_augment; */

  /* ejector and schedules */
  res = KheEjectorMakeBegin(KheOptionsGet(options, "es_schedules", "1+;u-"), a);
  /* ***
  KheEjectorSetSchedulesFromString(res,
    KheOptionsGet(options, "es_schedules", "1+,u-"));
  *** */

  /* augment types - event monitors */
  KheEjectorAddAugmentType(res, KHE_AUGMENT_ORDINARY_DEMAND,
    "Ordinary demand");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_WORKLOAD_DEMAND,
    "Workload demand");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_SPLIT_EVENTS,
    "Split events");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_ASSIGN_TIME,
    "Assign time");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_PREFER_TIMES,
    "Prefer times");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_SPREAD_EVENTS,
    "Spread events");
  /* no function for link events defects */
  KheEjectorAddAugmentType(res, KHE_AUGMENT_ORDER_EVENTS,
    "Order events");

  /* augment types - event resource monitors */
  KheEjectorAddAugmentType(res, KHE_AUGMENT_ASSIGN_RESOURCE,
    "Assign resource");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_PREFER_RESOURCES,
    "Prefer resources");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_AVOID_SPLIT_ASSIGNMENTS,
    "Avoid split assts");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_LIMIT_RESOURCES,
    "Limit resources");

  /* augment types - resource monitors */
  KheEjectorAddAugmentType(res, KHE_AUGMENT_AVOID_CLASHES,
    "Avoid clashes");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_AVOID_UNAVAILABLE_TIMES,
    "Avoid unavailable times");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_LIMIT_IDLE_TIMES,
    "Limit idle times");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_CLUSTER_BUSY_TIMES,
    "Cluster busy times");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_LIMIT_BUSY_TIMES,
    "Limit busy times");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_LIMIT_WORKLOAD,
    "Limit workload");
  KheEjectorAddAugmentType(res, KHE_AUGMENT_LIMIT_ACTIVE_INTERVALS,
    "Limit active intervals");

  /* repair types - meet moves and assignments */
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_MOVE_UNCHECKED,
    "Unchecked meet move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_MOVE_CHECKED,
    "Checked meet move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_MOVE_EJECTING,
    "Ejecting meet move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_MOVE_KEMPE,
    "Kempe meet move");
  KheEjectorAddRepairType(res, KHE_REPAIR_FUZZY_MEET_MOVE,
    "Fuzzy meet move");

  /* repair types - task moves and assignments */
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_DOUBLE_MOVE,
    "Task double move");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_SET_MOVE,
    "Task set move");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_SET_DOUBLE_MOVE,
    "Task set double move");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_SET_REPLACE,
    "Task set replace");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_SET_DOUBLE_REPLACE,
    "Task set double replace");
  KheEjectorAddRepairType(res, KHE_REPAIR_RESOURCE_MOVE,
    "Resource move");

  /* repair types - combined operations */
  KheEjectorAddRepairType(res, KHE_REPAIR_NODE_MEET_SWAP,
    "Node meet swap");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_SPLIT,
    "Meet split");
  KheEjectorAddRepairType(res, KHE_REPAIR_SPLIT_MOVE,
    "Split move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MERGE_MOVE,
    "Merge move");
  KheEjectorAddRepairType(res, KHE_REPAIR_SPLIT_TASKS_UNASSIGN,
    "Split tasks unassign and reduce");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_BOUND,
    "Meet bound");
  KheEjectorAddRepairType(res,
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN,
    "Kempe meet move + ejecting task assignment");
  KheEjectorAddRepairType(res,
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE,
    "Kempe meet move + ejecting task move");
  KheEjectorAddRepairType(res, KHE_REPAIR_COMPLEX_IDLE_MOVE,
    "Complex idle move");

  /* augment functions - demand monitors (and groups thereof) */
  KheEjectorAddAugment(res, KHE_ORDINARY_DEMAND_MONITOR_TAG,
    &KheDemandAugment, KHE_AUGMENT_ORDINARY_DEMAND);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_ORDINARY_DEMAND,
    &KheDemandGroupAugment, KHE_AUGMENT_ORDINARY_DEMAND);

  KheEjectorAddAugment(res, KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    &KheDemandAugment, KHE_AUGMENT_WORKLOAD_DEMAND);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_WORKLOAD_DEMAND,
    &KheDemandGroupAugment, KHE_AUGMENT_WORKLOAD_DEMAND);

  /* augment functions - event monitors (and groups thereof) */
  KheEjectorAddAugment(res, KHE_SPLIT_EVENTS_MONITOR_TAG,
    &KheSplitEventsAugment, KHE_AUGMENT_SPLIT_EVENTS);
  KheEjectorAddAugment(res, KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG,
    &KheSplitEventsAugment, KHE_AUGMENT_SPLIT_EVENTS);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_SPLIT_EVENTS,
    &KheSplitEventsGroupAugment, KHE_AUGMENT_SPLIT_EVENTS);

  KheEjectorAddAugment(res, KHE_ASSIGN_TIME_MONITOR_TAG,
    &KheAssignTimeAugment, KHE_AUGMENT_ASSIGN_TIME);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_ASSIGN_TIME,
    &KheAssignTimeGroupAugment, KHE_AUGMENT_ASSIGN_TIME);

  KheEjectorAddAugment(res, KHE_PREFER_TIMES_MONITOR_TAG,
    &KhePreferTimesAugment, KHE_AUGMENT_PREFER_TIMES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_PREFER_TIMES,
    &KhePreferTimesGroupAugment, KHE_AUGMENT_PREFER_TIMES);

  KheEjectorAddAugment(res, KHE_SPREAD_EVENTS_MONITOR_TAG,
    &KheSpreadEventsAugment, KHE_AUGMENT_SPREAD_EVENTS);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_SPREAD_EVENTS,
    &KheSpreadEventsGroupAugment, KHE_AUGMENT_SPREAD_EVENTS);

  KheEjectorAddAugment(res, KHE_ORDER_EVENTS_MONITOR_TAG,
    &KheOrderEventsAugment, KHE_AUGMENT_ORDER_EVENTS);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_ORDER_EVENTS,
    &KheOrderEventsGroupAugment, KHE_AUGMENT_ORDER_EVENTS);

  /* augment functions - event resource monitors (and groups thereof) */
  KheEjectorAddAugment(res, KHE_ASSIGN_RESOURCE_MONITOR_TAG,
    &KheAssignResourceAugment, KHE_AUGMENT_ASSIGN_RESOURCE);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_ASSIGN_RESOURCE,
    &KheAssignResourceGroupAugment, KHE_AUGMENT_ASSIGN_RESOURCE);

  KheEjectorAddAugment(res, KHE_PREFER_RESOURCES_MONITOR_TAG,
    &KhePreferResourcesAugment, KHE_AUGMENT_PREFER_RESOURCES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_PREFER_RESOURCES,
    &KhePreferResourcesGroupAugment, KHE_AUGMENT_PREFER_RESOURCES);

  KheEjectorAddAugment(res, KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG,
    &KheAvoidSplitAssignmentsAugment, KHE_AUGMENT_AVOID_SPLIT_ASSIGNMENTS);

  KheEjectorAddAugment(res, KHE_LIMIT_RESOURCES_MONITOR_TAG,
    &KheLimitResourcesAugment, KHE_AUGMENT_LIMIT_RESOURCES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_LIMIT_RESOURCES,
    &KheLimitResourcesGroupAugment, KHE_AUGMENT_LIMIT_RESOURCES);

  /* augment functions - resource monitors (and groups thereof) */
  KheEjectorAddAugment(res, KHE_AVOID_CLASHES_MONITOR_TAG,
    &KheAvoidClashesAugment, KHE_AUGMENT_AVOID_CLASHES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_AVOID_CLASHES,
    &KheAvoidClashesGroupAugment, KHE_AUGMENT_AVOID_CLASHES);

  KheEjectorAddAugment(res, KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
    &KheAvoidUnavailableTimesAugment, KHE_AUGMENT_AVOID_UNAVAILABLE_TIMES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_AVOID_UNAVAILABLE_TIMES,
    &KheAvoidUnavailableTimesGroupAugment, KHE_AUGMENT_AVOID_UNAVAILABLE_TIMES);

  KheEjectorAddAugment(res, KHE_LIMIT_IDLE_TIMES_MONITOR_TAG,
    &KheLimitIdleTimesAugment, KHE_AUGMENT_LIMIT_IDLE_TIMES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_LIMIT_IDLE_TIMES,
    &KheLimitIdleTimesGroupAugment, KHE_AUGMENT_LIMIT_IDLE_TIMES);

  KheEjectorAddAugment(res, KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    &KheClusterBusyTimesAugment, KHE_AUGMENT_CLUSTER_BUSY_TIMES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_CLUSTER_BUSY_TIMES,
    &KheClusterBusyTimesGroupAugment, KHE_AUGMENT_CLUSTER_BUSY_TIMES);

  KheEjectorAddAugment(res, KHE_LIMIT_BUSY_TIMES_MONITOR_TAG,
    &KheLimitBusyTimesAugment, KHE_AUGMENT_LIMIT_BUSY_TIMES);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_LIMIT_BUSY_TIMES,
    &KheLimitBusyTimesGroupAugment, KHE_AUGMENT_LIMIT_BUSY_TIMES);

  KheEjectorAddAugment(res, KHE_LIMIT_WORKLOAD_MONITOR_TAG,
    &KheLimitWorkloadAugment, KHE_AUGMENT_LIMIT_WORKLOAD);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_LIMIT_WORKLOAD,
    &KheLimitWorkloadGroupAugment, KHE_AUGMENT_LIMIT_WORKLOAD);

  KheEjectorAddAugment(res, KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
    &KheLimitActiveIntervalsAugment, KHE_AUGMENT_LIMIT_ACTIVE_INTERVALS);
  KheEjectorAddGroupAugment(res, KHE_SUBTAG_LIMIT_ACTIVE_INTERVALS,
    &KheLimitActiveIntervalsGroupAugment, KHE_AUGMENT_LIMIT_ACTIVE_INTERVALS);

  /* wrap up */
  KheEjectorMakeEnd(res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EJECTOR KheEjectionChainEjectorOption(KHE_OPTIONS options, char *key)*/
/*                                                                           */
/*  Obtain an ejector from options, or create one if not present.            */
/*                                                                           */
/*****************************************************************************/

KHE_EJECTOR KheEjectionChainEjectorOption(KHE_OPTIONS options, char *key)
{
  void *value;  KHE_EJECTOR res;
  value = KheOptionsGetObject(options, key, NULL);
  if( value != NULL )
    res = (KHE_EJECTOR) value;
  else
  {
    res = KheEjectionChainEjectorMake(options, KheOptionsArena(options));
    KheOptionsSetObject(options, key, (void *) res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectionChainNodeRepairTimes(KHE_NODE parent_node,               */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Use an ejection chain to repair the time assignments of the meets of     */
/*  the descendants of parent_node.  The repair operation are Kempe meet     */
/*  moves (required to preserve zones, where present) and node swaps if      */
/*  nodes lie in layers.  Return true if any progress was made.              */
/*                                                                           */
/*  The ejector used is es_ejector1 from options, which will be created      */
/*  if it does not already exist.                                            */
/*                                                                           */
/*  If the vizier_node option is true, insert and split a vizier node        */
/*  before starting work, and remove it afterwards.                          */
/*                                                                           */
/*****************************************************************************/

bool KheEjectionChainNodeRepairTimes(KHE_NODE parent_node, KHE_OPTIONS options)
{
  KHE_GROUP_MONITOR kempe_gm, start_gm, limit_gm;  KHE_NODE viz_node;
  KHE_SOLN soln;  KHE_EJECTOR ej;  KHE_AUGMENT_OPTIONS ao;
  bool res, /* vizier_node, */ no_node_regularity;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEjectionChainNodeRepairTimes(");
    KheNodeDebug(parent_node, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }

  /* set the control options and get ejector and augment options */
  KheOptionsSetBool(options, "es_repair_times", true);
  KheOptionsSetObject(options, "es_limit_node",
    KheNodeIsCycleNode(parent_node) ? NULL : parent_node);
  KheOptionsSetBool(options, "es_repair_resources", false);
  ej = KheEjectionChainEjectorOption(options, "es_ejector1");
  soln = KheNodeSoln(parent_node);
  ao = KheAugmentOptionsMake(soln, options, KheEjectorArena(ej));

  /* get some options used by this function */
  /* vizier_node = KheOptionsGetBool(options, "es_vizier_node", false); */
  no_node_regularity = KheOptionsGetBool(options, "ts_no_node_regularity",
    false);

  /* insert and split a vizier node, if required */
  if( ao->vizier_node )
  {
    viz_node = KheNodeVizierMake(parent_node);
    KheNodeMeetSplit(viz_node, false);
  }

  /* build the required group monitors and solve */
  KheGroupCorrelatedMonitors(soln, options);
  kempe_gm = KheKempeDemandGroupMonitorMake(soln);
  start_gm = KheNodeTimeRepairStartGroupMonitorMake(parent_node);
  limit_gm = KheGroupEventMonitors(soln, KHE_ASSIGN_TIME_MONITOR_TAG,
    KHE_SUBTAG_ASSIGN_TIME);
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial defects:\n");
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }
  KheEjectorSolveBegin(ej, start_gm, (KHE_GROUP_MONITOR) soln, ao, options);
  KheEjectorAddMonitorCostLimitReducing(ej, (KHE_MONITOR) limit_gm);
  res = KheEjectorSolveEnd(ej);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final defects:\n");
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }

  /* remove any vizier node and make sure the parent is fully zoned */
  if( ao->vizier_node )
  {
    KheNodeVizierDelete(parent_node);
    if( !no_node_regularity )
      KheNodeExtendZones(parent_node);
  }

  /* clean up, including deleting the group monitors, and return */
  KheGroupMonitorDelete(kempe_gm);
  KheGroupMonitorDelete(start_gm);
  KheGroupMonitorDelete(limit_gm);
  KheUnGroupCorrelatedMonitors(soln);
  if( DEBUG1 )
    fprintf(stderr, "] KheEjectionChainNodeRepairTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectionChainLayerRepairTimes(KHE_LAYER layer,                   */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Use an ejection chain to repair the time assignments of the meets of     */
/*  layer.  In other respects the same as KheEjectionChainNoderRepairTimes.  */
/*                                                                           */
/*  The ejector used is es_ejector1 from options, which will be created      */
/*  if it does not already exist.                                            */
/*                                                                           */
/*  If the es_vizier_node option is true, insert and split a vizier          */
/*  node before starting work, and remove it afterwards.                     */
/*                                                                           */
/*****************************************************************************/

bool KheEjectionChainLayerRepairTimes(KHE_LAYER layer, KHE_OPTIONS options)
{
  KHE_GROUP_MONITOR kempe_gm, start_gm, limit_gm;
  KHE_SOLN soln;  KHE_EJECTOR ej;  KHE_NODE parent_node, viz_node;
  bool res, /* vizier_node, */ no_node_regularity /*, layer_repair_long */;
  KHE_AUGMENT_OPTIONS ao;

  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEjectionChainLayerRepairTimes(");
    KheLayerDebug(layer, 1, -1, stderr);
    fprintf(stderr, ")\n");
    KheLayerDebug(layer, 2, 2, stderr);
  }

  /* set the control options and get ejector and augment options */
  parent_node = KheLayerParentNode(layer);
  KheOptionsSetBool(options, "es_repair_times", true);
  KheOptionsSetObject(options, "es_limit_node",
    KheNodeIsCycleNode(parent_node) ? NULL : parent_node);
  KheOptionsSetBool(options, "es_repair_resources", false);
  soln = KheLayerSoln(layer);
  ej = KheEjectionChainEjectorOption(options, "es_ejector1");
  ao = KheAugmentOptionsMake(soln, options, KheEjectorArena(ej));

  /* get some options used by this function */
  /* vizier_node = KheOptionsGetBool(options, "es_vizier_node", false); */
  no_node_regularity = KheOptionsGetBool(options, "ts_no_node_regularity",
    false);
  /* ***
  layer_repair_long = KheOptionsGetBool(options, "es_layer_repair_long", false);
  *** */

  /* get an ejector */

  /* set the control options */

  /* insert and split a vizier node, if required */
  if( ao->vizier_node )
  {
    viz_node = KheNodeVizierMake(parent_node);
    KheNodeMeetSplit(viz_node, false);
  }

  /* build the required group monitors and solve */
  KheGroupCorrelatedMonitors(soln, options);
  kempe_gm = KheKempeDemandGroupMonitorMake(soln);
  if( ao->layer_repair_long )
    start_gm = KheLayerTimeRepairLongStartGroupMonitorMake(layer);
  else
    start_gm = KheLayerTimeRepairStartGroupMonitorMake(layer);
  limit_gm = KheGroupEventMonitors(soln, KHE_ASSIGN_TIME_MONITOR_TAG,
    KHE_SUBTAG_ASSIGN_TIME);
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial defects (soln cost %.5f):\n",
      KheCostShow(KheSolnCost(soln)));
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }
  KheEjectorSolveBegin(ej, start_gm, (KHE_GROUP_MONITOR) soln, ao, options);
  KheEjectorAddMonitorCostLimitReducing(ej, (KHE_MONITOR) limit_gm);
  res = KheEjectorSolveEnd(ej);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final defects (soln cost %.5f):\n",
      KheCostShow(KheSolnCost(soln)));
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }

  /* remove any vizier node and make sure the parent is fully zoned */
  if( ao->vizier_node )
  {
    KheNodeVizierDelete(parent_node);
    if( !no_node_regularity )
      KheNodeExtendZones(parent_node);
  }

  /* clean up, including deleting the group monitors, and return */
  KheGroupMonitorDelete(kempe_gm);
  KheGroupMonitorDelete(start_gm);
  KheGroupMonitorDelete(limit_gm);
  KheUnGroupCorrelatedMonitors(soln);
  if( DEBUG1 )
    fprintf(stderr, "] KheEjectionChainLayerRepairTimes returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectionChainRepairResources(KHE_TASKING tasking,                */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Ejection chain local search for improving resource assts.  If the        */
/*  rs_invariant option is true, preserve the resource assignment            */
/*  invariant.                                                               */
/*                                                                           */
/*  The ejector used is es_ejector1 from options, which will be created      */
/*  if it does not already exist.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheEjectionChainRepairResources(KHE_TASKING tasking, KHE_OPTIONS options)
{
  KHE_EJECTOR ej;  KHE_GROUP_MONITOR kempe_gm, start_gm, limit_gm;
  KHE_SOLN soln;  bool res, resource_invariant;  /* char *opt; */
  KHE_AUGMENT_OPTIONS ao;

  /* return immediately if not wanted */
  if( KheOptionsGetBool(options, "rs_eject_off", false) )
    return false;

  if( DEBUG1 )
    fprintf(stderr, "[ KheEjectionChainRepairResources(tasking)\n");

  /* get options used by this function */
  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);

  /* get an ejector */
  ej = KheEjectionChainEjectorOption(options, "es_ejector2");

  /* set the control options */
  KheOptionsSetBool(options, "es_repair_times", false);
  KheOptionsSetObject(options, "es_limit_node", NULL);
  KheOptionsSetBool(options, "es_repair_resources", true);

  /* build the required group monitors and solve */
  soln = KheTaskingSoln(tasking);
  KheGroupCorrelatedMonitors(soln, options);
  kempe_gm = KheKempeDemandGroupMonitorMake(soln);
  start_gm = KheTaskingStartGroupMonitorMake(tasking);
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial defects:\n");
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }
  ao = KheAugmentOptionsMake(soln, options, KheEjectorArena(ej));
  KheEjectorSolveBegin(ej, start_gm, (KHE_GROUP_MONITOR) soln, ao, options);
  limit_gm = NULL;
  if( resource_invariant )
  {
    limit_gm = KheAllDemandGroupMonitorMake(soln);
    KheEjectorAddMonitorCostLimitReducing(ej, (KHE_MONITOR) limit_gm);
  }
  res = KheEjectorSolveEnd(ej);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final defects:\n");
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }

  /* clean up, including deleting the group monitors, and return */
  KheGroupMonitorDelete(kempe_gm);
  if( resource_invariant )
    KheGroupMonitorDelete(limit_gm);
  KheGroupMonitorDelete(start_gm);
  KheUnGroupCorrelatedMonitors(soln);
  if( DEBUG1 )
    fprintf(stderr, "] KheEjectionChainRepairResources returning %s\n",
      res ? "true" : "false");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectionChainRepairInitialResourceAssignment(                    */
/*    KHE_GROUP_MONITOR limit_resources_gm, KHE_OPTIONS options)             */
/*                                                                           */
/*  Ejection chain local search for improving resource assts.  If the        */
/*  rs_invariant option is true, preserve the resource assignment            */
/*  invariant.                                                               */
/*                                                                           */
/*  The ejector used is es_ejector1 from options, which will be created      */
/*  if it does not already exist.                                            */
/*                                                                           */
/*****************************************************************************/

bool KheEjectionChainRepairInitialResourceAssignment(
  KHE_GROUP_MONITOR limit_resources_gm, KHE_OPTIONS options)
{
  KHE_EJECTOR ej;  KHE_SOLN soln;  bool res, save_opt;  /* char *opt; */
  KHE_AUGMENT_OPTIONS ao;

  /* return immediately if not wanted */
  if( KheOptionsGetBool(options, "rs_eject_off", false) )
    return false;

  if( DEBUG24 )
  {
    fprintf(stderr, "[ KheEjectionChainRepairInitialResourceAssignment()\n");
    KheGroupMonitorDebug(limit_resources_gm, 3, 2, stderr);
  }

  /* get an ejector */
  ej = KheEjectionChainEjectorOption(options, "es_ejector2");

  /* set the control options */
  KheOptionsSetBool(options, "es_repair_times", false);
  KheOptionsSetObject(options, "es_limit_node", NULL);
  KheOptionsSetBool(options, "es_repair_resources", true);
  save_opt = KheOptionsGetBool(options, "es_group_limit_resources_off", false);
  KheOptionsSetBool(options, "es_group_limit_resources_off", true);

  /* build the required group monitors and solve */
  soln = KheMonitorSoln((KHE_MONITOR) limit_resources_gm);
  KheGroupCorrelatedMonitors(soln, options);
  if( DEBUG24 )
  {
    fprintf(stderr, "  initial defects:\n");
    KheGroupMonitorDefectDebug(limit_resources_gm, 2, 4, stderr);
  }
  ao = KheAugmentOptionsMake(soln, options, KheEjectorArena(ej));
  KheEjectorSolveBegin(ej, limit_resources_gm, (KHE_GROUP_MONITOR) soln,
    ao, options);
  res = KheEjectorSolveEnd(ej);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final defects:\n");
    KheGroupMonitorDefectDebug(limit_resources_gm, 2, 4, stderr);
  }

  /* clean up, including deleting the group monitors, and return */
  KheOptionsSetBool(options, "es_group_limit_resources_off", save_opt);
  KheUnGroupCorrelatedMonitors(soln);
  if( DEBUG1 )
   fprintf(stderr, "] %s returning %s\n",
     "KheEjectionChainRepairInitialResourceAssignment",
	res ? "true" : "false");
  return res;
}


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
#include "khe_set.h"
#include <limits.h>
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bool_show(x) ((x) ? "true" : "false")

#define IGNORE_NOCOST true

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

#define DEBUG35 0	/* KheIncreaseOrDecreaseLoadMultiRepair */
#define DEBUG36 0	/* KheMoveRepair and KheSwapRepair */
#define DEBUG37	0	/* KheDoDecreaseLoad */
#define DEBUG38	0	/* KheGetIntervalAndDomain */
#define DEBUG39	0	/* KheEjectionChainRepairInitialResourceAssignment */

#define DEBUG40 0

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
/*  Submodule "type definitions"                                             */
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
/*  PATAT14 paper, but they have been fiddled with since then.  So at        */
/*  present they are just listed in order of first appearance.               */
/*                                                                           */
/*  It's not clear that these categories are particularly useful.  In some   */
/*  cases they don't distinguish move from assignment and unassignment.      */
/*  In other cases the same repair is called from different contexts, and    */
/*  it would be better to measure its effectiveness in each context.         */
/*                                                                           */
/*****************************************************************************/

typedef enum {

  /* repairs applied to meets only */
  KHE_REPAIR_MEET_MOVE_UNCHECKED,
  KHE_REPAIR_MEET_MOVE_CHECKED,
  KHE_REPAIR_MEET_MOVE_EJECTING,
  KHE_REPAIR_MEET_MOVE_KEMPE,
  KHE_REPAIR_FUZZY_MEET_MOVE,
  KHE_REPAIR_NODE_MEET_SWAP,
  KHE_REPAIR_MEET_SPLIT,
  KHE_REPAIR_SPLIT_MOVE,
  KHE_REPAIR_MERGE_MOVE,
  KHE_REPAIR_MEET_BOUND,
  KHE_REPAIR_COMPLEX_IDLE_MOVE,

  /* repairs applied to tasks only */
  KHE_REPAIR_TASK_UNASSIGN,
  KHE_REPAIR_TASK_SWAP,
  KHE_REPAIR_TASK_EJECTING_REASSIGN,
  KHE_REPAIR_SPLIT_TASKS_UNASSIGN

  /* repairs applied to both meets and tasks */
  /* none at present, apparently */

  /* *** currently unused
  KHE_REPAIR_TASK_SET_MOVE,
  KHE_REPAIR_TASK_SET_DOUBLE_MOVE,
  KHE_REPAIR_TASK_SET_REPLACE,
  KHE_REPAIR_TASK_SET_DOUBLE_REPLACE,
  KHE_REPAIR_RESOURCE_MOVE,
  KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN,
  KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE,
  *** */
} KHE_REPAIR_TYPE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_EXCLUDE_DAYS - first days to exclude from iterations                 */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_exclude_days_rec {
  KHE_INTERVAL			unassign_in;
  KHE_INTERVAL			swap_in;
  /* ***
  KHE_INTERVAL			underload_move_in;
  KHE_INTERVAL			underload_swap_in;
  KHE_INTERVAL			overload_move_in;
  KHE_INTERVAL			overload_swap_in;
  *** */
} *KHE_EXCLUDE_DAYS;


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
  KHE_OPTIONS_KEMPE		kempe_moves;
  bool				fuzzy_moves;
  bool				no_ejecting_moves;

  /* resource repair options */
  bool				widening_off;
  bool				reversing_off;
  /* bool			balancing_off; */
  int				move_widening_max;
  int				swap_widening_max;
  /* int			balancing_max; */
  bool 				full_widening_on;
  bool				optimal_on;
  int				optimal_width;

  /* options that are set by functions, not by the user */
  KHE_EJECTOR			ejector;
  bool				repair_times;
  bool				split_moves;
  KHE_KEMPE_STATS		kempe_stats;
  KHE_NODE			limit_node;
  bool				repair_resources;
  KHE_SOLN			soln;
  KHE_OPTIONS			options;
  KHE_FRAME			frame;
  /* KHE_EVENT_TIMETABLE_MONITOR	etm; */
  KHE_MTASK_FINDER		mtask_finder;
  KHE_MTASK_SET			scratch_mts1;
  KHE_MTASK_SET			scratch_mts2;
  /* KHE_DYNAMIC_RESOURCE_SOLVER	dynamic_solver; */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "exclude days"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheExcludeDaysInit(KHE_EXCLUDE_DAYS ed)                             */
/*                                                                           */
/*  Initialize ed.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheExcludeDaysInit(KHE_EXCLUDE_DAYS ed)
{
  ed->unassign_in = KheIntervalMake(1, 0);
  ed->swap_in = KheIntervalMake(1, 0);
  /* ***
  ed->underload_move_in = KheIntervalMake(1, 0);
  ed->underload_swap_in = KheIntervalMake(1, 0);
  ed->overload_move_in = KheIntervalMake(1, 0);
  ed->overload_swap_in = KheIntervalMake(1, 0);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheExcludeDaysShow(KHE_EXCLUDE_DAYS ed, KHE_FRAME frame)           */
/*                                                                           */
/*  Show ed.                                                                 */
/*                                                                           */
/*****************************************************************************/

static char *KheExcludeDaysShow(KHE_EXCLUDE_DAYS ed, KHE_FRAME frame)
{
  static char buff[500];
  sprintf(buff, "{U %s, S %s}", KheIntervalShow(ed->unassign_in, frame),
    KheIntervalShow(ed->swap_in, frame));
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "augment options"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AUGMENT_OPTIONS KheAugmentOptionsMake(KHE_SOLN soln,                 */
/*    KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)                 */
/*                                                                           */
/*  Create and return a new augment options object based on soln, rt, and    */
/*  options, and stored in arena a.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_AUGMENT_OPTIONS KheAugmentOptionsMake(KHE_EJECTOR ej,
  KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_AUGMENT_OPTIONS res;  char *opt;
  KHE_EVENT_TIMETABLE_MONITOR etm;
  HaMake(res, a);

  /* time repair options */
  res->vizier_node = KheOptionsGetBool(options, "es_vizier_node", false);
  res->layer_repair_long = KheOptionsGetBool(options, "es_layer_repair_long",
    false);
  res->nodes_before_meets =
    KheOptionsGetBool(options, "es_nodes_before_meets", false);
  opt = KheOptionsGet(options, "es_kempe_moves", "large_layers");
  if( strcmp(opt, "false") == 0 )
    res->kempe_moves = KHE_OPTIONS_KEMPE_FALSE;
  else if( strcmp(opt, "large_layers") == 0 )
    res->kempe_moves = KHE_OPTIONS_KEMPE_LARGE_LAYERS;
  else if( strcmp(opt, "true") == 0 )
    res->kempe_moves = KHE_OPTIONS_KEMPE_TRUE;
  else
  {
    HnAbort("KheAugmentOptionsMake: es_kempe_moves option has invalid value"
      " \"%s\"", opt);
    return KHE_OPTIONS_KEMPE_FALSE;
  }
  res->fuzzy_moves = KheOptionsGetBool(options, "es_fuzzy_moves", false);
  res->no_ejecting_moves =
    KheOptionsGetBool(options, "es_no_ejecting_moves",false);

  /* resource repair options */
  res->widening_off = KheOptionsGetBool(options, "es_widening_off", false);
  res->reversing_off = KheOptionsGetBool(options, "es_reversing_off", false);
  /* ***
  res->balancing_off = KheOptionsGetBool(options, "es_balancing_off", false);
  *** */
  res->move_widening_max = (res->widening_off ? 0 :
    KheOptionsGetInt(options, "es_move_widening_max", 4));
  res->swap_widening_max = (res->widening_off ? 0 :
    KheOptionsGetInt(options, "es_swap_widening_max", 16));
  /* ***
  res->balancing_max = (res->balancing_off ? 0 :
    KheOptionsGetInt(options, "es_balancing_max", 12));
  *** */
  res->full_widening_on = KheOptionsGetBool(options, "es_full_widening_on",
    false);
  res->optimal_on = KheOptionsGetBool(options, "es_optimal_on", false);
  res->optimal_width = KheOptionsGetInt(options, "es_optimal_width", 6);

  /* options that are set by functions, not by the user */
  res->ejector = ej;
  res->repair_times = KheOptionsGetBool(options, "es_repair_times", false);
  res->split_moves = KheOptionsGetBool(options, "es_split_moves", false);
  res->kempe_stats = KheKempeStatsOption(options, "ts_kempe_stats");
  res->limit_node =
    (KHE_NODE) KheOptionsGetObject(options, "es_limit_node", NULL);
  res->repair_resources =
    KheOptionsGetBool(options, "es_repair_resources", false);
  res->soln = soln;
  res->options = options;
  res->frame = KheOptionsFrame(options, "gs_common_frame", soln);
  /* *** allows mtask finding when tasks split, which does not work
  res->mtask_finder = KheMTaskFinderMake(soln, NULL ** not rt! **,
    res->frame, res->etm, !res->repair_times, a);
  res->scratch_mts1 = KheMTaskSetMake(res->mtask_finder);
  res->scratch_mts2 = KheMTaskSetMake(res->mtask_finder);
  *** */
  if( res->repair_resources )
  {
    etm = KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
    HnAssert(etm != NULL, "KheAugmentOptionsMake internal error (etm)");
    HnAssert(!res->repair_times, "KheAugmentOptionsMake internal error (t+r)");
    res->mtask_finder = KheMTaskFinderMake(soln, NULL /* not rt! */,
      res->frame, etm, !res->repair_times, a);
    res->scratch_mts1 = KheMTaskSetMake(res->mtask_finder);
    res->scratch_mts2 = KheMTaskSetMake(res->mtask_finder);
  }
  else
  {
    res->mtask_finder = NULL;
    res->scratch_mts1 = NULL;
    res->scratch_mts2 = NULL;
  }
  /* res->dynamic_solver = NULL; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAugmentOptionsRandomOffset(KHE_AUGMENT_OPTIONS ao, int v1, int v2)*/
/*                                                                           */
/*  Return a random offset based on the number of augments so far and the    */
/*  solution diversifier.                                                    */
/*                                                                           */
/*****************************************************************************/

static int KheAugmentOptionsRandomOffset(KHE_AUGMENT_OPTIONS ao, int v1, int v2)
{
  return v1 * KheEjectorCurrAugmentCount(ao->ejector) +
    v2 * KheSolnDiversifier(ao->soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAugmentOptionsDelete(KHE_AUGMENT_OPTIONS ao)                     */
/*                                                                           */
/*  Delete ao.  Actually the object itself is deleted when its arena is      */
/*  deleted; this function deletes stuff inside ao.                          */
/*                                                                           */
/*****************************************************************************/

static void KheAugmentOptionsDelete(KHE_AUGMENT_OPTIONS ao)
{
  /* ***
  if( ao->dynamic_solver != NULL )
    KheDynamicResourceSolverDelete(ao->dynamic_solver);
  *** */
}


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

/* *** currently unused
static void KheNoClashesCheck(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r)
{
  if( DEBUG26 && r != NULL )
    KheFrameResourceAssertNoClashes(ao->frame, r);
}
*** */


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
/*  Submodule "interval iterator"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_ITERATOR - the intervals satisfying certain conditions      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_interval_iterator_rec {
  int				frame_last;
  int				kernel_first;
  int				kernel_last;
  int				max_extra_left;
  int				max_extra_right;
  int				max_extra_total;
  bool				largest_first;
  bool				include_to_end;
  KHE_INTERVAL			exclude_first_days_in;
  int				curr_extra_left;
  int				curr_extra_right;
  int				curr_extra_total;
  int				curr_done_to_end;
} *KHE_INTERVAL_ITERATOR;


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheIntervalIteratorFirstDays(KHE_INTERVAL_ITERATOR ii)      */
/*                                                                           */
/*  Return the interval of first days visited by ii.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheIntervalIteratorFirstDays(KHE_INTERVAL_ITERATOR ii)
{
  return KheIntervalMake(ii->kernel_first - ii->max_extra_left,
    ii->kernel_first);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalIteratorInit(KHE_INTERVAL_ITERATOR ii,                   */
/*    KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL kernel_in, int max_extra,         */
/*    bool largest_first, bool include_to_end,                               */
/*    KHE_INTERVAL exclude_first_days_in)                                    */
/*                                                                           */
/*  Initialize interval iterator ii.  It will iterate over all intervals     */
/*  containing kernel_in that have at most max_extra places to the left,     */
/*  at most max_extra places to the right, and at most max_extra places      */
/*  altogether.                                                              */
/*                                                                           */
/*  If largest_first is true, the iterator will try the largest intervals    */
/*  first.  Otherwise it will try the smallest intervals first.              */
/*                                                                           */
/*  After all these intervals are tried, if include_to_end is true it        */
/*  will try one last interval, from KheIntervalFirst(kernel_in) to          */
/*  the end of the common frame - but only if that interval is not           */
/*  one of the intervals already tried.                                      */
/*                                                                           */
/*  if *exclude_first_days_in != NULL, then any interval that would          */
/*  otherwise have beed tried whose first day lies in *exclude_first_days_in */
/*  is skipped, and *exclude_first_days_in is reset to the first days of     */
/*  this iterator.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalIteratorInit(KHE_INTERVAL_ITERATOR ii,
  KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL kernel_in, int max_extra,
  bool largest_first, bool include_to_end,
  KHE_INTERVAL *exclude_first_days_in)
{
  int avail_at_left, avail_at_right;
  ii->frame_last = KheFrameTimeGroupCount(ao->frame) - 1;
  ii->kernel_first = KheIntervalFirst(kernel_in);
  ii->kernel_last = KheIntervalLast(kernel_in);
  avail_at_left = ii->kernel_first;
  avail_at_right = ii->frame_last - ii->kernel_last;
  ii->max_extra_left = min(max_extra, avail_at_left);
  ii->max_extra_right = min(max_extra, avail_at_right);
  ii->max_extra_total = min(max_extra, ii->max_extra_left+ii->max_extra_right);
  ii->largest_first = largest_first;
  ii->include_to_end = include_to_end &&
    ii->kernel_last + ii->max_extra_right < ii->frame_last;
  if( exclude_first_days_in != NULL )
  {
    ii->exclude_first_days_in = *exclude_first_days_in;
    *exclude_first_days_in = KheIntervalIteratorFirstDays(ii);
  }
  else
    ii->exclude_first_days_in = KheIntervalMake(1, 0);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalIteratorStart(KHE_INTERVAL_ITERATOR ii)                  */
/*                                                                           */
/*  Start an iteration of ii.  This function is called only by the           */
/*  KheForEachInterval iterator macro below.                                 */
/*                                                                           */
/*  Implementation note.  The values set here ensure that the first call     */
/*  to KheIntervalIteratorNext goes to the second case if largest_first      */
/*  is false, and to the third case if largest_first is true.                */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalIteratorStart(KHE_INTERVAL_ITERATOR ii)
{
  if( !ii->largest_first )
    ii->curr_extra_total = -1;
  else
    ii->curr_extra_total = ii->max_extra_total + 1;
  ii->curr_extra_left = 0;
  ii->curr_extra_right = ii->max_extra_right;
  ii->curr_done_to_end = !ii->include_to_end;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalIteratorNewExtraTotal(KHE_INTERVAL_ITERATOR ii,          */
/*    int extra_total)                                                       */
/*                                                                           */
/*  This helper function sets ii->curr_extra_total, ii->curr_extra_left,     */
/*  and ii->curr_extra_right to suitable values for starting an iteration    */
/*  over all elements whose extra width is extra_total.                      */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalIteratorNewExtraTotal(KHE_INTERVAL_ITERATOR ii,
  int extra_total)
{
  ii->curr_extra_total = extra_total;
  ii->curr_extra_left = min(ii->curr_extra_total, ii->max_extra_left);
  ii->curr_extra_right = ii->curr_extra_total - ii->curr_extra_left;
  HnAssert(ii->curr_extra_right >= 0,
    "KheIntervalIteratorNewExtraTotal internal error 1");
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIntervalIteratorNext(KHE_INTERVAL_ITERATOR ii, KHE_INTERVAL *in) */
/*                                                                           */
/*  Return the next interval if there is one, or false otherwise.  This      */
/*  function is called only by the KheForEachInterval iterator macro below.  */
/*                                                                           */
/*****************************************************************************/

static bool KheIntervalIteratorNext(KHE_INTERVAL_ITERATOR ii, KHE_INTERVAL *in)
{
  for( ; ; )
  {
    if( ii->curr_extra_right < ii->max_extra_right && ii->curr_extra_left > 0 )
    {
      /* we can add one on the right and subtract one on the left, so do that */
      ii->curr_extra_right++;
      ii->curr_extra_left--;
      if( !KheIntervalContains(ii->exclude_first_days_in,
	    ii->kernel_first - ii->curr_extra_left) )
	return *in = KheIntervalMake(ii->kernel_first - ii->curr_extra_left,
	   ii->kernel_last + ii->curr_extra_right), true;
    }
    else if( !ii->largest_first && ii->curr_extra_total < ii->max_extra_total)
    {
      /* we can add one to ii->curr_extra_total, so do that */
      KheIntervalIteratorNewExtraTotal(ii, ii->curr_extra_total + 1);
      if( !KheIntervalContains(ii->exclude_first_days_in,
	    ii->kernel_first - ii->curr_extra_left) )
	return *in = KheIntervalMake(ii->kernel_first - ii->curr_extra_left,
	   ii->kernel_last + ii->curr_extra_right), true;
    }
    else if( ii->largest_first && ii->curr_extra_total > 1 )
    {
      /* we can subtract one from ii->curr_extra_total, so do that */
      KheIntervalIteratorNewExtraTotal(ii, ii->curr_extra_total - 1);
      if( !KheIntervalContains(ii->exclude_first_days_in,
	    ii->kernel_first - ii->curr_extra_left) )
	return *in = KheIntervalMake(ii->kernel_first - ii->curr_extra_left,
	   ii->kernel_last + ii->curr_extra_right), true;
    }
    else if( !ii->curr_done_to_end )
    {
      /* we can try the interval right to the end, so do that */
      ii->curr_done_to_end = true;
      if( !KheIntervalContains(ii->exclude_first_days_in, ii->kernel_first) )
	return *in = KheIntervalMake(ii->kernel_first, ii->frame_last), true;
    }
    else
    {
      /* we're out of options, so return false */
      return false;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheForEachInterval - iterator over intervals                             */
/*                                                                           */
/*****************************************************************************/

#define KheForEachInterval(ii, in)		\
for( KheIntervalIteratorStart(ii);		\
     KheIntervalIteratorNext(ii, &(in)); )


/*****************************************************************************/
/*                                                                           */
/*  Submodule "all mtask iterator"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ALL_MTASK_ITERATOR - all mtasks in time group tg or interval in      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_all_mtask_iterator_rec {
  KHE_AUGMENT_OPTIONS			ao;
  int					random_offset;
  KHE_MTASK_SET				mts;
  int					curr_mts_index;
} *KHE_ALL_MTASK_ITERATOR;


/*****************************************************************************/
/*                                                                           */
/*  void KheAllMTaskIteratorTimeGroupInit(KHE_ALL_MTASK_ITERATOR ami,        */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)       */
/*                                                                           */
/*  Initialize all mtask iterator ami to iterate over all mtasks of          */
/*  resource type rt running during any of the times of in.                  */
/*                                                                           */
/*****************************************************************************/

static void KheAllMTaskIteratorTimeGroupInit(KHE_ALL_MTASK_ITERATOR ami,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  ami->ao = ao;
  ami->random_offset = 0;
  ami->mts = KheMTaskFinderMTasksInTimeGroup(ao->mtask_finder, rt, tg);
  ami->curr_mts_index = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAllMTaskIteratorIntervalInit(KHE_ALL_MTASK_ITERATOR ami,         */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)         */
/*                                                                           */
/*  Initialize all mtask iterator ami to iterate over all mtasks of          */
/*  resource type rt running during any of the times of in.                  */
/*                                                                           */
/*****************************************************************************/

static void KheAllMTaskIteratorIntervalInit(KHE_ALL_MTASK_ITERATOR ami,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  ami->ao = ao;
  ami->mts = KheMTaskFinderMTasksInInterval(ao->mtask_finder, rt, in);
  ami->curr_mts_index = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAllMTaskIteratorStart(KHE_ALL_MTASK_ITERATOR ami)                */
/*                                                                           */
/*  Start an iteration of mi.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheAllMTaskIteratorStart(KHE_ALL_MTASK_ITERATOR ami)
{
  ami->random_offset = KheAugmentOptionsRandomOffset(ami->ao, 13, 19);
  ami->curr_mts_index = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAllMTaskIteratorNext(KHE_ALL_MTASK_ITERATOR ami, KHE_MTASK *mt)  */
/*                                                                           */
/*  Return the next mtask if there is one, or false otherwise.  This         */
/*  function is called only by the KheAllMTaskForEach iterator macro below.  */
/*                                                                           */
/*****************************************************************************/

static bool KheAllMTaskIteratorNext(KHE_ALL_MTASK_ITERATOR ami, KHE_MTASK *mt)
{
  int count;

  /* search on through mts from ami->curr_mts_index */
  count = KheMTaskSetMTaskCount(ami->mts);
  while( ami->curr_mts_index < KheMTaskSetMTaskCount(ami->mts) )
  {
    *mt = KheMTaskSetMTask(ami->mts,
      (ami->curr_mts_index + ami->random_offset) % count);
    ami->curr_mts_index++;
    /* ***
    if( ami->tg_offset == -1 ||
        ami->tg_offset == KheMTaskIndexInFrameTimeGroup(ami->ao, *mt) )
    *** */
    return true;
  }

  /* all done */
  *mt = NULL;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAllMTaskIteratorSetTimeGroupOffset(KHE_ALL_MTASK_ITERATOR ami,   */
/*    int tg_offset)                                                         */
/*                                                                           */
/*  Set the time group offset attribute of ami to tg_offset.  This will      */
/*  cause the iterator to omit mtasks whose first time does not have this    */
/*  offset in the time group of the common frame that has this first time.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheAllMTaskIteratorSetTimeGroupOffset(KHE_ALL_MTASK_ITERATOR ami,
  int tg_offset)
{
  ami->tg_offset = tg_offset;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KheAllMTaskForEach - iterator over all mtasks in in or tg                */
/*                                                                           */
/*****************************************************************************/

#define KheAllMTaskForEach(ami, mt)		\
  for( KheAllMTaskIteratorStart(ami); KheAllMTaskIteratorNext(ami, &(mt)); )


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource task iterator"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TASK_ITERATOR - all tasks assigned resource r in tg         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_task_iterator_rec {
  /* KHE_AUGMENT_OPTIONS		ao; */
  KHE_RESOURCE				resource;
  KHE_RESOURCE_TIMETABLE_MONITOR	rtm;
  KHE_TIME_GROUP			time_group;
  int					random_offset;
  int					curr_time_index;
  int					curr_task_index;
  KHE_TASK				curr_task;
} *KHE_RESOURCE_TASK_ITERATOR;


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMTaskIteratorStart(KHE_RESOURCE_MTASK_ITERATOR rmi,      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg)             */
/*                                                                           */
/*  Start an iteration of mi which iterates over all mtasks assigned r that  */
/*  lie within time group tg, starting with the time whose index within tg   */
/*  is rmi->random_offset.  This function is called only by the              */
/*  KheResourceMTaskForEach iterator macro below.                            */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTaskIteratorStart(KHE_RESOURCE_TASK_ITERATOR rti,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg)
{
  HnAssert(r != NULL, "KheResourceMTaskIteratorStart internal error");
  /* rti->ao = ao; */
  rti->resource = r;
  rti->rtm = KheResourceTimetableMonitor(ao->soln, r);
  rti->time_group = tg;
  rti->random_offset = KheAugmentOptionsRandomOffset(ao, 11, 37);
  rti->curr_time_index = 0;	/* next task has time with this index */
  rti->curr_task_index = 0;	/* next task has task with this index */
  rti->curr_task = NULL;		/* most recently returned task */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTaskIteratorNext(KHE_RESOURCE_TASK_ITERATOR rti,         */
/*    KHE_TASK *t)                                                           */
/*                                                                           */
/*  Return the next task if there is one, or false otherwise.  This          */
/*  is called only by the KheResourceTaskForEach iterator macro below.       */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceTaskIteratorNext(KHE_RESOURCE_TASK_ITERATOR rti,
  KHE_TASK *task)
{
  KHE_TIME t;  int time_count, task_count;
  time_count = KheTimeGroupTimeCount(rti->time_group);
  for( ;  rti->curr_time_index < time_count;  rti->curr_time_index++ )
  {
    t = KheTimeGroupTime(rti->time_group,
      (rti->curr_time_index + rti->random_offset) % time_count);
    task_count = KheResourceTimetableMonitorTimeTaskCount(rti->rtm, t);
    for( ;  rti->curr_task_index < task_count;  rti->curr_task_index++  )
    {
      *task = KheResourceTimetableMonitorTimeTask(rti->rtm, t,
	rti->curr_task_index);
      rti->curr_task_index++;
      if( rti->curr_task != *task )
      {
	rti->curr_task = *task;
	return true;
      }
    }
    rti->curr_task_index = 0;
  }
  return *task = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMTaskIteratorDropCurrentMTask(                           */
/*    KHE_RESOURCE_MTASK_ITERATOR rmi)                                       */
/*                                                                           */
/*  The task most recently returned by the iterator is no longer part of     */
/*  the iterator.  Make sure that the iterator does not go wrong over this.  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheResourceMTaskIteratorDropCurrentMTask(
  KHE_RESOURCE_MTASK_ITERATOR rmi)
{
  rmi->curr_task_index--;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KheResourceTaskForEach - for each task lying in tg and assigned r        */
/*                                                                           */
/*****************************************************************************/

#define KheResourceTaskForEach(rti, ao, r, tg, task)		\
for( KheResourceTaskIteratorStart(rti, ao, r, tg);			\
     KheResourceTaskIteratorNext(rti, &(task)); )


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource mtask iterator"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_MTASK_ITERATOR - all mtasks assigned resource r in tg       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_mtask_iterator_rec {
  KHE_AUGMENT_OPTIONS			ao;
  KHE_RESOURCE				resource;
  KHE_RESOURCE_TIMETABLE_MONITOR	rtm;
  KHE_TIME_GROUP			time_group;
  int					random_offset;
  int					curr_time_index;
  int					curr_task_index;
  KHE_MTASK				curr_mt;
} *KHE_RESOURCE_MTASK_ITERATOR;


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMTaskIteratorInit(KHE_RESOURCE_MTASK_ITERATOR rmi,       */
/*    KHE_AUGMENT_OPTIONS ao)                                                */
/*                                                                           */
/*  Initialize resource mtask iterator rmi.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used; folded into KheResourceMTaskIteratorStart
static void KheResourceMTaskIteratorInit(KHE_RESOURCE_MTASK_ITERATOR rmi,
  KHE_AUGMENT_OPTIONS ao)
{
  rmi->ao = ao;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMTaskIteratorStart(KHE_RESOURCE_MTASK_ITERATOR rmi,      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg)             */
/*                                                                           */
/*  Start an iteration of mi which iterates over all mtasks assigned r that  */
/*  lie within time group tg, starting with the time whose index within tg   */
/*  is rmi->random_offset.  This function is called only by the              */
/*  KheResourceMTaskForEach iterator macro below.                            */
/*                                                                           */
/*****************************************************************************/

static void KheResourceMTaskIteratorStart(KHE_RESOURCE_MTASK_ITERATOR rmi,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg)
{
  HnAssert(r != NULL, "KheResourceMTaskIteratorStart internal error");
  rmi->ao = ao;
  rmi->resource = r;
  rmi->rtm = KheResourceTimetableMonitor(rmi->ao->soln, r);
  rmi->time_group = tg;
  rmi->random_offset = KheAugmentOptionsRandomOffset(ao, 11, 37);
  rmi->curr_time_index = 0;	/* next mtask has time with this index */
  rmi->curr_task_index = 0;	/* next mtask has task with this index */
  rmi->curr_mt = NULL;		/* most recently returned mtask */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceMTaskIteratorNext(KHE_RESOURCE_MTASK_ITERATOR rmi,       */
/*    KHE_MTASK *mt)                                                         */
/*                                                                           */
/*  Return the next mtask if there is one, or false otherwise.  This         */
/*  is called only by the KheResourceMTaskForEach iterator macro below.      */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceMTaskIteratorNext(KHE_RESOURCE_MTASK_ITERATOR rmi,
  KHE_MTASK *mt)
{
  KHE_TIME t;  int time_count, task_count;  KHE_TASK task;
  time_count = KheTimeGroupTimeCount(rmi->time_group);
  for( ;  rmi->curr_time_index < time_count;  rmi->curr_time_index++ )
  {
    t = KheTimeGroupTime(rmi->time_group,
      (rmi->curr_time_index + rmi->random_offset) % time_count);
    task_count = KheResourceTimetableMonitorTimeTaskCount(rmi->rtm, t);
    for( ;  rmi->curr_task_index < task_count;  rmi->curr_task_index++  )
    {
      task = KheResourceTimetableMonitorTimeTask(rmi->rtm, t,
	rmi->curr_task_index);
      rmi->curr_task_index++;
      *mt = KheMTaskFinderTaskToMTask(rmi->ao->mtask_finder, task);
      if( rmi->curr_mt != *mt )
      {
	rmi->curr_mt = *mt;
	return true;
      }
    }
    rmi->curr_task_index = 0;
  }
  return *mt = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceMTaskIteratorDropCurrentMTask(                           */
/*    KHE_RESOURCE_MTASK_ITERATOR rmi)                                       */
/*                                                                           */
/*  The task most recently returned by the iterator is no longer part of     */
/*  the iterator.  Make sure that the iterator does not go wrong over this.  */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static void KheResourceMTaskIteratorDropCurrentMTask(
  KHE_RESOURCE_MTASK_ITERATOR rmi)
{
  rmi->curr_task_index--;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KheResourceMTaskForEach - for each mtask lying in tg and assigned r      */
/*                                                                           */
/*****************************************************************************/

#define KheResourceMTaskForEach(rmi, ao, r, tg, mt)		\
for( KheResourceMTaskIteratorStart(rmi, ao, r, tg);		\
     KheResourceMTaskIteratorNext(rmi, &(mt)); )


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource iterator"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_ITERATOR - a set of resources, defined very flexibly        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_resource_iterator_rec {
  KHE_AUGMENT_OPTIONS		ao;
  KHE_RESOURCE_GROUP		include_rg;
  KHE_RESOURCE_GROUP		exclude_rg;
  KHE_RESOURCE			exclude_r;
  bool				include_unassigned;
  int				curr_index;
  int				stop_index;
  int				random_offset;
} *KHE_RESOURCE_ITERATOR;


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceIteratorInit(KHE_RESOURCE_ITERATOR ri,                   */
/*    KHE_RESOURCE_GROUP include_rg, KHE_RESOURCE_GROUP exclude_rg,          */
/*    KHE_RESOURCE exclude_r, bool include_unassigned)                       */
/*                                                                           */
/*  Initialize a resource iterator.  It will iterate over the resources      */
/*  of include_rg, skipping the resources of exclude_rg and exclude_r if     */
/*  non-NULL.  If include_unassigned is true, non-assignment (a NULL         */
/*  resource) will be included in the iteration, tried first.                */
/*                                                                           */
/*****************************************************************************/

static void KheResourceIteratorInit(KHE_RESOURCE_ITERATOR ri,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_GROUP include_rg,
  KHE_RESOURCE_GROUP exclude_rg, KHE_RESOURCE exclude_r,
  bool include_unassigned)
{
  HnAssert(include_rg != NULL, "KheResourceIteratorInit: include_rg == NULL");
  ri->ao = ao;
  ri->include_rg = include_rg;
  ri->exclude_rg = exclude_rg;
  ri->exclude_r = exclude_r;
  ri->include_unassigned = include_unassigned;
  /* other fields are undefined here; they are defined only while iterating */
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheResourceIteratorResourceType(                       */
/*    KHE_RESOURCE_ITERATOR ri)                                              */
/*                                                                           */
/*  Return the resource type of the resources iterated over by ri.  This     */
/*  function takes advantage of the fact that ri->include_rg != NULL.        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_RESOURCE_TYPE KheResourceIteratorResourceType(
  KHE_RESOURCE_ITERATOR ri)
{
  return KheResourceGroupResourceType(ri->include_rg);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceIteratorStart(KHE_RESOURCE_ITERATOR ri)                  */
/*                                                                           */
/*  Start an iteration of ri.  This function is called only by the           */
/*  KheForEachResource iterator macro below.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheResourceIteratorStart(KHE_RESOURCE_ITERATOR ri)
{
  ri->curr_index = (ri->include_unassigned ? -1 : 0);
  ri->stop_index = KheResourceGroupResourceCount(ri->include_rg);
  ri->random_offset = KheAugmentOptionsRandomOffset(ri->ao, 13, 19);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceIteratorNonNullStart(KHE_RESOURCE_ITERATOR ri)           */
/*                                                                           */
/*  Start an iteration of ri, but omitting NULL even if it is present.       */
/*  This function is called only by the KheForEachNonNullResource iterator   */
/*  macro below.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheResourceIteratorNonNullStart(KHE_RESOURCE_ITERATOR ri)
{
  ri->curr_index = 0;
  ri->stop_index = KheResourceGroupResourceCount(ri->include_rg);
  ri->random_offset = KheAugmentOptionsRandomOffset(ri->ao, 13, 19);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIteratorNext(KHE_RESOURCE_ITERATOR ri, KHE_RESOURCE *r)  */
/*                                                                           */
/*  Return the next resource if there is one, or false otherwise.  This      */
/*  function is called only by the KheForEachResource iterator macro below.  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIteratorNext(KHE_RESOURCE_ITERATOR ri, KHE_RESOURCE *r)
{
  KHE_RESOURCE r2;
  if( ri->curr_index == -1 )
  {
    /* get the first (NULL) resource */
    ri->curr_index++;
    return *r = NULL, true;
  }
  else
  {
    while( ri->curr_index < ri->stop_index )
    {
      /* get the next non-NULL resource from ri->include_rg, then increment */
      r2 = KheResourceGroupResource(ri->include_rg,
	(ri->curr_index + ri->random_offset) % ri->stop_index);
      ri->curr_index++;

      /* return if r2 suits, otherwise keep trying */
      if( r2 != ri->exclude_r && (ri->exclude_rg == NULL ||
	    !KheResourceGroupContains(ri->exclude_rg, r2)) )
	return *r = r2, true;
    }
    return *r = NULL, false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KheForEachResource - iterator over resources                             */
/*  KheForEachNonNullResource - iterator over non-NULL resources             */
/*                                                                           */
/*****************************************************************************/

#define KheForEachResource(ri, r)		\
for( KheResourceIteratorStart(ri);  KheResourceIteratorNext(ri, &r); )

#define KheForEachNonNullResource(ri, r)	\
for( KheResourceIteratorNonNullStart(ri);  KheResourceIteratorNext(ri, &r); )


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIteratorContainsResource(KHE_RESOURCE_ITERATOR ri,       */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return true if r is one of the resources that ri iterates over.          */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIteratorContainsResource(KHE_RESOURCE_ITERATOR ri,
  KHE_RESOURCE r)
{
  if( r == NULL )
    return ri->include_unassigned;
  else if( r == ri->exclude_r )
    return false;
  else if( ri->exclude_rg!=NULL && KheResourceGroupContains(ri->exclude_rg, r) )
    return false;
  else
    return KheResourceGroupContains(ri->include_rg, r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIteratorContainsNullOnly(KHE_RESOURCE_ITERATOR ri)       */
/*                                                                           */
/*  Return true if the only resource that ri iterates over is NULL.          */
/*                                                                           */
/*  Implementation note.  We don't reference ri->exclude_rg in this test,    */
/*  even though strictly we should.  Probably it will be NULL anyway.        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheResourceIteratorContainsNullOnly(KHE_RESOURCE_ITERATOR ri)
{
  return ri->include_unassigned &&
    KheResourceGroupResourceCount(ri->include_rg) == 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceIteratorDebug(KHE_RESOURCE_ITERATOR ri,                  */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ri with the given verbosity and indent.                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheResourceIteratorDebug(KHE_RESOURCE_ITERATOR ri,
  int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "ResourceIterator(include %s, exclude %s, exclude %s, %s)\n",
    KheResourceGroupId(ri->include_rg),
    ri->exclude_rg == NULL ? "-" : KheResourceGroupId(ri->exclude_rg),
    ri->exclude_r == NULL ? "-" : KheResourceId(ri->exclude_r),
    ri->include_unassigned ? "include_unassigned" : "-");
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


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
  if( KheEjectorCurrDebug(ej) )
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
  if( KheEjectorCurrDebug(ej) )
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
  if( KheEjectorCurrDebug(ej) )
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
  if( KheEjectorCurrDebug(ej) )
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
    if( KheEjectorCurrDebug(ej) )
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
    if( KheEjectorCurrDebug(ej) )
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
  if( KheEjectorCurrDebug(ej) )
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
      KheEjectorCurrDebugIndent(ej), "");
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
        "ancestor)\n", KheEjectorCurrDebugIndent(ej), "");
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
	KheEjectorCurrDebugIndent(ej), "");
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
	    KheEjectorCurrDebugIndent(ej), "");
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
			KheEjectorCurrDebugIndent(ej), "");
		    return true;
		  }

		  /* try basic, unless already tried by ejecting */
		  if( !basic && try_basic && KheMeetMoveRepair(ej, anc_meet,
		      target_meet, offs, KHE_MOVE_UNCHECKED, true,&basic,NULL) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,
			"%*s] KheMeetMultiRepair ret true (basic asst)\n",
			KheEjectorCurrDebugIndent(ej), "");
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
			"(kempe)\n", KheEjectorCurrDebugIndent(ej), "");
		    return true;
		  }

		  /* try ejecting, unless already tried by kempe */
		  if( !basic && try_ejecting &&
		      KheMeetMoveRepair(ej, anc_meet, target_meet, offs,
			KHE_MOVE_EJECTING, true, &basic, NULL) )
		  {
		    if( DEBUG4 )
		     fprintf(stderr, "%*s] KheMeetMultiRepair returning true "
			"(ejecting)\n", KheEjectorCurrDebugIndent(ej), "");
		    return true;
		  }

		  /* try basic, unless already tried by kempe or ejecting */
		  if( !basic && try_basic && KheMeetMoveRepair(ej, anc_meet,
		      target_meet, offs, KHE_MOVE_UNCHECKED, true,&basic,NULL) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(basic)\n", KheEjectorCurrDebugIndent(ej), "");
		    return true;
		  }

		  /* try fuzzy, if allowed and depth is 1 */
		  if( ao->fuzzy_moves && KheEjectorCurrLength(ej) == 1 &&
		      KheFuzzyMeetMoveRepair(ej, meet, target_meet, offs) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(fuzzy)\n", KheEjectorCurrDebugIndent(ej), "");
		    return true;
		  }

		  /* try split, if allowed and depth is 1 */
		  if( ao->split_moves && KheEjectorCurrLength(ej) == 1 &&
		      KheSplitMoveMultiRepair(ej, meet, target_meet, offs) )
		  {
		    if( DEBUG4 )
		      fprintf(stderr,"%*s] KheMeetMultiRepair returning true "
			"(split)\n", KheEjectorCurrDebugIndent(ej), "");
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
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
    }
    base += KheMeetAsstOffset(anc_meet);
    anc_meet = KheMeetAsst(anc_meet);
  }
  if( DEBUG4 )
    fprintf(stderr,"%*s] KheMeetMultiRepair returning false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "mtask reassignments, swaps, etc."                             */
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
  return r == NULL ? "-" : KheResourceId(r);
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

/* *** currently unused
static bool KheWidenedTaskSetMoveRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetMove(wts, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMoveRepair(KHE_EJECTOR ej,                         */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int before, int after, int *from_r_durn_change, int *to_r_durn_change) */
/*                                                                           */
/*  Try a repair that moves wts from from_r to to_r.                         */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheWidenedMTaskSetMoveRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_MTASK_SET wmts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetMove(wmts, from_r, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetMove(", success ? '+' : '-');
    KheWidenedMTaskSetMoveDebug(wmts, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalMoveRepair(KHE_EJECTOR ej,                  */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Like KheWidenedTaskSetMoveRepair, except moving wts optimally.           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheWidenedTaskSetOptimalMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetOptimalMove(wts, to_r,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetOptimalMoveRepair(KHE_EJECTOR ej,                  */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Like KheWidenedTaskSetMoveRepair, except moving wts optimally.           */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheWidenedMTaskSetOptimalMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetOptimalMove(wmts, from_r, to_r,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetOptimalMove(", success ? '+' : '-');
    KheWidenedMTaskSetOptimalMoveDebug(wmts, to_r, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}
*** */


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

/* *** currently unused
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
  if( KheEjectorCurrDebug(ej) )
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
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetDoubleMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_MTASK_SET wmts1, int before, int after,
  KHE_WIDENED_MTASK_SET wmts2, int first_index, int last_index)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetMove(wmts1, from_r, to_r, before, after,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedMTaskSetMovePartial(wmts2, to_r, from_r, first_index, last_index);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetDoubleMovePartial(", success ? '+' : '-');
    KheWidenedMTaskSetMoveDebug(wmts1, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedMTaskSetMovePartialDebug(wmts2, from_r, first_index, last_index,
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
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetOptimalDoubleMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_MTASK_SET wmts1, KHE_WIDENED_MTASK_SET wmts2,
  int first_index, int last_index)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetOptimalMove(wmts1, from_r, to_r,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedMTaskSetMovePartial(wmts2, to_r, from_r, first_index, last_index);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetOptimalDoubleMovePartial(",
      success ? '+' : '-');
    KheWidenedMTaskSetOptimalMoveDebug(wmts1, to_r, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedMTaskSetMovePartialDebug(wmts2, from_r, first_index, last_index,
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
*** */


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

/* *** currently unused
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
  if( KheEjectorCurrDebug(ej) )
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
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetDoubleMoveWholeRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_MTASK_SET wmts1, int before, int after,
  KHE_WIDENED_MTASK_SET wmts2)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetMove(wmts1, from_r, to_r, before, after,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedMTaskSetMove(wmts2, to_r, from_r, 0, 0,
      &from_r_durn_change, &to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetDoubleMoveWhole(", success ? '+' : '-');
    KheWidenedMTaskSetMoveDebug(wmts1, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedMTaskSetMoveDebug(wmts2, from_r, 0, 0, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}
*** */


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

/* *** currently unused
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
  if( KheEjectorCurrDebug(ej) )
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
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetOptimalDoubleMoveWholeRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_MTASK_SET wmts1, KHE_WIDENED_MTASK_SET wmts2)
{
  bool success;  int from_r_durn_change, to_r_durn_change;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetOptimalMove(wmts1, from_r, to_r,
      &from_r_durn_change, &to_r_durn_change) &&
    KheWidenedMTaskSetMove(wmts2, to_r, from_r, 0, 0,
      &from_r_durn_change, &to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetOptimalDoubleMoveWhole(",
      success ? '+' : '-');
    KheWidenedMTaskSetOptimalMoveDebug(wmts1, to_r, 2, -1, stderr);
    fprintf(stderr, ", ");
    KheWidenedMTaskSetMoveDebug(wmts2, from_r, 0, 0, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}
*** */


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

/* *** currently unused
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
  if( KheEjectorCurrDebug(ej) )
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetSwapRepair(KHE_EJECTOR ej,                         */
/*    KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,      */
/*    int before, int after, int *from_r_durn_change, int *to_r_durn_change) */
/*                                                                           */
/*  Try a repair that swaps wts between from_r and to_r.                     */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheWidenedTaskSetSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_TASK_SET wts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedTaskSetSwap(wts, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,*/
/*    KHE_WIDENED_MTASK_SET wmts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,    */
/*    int before, int after, int *from_r_durn_change, int *to_r_durn_change) */
/*                                                                           */
/*  Try a repair that swaps wmts between from_r and to_r.                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheWidenedMTaskSetSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_WIDENED_MTASK_SET wmts, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int before, int after, int *from_r_durn_change, int *to_r_durn_change)
{
  bool success;
  KheEjectorRepairBegin(ej);
  success = KheWidenedMTaskSetSwap(wmts, to_r, before, after,
    from_r_durn_change, to_r_durn_change);
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cWidenedMTaskSetSwap(", success ? '+' : '-');
    KheWidenedMTaskSetSwapDebug(wmts, to_r, before, after, 2, -1, stderr);
    fprintf(stderr, ")");
  }
  if( success )
  {
    KheNoClashesCheck(ej, ao, from_r);
    KheNoClashesCheck(ej, ao, to_r);
  }
  return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SET_REPLACE, success);
}
*** */


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

/* *** currently unused
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
	** try left end **
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
	** try right end **
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
      ** try each end of wts2 **
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
    ** try whole of wts2 **
    if( KheWidenedTaskSetDoubleMoveWholeRepair(ej, ao, from_r, to_r, wts1,
	before, after, wts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedTaskSetDelete(wts2);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryMTaskRuns(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,             */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_WIDENED_MTASK_SET wmts1,   */
/*    int before, int after, KHE_WIDENED_MTASK_SET wmts2, int wanted_durn,   */
/*    int *attempts, KHE_INTERVAL *res_in)                                   */
/*                                                                           */
/*  Helper function that tries one or two double move repairs, depending     */
/*  on how the duration of wts2 compares with wanted_durn.                   */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheTryMTaskRuns(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_WIDENED_MTASK_SET wmts1,
  int before, int after, KHE_WIDENED_MTASK_SET wmts2, int wanted_durn,
  int *attempts, KHE_INTERVAL *res_in)
{
  int actual_durn, fi, li;
  actual_durn = KheIntervalLength(*res_in);
  *res_in = KheWidenedMTaskSetInterval(wmts2, 0, 0);
  if( actual_durn > wanted_durn )
  {
    if( RANDOMIZED )
    {
      int random_offset = 13 * KheEjectorCurrAugmentCount(ej) +
	31 * KheSolnDiversifier(KheEjectorSoln(ej));
      if( random_offset % 2 == 0 )
      {
	** try left end **
	if( KheWidenedMTaskSetFindInitial(wmts2, wanted_durn, &fi, &li) &&
	    KheWidenedMTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wmts1,
	      before, after, wmts2, fi, li) )
	{
	  KheWidenedMTaskSetDelete(wmts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedMTaskSetDelete(wmts2);
	return false;
      }
      else
      {
	** try right end **
	if( KheWidenedMTaskSetFindFinal(wmts2, wanted_durn, &fi, &li) &&
	    KheWidenedMTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wmts1,
	      before, after, wmts2, fi, li) )
	{
	  KheWidenedMTaskSetDelete(wmts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedMTaskSetDelete(wmts2);
	return false;
      }
    }
    else
    {
      ** try each end of wts2 **
      if( KheWidenedMTaskSetFindInitial(wmts2, wanted_durn, &fi, &li) &&
	  KheWidenedMTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wmts1,
	    before, after, wmts2, fi, li) )
      {
	KheWidenedMTaskSetDelete(wmts2);
	return true;
      }
      if( KheWidenedMTaskSetFindFinal(wmts2, wanted_durn, &fi, &li) &&
	  KheWidenedMTaskSetDoubleMoveRepair(ej, ao, from_r, to_r, wmts1,
	    before, after, wmts2, fi, li) )
      {
	KheWidenedMTaskSetDelete(wmts2);
	return true;
      }
      *attempts += 2;
    }
  }
  else if( actual_durn == wanted_durn )
  {
    ** try whole of wts2 **
    if( KheWidenedMTaskSetDoubleMoveWholeRepair(ej, ao, from_r, to_r, wmts1,
	before, after, wmts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedMTaskSetDelete(wmts2);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryOptimalRuns(KHE_EJECTOR ej, KHE_RESOURCE from_r,              */
/*    KHE_RESOURCE to_r, KHE_WIDENED_TASK_SET wts1,KHE_WIDENED_TASK_SET wts2,*/
/*    int wanted_durn, int *attempts, int *first_index, int *last_index)     */
/*                                                                           */
/*  Like KheTryRuns except making an optimal move of wts1.                   */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheTryOptimalMTaskRuns(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_WIDENED_MTASK_SET wmts1, KHE_WIDENED_MTASK_SET wmts2,
  int wanted_durn, int *attempts, KHE_INTERVAL *res_in)
{
  int actual_durn, fi, li;
  actual_durn = KheIntervalLength(*res_in);
  *res_in = KheWidenedMTaskSetInterval(wmts2, 0, 0);
  if( actual_durn > wanted_durn )
  {
    if( RANDOMIZED )
    {
      int random_offset = 13 * KheEjectorCurrAugmentCount(ej) +
	31 * KheSolnDiversifier(KheEjectorSoln(ej));
      if( random_offset % 2 == 0 )
      {
	** try left end **
	if( KheWidenedMTaskSetFindInitial(wmts2, wanted_durn, &fi, &li) &&
	    KheWidenedMTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	      to_r, wmts1, wmts2, fi, li) )
	{
	  KheWidenedMTaskSetDelete(wmts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedMTaskSetDelete(wmts2);
	return false;
      }
      else
      {
	** try right end **
	if( KheWidenedMTaskSetFindFinal(wmts2, wanted_durn, &fi, &li) &&
	    KheWidenedMTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	      to_r, wmts1, wmts2, fi, li) )
	{
	  KheWidenedMTaskSetDelete(wmts2);
	  return true;
	}
	*attempts += 1;
	KheWidenedMTaskSetDelete(wmts2);
	return false;
      }
    }
    else
    {
      ** try each end of wmts2 **
      if( KheWidenedMTaskSetFindInitial(wmts2, wanted_durn, &fi, &li) &&
	  KheWidenedMTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	    to_r, wmts1, wmts2, fi, li) )
      {
	KheWidenedMTaskSetDelete(wmts2);
	return true;
      }
      if( KheWidenedMTaskSetFindFinal(wmts2, wanted_durn, &fi, &li) &&
	  KheWidenedMTaskSetOptimalDoubleMoveRepair(ej, ao, from_r,
	    to_r, wmts1, wmts2, fi, li) )
      {
	KheWidenedMTaskSetDelete(wmts2);
	return true;
      }
      *attempts += 2;
    }
  }
  else if( actual_durn == wanted_durn )
  {
    ** try whole of wts2 **
    if( KheWidenedMTaskSetOptimalDoubleMoveWholeRepair(ej, ao, from_r, to_r,
	  wmts1, wmts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedMTaskSetDelete(wmts2);
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryOptimalRuns(KHE_EJECTOR ej, KHE_RESOURCE from_r,              */
/*    KHE_RESOURCE to_r, KHE_WIDENED_TASK_SET wts1,KHE_WIDENED_TASK_SET wts2,*/
/*    int wanted_durn, int *attempts, int *first_index, int *last_index)     */
/*                                                                           */
/*  Like KheTryRuns except making an optimal move of wts1.                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
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
	** try left end **
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
	** try right end **
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
      ** try each end of wts2 **
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
    ** try whole of wts2 **
    if( KheWidenedTaskSetOptimalDoubleMoveWholeRepair(ej, ao, from_r, to_r,
	  wts1, wts2) )
      return true;
    *attempts += 1;
  }
  KheWidenedTaskSetDelete(wts2);
  return false;
}
*** */


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

/* *** currently unused
static bool KheWidenedTaskSetMoveAndDoubleMovesOptimized(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_TASK_SET wts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;
  int prev_first_index, foll_last_index, index;
  bool prev_active, foll_active;  KHE_WIDENED_TASK_SET wts2;

  ** try an optimal move **
  if( KheWidenedTaskSetOptimalMoveRepair(ej, ao, wts, from_r, to_r,
        &from_r_durn_change, &to_r_durn_change) )
    return true;

  ** return if not trying double moves, or no change in to_r's durn **
  if( ao->balancing_off || ao->balancing_max == 0 || ** to_r == NULL || **
      to_r_durn_change == 0 )
    return false;

  if( RANDOMIZED )
  {
    int random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
      53 * KheSolnDiversifier(KheEjectorSoln(ej));
    if( random_offset % 2 == 0 )
      return false;
  }

  ** boilerplate **
  KheWidenedTaskSetFullInterval(wts, &prev_first_index, &foll_last_index);

  ** debug print of balancing **
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  ** make up to eo->balancing_max attempts at a balanced double move **
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
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s]\n", KheEjectorCurrDebugIndent(ej), "");
  return false;
}
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetMoveAndDoubleMovesOptimized(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;  KHE_INTERVAL in;
  ** int prev_first_index, foll_last_index, index; **
  bool prev_active, foll_active;  KHE_WIDENED_MTASK_SET wmts2;

  ** try an optimal move **
  if( KheWidenedMTaskSetOptimalMoveRepair(ej, ao, wmts, from_r, to_r,
        &from_r_durn_change, &to_r_durn_change) )
    return true;

  ** return if not trying double moves, or no change in to_r's durn **
  if( ao->balancing_off || ao->balancing_max == 0 || ** to_r == NULL || **
      to_r_durn_change == 0 )
    return false;

  if( RANDOMIZED )
  {
    int random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
      53 * KheSolnDiversifier(KheEjectorSoln(ej));
    if( random_offset % 2 == 0 )
      return false;
  }

  ** boilerplate **
  in = KheWidenedMTaskSetFullInterval(wmts);
  ** ***
  KheWidenedMTaskSetFullInterval(wmts, &prev_first_index, &foll_last_index);
  *** **

  ** debug print of balancing **
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  ** make up to eo->balancing_max attempts at a balanced double move **
  attempts = 0;
  prev_active = foll_active = true;
  while( prev_active || foll_active )
  {
    if( foll_active )
    {
      foll_active = KheFindMovableWidenedMTaskSetRight(ao->mtask_finder, to_r,
	from_r, ** foll_last_index ** KheIntervalLast(in) + 1, &wmts2);
      if( foll_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found right run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedMTaskSetDebug(wmts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryOptimalMTaskRuns(ej, ao, from_r, to_r, wmts, wmts2,
	      to_r_durn_change, &attempts, &in) )
	** ***
	if( KheTryOptimalMTaskRuns(ej, ao, from_r, to_r, wmts, wmts2,
	      to_r_durn_change, &attempts, &index, &foll_last_index) )
	*** **
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
      prev_active = KheFindMovableWidenedMTaskSetLeft(ao->mtask_finder,
	to_r, from_r, KheIntervalFirst(in) ** prev_first_index ** - 1, &wmts2);
      if( prev_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found left run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedMTaskSetDebug(wmts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryOptimalMTaskRuns(ej, ao, from_r, to_r, wmts, wmts2,
	      to_r_durn_change, &attempts, &in ** &prev_first_index,&index **) )
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
*** */


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

/* *** currently unused
static bool KheWidenedTaskSetMoveAndDoubleMoves(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_TASK_SET wts,
  int before, int after, KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;
  int prev_first_index, foll_last_index, index;
  bool prev_active, foll_active;  KHE_WIDENED_TASK_SET wts2;

  ** try a move **
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

  ** return if not trying double moves, or no change in to_r's durn **
  if( ao->balancing_off || ao->balancing_max == 0 || ** to_r == NULL || **
      to_r_durn_change == 0 )
    return false;

  ** boilerplate **
  KheWidenedTaskSetInterval(wts, before, after, &prev_first_index,
    &foll_last_index);

  ** debug print of balancing **
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (max %d, durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", ao->balancing_max, to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  ** make up to eo->balancing_max attempts at a balanced double move **
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
*** */


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

/* *** no longer used
static bool KheWidenedMTaskSetMoveAndDoubleMoves(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE_TYPE rt, KHE_WIDENED_MTASK_SET wmts,
  int before, int after, KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  int attempts, from_r_durn_change, to_r_durn_change;  KHE_INTERVAL in;
  ** int prev_first_index, foll_last_index, index; **
  bool prev_active, foll_active;  KHE_WIDENED_MTASK_SET wmts2;

  ** try a move **
  if( KheWidenedMTaskSetMoveRepair(ej, ao, wmts, from_r, to_r, before, after,
	&from_r_durn_change, &to_r_durn_change) )
    return true;
  if( RANDOMIZED )
  {
    int random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
      53 * KheSolnDiversifier(KheEjectorSoln(ej));
    if( random_offset % 2 == 0 )
      return false;
  }

  ** return if not trying double moves, or no change in to_r's durn **
  if( ao->balancing_off || ao->balancing_max == 0 || ** to_r == NULL || **
      to_r_durn_change == 0 )
    return false;

  ** boilerplate **
  in = KheWidenedMTaskSetInterval(wmts, before, after);
    ** , &prev_first_index, &foll_last_index); **

  ** debug print of balancing **
  if( DEBUG33 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ balancing (max %d, durn %d) %s -> %s\n",
      KheEjectorCurrDebugIndent(ej), "", ao->balancing_max, to_r_durn_change,
      KheResourceShow(from_r), KheResourceShow(to_r));

  ** make up to eo->balancing_max attempts at a balanced double move **
  attempts = 0;
  prev_active = foll_active = true;
  while( prev_active || foll_active )
  {
    if( foll_active )
    {
      foll_active = KheFindMovableWidenedMTaskSetRight(ao->mtask_finder,
	to_r, from_r, ** foll_last_index ** KheIntervalLast(in) + 1, &wmts2);
      if( foll_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found right run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedMTaskSetDebug(wmts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryMTaskRuns(ej, ao, from_r, to_r, wmts, before, after, wmts2,
	      to_r_durn_change, &attempts, &in ** &index, &foll_last_index **) )
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
      prev_active = KheFindMovableWidenedMTaskSetLeft(ao->mtask_finder,
	to_r, from_r, ** prev_first_index ** KheIntervalFirst(in) - 1, &wmts2);
      if( prev_active )
      {
	if( DEBUG33 && KheEjectorCurrDebug(ej) )
	{
	  fprintf(stderr, "%*s  found left run ",
	    KheEjectorCurrDebugIndent(ej), "");
	  KheWidenedMTaskSetDebug(wmts2, 0, 0, 2, 0, stderr);
	}
	if( KheTryMTaskRuns(ej, ao, from_r, to_r, wmts, before, after, wmts2,
	      to_r_durn_change, &attempts, &in ** &prev_first_index,&index **) )
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
*** */


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

/* *** currently unused
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
*** */


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

/* *** replaced by KheMTaskSetMoveMultiRepair
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

  ** get tf and exit if not available **
  ** tf = KheEjectorTaskFinder(ej); **
  if( ao->task_finder == NULL )
    return false;
  soln = KheEjectorSoln(ej);

  ** make sure from_r_ts is non-empty and get from_r from its first task **
  from_r_ts_count = KheTaskSetTaskCount(from_r_ts);
  HnAssert(from_r_ts_count > 0, "KheTaskSetMoveMultiRepair internal error 1");
  task = KheTaskSetTask(from_r_ts, 0);
  rt = KheTaskResourceType(task);
  from_r = KheTaskAsstResource(task);

  if( ao->optimal_on )
  {
    ** make the widened task set **
    if( !KheWidenedTaskSetMakeFlexible(ao->task_finder, from_r, from_r_ts,
	  ao->optimal_width, &wts) )
      return false;
    max_extra = 0;  ** actually unused **
  }
  else
  {
    ** get max_extra **
    if( ao->widening_off )
    {
      max_extra = 0;
    }
    else
    {
      int from_r_ts_durn;
      KheTaskFinde rTaskSetInterval(ao->task_finder, from_r_ts, &first_index,
	&last_index);
      from_r_ts_durn = last_index - first_index + 1;
      max_extra = max(ao->widening_max, from_r_ts_durn) - from_r_ts_durn;
    }

    ** make the widened task set **
    if( !KheWidenedTaskSetMake(ao->task_finder, from_r, from_r_ts,
	max_extra, max_extra, &wts) )
      return false;

  }

  ** for each resource to_r in rg, not in not_rg, plus possibly unassignment **
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
	** try moving each possible number of tasks before and after **
	** if( random_offset % 2 == 0 ) **
	if( true )
	{
	  ** shortest intervals first **
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
	  ** longest intervals first **
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
	** try exact swapping **
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
      ** *** forced moves - omit
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
      *** **
	** *** widened version
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
	*** **
      if( RANDOMIZED )
      {
	KheWidenedTaskSetDelete(wts);
	return false;
      }
    }
  }

  ** delete the widened task set and scratch task sets we made earlier **
  KheWidenedTaskSetDelete(wts);

  ** try meet plus task moves, if permitted and just one task **
  ** if( KheEjectorRepairTimes(ej) && from_r_ts_count == 1 ) **
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskDomainCompatible(KHE_MTASK mt, KHE_RESOURCE_GROUP rg,       */
/*    int count)                                                             */
/*                                                                           */
/*  Return true if mt's domain is compatible with rg and r.                  */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMTaskDomainCompatible(KHE_MTASK mt, KHE_RESOURCE_GROUP rg)
{
  int count = KheResourceGroupResourceCount(rg);
  return (rg == NULL ? true :
    KheResourceGroupIntersectCount(rg, KheMTaskDomain(mt)) >= count);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt)                          */
/*                                                                           */
/*  Return the index in its days_frame time group of the first time that mt  */
/*  is running.                                                              */
/*                                                                           */
/*  This function assumes that mt has fixed times.                           */
/*                                                                           */
/*****************************************************************************/

/* *** moved up into the iterator code
static int KheMTaskIndexInFrameTimeGroup(KHE_AUGMENT_OPTIONS ao, KHE_MTASK mt)
{
  KHE_TIME_GROUP tg;  KHE_TIME time;  float junk;  int pos;
  HnAssert(KheMTaskHasFixedTimes(mt),
    "KheMTaskIndexInFrameTimeGroup internal error");
  time = KheMTaskDayTime(mt, KheIntervalFirst(KheMTaskInterval(mt)), &junk);
  tg = KheFrameTimeTimeGroup(ao->frame, time);
  if( KheTimeGroupContains(tg, time, &pos) )
    return pos;
  HnAbort("KheTaskIndexInFrameTimeGroup internal error");
  return 0;  ** keep compiler happy **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGetMTask(KHE_MTASK_FINDER mtf, int day_index,                    */
/*    KHE_INTERVAL disjoint_in, KHE_INTERVAL subset_in,                      */
/*    bool allow_preassigned, KHE_RESOURCE from_r,                           */
/*    KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,                          */
/*    KHE_MTASK *mt, KHE_RESOURCE_GROUP *rg, int *tg_start)                  */
/*                                                                           */
/*  Search for an mtask satisfying the conditions given below.  If found,    */
/*  set *mt to the mtask, possibly replace *rg and *tg_start as explained    */
/*  below, and return true.  If not found, set *mt to NULL, leave *rg and    */
/*  *tg_start as they are, and return false.                                 */
/*                                                                           */
/*  These conditions always apply:                                           */
/*                                                                           */
/*    (1) The mtask must be running on the day with index day_index.         */
/*                                                                           */
/*    (2) The mtask's interval must be disjoint from disjoint_in.  If        */
/*        disjoint_in contains day_index, this condition is incompatible     */
/*        with condition (1) and false will be returned.                     */
/*                                                                           */
/*    (3) The mtask's interval must be a subset of subset_in.  If subset_in  */
/*        does not contain day_index, this condition is incompatible with    */
/*        condition (1) and false will be returned.                          */
/*                                                                           */
/*    (4) If allow_preassigned is true, the mtask may be preassigned,        */
/*        otherwise it must be unpreassigned.                                */
/*                                                                           */
/*    (5) If to_r != NULL, the mtask must be movable from from_r to to_r.    */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*    (6) The mtask must contain a task assigned from_r.                     */
/*                                                                           */
/*    (7) Parameters from_rt, *rg, and *tg_start are unused.                 */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*    (8) The mtask must contain an unassigned task of type from_rt that     */
/*        needs assignment.                                                  */
/*                                                                           */
/*    (9) If *rg != NULL, then (9a) mtask's domain must be compatible with   */
/*        *rg, in a sense that we do not define here; and (9b) the mtask     */
/*        should preferably run at index *tg_start in the times of its day.  */
/*                                                                           */
/*  If from_r == NULL && *rg == NULL, KheGetMTask resets *rg and *tg_start   */
/*  to values appropriate for the mtask it finds, if it finds an mtask.      */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used, except as inspiration
static bool KheGetMTask(KHE_AUGMENT_OPTIONS ao, int day_index,
  KHE_INTERVAL disjoint_in, KHE_INTERVAL subset_in,
  bool allow_preassigned, KHE_RESOURCE from_r,
  KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,
  KHE_MTASK *mt, KHE_RESOURCE_GROUP *rg, int *tg_start)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, t_index;
  KHE_RESOURCE r2;  KHE_MEET meet;  KHE_TASK task;  KHE_INTERVAL mt_in;
  KHE_RESOURCE_TIMETABLE_MONITOR from_rtm;
  tg = KheFrameTimeGroup(ao->frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheGetMTask trying %s (from_r %s)\n",
      KheTimeGroupId(tg), KheResourceShow(from_r));
  if( from_r != NULL )
  {
    from_rtm = KheResourceTimetableMonitor(ao->soln, from_r);
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )  ** (1) **
    {
      t = KheTimeGroupTime(tg, i);
      count = KheResourceTimetableMonitorTimeTaskCount(from_rtm, t);
      for( j = 0;  j < count;  j++ )
      {
	task = KheResourceTimetableMonitorTimeTask(from_rtm, t, j);  ** (6) **
	*mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
	if( allow_preassigned || !KheMTaskIsPreassigned(*mt, &r2) ) ** (4) **
	{
	  mt_in = KheMTaskInterval(*mt);
	  if( KheIntervalDisjoint(mt_in, disjoint_in) &&  ** (2) **
	      KheIntervalSubset(mt_in, subset_in) &&  ** (3), and (5) below **
	      (to_r==NULL || KheMTaskResourceReassignCheck(*mt, from_r, to_r)) )
	    return true;
	}
      }
    }
  }
  else
  {
    count = KheTimeGroupTimeCount(tg);
    for( i = 0;  i < count;  i++ )  ** (1) **
    {
      t_index = (*rg == NULL ? i : (i + *tg_start) % count);
      t = KheTimeGroupTime(tg, t_index);  ** (9b) **
      for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(ao->etm, t); j++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(ao->etm, t, j);
	for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
	{
	  task = KheMeetTask(meet, k);
	  *mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
	  if( (allow_preassigned || !KheTaskIsPreassigned(task, &r2)) && **(4)**
	      KheMTaskResourceType(*mt) == from_rt &&   ** (8)  **
	      KheMTaskNeedsAssignment(*mt) &&           ** (8)  **
	      KheMTaskDomainCompatible(*mt, *rg) )        ** (9a) **
	  {
	    mt_in = KheMTaskInterval(*mt);
	    if( KheIntervalDisjoint(mt_in, disjoint_in) &&   ** (2) **
		KheIntervalSubset(mt_in, subset_in) &&  ** (3), and (5) below **
		(to_r == NULL || KheMTaskResourceReassignCheck(*mt, from_r, to_r)) )
	    {
	      if( *rg == NULL )
	      {
		** reset *rg and *tg_start **
		*rg = KheMTaskDomain(*mt);
		*tg_start = KheMTaskIndexInFrameTimeGroup(ao, *mt);
	      }
	      return true;
	    }
	  }
	}
      }
    }
  }
  *mt = NULL;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindReassignableMTaskSet(KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL in,*/
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_MTASK_SET mts)             */
/*                                                                           */
/*  Find a maximal set of mtasks running within in that are reassignable     */
/*  from from_r to to_r (either may be NULL).  The set may be empty.         */
/*                                                                           */
/*  In detail, first clear out pre-existing mtask set mts.  Then find a      */
/*  maximal set of mtasks (possibly the empty set) that satisfies the        */
/*  following conditions, add those mtasks to mts, and return true.  See     */
/*  (3) below for when false is returned.  The conditions on mtasks are:     */
/*                                                                           */
/*  (1) Each must run on days whose indexes lie entirely within in.          */
/*                                                                           */
/*  (2) Each must be reassignable from from_r to to_r (either may be NULL).  */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*  (3) For the most part, mtasks not satisfying the conditions are simply   */
/*      ignored.  However, as an exception, if there is an mtask assigned    */
/*      a non-NULL from_r satisfying (1) which does not satisfy (2), the     */
/*      whole operation returns false.                                       */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*  (4) Each mtask must run on days disjoint from the other mtasks' days.    */
/*                                                                           */
/*  (5) Each mtask must contain an unassigned task of the type of from_r or  */
/*      to_r that needs assignment (not the same as being reassignable).     */
/*                                                                           */
/*  (6) Each mtask's domain must be compatible with the domains of the       */
/*      other mtasks in mts.                                                 */
/*                                                                           */
/*  (7) Each mtask's starting time should preferably be at the same time     */
/*      of day as the starting times of the other mtasks.                    */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static bool KheFindReassignableMTaskSet(KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL in,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_MTASK_SET mts)
{
  int di, tg_offset;  KHE_TIME_GROUP tg;  KHE_MTASK mt;
  KHE_RESOURCE_GROUP rg;  KHE_TASK task;  KHE_RESOURCE_TYPE rt;
  KHE_INTERVAL mt_in;  struct khe_mtask_iterator_rec mi_rec;
  KheMTaskSetClear(mts);
  rt = KheResourceResourceType(from_r != NULL ? from_r : to_r);
  rg = NULL;
  tg_offset = 0;
  for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
  {
    tg = KheFrameTimeGroup(ao->frame, di);
    KheMTaskIteratorInit(&mi_rec, ao, rt);
    KheForEachMTask(&mi_rec, from_r, tg, tg_offset, mt, task)
    {
      mt_in = KheMTaskInterval(mt);
      if( KheIntervalSubset(mt_in, in) )                               ** (1) **
      {
	if( from_r != NULL )
	{
	  ** assigned task; check movable and add to mts **
	  if( !KheMTaskResourceReassignCheck(mt, from_r, to_r) )       ** (2) **
	  {
	    ** assigned task cannot move, so fail **
	    KheMTaskSetClear(mts);
	    return false;                                              ** (3) **
	  }
	  KheMTaskSetAddMTask(mts, mt);
	}
	else
	{
	  ** unassigned task; check suitable, add to mts, update rg etc **
	  if( KheMTaskResourceReassignCheck(mt, from_r, to_r) &&       ** (2) **
	      KheIntervalDisjoint(KheMTaskSetInterval(mts), mt_in) &&  ** (4) **
	      KheMTaskNeedsAssignment(mt) &&			       ** (5) **
	      KheMTaskDomainCompatible(mt, rg) )                       ** (6) **
	  {
	    ** mt is suitable **
	    KheMTaskSetAddMTask(mts, mt);
	    if( rg == NULL )
	    {
	      rg = KheMTaskDomain(mt);
	      tg_offset = KheMTaskIndexInFrameTimeGroup(ao, mt);       ** (7) **
	    }
	    di = KheIntervalLast(KheMTaskSetInterval(mts));
	    KheMTaskIteratorSkipToEnd(&mi_rec);  ** end of tg, that is **
	  }
	}
      }
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt)                          */
/*                                                                           */
/*  Return the index in its days_frame time group of the first time that mt  */
/*  is running.  Or return false if mt does not have fixed times.            */
/*                                                                           */
/*  This function assumes that mt has fixed times.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskIndexInFrameTimeGroup(KHE_AUGMENT_OPTIONS ao,
  KHE_MTASK mt, int *res)
{
  KHE_TIME_GROUP tg;  KHE_TIME time;  float junk;  int pos;
  if( !KheMTaskHasFixedTimes(mt) )
    return *res = 0, false;
  time = KheMTaskDayTime(mt, KheIntervalFirst(KheMTaskInterval(mt)), &junk);
  tg = KheFrameTimeTimeGroup(ao->frame, time);
  if( KheTimeGroupContains(tg, time, &pos) )
    return *res = pos, true;
  HnAbort("KheTaskIndexInFrameTimeGroup internal error (mtask %s, time %s, "
    "tg %s)", KheMTaskId(mt), KheTimeId(time), KheTimeGroupId(tg));
  return *res = 0, false;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskTimeGroupOffsetMatches(KHE_MTASK mt, int tg_offset,         */
/*    KHE_AUGMENT_OPTIONS ao)                                                */
/*                                                                           */
/*  Return true if mt has a suitable offset for matching tg_offset.          */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskTimeGroupOffsetMatches(KHE_MTASK mt, int tg_offset,
  KHE_AUGMENT_OPTIONS ao)
{
  int pos;
  return KheMTaskIndexInFrameTimeGroup(ao, mt, &pos) && tg_offset == pos;
  /* ***
  return tg_offset == -1 || tg_offset == KheMTaskIndexInFrameTimeGroup(ao, mt);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskBlockedByTimeGroup(KHE_MTASK mt, KHE_TIME_GROUP tg)         */
/*                                                                           */
/*  Return true if mt's times intersect with tg's times.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskBlockedByTimeGroup(KHE_MTASK mt, KHE_TIME_GROUP tg)
{
  KHE_INTERVAL in;  KHE_TIME time;  float junk;  int i, pos;
  if( tg != NULL )
  {
    in = KheMTaskInterval(mt);
    for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
    {
      time = KheMTaskDayTime(mt, i, &junk);
      if( time != NULL && KheTimeGroupContains(tg, time, &pos) )
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

/* *** currently unused
static bool KheTaskBlockedByMonitor(KHE_TASK task, KHE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  int i;  KHE_SOLN soln;  KHE_TASK child_task;

  ** do the job for task itself **
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
      if( KheSolnEventResourceMonitor(soln, er, i) == m )
	return true;

  ** do the job for the children of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskBlockedByMonitor(child_task, m) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskBlockedByMonitor(KHE_MTASK mt, KHE_MONITOR m)               */
/*                                                                           */
/*  Return true if moving mt is blocked because some of the tasks are        */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMTaskBlockedByMonitor(KHE_MTASK mt, KHE_MONITOR m)
{
  int i;  KHE_COST non_asst_cost, asst_cost;  KHE_TASK task;
  if( m != NULL )
  {
    for( i = 0;  i < KheMTaskTaskCount(mt);  i++ )
    {
      task = KheMTaskTask(mt, i, &non_asst_cost, &asst_cost);
      if( KheTaskBlockedByMonitor(task, m) )
	return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_FAIL - one failure to build reassignable mtasks                      */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_FAIL_NONE,
  KHE_FAIL_SUBSET,
  KHE_FAIL_REASSIGN,
  KHE_FAIL_BLOCKING,
  KHE_FAIL_OFFSET
} KHE_FAIL_TYPE;

typedef struct khe_fail_rec {
  KHE_FAIL_TYPE		type;
  KHE_MTASK		mtask;
} KHE_FAIL;

static KHE_FAIL KheFailMake(KHE_FAIL_TYPE type, KHE_MTASK mt)
{
  KHE_FAIL res;
  res.type = type;
  res.mtask = mt;
  return res;
}

static char *KheFailTypeShow(KHE_FAIL_TYPE type)
{
  switch( type )
  {
    case KHE_FAIL_NONE:		return "none";
    case KHE_FAIL_SUBSET:	return "subset";
    case KHE_FAIL_REASSIGN:	return "reassign";
    case KHE_FAIL_BLOCKING:	return "blocking";
    case KHE_FAIL_OFFSET:	return "offset";
    default:			return "??";
  }
}

static char *KheFailShow(KHE_FAIL f)
{
  static char buff[200];
  snprintf(buff, 200, "%s %s", KheFailTypeShow(f.type),
    f.mtask == NULL ? "-" : KheMTaskId(f.mtask));
  return buff;
}

/*****************************************************************************/
/*                                                                           */
/*  bool KheFindReassignableMTasksInInterval(KHE_AUGMENT_OPTIONS ao,         */
/*    KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,               */
/*    bool require_subset, KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg,    */
/*    KHE_MONITOR blocking_m, KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Clear mts and set it to a maximal set of mtasks S lying wholly or        */
/*  partly within in and assigned from_r which are reassignable from from_r  */
/*  to to_r.  If this can be done in accordance with the conditions listed   */
/*  below, then return true, else return false.  Either from_r or to_r may   */
/*  be NULL, but they must be different, otherwise the function aborts.      */
/*                                                                           */
/*  If from_r != NULL, S is a particular fixed set:  the set of all mtasks   */
/*  lying wholly or partly within in which are assigned from_r.              */
/*                                                                           */
/*  If from_r == NULL, S varies depending on choices made by the function.   */
/*  However it will be a maximal set of mtasks satisfying these conditions:  */
/*                                                                           */
/*  (a)  S includes from_mt, which is always non-NULL when from_r == NULL.   */
/*                                                                           */
/*  (b)  Each mtask of S lies wholly within in if require_subset is true,    */
/*       and wholly or partly within in if require_subset is false.          */
/*                                                                           */
/*  (c)  No two mtasks of S run on the same day.                             */
/*                                                                           */
/*  (d)  (obsolete, was preventing useful assignments) Each mtask of S       */
/*       needs assignment, according to KheMTaskNeedsAssignment.             */
/*                                                                           */
/*  (e)  Each mtask of S is reassignable from from_r to to_r, according to   */
/*       KheMTaskResourceReassignCheck.                                      */
/*                                                                           */
/*  (f)  The starting times of the mtasks of S all occur at the same time    */
/*       of day (although on different days), that is, they all have the     */
/*       same index in their various frame time groups.                      */
/*                                                                           */
/*  (g)  If blocking_tg != NULL, then none of the mtasks is running at       */
/*       a time of blocking_tg.  If such a thing could happen we just        */
/*       return false.                                                       */
/*                                                                           */
/*  (h)  If blocking_m != NULL, then none of the mtasks is monitored by      */
/*       blocking_m.  If such a thing could happen we just return false.     */
/*                                                                           */
/*  These conditions guide the function towards choosing a set S that is     */
/*  likely to work well when assigned to_r.                                  */
/*                                                                           */
/*  Whatever the value of from_r, if the operation returns true, then the    */
/*  following conditions on S (the reassignable mtasks) hold afterwards:     */
/*                                                                           */
/*  (1)  S is a maximal set of mtasks satisfying these six conditions;       */
/*       that is, there is no larger set containing S that satisfies them.   */
/*                                                                           */
/*  (2)  Each mtask of S lies wholly or partly within in.                    */
/*                                                                           */
/*  (3)  Each mtask of S is reassignable from from_r to to_r, according      */
/*       to KheMTaskResourceReassignCheck.  Also, if from_r != NULL, then    */
/*       these reassignments will make from_r completely free during in.     */
/*                                                                           */
/*  (4)  If require_subset is true, then the set of days that S is           */
/*       running is a subset of in (possibly equal to in).                   */
/*                                                                           */
/*  (5)  If blocking_tg != NULL, then none of the mtasks is running at       */
/*       a time of blocking_tg.  If such a thing could happen we just        */
/*       return false.                                                       */
/*                                                                           */
/*  (6)  If blocking_m != NULL, then none of the mtasks is monitored by      */
/*       blocking_m.  If such a think could happen we just return false.     */
/*                                                                           */
/*  Implementation note.  (1) and (2) are ensured by iterating over all      */
/*  mtasks mt lying wholly or partly within in (and assigned from_r if       */
/*  from_r != NULL) and trying to add each mt to S.  (3), (4), (5), and      */
/*  (6) are ensured by tests on the individual mtasks while iterating.       */
/*                                                                           */
/*  When from_r != NULL, there is no choice about the mtasks to include.     */
/*  Inevitably, then, if any of those mtasks fails to satisfy (3), (4),      */
/*  (5), or (6), KheFindReassignableMTasksInInterval returns false.          */
/*  When from_r == NULL, similar remarks apply to from_mt.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheFindReassignableMTasksInInterval(KHE_AUGMENT_OPTIONS ao,
  KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool require_subset, KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg,
  /* KHE_MONITOR blocking_m, */ KHE_MTASK_SET mts, KHE_FAIL *fail)
{
  int di, tg_offset;  KHE_TIME_GROUP tg;  KHE_MTASK mt /* , prev_mt */;
  KHE_INTERVAL mt_in;
  struct khe_all_mtask_iterator_rec ami_rec;
  struct khe_resource_mtask_iterator_rec rmi_rec;

  HnAssert(from_r != to_r, "KheReassignMTasksInInterval internal error 1");
  KheMTaskSetClear(mts);
  if( from_r != NULL )
  {
    /* for all mtasks mt lying wholly or partly within in and assigned from_r */
    for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
    {
      tg = KheFrameTimeGroup(ao->frame, di);
      KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, mt)
      {
	mt_in = KheMTaskInterval(mt);
	if( di == KheIntervalFirst(in) || di == KheIntervalFirst(mt_in) )
	{
	  /* at this point, mt is an mtask running wholly or partly within  */
	  /* in and assigned from_r, and we have not already encountered it */
	  KheMTaskSetAddMTask(mts, mt);
	  if( require_subset && !KheIntervalSubset(mt_in, in) )  /* (4) */
	    return *fail = KheFailMake(KHE_FAIL_SUBSET, mt), false;
	  if( !KheMTaskResourceReassignCheck(mt, from_r, to_r, true) ) /* (3) */
	    return *fail = KheFailMake(KHE_FAIL_REASSIGN, mt), false;
	  if( KheMTaskBlockedByTimeGroup(mt, blocking_tg) )      /* (5) */
	    return *fail = KheFailMake(KHE_FAIL_BLOCKING, mt), false;
	  /* ***
	  if( KheMTaskBlockedByMonitor(mt, blocking_m) )         ** (6) **
	    return false;
	  *** */
	  /* KheResourceMTaskIteratorDropCurrentMTask(&rmi_rec); */
	}
      }
    }
  }
  else
  {
    /* begin by adding from_mt to mts, or return false if not suitable */
    HnAssert(from_mt != NULL, "KheReassignMTasksInInterval internal error 2");
    HnAssert(KheIntervalSubset(KheMTaskInterval(from_mt), in),    /* (b) */
      "KheReassignMTasksInInterval internal error 3");
    /* ***
    if( ** !KheMTaskNeedsAssignment(from_mt) || **                ** (d) **
        !KheMTaskResourceReassignCheck(from_mt, from_r, to_r) ||  ** (e) **
        KheMTaskBlockedByTimeGroup(mt, blocking_tg)               ** (g) **
        ** || KheMTaskBlockedByMonitor(mt, blocking_m) ** )       ** (h) **
      return false;
    *** */
    if( !KheMTaskResourceReassignCheck(from_mt, from_r, to_r, true) ) /* (e) */
      return *fail = KheFailMake(KHE_FAIL_REASSIGN, from_mt), false;
    if( KheMTaskBlockedByTimeGroup(mt, blocking_tg) )             /* (g) */
      return *fail = KheFailMake(KHE_FAIL_BLOCKING, from_mt), false;
    if( !KheMTaskIndexInFrameTimeGroup(ao, from_mt, &tg_offset) )
      return *fail = KheFailMake(KHE_FAIL_OFFSET, from_mt), false;
    KheMTaskSetAddMTask(mts, from_mt);                            /* (a) */

    /* for all mtasks mt lying wholly or partly within in */
    KheAllMTaskIteratorIntervalInit(&ami_rec, ao,
      KheResourceResourceType(to_r), in);
    KheAllMTaskForEach(&ami_rec, mt)
    {
      /* at this point, mt is an mtask running wholly or partly within in */
      mt_in = KheMTaskInterval(mt);
      if( (!require_subset || KheIntervalSubset(mt_in, in)) &&  /* (b), (4) */
	  KheIntervalDisjoint(KheMTaskSetInterval(mts), mt_in)  /* (c)      */
	  /* && KheMTaskNeedsAssignment(mt) */ &&               /* (d)      */
	  KheMTaskResourceReassignCheck(mt, from_r, to_r, true)&& /* (e), (3) */
          KheMTaskTimeGroupOffsetMatches(mt, tg_offset, ao) &&  /* (f)      */
	  !KheMTaskBlockedByTimeGroup(mt, blocking_tg)          /* (g), (5) */
	  /* && !KheMTaskBlockedByMonitor(mt, blocking_m) */ )  /* (h), (6) */
	    KheMTaskSetAddMTask(mts, mt);
    }
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheReassignMTasksInInterval(KHE_AUGMENT_OPTIONS ao,                 */
/*    KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,               */
/*    bool require_non_empty, bool require_superset, bool require_subset)    */
/*                                                                           */
/*  Find a maximal set of mtasks S lying wholly or partly within in and      */
/*  assigned from_r, and try to reassign those mtasks from from_r to to_r.   */
/*  If this succeeds then return true.  Otherwise return false, leaving the  */
/*  solution in a partially altered state.  Either from_r or to_r may be     */
/*  NULL, but they must be different, otherwise the function aborts.         */
/*                                                                           */
/*  If from_r != NULL, S is a particular fixed set:  the set of all mtasks   */
/*  lying wholly or partly within in which are assigned from_r.              */
/*                                                                           */
/*  If from_r == NULL, S varies depending on choices made by the function.   */
/*  However it will be a maximal set of mtasks satisfying these conditions:  */
/*                                                                           */
/*  (a)  Each mtask of S lies wholly within in if require_subset is true,    */
/*       and wholly or partly within in if require_subset is false.          */
/*                                                                           */
/*  (b)  No two mtasks of S run on the same day.                             */
/*                                                                           */
/*  (c)  Each mtask of S needs assignment, according to                      */
/*       KheMTaskNeedsAssignment.                                            */
/*                                                                           */
/*  (d)  Each mtask of S is reassignable from from_r to to_r, according to   */
/*       KheMTaskResourceReassignCheck.                                      */
/*                                                                           */
/*  (e)  The starting times of the mtasks of S all occur at the same time    */
/*       of day (although on different days), that is, they all have the     */
/*       same index in their various frame time groups.                      */
/*                                                                           */
/*  These conditions guide the function towards choosing a set S that is     */
/*  likely to work well when assigned to_r.                                  */
/*                                                                           */
/*  Whatever the value of from_r, if the operation returns true, then the    */
/*  following conditions on S (the reassigned mtasks) hold afterwards:       */
/*                                                                           */
/*  (1)  S is a maximal set of mtasks satisfying these six conditions;       */
/*       that is, there is no larger set containing S that satisfies them.   */
/*                                                                           */
/*  (2)  Each mtask of S lies wholly or partly within in.                    */
/*                                                                           */
/*  (3)  Each mtask of S is has been reassigned from from_r to to_r, using   */
/*       KheMTaskResourceReassign.  Furthermore, if from_r != NULL, then     */
/*       these reassignments have made from_r completely free during in.     */
/*                                                                           */
/*  (4)  If require_subset is true, then the set of days that S is           */
/*       running is a subset of in (possibly equal to in).                   */
/*                                                                           */
/*  (5)  If require_superset is true, then the set of days that S is         */
/*       running is a superset of in (possibly equal to in).                 */
/*                                                                           */
/*  (6)  If require_non_empty is true, then S is non-empty.                  */
/*                                                                           */
/*  Implementation note.  (1) and (2) are ensured by iterating over all      */
/*  mtasks mt lying wholly or partly within in (and assigned from_r if       */
/*  from_r != NULL) and trying to add each mt to S.  (3) and (4) are         */
/*  ensured by tests on the individual mtasks as the function iterates.      */
/*  (5) and (6) are ensured by tests just before the function returns.       */
/*                                                                           */
/*  When from_r != NULL, it is not possible to satisfy both (3) and (4)      */
/*  when require_subset is true and there is an mtask assigned from_r        */
/*  running partly but not wholly within in.  Accordingly false is           */
/*  returned in that case.  Similarly, when from_r != NULL, it is not        */
/*  possible to satisfy both (3) and (4) when there is an mtask assigned     */
/*  from_r running wholly or partly within in which refuses to reassign,     */
/*  and again false is returned in that case.                                */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheFindReassignableMTasksInInterval
static bool KheReassignMTasksInInterval(KHE_AUGMENT_OPTIONS ao,
  KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool require_non_empty, bool require_superset, bool require_subset)
{
  int di, tg_offset;  KHE_TIME_GROUP tg;  KHE_MTASK mt ** , prev_mt **;
  KHE_TASK task;  KHE_INTERVAL mt_in, all_in;
  struct khe_all_mtask_iterator_rec ami_rec;
  struct khe_resource_mtask_iterator_rec rmi_rec;

  HnAssert(from_r != to_r, "KheReassignMTasksInInterval internal error 1");
  all_in = KheIntervalMake(1, 0);
  if( from_r != NULL )
  {
    ** for all mtasks mt lying wholly or partly within in and assigned from_r **
    for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
    {
      tg = KheFrameTimeGroup(ao->frame, di);
      KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, mt)
      {
	mt_in = KheMTaskInterval(mt);
	if( di == KheIntervalFirst(in) || di == KheIntervalFirst(mt_in) )
	{
	  ** at this point, mt is an mtask running wholly or partly within  **
	  ** in and assigned from_r, and we have not already encountered it **
	  if( require_subset && !KheIntervalSubset(mt_in, in) )  ** (4) **
	    return false;
	  if( !KheMTaskResourceReassign(mt, from_r, to_r) )      ** (3) **
	    return false;
	  KheResourceMTaskIteratorDropCurrentMTask(&rmi_rec);
	  all_in = KheIntervalUnion(all_in, mt_in);
	}
      }
    }
  }
  else
  {
    ** for all mtasks mt lying wholly or partly within in **
    tg_offset = -1;
    KheAllMTaskIteratorIntervalInit(&ami_rec, ao,
      KheResourceResourceType(to_r), in);
    KheAllMTaskForEach(&ami_rec, mt)
    {
      ** at this point, mt is an mtask running wholly or partly within in **
      mt_in = KheMTaskInterval(mt);
      if( (!require_subset || KheIntervalSubset(mt_in, in)) &&  ** (a), (4) **
	  KheIntervalDisjoint(all_in, mt_in) &&                 ** (b) **
	  KheMTaskNeedsAssignment(mt) &&                        ** (c) **
          KheMTaskTimeGroupOffsetMatches(mt, tg_offset, ao) )   ** (e) **
      {
	** do the reassignment, or ignore if can't **
	if( KheMTaskResourceReassign(mt, from_r, to_r) )        ** (d), (3) **
	{
	  if( tg_offset == -1 )
	    tg_offset = KheMTaskIndexInFrameTimeGroup(ao, mt);
	  ** ***
	  {
	    ** KheAllMTaskIteratorSetTimeGroupOffset(&ami_rec, tg_offset); **
	  }
	  *** **
	  all_in = KheIntervalUnion(all_in, mt_in);
	}
      }
    }
  }
  return (!require_superset || KheIntervalSubset(in, all_in))     ** (5) **
    && (!require_non_empty || KheIntervalLength(all_in) > 0);     ** (6) **
}
*** */


/* *** aborted version
static bool KheReassignMTasksInInterval(KHE_AUGMENT_OPTIONS ao,
  KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool require_non_empty, bool require_superset, bool require_subset)
{
  int di, tg_offset;  KHE_TIME_GROUP tg;  KHE_MTASK mt;
  ** KHE_RESOURCE_GROUP rg; **  KHE_TASK task;  KHE_RESOURCE_TYPE rt;
  KHE_INTERVAL mt_in, all_in;  struct khe_mtask_iterator_rec mi_rec;

  HnAssert(from_r != to_r, "KheReassignMTasksInInterval internal error 1");
  ** rg = NULL; **
  tg_offset = -1;  ** means undefined **
  all_in = KheIntervalMake(1, 0);
  rt = KheResourceResourceType(from_r != NULL ? from_r : to_r);
  KheMTaskIteratorInit(&mi_rec, ao, rt);
  for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ ) ** (3)  **
  {
    tg = KheFrameTimeGroup(ao->frame, di);
    KheForEachMTask(&mi_rec, from_r, tg, tg_offset, mt, task)         ** (4)  **
    {
      mt_in = KheMTaskInterval(mt);
      if( di==KheIntervalFirst(in) || di==KheIntervalFirst(mt_in) )   ** (2)  **
      {
	** this is mt's first day, so it can't have been added previously **
	** unless it runs twice on one day, which we aren't handling here **
	if( from_r != NULL )
	{
	  if( require_subset && !KheIntervalSubset(mt_in, in) )	      ** (10) **
	    return false;
	  if( !KheMTaskResourceReassign(mt, from_r, to_r) )           ** (5)  **
	    return false;
	  all_in = KheIntervalUnion(all_in, mt_in);
	}
	else
	{
	  ** unassigned task; do the reassignment if suitable **
	  if( (require_subset ? KheIntervalSubset(mt_in, in) : true)  ** (9)  **
	      && KheIntervalDisjoint(all_in, mt_in) &&                ** (12) **
	      KheMTaskNeedsAssignment(mt) )			      ** (4)  **
	  {
	    ** do the reassignment, or ignore if can't **
	    if( KheMTaskResourceReassign(mt, from_r, to_r) )          ** (5)  **
	    {
	      ** if( rg == NULL ) **
	      if( tg_offset == -1 )
	      {
		** first reassignment; it determines rg and tg_offset **
		** rg = KheMTaskDomain(mt); **
		tg_offset = KheMTaskIndexInFrameTimeGroup(ao, mt);    ** (x)  **
	      }
	      all_in = KheIntervalUnion(all_in, mt_in);
	      di = KheIntervalLast(all_in);
	      KheMTaskIteratorSkipToEnd(&mi_rec);  ** end of tg, that is **
	    }
	  }
	}
      }
    }
  }
  return (require_non_empty ? KheIntervalLength(all_in) > 0 : true)   ** (7) **
    && (require_superset ? KheIntervalSubset(in, all_in) : true);     ** (8) **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetUnionIsSuperset(KHE_MTASK_SET mts1, KHE_MTASK_SET mts2,  */
/*    KHE_INTERVAL in)                                                       */
/*                                                                           */
/*  Return true if the union of the intervals covered by mts1 and mts2       */
/*  is a superset of in.  Provided in is non-empty, this will also mean      */
/*  that the union is non-empty.                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskSetUnionIsSuperset(KHE_MTASK_SET mts1, KHE_MTASK_SET mts2,
  KHE_INTERVAL in)
{
  KHE_INTERVAL res;
  res = KheIntervalUnion(KheMTaskSetInterval(mts1), KheMTaskSetInterval(mts2));
  return KheIntervalSubset(in, res);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMoveRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,               */
/*    KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r,               */
/*    KHE_MTASK from_mt)                                                     */
/*                                                                           */
/*  Carry out a single repair which reassigns the mtasks currently assigned  */
/*  from_r in interval in to to_r, respecting ao's options.  This is an      */
/*  ejecting move:  any tasks assigned to_r in in initially are unassigned   */
/*  before the main move is made.                                            */
/*                                                                           */
/*  The calls to KheIntervalSubset and KheMTaskSetUnionIsSuperset ensure     */
/*  that the entire interval is used.  The point of that is that if a        */
/*  smaller interval is used, then the same repair will have been tried      */
/*  earlier with a value of in equal to that smaller interval.               */
/*                                                                           */
/*****************************************************************************/

static bool KheMoveRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_INTERVAL in, KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_MTASK from_mt)
{
  bool success;  KHE_FAIL fail;
  if( ao->repair_resources )
  {
    KheEjectorRepairBegin(ej);
    if( to_r == NULL )
    {
      /* ordinary move */
      success = KheFindReassignableMTasksInInterval(ao, in, from_r, to_r,
	  true, from_mt, NULL, /* NULL, */ ao->scratch_mts2, &fail) &&
	KheIntervalSubset(in, KheMTaskSetInterval(ao->scratch_mts2));
      if( KheEjectorCurrDebug(ej) )
      {
	fprintf(stderr, "%cMove(%s, %s, %s, %s): ", success ? '+' : '-',
	  KheIntervalShow(in, ao->frame), KheResourceShow(from_r),
	  KheResourceShow(to_r), from_mt != NULL ? KheMTaskId(from_mt) : "-");
	KheMTaskSetDebug(ao->scratch_mts2, 1, -1, stderr);
	fprintf(stderr, " ---> NULL");
      }
      if( success )
      {
	success =KheMTaskSetResourceReassign(ao->scratch_mts2,from_r,to_r,true);
	HnAssert(success, "KheMoveRepair internal error");
      }
    }
    else
    {
      /* ejecting move */
      success = KheFindReassignableMTasksInInterval(ao, in, to_r, NULL,
	  false, NULL, NULL, /* NULL, */ ao->scratch_mts1, &fail) &&
	KheFindReassignableMTasksInInterval(ao, in, from_r, to_r,
	  true, from_mt, NULL, /* NULL, */ ao->scratch_mts2, &fail) &&
	KheMTaskSetUnionIsSuperset(ao->scratch_mts1, ao->scratch_mts2, in);
      if( KheEjectorCurrDebug(ej) )
      {
	fprintf(stderr, "%cMove(%s, %s, %s, %s): move ", success ? '+' : '-',
	  KheIntervalShow(in, ao->frame), KheResourceShow(from_r),
	  KheResourceShow(to_r), from_mt != NULL ? KheMTaskId(from_mt) : "-");
	KheMTaskSetDebug(ao->scratch_mts2, 1, -1, stderr);
	fprintf(stderr, " and eject ");
	KheMTaskSetDebug(ao->scratch_mts1, 1, -1, stderr);
      }
      if( success )
      {
	success = KheMTaskSetResourceReassign(ao->scratch_mts1, to_r, NULL,true)
	  && KheMTaskSetResourceReassign(ao->scratch_mts2, from_r, to_r, true);
	HnAssert(success, "KheMoveRepair internal error");
      }
    }
    return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_EJECTING_REASSIGN, success);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSwapMTasksInInterval(KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL in,    */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int *moves)                    */
/*                                                                           */
/*  Reassign a maximal set of mtasks, satisfying the conditions below, and   */
/*  return true.  The set may be empty.  See (3) below for when false is     */
/*  returned.  Increase *moves by one for each reassignment of one mtask.    */
/*  The conditions on mtasks are:                                            */
/*                                                                           */
/*  (1) Each must run on days whose indexes lie entirely within in.          */
/*                                                                           */
/*  (2) Each must be reassignable from from_r to to_r (either may be NULL).  */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*  (3) For the most part, mtasks not satisfying the conditions are simply   */
/*      ignored.  However, as an exception, if there is an mtask assigned    */
/*      a non-NULL from_r satisfying (1) which does not satisfy (2), the     */
/*      whole operation returns false.                                       */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*  (4) Each mtask must run on days disjoint from the other mtasks' days.    */
/*                                                                           */
/*  (5) Each mtask must contain an unassigned task of the type of from_r or  */
/*      to_r that needs assignment (not the same as being reassignable).     */
/*                                                                           */
/*  (6) Each mtask's domain must be compatible with the domains of the       */
/*      other mtasks in mts.                                                 */
/*                                                                           */
/*  (7) Each mtask's starting time should preferably be at the same time     */
/*      of day as the starting times of the other mtasks.                    */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
static bool KheSwapMTasksInInterval(KHE_AUGMENT_OPTIONS ao, KHE_INTERVAL in,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m)
{
  int di, tg_offset;  KHE_TIME_GROUP tg;  KHE_MTASK mt;
  KHE_RESOURCE_GROUP rg;  KHE_TASK task;  KHE_RESOURCE_TYPE rt;
  KHE_INTERVAL mt_in, all_in;  struct khe_mtask_iterator_rec mi_rec;
  HnAbort("KheSwapMTasksInInterval sti ll to do");
  rt = KheResourceResourceType(from_r != NULL ? from_r : to_r);
  rg = NULL;
  tg_offset = 0;
  all_in = KheIntervalMake(1, 0);
  for( di = KheIntervalFirst(in);  di <= KheIntervalLast(in);  di++ )
  {
    tg = KheFrameTimeGroup(ao->frame, di);
    KheMTaskIteratorInit(&mi_rec, ao, rt);
    KheForEachMTask(&mi_rec, from_r, tg, tg_offset, mt, task)
    {
      mt_in = KheMTaskInterval(mt);
      if( KheIntervalSubset(mt_in, in) )                               ** (1) **
      {
	if( from_r != NULL )
	{
	  ** assigned task; check movable and add to mts **
	  if( !KheMTaskResourceReassign(mt, from_r, to_r) )            ** (2) **
	    return false;                                              ** (3) **
	  all_in = KheIntervalUnion(all_in, mt_in);
	}
	else
	{
	  ** unassigned task; check suitable, add to mts, update rg etc **
	  if( KheIntervalDisjoint(all_in, mt_in) &&                    ** (4) **
	      KheMTaskNeedsAssignment(mt) &&			       ** (5) **
	      KheMTaskDomainCompatible(mt, rg) )                       ** (6) **
	  {
	    ** mt is suitable **
	    if( KheMTaskResourceReassign(mt, from_r, to_r) )           ** (2) **
	    {
	      if( rg == NULL )
	      {
		rg = KheMTaskDomain(mt);
		tg_offset = KheMTaskIndexInFrameTimeGroup(ao, mt);     ** (7) **
	      }
	      all_in = KheIntervalUnion(all_in, mt_in);
	      di = KheIntervalLast(all_in);
	      KheMTaskIteratorSkipToEnd(&mi_rec);  ** end of tg, that is **
	    }
	  }
	}
      }
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,               */
/*    KHE_INTERVAL in, bool all, KHE_RESOURCE from_r, KHE_RESOURCE to_r,     */
/*    KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg)                         */
/*                                                                           */
/*  Carry out a single repair which swaps the mtasks of from_r in interval   */
/*  in with the mtasks of to_r in interval in, respecting ao's options.      */
/*                                                                           */
/*  If blocking_tg != NULL, block reassignments from to_r to from_r that     */
/*  move tasks to blocking_tg.  If blocking_m != NULL, block reassignments   */
/*  from to_r to from_r that move tasks monitored by blocking_m.             */
/*                                                                           */
/*  If parameter all is true, the call to KheMTaskSetUnionIsSuperset ensures */
/*  that the entire interval is used.  The point of that is that if a        */
/*  smaller interval is used, then the same repair will have been tried      */
/*  earlier with a value of in equal to that smaller interval.  There is     */
/*  just one case where all is false, because in that case we do not try a   */
/*  sequence of larger and larger intervals.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheSwapRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_INTERVAL in, bool all, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg /* , KHE_MONITOR blocking_m */)
{
  bool success, a, b;  KHE_FAIL a_fail, b_fail;
  if( ao->repair_resources )
  {
    KheEjectorRepairBegin(ej);
    a = b = false;
    a_fail = b_fail = KheFailMake(KHE_FAIL_NONE, NULL);
    success = (a = KheFindReassignableMTasksInInterval(ao, in, from_r, to_r,
	true, from_mt, NULL, /* NULL, */ ao->scratch_mts1, &a_fail)) &&
      (b = KheFindReassignableMTasksInInterval(ao, in, to_r, from_r,
	true, NULL, blocking_tg, /*blocking_m,*/ ao->scratch_mts2, &b_fail)) &&
      (!all ||KheMTaskSetUnionIsSuperset(ao->scratch_mts1,ao->scratch_mts2,in));
    if( KheEjectorCurrDebug(ej) )
    {
      fprintf(stderr, "%cSwap(%s, %s ", success ? '+' : '-',
	KheIntervalShow(in, ao->frame), KheResourceShow(from_r));
      KheMTaskSetDebug(ao->scratch_mts1, 1, -1, stderr);
      fprintf(stderr, ", %s ", KheResourceShow(to_r));
      KheMTaskSetDebug(ao->scratch_mts2, 1, -1, stderr);
      fprintf(stderr, ", %s, %s): ",
	from_mt != NULL ? KheMTaskId(from_mt) : "-",
	blocking_tg != NULL ? KheTimeGroupId(blocking_tg) : "-");
      if( !success )
      {
	if( !a )
	  fprintf(stderr, " (a %s)", KheFailShow(a_fail));
	else if( !b )
	  fprintf(stderr, " (b %s)", KheFailShow(b_fail));
	else
	  fprintf(stderr, " (c)");
      }
    }
    if( success )
    {
      success = KheMTaskSetResourceReassign(ao->scratch_mts1, from_r, to_r,true)
	&& KheMTaskSetResourceReassign(ao->scratch_mts2, to_r, from_r, true);
      HnAssert(success, "KheSwapRepair internal error");
    }
    return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_SWAP, success);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSwapAndMoveMultiRepair(KHE_EJECTOR ej,                      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR ri, */
/*    KHE_INTERVAL_ITERATOR move_ii, KHE_INTERVAL_ITERATOR swap_ii,          */
/*    KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m) */
/*                                                                           */
/*  Try repairs that move mtasks from resource from_r to a resource to_r,    */
/*  one of the resources returned by resource iterator ri.                   */
/*                                                                           */
/*  The repairs include swaps of tasks in the intervals of swap_ii.  For     */
/*  them, if blocking_tg != NULL, block swaps that move tasks to             */
/*  blocking_tg.  If blocking_m != NULL, block swaps that move tasks         */
/*  monitored by blocking_m.                                                 */
/*                                                                           */
/*  The repairs also include moves from from_r to to_r of tasks in the       */
/*  intervals of to_r, including double moves.                               */
/*                                                                           */
/*  If the ejector allows meet moves, try them too, but without widening,    */
/*  reversing, or balancing.                                                 */
/*                                                                           */
/*  If from_mt != NULL, include from_mt in the moves from from_r to ri.      */
/*                                                                           */
/*  Obsolete:                                                                */
/*  This function uses widened task sets, and the repairs it tries are       */
/*  widened task set move and swap operations.  There is also a double       */
/*  move operation which consists of two widened task set moves.             */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheMTaskSwapAndMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR to_ri,
  KHE_INTERVAL_ITERATOR move_ii, KHE_INTERVAL_ITERATOR swap_ii,
  KHE_MTASK from_mt, KHE_TIME_GROUP blocking_tg ** , KHE_MONITOR blocking_m **)
{
  KHE_RESOURCE to_r;  KHE_INTERVAL in;

  ** boilerplate **
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%*sKheMTaskSwapAndMoveMultiRepair trying ",
      KheEjectorCurrDebug(ej), "");
    KheResourceIteratorDebug(to_ri, 1, 0, stderr);
  }

  ** for each resource in to_ri **
  KheForEachResource(to_ri, to_r)
  {
    if( DEBUG30 && KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*sKheMTaskSwapAndMoveMultiRepair trying "
	"resource %s\n", KheEjectorCurrDebugIndent(ej), "",
	KheResourceShow(to_r));
    HnAssert(to_r != from_r,
      "KheMTaskSwapAndMoveMultiRepair internal error 1");

    ** try one move repair for each of move_ii's intervals **
    KheForEachInterval(move_ii, in)
      if( KheMo veRepair(ej, ao, in, from_r, to_r, from_mt) )
	return true;

    ** try one swap repair for each of swap_ii's intervals **
    if( from_r != NULL && to_r != NULL && !ao->reversing_off )
      KheForEachInterval(swap_ii, in)
	if( KheSwapRe pair(ej, ao, in, from_r, to_r, from_mt, blocking_tg) )
	  return true;
  }

  ** no luck **
  return false;
}
*** */


/* *** old version that uses widened task sets
static bool KheMeetMoveAndTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_TASK task,
  KHE_RESOURCE r);
static bool KheMeetMoveAndMTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_MTASK mt,
  KHE_RESOURCE r);

static bool KheMTaskSetMoveMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MTASK_SET from_r_mts, KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR ri,
  KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_m)
{
  KHE_RESOURCE to_r;  KHE_INTERVAL in;  KHE_RESOURCE_TYPE rt;
  int from_r_mts_count, from_r_durn_change, to_r_durn_change;
  int random_offset1, random_offset2, random_offset3;
  int max_extra, max_before, max_after, before, after, lim_after;
  KHE_WIDENED_MTASK_SET wmts;

  ** get task finder and exit if not available **
  if( ao->mtask_finder == NULL )
    return false;

  ** make sure from_r_mts is non-empty and get rt from ri **
  from_r_mts_count = KheMTaskSetMTaskCount(from_r_mts);
  HnAssert(from_r_mts_count > 0, "KheMTaskSetMoveMultiRepair internal error 1");
  rt = KheResourceIteratorResourceType(ri);

  if( ao->optimal_on )
  {
    ** make the widened task set **
    if( !KheWidenedMTaskSetMakeFlexible(ao->mtask_finder, from_r, from_r_mts,
	  ao->optimal_width, &wmts) )
      return false;
    max_extra = 0;  ** actually unused **
  }
  else
  {
    ** get max_extra **
    if( ao->widening_off )
    {
      max_extra = 0;
    }
    else
    {
      int from_r_mts_durn;
      in = KheMTaskSetInterval(from_r_mts);
      ** KheMTaskSetInterval(from_r_mts, &first_index, &last_index); **
      ** from_r_mts_durn = last_index - first_index + 1; **
      from_r_mts_durn = KheIntervalLength(in);
      max_extra = max(ao->widening_max, from_r_mts_durn) - from_r_mts_durn;
    }

    ** make the widened task set **
    if( !KheWidenedMTaskSetMake(ao->mtask_finder, from_r, from_r_mts,
	max_extra, max_extra, &wmts) )
      return false;

  }

  ** for each resource to_r in rg, not in not_rg, plus possibly unassignment **
  if( DEBUG30 && KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%*sKheTaskSetMoveMultiRepair trying ",
      KheEjectorCurrDebug(ej), "");
    KheResourceIteratorDebug(ri, 1, 0, stderr);
  }
  random_offset1 = 11 * KheEjectorCurrAugmentCount(ej) +
    53 * KheSolnDiversifier(ao->soln);
  random_offset2 = 7 * KheEjectorCurrAugmentCount(ej) +
    37 * KheSolnDiversifier(ao->soln);
  random_offset3 = 73 * KheEjectorCurrAugmentCount(ej) +
    5 * KheSolnDiversifier(ao->soln);
  ** KheForEachResource(srs_rec,rg,not_rg,allow_unassign,random_offset1,to_r)**
  KheForEachResource(ri, random_offset1, to_r)
  {
    if( DEBUG30 && KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*sKheTaskSetMoveMultiRepair trying resource %s\n",
	KheEjectorCurrDebugIndent(ej), "", KheResourceShow(to_r));
    if( to_r != from_r )
    {
      if( ao->optimal_on )
      {
	if( KheWidenedMTaskSetOptimalMoveCheck(wmts, from_r, to_r, blocking_tg,
	      blocking_m) &&
	    KheWidenedMTaskSetMoveAndDoubleMovesOptimized(ej,
		ao, rt, wmts, from_r, to_r) )
	{
	  KheWidenedMTaskSetDelete(wmts);
	  return true;
	}
      }
      else
      if( KheWidenedMTaskSetMoveCheck(wmts, from_r, to_r, false,
	    &max_before, &max_after) )
      {
	** try moving each possible number of tasks before and after **
	** if( random_offset % 2 == 0 ) **
	if( true )
	{
	  ** shortest intervals first **
	  for( before = 0;  before <= max_before;  before++ )
	  {
	    lim_after = min(max_after, max_extra - before);
	    for( after = 0;  after <= lim_after;  after++ )
	    {
	      if( KheWidenedMTaskSetMoveAndDoubleMoves(ej, ao, rt, wmts,
		    (before + random_offset2) % (max_before + 1),
		    (after + random_offset3) % (lim_after + 1), from_r, to_r) )
	      {
		KheWidenedMTaskSetDelete(wmts);
		return true;
	      }
	      if( RANDOMIZED )
	      {
		KheWidenedMTaskSetDelete(wmts);
		return false;
	      }
	    }
	  }
	}
	else
	{
	  ** longest intervals first **
	  for( before = max_before;  before >= 0;  before-- )
	  {
	    lim_after = min(max_after, max_extra - before);
	    for( after = lim_after;  after >= 0;  after-- )
	    {
	      if( KheWidenedMTaskSetMoveAndDoubleMoves(ej, ao, rt, wmts,
		    (before + random_offset2) % (max_before + 1),
		    (after + random_offset3) % (lim_after + 1), from_r, to_r) )
	      {
		KheWidenedMTaskSetDelete(wmts);
		return true;
	      }
	      if( RANDOMIZED )
	      {
		KheWidenedMTaskSetDelete(wmts);
		return false;
	      }
	    }
	  }
	}
      }
      else if( !ao->reversing_off && KheWidenedMTaskSetSwapCheck(wmts, to_r,
	  true, blocking_tg, blocking_m, &max_before, &max_after) )
      {
	** try exact swapping **
	for( before = 0;  before <= max_before;  before++ )
	{
	  lim_after = min(max_after, max_extra - before);
	  for( after = 0;  after <= lim_after;  after++ )
	  {
	    if( KheWidenedMTaskSetSwapRepair(ej, ao, wmts, from_r, to_r,
		  before, after, &from_r_durn_change, &to_r_durn_change) )
	    {
	      KheWidenedMTaskSetDelete(wmts);
	      return true;
	    }
	    if( RANDOMIZED )
	    {
	      KheWidenedMTaskSetDelete(wmts);
	      return false;
	    }
	  }
	}
      }
      ** *** forced moves - omit
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
      *** **
	** *** widened version
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
	*** **
      if( RANDOMIZED )
      {
	KheWidenedMTaskSetDelete(wmts);
	return false;
      }
    }
  }

  ** delete the widened mtask set and scratch mtask sets we made earlier **
  KheWidenedMTaskSetDelete(wmts);

  ** try meet plus mtask moves, if permitted and just one task **
  ** *** out of play, too hard to adapt from tasks to mtasks
  if( ao->repair_times && from_r_mts_count == 1 )
  {
    mt = KheMTaskSetMTask(from_r_mts, 0);
    random_offset1 = 29 * KheEjectorCurrAugmentCount(ej) +
      59 * KheSolnDiversifier(soln);
    KheForEachResource(srs_rec, rg, not_rg, allow_unassign,random_offset1,to_r)
      if( to_r != from_r )
      {
	if( KheMeetMoveAndMTaskMoveMultiRepair(ej, ao, KHE_MOVE_KEMPE,
	      mt, to_r) )
	  return true;
      }
  }
  *** **
  return false;
}
*** */


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

/* *** currently unused
static bool KheTaskSetSwapToEndRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK_SET ts, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  struct khe_se_resource_set_type srs_rec;  KHE_SOLN soln;
  KHE_RESOURCE r1, r2;  KHE_RESOURCE_TYPE rt;  KHE_WIDENED_TASK_SET wts;
  KHE_TASK_SET ts1;  int offset, ts_first, ts_last, x1, x2, final_index;

  ** make sure that ts is not empty; do nothing if it's unassigned **
  HnAssert(KheTaskSetTaskCount(ts) > 0,
    "KheTaskSetSwapToEndRepair internal error 1");
  r1 = KheTaskAsstResource(KheTaskSetTask(ts, 0));

  if( r1 != NULL )
  {
    ** boilerplate **
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
    ** tf = ao->task_finder; **
    ** tf = KheEjectorTaskFinder(ej); **
    final_index = KheTaskFinderLastIndex(ao->task_finder);
    soln = KheEjectorSoln(ej);
    ts1 = KheTaskSetMake(soln);
    KheTaskFinderTaskS etInterval(ao->task_finder, ts, &ts_first, &ts_last);

    ** get the tasks for r1 from the start to ts_last **
    KheFindTas ksInInterval(ao->task_finder, 0, ts_last, rt, r1, false,
      false, ts1, &x1, &x2);
    if( KheWidenedTaskSetMake(ao->task_finder, r1, ts1, 0, 0, &wts) )
    {
      ** try swapping r1 and r2's timetables from start to ts_last **
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      KheForEachResource(srs_rec, rg, not_rg, false, offset, r2)
      {
	** get the tasks for r2 from the start to ts, and try the move **
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

    ** get the tasks for r1 from ts_first to the end **
    KheFindTasks InInterval(ao->task_finder, ts_first, final_index, rt,
      r1, false, false, ts1, &x1, &x2);
    if( KheWidenedTaskSetMake(ao->task_finder, r1, ts1, 0, 0, &wts) )
    {
      ** try swapping r1 and r2's timetables from ts_first to the end **
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      KheForEachResource(srs_rec, rg, not_rg, false, offset, r2)
      {
	** get the tasks for r2 from ts to the end, and try the move **
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

    ** no luck; tidy up and exit **
    KheTaskSetDelete(ts1);
  }
  if( DEBUG34 )
    fprintf(stderr, "] KheTaskSetSwapToEndRepair returning false\n");
  return false;
}
*** */


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

/* ***
static bool KheMTaskSetSwapToEndRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MTASK_SET mts, KHE_RESOURCE r, KHE_RESOURCE_ITERATOR ri,
  ** KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg, bool allow_unassign, **
  KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  ** struct khe_se_resource_set_type srs_rec; **
  KHE_SOLN soln;  KHE_MTASK_SET mts1;
  KHE_RESOURCE r1, r2;  KHE_RESOURCE_TYPE rt;  KHE_WIDENED_MTASK_SET wmts;
  int offset, ** mts_first, mts_last, ** final_index;
  KHE_INTERVAL mts_in;  int max_left_count, max_right_count;
  int from_r_durn_change, to_r_durn_change;

  ** make sure that mts is not empty; do nothing if it's unassigned **
  HnAssert(KheMTaskSetMTaskCount(mts) > 0,
    "KheMTaskSetSwapToEndRepair internal error 1");
  ** r1 = KheTaskAsstResource(KheMTaskSetMTask(mts, 0)); **
  r1 = r;

  if( r1 != NULL )
  {
    ** boilerplate **
    if( DEBUG34 )
    {
      fprintf(stderr, "[ KheMTaskSetSwapToEndRepair(ej, ");
      KheMTaskSetDebug(mts, 2, -1, stderr);
      fprintf(stderr, " (assigned %s), ", KheResourceId(r1));
      KheResourceIteratorDebug(ri, 1, -1, stderr);
      ** ***
      KheResourceGroupDebug(rg, 1, -1, stderr);
      fprintf(stderr, ", ");
      if( not_rg != NULL )
	KheResourceGroupDebug(not_rg, 1, -1, stderr);
      else
	fprintf(stderr, "-");
      *** **
      fprintf(stderr, ")\n");
    }
    rt = KheResourceResourceType(r1);
    ** tf = ao->task_finder; **
    ** tf = KheEjectorTaskFinder(ej); **
    final_index = KheMTaskFinderLastIndex(ao->mtask_finder);
    soln = KheEjectorSoln(ej);
    mts1 = KheMTaskSetMake(ao->mtask_finder);
    mts_in = KheMTaskSetInterval(mts);
    ** KheMTaskSetInterval(mts, &mts_first, &mts_last); **

    ** get the tasks for r1 from the start to mts_last **
    KheFindMTasksInInterval(ao->mtask_finder,
      KheIntervalMake(0, KheIntervalLast(mts_in)),
      true, true, r1, rt, NULL, mts1);
    if( KheWidenedMTaskSetMake(ao->mtask_finder, r1, mts1, 0, 0, &wmts) )
    {
      ** try swapping r1 and r2's timetables from start to mts_last **
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      ** KheForEachResource(srs_rec, rg, not_rg, false, offset, r2) **
      KheForEachResource(ri, offset, r2)
      {
	** get the tasks for r2 from the start to mts, and try the move **
	if( r2 != r1 && KheWidenedMTaskSetSwapCheck(wmts, r2, false,
	    blocking_tg, blocking_d, &max_left_count, &max_right_count) &&
            KheWidenedMTaskSetSwapRepair(ej, ao, wmts, r1, r2, 0, 0,
		&from_r_durn_change, &to_r_durn_change) )
	{
	  KheMTaskSetDelete(mts1, ao->mtask_finder);
	  if( DEBUG34 )
	    fprintf(stderr, "] KheMTaskSetSwapToEndRepair returning true\n");
	  return true;
	}
      }
      KheWidenedMTaskSetDelete(wmts);
    }

    ** get the tasks for r1 from mts_first to the end **
    KheFindMTasksInInterval(ao->mtask_finder,
      KheIntervalMake(KheIntervalFirst(mts_in), final_index),
      true, true, r1, rt, NULL, mts1);
    ** ***
    KheFindMTas ksInInterval(ao->mtask_finder, mts_first, final_index, rt,
      r1, false, false, mts1, &x1, &x2);
    *** **
    if( KheWidenedMTaskSetMake(ao->mtask_finder, r1, mts1, 0, 0, &wmts) )
    {
      ** try swapping r1 and r2's timetables from mts_first to the end **
      offset = KheEjectorCurrAugmentCount(ej) + 5 * KheSolnDiversifier(soln);
      KheForEachResource(ri, offset, r2)
      {
	** get the tasks for r2 from mts to the end, and try the move **
	if( r2 != r1 && KheWidenedMTaskSetSwapCheck(wmts, r2, false,
	    blocking_tg, blocking_d, &max_left_count, &max_right_count) &&
            KheWidenedMTaskSetSwapRepair(ej, ao, wmts, r1, r2, 0, 0,
	      &from_r_durn_change, &to_r_durn_change) )
	{
	  KheMTaskSetDelete(mts1, ao->mtask_finder);
	  if( DEBUG34 )
	    fprintf(stderr, "] KheMTaskSetSwapToEndRepair returning true\n");
	  return true;
	}
      }
      KheWidenedMTaskSetDelete(wmts);
    }

    ** no luck; tidy up and exit **
    KheMTaskSetDelete(mts1, ao->mtask_finder);
  }
  if( DEBUG34 )
    fprintf(stderr, "] KheMTaskSetSwapToEndRepair returning false\n");
  return false;
}
*** */


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

/* *** replaced by KheMTaskSetMoveAugment
static bool KheTaskSetMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK_SET ts, KHE_RESOURCE_GROUP rg, KHE_RESOURCE_GROUP not_rg,
  bool allow_unassign, KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  if( KheTaskSetTaskCount(ts) > 0 )
  {
    if( KheTaskSetMoveMultiRepair(ej, ao, ts, rg, not_rg, allow_unassign,
	  blocking_tg, blocking_d) )
      return true;
    if( KheEjectorCurrLength(ej) == 1 && ao->full_widening_on &&
	KheTaskSetSwapToEndRepair(ej, ao, ts, rg, not_rg, allow_unassign,
	  blocking_tg, blocking_d) )
      return true;
  }
  return false;
}
*** */


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

/* *** no longer used
static bool KheMTaskSetMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR ri,
  KHE_INTERVAL_ITERATOR swap_ii, KHE_INTERVAL_ITERATOR move_ii,
  KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  if( KheMTaskSwapA ndMoveMultiRepair(ej, ao, from_r, ri,
	swap_ii, move_ii, blocking_tg, blocking_d) )
    return true;
  ** *** part of the swap iterator now
  if( KheEjectorCurrLength(ej) == 1 && ao->full_widening_on &&
      KheMTaskSetSwapToEndRepair(ej, ao, from_r_mts, from_r, ri,
	blocking_tg, blocking_d) )
    return true;
  *** **
  return false;
}
*** */


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

/* *** replaced by KheMTaskMoveAugment
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,         */
/*    KHE_MTASK from_mt, KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR ri,      */
/*    KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d,                    */
/*    KHE_MTASK_SET scratch_mts)                                             */
/*                                                                           */
/*  Like KheMTaskSetMoveAugment but for a single mtask.                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheMTaskMoveAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE from_r, KHE_RESOURCE_ITERATOR ri,
  KHE_INTERVAL_ITERATOR swap_ii, KHE_INTERVAL_ITERATOR move_ii,
  KHE_TIME_GROUP blocking_tg, KHE_MONITOR blocking_d)
{
  ** ***
  KheMTaskSetClear(scratch_mts);
  KheMTaskSetAddMTask(scratch_mts, from_mt);
  *** **
  if( KheMTaskSw apAndMoveMultiRepair(ej, ao, from_r, ri,
      swap_ii, move_ii, blocking_tg, blocking_d) )
    return true;
  ** *** part of the swap iterator now
  if( KheEjectorCurrLength(ej) == 1 && ao->full_widening_on &&
      KheMTaskSetSwapToEndRepair(ej, ao, scratch_mts, from_r, ri,
	blocking_tg, blocking_d) )
    return true;
  *** **
  return false;
}
*** */


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

/* *** currently unused
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
  ** frame = KheEjectorFrame(ej); **
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetMoveAndTaskMoveRepair(KHE_EJECTOR ej, KHE_MEET meet,         */
/*    KHE_MEET target_meet, int offset, KHE_MOVE_TYPE meet_mt,               */
/*    KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE task_mt)                  */
/*                                                                           */
/*  Carry out a combined meet move and task move augment.                    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMeetMoveAndMTaskMoveRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MEET meet,
  KHE_MEET target_meet, int offset, KHE_MOVE_TYPE meet_mt,
  KHE_MTASK mt, KHE_RESOURCE r, KHE_MOVE_TYPE task_mt)
{
  bool success ** basic, move **;  ** int d; **  KHE_RESOURCE r2;
  ** KHE_FRAME frame; **
  if( KheMTaskIsPreassigned(mt, &r2) )
    return false;
  HnAssert(KheMeetAsst(meet) != NULL,
    "KheMeetMoveAndMTaskMoveRepair internal error 1");
  KheEjectorRepairBegin(ej);
  HnAbort("KheMeetMoveAndMTaskMoveRepair fixing problem is sti ll to do");
  move = (KheTaskAsst(task) != NULL);
  frame = ao->frame;
  success = (move ? KheTaskUnAssign(task) : true) &&
    KheTypedMeetMove(meet, target_meet, offset, meet_mt, false, &d,&basic,NULL)
    && KheTypedTaskMoveFrame(task, r, task_mt, frame);
  success = false;
  if( DEBUG5 || KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%cMeetMoveAndMTaskMove(", success ? '+' : '-');
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheMeetDebug(target_meet, 1, -1, stderr);
    fprintf(stderr, ", %d, meet_mt %s", offset, KheMoveTypeShow(meet_mt));
    KheMTaskDebug(mt, 1, -1, stderr);
    fprintf(stderr, ", %s, task_mt %s)", KheResourceId(r),
      KheMoveTypeShow(task_mt));
  }
  return KheEjectorRepairEnd(ej,
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE, success);
  ** ***
  return KheEjectorRepairEnd(ej,
    move ? KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE :
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN, success);
  *** **
}
*** */


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

/* *** currenly unused
static bool KheMeetMoveAndTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_TASK task,
  KHE_RESOURCE r)
{
  int max_offset, offset, base, i, junk, index, random_offset;
  KHE_MEET anc_meet, target_meet;  KHE_NODE node, parent_node;
  if( DEBUG5 )
  {
    fprintf(stderr, "%*s[ KheMeetMoveAndTaskMoveMultiRepair(",
      KheEjectorCurrDebugIndent(ej), "");
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
		KheEjectorCurrDebugIndent(ej), "");
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
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}
*** */


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

  /* *** out of play, too hard to adapt from tasks to mtasks
static bool KheMeetMoveAndMTaskMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MOVE_TYPE meet_mt, KHE_MTASK mt,
  KHE_RESOURCE r)
{
  int max_offset, offset, base, i, junk, index, random_offset;
  KHE_MEET anc_meet, target_meet;  KHE_NODE node, parent_node;
  if( DEBUG5 )
  {
    fprintf(stderr, "%*s[ KheMeetMoveAndMTaskMoveMultiRepair(",
      KheEjectorCurrDebugIndent(ej), "");
    KheMTaskDebug(mt, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheResourceId(r));
  }
  if( KheMTaskHasSoleMeet(mt, &anc_meet) )
  {
    ** *** problem here, now fixed thanks to KheMTaskHasSoleMee*
    anc_meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
    anc_meet = NULL;
    *** **
    anc_meet = KheMeetFirstMovable(anc_meet, &junk);
    base = 0;
    while( anc_meet != NULL )
    {
      node = KheMeetNode(anc_meet);
      if( node != NULL && KheNodeParent(node) != NULL && !KheNodeIsVizier(node))
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
	    if( KheMeetMoveAndMTaskMoveRepair(ej, ao, anc_meet, target_meet,
		  offset, meet_mt, mt, r, KHE_MOVE_EJECTING) )
	    {
	      if( DEBUG5 )
		fprintf(stderr,
		  "%*s] KheMeetMoveAndMTaskMoveMultiRepair returning true\n",
		  KheEjectorCurrDebugIndent(ej), "");
	      return true;
	    }
	  }
	}
      }
      base += KheMeetAsstOffset(anc_meet);
      anc_meet = KheMeetAsst(anc_meet);
    }
  }
  if( DEBUG5 )
    fprintf(stderr, "%*s] KheMeetMoveAndMTaskMoveMultiRepair ret false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "increase and decrease resource load - old"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorAddMTasks(                               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,                 */
/*    bool include_preassigned, KHE_AUGMENT_OPTIONS ao, KHE_MTASK_SET mts)   */
/*                                                                           */
/*  Add to mts the mtasks containing the tasks that the resource whose       */
/*  timetable monitor is rtm is assigned to during tg.  Omit mtasks that     */
/*  overlap mtasks that are already present (this includes duplicates).      */
/*  Return false if any preassigned mtasks are encountered, or if mts        */
/*  is empty in the end.                                                     */
/*                                                                           */
/*  Implementation note.  This function cannot be moved to KheMTaskSet,      */
/*  at least not as things stand, because it references ao->mtask_finder.    */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMTasksForResource just below
static bool KheResourceTimetableMonitorAddMTasks(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  KHE_AUGMENT_OPTIONS ao, KHE_MTASK_SET mts)
{
  int i, j, ** pos, ** count;  KHE_TIME t;  KHE_TASK task;  KHE_RESOURCE r2;
  KHE_MTASK mt;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
    for( j = 0;  j < count;  j++ )
    {
      task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
      if( KheTaskIsPreassigned(task, &r2) )
	return false;
      mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
      KheMTaskSetAd dMTaskDisjoint(mts, mt);
    }
  }
  return KheMTaskSetMTaskCount(mts) > 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTasksForResource(KHE_RESOURCE r, KHE_TIME_GROUP tg,    */
/*    KHE_AUGMENT_OPTIONS ao)                                                */
/*                                                                           */
/*  Return a fresh mtask set containing the unpreassigned mtasks running     */
/*  during tg that are assigned r.                                           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_MTASK_SET KheMTasksForResource(KHE_RESOURCE r, KHE_TIME_GROUP tg,
  KHE_AUGMENT_OPTIONS ao)
{
  KHE_MTASK_SET res;  struct khe_mtask_iterator_rec mi_rec;  int pos;
  KHE_MTASK mt;  KHE_RESOURCE r2;
  res = KheMTaskSetMake(ao->mtask_finder);
  KheMTaskIteratorInit(&mi_rec, ao, KheResourceResourceType(r));
  KheForEachMTask(&mi_rec, r, tg, mt)
    if( !KheMTaskIsPreassigned(mt, &r2) &&
	!KheMTaskSetContainsMTask(res, mt, &pos) )
      KheMTaskSetAddMTask(res, mt);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceOverloadMultiRepair(KHE_EJECTOR ej,                      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_TIME_GROUP tg,        */
/*    bool require_zero)                                                     */
/*                                                                           */
/*  Carry out repairs which try to decrease the number of times that from_r  */
/*  is busy within time group tg.  If require_zero is true, only try         */
/*  repairs that will reduce this number to zero.                            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheDecreaseLoadMultiRepair
static bool KheResourceOverloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE from_r, KHE_TIME_GROUP tg,
  bool require_zero, KHE_EXCLUDE_DAYS ed)
{
  KHE_MEET meet;  KHE_RESOURCE_GROUP domain;
  int tg_time_count, all_time_count;
  KHE_SOLN soln;  bool not_ejecting;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_resource_mtask_iterator_rec rmi_rec;
  KHE_RESOURCE r2;  KHE_MTASK from_mt;  KHE_INTERVAL in;
  struct khe_interval_iterator_rec move_ii_rec, swap_ii_rec;

  if( DEBUG29 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheResourceOverloadMultiRepair(ej, %s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(from_r),
      KheTimeGroupId(tg), require_zero ? "true" : "false");

  if( require_zero )
  {
    ** move all of r's tasks running during tg together away from r **
    if( ao->repair_resources &&
      !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(from_r)) )
    {
      ** get the domain and interval of the tasks we need to move **
      domain = NULL;
      in = KheIntervalMake(1, 0);
      KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, from_mt)
      {
	if(KheMTaskIsPreassigned(from_mt,&r2) || KheMTaskAssignIsFixed(from_mt))
	{
	  ** no hope, because one of the mtasks is preassigned or fixed **
	  if( DEBUG29 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning "
	      "false (preass or fix)\n", KheEjectorCurrDebugIndent(ej), "");
	  return false;
	}
	if( domain == NULL )
	  domain = KheMTaskDomain(from_mt);
	in = KheIntervalUnion(in, KheMTaskInterval(from_mt));
      }

      ** must be at least one mtask to move, else why overloaded? **
      HnAssert(KheIntervalLength(in) > 0 && domain != NULL,
	"KheResourceOverloadMultiRepair internal error 1");

      ** try to move r's tasks in in away from r **
      KheResourceIteratorInit(&to_ri_rec, ao, domain, NULL, from_r, true);
      KheIntervalIteratorInit(&move_ii_rec, ao, in, ao->move_widening_max,
	false, false, &ed->unassign_in);
      KheIntervalIteratorInit(&swap_ii_rec, ao, in, ao->swap_widening_max,
	false, ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
	&ed->swap_in);
      if( KheMTaskSwapAndMoveMultiRepair(ej, ao, from_r, &to_ri_rec,
	  &move_ii_rec, &swap_ii_rec, NULL, tg ** , NULL **) )
      {
	if( DEBUG29 && KheEjectorCurrDebug(ej) )
	  fprintf(stderr,"%*s] KheResourceOverloadMultiRepair returning true "
	    "(d)\n", KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
      ** **
      ed->overload_move_in = KheIntervalIteratorFirstDaysInterval(&move_ii_rec);
      ed->overload_swap_in = KheIntervalIteratorFirstDaysInterval(&swap_ii_rec);
      *** **
    }
  }
  else
  {
    ** move each of r's tasks running during tg separately away from r **
    soln = KheEjectorSoln(ej);
    if( ao->repair_resources &&
       !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(from_r)) )
    {
      ** iterate over each separate mtask that it would be good to move **
      KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, from_mt)
      {
	** try to move from_mt away from from_r **
        KheResourceIteratorInit(&to_ri_rec, ao, KheMTaskDomain(from_mt), NULL,
	  from_r, true);
	KheIntervalIteratorInit(&move_ii_rec, ao, KheMTaskInterval(from_mt),
	  ao->move_widening_max, false, false, &ed->unassign_in);
	KheIntervalIteratorInit(&swap_ii_rec, ao, KheMTaskInterval(from_mt),
	  ao->swap_widening_max, false,
	  ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
	  &ed->swap_in);
	if( KheMTaskSwapAndMoveMultiRepair(ej, ao, from_r, &to_ri_rec,
	    &move_ii_rec, &swap_ii_rec, from_mt, tg ** , NULL **) )
	{
	  if( DEBUG29 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning "
	      "true (a)\n", KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
	** ***
	ed->overload_move_in =
	  KheIntervalIteratorFirstDaysInterval(&move_ii_rec);
	ed->overload_swap_in =
	  KheIntervalIteratorFirstDaysInterval(&swap_ii_rec);
	*** **
	if( RANDOMIZED )
	  return false;
      }
    }

    ** try ejecting meet moves of meets in tg away from tg **
    if( ao->repair_times )
    {
      tg_time_count = KheTimeGroupTimeCount(tg);
      all_time_count = KheInstanceTimeCount(KheSolnInstance(soln));
      if( tg_time_count < all_time_count )  ** saves time with workload c's **
      {
	not_ejecting = ao->no_ejecting_moves;
	KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, from_mt)
	{
	  if( KheMTaskHasSoleMeet(from_mt, &meet) )
	  {
	    if( KheMeetMultiRepair(ej, ao, meet, true,
		&KheDecreaseOverlapMeetMoveFn, (void *) tg,
		KHE_OPTIONS_KEMPE_FALSE, !not_ejecting, not_ejecting, false) )
	    {
	      if( DEBUG29 && KheEjectorCurrDebug(ej) )
		fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning "
		  "true (c)\n", KheEjectorCurrDebugIndent(ej), "");
	      return true;
	    }
	    if( RANDOMIZED )
	      return false;
	  }
	}
      }
    }
  }
  if( DEBUG29 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheResourceOverloadMultiRepair returning false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskOverlapsTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)          */
/*                                                                           */
/*  Return true if task overlaps tg.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGainTaskMultiRepair(KHE_EJECTOR ej,                      */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE to_r, KHE_TIME_GROUP tg,          */
/*    bool force)                                                            */
/*                                                                           */
/*  Carry out augments which move a task to to_r within time group tg.  If   */
/*  force is true, do this even at times when to_r is already busy.  The     */
/*  tasks moved can be unassigned or assigned.                               */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheIncreaseLoadMultiRepair
static bool KheResourceGainTaskMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE to_r, KHE_TIME_GROUP tg,
  KHE_EXCLUDE_DAYS ed)
{
  int i;  KHE_RESOURCE_GROUP to_r_rg;  KHE_MTASK from_mt;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_all_mtask_iterator_rec ami_rec;
  struct khe_interval_iterator_rec move_ii_rec, swap_ii_rec;
  KHE_RESOURCE from_r;

  to_r_rg = KheResourceSingletonResourceGroup(to_r);
  KheResourceIteratorInit(&to_ri_rec, ao, to_r_rg, NULL, NULL, false);
  KheAllMTaskIteratorTimeGroupInit(&ami_rec, ao,
    KheResourceResourceType(to_r), tg);
  KheAllMTaskForEach(&ami_rec, from_mt)
  {
    KheIntervalIteratorInit(&move_ii_rec, ao, KheMTaskInterval(from_mt),
      ao->move_widening_max, false, false, &ed->unassign_in);
    KheIntervalIteratorInit(&swap_ii_rec, ao, KheMTaskInterval(from_mt),
      ao->swap_widening_max, false,
      ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
      &ed->swap_in);
    for( i = 0;  i < KheMTaskAsstResourceCount(from_mt);  i++ )
    {
      from_r = KheMTaskAsstResource(from_mt, i);
      if( from_r != to_r && KheMTaskSwapAndMoveMultiRepair(ej, ao, from_r,
	    &to_ri_rec, &move_ii_rec, &swap_ii_rec, from_mt, NULL **, NULL **) )
	return true;
    }
    if( KheMTaskNeedsAssignment(from_mt) &&
	KheMTaskSwapAndMoveMultiRepair(ej, ao, NULL,
	  &to_ri_rec, &move_ii_rec, &swap_ii_rec, from_mt, NULL ** , NULL **) )
      return true;
    ** ***
    ed->underload_move_in = KheIntervalIteratorFirstDaysInterval(&move_ii_rec);
    ed->underload_swap_in = KheIntervalIteratorFirstDaysInterval(&swap_ii_rec);
    *** **
  }
  return false;
}
*** */


/* this might become the actual one soon */
/* *** nope, it got superseded
static bool KheResourceGainTaskMultiRepairAlt(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE to_r, KHE_TIME_GROUP tg,
  KHE_EXCLUDE_DAYS ed)
{
  int random_offset;  KHE_RESOURCE_GROUP to_r_rg, full_rg;
  KHE_MTASK from_mt;  KHE_RESOURCE from_r, r2;
  KHE_INTERVAL in, unassign_in, swap_in;
  struct khe_resource_iterator_rec from_ri_rec;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_all_mtask_iterator_rec ami_rec;
  struct khe_interval_iterator_rec move_ii_rec, swap_ii_rec;
  struct khe_resource_mtask_iterator_rec rmi_rec;

  to_r_rg = KheResourceSingletonResourceGroup(to_r);
  full_rg = KheResourceTypeFullResourceGroup(KheResourceResourceType(to_r));
  KheResourceIteratorInit(&to_ri_rec, ao, to_r_rg, NULL, NULL, false);

  ** find assigned mtasks and try those **
  KheResourceIteratorInit(&from_ri_rec, ao, full_rg, NULL, to_r, false);
  random_offset = 11 * KheEjectorCurrAugmentCount(ej) +
    53 * KheSolnDiversifier(ao->soln);
  KheForEachResource(&from_ri_rec, random_offset, from_r)
    KheResourceMTaskForEach(&rmi_rec, ao, from_r, tg, from_mt)
    {
      ** from_mt is an mtask, currently assigned from_r, lying in tg **
      ** this is a different from_r/from_mt on every iteration **
      if( KheMTaskIsPreassigned(from_mt,&r2) || KheMTaskAssignIsFixed(from_mt) )
	continue;
      unassign_in = ed->unassign_in;
      swap_in = ed->swap_in;
      in = KheMTaskInterval(from_mt);
      KheIntervalIteratorInit(&move_ii_rec, ao, in, ao->move_widening_max,
	false, false, &unassign_in);
      KheIntervalIteratorInit(&swap_ii_rec, ao, in, ao->swap_widening_max,
	false, ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
	&swap_in);
      if( KheMTaskSwapAndMoveMultiRepair(ej, ao, from_r, &to_ri_rec,
	  &move_ii_rec, &swap_ii_rec, NULL, tg ** , NULL **) )
	return true;
    }

  ** find unassigned mtasks and try those **
  KheAllMTaskIteratorTimeGroupInit(&ami_rec, ao,
    KheResourceResourceType(to_r), tg);
  random_offset = 37 * KheEjectorCurrAugmentCount(ej) +
    5 * KheSolnDiversifier(KheEjectorSoln(ej));
  KheAllMTaskForEach(&ami_rec, from_mt)
    if( KheMTaskNeedsAssignment(from_mt) )
    {
      ** from_mt is an mtask, currently unassigned, lying in tg **
      unassign_in = ed->unassign_in;
      swap_in = ed->swap_in;
      KheIntervalIteratorInit(&move_ii_rec, ao, KheMTaskInterval(from_mt),
	ao->move_widening_max, false, false, &unassign_in);
      KheIntervalIteratorInit(&swap_ii_rec, ao, KheMTaskInterval(from_mt),
	ao->swap_widening_max, false,
	ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
	&swap_in);
      if( KheMTaskSwapAndMoveMultiRepair(ej, ao, NULL,
	    &to_ri_rec, &move_ii_rec, &swap_ii_rec, from_mt, NULL) )
	return true;
    }
  return false;
}
*** */


/* *** old version without mtask iterator
static bool KheResourceGainTaskMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool force, KHE_MTASK_SET scratch_mts)
{
  int i, j, k, index, random_offset, tg_time_count;  KHE_RESOURCE_TYPE rt;
  KHE_SOLN soln;  KHE_RESOURCE_GROUP r_rg;  KHE_TIME t;  KHE_MEET meet;
  KHE_TASK r_task, task;  KHE_MTASK mt, prev_mt;  KHE_RESOURCE r2, from_r;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  struct khe_resource_iterator_rec ri_rec;

  ** boilerplate **
  soln = KheEjectorSoln(ej);
  r_rg = KheResourceSingletonResourceGroup(r);
  rt = KheResourceResourceType(r);
  rtm = KheResourceTimetableMonitor(soln, r);

  ** try each time of tg **
  tg_time_count = KheTimeGroupTimeCount(tg);
  random_offset = 37 * KheEjectorCurrAugmentCount(ej) +
    5 * KheSolnDiversifier(KheEjectorSoln(ej));;
  prev_mt = NULL;
  for( i = 0;  i < tg_time_count;  i++ )
  {
    index = (random_offset + i) % tg_time_count;
    t = KheTimeGroupTime(tg, index);

    ** skip t if r is already busy on its day with a task overlapping tg **
    if( !force )
    {
      KheResourceBusyOnDayOfTime(rtm, ao->frame, t, &r_task);
      if( r_task != NULL && (KheTaskIsPreassigned(r_task, &r2) ||
	  KheTaskOverlapsTimeGroup(r_task, tg)) )
	continue;
    }

    ** try to move a task to r at t **
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(ao->etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(ao->etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	if( KheTaskResourceType(task) == rt )
	{
	  mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
	  if( mt != prev_mt &&
	      KheMTaskResourceReas signTaskCheck(mt, task, r) )
	  {
	    from_r = KheTaskAsstResource(task);
	    KheResourceIteratorInit(&ri_rec, ao, r_rg, NULL, NULL, false);
	    if( KheMTaskMoveAugment(ej, ao, mt, from_r, &ri_rec, NULL,
		  NULL, scratch_mts) )
	      return true;
	    ** ***
	    if( KheMTaskMoveAugment(ej, ao, mt, from_r, r_rg, NULL, false, NULL,
		  NULL, scratch_mts) )
	    *** **
	    prev_mt = mt;
	  }
	  ** ***
	  task = KheTaskProperRoot(task);
	  if( KheTaskMoveResourceCheck(task, r) &&
	      (prev_task == NULL || !KheTaskEquiv(prev_task, task)) )
	  task = KheTaskProperRoot(task);
	  if( KheTaskMoveResourceCheck(task, r) &&
	      (prev_task == NULL || !KheTaskEquiv(prev_task, task)) )
	  {
	    if( KheTaskMoveAugment(ej, ao, task, r_rg, NULL, false, NULL,
		  NULL, scratch_ts) )
	      return true;
	    prev_task = task;
	  }
	  *** **
	}
      }
    }
  }
  return false;
}
*** */


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

/* *** replaced by KheIncreaseLoadMultiRepair
static bool KheResourceUnderloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool allow_zero, KHE_EXCLUDE_DAYS ed)
{
  int i, tg_time_count, all_time_count, durn, random_offset, index;
  bool not_ejecting;  KHE_MEET meet;  KHE_TIME t;  KHE_SOLN soln;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  soln = KheEjectorSoln(ej);
  tg_time_count = KheTimeGroupTimeCount(tg);

  ** if( KheEjectorRepairResources(ej) && **
  if( ao->repair_resources &&
      !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(r)) )
  {
    ** try clearing out tg altogether, if allow_zero **
    if( allow_zero && KheResourceOverloadMultiRepair(ej, ao, r, tg, true, ed) )
      return true;

    ** for each time t of tg, try to make r busy at t **
    if( KheResourceGainTaskMultiRepair(ej, ao, r, tg, ed) )
      return true;
  }

  ** if( KheEjectorRepairTimes(ej) ) **
  if( ao->repair_times )
  {
    all_time_count = KheInstanceTimeCount(KheSolnInstance(soln));
    if( tg_time_count < all_time_count )
    {
      ** try ejecting meet moves into tg **
      ** NB finding tasks to move into tg, so might as well search whole list **
      random_offset = KheAugmentOptionsRandomOffset(ao, 7, 23);
      ** not_ejecting = KheEjectorBasicNotEjecting(ej); **
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

      ** try clearing out tg altogether, using a cluster-like repair **
      rtm = KheResourceTimetableMonitor(soln, r);
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
*** */


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

/* *** replaced by KheIncreaseOrDecreaseLoadMultiRepair
static bool KheOverUnderMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r, KHE_TIME_GROUP tg, bool over, bool require_zero,
  bool allow_zero, KHE_EXCLUDE_DAYS ed)
{
  if( over )
    return KheResourceOverloadMultiRepair(ej, ao, r, tg, require_zero, ed);
  else
    return KheResourceUnderloadMultiRepair(ej, ao, r, tg, allow_zero, ed);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "multi-repairs for resource and event resource defects"        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheGetIntervalAndDomain(KHE_EJECTOR ej,                             */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,             */
/*    KHE_INTERVAL *res_in, KHE_RESOURCE_GROUP *res_domain)                  */
/*                                                                           */
/*  We're planning to move the tasks assigned r during tg.  Get the          */
/*  interval that these tasks lie within, and get the domain of any          */
/*  one of them.                                                             */
/*                                                                           */
/*  Return true if none of the tasks found are preassigned or fixed.         */
/*                                                                           */
/*****************************************************************************/

static bool KheGetIntervalAndDomain(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  KHE_INTERVAL *res_in, KHE_RESOURCE_GROUP *res_domain)
{
  struct khe_resource_mtask_iterator_rec rmi_rec;
  KHE_MTASK from_mt;  KHE_RESOURCE r2;  KHE_INTERVAL union_in;
  if( DEBUG38 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s  [ KheGetIntervalAndDomain(%s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "",
      KheResourceId(r), KheTimeGroupId(tg));
  *res_domain = NULL;
  *res_in = KheIntervalMake(1, 0);
  KheResourceMTaskForEach(&rmi_rec, ao, r, tg, from_mt)
  {
    if( KheMTaskIsPreassigned(from_mt, &r2) || KheMTaskAssignIsFixed(from_mt) )
    {
      /* no hope, because one of the mtasks is preassigned or fixed */
      if( DEBUG38 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s  ] KheGetIntervalAndDomain returning false\n",
	  KheEjectorCurrDebugIndent(ej), "");
      return false;
    }
    if( *res_domain == NULL )
      *res_domain = KheMTaskDomain(from_mt);
    union_in = KheIntervalUnion(*res_in, KheMTaskInterval(from_mt));
    if( DEBUG38 && KheEjectorCurrDebug(ej) )
    {
      KheMTaskDebug(from_mt, 2, KheEjectorCurrDebugIndent(ej), stderr);
      fprintf(stderr, "%*s    %s = KheIntervalUnion(%s, %s)\n",
	KheEjectorCurrDebugIndent(ej), "",
	KheIntervalShow(union_in, ao->frame),
	KheIntervalShow(*res_in, ao->frame),
	KheIntervalShow(KheMTaskInterval(from_mt), ao->frame));
    }
    *res_in = union_in;
  }
  HnAssert(*res_domain != NULL, "KheGetIntervalAndDomain internal interval");
  if( DEBUG38 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s  ] KheGetIntervalAndDomain returning true(%s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "",
      KheIntervalShow(*res_in, ao->frame), KheResourceGroupId(*res_domain));
  return true;
}


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
/*  bool KheDoDecreaseLoad(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,           */
/*    KHE_RESOURCE r, KHE_TIME_GROUP tg, KHE_EXCLUDE_DAYS ed,                */
/*    KHE_INTERVAL kernel_in, KHE_RESOURCE_GROUP domain)                     */
/*                                                                           */
/*  Carry out the swaps and moves of KheDecreaseLoadMultiRepair for the      */
/*  given kernel_in and domain.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheDoDecreaseLoad(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r, KHE_TIME_GROUP tg, KHE_EXCLUDE_DAYS ed,
  KHE_INTERVAL kernel_in, KHE_RESOURCE_GROUP domain)
{
  struct khe_interval_iterator_rec unassign_ii_rec, swap_ii_rec;
  struct khe_resource_iterator_rec other_ri_rec;
  KHE_INTERVAL in;  KHE_RESOURCE other_r;
  if( DEBUG37 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheDoDecreaseLoad(ej, %s, %s, %s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r), KheTimeGroupId(tg),
      KheExcludeDaysShow(ed, ao->frame), KheIntervalShow(kernel_in, ao->frame),
      KheResourceGroupId(domain));

  /* try widened swaps */
  KheIntervalIteratorInit(&swap_ii_rec, ao, kernel_in,
    ao->swap_widening_max, false,
    ao->full_widening_on && KheEjectorCurrLength(ej) == 1, &ed->swap_in);
  KheResourceIteratorInit(&other_ri_rec, ao, domain, NULL, r, false);
  KheForEachInterval(&swap_ii_rec, in)
    KheForEachResource(&other_ri_rec, other_r)
      if( KheSwapRepair(ej, ao, in, true, r, other_r, NULL, tg) )
      {
	if( DEBUG37 && KheEjectorCurrDebug(ej) )
	  fprintf(stderr, "%*s] KheDoDecreaseLoad returning true\n",
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }

  /* try widened unassignments */
  KheIntervalIteratorInit(&unassign_ii_rec, ao, kernel_in,
    ao->move_widening_max, false, false, &ed->unassign_in);
  KheForEachInterval(&unassign_ii_rec, in)
    if( KheMoveRepair(ej, ao, in, r, NULL, NULL) )
    {
      if( DEBUG37 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s] KheDoDecreaseLoad returning true\n",
	  KheEjectorCurrDebugIndent(ej), "");
      return true;
    }

  /* no luck */
  if( DEBUG37 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheDoDecreaseLoad returning false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDecreaseLoadMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,  */
/*    KHE_RESOURCE r, KHE_TIME_GROUP tg1, KHE_TIME_GROUP tg2,                */
/*    bool require_zero, KHE_EXCLUDE_DAYS ed)                                */
/*                                                                           */
/*  Multi-repair function that tries repairs which decrease the workload     */
/*  of r during time group tg1 (and through to tg2 if non-NULL).  If         */
/*  require_zero is true, only repairs that decrease the load to zero are    */
/*  any use.                                                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheDecreaseLoadMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool require_zero, KHE_EXCLUDE_DAYS ed)
{
  KHE_RESOURCE_GROUP domain;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r2;
  KHE_INTERVAL kernel_in;  bool not_ejecting;  KHE_MTASK from_mt;
  int tg_time_count, all_time_count;  KHE_MEET meet;  KHE_TASK from_task;
  struct khe_resource_mtask_iterator_rec rmi_rec;
  struct khe_resource_task_iterator_rec rti_rec;

  if( DEBUG29 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheDecreaseLoadMultiRepair(ej, %s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r),
      KheTimeGroupId(tg), require_zero ? "true" : "false");

  rt = KheResourceResourceType(r);
  if( require_zero )
  {
    /* move all of r's tasks running during tg1-tg2 together away from r */
    if( ao->repair_resources && !KheResourceTypeDemandIsAllPreassigned(rt) &&
        KheGetIntervalAndDomain(ej, ao, r, tg, &kernel_in, &domain) &&
        KheDoDecreaseLoad(ej, ao, r, tg, ed, kernel_in, domain) )
    {
      if( DEBUG29 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s] KheDecreaseLoadMultiRepair returning true (a)\n",
	  KheEjectorCurrDebugIndent(ej), "");
      return true;
    }
  }
  else
  {
    /* move each of r's tasks running during tg1/tg2 separately away from r */
    if( ao->repair_resources && !KheResourceTypeDemandIsAllPreassigned(rt) )
    {
      /* iterate over each separate mtask that it would be good to move */
      KheResourceMTaskForEach(&rmi_rec, ao, r, tg, from_mt)
        if( !KheMTaskIsPreassigned(from_mt, &r2) &&
	    !KheMTaskAssignIsFixed(from_mt) &&
	    KheDoDecreaseLoad(ej, ao, r, tg, ed, KheMTaskInterval(from_mt),
	      KheMTaskDomain(from_mt)) )
	{
	  if( DEBUG29 && KheEjectorCurrDebug(ej) )
	    fprintf(stderr, "%*s] KheDecreaseLoadMultiRepair returning true"
	      " (b)\n", KheEjectorCurrDebugIndent(ej), "");
	  return true;
	}
    }

    /* try ejecting meet moves of meets in tg away from tg */
    if( ao->repair_times )
    {
      tg_time_count = KheTimeGroupTimeCount(tg);
      all_time_count = KheInstanceTimeCount(KheSolnInstance(ao->soln));
      if( tg_time_count < all_time_count )  /* saves time with workload c's */
      {
	not_ejecting = ao->no_ejecting_moves;
	KheResourceTaskForEach(&rti_rec, ao, r, tg, from_task)
	{
	  meet = KheTaskMeet(from_task);
	  if( meet != NULL )
	  {
	    if( KheMeetMultiRepair(ej, ao, meet, true,
		&KheDecreaseOverlapMeetMoveFn, (void *) tg,
		KHE_OPTIONS_KEMPE_FALSE, !not_ejecting, not_ejecting, false) )
	    {
	      if( DEBUG29 && KheEjectorCurrDebug(ej) )
		fprintf(stderr, "%*s] KheDecreaseLoadMultiRepair returning "
		  "true (c)\n", KheEjectorCurrDebugIndent(ej), "");
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoClearInterval(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,          */
/*    KHE_RESOURCE r, KHE_TIME_GROUP tg1, KHE_EXCLUDE_DAYS ed,               */
/*    KHE_INTERVAL kernel_in, KHE_RESOURCE_GROUP domain)                     */
/*                                                                           */
/*  Like KheDoDecreaseLoad but with no expansion of kernel_in.               */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheDoClearInterval(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_RESOURCE r, KHE_TIME_GROUP tg1, KHE_EXCLUDE_DAYS ed,
  KHE_INTERVAL kernel_in, KHE_RESOURCE_GROUP domain)
{
  struct khe_resource_iterator_rec other_ri_rec;  KHE_RESOURCE other_r;
  if( DEBUG37 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheDoClearInterval(ej, %s, %s, %s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r), KheTimeGroupId(tg1),
      KheExcludeDaysShow(ed, ao->frame), KheIntervalShow(kernel_in, ao->frame),
      KheResourceGroupId(domain));

  ** try unwidened swaps **
  KheResourceIteratorInit(&other_ri_rec, ao, domain, NULL, r, false);
  KheForEachResource(&other_ri_rec, other_r)
    if( KheSwapRepair(ej, ao, kernel_in, true, r, other_r, NULL, tg1) )
    {
      if( DEBUG37 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s] KheDoClearInterval returning true\n",
	  KheEjectorCurrDebugIndent(ej), "");
      return true;
    }

  ** try unwidened unassignments **
  if( KheMoveRepair(ej, ao, kernel_in, r, NULL, NULL) )
  {
    if( DEBUG37 && KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%*s] KheDoClearInterval returning true\n",
	KheEjectorCurrDebugIndent(ej), "");
    return true;
  }

  ** no luck **
  if( DEBUG37 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] KheDoClearInterval returning false\n",
      KheEjectorCurrDebugIndent(ej), "");
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheClearIntervalMultiRepair(KHE_EJECTOR ej,                         */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg1,            */
/*    KHE_TIME_GROUP tg2, KHE_EXCLUDE_DAYS ed)                               */
/*                                                                           */
/*  Similar to KheDecreaseLoadMultiRepair, except that we want to clear      */
/*  everything from tg1 to tg2 inclusive.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheClearIntervalMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg1,
  KHE_TIME_GROUP tg2, KHE_EXCLUDE_DAYS ed)
{
  KHE_INTERVAL kernel_in, kernel_in1, kernel_in2;
  KHE_RESOURCE_GROUP domain, domain1, domain2;  KHE_RESOURCE_TYPE rt;
  rt = KheResourceResourceType(r);
  if( ao->repair_resources && !KheResourceTypeDemandIsAllPreassigned(rt) &&
      KheGetIntervalAndDomain(ej, ao, r, tg1, &kernel_in1, &domain1) &&
      KheGetIntervalAndDomain(ej, ao, r, tg2, &kernel_in2, &domain2) )
  {
    kernel_in = KheIntervalUnion(kernel_in1, kernel_in2);
    domain = (domain1 == NULL ? domain2 : domain1);
    if( KheDoDecreaseLoad(ej, ao, r, tg1, ed, kernel_in, domain) )
      return true;
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

/* ***
static bool KheDecreaseLoadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool allow_zero, KHE_EXCLUDE_DAYS ed)
{
  int i, tg_time_count, all_time_count, durn, random_offset, index;
  bool not_ejecting;  KHE_MEET meet;  KHE_TIME t;  KHE_SOLN soln;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  soln = KheEjectorSoln(ej);
  tg_time_count = KheTimeGroupTimeCount(tg);

  ** if( KheEjectorRepairResources(ej) && **
  if( ao->repair_resources &&
      !KheResourceTypeDemandIsAllPreassigned(KheResourceResourceType(r)) )
  {
    ** try clearing out tg altogether, if allow_zero **
    if( allow_zero && KheResourceOverloadMultiRepair(ej, ao, r, tg, true, ed) )
      return true;

    ** for each time t of tg, try to make r busy at t **
    if( KheResourceGainTaskMultiRepair(ej, ao, r, tg, ed) )
      return true;
  }

  ** if( KheEjectorRepairTimes(ej) ) **
  if( ao->repair_times )
  {
    all_time_count = KheInstanceTimeCount(KheSolnInstance(soln));
    if( tg_time_count < all_time_count )
    {
      ** try ejecting meet moves into tg **
      ** NB finding tasks to move into tg, so might as well search whole list **
      random_offset = 7 * KheEjectorCurrAugmentCount(ej) +
	23 * KheSolnDiversifier(KheEjectorSoln(ej));
      ** not_ejecting = KheEjectorBasicNotEjecting(ej); **
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

      ** try clearing out tg altogether, using a cluster-like repair **
      rtm = KheResourceTimetableMonitor(soln, r);
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg,                     */
/*    KHE_AUGMENT_OPTIONS ao)                                                */
/*                                                                           */
/*  Return the smallest interval of days containing tg.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL KheTimeGroupInterval(KHE_TIME_GROUP tg,
  KHE_AUGMENT_OPTIONS ao)
{
  KHE_TIME t1, t2;  int count;
  count = KheTimeGroupTimeCount(tg);
  if( count == 0 )
    return KheIntervalMake(1, 0);
  else
  {
    t1 = KheTimeGroupTime(tg, 0);
    t2 = KheTimeGroupTime(tg, count - 1);
    return KheIntervalMake(KheFrameTimeIndex(ao->frame, t1),
      KheFrameTimeIndex(ao->frame, t2));
  }
}


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
/*  bool KheIncreaseLoadMultiRepair(KHE_EJECTOR ej,                          */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,             */
/*    bool allow_zero, KHE_EXCLUDE_DAYS ed)                                  */
/*                                                                           */
/*  Multi-repair function that tries repairs which increase the workload     */
/*  of r during time group tg.  If allow_zero is true, another option        */
/*  is to decrease the load to zero.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheIncreaseLoadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool allow_zero, KHE_EXCLUDE_DAYS ed)
{
  int i, tg_time_count, all_time_count, durn, random_offset, index, need;
  bool not_ejecting;  KHE_MEET meet;  KHE_TIME t;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_RESOURCE_TYPE rt;
  KHE_INTERVAL day_in, in;  KHE_RESOURCE other_r;  KHE_MTASK mt;
  struct khe_resource_iterator_rec other_ri_rec;
  struct khe_all_mtask_iterator_rec ami_rec;
  struct khe_interval_iterator_rec assign_ii_rec, swap_ii_rec;
  tg_time_count = KheTimeGroupTimeCount(tg);

  /* if( KheEjectorRepairResources(ej) && */
  rt = KheResourceResourceType(r);
  if( ao->repair_resources && !KheResourceTypeDemandIsAllPreassigned(rt) )
  {
    /* try clearing out tg altogether, if allow_zero */
    if( allow_zero && KheDecreaseLoadMultiRepair(ej, ao, r, tg, true, ed) )
      return true;

    /* try widened swaps */
    day_in = KheTimeGroupInterval(tg, ao);
    for( i = KheIntervalFirst(day_in);  i <= KheIntervalLast(day_in);  i++ )
    {
      KheIntervalIteratorInit(&swap_ii_rec, ao, KheIntervalMake(i, i),
	ao->swap_widening_max, false,
	ao->full_widening_on && KheEjectorCurrLength(ej) == 1, &ed->swap_in);
      KheResourceIteratorInit(&other_ri_rec, ao,
	KheResourceTypeFullResourceGroup(rt), NULL, r, false);
      KheForEachInterval(&swap_ii_rec, in)
	KheForEachResource(&other_ri_rec, other_r)
	  if( KheSwapRepair(ej, ao, in, true, r, other_r, NULL, NULL) )
	    return true;
          /* *** trying to increase load in tg, not block it JeffK 16/01/24
	  if( KheSwapRepair(ej, ao, in, true, r, other_r, NULL, tg) )
	    return true;
	  *** */
    }

    /* try widened assignments of mtasks that need assignment, and that don't */
    for( need = 1;  need >= 0;  need-- )
    {
      KheAllMTaskIteratorTimeGroupInit(&ami_rec, ao, rt, tg);
      KheAllMTaskForEach(&ami_rec, mt)
	if( need == (int) KheMTaskNeedsAssignment(mt) )
	{
	  KheIntervalIteratorInit(&assign_ii_rec, ao, KheMTaskInterval(mt),
	    ao->move_widening_max, false, false, NULL);
	  KheForEachInterval(&assign_ii_rec, in)
	    if( KheMoveRepair(ej, ao, in, NULL, r, mt) )
	      return true;
	}
    }
  }

  if( ao->repair_times )
  {
    /* *** old code, auditing it is still to do, but it seems OK */
    all_time_count = KheInstanceTimeCount(KheSolnInstance(ao->soln));
    if( tg_time_count < all_time_count )
    {
      /* try ejecting meet moves into tg */
      /* NB finding tasks to move into tg, so might as well search whole list */
      random_offset = KheAugmentOptionsRandomOffset(ao, 7, 23);
      /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
      not_ejecting = ao->no_ejecting_moves;
      for( i = 0;  i < KheResourceAssignedTaskCount(ao->soln, r);  i++ )
      {
	index = (random_offset + i) % KheResourceAssignedTaskCount(ao->soln, r);
	meet = KheTaskMeet(KheResourceAssignedTask(ao->soln, r, index));
	t = KheMeetAsstTime(meet);
	durn = KheMeetDuration(meet);
	if( t != NULL && KheTimeGroupOverlap(tg, t, durn) < durn &&
	  KheMeetMultiRepair(ej, ao, meet, false, &KheIncreaseOverlapMeetMoveFn,
	    (void *) tg, KHE_OPTIONS_KEMPE_TRUE, !not_ejecting, not_ejecting,
	    false) )
	  return true;
      }

      /* try clearing out tg altogether, using a cluster-like repair */
      rtm = KheResourceTimetableMonitor(ao->soln, r);
      if( allow_zero && (KheEjectorCurrLength(ej) == 1 || KheOneMeet(rtm, tg)) )
      {
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewGlobalVisit(ao->soln);
	if( KheMeetBoundRepair(ej, r, KheSolnComplementTimeGroup(ao->soln,tg)) )
	  return true;
      }
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheIncreaseOrDecreaseLoadMultiRepair(KHE_EJECTOR ej,                */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg1,            */
/*    KHE_TIME_GROUP tg2, bool decrease, bool require_zero,                  */
/*    bool allow_zero, KHE_EXCLUDE_DAYS ed)                                  */
/*                                                                           */
/*  Call KheDecreaseLoadMultiRepair if decrease is true, or call             */
/*  KheIncreaseLoadMultiRepair if decrease is false.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheIncreaseOrDecreaseLoadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP tg,
  bool decrease, bool require_zero, bool allow_zero, KHE_EXCLUDE_DAYS ed)
{
  bool res;
  if( DEBUG35 )
    fprintf(stderr, "%*s[ KheIncreaseOrDecreaseLoadMultiRepair(%s, %s, %s)\n",
      KheEjectorCurrDebugIndent(ej), "", KheResourceId(r),
      KheTimeGroupId(tg), decrease ?
        (require_zero ? "decrease && require_zero" : "decrease") :
        (allow_zero ? "increase && allow_zero" : "increase"));
  if( decrease )
    res = KheDecreaseLoadMultiRepair(ej, ao, r, tg, require_zero, ed);
  else
    res = KheIncreaseLoadMultiRepair(ej, ao, r, tg, allow_zero, ed);
  if( DEBUG35 )
    fprintf(stderr, "%*s] KheIncreaseOrDecreaseLoadMultiRepair returning %s\n",
      KheEjectorCurrDebugIndent(ej), "", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao, */
/*    KHE_EVENT_RESOURCE er, KHE_RESOURCE_ITERATOR to_ri,                    */
/*    KHE_EXCLUDE_DAYS ed, KHE_MTASK *prev_mt, KHE_RESOURCE *prev_r)         */
/*                                                                           */
/*  Try to ensure that the tasks derived from event resource er are          */
/*  assigned resources from to_ri.  Here to_ri may include NULL.  As         */
/*  usual, exclude repairs in intervals forbidden by ed.                     */
/*                                                                           */
/*  Here (*prev_mt, *prev_r) is the mtask and from_r that this was tried     */
/*  on previously, or NULL if none.  This helps to avoid trying the same     */
/*  repair repeatedly.  That would probably not happen anyway, except when   */
/*  a limit resources monitor monitors all of the tasks of a single mtask,   */
/*  and two or more of those are unassigned.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceMultiRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_EVENT_RESOURCE er, KHE_RESOURCE_ITERATOR to_ri,
  KHE_EXCLUDE_DAYS ed, KHE_MTASK *prev_mt, KHE_RESOURCE *prev_r)
{
  int i, count, random_offset;  KHE_TASK task;
  KHE_RESOURCE from_r, other_r;  KHE_MTASK from_mt;  KHE_INTERVAL in;
  struct khe_interval_iterator_rec swap_ii_rec, unassign_ii_rec, assign_ii_rec;
  random_offset = KheAugmentOptionsRandomOffset(ao, 23, 29);
  count = KheEventResourceTaskCount(ao->soln, er);
  for( i = 0;  i < count;  i++ )
  {
    /* here task is one of the tasks being monitored */
    task = KheEventResourceTask(ao->soln, er, (i + random_offset) % count);
    from_r = KheTaskAsstResource(task);
    from_mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
    if( (from_mt != *prev_mt || from_r != *prev_r) &&
	!KheResourceIteratorContainsResource(to_ri, from_r))
    {
      if( from_r != NULL )
      {
	/* try widened swaps */
	KheIntervalIteratorInit(&swap_ii_rec, ao, KheMTaskInterval(from_mt),
	  ao->swap_widening_max, false,
	  ao->full_widening_on && KheEjectorCurrLength(ej) == 1, &ed->swap_in);
	KheForEachInterval(&swap_ii_rec, in)
	  KheForEachNonNullResource(to_ri, other_r)
	    if( KheSwapRepair(ej, ao, in, true, from_r, other_r, NULL, NULL) )
	      return true;

	/* try widened unassignments, if allowed by to_ri */
	if( KheResourceIteratorContainsResource(to_ri, NULL) )
	{
	  KheIntervalIteratorInit(&unassign_ii_rec, ao,
	    KheMTaskInterval(from_mt), ao->move_widening_max,
	    false, false, &ed->unassign_in);
	  KheForEachInterval(&unassign_ii_rec, in)
	    if( KheMoveRepair(ej, ao, in, from_r, NULL, NULL) )
	      return true;
	}
      }
      else
      {
	/* try widened assignments */
	KheIntervalIteratorInit(&assign_ii_rec, ao, KheMTaskInterval(from_mt),
	  ao->move_widening_max, false, false, NULL);
	KheForEachInterval(&assign_ii_rec, in)
	  KheForEachNonNullResource(to_ri, other_r)
	    if( KheMoveRepair(ej, ao, in, NULL, other_r, from_mt) )
	      return true;
      }
      *prev_mt = from_mt;
      *prev_r = from_r;
    }
  }
  return false;
}


/* *** old version
static bool KheEventResourceMoveMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_EVENT_RESOURCE er,
  KHE_RESOURCE_ITERATOR to_ri, ** KHE_MONITOR d, **
  KHE_MTASK *prev_mt, KHE_RESOURCE *prev_r)
{
  int i, count, random_offset;  KHE_SOLN soln;  KHE_TASK task;
  KHE_RESOURCE from_r;  KHE_MTASK from_mt;
  struct khe_interval_iterator_rec move_ii_rec, swap_ii_rec;
  soln = KheEjectorSoln(ej);
  random_offset = KheAugmentOptionsRandomOffset(ao, 23, 29);
  count = KheEventResourceTaskCount(soln, er);
  for( i = 0;  i < count;  i++ )
  {
    ** here task is one of the tasks being monitored **
    task = KheEventResourceTask(soln, er, (i + random_offset) % count);
    from_r = KheTaskAsstResource(task);
    from_mt = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
    if( (from_mt != *prev_mt || from_r != *prev_r) &&
	!KheResourceIteratorContainsResource(to_ri, from_r))
    {
      ** try moving from_mt to a resource from to_ri **
      KheIntervalIteratorInit(&move_ii_rec, ao, KheMTaskInterval(from_mt),
	ao->move_widening_max, false, false, NULL);
      KheIntervalIteratorInit(&swap_ii_rec, ao, KheMTaskInterval(from_mt),
	ao->swap_widening_max, false,
	ao->full_widening_on && KheEjectorCurrLength(ej) == 1, NULL);
      if( KheMTaskSwapAndMoveMultiRepair(ej, ao, from_r, to_ri,
	    &move_ii_rec, &swap_ii_rec, from_mt, NULL ** , d **) )
	return true;
      ** ***
      ed->overload_move_in = KheIntervalIteratorFirstDaysInterval(&move_ii_rec);
      ed->overload_swap_in = KheIntervalIteratorFirstDaysInterval(&swap_ii_rec);
      *** **

      ** if the domain allows unassignment only, try a double move **
      ** *** not sure about this, understanding it is sti ll to do
      t = KheMeetAsstTime(KheTaskMeet(task));
      if( KheResourceIteratorContainsNullOnly(ri) && t != NULL && r != NULL )
      {
	t_tg = KheTimeSingletonTimeGroup(t);
	if( KheResourceGainTaskMultiRepair(ej, ao, r, t_tg, true) )
	  return true;
      }
      *** **
      if( RANDOMIZED )
	return false;
      *prev_mt = from_mt;
      *prev_r = from_r;
    }
  }
  return false;
}
*** */


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
  KHE_EVENT e;  KHE_MEET meet1, meet2;
  if( DEBUG4 )
  {
    fprintf(stderr, "%*s[ KheSplitEventsAugment(",
      KheEjectorCurrDebugIndent(ej), "");
    KheMonitorDebug(d, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  HnAssert(KheMonitorTag(d) == KHE_SPLIT_EVENTS_MONITOR_TAG ||
    KheMonitorTag(d) == KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG,
    "KheSplitEventsAugment internal error 1");
  /* if( KheEjectorUseSplitMoves(ej) ) */
  if( ao->split_moves )
  {
    e = KheSplitMonitorEvent(d);
    if( !KheSolnEventMeetsVisited(ao->soln, e, 0) )
    {
      /* get split and merge suggestions */
      sa = KheSplitAnalyserOption(KheEjectorOptions(ej), "ts_split_analyser",
	ao->soln);
      /* ***
      sa = (KHE_SPLIT_ANALYSER) KheOptionsGetObject(KheEjectorOptions(ej),
	"ts_split_analyser", NULL);
      *** */
      /* sa = KheOptionsStructuralSplitAnalyser(KheEjectorOptions(ej)); */
      KheSolnEventMeetsVisit(ao->soln, e);
      KheSplitAnalyserAnalyse(sa, e);
      if( DEBUG10 )
	KheSplitAnalyserDebug(sa, 1, KheEjectorCurrDebugIndent(ej), stderr);

      /* carry out sa's split suggestions */
      for( i = 0;  i < KheSplitAnalyserSplitSuggestionCount(sa);  i++ )
      {
	KheSplitAnalyserSplitSuggestion(sa, i, &merged_durn, &split1_durn);
	for( j = 0;  j < KheEventMeetCount(ao->soln, e);  j++ )
	{
	  meet1 = KheEventMeet(ao->soln, e, j);
	  if( KheMeetAsst(meet1)!=NULL && KheMeetDuration(meet1)==merged_durn )
	  {
            if( KheMeetSplitRepair(ej, meet1, split1_durn) )
	    {
	      if( DEBUG4 )
		fprintf(stderr, "%*s] KheSplitEventsAugment ret true (a)\n",
		  KheEjectorCurrDebugIndent(ej), "");
	      return true;
	    }
	  }
	}
      }

      /* carry out sa's merge suggestions */
      for( i = 0;  i < KheSplitAnalyserMergeSuggestionCount(sa);  i++ )
      {
	KheSplitAnalyserMergeSuggestion(sa, i, &split1_durn, &split2_durn);
	for( j = 0;  j < KheEventMeetCount(ao->soln, e);  j++ )
	{
	  meet1 = KheEventMeet(ao->soln, e, j);
	  if( KheMeetAsst(meet1)!=NULL && KheMeetDuration(meet1)==split1_durn )
	  {
	    for( k = j + 1;  k < KheEventMeetCount(ao->soln, e);  k++ )
	    {
	      meet2 = KheEventMeet(ao->soln, e, k);
	      if( KheMeetAsst(meet2) != NULL &&
		  KheMeetDuration(meet2) == split2_durn )
	      {
		if( KheMergeMoveRepair(ej, meet1, meet2, false) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej), "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet1, meet2, true) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej), "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet2, meet1, false) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej), "");
		  return true;
		}
		if( KheMergeMoveRepair(ej, meet2, meet1, true) )
		{
		  if( DEBUG4 )
		    fprintf(stderr, "%*s] KheSplitEventsAugment ret true\n",
		      KheEjectorCurrDebugIndent(ej), "");
		  return true;
		}
	      }
	    }
	  }
	}
      }
      if( KheEjectorCurrMayRevisit(ej) )
	KheSolnEventMeetsUnVisit(ao->soln, e);
    }
  }
  if( DEBUG4 )
    fprintf(stderr, "%*s] KheSplitEventsAugment ret false\n",
      KheEjectorCurrDebugIndent(ej), "");
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

static void KheDebugAugmentBegin(KHE_EJECTOR ej, KHE_MONITOR d, char *str)
{
  if( KheEjectorCurrDebug(ej) )
  {
    fprintf(stderr, "%*s[ %s(", KheEjectorCurrDebugIndent(ej), "", str);
    KheMonitorDebug(d, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
}

static bool KheDebugAugmentEnd(KHE_EJECTOR ej, KHE_MONITOR d, char *str,
  bool res)
{
  if( KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s] %s ret %s\n", KheEjectorCurrDebugIndent(ej), "",
      str, res ? "true" : "false");
  return res;
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
  KHE_EVENT e;  KHE_MEET meet;  int i;
  bool not_ejecting;  KHE_ASSIGN_TIME_MONITOR atm;
  KheDebugAugmentBegin(ej, d, "KheAssignTimeAugment");
  HnAssert(KheMonitorTag(d) == KHE_ASSIGN_TIME_MONITOR_TAG,
    "KheAssignTimeAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    atm = (KHE_ASSIGN_TIME_MONITOR) d;
    e = KheAssignTimeMonitorEvent(atm);
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    for( i = 0;  i < KheEventMeetCount(ao->soln, e);  i++ )
    {
      meet = KheEventMeet(ao->soln, e, i);
      if( KheMeetAsstTime(meet) == NULL )
      {
	/* ***
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewG lobalVisit(ao->soln);
	*** */
	if( KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
	  (void *) KheMeetDomain(meet), KHE_OPTIONS_KEMPE_FALSE,
	  !not_ejecting, not_ejecting, false) )
	  return KheDebugAugmentEnd(ej, d, "KheAssignTimeAugment", true);
      }
    }
  }
  return KheDebugAugmentEnd(ej, d, "KheAssignTimeAugment", false);
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
  KHE_EVENT e;  KHE_MEET meet;  int i, durn, pos;
  KHE_PREFER_TIMES_CONSTRAINT ptc;  KHE_TIME_GROUP domain;  KHE_TIME t;
  bool not_ejecting;  KHE_OPTIONS_KEMPE kempe;  KHE_PREFER_TIMES_MONITOR ptm;
  KheDebugAugmentBegin(ej, d, "KhePreferTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_PREFER_TIMES_MONITOR_TAG,
    "KhePreferTimesAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    ptm = (KHE_PREFER_TIMES_MONITOR) d;
    e = KhePreferTimesMonitorEvent(ptm);
    ptc = KhePreferTimesMonitorConstraint(ptm);
    domain = KhePreferTimesConstraintDomain(ptc);
    durn = KhePreferTimesConstraintDuration(ptc);
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    /* kempe = KheEjectorUseKempeMoves(ej); */
    kempe = ao->kempe_moves;
    for( i = 0;  i < KheEventMeetCount(ao->soln, e);  i++ )
    {
      meet = KheEventMeet(ao->soln, e, i);
      t = KheMeetAsstTime(meet);
      if( (durn == KHE_ANY_DURATION || KheMeetDuration(meet) == durn) &&
	  t != NULL && !KheTimeGroupContains(domain, t, &pos) )
      {
	/* ***
	if( KheEjectorCurrLength(ej) == 1 )
	  KheSolnNewG lobalVisit(ao->soln);
	*** */
	if( KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
	    (void *) domain, kempe, !not_ejecting, not_ejecting,
	    WITH_PREFER_TIMES_NODE_SWAPS) )
	  return KheDebugAugmentEnd(ej, d, "KhePreferTimesAugment", true);
      }
    }
  }
  return KheDebugAugmentEnd(ej, d, "KhePreferTimesAugment", false);
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
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  KHE_MEET meet;
  KHE_SPREAD_EVENTS_MONITOR sem;  KHE_TIME time;  KHE_TIME_GROUP tg;
  int i, j, k, minimum, maximum, inc, index, random_offset, pos;
  bool not_ejecting;  KHE_OPTIONS_KEMPE kempe;
  KheDebugAugmentBegin(ej, d, "KheSpreadEventsAugment");
  HnAssert(KheMonitorTag(d) == KHE_SPREAD_EVENTS_MONITOR_TAG,
    "KheSpreadEventsAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
    not_ejecting = ao->no_ejecting_moves;
    /* kempe = KheEjectorUseKempeMoves(ej); */
    kempe = ao->kempe_moves;
    sem = (KHE_SPREAD_EVENTS_MONITOR) d;
    eg = KheSpreadEventsMonitorEventGroup(sem);
    random_offset = KheAugmentOptionsRandomOffset(ao, 19, 43);
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
	  for( k = 0;  k < KheEventMeetCount(ao->soln, e);  k++ )
	  {
	    index = (k + random_offset) % KheEventMeetCount(ao->soln, e);
	    meet = KheEventMeet(ao->soln, e, index);
	    time = KheMeetAsstTime(meet);
	    if( time != NULL && !KheTimeGroupContains(tg, time, &pos) &&
	        KheMeetMultiRepair(ej, ao, meet, true, &KheInDomainMeetMoveFn,
		  (void *) tg, kempe, !not_ejecting, not_ejecting,
		  WITH_SPREAD_EVENTS_NODE_SWAPS) )
	      return KheDebugAugmentEnd(ej, d, "KheSpreadEventsAugment (a)",
		true);
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
	  for( k = 0;  k < KheEventMeetCount(ao->soln, e);  k++ )
	  {
            index = (k + random_offset) % KheEventMeetCount(ao->soln, e);
	    meet = KheEventMeet(ao->soln, e, index);
	    time = KheMeetAsstTime(meet);
	    if( time != NULL && KheTimeGroupContains(tg, time, &pos) &&
	        KheMeetMultiRepair(ej, ao, meet, true,
		  &KheNotInDomainMeetMoveFn, (void *) tg, kempe,
		  !not_ejecting, not_ejecting, WITH_SPREAD_EVENTS_NODE_SWAPS) )
	      return KheDebugAugmentEnd(ej, d, "KheSpreadEventsAugment (b)",
		true);
	  }
	}
      }
    }
  }
  return KheDebugAugmentEnd(ej, d, "KheSpreadEventsAugment", false);
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
  KheDebugAugmentBegin(ej, d, "KheOrderEventsAugment");
  HnAssert(KheMonitorTag(d) == KHE_ORDER_EVENTS_MONITOR_TAG,
    "KheOrderEventsAugment internal error 1");
  /* if( KheEjectorRepairTimes(ej) ) */
  if( ao->repair_times )
  {
    /* still to do */
  }
  return KheDebugAugmentEnd(ej, d, "KheOrderEventsAugment", false);
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
/*  bool KheAssignResourceAugment(KHE_EJECTOR ej, KHE_MONITOR d)             */
/*                                                                           */
/*  Augment function for individual assign resource defects.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheAssignResourceAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_RESOURCE_GROUP domain;
  KHE_MTASK prev_mt;  KHE_RESOURCE prev_r;  KHE_EVENT_RESOURCE er;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheAssignResourceAugment");
  HnAssert(KheMonitorTag(d) == KHE_ASSIGN_RESOURCE_MONITOR_TAG,
    "KheAssignResourceAugment internal error 1");
  if( ao->repair_resources )
  {
    arm = (KHE_ASSIGN_RESOURCE_MONITOR) d;
    er = KheAssignResourceMonitorEventResource(arm);
    domain = KheEventResourceHardDomain(er);
    prev_mt = NULL;
    prev_r = NULL;
    KheResourceIteratorInit(&to_ri_rec, ao, domain, NULL, NULL, false);
    KheExcludeDaysInit(&ed_rec);
    if( KheEventResourceMultiRepair(ej, ao, er, &to_ri_rec, &ed_rec,
	  &prev_mt, &prev_r) )
      return KheDebugAugmentEnd(ej, d, "KheAssignResourceAugment", true);
  }
  return KheDebugAugmentEnd(ej, d, "KheAssignResourceAugment", false);
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
/*****************************************************************************/

static bool KhePreferResourcesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_PREFER_RESOURCES_CONSTRAINT prc;  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_EVENT_RESOURCE er;  KHE_MTASK prev_mt;  KHE_RESOURCE prev_r;
  KHE_RESOURCE_GROUP domain;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KhePreferResourcesAugment");
  HnAssert(KheMonitorTag(d) == KHE_PREFER_RESOURCES_MONITOR_TAG,
    "KhePreferResourcesAugment internal error 1");
  if( ao->repair_resources )
  {
    prm = (KHE_PREFER_RESOURCES_MONITOR) d;
    er = KhePreferResourcesMonitorEventResource(prm);
    prc = KhePreferResourcesMonitorConstraint(prm);
    domain = KhePreferResourcesConstraintDomain(prc);
    prev_mt = NULL;
    prev_r = NULL;
    KheResourceIteratorInit(&to_ri_rec, ao, domain, NULL, NULL, true);
    KheExcludeDaysInit(&ed_rec);
    if( KheEventResourceMultiRepair(ej, ao, er, &to_ri_rec, &ed_rec,
	  &prev_mt, &prev_r) )
      return KheDebugAugmentEnd(ej, d, "KhePreferResourcesAugment", true);
  }
  return KheDebugAugmentEnd(ej, d, "KhePreferResourcesAugment", false);
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
  return KheEjector RepairEnd(ej, KHE_REPA IR_TASK_SET_MOVE, success);
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
  KheDebugAugmentBegin(ej, d, "KheAvoidSplitAssignmentsAugment");
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
	      return KheDebugAugmentEnd(ej, d,
		"KheAvoidSplitAssignmentsAugment", true);
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
	      return KheDebugAugmentEnd(ej, d,
		"KheAvoidSplitAssignmentsAugment", true);
	  }
	}
      }

      if( KheEjectorCurrMayRevisit(ej) )
	KheAssignedTasksUnVisit(asam);
    }
  }
  return KheDebugAugmentEnd(ej, d, "KheAvoidSplitAssignmentsAugment", false);
}
#else
static bool KheAvoidSplitAssignmentsAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;  int i;  KHE_RESOURCE r;
  /* bool save_ejector_repair_times; */
  KheDebugAugmentBegin(ej, d, "KheAvoidSplitAssignmentsAugment");
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
	      KheEjectorCurrDebugIndent(ej), "",
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
  return KheDebugAugmentEnd(ej, d, "KheAvoidSplitAssignmentsAugment", false);
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
/*****************************************************************************/

static bool KheLimitResourcesAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT c;  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_MTASK prev_mt;  KHE_RESOURCE prev_r;
  int eg_index, i, count, maximum, minimum, active_durn;
  KHE_RESOURCE_GROUP domain, rg;  KHE_EVENT_RESOURCE er;
  struct khe_resource_iterator_rec to_ri_rec;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheLimitResourcesAugment");
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_RESOURCES_MONITOR_TAG,
    "KheLimitResourcesAugment internal error 1");
  KheExcludeDaysInit(&ed_rec);
  if( ao->repair_resources )
  {
    lrm = (KHE_LIMIT_RESOURCES_MONITOR) d;
    c = KheLimitResourcesMonitorConstraint(lrm);
    eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
    count = KheLimitResourcesConstraintEventResourceCount(c, eg_index);
    domain = KheLimitResourcesConstraintDomain(c);
    KheLimitResourcesMonitorActiveDuration(lrm, &minimum,&maximum,&active_durn);
    if( active_durn < minimum )
    {
      /* too few resources from c's domain */
      prev_mt = NULL;
      prev_r = NULL;
      KheResourceIteratorInit(&to_ri_rec, ao, domain, NULL, NULL, false);
      for( i = 0;  i < count;  i++ )
      {
	er = KheLimitResourcesConstraintEventResource(c, eg_index, i);
        if( KheEventResourceMultiRepair(ej, ao, er, &to_ri_rec, &ed_rec,
	      &prev_mt, &prev_r) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitResourcesAugment", true);
      }
    }
    else if( active_durn > maximum )
    {
      /* too many resources from c's domain */
      prev_mt = NULL;
      prev_r = NULL;
      for( i = 0;  i < count;  i++ )
      {
	er = KheLimitResourcesConstraintEventResource(c, eg_index, i);
	rg = KheEventResourceHardDomain(er);
	KheResourceIteratorInit(&to_ri_rec, ao, rg, domain, NULL, true);
        if( KheEventResourceMultiRepair(ej, ao, er, &to_ri_rec, &ed_rec,
	      &prev_mt, &prev_r) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitResourcesAugment", true);
      }
    }
    else
      HnAbort("KheLimitResourcesAugment internal error 2");
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitResourcesAugment", false);
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
/*  bool KheTaskUnAssignRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,       */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Carry out one repair, which is the unassignment of task.                 */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskUnAssignRepair(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_TASK task)
{
  bool success;  KHE_RESOURCE r;
  if( ao->repair_resources && !KheTaskIsPreassigned(task, &r) )
  {
    KheEjectorRepairBegin(ej);
    success = KheTaskUnAssign(task);
    if( KheEjectorCurrDebug(ej) )
      fprintf(stderr, "%cTaskUnAssign(%s)", success ? '+' : '-',
	KheTaskId(task));
    return KheEjectorRepairEnd(ej, KHE_REPAIR_TASK_UNASSIGN, success);
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidClashesAugment(KHE_EJECTOR ej, KHE_MONITOR d)               */
/*                                                                           */
/*  Augment function for individual avoid clashes defects.                   */
/*                                                                           */
/*  For each clashing task, try moving it to any other resource in its       */
/*  domain.  This function expects to be called only during resource repair, */
/*  because during time repair avoid clashes monitors are usually detached.  */
/*                                                                           */
/*  Implementation note.  We don't use KheDecreaseLoadMultiRepair, because   */
/*  it assumes that there is at most one task per day, and there is no easy  */
/*  way around that.  So instead we just try unassigning each clashing task  */
/*  in turn.  We leave it to the next step in the chain to find an           */
/*  alternative assignment.                                                  */
/*                                                                           */
/*  Obsolete:                                                                */
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
  KHE_AVOID_CLASHES_MONITOR acm;  KHE_RESOURCE r;  KHE_TASK task;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j;  KHE_TIME t;

  /* boilerplate */
  HnAssert(KheMonitorTag(d) == KHE_AVOID_CLASHES_MONITOR_TAG,
    "KheAvoidClashesAugment internal error 1");
  acm = (KHE_AVOID_CLASHES_MONITOR) d;
  r = KheAvoidClashesMonitorResource(acm);
  rtm = KheResourceTimetableMonitor(ao->soln, r);

  /* find each clashing task and unassign it */
  for( i = 0;  i < KheResourceTimetableMonitorClashingTimeCount(rtm);  i++ )
  {
    t = KheResourceTimetableMonitorClashingTime(rtm, i);
    for( j = 0;  j < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  j++ )
    {
      task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
      task = KheTaskProperRoot(task);
      if( KheTaskUnAssignRepair(ej, ao, task) )
        return KheDebugAugmentEnd(ej, d, "KheAvoidClashesAugment", true);
    }
  }

  /* wrapup */
  return KheDebugAugmentEnd(ej, d, "KheAvoidClashesAugment", false);
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
/*  bool KheReassignWholeTimetableMultiRepair(KHE_EJECTOR ej,                */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r)                                */
/*                                                                           */
/*  Optimal reassignment of two resources, but only at depth 1.              */
/*                                                                           */
/*****************************************************************************/

static bool KheReassignWholeTimetableMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r)
{
  /* *** removed; I'm getting better results without it
  struct khe_resource_iterator_rec ri_rec;  KHE_RESOURCE_TYPE rt;
  KHE_RESOURCE r2;  KHE_DYNAMIC_RESOURCE_SOLVER drs;
  if( KheEjectorCurrLength(ej) == 1 )
  {
    if( DEBUG40 )
      fprintf(stderr, "[ KheReassignWholeTimetableMultiRepair(ej, ao, %s)\n",
	KheResourceId(r));

    ** make a dynamic solver **
    rt = KheResourceResourceType(r);
    drs = KheDynamicResourceSolverMake(ao->soln, rt, ao->options);

    ** load and solve for r and one other resource **
    KheResourceIteratorInit(&ri_rec, ao, KheResourceTypeFullResourceGroup(rt),
      NULL, r, false);
    KheForEachResource(&ri_rec, r2)
    {
      KheDynamicResourceSolverAddDayRange(drs,
	0, KheFrameTimeGroupCount(ao->frame) - 1);
      KheDynamicResourceSolverAddResource(drs, r);
      KheDynamicResourceSolverAddResource(drs, r2);
      if( KheDynamicResourceSolverSolve(drs, false, false,
	  false, false, true, 10000, 0, 0, 0, KHE_DRS_DOM_INDEXED_TABULATED,
	  false, KHE_DRS_DOM_INDEXED_TABULATED) )
      {
	if( DEBUG40 )
	  fprintf(stderr, "] KheReassignWholeTimetableMultiRepair ret. true\n");
	KheDynamicResourceSolverDelete(drs);
	return true;
      }
    }
    KheDynamicResourceSolverDelete(drs);
    if( DEBUG40 )
      fprintf(stderr, "] KheReassignWholeTimetableMultiRepair ret. false\n");
  }
  *** */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSwapWholeTimetableMultiRepair(KHE_EJECTOR ej,                    */
/*    KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r)                                */
/*                                                                           */
/*  Try swapping the entire timetable of r with another resource, but        */
/*  only at depth 1.                                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheSwapWholeTimetableMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_RESOURCE r, KHE_TIME_GROUP blocking_tg)
{
  struct khe_resource_iterator_rec ri_rec;
  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r2;  KHE_INTERVAL in;
  if( KheEjectorCurrLength(ej) == 1 )
  {
    in = KheIntervalMake(0, KheFrameTimeGroupCount(ao->frame) - 1);
    rt = KheResourceResourceType(r);
    KheResourceIteratorInit(&ri_rec, ao, KheResourceTypeFullResourceGroup(rt),
      NULL, r, false);
    KheForEachResource(&ri_rec, r2)
      if( KheSwapRepair(ej, ao, in, false, r, r2, NULL, blocking_tg) )
	return true;
  }
  return false;
}


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
  KHE_AVOID_UNAVAILABLE_TIMES_MONITOR autm;  KHE_TIME_GROUP tg;
  KHE_RESOURCE r; KHE_AVOID_UNAVAILABLE_TIMES_CONSTRAINT autc;  bool res;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheAvoidUnavailableTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG,
    "KheAvoidUnavailableTimesAugment internal error 1");
  autm = (KHE_AVOID_UNAVAILABLE_TIMES_MONITOR) d;
  autc = KheAvoidUnavailableTimesMonitorConstraint(autm);
  tg = KheAvoidUnavailableTimesConstraintUnavailableTimes(autc);
  r = KheAvoidUnavailableTimesMonitorResource(autm);
  KheExcludeDaysInit(&ed_rec);
  res = KheDecreaseLoadMultiRepair(ej, ao, r, tg, false, &ed_rec) ||
    KheSwapWholeTimetableMultiRepair(ej, ao, r, tg) ||
    KheReassignWholeTimetableMultiRepair(ej, ao, r);
  return KheDebugAugmentEnd(ej, d, "KheAvoidUnavailableTimesAugment", res);
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
  a = KheSolnArenaBegin(soln);
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

  KheDebugAugmentBegin(ej, d, "KheLimitIdleTimesAugment");
  if( KheEjectorRepairTimes(ej) )
  {
    /* simple repairs */
    soln = KheEjectorSoln(ej);
    litm = (KHE_LIMIT_IDLE_TIMES_MONITOR) d;
    /* ejecting = KheEjectorEjectingNotBasic(ej); */
    not_ejecting = ao->no_ejecting_moves;
    kempe = KheEjectorUseKempeMoves(ej);
    random_offset = KheAugmentOptionsRandomOffset(ao, 3, 29);
    r = KheLimitIdleTimesMonitorResource(litm);
    for( i = 0;  i < KheResourceAssignedTaskCount(soln, r);  i++ )
    {
      index = (random_offset + i) % KheResourceAssignedTaskCount(soln, r);
      task = KheResourceAssignedTask(soln, r, index);
      meet = KheMeetFirstMovable(KheTaskMeet(task), &junk);
      if( meet != NULL && KheMeetMultiRepair(ej, meet, true,
	    &KheLimitIdleMeetMoveFn, (void *) litm, kempe, !not_ejecting,
	    not_ejecting, false) )
	return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment", true);
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
	    return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment (c)",
	      true);
	}
      }
    }
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment", false);
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

  KheDebugAugmentBegin(ej, d, "KheLimitIdleTimesAugment");
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
    random_offset = KheAugmentOptionsRandomOffset(ao, 7, 59);
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
		return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment",
		  true);
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
	    return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment (d)",
	      true);
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
	    return KheDebugAugmentEnd(ej, d,
	      "KheLimitIdleTimesAugment (c)", true);
	}
      }
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitIdleTimesAugment", false);
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

/* *** replaced by KheIncreaseLoadMultiRepair
static bool KheClusterUnderloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  int active_count, bool allow_zero, KHE_EXCLUDE_DAYS ed)
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
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_NEGATIVE,
	    true, false, ed) )
      {
	if( DEBUG7 )
	  fprintf(stderr, "%*s] KheClusterUnderloadMultiRepair ret true\n",
	    KheEjectorCurrDebugIndent(ej), "");
	return true;
      }
    }
    else if( allow_zero && active_count == 1 )
    {
      ** try to reduce the number of active time groups to zero **
      if( DEBUG7 )
      {
	fprintf(stderr, "  trying allow_zero time group ");
	KheTimeGroupDebug(tg, 1, 0, stderr);
      }
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE,
	    true, false, ed) )
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheClusterMeetsUnassignTimeGroup(                         */
/*    KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm, KHE_TIME_GROUP tg)                */
/*                                                                           */
/*  Return a time group that is the cycle minus every time group of cbtm     */
/*  that has no meets, and minus tg as well.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterOverloadMultiRepair(KHE_EJECTOR ej,                       */
/*    KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)           */
/*                                                                           */
/*  Repair an overload defect in cbtm, by making an active time group        */
/*  inactive.                                                                */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheDecreaseLoadMultiRepair
static bool KheClusterOverloadMultiRepair(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  KHE_EXCLUDE_DAYS ed)
{
  int i, busy, ix, tg_count, count, random_offset;
  KHE_TIME_GROUP tg, include_tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_POLARITY po;  KHE_RESOURCE r;  KHE_SOLN soln;
  if( DEBUG28 && KheEjectorCurrDebug(ej) )
    fprintf(stderr, "%*s[ KheClusterOverloadMultiRepair(times %s, depth %d)\n",
      KheEjectorCurrDebugIndent(ej), "",
      ao->repair_times ? "true" : "false", KheEjectorCurrLength(ej));
  r = KheClusterBusyTimesMonitorResource(cbtm);

  ** underload/overload repair similar to repairing underloads **
  random_offset = 83 * KheEjectorCurrAugmentCount(ej) +
    7 * KheSolnDiversifier(KheEjectorSoln(ej));
  count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po, &busy) )
    {
      ** this time group is active **
      if( DEBUG28 && KheEjectorCurrDebug(ej) )
	fprintf(stderr, "%*s  KheClusterOverloadMultiRepair active %s\n",
	  KheEjectorCurrDebugIndent(ej), "", KheTimeGroupId(tg));
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE,
	    true, false, ed) )
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

  ** if meet moves are allowed, try a meet bound repair as last resort **
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
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceHasNeedlessAssignments(KHE_RESOURCE r,                   */
/*    KHE_TIME_GROUP tg, KHE_AUGMENT_OPTIONS ao)                             */
/*                                                                           */
/*  Return true if r has one or more needless assignments during tg (that    */
/*  is, assignments that no event resource monitor needs), and no others.    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheResourceHasNeedlessAssignments(KHE_RESOURCE r,
  KHE_TIME_GROUP tg, KHE_AUGMENT_OPTIONS ao)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j, count;  KHE_TIME t;
  KHE_TASK task;  KHE_MTASK mtask;
  rtm = KheResourceTimetableMonitor(ao->soln, r);
  count = 0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    for( j = 0;  j < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  j++ )
    {
      task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
      mtask = KheMTaskFinderTaskToMTask(ao->mtask_finder, task);
      if( !KheMTaskContainsNeedlessAssignment(mtask) )
	return false;
      count++;
    }
  }
  return count > 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterTimeGroupHasPriority(KHE_RESOURCE r,                      */
/*    KHE_TIME_GROUP tg, KHE_AUGMENT_OPTIONS ao, KHE_POLARITY po)            */
/*                                                                           */
/*  Return true if cluster time group tg should be given priority,           */
/*  because we are trying to remove an overload and tg contains a            */
/*  needless assignment.                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheClusterTimeGroupHasPriority(KHE_RESOURCE r,
  KHE_TIME_GROUP tg, KHE_AUGMENT_OPTIONS ao, KHE_POLARITY po)
{
  return po == KHE_POSITIVE && KheResourceHasNeedlessAssignments(r, tg, ao);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_MONITOR d)           */
/*                                                                           */
/*  Augment function for individual cluster busy times defects.              */
/*                                                                           */
/*****************************************************************************/

static bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  int active_count, open_count, minimum, maximum, tg_count, i, ix, b;
  int random_offset;
  bool allow_zero;  KHE_RESOURCE r;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheClusterBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    "KheClusterBusyTimesAugment internal error 1");
  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) d;
  r = KheClusterBusyTimesMonitorResource(cbtm);
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &active_count,
    &open_count, &minimum, &maximum, &allow_zero);
  tg_count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  KheExcludeDaysInit(&ed_rec);
  random_offset = KheAugmentOptionsRandomOffset(ao, 13, 19);
  if( active_count > maximum )
  {
    /* defect is an overload - visit each active time group */
    for( i = 0;  i < tg_count;  i++ )
    {
      ix = (random_offset + i) % tg_count;
      if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po, &b) )
      {
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	    po == KHE_POSITIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", true);
      }
    }
  }
  else if( active_count < minimum )
  {
    /* defect is an underload - visit each inactive time group */
    for( i = 0;  i < tg_count;  i++ )
    {
      ix = (random_offset + i) % tg_count;
      if( !KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg, &po, &b) )
      {
        if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	    po == KHE_NEGATIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", true);
      }
      else if( allow_zero && active_count == 1 )
      {
        if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	    po == KHE_POSITIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", true);
      }
    }
  }
  else
    HnAbort("KheClusterBusyTimesAugment internal error 3");
  return KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", false);
}


/* not so old version
static bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_INTERVAL swap_exclude_first_days, move_exclude_first_days, in;
  int active_count, open_count, minimum, maximum, tg_count, i, ix;
  int random_offset1, random_offset2, busy;
  bool allow_zero;  KHE_RESOURCE r, other_r;  KHE_MTASK mt;
  KHE_TIME_GROUP tg;  KHE_POLARITY po;  KHE_RESOURCE_GROUP domain;
  struct khe_interval_iterator_rec move_ii_rec, swap_ii_rec;
  struct khe_resource_iterator_rec other_ri_rec;
  struct khe_exclude_days_rec ed_rec;
  struct khe_all_mtask_iterator_rec ami_rec;
  KheDebugAugmentBegin(ej, d, "KheClusterBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    "KheClusterBusyTimesAugment internal error 1");
  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) d;
  r = KheClusterBusyTimesMonitorResource(cbtm);
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &active_count,
    &open_count, &minimum, &maximum, &allow_zero);
  tg_count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  KheExcludeDaysInit(&ed_rec);
  if( active_count > maximum )
  {
    ** defect is an overload **
    swap_exclude_first_days = KheIntervalMake(1, 0);
    move_exclude_first_days = KheIntervalMake(1, 0);
    random_offset1 = 13 * KheEjectorCurrAugmentCount(ej) +
      19 * KheSolnDiversifier(KheEjectorSoln(ej));
    for( i = 0;  i < tg_count;  i++ )
    {
      ix = (random_offset1 + i) % tg_count;
      if( KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, ix, &tg,&po,&busy)
	  && KheGetIntervalAndDomain(ej, ao, r, tg, &in, &domain) )
      {
	** try swaps **
	KheIntervalIteratorInit(&swap_ii_rec, ao, in, ao->swap_widening_max,
	  false, ao->full_widening_on && KheEjectorCurrLength(ej) == 1,
	  &swap_exclude_first_days);
	KheResourceIteratorInit(&other_ri_rec, ao, domain, NULL, r, false);
	KheForEachInterval(&swap_ii_rec, in)
	{
	  random_offset2 = 11 * KheEjectorCurrAugmentCount(ej) +
	    53 * KheSolnDiversifier(ao->soln);
	  KheForEachResource(&other_ri_rec, random_offset2, other_r)
	    if( KheSwapRe pair(ej, ao, in, r, other_r, NULL, tg) )
	    {
	      KheDebugAugmentEnd(ej, d,
		"KheClusterBusyTimesAugment swap", true);
	      return true;
	    }
	}

	** try moves to or from NULL **
	if( po == KHE_POSITIVE )
	{
	  ** active positive time group, try unassignment **
	  if( KheMoveRepair(ej, ao, in, r, NULL, NULL) )
	  {
	    KheDebugAugmentEnd(ej, d,
	      "KheClusterBusyTimesAugment unassign", true);
	    return true;
	  }
	}
	else
	{
	  ** active negative time group, try assignments **
	  HnAssert(KheIntervalEmpty(in),
	    "KheClusterBusyTimesAugment internal error 2");
          KheAllMTaskIteratorTimeGroupInit(&ami_rec, ao,
	    KheResourceResourceType(r), tg);
	  random_offset2 = 11 * KheEjectorCurrAugmentCount(ej) +
	    53 * KheSolnDiversifier(ao->soln);
	  KheAllMTaskForEach(&ami_rec, mt)
	    if( KheMoveRepair(ej, ao, in, NULL, r, mt) )
	    {
	      KheDebugAugmentEnd(ej, d,
		"KheClusterBusyTimesAugment assign", true);
	      return true;
	    }
	}
      }
    }
    KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", false);
    return false;  ** keep compiler happy **
  }
  else if( active_count < minimum )
  {
    ** defect is an underload **
    HnAbort("KheClusterBusyTimesAugment underload case st ill to do");
    return false;  ** keep compiler happy **
  }
  else
  {
    HnAbort("KheClusterBusyTimesAugment internal error 3");
    KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", false);
    return false;  ** keep compiler happy **
  }
}
*** */


/* *** old version
static bool KheClusterBusyTimesAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  int count, ind_count, minimum, maximum;  bool allow_zero, success;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheClusterBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG,
    "KheClusterBusyTimesAugment internal error 1");
  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) d;
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &count,
    &ind_count, &minimum, &maximum, &allow_zero);
  success = false;
  KheExcludeDaysInit(&ed_rec);
  if( count > maximum )
    success = KheClusterOverloadMultiRepair(ej, ao, cbtm, &ed_rec);
  else if( count < minimum )
    success = KheClusterUnderloadMultiRepair(ej, ao, cbtm, count, allow_zero,
      &ed_rec);
  else
    HnAbort("KheClusterBusyTimesAugment internal error 2");
  KheDebugAugmentEnd(ej, d, "KheClusterBusyTimesAugment", success);
  return success;
}
*** */


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
  bool allow_zero;  struct khe_exclude_days_rec ed_rec;
  int i, ix, count, random_offset, busy_count, open_count, minimum, maximum;
  KheDebugAugmentBegin(ej, d, "KheLimitBusyTimesAugment");
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG,
    "KheLimitBusyTimesAugment internal error 1");
  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) d;
  r = KheLimitBusyTimesMonitorResource(lbtm);
  count = KheLimitBusyTimesMonitorDefectiveTimeGroupCount(lbtm);
  random_offset = KheAugmentOptionsRandomOffset(ao, 47, 53);
  KheExcludeDaysInit(&ed_rec);
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    KheLimitBusyTimesMonitorDefectiveTimeGroup(lbtm, ix, &tg, &busy_count,
      &open_count, &minimum, &maximum, &allow_zero);
    /* ***
    if( KheOverUnderMultiRepair(ej, ao, r, tg, busy_count > maximum, false,
	  allow_zero, &ed_rec) )
    *** */
    if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	  busy_count > maximum, false, allow_zero, &ed_rec) )
      return KheDebugAugmentEnd(ej, d, "KheLimitBusyTimesAugment", true);
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitBusyTimesAugment", false);
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
  int i, ix, count, open_count, random_offset, minimum, maximum;
  KHE_TIME_GROUP tg;  float workload;  bool allow_zero;
  struct khe_exclude_days_rec ed_rec;
  KheDebugAugmentBegin(ej, d, "KheLimitWorkloadAugment");
  lwm = (KHE_LIMIT_WORKLOAD_MONITOR) d;
  r = KheLimitWorkloadMonitorResource(lwm);
  tg = KheInstanceFullTimeGroup(KheResourceInstance(r));
  count = KheLimitWorkloadMonitorDefectiveTimeGroupCount(lwm);
  random_offset = KheAugmentOptionsRandomOffset(ao, 23, 73);
  KheExcludeDaysInit(&ed_rec);
  for( i = 0;  i < count;  i++ )
  {
    ix = (random_offset + i) % count;
    KheLimitWorkloadMonitorDefectiveTimeGroup(lwm, ix, &tg, &workload,
      &open_count, &minimum, &maximum, &allow_zero);
    if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	  workload > maximum, false, false, &ed_rec) )
      return KheDebugAugmentEnd(ej, d, "KheLimitWorkloadAugment (overload)",
	true);
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitWorkloadAugment", false);
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
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  KHE_POLARITY po, po1, po2;
  KHE_TIME_GROUP tg, tg1, tg2;  int busy;  KHE_RESOURCE r;
  int /* min_lim, full_len, */ tries, j;
  int i, history, first_index, last_index, history_after, random_offset, count;
  bool too_long;  struct khe_exclude_days_rec ed_rec;
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
    "KheLimitActiveIntervalsAugment internal error 1");
  KheDebugAugmentBegin(ej, d, "KheLimitActiveIntervalsAugment");
  laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) d;
  random_offset = KheAugmentOptionsRandomOffset(ao, 89, 29);
  count = KheLimitActiveIntervalsMonitorDefectiveIntervalCount(laim);
  r = KheLimitActiveIntervalsMonitorResource(laim);
  KheExcludeDaysInit(&ed_rec);
  for( i = 0;  i < count;  i++ )
  {
    KheLimitActiveIntervalsMonitorDefectiveInterval(laim,
      (random_offset + i) % count, &history, &first_index,
      &last_index, &history_after, &too_long);
    /* *** this horror removed 26/4/24 by JeffK; corresponding fixes below
    if( last_index < 0 )
      continue;
    *** */
    if( too_long )  /* defective interval is too long */
    {
      /* try to make the first time group inactive, if no history before */
      tries = 0;
      if( history == 0 && first_index <= last_index )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg, &po, &busy);
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	      po == KHE_POSITIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	tries++;
      }

      /* try to make the last time group inactive, if no history after */
      /* and this repair is different from the previous one */
      if( history_after == 0 && first_index < last_index )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index,
	  &tg, &po, &busy);
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	      po == KHE_POSITIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
	tries++;
      }

      /* try the time groups in between, but only if nothing else tried */
      if( tries == 0 )
	for( j = first_index + 1;  j < last_index;  j++ )
	{
	  KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, j,
	    &tg, &po, &busy);
	  if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
		po == KHE_POSITIVE, true, false, &ed_rec) )
	    return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	      true);
	}

    }
    else /* defective interval is too short */
    {
      /* get the minimum limit and the full length of the defective interval */
      /* ***
      min_lim = KheLimitActiveIntervalsMonitorMinimum(laim);
      full_len = history + (last_index - first_index + 1) + history_after;
      HnAssert(full_len < min_lim,
	"KheLimitActiveIntervalsAugment internal error 3");
      *** */

      /* try to make the interval longer, by extending at the left end */
      if( first_index > 0 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index - 1,
	  &tg, &po, &busy);
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	      po == KHE_NEGATIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
      }

      /* try to make the interval longer, by extending at the right end */
      if( last_index < KheLimitActiveIntervalsMonitorTimeGroupCount(laim) - 1 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index + 1,
	  &tg, &po, &busy);
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	      po == KHE_NEGATIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
      }

      /* try clearing out the whole interval, if any of it is non-history */
      /* NB we are relying on expansion to extend beyond first_index */
      if( first_index <= last_index )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg1, &po1, &busy);
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index,
	  &tg2, &po2, &busy);
	if( po1 == KHE_POSITIVE && po2 == KHE_POSITIVE &&
	    KheClearIntervalMultiRepair(ej, ao, r, tg1, tg2, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
      }

      /* *** old clunky version
      ** if full_len <= 2, try to remove the interval altogether **
      ** NB repair will be enhanced if sequence length is 2 **
      if( 1 <= full_len && full_len <= 2 && history == 0 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg, &po, &busy);
	if( KheIncreaseOrDecreaseLoadMultiRepair(ej, ao, r, tg,
	      po == KHE_POSITIVE, true, false, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
      }
      *** */

      /* try clearing out whole interval, if no history and seems positive */
      /* NB it is fine if first_index == last_index here */
      /* ***
      if( history == 0 && history_after == 0 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg1, &po1, &busy);
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index,
	  &tg2, &po2, &busy);
	if( po1 == KHE_POSITIVE && po2 == KHE_POSITIVE &&
	    KheClearIntervalMultiRepair(ej, ao, r, tg1, tg2, &ed_rec) )
	  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment",
	    true);
      }
      *** */
    }
  }
  return KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", false);
}


/* *** old version
static bool KheLimitActiveIntervalsAugment(KHE_EJECTOR ej,
  KHE_AUGMENT_OPTIONS ao, KHE_MONITOR d)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  KHE_POLARITY po;  KHE_TIME_GROUP tg;  int busy;  KHE_RESOURCE r;
  int min_lim, len, j;
  int i, history, first_index, last_index, history_after, random_offset, count;
  bool too_long;  struct khe_exclude_days_rec ed_rec;
  HnAssert(KheMonitorTag(d) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG,
    "KheLimitActiveIntervalsAugment internal error 1");
  KheDebugAugmentBegin(ej, d, "KheLimitActiveIntervalsAugment");
  laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) d;
  random_offset = KheAugmentOptionsRandomOffset(ao, 89, 29);
  count = KheLimitActiveIntervalsMonitorDefectiveIntervalCount(laim);
  r = KheLimitActiveIntervalsMonitorResource(laim);
  KheExcludeDaysInit(&ed_rec);
  for( i = 0;  i < count;  i++ )
  {
    KheLimitActiveIntervalsMonitorDefectiveInterval(laim,
      (random_offset + i) % count, &history, &first_index,
      &last_index, &history_after, &too_long);
    if( last_index < 0 )
      continue;
    if( too_long )
    {
      ** try to make the first time group inactive **
      KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	&tg, &po, &busy);
      if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE,
	    true, false, &ed_rec) )
      {
	KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	return true;
      }

      ** try to make the last time group inactive, if it's not the first **
      if( last_index != first_index )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_POSITIVE,
	      true, false, &ed_rec) )
	{
	  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	  return true;
	}
      }

      ** as a last resort, try the time groups in between **
      for( j = first_index + 1;  j < last_index;  j++ )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, j,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg,po==KHE_POSITIVE,
	      true, false, &ed_rec) )
	{
	  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	  return true;
	}
      }
    }
    else
    {
      ** if len <= 2, try to remove the interval altogether **
      ** NB repair will be enhanced if sequence length is 2 **
      min_lim = KheLimitActiveIntervalsMonitorMinimum(laim);
      len = history + (last_index - first_index + 1) + history_after;
      HnAssert(min_lim>len,"KheLimitActiveIntervalsAugment internal error 3");
      if( 1 <= len && len <= 2 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_POSITIVE,
	      true, false, &ed_rec) )
	{
	  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	  return true;
	}
      }

      ** try to make the interval longer, by extending at the left end **
      if( first_index > 0 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, first_index - 1,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_NEGATIVE,
	      true, false, &ed_rec) )
	{
	  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	  return true;
	}
      }

      ** try to make the interval longer, by extending at the right end **
      if( last_index < KheLimitActiveIntervalsMonitorTimeGroupCount(laim) - 1 )
      {
	KheLimitActiveIntervalsMonitorTimeGroupIsActive(laim, last_index + 1,
	  &tg, &po, &busy);
	if( KheOverUnderMultiRepair(ej, ao, r, tg, po==KHE_NEGATIVE,
	      true, false, &ed_rec) )
	{
	  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", true);
	  return true;
	}
      }
    }
    if( RANDOMIZED )
      return false;
  }
  KheDebugAugmentEnd(ej, d, "KheLimitActiveIntervalsAugment", false);
  return false;
}
*** */


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
/*  each of the clashing meets away.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheDemandAugment(KHE_EJECTOR ej, KHE_AUGMENT_OPTIONS ao,
  KHE_MONITOR d)
{
  KHE_TIME_GROUP tg;  KHE_MONITOR m;  KHE_MEET meet;  bool not_ejecting;
  KHE_OPTIONS_KEMPE kempe;  int i;  KHE_RESOURCE r;
  ARRAY_KHE_MEET meets;  KHE_ORDINARY_DEMAND_MONITOR odm;
  struct khe_exclude_days_rec ed_rec;

  /* boilerplate */
  KheDebugAugmentBegin(ej, d, "KheDemandAugment");
  HnAssert(KheMonitorTag(d) == KHE_ORDINARY_DEMAND_MONITOR_TAG ||
          KheMonitorTag(d) == KHE_WORKLOAD_DEMAND_MONITOR_TAG,
    "KheDemandAugment internal error 1");
  /* not_ejecting = KheEjectorBasicNotEjecting(ej); */
  not_ejecting = ao->no_ejecting_moves;
  /* kempe = KheEjectorUseKempeMoves(ej); */
  kempe = ao->kempe_moves;

  if( KheDemandDefectIsWorkloadOverload(d, &r, &tg) )
  {
    /* defect is workload overload of r in tg, so handle appropriately */
    KheExcludeDaysInit(&ed_rec);
    if( KheDecreaseLoadMultiRepair(ej, ao, r, tg, false, &ed_rec) )
    {
      if( DEBUG6 )
	fprintf(stderr, "%*s] KheDemandAugment ret true: %.5f\n",
	  KheEjectorCurrDebugIndent(ej), "",
	  KheCostShow(KheSolnCost(ao->soln)));
      return true;
    }
  }
  /* else if( KheEjectorRepairTimes(ej) ) */
  else if( ao->repair_times )
  {
    /* defect is ordinary overload; get meets */
    /* KheDemandDefectIsWorkloadOverload calls KheSolnMatchingSetCompetitors */
    HaArrayInit(meets, KheEjectorArena(ej));
    for( i = 0;  i < KheSolnMatchingCompetitorCount(ao->soln);  i++ )
    {
      m = KheSolnMatchingCompetitor(ao->soln, i);
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
	HaArrayFree(meets);
	return KheDebugAugmentEnd(ej, d, "KheDemandAugment", true);
      }
    }
    HaArrayFree(meets);
  }

  /* no luck */
  return KheDebugAugmentEnd(ej, d, "KheDemandAugment", false);
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
  KHE_EJECTOR res;

  /* ejector and schedules */
  res = KheEjectorMakeBegin(KheOptionsGet(options, "es_schedules", "1+;u-"), a);

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

  /* repair types - repairs applied to meets only */
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
  KheEjectorAddRepairType(res, KHE_REPAIR_NODE_MEET_SWAP,
    "Node meet swap");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_SPLIT,
    "Meet split");
  KheEjectorAddRepairType(res, KHE_REPAIR_SPLIT_MOVE,
    "Split move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MERGE_MOVE,
    "Merge move");
  KheEjectorAddRepairType(res, KHE_REPAIR_MEET_BOUND,
    "Meet bound");
  KheEjectorAddRepairType(res, KHE_REPAIR_COMPLEX_IDLE_MOVE,
    "Complex idle move");

  /* repair types - repairs applied to tasks only */
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_UNASSIGN,
    "Task unassign");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_SWAP,
    "Task swap");
  KheEjectorAddRepairType(res, KHE_REPAIR_TASK_EJECTING_REASSIGN,
    "Task ejecting reassign");
  KheEjectorAddRepairType(res, KHE_REPAIR_SPLIT_TASKS_UNASSIGN,
    "Split tasks unassign and reduce");

  /* *** repair types - currently unused
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
  KheEjectorAddRepairType(res,
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_ASSIGN,
    "Kempe meet move + ejecting task assignment");
  KheEjectorAddRepairType(res,
    KHE_REPAIR_KEMPE_MEET_MOVE_AND_EJECTING_TASK_MOVE,
    "Kempe meet move + ejecting task move");
  *** */

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
  bool res, no_node_regularity;

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
  ao = KheAugmentOptionsMake(ej, soln, NULL, options, KheEjectorArena(ej));

  /* get some options used by this function */
  no_node_regularity = KheOptionsGetBool(options, "ts_no_node_regularity",
    false);

  /* insert and split a vizier node, if required */
  if( ao->vizier_node )
  {
    viz_node = KheNodeVizierMake(parent_node);
    KheNodeMeetSplit(viz_node, false);
  }

  /* build the required group monitors and solve */
  KheGroupCorrelatedMonitors(soln);
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
  KheAugmentOptionsDelete(ao);
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
  bool res, no_node_regularity;  KHE_AUGMENT_OPTIONS ao;

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
  ao = KheAugmentOptionsMake(ej, soln, NULL, options, KheEjectorArena(ej));

  /* get options used by this function */
  no_node_regularity = KheOptionsGetBool(options, "ts_no_node_regularity",
    false);

  /* insert and split a vizier node, if required */
  if( ao->vizier_node )
  {
    viz_node = KheNodeVizierMake(parent_node);
    KheNodeMeetSplit(viz_node, false);
  }

  /* build the required group monitors and solve */
  KheGroupCorrelatedMonitors(soln);
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
  KheAugmentOptionsDelete(ao);
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

  /* remove unnecessary task assignments */
  soln = KheTaskingSoln(tasking);
  KheSolnTryTaskUnAssignments(soln, false, options);

  /* get options used by this function */
  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);

  /* get an ejector */
  ej = KheEjectionChainEjectorOption(options, "es_ejector2");

  /* set the control options */
  KheOptionsSetBool(options, "es_repair_times", false);
  KheOptionsSetObject(options, "es_limit_node", NULL);
  KheOptionsSetBool(options, "es_repair_resources", true);

  /* build the required group monitors and solve */
  ao = KheAugmentOptionsMake(ej, soln, KheTaskingResourceType(tasking),
    options, KheEjectorArena(ej));
  KheGroupCorrelatedMonitors(soln);
  kempe_gm = KheKempeDemandGroupMonitorMake(soln);
  start_gm = KheTaskingStartGroupMonitorMake(tasking);
  if( DEBUG2 )
  {
    fprintf(stderr, "  initial defects:\n");
    KheGroupMonitorDefectDebug(start_gm, 2, 4, stderr);
  }
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
  KheAugmentOptionsDelete(ao);
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
/*  The ejector used is es_ejector2 from options, which will be created      */
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

  soln = KheMonitorSoln((KHE_MONITOR) limit_resources_gm);
  if( DEBUG1 || DEBUG39 )
  {
    fprintf(stderr, "[ KheEjectionChainRepairInitialResourceAssignment(%.5f)\n",
      KheCostShow(KheSolnCost(soln)));
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
  KheGroupCorrelatedMonitors(soln);
  if( DEBUG39 )
    fprintf(stderr, "  cost after KheGroupCorrelatedMonitors: %.5f\n",
      KheCostShow(KheSolnCost(soln)));
  if( DEBUG24 )
  {
    fprintf(stderr, "  initial defects:\n");
    KheGroupMonitorDefectDebug(limit_resources_gm, 2, 4, stderr);
  }
  ao = KheAugmentOptionsMake(ej, soln, NULL, options, KheEjectorArena(ej));
  KheEjectorSolveBegin(ej, limit_resources_gm, (KHE_GROUP_MONITOR) soln,
    ao, options);
  res = KheEjectorSolveEnd(ej);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final defects:\n");
    KheGroupMonitorDefectDebug(limit_resources_gm, 2, 4, stderr);
  }
  if( DEBUG39 )
    fprintf(stderr, "  cost after KheEjectorSolveEnd: %.5f\n",
      KheCostShow(KheSolnCost(soln)));

  /* clean up, including deleting the group monitors, and return */
  KheOptionsSetBool(options, "es_group_limit_resources_off", save_opt);
  KheUnGroupCorrelatedMonitors(soln);
  if( DEBUG39 )
    fprintf(stderr, "  cost after KheUnGroupCorrelatedMonitors: %.5f\n",
      KheCostShow(KheSolnCost(soln)));
  if( DEBUG1 || DEBUG39 )
   fprintf(stderr, "] %s returning %s (cost %.5f)\n",
     "KheEjectionChainRepairInitialResourceAssignment",
	res ? "true" : "false", KheCostShow(KheSolnCost(soln)));
  return res;
}

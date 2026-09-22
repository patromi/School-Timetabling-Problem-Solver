
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
/*  FILE:         khe_sr_comb_grouper.c                                      */
/*  DESCRIPTION:  Combinatorial grouping                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0	/* recursive search */
#define DEBUG4 0	/* KheCombGrouperAddCombMTask */
#define DEBUG5 0	/* adding requirements */
#define DEBUG6 0	/* groups with debug label */
#define DEBUG7 0	/* bases of recursion */
#define DEBUG8 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_COVER_TYPE - type of cover for cover requirement           */
/*                                                                           */
/*****************************************************************************/

/* defined in khe_solvers.h */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_VARIANT_TYPE - variant of algorithm                        */
/*                                                                           */
/*****************************************************************************/

/* defined in khe_solvers.h */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MTASK_GROUPING - a grouping of mtasks                           */
/*                                                                           */
/*  The last element of leader_mtasks is the leader mtask.  This array is    */
/*  added to each time a task is added to the mtask set, and its last        */
/*  element is removed each time an element (always the last) is deleted     */
/*  from the mtask set.                                                      */
/*                                                                           */
/*  We use ARRAY_KHE_MTASK because we don't think of leader_mtasks as a      */
/*  set.  For example, the same leader task can occur several times.         */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;

typedef struct khe_mtask_grouping_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_MTASK_SET			mtask_set;
  ARRAY_KHE_MTASK		leader_mtasks;
  KHE_COST			cost;
} *KHE_MTASK_GROUPING;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COVER_REQUIREMENT                                               */
/*                                                                           */
/*  One cover requirement (one time group or mtask requirement), including   */
/*  how many times it has been covered.                                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_cover_requirement_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_TIME_GROUP		time_group;	 /* if reqt is a time group  */
  KHE_MTASK			mtask;		 /* if reqt is am mtask      */
  KHE_TIME_SET			time_set;
  KHE_COMB_COVER_TYPE		cover_type;
  bool				last_cover_done; /* last mtask covering e set*/
  int				cover_count;
} *KHE_COVER_REQUIREMENT;

typedef HA_ARRAY(KHE_COVER_REQUIREMENT) ARRAY_KHE_COVER_REQUIREMENT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_MTASK                                                      */
/*                                                                           */
/*  Information about one mtask, including the cover_reqts it covers.        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_comb_mtask_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_MTASK			mtask;
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  ARRAY_KHE_COVER_REQUIREMENT	last_cover_reqts;
} *KHE_COMB_MTASK;

typedef HA_ARRAY(KHE_COMB_MTASK) ARRAY_KHE_COMB_MTASK;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_GROUPER - solver for combinatorial grouping                */
/*                                                                           */
/*****************************************************************************/

struct khe_comb_grouper_rec {

  /* free lists */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqt_free_list;
  ARRAY_KHE_COMB_MTASK		comb_mtask_free_list;

  /* constant for the lifetime of the solver */
  HA_ARENA			arena;
  KHE_MTASK_FINDER		mtask_finder;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;

  /* requirements, may vary freely but always defined */
  ARRAY_KHE_COVER_REQUIREMENT	cover_reqts;
  /* *** max not be needed
  KHE_COMB_MTASK_FN		mtask_fn;
  void				*mtask_fn_impl;
  KHE_COMB_MTASK_SET_FN		mtask_set_fn;
  void				*mtask_set_fn_impl;
  *** */
  bool				no_singles;
  KHE_RESOURCE_GROUP		preferred_domain;
  /* ***
  KHE_PROFILE_TIME_GROUP	profile_tg;
  KHE_RESOURCE_GROUP		profile_domain;
  bool				profile_max_len_on;
  *** */

  /* derived from the requirements; need resetting only when they are reset */
  bool				derived_info_up_to_date;
  ARRAY_KHE_COMB_MTASK		comb_mtasks;
  ARRAY_KHE_MTASK		reqd_mtasks;
  KHE_INTERVAL			busy_in;	/* days selected mtasks busy */
  /* ***
  int				first_time_index;
  int				last_time_index;
  int				first_frame_index;
  int				last_frame_index;
  *** */

  /* varying between and during solves */
  KHE_COMB_VARIANT_TYPE		variant;
  /* char			*debug_str; */
  KHE_GROUP_MONITOR		group_monitor;
  KHE_RESOURCE			group_monitor_resource;
  KHE_MTASK_GROUPING		curr_mg;
  KHE_MTASK_GROUPING		best_mg;
  int				zero_cost_groups_count;
  int				singles_tasks_count;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_COVER_TYPE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)                      */
/*                                                                           */
/*  Show a cover type.                                                       */
/*                                                                           */
/*****************************************************************************/

static char *CoverTypeShow(KHE_COMB_COVER_TYPE cover_type)
{
  switch( cover_type )
  {
    case KHE_COMB_COVER_YES:	return "comb_cover_yes";
    case KHE_COMB_COVER_NO:	return "comb_cover_no";
    case KHE_COMB_COVER_PREV:	return "comb_cover_prev";
    case KHE_COMB_COVER_FREE:	return "comb_cover_free";

    default:

      HnAbort("CoverTypeShow: unexpected cover type (%d)", cover_type);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_VARIANT_TYPE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)                      */
/*                                                                           */
/*  Show a comb variant.                                                     */
/*                                                                           */
/*****************************************************************************/

static char *KheCombVariantShow(KHE_COMB_VARIANT_TYPE cgv)
{
  switch( cgv )
  {
    case KHE_COMB_VARIANT_MIN:	 	return "comb_variant_min";
    case KHE_COMB_VARIANT_ZERO:	 	return "comb_variant_zero";
    case KHE_COMB_VARIANT_SOLE_ZERO:	return "comb_variant_sole_zero";
    case KHE_COMB_VARIANT_SINGLES:	return "comb_variant_singles";

    default:

      HnAbort("KheCombVariantShow: unexpected cost type (%d)", cgv);
      return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_GROUPING"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUPING KheMTaskGroupingMake(KHE_COMB_GROUPER cg)             */
/*                                                                           */
/*  Make a new, empty grouping.                                              */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_GROUPING KheMTaskGroupingMake(KHE_COMB_GROUPER cg)
{
  KHE_MTASK_GROUPING res;
  HaMake(res, cg->arena);
  res->grouper = cg;
  res->mtask_set = KheMTaskSetMake(cg->mtask_finder);
  HaArrayInit(res->leader_mtasks, cg->arena);
  res->cost = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingClear(KHE_MTASK_GROUPING mg)                        */
/*                                                                           */
/*  Clear mg.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingClear(KHE_MTASK_GROUPING mg)
{
  KheMTaskSetClear(mg->mtask_set);
  HaArrayClear(mg->leader_mtasks);
  mg->cost = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingCopy(KHE_MTASK_GROUPING dst_mg,                     */
/*    KHE_MTASK_GROUPING src_mg)                                             */
/*                                                                           */
/*  Copy src_mg into dst_mg.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingCopy(KHE_MTASK_GROUPING dst_mg,
  KHE_MTASK_GROUPING src_mg)
{
  int i;
  KheMTaskSetClear(dst_mg->mtask_set);
  KheMTaskSetAddMTaskSet(dst_mg->mtask_set, src_mg->mtask_set);
  HaArrayClear(dst_mg->leader_mtasks);
  HaArrayAppend(dst_mg->leader_mtasks, src_mg->leader_mtasks, i);
  dst_mg->cost = src_mg->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGroupingMTaskCount(KHE_MTASK_GROUPING mg)                    */
/*                                                                           */
/*  Return the number of mtasks in mg.                                       */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskGroupingMTaskCount(KHE_MTASK_GROUPING mg)
{
  return KheMTaskSetMTaskCount(mg->mtask_set);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupingIsEmpty(KHE_MTASK_GROUPING mg)                      */
/*                                                                           */
/*  Return true if mg is empty.                                              */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGroupingIsEmpty(KHE_MTASK_GROUPING mg)
{
  return KheMTaskGroupingMTaskCount(mg) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFollows(KHE_MTASK mt, KHE_MTASK leader_mt)                  */
/*                                                                           */
/*  Return true if mt can follow leader_mt.                                  */
/*                                                                           */
/*  Implementation note.  When you sort through everything, the only part    */
/*  of KheTaskMoveCheck that could produce a false result here is the        */
/*  task domain check.                                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFollows(KHE_MTASK mt, KHE_MTASK leader_mt)
{
  KHE_TASK task, leader_task;  KHE_COST non_asst_cost, asst_cost;
  /* KHE_RESOURCE r, leader_r; */

  /* make sure assigned resources are compatible */
  /* *** only interested in the unassigned tasks of mtasks now
  r = KheMTaskAsstResource(mt);
  leader_r = KheMTaskAsstResource(leader_mt);
  if( leader_r != NULL )
  {
    ** compatible if r is NULL or equal to leader_r **
    if( r != NULL && r != leader_r )
      return false;
  }
  else
  {
    ** compatible if r is NULL **
    if( r != NULL )
      return false;
  }
  *** */

  /* make sure tasks are assignable */
  task = KheMTaskTask(mt, 0, &non_asst_cost, &asst_cost);
  leader_task = KheMTaskTask(leader_mt, 0, &non_asst_cost, &asst_cost);
  return KheTaskMoveCheck(task, leader_task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetFollows(KHE_MTASK_SET mts, KHE_MTASK leader_mt)          */
/*                                                                           */
/*  Return true if every element of mts can follow leader_mt.                */
/*                                                                           */
/*****************************************************************************/

/* *** not needed now that resource assignments are not being checked
static bool KheMTaskSetFollows(KHE_MTASK_SET mts, KHE_MTASK leader_mt)
{
  KHE_MTASK mt;  int i;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( !KheMTaskFollows(mt, leader_mt) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupingAddMTask(KHE_MTASK_GROUPING mg, KHE_MTASK mt)       */
/*                                                                           */
/*  If mg can receive mt while avoiding overlaps and maintaining a valid     */
/*  leader task, add mt to mg and return true.  Otherwise leave mg as is     */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGroupingAddMTask(KHE_MTASK_GROUPING mg, KHE_MTASK mt)
{
  KHE_MTASK leader_mtask;  KHE_INTERVAL mts_in;

  /* if mt's interval overlaps the existing intervals, no luck */
  mts_in = KheMTaskSetInterval(mg->mtask_set);
  if( !KheIntervalDisjoint(mts_in, KheMTaskInterval(mt)) )
    return false;

  /* if mt is the first mtask, make mt the leader mtask */
  if( KheMTaskSetMTaskCount(mg->mtask_set) == 0 )
  {
    KheMTaskSetAddMTask(mg->mtask_set, mt);
    HaArrayAddLast(mg->leader_mtasks, mt);
    return true;
  }

  /* if mt follows the existing leader mtask, continue with that leader */
  leader_mtask = HaArrayLast(mg->leader_mtasks);
  if( KheMTaskFollows(mt, leader_mtask) )
  {
    KheMTaskSetAddMTask(mg->mtask_set, mt);
    HaArrayAddLast(mg->leader_mtasks, leader_mtask);
    return true;
  }

  /* if the existing leader task follows mt, add mt as leader */
  if( KheMTaskFollows(leader_mtask, mt) )
  {
    KheMTaskSetAddMTask(mg->mtask_set, mt);
    HaArrayAddLast(mg->leader_mtasks, mt);
    return true;
  }

  /* no suitable leader task, so return false */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingDeleteMTask(KHE_MTASK_GROUPING mg, KHE_MTASK mt)    */
/*                                                                           */
/*  Delete mt from mg.  This must be the last mtask.                         */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingDeleteMTask(KHE_MTASK_GROUPING mg, KHE_MTASK mt)
{
  KHE_MTASK last_mt;
  last_mt = KheMTaskSetLastAndDelete(mg->mtask_set);
  HnAssert(mt == last_mt, "KheMTaskGroupingDeleteMTask internal error");
  HaArrayDeleteLast(mg->leader_mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingSetCost(KHE_MTASK_GROUPING mg, KHE_COST cost)       */
/*                                                                           */
/*  Set the cost of mg.                                                      */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
static void KheMTaskGroupingSetCost(KHE_MTASK_GROUPING mg, KHE_COST cost)
{
  mg->cost = cost;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupingIsBetter(KHE_MTASK_GROUPING new_mg,                 */
/*    KHE_MTASK_GROUPING old_mg)                                             */
/*                                                                           */
/*  Return true if new_mg is better than old_mg, either because old_mg       */
/*  is empty or because new_mg has smaller cost.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGroupingIsBetter(KHE_MTASK_GROUPING new_mg,
  KHE_MTASK_GROUPING old_mg)
{
  return KheMTaskGroupingIsEmpty(old_mg) || new_mg->cost < old_mg->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAvailable(KHE_COMB_GROUPER cg, KHE_RESOURCE r)         */
/*                                                                           */
/*  Return true if r is available for all time groups lying in interval      */
/*  cg->busy_in of the days frame.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAvailable(KHE_COMB_GROUPER cg, KHE_RESOURCE r)
{
  int i;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(cg->soln, r);
  for(i = KheIntervalFirst(cg->busy_in); i <= KheIntervalLast(cg->busy_in); i++)
  {
    tg = KheFrameTimeGroup(cg->days_frame, i);
    if( !KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg, NULL,
	  NULL, false) )
      return false;
    if( !KheResourceTimetableMonitorAvailableForTimeGroup(rtm, tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindSuitableResource(KHE_MTASK_GROUPING mg, KHE_RESOURCE *r)     */
/*                                                                           */
/*  Find a suitable resource to assign to the mtasks of mg.  If found,       */
/*  set *r to the resource and return true; otherwise, set *r to NULL        */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheFindSuitableResource(KHE_MTASK_GROUPING mg, KHE_RESOURCE *r)
{
  KHE_MTASK leader_mt;  KHE_RESOURCE_GROUP domain;  int i;

  HnAssert(KheMTaskGroupingMTaskCount(mg) > 0,
    "KheMTaskGroupingFindSuitableResource internal error");
  leader_mt = HaArrayLast(mg->leader_mtasks);
  domain = KheMTaskDomain(leader_mt);
  for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
  {
    *r = KheResourceGroupResource(domain, i);
    if( KheResourceIsAvailable(mg->grouper, *r) )
      return true;
  }
  return *r = NULL, false;

  /* *** no longer dealing with assigned mtasks
  KHE_MTASK mt;  int i;  KHE_RESOURCE_GROUP domain;
  mt = KheTaskerGroupingMTask(cg->tasker, 0);
  *r = KheMTaskAsstResource(mt, 0);
  if( *r != NULL )
  {
    ** leader mtask is already assigned, use the assigned resource **
    return true;
  }
  else
  {
    ** leader task is not already assigned, search its domain **
    domain = KheMTaskDomain(mt);
    for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
    {
      *r = KheResourceGroupResource(domain, i);
      if( KheResourceIsAvailable(cg, *r) )
	return true;
    }
    return *r = NULL, false;
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingClearAssignments(KHE_MTASK_GROUPING mg,             */
/*    KHE_RESOURCE r, int stop)                                              */
/*                                                                           */
/*  Unassign r from mg->mtasks[0 .. stop-1].                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingClearAssignments(KHE_MTASK_GROUPING mg,
  KHE_RESOURCE r, int stop)
{
  int i;  KHE_MTASK mt;
  for( i = 0;  i < stop;  i++ )
  {
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    KheMTaskMoveResource(mt, r, NULL, false);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupingSetCost(KHE_MTASK_GROUPING mg)                      */
/*                                                                           */
/*  Set mg->cost.  This involves assigning one suitable resource to mg.  If  */
/*  there is any problem doing this (e.g. can't find a suitable resource),   */
/*  change nothing and return false.  Either way, all assignments are        */
/*  removed before returning.                                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGroupingSetCost(KHE_MTASK_GROUPING mg)
{
  KHE_MTASK mt;  int i, count, first_time_index, last_time_index;
  KHE_RESOURCE r;  KHE_COMB_GROUPER cg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TIME_GROUP tg;

  /* find a suitable resource */
  if( !KheFindSuitableResource(mg, &r) )
    return false;

  /* assign r to the mtasks of mg */
  count = KheMTaskSetMTaskCount(mg->mtask_set);
  for( i = 0;  i < count;  i++ )
  {
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    if( !KheMTaskMoveResource(mt, NULL, r, false) )
    {
      KheMTaskGroupingClearAssignments(mg, r, i);
      return false;
    }
  }

  /* work out the cost */
  cg = mg->grouper;
  if( cg->group_monitor_resource != r )
  {
    if( cg->group_monitor != NULL )
      KheGroupMonitorDelete(cg->group_monitor);
    cg->group_monitor = KheGroupMonitorMake(cg->soln, 11, "comb grouping");
    cg->group_monitor_resource = r;
    rtm = KheResourceTimetableMonitor(cg->soln, r);
    tg = KheFrameTimeGroup(cg->days_frame, KheIntervalFirst(cg->busy_in));
    first_time_index = KheTimeIndex(KheTimeGroupTime(tg, 0));
    tg = KheFrameTimeGroup(cg->days_frame, KheIntervalLast(cg->busy_in));
    last_time_index = KheTimeIndex(KheTimeGroupTime(tg,
	KheTimeGroupTimeCount(tg) - 1));
    KheResourceTimetableMonitorAddRange(rtm, /* cg-> */ first_time_index,
      /* cg-> */ last_time_index, cg->group_monitor);
  }
  mg->cost = KheMonitorCost((KHE_MONITOR) cg->group_monitor);

  /* unassign the mtasks (we just need the cost) and return true */
  KheMTaskGroupingClearAssignments(mg, r, count);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskSetMinNeedsAssignmentCount(KHE_MTASK_SET mts)                */
/*                                                                           */
/*  Return the minimum, over the mtasks mt of mts, of the number of          */
/*  tasks needing assignment in mt.                                          */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskSetMinNeedsAssignmentCount(KHE_MTASK_SET mts)
{
  int res, val, i;  KHE_MTASK mt;
  res = INT_MAX;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    val = KheMTaskNeedsAssignmentCount(mt);
    if( val < res )
      res = val;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingMakeOneGroup(KHE_MTASK_GROUPING mg,                 */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make one group out of the mtasks of mg.  If sa != NULL, record the       */
/*  changes in sa so that they can be undone later.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingMakeOneGroup(KHE_MTASK_GROUPING mg,
  KHE_SOLN_ADJUSTER sa, char *debug_str)
{
  KHE_MTASK leader_mt, mt;  KHE_TASK leader_task, task;  int i;
  KHE_COST non_asst_cost, asst_cost;  KHE_MTASK_FINDER mtf;

  /* find the leader task */
  leader_mt = HaArrayLast(mg->leader_mtasks);
  HnAssert(KheMTaskUnassignedTaskCount(leader_mt) > 0,
    "KheMTaskGroupingMakeOneGroup internal error 1");
  leader_task = KheMTaskTask(leader_mt, KheMTaskAssignedTaskCount(leader_mt),
    &non_asst_cost, &asst_cost);

  /* build the group */
  mtf = mg->grouper->mtask_finder;
  KheMTaskFinderGroupBegin(mtf, leader_task);
  for( i = 0;  i < KheMTaskSetMTaskCount(mg->mtask_set);  i++ )
  {
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    if( mt != leader_mt )
    {
      HnAssert(KheMTaskUnassignedTaskCount(mt) > 0,
	"KheMTaskGroupingMakeOneGroup internal error 2");
      task = KheMTaskTask(mt, KheMTaskAssignedTaskCount(mt),
	&non_asst_cost, &asst_cost);
      if( !KheMTaskFinderGroupAddTask(mtf, task) )
	HnAbort("KheMTaskGroupingMakeOneGroup internal error 3");
    }
  }
  KheMTaskFinderGroupEnd(mtf, sa);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGroupingExecute(KHE_MTASK_GROUPING mg, int max_num,          */
/*    KHE_SOLN_ADJUSTER sa, char *debug_str)                                 */
/*                                                                           */
/*  Carry out a grouping of the mtasks of mg.  Make as many groups as        */
/*  possible, but not more than max_num.  If sa != NULL, record the          */
/*  changes in sa so that they can be undone later.                          */
/*                                                                           */
/*****************************************************************************/
static void KheMTaskGroupingDebug(KHE_MTASK_GROUPING mg,
  int verbosity, int indent, FILE *fp);

static int KheMTaskGroupingExecute(KHE_MTASK_GROUPING mg, int max_num,
  KHE_SOLN_ADJUSTER sa, char *debug_str)
{
  int num, res;
  res = 0;
  if( KheMTaskSetMTaskCount(mg->mtask_set) >= 2 )
  {
    /* set num to the number of groups to make */
    num = KheMTaskSetMinNeedsAssignmentCount(mg->mtask_set);
    if( max_num < num )
      num = max_num;

    /* make num groups */
    if( DEBUG6 )
    {
      fprintf(stderr, "  %s: %d x ", debug_str, num);
      KheMTaskGroupingDebug(mg, 2, 0, stderr);
    }
    for( res = 0;  res < num;  res++ )
      KheMTaskGroupingMakeOneGroup(mg, sa, debug_str);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingDebug(KHE_MTASK_GROUPING mg,                        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of mg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingDebug(KHE_MTASK_GROUPING mg,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_MTASK mt;  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{%.5f: ", KheCostShow(mg->cost));
  for( i = 0;  i < KheMTaskSetMTaskCount(mg->mtask_set);  i++ )
  {
    if( i > 0 )
      fprintf(fp, ", ");
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    if( mt == HaArrayLast(mg->leader_mtasks) )
      fprintf(fp, "|");
    /* fprintf(fp, "%s", KheMTaskId(mt)); */
    /* KheMTaskDebug(mt, verbosity, -1, fp); */
    task = KheMTaskTask(mt, 0, &non_asst_cost, &asst_cost);
    KheTaskDebug(task, 2, -1, fp);
    if( mt == HaArrayLast(mg->leader_mtasks) )
      fprintf(fp, "|");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COVER_REQUIREMENT"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)            */
/*                                                                           */
/*  Make a cover requirement object.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_COVER_REQUIREMENT KheCoverRequirementMake(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  if( HaArrayCount(cg->cover_reqt_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->cover_reqt_free_list);
    KheTimeSetClear(res->time_set);
  }
  else
  {
    HaMake(res, cg->arena);
    res->time_set = KheTimeSetMake(KheSolnInstance(cg->soln), cg->arena);
  }
  res->grouper = cg;
  res->time_group = tg;
  res->mtask = mt;
  if( tg != NULL )
    KheTimeSetAddTimeGroup(res->time_set, tg);
  res->cover_type = cover;
  res->last_cover_done = false;
  res->cover_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)  */
/*                                                                           */
/*  Return true if cr is covered by mt.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheCoverRequirementCovered(KHE_COVER_REQUIREMENT cr, KHE_MTASK mt)
{
  if( cr->time_group != NULL )
    return !KheTimeSetDisjoint(cr->time_set, KheMTaskTimeSet(mt));
  else
    return cr->mtask == mt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr, FILE *fp)  */
/*                                                                           */
/*  Debug print of the header of a cover requirement.                        */
/*                                                                           */
/*****************************************************************************/

static void KheCoverRequirementHeaderDebug(KHE_COVER_REQUIREMENT cr,
  FILE *fp)
{
  if( cr->time_group != NULL )
    fprintf(fp, "CoverRequirement(tg %s, %s, %d covers)",
      KheTimeGroupId(cr->time_group), CoverTypeShow(cr->cover_type),
      cr->cover_count);
  else
    fprintf(fp, "CoverRequirement(mt %p %s %s, %d tasks, %s, %d covers)",
      (void *) cr->mtask, KheMTaskId(cr->mtask),
      KheIntervalShow(KheMTaskInterval(cr->mtask), cr->grouper->days_frame),
      KheMTaskTaskCount(cr->mtask), CoverTypeShow(cr->cover_type),
      cr->cover_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/
static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp);

static void KheCoverRequirementDebug(KHE_COVER_REQUIREMENT cr, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCoverRequirementHeaderDebug(cr, fp);
    fprintf(fp, "\n");
  }
  else
    KheCoverRequirementHeaderDebug(cr, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_MTASK"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)       */
/*                                                                           */
/*  Make an mtask info object.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_COMB_MTASK KheCombMTaskMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COMB_MTASK res;
  if( HaArrayCount(cg->comb_mtask_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->comb_mtask_free_list);
    HaArrayClear(res->cover_reqts);
    HaArrayClear(res->last_cover_reqts);
  }
  else
  {
    HaMake(res, cg->arena);
    HaArrayInit(res->cover_reqts, cg->arena);
    HaArrayInit(res->last_cover_reqts, cg->arena);
  }
  res->grouper = cg;
  res->mtask = mt;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)                           */
/*                                                                           */
/*  Return the "size" of cmt's domain, which is either the number of         */
/*  resources in cmt->mtask's domain, or INT_MAX if that does not suit.      */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskDomainSize(KHE_COMB_MTASK cmt)
{
  KHE_RESOURCE_GROUP domain;
  domain = KheMTaskDomain(cmt->mtask);
  if( KheResourceGroupSubset(cmt->grouper->preferred_domain, domain) )
    return KheResourceGroupResourceCount(domain);
  else
    return INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombMTaskCmp(const void *t1, const void *t2)                      */
/*                                                                           */
/*  Comparison function for sorting an array of comb mtask objects.          */
/*                                                                           */
/*  The first priority is to sort the array so that mtasks that start on     */
/*  earlier days come before mtasks that start on later days.  This is       */
/*  vital because we use intervals during the solve and they assume that     */
/*  sets of mtasks have no gaps.                                             */
/*                                                                           */
/*  The second priority is to sort the array so that mtasks whose domains    */
/*  are a better fit for cg->preferred_domain (if present) come before       */
/*  mtasks whose fit is worse.  If the mtask's domain is not a superset      */
/*  of the preferred domain, it is maximally bad.  If it is a superset,      */
/*  then smaller domains are preferred to larger ones.                       */
/*                                                                           */
/*****************************************************************************/

static int KheCombMTaskCmp(const void *t1, const void *t2)
{
  KHE_COMB_MTASK cmt1, cmt2;  KHE_INTERVAL in1, in2;  int cmp;
  cmt1 = * (KHE_COMB_MTASK *) t1;
  cmt2 = * (KHE_COMB_MTASK *) t2;

  /* first priority: chronologically earlier mtasks precede later ones */
  in1 = KheMTaskInterval(cmt1->mtask);
  in2 = KheMTaskInterval(cmt2->mtask);
  cmp = KheIntervalFirst(in1) - KheIntervalFirst(in2);
  if( cmp != 0 )
    return cmp;

  /* second priority: preferred domain, if requested */
  if( cmt1->grouper->preferred_domain != NULL )
  {
    cmp = KheCombMTaskDomainSize(cmt1) - KheCombMTaskDomainSize(cmt2);
    if( cmp != 0 )
      return cmp;
  }

  /* no difference */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)                      */
/*                                                                           */
/*  Return true if cmt is the last possible cover for a cover requirement    */
/*  that is not yet covered, so that consequently it must be included.       */
/*                                                                           */
/*****************************************************************************/

static bool KheCombMTaskMustBeIncluded(KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->last_cover_reqts, cr, i)
    if( cr->cover_count == 0 )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)               */
/*                                                                           */
/*  Debug print of the header of cmt onto fp.                                */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskHeaderDebug(KHE_COMB_MTASK cmt, FILE *fp)
{
  fprintf(fp, "CombMTask(%p %s, %d tasks, %d unassigned, %d cover_reqts, "
    "%d last_cover)", (void *) cmt->mtask, KheMTaskId(cmt->mtask),
    KheMTaskTaskCount(cmt->mtask), KheMTaskUnassignedTaskCount(cmt->mtask),
    HaArrayCount(cmt->cover_reqts), HaArrayCount(cmt->last_cover_reqts));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cmt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheCombMTaskDebug(KHE_COMB_MTASK cmt, int verbosity,
  int indent, FILE *fp)
{
  if( indent >= 0 )
  {
    fprintf(fp, "%*s", indent, "");
    KheCombMTaskHeaderDebug(cmt, fp);
    fprintf(fp, "\n");
  }
  else
    KheCombMTaskHeaderDebug(cmt, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - construction"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,                */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new comb grouper based on mtf.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_COMB_GROUPER res;

  /* free lists */
  HaMake(res, a);
  HaArrayInit(res->cover_reqt_free_list, a);
  HaArrayInit(res->comb_mtask_free_list, a);

  /* constant for the lifetime of the solver */
  res->arena = a;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->resource_type = rt;  /* KheMTaskFinderResourceType(mtf); */
  res->days_frame = KheMTaskFinderDaysFrame(mtf);

  /* requirements, may vary freely but always defined */
  HaArrayInit(res->cover_reqts, a);
  /* ***
  res->mtask_fn = NULL;
  res->mtask_fn_impl = NULL;
  res->mtask_set_fn = NULL;
  res->mtask_set_fn_impl = NULL;
  *** */
  res->no_singles = false;
  res->preferred_domain = NULL;
  /* ***
  res->profile_tg = NULL;
  res->profile_domain = NULL;
  res->profile_max_len_on = false;
  *** */

  /* derived from the requirements; need resetting only when they are reset */
  res->derived_info_up_to_date = false;
  HaArrayInit(res->comb_mtasks, a);
  HaArrayInit(res->reqd_mtasks, a);
  res->busy_in = KheIntervalMake(1, 0);
  /* ***
  res->first_time_index = 0;
  res->last_time_index = 0;
  res->first_frame_index = 0;
  res->last_frame_index = 0;
  *** */

  /* varying between and during solves */
  res->variant = KHE_COMB_VARIANT_SINGLES;
  /* res->debug_str = NULL; */
  res->group_monitor = NULL;
  res->group_monitor_resource = NULL;
  res->curr_mg = KheMTaskGroupingMake(res);
  res->best_mg = KheMTaskGroupingMake(res);
  res->zero_cost_groups_count = 0;
  res->singles_tasks_count = 0;

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Return the mtask finder attribute of cg.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_FINDER KheCombGrouperMTaskFinder(KHE_COMB_GROUPER cg)
{
  return cg->mtask_finder;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)        */
/*                                                                           */
/*  Return the resource type handled by cg.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheCombGrouperResourceType(KHE_COMB_GROUPER cg)
{
  return cg->resource_type;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Clear away all requirements.                                             */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)
{
  int i;
  HaArrayAppend(cg->cover_reqt_free_list, cg->cover_reqts, i);
  HaArrayClear(cg->cover_reqts);
  /* ***
  cg->mtask_fn = NULL;
  cg->mtask_fn_impl = NULL;
  cg->mtask_set_fn = NULL;
  cg->mtask_set_fn_impl = NULL;
  *** */
  /* ***
  cg->profile_tg = NULL;
  cg->profile_domain = NULL;
  cg->profile_max_len_on = false;
  *** */
  cg->no_singles = false;
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperClearRequirements(cg) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)                          */
/*                                                                           */
/*  Add a time group cover requirement to the solver.                        */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, tg, NULL, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddTimeGroupRequirement(cg, %s, %s) ]\n",
      KheTimeGroupId(tg) != NULL ? KheTimeGroupId(tg) : "-",
      CoverTypeShow(cover));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,       */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Undo the effect of a corresponding time group add.                       */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->time_group == tg )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteTimeGroupRequirement: tg not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,              */
/*    KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)                               */
/*                                                                           */
/*  Add an mtask cover requirement to the solver.                            */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_COVER_REQUIREMENT res;
  res = KheCoverRequirementMake(cg, NULL, mt, cover);
  HaArrayAddLast(cg->cover_reqts, res);
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperAddMTaskRequirement(cg, mt) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,           */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Undo the effect of a corresponding mtask add.                            */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->mtask == mt )
    {
      HaArrayDeleteAndShift(cg->cover_reqts, i);
      HaArrayAddLast(cg->cover_reqt_free_list, cr);
      cg->derived_info_up_to_date = false;
      return;
    }
  HnAbort("KheCombGrouperDeleteMTaskRequirement: mt not present");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddMTaskFnRequirement(KHE_COMB_GROUPER cg,            */
/*    KHE_COMB_MTASK_FN mtask_fn, void *impl)                                */
/*                                                                           */
/*  Add an mtask function requirement to cg.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** may not be needed
void KheCombGrouperAddMTaskFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_FN mtask_fn, void *impl)
{
  HnAssert(cg->mtask_fn == NULL, "KheCombGrouperAddMTaskFnRequirement "
    "internal error (mtask_fn already set)");
  cg->mtask_fn = mtask_fn;
  cg->mtask_fn_impl = impl;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskFnRequirement(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Delete any mtask function requirement from cg.                           */
/*                                                                           */
/*****************************************************************************/

/* *** may not be needed
void KheCombGrouperDeleteMTaskFnRequirement(KHE_COMB_GROUPER cg)
{
  cg->mtask_fn = NULL;
  cg->mtask_fn_impl = NULL;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddMTaskSetFnRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_COMB_MTASK_SET_FN mtask_set_fn, void *impl)                        */
/*                                                                           */
/*  Add an mtask set function requirement to cg.                             */
/*                                                                           */
/*****************************************************************************/

/* *** may not be needed
void KheCombSolverAddMTaskSetFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_SET_FN mtask_set_fn, void *impl)
{
  HnAssert(cg->mtask_set_fn == NULL, "KheCombGrouperAddMTaskSetFnRequirement "
    "internal error (mtask_set_fn already set)");
  cg->mtask_set_fn = mtask_set_fn;
  cg->mtask_set_fn_impl = impl;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteMTaskSetFnRequirement(KHE_COMB_GROUPER cg)       */
/*                                                                           */
/*  Delete any mtask set function requirement to cg.                         */
/*                                                                           */
/*****************************************************************************/

/* *** may not be needed
void KheCombSolverDeleteMTaskSetFnRequirement(KHE_COMB_GROUPER cg)
{
  cg->mtask_set_fn = NULL;
  cg->mtask_set_fn_impl = NULL;
  cg->derived_info_up_to_date = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddProfileGroupRequirement(KHE_COMB_GROUPER cg,       */
/*    KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain)                 */
/*                                                                           */
/*  Add a requirement that all combinations tried must contain a mtask       */
/*  that covers ptg and (optionally) has the given domain.                   */
/*                                                                           */
/*  At most one requirement can be added in this way.                        */
/*                                                                           */
/*  Implementation note.  Changing this requirement does make the derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperAddProfileGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_PROFILE_TIME_GROUP ptg, KHE_RESOURCE_GROUP domain)
{
  HnAssert(ptg!=NULL,"KheCombGrouperAddProfileGroupRequirement: ptg is NULL");
  cg->profile_tg = ptg;
  cg->profile_domain = domain;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteProfileGroupRequirement(KHE_COMB_GROUPER cg,    */
/*    KHE_PROFILE_TIME_GROUP ptg)                                            */
/*                                                                           */
/*  Undo the effect of a corresponding profile group add.                    */
/*                                                                           */
/*  Implementation note.  Changing this requirement does make the derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperDeleteProfileGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_PROFILE_TIME_GROUP ptg)
{
  HnAssert(ptg != NULL,
    "KheCombGrouperDeleteProfileGroupRequirement: ptg is NULL");
  HnAssert(ptg == cg->profile_tg,
    "KheCombGrouperDeleteProfileGroupRequirement: ptg not present");
  cg->profile_tg = NULL;
  cg->profile_domain = NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddProfileMaxLenRequirement(KHE_COMB_GROUPER cg)      */
/*                                                                           */
/*  Add a profile max len requirement.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperAddProfileMaxLenRequirement(KHE_COMB_GROUPER cg)
{
  if( !cg->profile_max_len_on )
  {
    cg->profile_max_len_on = true;
    cg->derived_info_up_to_date = false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteProfileMaxLenRequirement(KHE_COMB_GROUPER cg)   */
/*                                                                           */
/*  Delete a corresponding profile max len add.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheCombGrouperDeleteProfileMaxLenRequirement(KHE_COMB_GROUPER cg)
{
  if( cg->profile_max_len_on )
  {
    cg->profile_max_len_on = false;
    cg->derived_info_up_to_date = false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)          */
/*                                                                           */
/*  Add a requirement that there be no singles.                              */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)       */
/*                                                                           */
/*  Undo the effect of a corresponding no singles add.                       */
/*                                                                           */
/*  Implementation note.  Changing this requirement does not make derived    */
/*  info out of date, hence we don't change cg->derived_info_up_to_date.     */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteNoSinglesRequirement(KHE_COMB_GROUPER cg)
{
  cg->no_singles = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,    */
/*    KHE_RESOURCE_GROUP rg)                                                 */
/*                                                                           */
/*  Add a preferred domain requirement (for rg) to cg.                       */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddPreferredDomainRequirement(KHE_COMB_GROUPER cg,
  KHE_RESOURCE_GROUP rg)
{
  cg->preferred_domain = rg;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg) */
/*                                                                           */
/*  Delete any preferred domain requirement from cg.                         */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeletePreferredDomainRequirement(KHE_COMB_GROUPER cg)
{
  cg->preferred_domain = NULL;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - setting up for solving"                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Set the required mtasks of cg.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperSetRequiredMTasks(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayClear(cg->reqd_mtasks);
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_YES && cr->mtask != NULL )
      HaArrayAddLast(cg->reqd_mtasks, cr->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,    */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Return true if mt clashes with any of the required mtasks of cg,         */
/*  without being required itself.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskClashesWithRequiredMTask(KHE_COMB_GROUPER cg,
  KHE_MTASK mt)
{
  KHE_MTASK mt2;  int i, pos;
  if( !HaArrayContains(cg->reqd_mtasks, mt, &pos) )
  {
    HaArrayForEach(cg->reqd_mtasks, mt2, i)
      if( !KheIntervalDisjoint(KheMTaskInterval(mt), KheMTaskInterval(mt2)) )
	return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)      */
/*                                                                           */
/*  Return true if mt is wanted, because it has not already been chosen, it  */
/*  does not have fixed assignments, it has at least one unassigned task,    */
/*  it does not cover any No elt, and the mtask_fn requirement (if present)  */
/*  allows it.                                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskIsWanted(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COVER_REQUIREMENT cr;  int i;  KHE_COMB_MTASK cmt;

  /* if mt has already been chosen, we don't want it again */
  HaArrayForEach(cg->comb_mtasks, cmt, i)
    if( cmt->mtask == mt )
      return false;

  /* if mt covers any No requirement, we don't want it (2) */
  HaArrayForEach(cg->cover_reqts, cr, i)
    if( cr->cover_type == KHE_COMB_COVER_NO &&
	KheCoverRequirementCovered(cr, mt) )
      return false;

  /* if mt is fixed or does not need assignment, we don't want it (3) */
  if( KheMTaskAssignIsFixed(mt) || !KheMTaskNeedsAssignment(mt) )
    return false;

  /* if mt has no unassigned tasks, we don't want it (4)  */
  /* ***
  if( KheMTaskUnassignedTaskCount(mt) == 0 )
    return false;
  *** */

  /* if mt is not in cg->reqd_mtasks but it does clash with them, no */
  if( KheCombGrouperMTaskClashesWithRequiredMTask(cg, mt) )
    return false;

  /* if domain is wanted but c doesn't have it, we don't want c */
  /* ***
  if( domain != NULL && !KheResourceGroupEqual(domain,KheTaskerMTaskDomain(c)) )
    return false;
  *** */

  /* if mtask_fn is set, check that requirement (5) */
  /* ***
  if( cg->mtask_fn != NULL && !cg->mtask_fn(mt, cg->mtask_fn_impl) )
    return false;
  *** */

  /* if profile_max_len_on, check that requirement */
  /* ***
  if( cg->profile_max_len_on &&
      KheTaskerMTaskProfileTimeCount(mt) > KheTaskerProfileMaxLen(cg->tasker) )
    return false;
  *** */

  /* otherwise all good and we want mt */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddMTask(KHE_COMB_GROUPER cg, KHE_MTASK mt)           */
/*                                                                           */
/*  We've decided that mt is wanted by the solve.  So make a mtask info      */
/*  object cmt for it and add the cover_reqts covered by mt to cmt.  Also    */
/*  add cmt to the first cover requirement it covers.                        */
/*                                                                           */
/*  Only do this if mt actually covers something.  But it will.              */
/*                                                                           */
/*****************************************************************************/

/* *** simplified then inlined
static void KheCombGrouperAddMTask(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_COMB_MTASK cmt;  KHE_COVER_REQUIREMENT e;  int i;
  cmt = NULL;
  HaArrayForEach(cg->cover_reqts, e, i)
  {
    HnAssert(e->cover_type != KHE_COMB_COVER_NO,
      "KheCombGrouperAddMTask internal error");
    if( KheCoverRequirementCovered(e, mt) )
    {
      if( cmt == NULL )
      {
	cmt = KheCombMTaskMake(cg, mt);
	HaArrayAddLast(cg->comb_mtask, cmt);
	** HaArrayAddLast(e->first_cover_comb_mtask, cmt); **
      }
      HaArrayAddLast(cmt->cover_reqts, e);
    }
  }
  HnAssert(cmt != NULL, "KheCombGrouperAddMTask internal error");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperMakeCombMTaskReady(KHE_COMB_GROUPER cg)               */
/*                                                                           */
/*  Make mtask info, ready for solving.  This finds the mtasks that lie in   */
/*  the search space, and sets cg->first_time_index and cg->last_time_index. */
/*                                                                           */
/*****************************************************************************/

/* *** inlined now
static void KheCombGrouperMakeCombMTaskReady(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT e;  int i, j, k, tg_count, index, count;  KHE_INSTANCE ins;
  KHE_MTASK mt;  KHE_TIME time;

  if( !cg->derived_info_up_to_date )
  {
    ** initialize comb_mtask for this solve, first removing any old mtasks **
    HaArrayAppend(cg->comb_mtask_free_list, cg->comb_mtask, i);
    HaArrayClear(cg->comb_mtask);
    HaArrayForEach(cg->cover_reqts, e, i)
      if( e->cover_type != KHE_COMB_COVER_NO )
      {
	if( e->time_group != NULL )
	{
	  ** cover reqt is a time group **
	  for( j = 0;  j < KheTimeGroupTimeCount(e->time_group);  j++ )
	  {
	    time = KheTimeGroupTime(e->time_group, j);
	    count = KheMTaskFin derMTaskAtTimeCount(cg->mtask_finder, time);
	    for( k = 0;  k < count;  k++ )
	    {
	      mt = KheMTaskFinderMTaskAtTime(cg->mtask_finder, time, k);
	      if( KheCombGrouperMTaskIsWanted(cg, mt) )
		KheCombGrouperAddMTask(cg, mt);
	    }
	  }
	}
	else
	{
	  ** cover reqt is an mtask **
	  if( KheCombGrouperMTaskIsWanted(cg, e->mtask) )
	    KheCombGrouperAddMTask(cg, e->mtask);
	}
      }

    ** initialize first_time_index and last_time_index **
    cg->first_time_index = INT_MAX;
    cg->last_time_index = -1;
    HaArrayForEach(cg->cover_reqts, e, i)
      if( e->time_group != NULL )
      {
	tg_count = KheTimeGroupTimeCount(e->time_group);
	if( tg_count > 0 )
	{
	  index = KheTimeIndex(KheTimeGroupTime(e->time_group, 0));
	  if( index < cg->first_time_index )
	    cg->first_time_index = index;
	  index = KheTimeIndex(KheTimeGroupTime(e->time_group, tg_count - 1));
	  if( index > cg->last_time_index )
	    cg->last_time_index = index;
	}
      }

    ** initialize first_frame_index and last_frame_index **
    ins = KheSolnInstance(cg->soln);
    time = KheInstanceTime(ins, cg->first_time_index);
    cg->first_frame_index = KheFrameTimeIndex(cg->days_frame, time);
    time = KheInstanceTime(ins, cg->last_time_index);
    cg->last_frame_index = KheFrameTimeIndex(cg->days_frame, time);

    ** ready now **
    cg->derived_info_up_to_date = true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - the actual solve"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt) */
/*                                                                           */
/*  Add cmt and return true, or return false and change nothing if adding    */
/*  cmt would not work with the current grouping.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperAddCombMTask(KHE_COMB_GROUPER cg, KHE_COMB_MTASK cmt,
  int curr_index)
{
  KHE_COVER_REQUIREMENT cr;  int i;

  /* make sure that no reqt would be covered twice if cmt were added */
  /* *** we may not need this now that we are checking intervals
  HaArrayForEach(cmt->cover_reqts, e, i)
    if( e->cover_count > 0 )
    {
      if( DEBUG4 )
      {
	fprintf(stderr, "%*s  add mtask info failed on:\n", 2*curr_index, "");
	KheCoverRequirementDebug(e, 1, 2*curr_index + 2, stderr);
      }
      return false;
    }
  *** */

  /* make sure that cmt's mtask is acceptable to the current grouping */
  if( !KheMTaskGroupingAddMTask(cg->curr_mg, cmt->mtask) )    /* (6), (7) */
  /* if( !KheTaskFinderGroupAddTask(cg->mtask_finder, cmt->mtask) ) */
  {
    if( DEBUG4 )
      fprintf(stderr, "%*s  add mtask info failed on grouping (%d mtasks)\n",
	2*curr_index, "", /* KheTaskerGroupingMTaskCount(cg->tasker) */
	KheMTaskGroupingMTaskCount(cg->curr_mg));
    return false;
  }

  /* all good, update the cover counts and return true */
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,                  */
/*    KHE_COMB_MTASK cmt)                                                    */
/*                                                                           */
/*  Undo the corresponding call to KheCombGrouperAddCombMTask.               */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperDeleteCombMTask(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK cmt)
{
  KHE_COVER_REQUIREMENT cr;  int i;
  HaArrayForEach(cmt->cover_reqts, cr, i)
    cr->cover_count--;
  KheMTaskGroupingDeleteMTask(cg->curr_mg, cmt->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperCurrGroupingCoversRequirements(KHE_COMB_GROUPER cg)   */
/*                                                                           */
/*  Return true if cg's current mtask set has valid cover.  This is (11).    */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperCurrGroupingCoversRequirements(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT cr, prev_cr;  int i;
  prev_cr = NULL;
  HaArrayForEach(cg->cover_reqts, cr, i)
  {
    switch( cr->cover_type )
    {
      case KHE_COMB_COVER_YES:

	/* should be covered at least once by now */
	if( DEBUG8 && cr->cover_count <= 0 )
	{
	  fprintf(stderr, "  KheCurrGroupingCoversRequirements failing:\n");
          KheCombGrouperDebug(cg, 2, 2, stderr);
	}
	HnAssert(cr->cover_count > 0,
	  "KheCombGrouperCurrGroupingCoversRequirements internal error 1");
	break;

      case KHE_COMB_COVER_NO:

	/* should be not covered, in fact no mtasks that cover it are tried */
	HnAssert(cr->cover_count == 0,
	  "KheCombGrouperCurrGroupingCoversRequirements internal error 2");
	break;

      case KHE_COMB_COVER_PREV:

	/* should be covered if and only if prev is covered */
	if( prev_cr != NULL &&
	    (cr->cover_count > 0) != (prev_cr->cover_count > 0) )
	  return false;
	break;

      case KHE_COMB_COVER_FREE:

	/* any cover is acceptable here */
	break;

      default:

	HnAbort("KheCombGrouperCurrGroupingCoversRequirements internal error"
	  " (%d)", cr->cover_type);
    }
    prev_cr = cr;
  }
  return true;
}


/* *** old version used when iterating over cover_reqts
static bool KheCombGrouperCurrGroupingCoversRequirements(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT e, prev_e;  int i;
  prev_e = NULL;
  HaArrayForEach(cg->cover_reqts, e, i)
  {
    HnAssert(e->cover_count <= 1, "KheCombGrouperMTaskSetIsValid: 1");
    switch( e->cover_type )
    {
      case KHE_COMB_COVER_YES:

	** should be covered once **
	if( e->cover_count != 1 )
	  return false;
	break;

      case KHE_COMB_COVER_NO:

	** should be not covered **
	HnAssert(e->cover_count == 0, "KheCombGrouperMTaskSetIsValid: 2");
	break;

      case KHE_COMB_COVER_PREV:

	** should be covered if and only if prev is covered **
	if( prev_e != NULL && e->cover_count != prev_e->cover_count )
	  return false;
	break;

      case KHE_COMB_COVER_FREE:

	** 0 or 1 is fine here **
	break;

      default:

	HnAbort("KheCombGrouperMTaskSetIsValid: invalid cover (%d)",
	  e->cover_type);
    }
    prev_e = e;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTry(KHE_COMB_GROUPER cg, int curr_index,                         */
/*    KHE_COVER_REQUIREMENT curr_e, bool try_nothing, bool try_something)    */
/*                                                                           */
/*  Handle curr_e at curr_index.  If try_nothing is true, try assigning      */
/*  nothing here; if try_something is true, try assigning a mtask here.      */
/*                                                                           */
/*****************************************************************************/

/* *** inlined now
static void KheCombGrouper SolveFrom(KHE_COMB_GROUPER cg, int curr_index);

static void KheTry(KHE_COMB_GROUPER cg, int curr_index, KHE_COVER_REQUIREMENT curr_e,
  bool try_nothing, bool try_something)
{
  KHE_COMB_MTASK cmt;  int i;

  ** try nothing if requested **
  if( try_nothing )
  {
    KheCombGroup erSolveFrom(cg, curr_index + 1);
    if( cg->stopping )
      return;
  }

  ** try something if requested and it would not violate singles limit **
  if( try_something && (cg->alg != KHE_COMB_SINGLES ||
	KheMTaskGroupingMTaskCount(cg->curr_mg) == 0) )
    HaArrayForEach(curr_e->first_cover_comb_mtask, cmt, i)
    {
      if( KheCombGrouperAddCombMTask(cg, cmt, curr_index) )
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  [ adding mtask:\n", 2*curr_index, "");
	  KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	}
	KheCombGroup erSolveFrom(cg, curr_index + 1);
	KheCombGrouperDeleteCombMTask(cg, cmt);
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  ] removing mtask:\n", 2*curr_index, "");
	  KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	}
	if( cg->stopping )
	  return;
      }
      else
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  not adding mtask:\n", 2*curr_index, "");
	  KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	}
      }
    }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingSatisfiesProfileRequirement(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Return true if the current grouping satisfies cg's profile requirement.  */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn, but save it for future reference
static bool KheGroupingSatisfiesProfileRequirement(KHE_COMB_GROUPER cg)
{
  int i;  KHE_MTASK c;
  if( cg->profile_tg == NULL )
    return true;
  for( i = 0;  i < KheTaskerGroupingMTaskCount(cg->tasker);  i++ )
  {
    c = KheTaskerGroupingMTask(cg->tasker, i);
    if( KheTaskerMTaskCoversProfileTimeGroup(c, cg->profile_tg) &&
	(cg->profile_domain == NULL ||
	 KheResourceGroupEqual(cg->profile_domain, KheTaskerMTaskDomain(c))) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupingSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Return true if the current grouping satisfies cg's singles requirement.  */
/*                                                                           */
/*****************************************************************************/

static bool KheCurrGroupingSatisfiesSinglesRequirement(KHE_COMB_GROUPER cg)
{
  return KheMTaskGroupingMTaskCount(cg->curr_mg) >= (cg->no_singles ? 2 : 1);
  /* ***
  return KheTaskerGroupingMTaskCount(cg->tasker) >= (cg->no_singles ? 2 : 1);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCurrGroupingSatisfiesMTaskSetFnRequirement(KHE_COMB_GROUPER cg)  */
/*                                                                           */
/*  Return true if cg satisfies the mtask set fn requirement.                */
/*                                                                           */
/*****************************************************************************/

/* *** may not be needed
static bool KheCurrGroupingSatisfiesMTaskSetFnRequirement(KHE_COMB_GROUPER cg)
{
  return cg->mtask_set_fn == NULL ||
    cg->mtask_set_fn(cg->curr_mg->mtask_set, cg->mtask_set_fn_impl);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)                      */
/*                                                                           */
/*  Return true if singles requires all mtasks from now on to be excluded.   */
/*                                                                           */
/*****************************************************************************/

static bool KheSinglesRequiresExclude(KHE_COMB_GROUPER cg)
{
  return cg->variant == KHE_COMB_VARIANT_SINGLES &&
    KheMTaskGroupingMTaskCount(cg->curr_mg) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)        */
/*                                                                           */
/*  Solve cg from cg->comb_mtask[curr_index] onwards.  Return true if it's   */
/*  time to stop.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)
{
  KHE_MTASK mt;  KHE_COMB_MTASK curr_cmt;  bool stopping;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cg, %d):\n", 2*curr_index+2,"",curr_index);
  if( curr_index >= HaArrayCount(cg->comb_mtasks) )
  {
    /* end of recursion; if suitable mtasks, try them */
    if( KheCurrGroupingSatisfiesSinglesRequirement(cg) &&        /* (9)  */
        /* KheCurrGroupingSatisfiesMTaskSetFnRequirement(cg) */  /* (10) */
        KheCombGrouperCurrGroupingCoversRequirements(cg) )       /* (11) */
    {
      switch( cg->variant )
      {
	case KHE_COMB_VARIANT_MIN:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    if( DEBUG3 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( DEBUG7 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	  }
	  break;

	case KHE_COMB_VARIANT_ZERO:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    if( DEBUG3 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( DEBUG7 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	    if( cg->curr_mg->cost == 0 )
	    {
	      /* new zero cost; save grouping and stop */
	      cg->zero_cost_groups_count++;
	      if( DEBUG3 )
		fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	      return true;
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SOLE_ZERO:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    if( DEBUG3 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( DEBUG7 )
	      KheMTaskGroupingDebug(cg->curr_mg, 1, 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	    if( cg->curr_mg->cost == 0 )
	    {
	      /* new zero cost; record its presence */
	      cg->zero_cost_groups_count++;
	      if( cg->zero_cost_groups_count > 1 )
	      {
		if( DEBUG3 )
		  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
		return true;
	      }
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SINGLES:

	  if( KheMTaskGroupingMTaskCount(cg->curr_mg) == 1 )
	  {
	    mt = KheMTaskSetMTask(cg->curr_mg->mtask_set, 0);
	    cg->singles_tasks_count += KheMTaskUnassignedTaskCount(mt);
	  }
	  break;

	default:

	  HnAbort("KheCombGrouperSolveFrom internal error (variant %d)",
	    cg->variant);
	  break;
      }
    }
  }
  else
  {
    /* try including curr_cmt */
    curr_cmt = HaArray(cg->comb_mtasks, curr_index);
    if( DEBUG3 )
      KheCombMTaskDebug(curr_cmt, 2, 2*curr_index + 4, stderr);
    if( KheCombGrouperAddCombMTask(cg, curr_cmt, curr_index) )
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      KheCombGrouperDeleteCombMTask(cg, curr_cmt);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }

    /* try excluding curr_cmt, unless it is the last cover for some reqt */
    if( !KheCombMTaskMustBeIncluded(curr_cmt) &&
        !KheSinglesRequiresExclude(cg) )              /* (8) */
    {
      stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
      if( stopping )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", 2*curr_index + 2, "");
  return false;
}


/* *** old version that iterated over cover_reqts
static bool KheCombGrouper SolveFrom(KHE_COMB_GROUPER cg, int curr_index)
{
  KHE_COVER_REQUIREMENT curr_e, prev_e;  int i;  KHE_COMB_MTASK cmt;
  KHE_MTASK mt;  bool try_excluding_curr_e, try_including_curr_e, stopping;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cg, %d):\n", 2*curr_index + 2, "",
      curr_index);
  if( curr_index >= HaArrayCount(cg->cover_reqts) )
  {
    ** end of recursion; if suitable mtasks, try them **
    if( KheCurrGroupingSatisfiesSinglesRequirement(cg) &&       ** (9) **
        KheCurrGroupingSatisfiesMTaskSetFnRequirement(cg) )     ** (10) **
    {
      ** verify that the mtasks correctly cover the cover_reqts **
      HnAssert(KheCombGrouperCurrGroupingCoversRequirements(cg),
	"KheCombGrou perSolveFrom internal error");

      switch( cg->variant )
      {
	case KHE_COMB_VARIANT_MIN:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	  }
	  break;

	case KHE_COMB_VARIANT_ZERO:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	    if( cg->curr_mg->cost == 0 )
	    {
	      ** new zero cost; save grouping and stop **
	      cg->zero_cost_groups_count++;
	      if( DEBUG3 )
		fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	      return true;
	      ** cg->stopping = true; **
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SOLE_ZERO:

          if( KheMTaskGroupingSetCost(cg->curr_mg) )
	  {
	    KheMTaskGroupingDebug(cg->curr_mg, 1, 2*curr_index + 4, stderr);
	    if( KheMTaskGroupingIsBetter(cg->curr_mg, cg->best_mg) )
	      KheMTaskGroupingCopy(cg->best_mg, cg->curr_mg);
	    if( cg->curr_mg->cost == 0 )
	    {
	      ** new zero cost; record its presence **
	      cg->zero_cost_groups_count++;
	      if( cg->zero_cost_groups_count > 1 )
	      {
		if( DEBUG3 )
		  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
		return true;
		** cg->stopping = true; **
	      }
	    }
	  }
	  break;

	case KHE_COMB_VARIANT_SINGLES:

	  if( KheMTaskGroupingMTaskCount(cg->curr_mg) == 1 )
	  {
	    mt = KheMTaskSetMTask(cg->curr_mg->mtask_set, 0);
	    cg->singles_tasks_count += KheMTaskUnassignedTaskCount(mt);
	  }
	  break;

	default:

	  HnAbort("KheCombGrou perSolveFrom internal error (variant %d)",
	    cg->variant);
	  break;
      }
    }
  }
  else
  {
    curr_e = HaArray(cg->cover_reqts, curr_index);
    if( DEBUG3 )
      KheCoverRequirementDebug(curr_e, 2, 2*curr_index + 4, stderr);

    ** work out whether to try excluding and including curr_e **
    switch( curr_e->cover_type )
    {
      case KHE_COMB_COVER_YES:

	try_excluding_curr_e = (curr_e->cover_count > 0);
	try_including_curr_e = (curr_e->cover_count == 0);
	break;

      case KHE_COMB_COVER_NO:

	try_excluding_curr_e = (curr_e->cover_count == 0);
	try_including_curr_e = false;
	break;

      case KHE_COMB_COVER_PREV:

	prev_e = HaArray(cg->cover_reqts, curr_index - 1);
	if( prev_e->cover_count == 0 )
	{
	  ** like KHE_COMB_COVER_NO for curr_e **
	  try_excluding_curr_e = (curr_e->cover_count == 0);
	  try_including_curr_e = false;
	}
	else
	{
	  ** like KHE_COMB_COVER_YES for curr_e **
	  try_excluding_curr_e = (curr_e->cover_count > 0);
	  try_including_curr_e = (curr_e->cover_count == 0);
	}
	break;

      case KHE_COMB_COVER_FREE:

	try_excluding_curr_e = true;
	try_including_curr_e = (curr_e->cover_count == 0);
	break;

      default:

	HnAbort("KheCombGro uperSolveFrom: unexpected cover_type (%d)",
	  curr_e->cover_type);
	try_excluding_curr_e = false;   ** keep compiler happy **
	try_including_curr_e = false;   ** keep compiler happy **
	break;
    }
    if( cg->variant == KHE_COMB_VARIANT_SINGLES &&        ** (8) **
        KheMTaskGroupingMTaskCount(cg->curr_mg) > 0 )
      try_including_curr_e = false;

    ** try excluding curr_e if requested **
    if( try_excluding_curr_e )
    {
      if( KheCombGroup erSolveFrom(cg, curr_index + 1) )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }

    ** try including curr_e if requested **
    if( try_including_curr_e )
      HaArrayForEach(curr_e->first_cover_comb_mtask, cmt, i)
      {
	if( KheCombGrouperAddCombMTask(cg, cmt, curr_index) )
	{
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "%*s  [ adding mtask:\n", 2*curr_index, "");
	    KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	  }
	  stopping = KheCombGrou perSolveFrom(cg, curr_index + 1);
	  KheCombGrouperDeleteCombMTask(cg, cmt);
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "%*s  ] removing mtask:\n", 2*curr_index, "");
	    KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	  }
	  if( stopping )
	  {
	    if( DEBUG3 )
	      fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	    return true;
	  }
	}
	else
	{
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "%*s  not adding mtask:\n", 2*curr_index, "");
	    KheCombMTaskDebug(cmt, 1, 2*curr_index + 2, stderr);
	  }
	}
      }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", 2*curr_index + 2, "");
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouperSolve(KHE_COMB_GROUPER cg, int max_num,                */
/*    KHE_COMB_VARIANT_TYPE cg_variant, KHE_SOLN_ADJUSTER sa,                */
/*    char *debug_str)                                                       */
/*                                                                           */
/*  Carry out a solve, assuming that the requirements are all set.           */
/*                                                                           */
/*****************************************************************************/

int KheCombGrouperSolve(KHE_COMB_GROUPER cg, int max_num,
  KHE_COMB_VARIANT_TYPE cg_variant, KHE_SOLN_ADJUSTER sa,
  char *debug_str)
{
  KHE_COVER_REQUIREMENT cr;  int i, j, res;
  KHE_MTASK mt;  KHE_COMB_MTASK cmt;  KHE_MTASK_SET mts;

  if( DEBUG2 )
  {
    if( max_num == INT_MAX )
      fprintf(stderr, "[ KheCombGrouperSolve(cg, %s, INT_MAX, \"%s\")\n",
	KheCombVariantShow(cg_variant), debug_str);
    else
      fprintf(stderr, "[ KheCombGrouperSolve(cg, %s, %d, \"%s\")\n",
	KheCombVariantShow(cg_variant), max_num, debug_str);
  }

  /* derived from the requirements; need resetting only when they are reset */
  if( !cg->derived_info_up_to_date )
  {
    /* initialize reqd_comb_mtasks */
    KheCombGrouperSetRequiredMTasks(cg);

    /* initialize comb_mtasks for this solve, first removing any old mtasks */
    HaArrayAppend(cg->comb_mtask_free_list, cg->comb_mtasks, i);
    HaArrayClear(cg->comb_mtasks);
    HaArrayForEach(cg->cover_reqts, cr, i)
      if( cr->cover_type != KHE_COMB_COVER_NO )
      {
	if( cr->time_group != NULL )
	{
	  /* cr is a time group */
	  mts = KheMTaskFinderMTasksInTimeGroup(cg->mtask_finder,
	    cg->resource_type, cr->time_group);
	  for( j = 0;  j < KheMTaskSetMTaskCount(mts);  j++ )
	  {
	    mt = KheMTaskSetMTask(mts, j);
	    if( KheCombGrouperMTaskIsWanted(cg, mt) )
	      HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, mt));
	  }
	  /* ***
	  for( j = 0;  j < KheTimeGroupTimeCount(cr->time_group);  j++ )
	  {
	    time = KheTimeGroupTime(cr->time_group, j);
	    count = KheMTaskFinderMTaskAtTimeCount(cg->mtask_finder, time);
	    for( k = 0;  k < count;  k++ )
	    {
	      mt = KheMTaskFinderMTaskAtTime(cg->mtask_finder, time, k);
	      if( KheCombGrouperMTaskIsWanted(cg, mt) )
		HaArrayAddLast(cg->comb_mtask, KheCombMTaskMake(cg, mt));
	    }
	  }
	  *** */
	}
	else
	{
	  /* cr is an mtask */
	  if( KheCombGrouperMTaskIsWanted(cg, cr->mtask) )
	    HaArrayAddLast(cg->comb_mtasks, KheCombMTaskMake(cg, cr->mtask));
	}
      }

    /* sort comb_mtasks into chronological order */
    HaArraySort(cg->comb_mtasks, &KheCombMTaskCmp);

    /* clear out last_cover_done fields */
    HaArrayForEach(cg->cover_reqts, cr, j)
      cr->last_cover_done = false;

    /* set last_cover_done and last_cover_reqts fields */
    HaArrayForEachReverse(cg->comb_mtasks, cmt, i)
      HaArrayForEach(cg->cover_reqts, cr, j)
        if( KheCoverRequirementCovered(cr, cmt->mtask) )
	{
	  /* add cr to cmt->cover_reqts */
	  HaArrayAddLast(cmt->cover_reqts, cr);

	  /* sort out last_cover */
	  if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
	  {
	    /* cmt is e's last cover */
            cr->last_cover_done = true;
	    HaArrayAddLast(cmt->last_cover_reqts, cr);
	  }
	}

    /* if any Yes cover requirement has no last cover, we can quit now */
    HaArrayForEach(cg->cover_reqts, cr, j)
      if( cr->cover_type == KHE_COMB_COVER_YES && !cr->last_cover_done )
      {
	if( DEBUG2 )
	{
	  KheCombGrouperDebug(cg, 2, 2, stderr);
	  fprintf(stderr, "  not covered: ");
	  KheCoverRequirementDebug(cr, 2, 0, stderr);
	  fprintf(stderr, "] KheCombGrouperSolve returning 0 (early)\n");
	}
	return 0;
      }

    /* initialize cg->busy_in to the mtasks' days plus one day each side */
    cg->busy_in = KheIntervalMake(1, 0);
    HaArrayForEach(cg->comb_mtasks, cmt, i)
      cg->busy_in = KheIntervalUnion(cg->busy_in, KheMTaskInterval(cmt->mtask));
    if( cg->busy_in.first > 0 )
      cg->busy_in.first -= 1;
    if( cg->busy_in.last < KheFrameTimeGroupCount(cg->days_frame) - 1 )
      cg->busy_in.last += 1;

    /* initialize first_time_index and last_time_index */
    /* ***
    cg->first_time_index = INT_MAX;
    cg->last_time_index = -1;
    HaArrayForEach(cg->cover_reqts, e, i)
      if( e->time_group != NULL )
      {
	tg_count = KheTimeGroupTimeCount(e->time_group);
	if( tg_count > 0 )
	{
	  index = KheTimeIndex(KheTimeGroupTime(e->time_group, 0));
	  if( index < cg->first_time_index )
	    cg->first_time_index = index;
	  index = KheTimeIndex(KheTimeGroupTime(e->time_group, tg_count - 1));
	  if( index > cg->last_time_index )
	    cg->last_time_index = index;
	}
      }
    *** */

    /* initialize first_frame_index and last_frame_index */
    /* ***
    ins = KheSolnInstance(cg->soln);
    time = KheInstanceTime(ins, cg->first_time_index);
    cg->first_frame_index = KheFrameTimeIndex(cg->days_frame, time);
    time = KheInstanceTime(ins, cg->last_time_index);
    cg->last_frame_index = KheFrameTimeIndex(cg->days_frame, time);
    *** */

    /* derived info is up to date now */
    cg->derived_info_up_to_date = true;
  }

  /* varying between and during solves */
  cg->variant = cg_variant;
  /* cg->debug_str = debug_str; */
  cg->group_monitor = NULL;
  cg->group_monitor_resource = NULL;
  KheMTaskGroupingClear(cg->curr_mg);
  KheMTaskGroupingClear(cg->best_mg);
  cg->zero_cost_groups_count = 0;
  cg->singles_tasks_count = 0;

  /* do the actual solve */
  HaArrayForEach(cg->cover_reqts, cr, i)
    HnAssert(cr->cover_count == 0,
      "KheCombGrouperSolve internal error 1 (index %d)\n", i);

  if( DEBUG2 )
    KheCombGrouperDebug(cg, 2, 2, stderr);
  KheCombGrouperSolveFrom(cg, 0);

  /* sort out the consequences */
  res = 0;
  switch( cg_variant )
  {
    case KHE_COMB_VARIANT_MIN:

      /* if there is a min, then do it */
      if( !KheMTaskGroupingIsEmpty(cg->best_mg) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (min)\n", max_num);
	res = KheMTaskGroupingExecute(cg->best_mg, max_num, sa, debug_str);
      }
      break;

    case KHE_COMB_VARIANT_ZERO:

      /* if there is a zero cost soln, then do it */
      if( cg->zero_cost_groups_count >= 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (zero cost)\n", max_num);
	res = KheMTaskGroupingExecute(cg->best_mg, max_num, sa, debug_str);
      }
      break;

    case KHE_COMB_VARIANT_SOLE_ZERO:

      /* if there is a single zero cost soln, then do it */
      if( cg->zero_cost_groups_count == 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (sole zero cost)\n", max_num);
	res = KheMTaskGroupingExecute(cg->best_mg, max_num, sa, debug_str);
      }
      break;

    case KHE_COMB_VARIANT_SINGLES:

      /* change nothing, but return the number of singles tasks found */
      res = cg->singles_tasks_count;
      break;

    default:

      HnAbort("KheCombGrouperSolve: unknown cg_variant (%d)", cg_variant);
      break;
  }

  /* tidy up and return */
  if( cg->group_monitor != NULL )
  {
    KheGroupMonitorDelete(cg->group_monitor);
    cg->group_monitor = NULL;
    cg->group_monitor_resource = NULL;
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheCombGrouperSolve returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouperSingleTasks(KHE_COMB_GROUPER cg)                       */
/*                                                                           */
/*  Return the number of individual tasks that satisfy cg's requirements.    */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn (KheCombGrouperSolve does it now)
int KheCombGrouperSingleTasks(KHE_COMB_GROUPER cg)
{
  KHE_COVER_REQUIREMENT e;  int i;

  ** initialize the "set at the start of each solve" fields **
  KheCombGrouperMakeCombMTaskReady(cg);

  ** initialize the "common to all algorithms" fields **
  cg->variant = KHE_COMB_VARIANT_SINGLES;
  ** cg->stopping = false; **

  ** initialize the "specific to KheCombGrouperSingleTasks" fields **
  cg->singles_tasks_count = 0;

  ** do the solve **
  HaArrayForEach(cg->cover_reqts, e, i)
    HnAssert(e->cover_count == 0,
      "KheCombGrouperSolve internal error 1 (index %d)\n", i);
  KheCombGrouperSolveFrom(cg, 0);

  ** tidy up and return **
  return cg->singles_tasks_count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,             */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDebug(KHE_COMB_GROUPER cg, int verbosity,
  int indent, FILE *fp)
{
  KHE_COVER_REQUIREMENT cr;  int i, pos;  KHE_COMB_MTASK cmt;
  fprintf(fp, "%*s[ CombGrouper(%s %s, busy_in %s, no_singles %s, domain %s)\n",
    indent, "", KheInstanceId(KheSolnInstance(cg->soln)),
    KheResourceTypeId(cg->resource_type),
    KheIntervalShow(cg->busy_in, cg->days_frame),
    bool_show(cg->no_singles), cg->preferred_domain != NULL ?
      KheResourceGroupId(cg->preferred_domain) : "-");
  HaArrayForEach(cg->cover_reqts, cr, i)
    KheCoverRequirementDebug(cr, verbosity, indent + 2, fp);
  fprintf(fp, "%*s------------\n", indent + 2, "");
  HaArrayForEach(cg->comb_mtasks, cmt, i)
  {
    fprintf(fp, "%*s", indent + 2, "");
    if( KheMTaskSetContainsMTask(cg->curr_mg->mtask_set, cmt->mtask, &pos) )
      fprintf(fp, "** ");
    else
      fprintf(fp, "   ");
    KheCombMTaskDebug(cmt, verbosity, 0, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}

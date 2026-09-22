
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
#define DEBUG4 0	/* KheCombGrouperAddMTaskInfo */
#define DEBUG5 0	/* adding requirements */


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_COVER_TYPE - type of cover for element                     */
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
/*  Type KHE_ELEMENT                                                         */
/*                                                                           */
/*  One element (one time group or mtask), including how many times it has   */
/*  been covered.                                                            */
/*                                                                           */
/*  Array first_cover_mtask_info contains those mtasks which cover this      */
/*  element but do not cover any previous element.  The solver will try      */
/*  to use these mtasks to cover this element.                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_info_rec *KHE_MTASK_INFO;
typedef HA_ARRAY(KHE_MTASK_INFO) ARRAY_KHE_MTASK_INFO;

typedef struct khe_element_rec {

  /* constant during one solve */
  KHE_TIME_GROUP		time_group;	/* if element is a time group*/
  KHE_MTASK			mtask;		/* if element is am mtask    */
  KHE_TIME_SET			time_set;
  ARRAY_KHE_MTASK_INFO		first_cover_mtask_info;
  KHE_COMB_COVER_TYPE		cover_type;

  /* varying during one solve */
  int				cover_count;
} *KHE_ELEMENT;

typedef HA_ARRAY(KHE_ELEMENT) ARRAY_KHE_ELEMENT;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MTASK_INFO                                                      */
/*                                                                           */
/*  Information about one mtask, including the elements it covers.           */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_info_rec {

  /* constant during one solve */
  KHE_MTASK			mtask;
  ARRAY_KHE_ELEMENT		elements;
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_COMB_GROUPER - solver for combinatorial grouping                */
/*                                                                           */
/*****************************************************************************/

struct khe_comb_grouper_rec {

  /* free lists */
  ARRAY_KHE_ELEMENT		element_free_list;
  ARRAY_KHE_MTASK_INFO		mtask_info_free_list;

  /* constant for the lifetime of the solver */
  HA_ARENA			arena;
  KHE_MTASK_FINDER		mtask_finder;
  KHE_SOLN			soln;
  KHE_FRAME			days_frame;

  /* requirements, may vary freely but always defined */
  ARRAY_KHE_ELEMENT		elements;
  KHE_COMB_MTASK_FN		mtask_fn;
  void				*mtask_fn_impl;
  KHE_COMB_MTASK_SET_FN		mtask_set_fn;
  void				*mtask_set_fn_impl;
  bool				no_singles;
  /* ***
  KHE_PROFILE_TIME_GROUP	profile_tg;
  KHE_RESOURCE_GROUP		profile_domain;
  bool				profile_max_len_on;
  *** */

  /* derived from the requirements; need resetting only when they are reset */
  bool				derived_info_up_to_date;
  ARRAY_KHE_MTASK_INFO		mtask_info;
  KHE_INTERVAL			busy_in;	/* days selected mtasks busy */
  /* ***
  int				first_time_index;
  int				last_time_index;
  int				first_frame_index;
  int				last_frame_index;
  *** */

  /* varying between and during solves */
  KHE_COMB_VARIANT_TYPE		variant;
  char				*debug_str;
  KHE_GROUP_MONITOR		group_monitor;
  KHE_RESOURCE			group_monitor_resource;
  KHE_MTASK_GROUPING		curr_mg;
  KHE_MTASK_GROUPING		best_mg;
  int				zero_cost_groups;
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
    case KHE_COMB_COVER_YES:	return "cover_yes";
    case KHE_COMB_COVER_NO:	return "cover_no";
    case KHE_COMB_COVER_PREV:	return "cover_prev";
    case KHE_COMB_COVER_FREE:	return "cover_free";

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
/*  leader task and, add mt to mg and return true.  Otherwise leave mg       */
/*  unchanged and return false.                                              */
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
/*  Delete mt from mg.                                                       */
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
  /*  for( i = cg->first_frame_index;  i <= cg->last_frame_index;  i++ )  */
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

  /* *** still to do
  KHE_MTASK mt;  int i;  KHE_RESOURCE_GROUP domain;
  still to do here mt = KheTaskerGroupingMTask(cg->tasker, 0);
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
/*  Set mg->cost.  This involves assigning any suitable resource to mg.  If  */
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
/*  int KheMTaskSetMinUnassignedCount(KHE_MTASK_SET mts)                     */
/*                                                                           */
/*  Return the minimum, over the mtasks mt of mts, of the number of          */
/*  unassigned tasks in mt.                                                  */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskSetMinUnassignedCount(KHE_MTASK_SET mts)
{
  int res, val, i;  KHE_MTASK mt;
  res = INT_MAX;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    val = KheMTaskUnassignedTaskCount(mt);
    if( val < res )
      res = val;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupingMakeOneGroup(KHE_MTASK_GROUPING mg)                 */
/*                                                                           */
/*  Make one group out of the mtasks of mg.                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskGroupingMakeOneGroup(KHE_MTASK_GROUPING mg)
{
  KHE_MTASK leader_mt, mt;  KHE_TASK leader_task, task;  int i;
  KHE_COST non_asst_cost, asst_cost;  KHE_MTASK_FINDER mtf;

  /* find the leader task */
  leader_mt = HaArrayLast(mg->leader_mtasks);
  HnAssert(KheMTaskUnassignedTaskCount(leader_mt) > 0,
    "KheMTaskGroupingMakeOneGroup internal error 1");
  leader_task = KheMTaskTask(leader_mt, KheMTaskAssignedTaskCount(leader_mt),
    &non_asst_cost, &asst_cost);
  mtf = mg->grouper->mtask_finder;

  /* build the group */
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
	return false;
    }
  }
  KheMTaskFinderGroupEnd(mtf);

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGroupingExecute(KHE_MTASK_GROUPING mg, int max_num)          */
/*                                                                           */
/*  Carry out a grouping of the mtasks of mg.  Make as many groups as        */
/*  possible, but not more than max_num.                                     */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskGroupingExecute(KHE_MTASK_GROUPING mg, int max_num)
{
  int num, res;
  res = 0;
  if( KheMTaskSetMTaskCount(mg->mtask_set) >= 2 )
  {
    /* set num to the number of groups to make */
    num = KheMTaskSetMinUnassignedCount(mg->mtask_set);
    if( max_num < num )
      num = max_num;

    /* make up to num groups */
    for( res = 0;  res < num;  res++ )
      if( !KheMTaskGroupingMakeOneGroup(mg) )
	break;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupingDebug(KHE_MTASK_GROUPING mg, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupingDebug(KHE_MTASK_GROUPING mg, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_MTASK mt;
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
    KheMTaskDebug(mt, verbosity, -1, fp);
    if( mt == HaArrayLast(mg->leader_mtasks) )
      fprintf(fp, "|");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ELEMENT"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ELEMENT KheElementMake(KHE_COMB_GROUPER cg, KHE_TIME_GROUP tg,       */
/*    KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)                               */
/*                                                                           */
/*  Make an element object.                                                  */
/*                                                                           */
/*****************************************************************************/

static KHE_ELEMENT KheElementMake(KHE_COMB_GROUPER cg, KHE_TIME_GROUP tg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_ELEMENT res;
  if( HaArrayCount(cg->element_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->element_free_list);
    KheTimeSetClear(res->time_set);
    HaArrayClear(res->first_cover_mtask_info);
  }
  else
  {
    HaMake(res, cg->arena);
    res->time_set = KheTimeSetMake(KheSolnInstance(cg->soln), cg->arena);
    HaArrayInit(res->first_cover_mtask_info, cg->arena);
  }

  /* constant during one solve */
  res->time_group = tg;
  res->mtask = mt;
  if( tg != NULL )
    KheTimeSetAddTimeGroup(res->time_set, tg);
  res->cover_type = cover;

  /* varying during one solve */
  res->cover_count = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheElementCovered(KHE_ELEMENT e, KHE_MTASK mt)                      */
/*                                                                           */
/*  Return true if e is covered by mt.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheElementCovered(KHE_ELEMENT e, KHE_MTASK mt)
{
  if( e->time_group != NULL )
    return !KheTimeSetDisjoint(e->time_set, KheMTaskTimeSet(mt));
  else
    return e->mtask == mt;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheElementDebug(KHE_ELEMENT e, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of e onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/
static void KheMTaskInfoDebug(KHE_MTASK_INFO mti, int verbosity,
  int indent, FILE *fp);

static void KheElementDebug(KHE_ELEMENT e, int verbosity, int indent, FILE *fp)
{
  KHE_MTASK_INFO mti;  int i;
  fprintf(fp, "%*s[ Element(%s, %s, %d covers)\n", indent, "",
    e->time_group != NULL ? KheTimeGroupId(e->time_group) : "mtask",
    CoverTypeShow(e->cover_type), e->cover_count);
  HaArrayForEach(e->first_cover_mtask_info, mti, i)
    KheMTaskInfoDebug(mti, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_INFO"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_INFO KheMTaskInfoMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)       */
/*                                                                           */
/*  Make an mtask info object.                                               */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_INFO KheMTaskInfoMake(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_MTASK_INFO res;
  if( HaArrayCount(cg->mtask_info_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(cg->mtask_info_free_list);
    HaArrayClear(res->elements);
  }
  else
  {
    HaMake(res, cg->arena);
    HaArrayInit(res->elements, cg->arena);
  }
  res->mtask = mt;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskInfoDebug(KHE_MTASK_INFO mti, int verbosity,                */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mti onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskInfoDebug(KHE_MTASK_INFO mti, int verbosity,
  int indent, FILE *fp)
{
  fprintf(fp, "%*s[ MTaskInfo (%d elements)\n", indent, "",
    HaArrayCount(mti->elements));
  KheMTaskDebug(mti->mtask, 1, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_COMB_GROUPER - construction"                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf, HA_ARENA a)    */
/*                                                                           */
/*  Make a new comb solver based on mtf.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_COMB_GROUPER KheCombGrouperMake(KHE_MTASK_FINDER mtf, HA_ARENA a)
{
  KHE_COMB_GROUPER res;

  /* free lists */
  HaMake(res, a);
  HaArrayInit(res->element_free_list, a);
  HaArrayInit(res->mtask_info_free_list, a);

  /* constant for the lifetime of the solver */
  res->arena = a;
  res->mtask_finder = mtf;
  res->soln = KheMTaskFinderSoln(mtf);
  res->days_frame = KheMTaskFinderDaysFrame(mtf);

  /* requirements, may vary freely but always defined */
  HaArrayInit(res->elements, a);
  res->mtask_fn = NULL;
  res->mtask_fn_impl = NULL;
  res->mtask_set_fn = NULL;
  res->mtask_set_fn_impl = NULL;
  res->no_singles = false;
  /* ***
  res->profile_tg = NULL;
  res->profile_domain = NULL;
  res->profile_max_len_on = false;
  *** */

  /* derived from the requirements; need resetting only when they are reset */
  res->derived_info_up_to_date = false;
  HaArrayInit(res->mtask_info, a);
  res->busy_in = KheIntervalMake(1, 0);
  /* ***
  res->first_time_index = 0;
  res->last_time_index = 0;
  res->first_frame_index = 0;
  res->last_frame_index = 0;
  *** */

  /* varying between and during solves */
  res->variant = KHE_COMB_VARIANT_SINGLES;
  res->debug_str = NULL;
  res->group_monitor = NULL;
  res->group_monitor_resource = NULL;
  res->curr_mg = KheMTaskGroupingMake(res);
  res->best_mg = KheMTaskGroupingMake(res);
  res->zero_cost_groups = 0;
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
/*  void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)                */
/*                                                                           */
/*  Clear away all requirements.                                             */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperClearRequirements(KHE_COMB_GROUPER cg)
{
  int i;
  HaArrayAppend(cg->element_free_list, cg->elements, i);
  HaArrayClear(cg->elements);
  cg->mtask_fn = NULL;
  cg->mtask_fn_impl = NULL;
  cg->mtask_set_fn = NULL;
  cg->mtask_set_fn_impl = NULL;
  /* ***
  cg->profile_tg = NULL;
  cg->profile_domain = NULL;
  cg->profile_max_len_on = false;
  *** */
  cg->no_singles = false;
  cg->derived_info_up_to_date = false;
  if( DEBUG5 )
    fprintf(stderr, "[ KheCombGrouperClearRequirements(cg) ]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)                          */
/*                                                                           */
/*  Add a time group element to the solver.                                  */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddTimeGroupRequirement(KHE_COMB_GROUPER cg,
  KHE_TIME_GROUP tg, KHE_COMB_COVER_TYPE cover)
{
  KHE_ELEMENT res;
  res = KheElementMake(cg, tg, NULL, cover);
  HaArrayAddLast(cg->elements, res);
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
  KHE_ELEMENT e;  int i;
  HaArrayForEach(cg->elements, e, i)
    if( e->time_group == tg )
    {
      HaArrayDeleteAndShift(cg->elements, i);
      HaArrayAddLast(cg->element_free_list, e);
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
/*  Add a mtask element to the solver.                                       */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperAddMTaskRequirement(KHE_COMB_GROUPER cg,
  KHE_MTASK mt, KHE_COMB_COVER_TYPE cover)
{
  KHE_ELEMENT res;
  res = KheElementMake(cg, NULL, mt, cover);
  HaArrayAddLast(cg->elements, res);
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
  KHE_ELEMENT e;  int i;
  HaArrayForEach(cg->elements, e, i)
    if( e->mtask == mt )
    {
      HaArrayDeleteAndShift(cg->elements, i);
      HaArrayAddLast(cg->element_free_list, e);
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

void KheCombGrouperAddMTaskFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_FN mtask_fn, void *impl)
{
  HnAssert(cg->mtask_fn == NULL, "KheCombGrouperAddMTaskFnRequirement "
    "internal error (mtask_fn already set)");
  cg->mtask_fn = mtask_fn;
  cg->mtask_fn_impl = impl;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskFnRequirement(KHE_COMB_GROUPER cg)         */
/*                                                                           */
/*  Delete any mtask function requirement from cg.                           */
/*                                                                           */
/*****************************************************************************/

void KheCombGrouperDeleteMTaskFnRequirement(KHE_COMB_GROUPER cg)
{
  cg->mtask_fn = NULL;
  cg->mtask_fn_impl = NULL;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverAddMTaskSetFnRequirement(KHE_COMB_GROUPER cg,          */
/*    KHE_COMB_MTASK_SET_FN mtask_set_fn, void *impl)                        */
/*                                                                           */
/*  Add an mtask set function requirement to cg.                             */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverAddMTaskSetFnRequirement(KHE_COMB_GROUPER cg,
  KHE_COMB_MTASK_SET_FN mtask_set_fn, void *impl)
{
  HnAssert(cg->mtask_set_fn == NULL, "KheCombGrouperAddMTaskSetFnRequirement "
    "internal error (mtask_set_fn already set)");
  cg->mtask_set_fn = mtask_set_fn;
  cg->mtask_set_fn_impl = impl;
  cg->derived_info_up_to_date = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombSolverDeleteMTaskSetFnRequirement(KHE_COMB_GROUPER cg)       */
/*                                                                           */
/*  Delete any mtask set function requirement to cg.                         */
/*                                                                           */
/*****************************************************************************/

void KheCombSolverDeleteMTaskSetFnRequirement(KHE_COMB_GROUPER cg)
{
  cg->mtask_set_fn = NULL;
  cg->mtask_set_fn_impl = NULL;
  cg->derived_info_up_to_date = false;
}


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
/*  Submodule "KHE_COMB_GROUPER - setting up for solving"                    */
/*                                                                           */
/*****************************************************************************/

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
  KHE_ELEMENT e;  int i;  KHE_MTASK_INFO mti;

  /* if mt has already been chosen, we don't want it again */
  HaArrayForEach(cg->mtask_info, mti, i)
    if( mti->mtask == mt )
      return false;

  /* if mt covers any No time group, we don't want it (2) */
  HaArrayForEach(cg->elements, e, i)
    if( e->cover_type == KHE_COMB_COVER_NO && KheElementCovered(e, mt) )
      return false;

  /* if mt's tasks are fixed, we don't want it (3) */
  if( KheMTaskAssignIsFixed(mt) )
    return false;

  /* if mt has no unassigned tasks, we don't want it (4)  */
  if( KheMTaskUnassignedTaskCount(mt) == 0 )
    return false;

  /* if domain is wanted but c doesn't have it, we don't want c */
  /* ***
  if( domain != NULL && !KheResourceGroupEqual(domain,KheTaskerMTaskDomain(c)) )
    return false;
  *** */

  /* if mtask_fn is set, check that requirement (5) */
  if( cg->mtask_fn != NULL && !cg->mtask_fn(mt, cg->mtask_fn_impl) )
    return false;

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
/*  object mti for it and add the elements covered by mt to mti.  Also       */
/*  add mti to the first element it covers.                                  */
/*                                                                           */
/*  Only do this if mt actually covers something.  But it will.              */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperAddMTask(KHE_COMB_GROUPER cg, KHE_MTASK mt)
{
  KHE_MTASK_INFO mti;  KHE_ELEMENT e;  int i;
  mti = NULL;
  HaArrayForEach(cg->elements, e, i)
  {
    HnAssert(e->cover_type != KHE_COMB_COVER_NO,
      "KheCombGrouperAddMTask internal error");
    if( KheElementCovered(e, mt) )
    {
      if( mti == NULL )
      {
	mti = KheMTaskInfoMake(cg, mt);
	HaArrayAddLast(cg->mtask_info, mti);
	HaArrayAddLast(e->first_cover_mtask_info, mti);
      }
      HaArrayAddLast(mti->elements, e);
    }
  }
  HnAssert(mti != NULL, "KheCombGrouperAddMTask internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperMakeMTaskInfoReady(KHE_COMB_GROUPER cg)               */
/*                                                                           */
/*  Make mtask info, ready for solving.  This finds the mtasks that lie in   */
/*  the search space, and sets cg->first_time_index and cg->last_time_index. */
/*                                                                           */
/*****************************************************************************/

/* *** inlined now
static void KheCombGrouperMakeMTaskInfoReady(KHE_COMB_GROUPER cg)
{
  KHE_ELEMENT e;  int i, j, k, tg_count, index, count;  KHE_INSTANCE ins;
  KHE_MTASK mt;  KHE_TIME time;

  if( !cg->derived_info_up_to_date )
  {
    ** initialize mtask_info for this solve, first removing any old mtasks **
    HaArrayAppend(cg->mtask_info_free_list, cg->mtask_info, i);
    HaArrayClear(cg->mtask_info);
    HaArrayForEach(cg->elements, e, i)
      if( e->cover_type != KHE_COMB_COVER_NO )
      {
	if( e->time_group != NULL )
	{
	  ** element is a time group **
	  for( j = 0;  j < KheTimeGroupTimeCount(e->time_group);  j++ )
	  {
	    time = KheTimeGroupTime(e->time_group, j);
	    count = KheMTaskFinderMTaskAtTimeCount(cg->mtask_finder, time);
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
	  ** element is an mtask **
	  if( KheCombGrouperMTaskIsWanted(cg, e->mtask) )
	    KheCombGrouperAddMTask(cg, e->mtask);
	}
      }

    ** initialize first_time_index and last_time_index **
    cg->first_time_index = INT_MAX;
    cg->last_time_index = -1;
    HaArrayForEach(cg->elements, e, i)
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
/*  bool KheCombGrouperAddMTaskInfo(KHE_COMB_GROUPER cg, KHE_MTASK_INFO mti) */
/*                                                                           */
/*  Add mti and return true, or return false and change nothing if adding    */
/*  mti covers an element twice or does not work with the current grouping.  */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperAddMTaskInfo(KHE_COMB_GROUPER cg, KHE_MTASK_INFO mti,
  int curr_index)
{
  KHE_ELEMENT e;  int i;

  /* make sure that no element would be covered twice if mti were added */
  /* *** we may not need this now that we are checking intervals
  HaArrayForEach(mti->elements, e, i)
    if( e->cover_count > 0 )
    {
      if( DEBUG4 )
      {
	fprintf(stderr, "%*s  add mtask info failed on:\n", 2*curr_index, "");
	KheElementDebug(e, 1, 2*curr_index + 2, stderr);
      }
      return false;
    }
  *** */

  /* make sure that mti's mtask is acceptable to the current grouping */
  if( !KheMTaskGroupingAddMTask(cg->curr_mg, mti->mtask) )    /* (6), (7) */
  /* if( !KheTaskFinderGroupAddTask(cg->mtask_finder, mti->mtask) ) */
  {
    if( DEBUG4 )
      fprintf(stderr, "%*s  add mtask info failed on grouping (%d mtasks)\n",
	2*curr_index, "", /* KheTaskerGroupingMTaskCount(cg->tasker) */
	KheMTaskGroupingMTaskCount(cg->curr_mg));
    return false;
  }

  /* all good, update the cover counts and return true */
  HaArrayForEach(mti->elements, e, i)
    e->cover_count++;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperDeleteMTaskInfo(KHE_COMB_GROUPER cg,                  */
/*    KHE_MTASK_INFO mti)                                                    */
/*                                                                           */
/*  Undo the corresponding call to KheCombGrouperAddMTaskInfo.               */
/*                                                                           */
/*****************************************************************************/

static void KheCombGrouperDeleteMTaskInfo(KHE_COMB_GROUPER cg,
  KHE_MTASK_INFO mti)
{
  KHE_ELEMENT e;  int i;
  HaArrayForEach(mti->elements, e, i)
    e->cover_count--;
  KheMTaskGroupingDeleteMTask(cg->curr_mg, mti->mtask);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheCombGrouperMTaskSetHasValidCover(KHE_COMB_GROUPER cg)            */
/*                                                                           */
/*  Return true if cg's current set of mtasks has valid cover.               */
/*                                                                           */
/*  This function is used for checking only.                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperMTaskSetHasValidCover(KHE_COMB_GROUPER cg)
{
  KHE_ELEMENT e, prev_e;  int i;
  prev_e = NULL;
  HaArrayForEach(cg->elements, e, i)
  {
    HnAssert(e->cover_count <= 1, "KheCombGrouperMTaskSetIsValid: 1");
    switch( e->cover_type )
    {
      case KHE_COMB_COVER_YES:

	/* should be covered once */
	if( e->cover_count != 1 )
	  return false;
	break;

      case KHE_COMB_COVER_NO:

	/* should be not covered */
	HnAssert(e->cover_count == 0, "KheCombGrouperMTaskSetIsValid: 2");
	break;

      case KHE_COMB_COVER_PREV:

	/* should be covered if and only if prev is covered */
	if( prev_e != NULL && e->cover_count != prev_e->cover_count )
	  return false;
	break;

      case KHE_COMB_COVER_FREE:

	/* 0 or 1 is fine here */
	break;

      default:

	HnAbort("KheCombGrouperMTaskSetIsValid: invalid cover (%d)",
	  e->cover_type);
    }
    prev_e = e;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTry(KHE_COMB_GROUPER cg, int curr_index, KHE_ELEMENT curr_e,     */
/*    bool try_nothing, bool try_something)                                  */
/*                                                                           */
/*  Handle curr_e at curr_index.  If try_nothing is true, try assigning      */
/*  nothing here; if try_something is true, try assigning a mtask here.      */
/*                                                                           */
/*****************************************************************************/

/* *** inlined now
static void KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index);

static void KheTry(KHE_COMB_GROUPER cg, int curr_index, KHE_ELEMENT curr_e,
  bool try_nothing, bool try_something)
{
  KHE_MTASK_INFO mti;  int i;

  ** try nothing if requested **
  if( try_nothing )
  {
    KheCombGrouperSolveFrom(cg, curr_index + 1);
    if( cg->stopping )
      return;
  }

  ** try something if requested and it would not violate singles limit **
  if( try_something && (cg->alg != KHE_COMB_SINGLES ||
	KheMTaskGroupingMTaskCount(cg->curr_mg) == 0) )
    HaArrayForEach(curr_e->first_cover_mtask_info, mti, i)
    {
      if( KheCombGrouperAddMTaskInfo(cg, mti, curr_index) )
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  [ adding mtask:\n", 2*curr_index, "");
	  KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
	}
	KheCombGrouperSolveFrom(cg, curr_index + 1);
	KheCombGrouperDeleteMTaskInfo(cg, mti);
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  ] removing mtask:\n", 2*curr_index, "");
	  KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
	}
	if( cg->stopping )
	  return;
      }
      else
      {
	if( DEBUG3 )
	{
	  fprintf(stderr, "%*s  not adding mtask:\n", 2*curr_index, "");
	  KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
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

static bool KheCurrGroupingSatisfiesMTaskSetFnRequirement(KHE_COMB_GROUPER cg)
{
  return cg->mtask_set_fn == NULL ||
    cg->mtask_set_fn(cg->curr_mg->mtask_set, cg->mtask_set_fn_impl);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)        */
/*                                                                           */
/*  Solve cg from cg->elements[curr_index] onwards.  Return true if it's     */
/*  time to stop.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheCombGrouperSolveFrom(KHE_COMB_GROUPER cg, int curr_index)
{
  KHE_MTASK mt;  KHE_ELEMENT curr_e, prev_e;  int i;  KHE_MTASK_INFO mti;
  bool try_excluding_curr_e, try_including_curr_e, stopping;
  if( DEBUG3 )
    fprintf(stderr, "%*s[ SolveFrom(cg, %d):\n", 2*curr_index + 2, "",
      curr_index);
  if( curr_index >= HaArrayCount(cg->elements) )
  {
    /* end of recursion; if suitable mtasks, try them */
    if( KheCurrGroupingSatisfiesSinglesRequirement(cg) &&       /* (9) */
        KheCurrGroupingSatisfiesMTaskSetFnRequirement(cg) )     /* (10) */
    {
      /* verify that the mtasks correctly cover the elements */
      HnAssert(KheCombGrouperMTaskSetHasValidCover(cg),
	"KheCombGrouperSolveFrom internal error");

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
	      /* new zero cost; save grouping and stop */
	      cg->zero_cost_groups++;
	      if( DEBUG3 )
		fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	      return true;
	      /* cg->stopping = true; */
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
	      /* new zero cost; record its presence */
	      cg->zero_cost_groups++;
	      if( cg->zero_cost_groups > 1 )
	      {
		if( DEBUG3 )
		  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
		return true;
		/* cg->stopping = true; */
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
    curr_e = HaArray(cg->elements, curr_index);
    if( DEBUG3 )
      KheElementDebug(curr_e, 2, 2*curr_index + 4, stderr);

    /* work out whether to try excluding and including curr_e */
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

	prev_e = HaArray(cg->elements, curr_index - 1);
	if( prev_e->cover_count == 0 )
	{
	  /* like KHE_COMB_COVER_NO for curr_e */
	  try_excluding_curr_e = (curr_e->cover_count == 0);
	  try_including_curr_e = false;
	}
	else
	{
	  /* like KHE_COMB_COVER_YES for curr_e */
	  try_excluding_curr_e = (curr_e->cover_count > 0);
	  try_including_curr_e = (curr_e->cover_count == 0);
	}
	break;

      case KHE_COMB_COVER_FREE:

	try_excluding_curr_e = true;
	try_including_curr_e = (curr_e->cover_count == 0);
	break;

      default:

	HnAbort("KheCombGrouperSolveFrom: unexpected cover_type (%d)",
	  curr_e->cover_type);
	try_excluding_curr_e = false;   /* keep compiler happy */
	try_including_curr_e = false;   /* keep compiler happy */
	break;
    }
    if( cg->variant == KHE_COMB_VARIANT_SINGLES &&        /* (8) */
        KheMTaskGroupingMTaskCount(cg->curr_mg) > 0 )
      try_including_curr_e = false;

    /* try excluding curr_e if requested */
    if( try_excluding_curr_e )
    {
      if( KheCombGrouperSolveFrom(cg, curr_index + 1) )
      {
	if( DEBUG3 )
	  fprintf(stderr, "%*s] stopping\n", 2*curr_index + 2, "");
	return true;
      }
    }

    /* try including curr_e if requested */
    if( try_including_curr_e )
      HaArrayForEach(curr_e->first_cover_mtask_info, mti, i)
      {
	if( KheCombGrouperAddMTaskInfo(cg, mti, curr_index) )
	{
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "%*s  [ adding mtask:\n", 2*curr_index, "");
	    KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
	  }
	  stopping = KheCombGrouperSolveFrom(cg, curr_index + 1);
	  KheCombGrouperDeleteMTaskInfo(cg, mti);
	  if( DEBUG3 )
	  {
	    fprintf(stderr, "%*s  ] removing mtask:\n", 2*curr_index, "");
	    KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
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
	    KheMTaskInfoDebug(mti, 1, 2*curr_index + 2, stderr);
	  }
	}
      }
  }
  if( DEBUG3 )
    fprintf(stderr, "%*s]\n", 2*curr_index + 2, "");
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCombGrouperSolve(KHE_COMB_GROUPER cg, int max_num,                */
/*    KHE_COMB_VARIANT_TYPE cg_variant, char *debug_str)                     */
/*                                                                           */
/*  Carry out a solve, assuming that the requirements are all set.           */
/*                                                                           */
/*****************************************************************************/

int KheCombGrouperSolve(KHE_COMB_GROUPER cg, int max_num,
  KHE_COMB_VARIANT_TYPE cg_variant, char *debug_str)
{
  KHE_ELEMENT e;  int i, j, k, count, res;
  KHE_MTASK mt;  KHE_TIME time;  KHE_MTASK_INFO mti;

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheCombGrouperSolve(cg, %s, %d, \"%s\")\n",
      KheCombVariantShow(cg_variant), max_num, debug_str);
    KheCombGrouperDebug(cg, 2, 2, stderr);
  }

  /* derived from the requirements; need resetting only when they are reset */
  if( !cg->derived_info_up_to_date )
  {
    /* initialize mtask_info for this solve, first removing any old mtasks */
    HaArrayAppend(cg->mtask_info_free_list, cg->mtask_info, i);
    HaArrayClear(cg->mtask_info);
    HaArrayForEach(cg->elements, e, i)
      if( e->cover_type != KHE_COMB_COVER_NO )
      {
	if( e->time_group != NULL )
	{
	  /* element is a time group */
	  for( j = 0;  j < KheTimeGroupTimeCount(e->time_group);  j++ )
	  {
	    time = KheTimeGroupTime(e->time_group, j);
	    count = KheMTaskFinderMTaskAtTimeCount(cg->mtask_finder, time);
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
	  /* element is an mtask */
	  if( KheCombGrouperMTaskIsWanted(cg, e->mtask) )
	    KheCombGrouperAddMTask(cg, e->mtask);
	}
      }

    /* initialize cg->busy_in */
    cg->busy_in = KheIntervalMake(1, 0);
    HaArrayForEach(cg->mtask_info, mti, i)
      cg->busy_in = KheIntervalUnion(cg->busy_in, KheMTaskInterval(mti->mtask));

    /* initialize first_time_index and last_time_index */
    /* ***
    cg->first_time_index = INT_MAX;
    cg->last_time_index = -1;
    HaArrayForEach(cg->elements, e, i)
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
  cg->debug_str = debug_str;
  cg->group_monitor = NULL;
  cg->group_monitor_resource = NULL;
  KheMTaskGroupingClear(cg->curr_mg);
  KheMTaskGroupingClear(cg->best_mg);
  cg->zero_cost_groups = 0;
  cg->singles_tasks_count = 0;

  /* do the actual solve */
  HaArrayForEach(cg->elements, e, i)
    HnAssert(e->cover_count == 0,
      "KheCombGrouperSolve internal error 1 (index %d)\n", i);
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
	res = KheMTaskGroupingExecute(cg->best_mg, max_num);
      }
      break;

    case KHE_COMB_VARIANT_ZERO:

      /* if there is a zero cost soln, then do it */
      if( cg->zero_cost_groups >= 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (zero cost)\n", max_num);
	res = KheMTaskGroupingExecute(cg->best_mg, max_num);
      }
      break;

    case KHE_COMB_VARIANT_SOLE_ZERO:

      /* if there is a single zero cost soln, then do it */
      if( cg->zero_cost_groups == 1 )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  grouping %d new best (sole zero cost)\n", max_num);
	res = KheMTaskGroupingExecute(cg->best_mg, max_num);
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
  KHE_ELEMENT e;  int i;

  ** initialize the "set at the start of each solve" fields **
  KheCombGrouperMakeMTaskInfoReady(cg);

  ** initialize the "common to all algorithms" fields **
  cg->variant = KHE_COMB_VARIANT_SINGLES;
  ** cg->stopping = false; **

  ** initialize the "specific to KheCombGrouperSingleTasks" fields **
  cg->singles_tasks_count = 0;

  ** do the solve **
  HaArrayForEach(cg->elements, e, i)
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
  KHE_ELEMENT e;  int i;
  fprintf(fp, "%*s[ CombGrouper(%s, type %s): ", indent, "",
    KheInstanceId(KheSolnInstance(cg->soln)),
    KheResourceTypeId(KheMTaskFinderResourceType(cg->mtask_finder)));
  HaArrayForEach(cg->elements, e, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    if( e->time_group != NULL )
      fprintf(fp, "%s", KheTimeGroupId(e->time_group));
    else
      fprintf(fp, "mtask");
  }
  fprintf(fp, "\n");
  fprintf(stderr, "%*s  frame time groups are %s\n", indent, "",
    KheIntervalShow(cg->busy_in, cg->days_frame));
    /* ***
    KheTimeGroupId(KheFrameTimeGroup(cg->days_frame, cg->first_frame_index)),
    KheTimeGroupId(KheFrameTimeGroup(cg->days_frame, cg->last_frame_index));
    *** */
  HaArrayForEach(cg->elements, e, i)
    KheElementDebug(e, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}

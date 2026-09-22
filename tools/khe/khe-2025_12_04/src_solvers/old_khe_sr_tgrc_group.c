
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
/*  FILE:         khe_sr_tgrc_group.c                                        */
/*  DESCRIPTION:  Task grouping by resource constraints - mtask groups       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG1 1	/* groups with debug label */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST_STATE - state of the cost of an mtask group                     */
/*                                                                           */
/*  KHE_COST_UNKNOWN                                                         */
/*    The cost is unknown and will have to be calculated.                    */
/*                                                                           */
/*  KHE_COST_NO                                                              */
/*    An attempt was made to calculate the cost but it failed, so the        */
/*    mtask group has no cost and is not usable.                             */
/*                                                                           */
/*  KHE_COST_YES                                                             */
/*    An attempt was made to calculate the cost and it succeeded, so the     */
/*    mtask group has a cost (stored in the cost field) and is usable.       */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_COST_UNKNOWN,
  KHE_COST_NO,
  KHE_COST_YES
} KHE_COST_STATE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_MTASK_GROUP - a group of mtasks                                 */
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

struct khe_mtask_grouping_rec {
  KHE_COMB_GROUPER		grouper;
  KHE_MTASK_SET			mtask_set;
  ARRAY_KHE_MTASK		leader_mtasks;
  KHE_COST_STATE		cost_state;
  KHE_COST			cost;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_GROUP KheMTaskGroupMake(KHE_COMB_GROUPER cg)                   */
/*                                                                           */
/*  Make a new, empty mtask group.                                           */
/*                                                                           */
/*  Implementation note.  Unwanted mtask group objects are held in a free    */
/*  list in cg.  So getting an object to start with is a bit messy.          */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_GROUP KheMTaskGroupMake(KHE_COMB_GROUPER cg)
{
  KHE_MTASK_GROUP res;  HA_ARENA a;
  res = KheCombGrouperGetMTaskGroup(cg);
  if( res == NULL )
  {
    a = KheCombGrouperArena(cg);
    HaMake(res, a);
    res->mtask_set = KheMTaskSetMake(KheCombGrouperMTaskFinder(cg));
    HaArrayInit(res->leader_mtasks, a);
  }
  else
  {
    KheMTaskSetClear(res->mtask_set);
    HaArrayClear(res->leader_mtasks);
  }
  res->grouper = cg;
  res->cost_state = KHE_COST_NO;
  res->cost = -1;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupDelete(KHE_MTASK_GROUP mg)                             */
/*                                                                           */
/*  Delete mg.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGroupDelete(KHE_MTASK_GROUP mg)
{
  KheCombGrouperPutMTaskGroup(mg->grouper, mg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupClear(KHE_MTASK_GROUP mg)                              */
/*                                                                           */
/*  Clear mg.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGroupClear(KHE_MTASK_GROUP mg)
{
  KheMTaskSetClear(mg->mtask_set);
  HaArrayClear(mg->leader_mtasks);
  mg->cost_state = KHE_COST_NO;
  mg->cost = -1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupOverwrite(KHE_MTASK_GROUP dst_mg,                      */
/*    KHE_MTASK_GROUP src_mg)                                                */
/*                                                                           */
/*  Overwrite src_mg onto dst_mg.                                            */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGroupOverwrite(KHE_MTASK_GROUP dst_mg,
  KHE_MTASK_GROUP src_mg)
{
  int i;
  KheMTaskSetClear(dst_mg->mtask_set);
  KheMTaskSetAddMTaskSet(dst_mg->mtask_set, src_mg->mtask_set);
  HaArrayClear(dst_mg->leader_mtasks);
  HaArrayAppend(dst_mg->leader_mtasks, src_mg->leader_mtasks, i);
  dst_mg->cost_state = src_mg->cost_state;
  dst_mg->cost = src_mg->cost;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGroupMTaskCount(KHE_MTASK_GROUP mg)                          */
/*                                                                           */
/*  Return the number of mtasks in mg.                                       */
/*                                                                           */
/*****************************************************************************/

int KheMTaskGroupMTaskCount(KHE_MTASK_GROUP mg)
{
  return KheMTaskSetMTaskCount(mg->mtask_set);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskGroupMTask(KHE_MTASK_GROUP mg, int i)                  */
/*                                                                           */
/*  Return the i'th mtask of mg.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskGroupMTask(KHE_MTASK_GROUP mg, int i)
{
  return KheMTaskSetMTask(mg->mtask_set, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupIsEmpty(KHE_MTASK_GROUP mg)                            */
/*                                                                           */
/*  Return true if mg is empty.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGroupIsEmpty(KHE_MTASK_GROUP mg)
{
  return KheMTaskGroupMTaskCount(mg) == 0;
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
/*  bool KheMTaskGroupAddMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)             */
/*                                                                           */
/*  If mg can receive mt while avoiding overlaps and maintaining a valid     */
/*  leader task, add mt to mg and return true.  Otherwise leave mg as is     */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGroupAddMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)
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
    mg->cost_state = KHE_COST_UNKNOWN;
    return true;
  }

  /* if mt follows the existing leader mtask, continue with that leader */
  leader_mtask = HaArrayLast(mg->leader_mtasks);
  if( KheMTaskFollows(mt, leader_mtask) )
  {
    KheMTaskSetAddMTask(mg->mtask_set, mt);
    HaArrayAddLast(mg->leader_mtasks, leader_mtask);
    mg->cost_state = KHE_COST_UNKNOWN;
    return true;
  }

  /* if the existing leader task follows mt, add mt as leader */
  if( KheMTaskFollows(leader_mtask, mt) )
  {
    KheMTaskSetAddMTask(mg->mtask_set, mt);
    HaArrayAddLast(mg->leader_mtasks, mt);
    mg->cost_state = KHE_COST_UNKNOWN;
    return true;
  }

  /* no suitable leader task, so change nothing and return false */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupDeleteMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)          */
/*                                                                           */
/*  Delete mt from mg.  This must be the last mtask.                         */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGroupDeleteMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)
{
  KHE_MTASK last_mt;
  last_mt = KheMTaskSetLastAndDelete(mg->mtask_set);
  HnAssert(mt == last_mt, "KheMTaskGroupDeleteMTask internal error");
  HaArrayDeleteLast(mg->leader_mtasks);
  mg->cost_state = KHE_COST_UNKNOWN;
  mg->cost = -1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupContainsMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)        */
/*                                                                           */
/*  Return true if mg contains mt.                                           */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGroupContainsMTask(KHE_MTASK_GROUP mg, KHE_MTASK mt)
{
  int pos;
  return KheMTaskSetContainsMTask(mg->mtask_set, mt, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "cost and execution"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAvailable(KHE_COMB_GROUPER cg, KHE_INTERVAL in,        */
/*    KHE_FRAME days_frame, KHE_RESOURCE r)                                  */
/*                                                                           */
/*  Return true if r is available for all time groups lying in interval      */
/*  in of days_frame.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAvailable(KHE_COMB_GROUPER cg, KHE_INTERVAL in,
  KHE_FRAME days_frame, KHE_RESOURCE r)
{
  int i;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(KheCombGrouperSoln(cg), r);
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++)
  {
    tg = KheFrameTimeGroup(days_frame, i);
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
/*  bool KheFindSuitableResource(KHE_MTASK_GROUP mg, KHE_INTERVAL in,        */
/*    KHE_FRAME days_frame, KHE_RESOURCE *r)                                 */
/*                                                                           */
/*  Find a suitable resource to assign to the mtasks of mg.  If found,       */
/*  set *r to the resource and return true; otherwise, set *r to NULL        */
/*  and return false.                                                        */
/*                                                                           */
/*****************************************************************************/

static bool KheFindSuitableResource(KHE_MTASK_GROUP mg,
  KHE_FRAME days_frame, KHE_RESOURCE *r)
{
   KHE_INTERVAL in;  KHE_MTASK leader_mt;  KHE_RESOURCE_GROUP domain;  int i;

  HnAssert(KheMTaskGroupMTaskCount(mg) > 0,
    "KheMTaskGroupFindSuitableResource internal error");
  in = KheMTaskSetInterval(mg->mtask_set);
  leader_mt = HaArrayLast(mg->leader_mtasks);
  domain = KheMTaskDomain(leader_mt);
  for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
  {
    *r = KheResourceGroupResource(domain, i);
    if( KheResourceIsAvailable(mg->grouper, in, days_frame, *r) )
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
/*  void KheMTaskGroupClearAssignments(KHE_MTASK_GROUP mg,                   */
/*    KHE_RESOURCE r, int stop)                                              */
/*                                                                           */
/*  Unassign r from mg->mtasks[0 .. stop-1].                                 */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupClearAssignments(KHE_MTASK_GROUP mg,
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
/*  bool KheMTaskGroupHasCost(KHE_MTASK_GROUP mg, KHE_COST *cost)            */
/*                                                                           */
/*  If mg has a cost, set *cost to that cost and return true.  Otherwise     */
/*  return false.                                                            */
/*                                                                           */
/*  Implementation note.  The mtask group object remembers whether           */
/*  its cost has been calculated since the most recent change to its         */
/*  mtask set.  If it has, then it does not calculate that cost again.       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGroupHasCost(KHE_MTASK_GROUP mg, KHE_COST *cost)
{
  KHE_MTASK mt;  int i, count;
  KHE_RESOURCE r;  KHE_COMB_GROUPER cg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_GROUP_MONITOR gm;  KHE_SOLN soln;
  KHE_FRAME days_frame;  KHE_INTERVAL in;

  switch( mg->cost_state )
  {
    case KHE_COST_UNKNOWN:

      /* if there are no mtasks then mg has no cost */
      count = KheMTaskSetMTaskCount(mg->mtask_set);
      if( count == 0 )
      {
	mg->cost_state = KHE_COST_NO;
	mg->cost = -1;
	return *cost = 0, false;
      }

      /* find a suitable resource; if we can't then mg has no cost */
      cg = mg->grouper;
      days_frame = KheMTaskFinderDaysFrame(KheCombGrouperMTaskFinder(cg));
      if( !KheFindSuitableResource(mg, days_frame, &r) )
      {
	mg->cost_state = KHE_COST_NO;
	mg->cost = -1;
	return *cost = 0, false;
      }

      /* assign r to the mtasks of mg; if we can't then mg has no cost */
      for( i = 0;  i < count;  i++ )
      {
	mt = KheMTaskSetMTask(mg->mtask_set, i);
	if( !KheMTaskMoveResource(mt, NULL, r, false) )
	{
	  KheMTaskGroupClearAssignments(mg, r, i);
	  mg->cost_state = KHE_COST_NO;
	  mg->cost = -1;
	  return *cost = 0, false;
	}
      }

      /* work out the cost */
      soln = KheCombGrouperSoln(cg);
      gm = KheGroupMonitorMake(soln, 11, "comb grouping");
      rtm = KheResourceTimetableMonitor(soln, r);
      in = KheMTaskSetInterval(mg->mtask_set);
      KheResourceTimetableMonitorAddInterval(rtm, days_frame,
	KheIntervalFirst(in) - 1, KheIntervalLast(in) + 1, gm);
      /* ***
      pos = KheIntervalFirst(in);
      if( pos > 0 )  pos--;
      tg = KheFrameTimeGroup(days_frame, pos);
      first_time_index = KheTimeIndex(KheTimeGroupTime(tg, 0));
      pos = KheIntervalLast(in);
      if( pos < KheFrameTimeGroupCount(days_frame) - 1 )
	pos++;
      tg = KheFrameTimeGroup(days_frame, pos);
      last_time_index = KheTimeIndex(KheTimeGroupTime(tg,
	  KheTimeGroupTimeCount(tg) - 1));
      KheResourceTimetableMonitorAddRange(rtm, first_time_index,
	last_time_index, gm);
      *** */
      mg->cost_state = KHE_COST_YES;
      mg->cost = KheMonitorCost((KHE_MONITOR) gm);
      KheGroupMonitorDelete(gm);

      /* unassign the mtasks (we just need the cost) and return true */
      KheMTaskGroupClearAssignments(mg, r, count);
      return *cost = mg->cost, true;

    case KHE_COST_NO:

      return *cost = 0, false;

    case KHE_COST_YES:

      return *cost = mg->cost, true;

    default:

      HnAbort("KheMTaskGroupHasCost internal error (%d)", mg->cost_state);
      return *cost = 0, false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskGroupIsBetter(KHE_MTASK_GROUP new_mg,                       */
/*    KHE_MTASK_GROUP old_mg)                                                */
/*                                                                           */
/*  Return true if new_mg is better than old_mg, either because old_mg       */
/*  has no cost or because new_mg has smaller cost.                          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskGroupIsBetter(KHE_MTASK_GROUP new_mg,
  KHE_MTASK_GROUP old_mg)
{
  KHE_COST new_mg_cost, old_mg_cost;
  if( !KheMTaskGroupHasCost(new_mg, &new_mg_cost) )
    return false;
  else if( !KheMTaskGroupHasCost(old_mg, &old_mg_cost) )
    return true;
  else
    return new_mg_cost < old_mg_cost;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheMTaskGroupCost(KHE_MTASK_GROUP mg)                           */
/*                                                                           */
/*  Return the cost of mg.  This is assuming that KheMTaskGroupSetCost       */
/*  was called and returned true.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
KHE_COST KheMTaskGroupCost(KHE_MTASK_GROUP mg)
{
  return mg->cost;
}
*** */


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
    val = KheMTaskNeedsAssignmentTaskCount(mt);
    if( val < res )
      res = val;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupMakeOneGroup(KHE_MTASK_GROUP mg,                       */
/*    KHE_SOLN_ADJUSTER sa, KHE_SOLN_ADJUSTER fix_leaders_sa)                */
/*                                                                           */
/*  Make one group out of the mtasks of mg.  If sa != NULL, record the       */
/*  changes in sa so that they can be undone later.                          */
/*                                                                           */
/*  If fix_leaders_sa != NULL, fix the assignments of the leader tasks       */
/*  (this will be to NULL) and add those fixes to fix_leaders_sa.            */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskGroupMakeOneGroup(KHE_MTASK_GROUP mg,
  KHE_SOLN_ADJUSTER sa /* , KHE_SOLN_ADJUSTER fix_leaders_sa */)
{
  KHE_MTASK leader_mt, mt;  KHE_TASK leader_task, task;  int i;
  KHE_COST non_asst_cost, asst_cost;  KHE_MTASK_FINDER mtf;

  /* find the leader task */
  leader_mt = HaArrayLast(mg->leader_mtasks);
  HnAssert(KheMTaskUnassignedTaskCount(leader_mt) > 0,
    "KheMTaskGroupMakeOneGroup internal error 1");
  leader_task = KheMTaskTask(leader_mt, KheMTaskAssignedTaskCount(leader_mt),
    &non_asst_cost, &asst_cost);

  /* build the group */
  mtf = KheCombGrouperMTaskFinder(mg->grouper);
  /* KheMTaskFinderGroupBegin(mtf, leader_task); */
  KheMTaskFinderTaskGrouperClear(mtf);
  KheMTaskFinderTaskGrouperAddTask(mtf, leader_task);
  for( i = 0;  i < KheMTaskSetMTaskCount(mg->mtask_set);  i++ )
  {
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    if( mt != leader_mt )
    {
      HnAssert(KheMTaskUnassignedTaskCount(mt) > 0,
	"KheMTaskGroupMakeOneGroup internal error 2");
      task = KheMTaskTask(mt, KheMTaskAssignedTaskCount(mt),
	&non_asst_cost, &asst_cost);
      /* ***
      if( !KheMTaskFinderGroupAddTask(mtf, task) )
	HnAbort("KheMTaskGroupMakeOneGroup internal error 3");
      *** */
      if( !KheMTaskFinderTaskGrouperAddTask(mtf, task) )
	HnAbort("KheMTaskGroupMakeOneGroup internal error 3");
    }
  }
  /* KheMTaskFinderGroupEnd(mtf, sa); */
  KheMTaskFinderTaskGrouperMakeGroup(mtf, sa);

  /* optionally fix the leader task assignment */
  /* ***
  if( fix_leaders_sa != NULL )
    KheSolnAdjust erTaskEnsureFixed(fix_leaders_sa, leader_task);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskGroupExecute(KHE_MTASK_GROUP mg, int max_num,                */
/*    KHE_SOLN_ADJUSTER sa, KHE_SOLN_ADJUSTER fix_leaders_sa,char *debug_str)*/
/*                                                                           */
/*  Carry out a grouping of the mtasks of mg.  Make as many groups as        */
/*  possible, but not more than max_num.  If sa != NULL, record the          */
/*  changes in sa so that they can be undone later.                          */
/*                                                                           */
/*  If fix_leaders_sa != NULL, fix the assignments of the leader tasks       */
/*  (this will be to NULL) and add those fixes to fix_leaders_sa.            */
/*                                                                           */
/*****************************************************************************/

int KheMTaskGroupExecute(KHE_MTASK_GROUP mg, int max_num,
  KHE_SOLN_ADJUSTER sa, /* KHE_SOLN_ADJUSTER fix_leaders_sa, */
  char *debug_str)
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
    if( DEBUG1 )
    {
      fprintf(stderr, "  %s: %d x ", debug_str, num);
      KheMTaskGroupDebug(mg, 2, 0, stderr);
    }
    for( res = 0;  res < num;  res++ )
      KheMTaskGroupMakeOneGroup(mg, sa /* , fix_leaders_sa */);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMTaskGroupCostShow(KHE_MTASK_GROUP mg)                          */
/*                                                                           */
/*  Show the cost of mg, possibly unknown or none.                           */
/*                                                                           */
/*****************************************************************************/

static char *KheMTaskGroupCostShow(KHE_MTASK_GROUP mg, char buff[10])
{
  switch( mg->cost_state )
  {
    case KHE_COST_UNKNOWN:

      snprintf(buff, 10, "unknown");
      break;

    case KHE_COST_NO:

      snprintf(buff, 10, "none");
      break;

    case KHE_COST_YES:

      snprintf(buff, 10, "%.5f", KheCostShow(mg->cost));
      break;

    default:

      snprintf(buff, 10, "?");
      break;
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskGroupDebug(KHE_MTASK_GROUP mg,                              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of mg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskGroupDebug(KHE_MTASK_GROUP mg,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_MTASK mt;  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  char buff[10];
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{cost %s: ", KheMTaskGroupCostShow(mg, buff));
  for( i = 0;  i < KheMTaskSetMTaskCount(mg->mtask_set);  i++ )
  {
    if( i > 0 )
      fprintf(fp, ", ");
    mt = KheMTaskSetMTask(mg->mtask_set, i);
    if( mt == HaArrayLast(mg->leader_mtasks) )
      fprintf(fp, "|");
    task = KheMTaskTask(mt, 0, &non_asst_cost, &asst_cost);
    KheTaskDebug(task, 2, -1, fp);
    if( mt == HaArrayLast(mg->leader_mtasks) )
      fprintf(fp, "|");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}

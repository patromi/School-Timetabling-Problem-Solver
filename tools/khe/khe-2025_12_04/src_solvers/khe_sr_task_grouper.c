
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
/*  FILE:         khe_sr_task_grouper.c                                      */
/*  DESCRIPTION:  Task grouper                                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0

/* flown in from khe_sr_interval_grouping.c */
#define EQ(task, val) (strcmp(KheTaskId(task), val) == 0)
#define DEBUG68(task)							    \
  (false && (EQ(task, "1Thu:Day.31") || EQ(task, "1Thu:Day.32")))
#define DEBUG5(task) (false && EQ(task, "3Sat:Night.4"))


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_GROUPER_ENTRY - one entry in a task grouper.               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_grouper_entry_rec {
  KHE_TASK				task;
  KHE_INTERVAL				interval;
  KHE_TASK_GROUP_DOMAIN			domain;
  KHE_RESOURCE				assigned_resource;
} *KHE_TASK_GROUPER_ENTRY;

typedef HA_ARRAY(KHE_TASK_GROUPER_ENTRY) ARRAY_KHE_TASK_GROUPER_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_GROUPER                                                    */
/*                                                                           */
/*****************************************************************************/

struct khe_task_grouper_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_FRAME			days_frame;
  KHE_TASK_GROUP_DOMAIN_FINDER	domain_finder;
  ARRAY_KHE_TASK_GROUPER_ENTRY	entries;
  ARRAY_KHE_TASK_GROUPER_ENTRY	task_grouper_entry_free_list;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_GROUPER_ENTRY"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMake(KHE_TASK_GROUPER tg,      */
/*    KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, KHE_INTERVAL in,           */
/*    KHE_TASK_GROUP_DOMAIN domain, KHE_RESOURCE assigned_resource)          */
/*                                                                           */
/*  Make a new task grouper entry object with these attributes.              */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMake(KHE_TASK_GROUPER tg,
  KHE_TASK task, KHE_INTERVAL in, KHE_TASK_GROUP_DOMAIN domain,
  KHE_RESOURCE assigned_resource)
{
  KHE_TASK_GROUPER_ENTRY res;
  HnAssert(domain != NULL, "KheTaskGrouperEntryMake internal error");
  if( HaArrayCount(tg->task_grouper_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(tg->task_grouper_entry_free_list);
  else
    HaMake(res, tg->arena);
  res->task = task;
  res->interval = in;
  res->domain = domain;
  res->assigned_resource = assigned_resource;
  if( task != NULL && DEBUG68(task) )
    fprintf(stderr, "KheTaskGrouperEntryMake(%s) returning %p\n",
      KheTaskId(task), (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_GROUPER"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER KheTaskGrouperMake(KHE_SOLN soln,                       */
/*    KHE_FRAME days_frame, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new task grouper object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUPER KheTaskGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a)
{
  KHE_TASK_GROUPER res;
  HnAssert(days_frame != NULL, "KheTaskGrouperMake: days_frame == NULL");
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->days_frame = days_frame;
  res->domain_finder = tgdf;
  HaArrayInit(res->entries, a);
  HaArrayInit(res->task_grouper_entry_free_list, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperClear(KHE_TASK_GROUPER tg)                            */
/*                                                                           */
/*  Clear out tg, ready to start a new task group.                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskGrouperClear(KHE_TASK_GROUPER tg)
{
  int i;
  HaArrayAppend(tg->task_grouper_entry_free_list, tg->entries, i);
  HaArrayClear(tg->entries);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperAddTaskCheck(KHE_TASK_GROUPER tg, KHE_TASK task)      */
/*                                                                           */
/*  Return true if KheTaskGrouperAddTask would succeed.                      */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGrouperAddTaskCheck(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KHE_INTERVAL new_interval;  KHE_TASK_GROUP_DOMAIN new_domain;
  KHE_RESOURCE new_assigned_resource;  KHE_TASK_GROUPER_ENTRY prev;
  if( HaArrayCount(tg->entries) == 0 )
  {
    if( KheTaskGrouperSeparateAddInitialTask(tg, task, &new_interval,
	  &new_domain, &new_assigned_resource) )
      return true;
  }
  else
  {
    prev = HaArrayLast(tg->entries);
    if( KheTaskGrouperSeparateAddTask(tg, prev->interval, prev->domain,
	  prev->assigned_resource, task, &new_interval, &new_domain,
	  &new_assigned_resource) )
      return true;
  }

  /* no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperAddTask(KHE_TASK_GROUPER tg, KHE_TASK task)           */
/*                                                                           */
/*  If task is compatible with any other tasks that have been added, add     */
/*  it and return true.  Otherwise add nothing and return false.             */
/*                                                                           */
/*****************************************************************************/
/* ***
static bool KheTaskGrouperEntryMakeTask(KHE_TASK_GROUPER tg,
  KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, bool unchecked,
  KHE_TASK_GROUPER_ENTRY *res);
*** */

bool KheTaskGrouperAddTask(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KHE_INTERVAL new_interval;  KHE_TASK_GROUP_DOMAIN new_domain;
  KHE_RESOURCE new_assigned_resource;  KHE_TASK_GROUPER_ENTRY prev, tge;
  if( HaArrayCount(tg->entries) == 0 )
  {
    if( KheTaskGrouperSeparateAddInitialTask(tg, task, &new_interval,
	  &new_domain, &new_assigned_resource) )
    {
      tge = KheTaskGrouperEntryMake(tg, task, new_interval, new_domain,
	new_assigned_resource);
      HaArrayAddLast(tg->entries, tge);
      return true;
    }
  }
  else
  {
    prev = HaArrayLast(tg->entries);
    if( KheTaskGrouperSeparateAddTask(tg, prev->interval, prev->domain,
	  prev->assigned_resource, task, &new_interval, &new_domain,
	  &new_assigned_resource) )
    {
      tge = KheTaskGrouperEntryMake(tg, task, new_interval, new_domain,
	new_assigned_resource);
      HaArrayAddLast(tg->entries, tge);
      return true;
    }
  }

  /* no luck */
  return false;
}

/* *** old version
bool KheTaskGrouperAddTask(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KHE_TASK_GROUPER_ENTRY entry, last;
  last = (HaArrayCount(tg->entries) == 0 ? NULL : HaArrayLast(tg->entries));
  entry = KheTaskGrouperMakeEntry(tg);
  if( KheTaskGrouperEntryAddTask(last, task, tg->days_frame,
	tg->domain_finder, entry) )
  {
    HaArrayAddLast(tg->entries, entry);
    return true;
  }
  else
  {
    KheTaskGrouperEntryFree(tg, entry);
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperDeleteTask(KHE_TASK_GROUPER tg, KHE_TASK task)        */
/*                                                                           */
/*  Delete previously added task task from tg.  Here task must be the most   */
/*  recently added but not deleted task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheTaskGrouperDeleteTask(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KHE_TASK_GROUPER_ENTRY last;
  HnAssert(HaArrayCount(tg->entries) > 0,
    "KheTaskGrouperDeleteTask internal error (no tasks)");
  last = HaArrayLastAndDelete(tg->entries);
  HnAssert(last->task == task,
    "KheTaskGrouperDeleteTask internal error (task not last added)");
  HaArrayAddLast(tg->task_grouper_entry_free_list, last);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperAddHistory(KHE_TASK_GROUPER tg,                       */
/*    KHE_RESOURCE r, int durn)                                              */
/*                                                                           */
/*  Add a history entry with these attributes to tg.                         */
/*                                                                           */
/*****************************************************************************/
/* ***
static KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMakeHistory(
  KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn);
*** */

void KheTaskGrouperAddHistory(KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn)
{
  KHE_TASK_GROUPER_ENTRY tge;  KHE_INTERVAL new_interval;
  KHE_TASK_GROUP_DOMAIN new_domain;  KHE_RESOURCE new_assigned_resource;
  HnAssert(HaArrayCount(tg->entries) == 0,
    "KheTaskGrouperAddHistoryEntry internal error (not first entry)");
  KheTaskGrouperSeparateAddHistory(tg, r, durn, &new_interval,
    &new_domain, &new_assigned_resource);
  tge = KheTaskGrouperEntryMake(tg, NULL, new_interval, new_domain,
    new_assigned_resource);
  HaArrayAddLast(tg->entries, tge);
}

/* *** old version
void KheTaskGrouperAddHistoryEntry(KHE_TASK_GROUPER tg,
  KHE_RESOURCE r, int durn)
{
  KHE_TASK_GROUPER_ENTRY entry;
  HnAssert(HaArrayCount(tg->entries) == 0,
    "KheTaskGrouperAddHistoryEntry internal error (not first entry)");
  entry = KheTaskGrouperMakeEntry(tg);
  KheTaskGrouperEntryAddHistory(NULL, r, durn, tg->domain_finder, entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperDeleteHistory(KHE_TASK_GROUPER tg, KHE_RESOURCE r,    */
/*    int durn)                                                              */
/*                                                                           */
/*  Delete a history entry with these attributes from tg.                    */
/*                                                                           */
/*****************************************************************************/

void KheTaskGrouperDeleteHistory(KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn)
{
  KHE_TASK_GROUPER_ENTRY last;
  HnAssert(HaArrayCount(tg->entries) >= 1,
    "KheTaskGrouperDeleteHistory internal error (no entries)");
  HnAssert(HaArrayCount(tg->entries) <= 1,
    "KheTaskGrouperDeleteHistory internal error (two or more entries)");
  last = HaArrayLastAndDelete(tg->entries);
  HnAssert(last->task == NULL,
    "KheTaskGrouperDeleteHistory internal error (sole entry not history)");
  HnAssert(last->assigned_resource == r,
    "KheTaskGrouperDeleteHistory internal error (wrong resource)");
  HnAssert(KheIntervalLength(last->interval) == durn,
    "KheTaskGrouperDeleteHistory internal error (wrong durn)");
  HaArrayAddLast(tg->task_grouper_entry_free_list, last);
}


/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

int KheTaskGrouperTaskCount(KHE_TASK_GROUPER tg)
{
  return HaArrayCount(tg->entries);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskGrouperTask(KHE_TASK_GROUPER tg, int i)                  */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskGrouperTask(KHE_TASK_GROUPER tg, int i)
{
  return HaArray(tg->entries, i)->task;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGrouperEntryCount(KHE_TASK_GROUPER tg)                        */
/*                                                                           */
/*  Return the number of entries in tg.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheTaskGrouperEntryCount(KHE_TASK_GROUPER tg)
{
  return HaArrayCount(tg->entries);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntry(KHE_TASK_GROUPER tg, int i)   */
/*                                                                           */
/*  Return the i'th entry of tg.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntry(KHE_TASK_GROUPER tg, int i)
{
  return HaArray(tg->entries, i);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperContainsTask(KHE_TASK_GROUPER tg, KHE_TASK task)      */
/*                                                                           */
/*  Return true if tg contains task.                                         */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn (never used, apparently)
bool KheTaskGrouperContainsTask(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KHE_TASK_GROUPER_ENTRY entry;  int i;
  HaArrayForEach(tg->entries, entry, i)
    if( entry->task == task )
      return true;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperCopy(KHE_TASK_GROUPER dst_tg, KHE_TASK_GROUPER src_tg)*/
/*                                                                           */
/*  Copy the contents of src_tg into dst_tg;                                 */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn (never used, apparently)
void KheTaskGrouperCopy(KHE_TASK_GROUPER dst_tg, KHE_TASK_GROUPER src_tg)
{
  KHE_TASK_GROUPER_ENTRY src_entry, dst_entry, prev_dst_entry;  int i;
  KheTaskGrouperClear(dst_tg);
  prev_dst_entry = NULL;
  HaArrayForEach(src_tg->entries, src_entry, i)
  {
    dst_entry = KheTaskGrouperMakeEntry(dst_tg);
    KheTaskGrouperEntryCopy(dst_entry, src_entry);
    dst_entry->prev = prev_dst_entry;
    prev_dst_entry = dst_entry;
    ** KheTaskGrouperAddTask(dst_tg, entry->task); **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskGrouperInterval(KHE_TASK_GROUPER tg)                 */
/*                                                                           */
/*  Return the interval of days covered by the tasks of tg.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_INTERVAL KheTaskGrouperInterval(KHE_TASK_GROUPER tg)
{
  KHE_TASK_GROUPER_ENTRY last;
  if( HaArrayCount(tg->entries) == 0 )
    return KheIntervalMake(1, 0);
  last = HaArrayLast(tg->entries);
  return KheTaskGrouperEntryInterval(last);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTaskGrouperCost(KHE_TASK_GROUPER tg)                         */
/*                                                                           */
/*  Return the cost of grouping tg's tasks.                                  */
/*                                                                           */
/*****************************************************************************/
/* ***
static KHE_COST KheTaskGrouperEntryCost(KHE_TASK_GROUPER_ENTRY last,
  KHE_TASK_GROUPER tg);
*** */

KHE_COST KheTaskGrouperCost(KHE_TASK_GROUPER tg)
{
  KHE_RESOURCE r;  KHE_TASK_GROUPER_ENTRY entry, last;
  KHE_GROUP_MONITOR gm;  KHE_INTERVAL in;  KHE_RESOURCE_GROUP domain;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_MARK mark;  KHE_COST res;
  KHE_TASK_SET ts;  KHE_TASK task;  int i;

  /* empty group not permitted */
  HnAssert(HaArrayCount(tg->entries) > 0,
    "KheTaskGrouperCost internal error (no entries)");
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheTaskGrouperCost(tg)\n");
    KheTaskGrouperDebug(tg, 2, 2, stderr);
  }

  /* history groups are not monitored so have zero cost */
  last = HaArrayLast(tg->entries);
  if( last->task == NULL )
    return 0;

  /* find a suitable resource to assign the tasks to */
  if( last->assigned_resource != NULL )
    r = last->assigned_resource;
  else
  {
    domain = KheTaskGroupDomainValue(last->domain);
    HnAssert(KheResourceGroupResourceCount(domain) > 0,
      "KheTaskGrouperCost internal error 1");
    r = KheResourceGroupResource(domain, 0);
  }

  /* find the interval of days_frame covered by these tasks */
  in = KheIntervalMake(KheIntervalFirst(last->interval) - 1,
    KheIntervalLast(last->interval) + 1);
  if( DEBUG4 )
    fprintf(stderr, "  using in %s and resource %s\n",
      KheIntervalShow(in, tg->days_frame), KheResourceId(r));

  /* find the cluster busy times and limit busy times monitors during in */
  gm = KheGroupMonitorMake(tg->soln, 11, "comb grouping");
  rtm = KheResourceTimetableMonitor(tg->soln, r);
  KheResourceTimetableMonitorAddInterval(rtm, tg->days_frame,
    KheIntervalFirst(in), KheIntervalLast(in),
    last->assigned_resource != NULL, gm);

  /* make sure r is free during in */
  mark = KheMarkBegin(tg->soln);
  ts = KheTaskSetMake(tg->soln);
  KheResourceTimetableMonitorAddProperRootTasksInInterval(rtm, tg->days_frame,
    KheIntervalFirst(in), KheIntervalLast(in), true, ts);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( KheTaskAssignIsFixed(task) )
      KheTaskAssignUnFix(task);
    if( KheTaskAsstResource(task) != NULL )
      KheTaskUnAssignResource(task);
  }
  KheTaskSetDelete(ts);

  /* assign r to the tasks of last */
  HaArrayForEach(tg->entries, entry, i)
    if( entry->task != NULL )
    {
      if( KheTaskAssignIsFixed(entry->task) )
	KheTaskAssignUnFix(entry->task);
      if( KheTaskAsstResource(entry->task) != NULL )
        KheTaskUnAssignResource(entry->task);
      if( !KheTaskAssignResource(entry->task, r) )
	HnAbort("KheTaskGrouperEntryCost internal error 3");
    }

  /* work out the cost */
  if( DEBUG4 )
    KheGroupMonitorDebug(gm, 2, 2, stderr);
  res = KheMonitorCost((KHE_MONITOR) gm);
  KheGroupMonitorDelete(gm);

  /* return to the initial state, and return the cost */
  KheMarkEnd(mark, true);
  if( DEBUG4 )
    fprintf(stderr, "] KheTaskGrouperCost returning %.5f\n", KheCostShow(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskGrouperMakeGroup(KHE_TASK_GROUPER tg,                    */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make a group and return its leader task.                                 */
/*                                                                           */
/*****************************************************************************/
/* ***
static KHE_TASK KheTaskGrouperEntryMakeGroup(KHE_TASK_GROUPER_ENTRY last,
  KHE_SOLN_ADJUSTER sa);
*** */

KHE_TASK KheTaskGrouperMakeGroup(KHE_TASK_GROUPER tg, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK_GROUPER_ENTRY entry, last;  bool has_fixed_child, fixed;
  KHE_RESOURCE_GROUP domain, leader_domain;
  KHE_TASK leader_task;
  KHE_TASK_BOUND tb;  int domain_count, leader_domain_count, i;

  /* group must be non-empty */
  HnAssert(HaArrayCount(tg->entries) > 0,
    "KheTaskGrouperEntryMakeGroup internal error (tg is empty)");

  /* find the last entry; it contains the leader domain and shared parent */
  last = HaArrayLast(tg->entries);
  if( DEBUG1 )
    fprintf(stderr, "  [ KheTaskGrouperEntryMakeGroup:\n");

  /* (0) find a leader task - one with a domain of minimum size */
  leader_task = NULL;
  leader_domain = NULL;
  leader_domain_count = INT_MAX;
  HaArrayForEach(tg->entries, entry, i)
    if( entry->task != NULL )
    {
      domain = KheTaskDomain(entry->task);
      domain_count = KheResourceGroupResourceCount(domain);
      if( KheResourceGroupResourceCount(domain) < leader_domain_count )
      {
	leader_task = entry->task;
	leader_domain = domain;
	leader_domain_count = domain_count;
      }
    }
  if( leader_task == NULL )
    return NULL;

  /* (1) make sure that the leader task has the correct domain */
  domain = KheTaskGroupDomainValue(last->domain);
  if( KheResourceGroupResourceCount(domain) !=
      KheResourceGroupResourceCount(leader_domain) )
  {
    if( DEBUG5(leader_task) )
    {
      fprintf(stderr, "[ KheTaskGrouperEntryMakeGroup:\n");
      HaArrayForEach(tg->entries, entry, i)
	if( entry->task != NULL )
	{
	  fprintf(stderr, "  %s: ", KheTaskId(entry->task));
	  KheResourceGroupDebug(KheTaskDomain(entry->task), 2, 0, stderr);
	}
      fprintf(stderr, "]\n");
    }
    tb = KheTaskGroupDomainTaskBound(last->domain);
    KheSolnAdjusterTaskAddTaskBound(sa, leader_task, tb);
  }

  /* (2) move followers to the leader, unfixing and refixing if needed. */
  /* We call KheSolnAdjusterTaskGroup rather than KheSolnAdjusterTaskMove */
  /* so that during undo, entry->task gets moved to its parent's parent */
  has_fixed_child = false;
  HaArrayForEach(tg->entries, entry, i)
    if( entry->task != NULL && entry->task != leader_task )
    {
      fixed = KheTaskAssignIsFixed(entry->task);
      if( fixed )
      {
	has_fixed_child = true;
	KheSolnAdjusterTaskAssignUnFix(sa, entry->task);
      }
      if( DEBUG1 )
	fprintf(stderr, "    grouping %s under %s\n",
	  KheTaskId(entry->task), KheTaskId(leader_task));
      if( !KheSolnAdjusterTaskGroup(sa, entry->task, leader_task) )
	HnAbort("KheTaskGrouperEntryMakeGroup internal error:  cannot group");
      if( fixed )
	KheSolnAdjusterTaskAssignFix(sa, entry->task);
    }

  /* (3) move the leader task to the assigned resource, if not already done */
  /* We don't inform sa about this, so during undo we do nothing here */
  if( KheTaskAsstResource(leader_task) != last->assigned_resource )
  {
    HnAssert(!KheTaskAssignIsFixed(leader_task),
     "KheTaskGrouperMakeGroup internal error:  fixed leader needs move");
    if( !KheTaskAssignResource(leader_task, last->assigned_resource) )
      HnAbort("KheTaskGrouperMakeGroup internal error:  cannot move leader");
  }

  /* (3) fix the leader task's assignment, if there is a fixed follower */
  if( has_fixed_child && !KheTaskAssignIsFixed(leader_task) )
    KheSolnAdjusterTaskAssignFix(sa, leader_task);

  /* all done, return leader task */
  if( DEBUG1 )
    fprintf(stderr, "  ] KheTaskGrouperEntryMakeGroup returning\n");
  return leader_task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperDebugHeader(KHE_TASK_GROUPER tg, FILE *fp)            */
/*                                                                           */
/*  Print onto fp the header part of the debug print of tg.                  */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGrouperDebugHeader(KHE_TASK_GROUPER tg, FILE *fp)
{
  fprintf(fp, "KheTaskGrouper");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperDebug(KHE_TASK_GROUPER tg,                            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tg onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheTaskGrouperDebug(KHE_TASK_GROUPER tg,
  int verbosity, int indent, FILE *fp)
{
  KHE_TASK_GROUPER_ENTRY tge;  int i;
  if( indent >= 0 && verbosity >= 2 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheTaskGrouperDebugHeader(tg, fp);
    fprintf(fp, "\n");
    HaArrayForEach(tg->entries, tge, i)
      if( tge->task != NULL )
	KheTaskDebug(tge->task, verbosity, indent + 2, fp);
      else
	fprintf(fp, "%*sHistory(%s, %d)\n", indent + 2, "",
	  KheResourceId(tge->assigned_resource),
	  KheIntervalLength(tge->interval));
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
  {
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{");
    HaArrayForEach(tg->entries, tge, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      if( tge->task != NULL )
	fprintf(fp, "%s", KheTaskId(tge->task));
      else
	fprintf(fp, "History(%s, %d)\n",
	  KheResourceId(tge->assigned_resource),
	  KheIntervalLength(tge->interval));
    }
    fprintf(fp, "}");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperAddDummyEntry(KHE_TASK_GROUPER tg)                    */
/*                                                                           */
/*  Add a dummy entry to tg.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KheTaskGrouperAddDummyEntry(KHE_TASK_GROUPER tg)
{
  KHE_TASK_GROUPER_ENTRY entry, last;
  last = (HaArrayCount(tg->entries) == 0 ? NULL : HaArrayLast(tg->entries));
  entry = KheTaskGrouperMakeEntry(tg);
  KheTaskGrouperEntryAddDummy(last, entry);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsBusy(KHE_RESOURCE r, KHE_INTERVAL in,                  */
/*    KHE_FRAME days_frame)                                                  */
/*                                                                           */
/*  Return true if r is busy anywhere during in.                             */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsBusy(KHE_RESOURCE r, KHE_INTERVAL in,
  KHE_FRAME days_frame, KHE_SOLN soln)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i;  KHE_TIME_GROUP tg;
  rtm = KheResourceTimetableMonitor(soln, r);
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(days_frame, i);
    if( !KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg, days_frame,
	  NULL, false) )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperEntryMakeTaskUnchecked(KHE_TASK_GROUPER_ENTRY prev,   */
/*    KHE_TASK task, KHE_FRAME days_frame, KHE_TASK_GROUPER_ENTRY next)      */
/*                                                                           */
/*  Like KheTaskGrouperEntryAddTask except there is no checking, and some    */
/*  of the fields of next may have rough and ready values.  The return       */
/*  value is always true.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheTaskGrouperEntryMakeTaskUnchecked(KHE_TASK_GROUPER tg,
  KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, KHE_TASK_GROUPER_ENTRY *res)
{
  KHE_RESOURCE_GROUP task_domain;  ** KHE_TASK new_leader; **
  ** bool task_has_fixed_unassigned, new_has_fixed_unassigned; **
  KHE_RESOURCE task_assigned_resource, new_assigned_resource;
  KHE_INTERVAL task_in, new_in;
  KHE_TASK_GROUP_DOMAIN new_domain;
  ** KHE_TASK_GROUP_DOMAIN_TYPE new_type; **


  *****************************************************************************
  **                                                                         **
  **  The task must be non-NULL and a proper root task                       **
  **                                                                         **
  *****************************************************************************

  HnAssert(task != NULL,
    "KheTaskGrouperAddTask internal error:  task is NULL");
  HnAssert(KheTaskIsProperRoot(task),
    "KheTaskGrouperAddTask internal error:  task is not a proper root task");

  ** get some basic facts about task **
  task_in = KheTaskInterval(task, tg->days_frame),
  task_domain = KheTaskDomain(task);
  task_assigned_resource = KheTaskAsstResource(task);

  ***************************************************************************
  **                                                                       **
  **  1. Task cannot have a fixed non-assignment.                          **
  **                                                                       **
  ***************************************************************************

  if( KheTaskAssignIsFixed(task) && task_assigned_resource == NULL )
  {
    if( res != NULL )
      *res = NULL;
    return false;
  }


  if( prev == NULL )
  {
    ***************************************************************************
    **                                                                       **
    **  2. Interval of days must be disjoint from prev.                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  3. The intersection of task's domain with the others is non-empty.   **
    **                                                                       **
    ***************************************************************************

    if( KheResourceGroupResourceCount(task_domain) == 0 )
    {
      if( DEBUG2 )
	fprintf(stderr,"  KheTaskGrouperEntryAddTask false (empty domain 1)\n");
      if( res != NULL )
	*res = NULL;
      return false;
    }
    new_domain = KheTaskGroupDomainMake(tg->domain_finder, NULL, task_domain);


    ***************************************************************************
    **                                                                       **
    **  4. Can't have conflicting resource assignments.                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  5. Intersection of domains must include any assigned resource.       **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  X. Can't have one task with an assigned resource and another with    **
    **     a fixed non-assignment.                                           **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  6. Can't have any interference.                                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  All good so make the new object and return true.                     **
    **                                                                       **
    ***************************************************************************

    if( res != NULL )
      *res = KheTaskGrouperEntryMake(tg, NULL, task, task_in, new_domain,
	task_assigned_resource);
    ** ***
    KheTaskGrouperEntryInit(next, task, ** task, ** new_domain,
      task_assigned_resource, NULL, task_in, KHE_TASK_GROUPER_ENTRY_ORDINARY,
      task_has_fixed_unassigned);
    *** **
    return true;
  }
  else
  {
    ***************************************************************************
    **                                                                       **
    **  2. Interval of days must be disjoint from prev.                      **
    **                                                                       **
    ***************************************************************************

    new_in = KheIntervalUnion(prev->interval, task_in);


    ***************************************************************************
    **                                                                       **
    **  3. The intersection of task's domain with the others is non-empty.   **
    **                                                                       **
    ***************************************************************************

    new_domain = KheTaskGroupDomainMake(tg->domain_finder,
      prev->domain **OK**, task_domain);


    ***************************************************************************
    **                                                                       **
    **  4. Can't have conflicting resource assignments.                      **
    **                                                                       **
    **     If either the existing tasks or the new task have an assigned     **
    **     resource, that is the new shared assigned resource.   If both     **
    **     have it, they must be equal, otherwise there is no suitable       **
    **     shared assigned resource, so return false.                        **
    **                                                                       **
    **     NB this works correctly when prev is a history entry:  in that    **
    **     case, prev->assigned_resource != NULL.                            **
    **                                                                       **
    ***************************************************************************

    if( prev->assigned_resource == NULL )
      new_assigned_resource = task_assigned_resource;
    else
      new_assigned_resource = prev->assigned_resource;


    ***************************************************************************
    **                                                                       **
    **  5. Intersection of domains must include any assigned resource.       **
    **                                                                       **
    ***************************************************************************


    ***************************************************************************
    **                                                                       **
    **  X. Can't have one task with an assigned resource and another with    **
    **     a fixed non-assignment.                                           **
    **                                                                       **
    **     If we are attempting to add a fixed unassigned task to a group    **
    **     containing a task with a non-NULL parent, return false.           **
    **                                                                       **
    **     If we are attempting to add a task with a non-NULL parent to a    **
    **     group containing a fixed unassigned task, return false.           **
    **                                                                       **
    **     NB this works correctly when prev is a history entry:  in that    **
    **     case, prev->assigned_resource != NULL.                            **
    **                                                                       **
    ***************************************************************************

    ** ***
    new_has_fixed_unassigned = prev->has_fixed_unassigned ||
      task_has_fixed_unassigned;
    *** **


    ***************************************************************************
    **                                                                       **
    **  6. Can't have any interference.                                      **
    **                                                                       **
    ***************************************************************************

    ***************************************************************************
    **                                                                       **
    **  All good so make the new object and return true.                     **
    **                                                                       **
    ***************************************************************************

    if( res != NULL )
      *res = KheTaskGrouperEntryMake(tg, prev, task, new_in, new_domain,
	new_assigned_resource);
    ** ***
    KheTaskGrouperEntryInit(next, task, ** new_leader, ** new_domain,
      new_assigned_resource, prev, new_in, KHE_TASK_GROUPER_ENTRY_ORDINARY,
      new_has_fixed_unassigned);
    *** **
    return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperEntryMakeTask(KHE_TASK_GROUPER tg,                    */
/*    KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, bool unchecked,            */
/*    KHE_TASK_GROUPER_ENTRY *res)                                           */
/*                                                                           */
/*  If task is compatible with prev, make a new entry in *res and return     */
/*  true.  Otherwise change nothing and return false.                        */
/*                                                                           */
/*  Alternatively, if res == NULL return the same value but do not           */
/*  make a new entry.                                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheTaskGrouperEntryMakeTask(KHE_TASK_GROUPER tg,
  KHE_TASK_GROUPER_ENTRY prev, KHE_TASK task, bool unchecked,
  KHE_TASK_GROUPER_ENTRY *res)
{
  KHE_RESOURCE_GROUP task_domain, new_rg;  ** KHE_TASK new_leader; **
  KHE_RESOURCE task_assigned_resource, new_assigned_resource;
  KHE_INTERVAL task_in, new_in;  KHE_SOLN soln;
  KHE_TASK_GROUP_DOMAIN new_domain;
  ** KHE_TASK_GROUP_DOMAIN_TYPE new_type; **

  if( unchecked )
    return KheTaskGrouperEntryMakeTaskUnchecked(tg, prev, task, res);

  *****************************************************************************
  **                                                                         **
  **  The task must be non-NULL and a proper root task                       **
  **                                                                         **
  *****************************************************************************

  HnAssert(task != NULL,
    "KheTaskGrouperAddTask internal error:  task is NULL");
  if( DEBUG3 && !KheTaskIsProperRoot(task) )
  {
    fprintf(stderr, "KheTaskGrouperEntryMakeTask failing (not proper root)\n");
    fprintf(stderr, "task:\n");
    KheTaskDebug(task, 2, 2, stderr);
    fprintf(stderr, "task's proper root:\n");
    KheTaskDebug(KheTaskProperRoot(task), 2, 2, stderr);
  }
  HnAssert(KheTaskIsProperRoot(task),
    "KheTaskGrouperAddTask internal error:  task is not a proper root task");

  ** get some basic facts about task **
  task_in = KheTaskInterval(task, tg->days_frame),
  task_domain = KheTaskDomain(task);
  task_assigned_resource = KheTaskAsstResource(task);

  ***************************************************************************
  **                                                                       **
  **  1. Task cannot have a fixed non-assignment.                          **
  **                                                                       **
  ***************************************************************************

  if( KheTaskAssignIsFixed(task) && task_assigned_resource == NULL )
  {
    if( res != NULL )
      *res = NULL;
    return false;
  }

  if( prev == NULL )
  {
    ***************************************************************************
    **                                                                       **
    **  2. Interval of days must be disjoint from prev.                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  3. The intersection of task's domain with the others is non-empty.   **
    **                                                                       **
    ***************************************************************************

    if( KheResourceGroupResourceCount(task_domain) == 0 )
    {
      if( DEBUG2 )
	fprintf(stderr,"  KheTaskGrouperEntryAddTask false (empty domain 1)\n");
      if( res != NULL )
	*res = NULL;
      return false;
    }
    new_domain = KheTaskGroupDomainMake(tg->domain_finder, NULL, task_domain);


    ***************************************************************************
    **                                                                       **
    **  4. Can't have conflicting resource assignments.                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  5. Intersection of domains must include any assigned resource.       **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  6. Can't have any interference.                                      **
    **                                                                       **
    ***************************************************************************

    ** nothing to do here when prev is NULL **


    ***************************************************************************
    **                                                                       **
    **  All good so make the new object and return true.                     **
    **                                                                       **
    ***************************************************************************

    if( res != NULL )
      *res = KheTaskGrouperEntryMake(tg, NULL, task, task_in, new_domain,
	task_assigned_resource);
    return true;
    ** ***
    KheTaskGrouperEntryInit(next, task, ** task, ** new_domain,
      task_assigned_resource, NULL, task_in, KHE_TASK_GROUPER_ENTRY_ORDINARY,
      task_has_fixed_unassigned);
    *** **
  }
  else
  {
    ***************************************************************************
    **                                                                       **
    **  2. Interval of days must be disjoint from prev.                      **
    **                                                                       **
    ***************************************************************************

    if( !KheIntervalDisjoint(prev->interval, task_in) )
    {
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskGrouperEntryAddTask false (interval)\n");
      if( res != NULL )
	*res = NULL;
      return false;
    }
    new_in = KheIntervalUnion(prev->interval, task_in);


    ***************************************************************************
    **                                                                       **
    **  3. The intersection of task's domain with the others is non-empty.   **
    **     Also calculate the new leader task based on the intersect type.   **
    **                                                                       **
    **     Obsolete:                                                         **
    **     We allow prev->domain == NULL, and interpret it to mean that      **
    **     prev->domain contains every resource of task's resource type.     **
    **                                                                       **
    ***************************************************************************

    ** NULL domain allowed, meaning every resource of task's resource type **
    ** ***
    HnAssert(prev->domain != NULL,
      "KheTaskGrouperEntryAddTask internal error 1");
    *** **
    new_domain = KheTaskGroupDomainMake(tg->domain_finder,
      prev->domain **OK**, task_domain);
    new_rg = KheTaskGroupDomainValue(new_domain ** , &new_type **);
    if( KheResourceGroupResourceCount(new_rg) == 0 )
    {
      if( DEBUG2 )
	fprintf(stderr,"  KheTaskGrouperEntryAddTask false (empty domain 2)\n");
      if( res != NULL )
	*res = NULL;
      return false;
    }

    ** *** omitting leader now
    switch( new_type )
    {
      case KHE_TASK_GROUP_DOMAIN_RG_ONLY:
      case KHE_TASK_GROUP_DOMAIN_RG_SUBSET_PREV:

	new_leader = task;
	break;

      case KHE_TASK_GROUP_DOMAIN_PREV_SUBSET_RG:
      case KHE_TASK_GROUP_DOMAIN_PREV_INTERSECT_RG:

	new_leader = prev->leader;
	break;

      default:

	HnAbort("KheTaskGrouperEntryAddTask internal error 2");
	new_leader = NULL;  ** keep compiler happy **
	break;
    }
    *** **


    ***************************************************************************
    **                                                                       **
    **  4. Can't have conflicting resource assignments.                      **
    **                                                                       **
    **     If either the existing tasks or the new task have an assigned     **
    **     resource, that is the new shared assigned resource.   If both     **
    **     have it, they must be equal, otherwise there is no suitable       **
    **     shared assigned resource, so return false.                        **
    **                                                                       **
    **     NB this works correctly when prev is a history entry:  in that    **
    **     case, prev->assigned_resource != NULL.                            **
    **                                                                       **
    ***************************************************************************

    if( prev->assigned_resource == NULL )
      new_assigned_resource = task_assigned_resource;
    else if( task_assigned_resource == NULL )
      new_assigned_resource = prev->assigned_resource;
    else
    {
      ** both are non-NULL, so they must be equal **
      if( prev->assigned_resource != task_assigned_resource )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  KheTaskGrouperEntryAddTask false (assignment: "
	    "prev %s, task %s)\n", KheResourceId(prev->assigned_resource),
	    KheResourceId(task_assigned_resource));
	if( res != NULL )
	  *res = NULL;
	return false;
      }
      new_assigned_resource = task_assigned_resource;
    }


    ***************************************************************************
    **                                                                       **
    **  5. Intersection of domains must include any assigned resource.       **
    **                                                                       **
    ***************************************************************************

    if( new_assigned_resource != NULL &&
        !KheResourceGroupContains(new_rg, new_assigned_resource) )
    {
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskGrouperEntryAddTask false (asst domain)\n");
      if( res != NULL )
	*res = NULL;
      return false;
    }


    ***************************************************************************
    **                                                                       **
    **  X. Can't have one task with an assigned resource and another with    **
    **     a fixed non-assignment.                                           **
    **                                                                       **
    **     If we are attempting to add a fixed unassigned task to a group    **
    **     containing a task with a non-NULL parent, return false.           **
    **                                                                       **
    **     If we are attempting to add a task with a non-NULL parent to a    **
    **     group containing a fixed unassigned task, return false.           **
    **                                                                       **
    **     NB this works correctly when prev is a history entry:  in that    **
    **     case, prev->assigned_resource != NULL.                            **
    **                                                                       **
    ***************************************************************************

    ** *** withdrawn, fixed non-assignments are not allowed now
    ** cannot add a fixed unassigned task if there is an assigned task **
    if( prev->assigned_resource != NULL && task_has_fixed_unassigned )
    {
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskGrouperEntryAddTask false (fixed asst 1)\n");
      return false;
    }

    ** cannot add an assigned task if there is a fixed unassigned task **
    if( prev->has_fixed_unassigned && task_assigned_resource != NULL )
    {
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskGrouperEntryAddTask false (fixed asst 2)\n");
      return false;
    }

    new_has_fixed_unassigned = prev->has_fixed_unassigned ||
      task_has_fixed_unassigned;
    *** **


    ***************************************************************************
    **                                                                       **
    **  6. Can't have any interference.                                      **
    **                                                                       **
    ***************************************************************************

    if( new_assigned_resource != NULL )
    {
      soln = KheTaskSoln(task);
      if( prev->assigned_resource != NULL && task_assigned_resource == NULL )
      {
	** there must be no interference in task's interval **
	if( KheResourceIsBusy(prev->assigned_resource, task_in,
	      tg->days_frame, soln) )
	{
	  if( DEBUG2 )
	    fprintf(stderr, "  KheTaskGrouperEntryAddTask false (busy 1)\n");
	  if( res != NULL )
	    *res = NULL;
	  return false;
	}
      }
      else if( prev->assigned_resource == NULL && task_assigned_resource!=NULL )
      {
	** there must be no interference in prev's interval **
	if( KheResourceIsBusy(task_assigned_resource, prev->interval,
	      tg->days_frame, soln) )
	{
	  if( DEBUG2 )
	    fprintf(stderr, "  KheTaskGrouperEntryAddTask false (busy 2)\n");
	  if( res != NULL )
	    *res = NULL;
	  return false;
	}
      }
    }


    ***************************************************************************
    **                                                                       **
    **  All good so make the new object and return true.                     **
    **                                                                       **
    ***************************************************************************

    if( res != NULL )
      *res = KheTaskGrouperEntryMake(tg, prev, task, new_in, new_domain,
	new_assigned_resource);
    ** ***
    KheTaskGrouperEntryInit(next, task, ** new_leader, ** new_domain,
      new_assigned_resource, prev, new_in, KHE_TASK_GROUPER_ENTRY_ORDINARY,
      new_has_fixed_unassigned);
    *** **
    return true;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMakeHistory(                   */
/*    KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn)                         */
/*                                                                           */
/*  Make a history entry.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryMakeHistory(
  KHE_TASK_GROUPER tg, KHE_RESOURCE r, int durn)
{
  KHE_TASK_GROUP_DOMAIN new_domain;
  new_domain = KheTaskGroupDomainMake(tg->domain_finder, NULL,
    KheResourceSingletonResourceGroup(r));
  return KheTaskGrouperEntryMake(tg, NULL, NULL, KheIntervalMake(-durn, -1),
    new_domain, r);
}
*** */

/* *** old interface
void KheTaskGrouperEntryAddHistory(KHE_TASK_GROUPER_ENTRY prev,
  KHE_RESOURCE r, int durn, KHE_TASK_GROUP_DOMAIN_FINDER tgdf,
  KHE_TASK_GROUPER_ENTRY next)
{
  KHE_TASK_GROUP_DOMAIN new_domain;
  HnAssert(prev == NULL,
    "KheTaskGrouperEntryAddHistory internal error (prev != NULL)");
  new_domain = KheTaskGroupDomainMake(tgdf, NULL,
    KheResourceSingletonResourceGroup(r));
  KheTaskGrouperEntryInit(next, NULL, ** NULL, ** new_domain, r, prev,
    KheIntervalMake(-durn, -1), KHE_TASK_GROUPER_ENTRY_HISTORY, false);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperEntryFree(KHE_TASK_GROUPER tg,                        */
/*    KHE_TASK_GROUPER_ENTRY entry)                                          */
/*                                                                           */
/*  Free a task grouper entry object.  It must have previously been          */
/*  allocated by a call to KheTaskGrouperMakeEntry.                          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskGrouperEntryFree(KHE_TASK_GROUPER_ENTRY tge,
  KHE_TASK_GROUPER tg)
{
  if( tge->task != NULL && DEBUG68(tge->task) )
    fprintf(stderr, "KheTaskGrouperEntryFree(%s) freeing %p\n",
      KheTaskId(tge->task), (void *) tge);
  tge->task = NULL;  ** bug catcher **
  HaArrayAddLast(tg->task_grouper_entry_free_list, tge);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperEntryCopy(KHE_TASK_GROUPER_ENTRY dst_last,            */
/*    KHE_TASK_GROUPER_ENTRY src_last)                                       */
/*                                                                           */
/*  Overwrite the contents of dst_last with the contents of src_last.        */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KheTaskGrouperEntryCopy(KHE_TASK_GROUPER_ENTRY dst_last,
  KHE_TASK_GROUPER_ENTRY src_last)
{
  KheTaskGrouperEntryInit(dst_last, src_last->task, ** src_last->leader, **
    src_last->domain, src_last->assigned_resource, src_last->prev,
    src_last->interval, src_last->type, src_last->has_fixed_unassigned);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY KheTaskGrouperLastEntry(KHE_TASK_GROUPER tg)      */
/*                                                                           */
/*  Return tg's last entry, or NULL if none.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK_GROUPER_ENTRY KheTaskGrouperLastEntry(KHE_TASK_GROUPER tg)
{
  return tg->last_entry;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheTaskGrouperEntryCost(KHE_TASK_GROUPER_ENTRY last,            */
/*    KHE_FRAME days_frame, KHE_SOLN soln)                                   */
/*                                                                           */
/*  Return the cost of grouping last's tasks.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_COST KheTaskGrouperEntryCost(KHE_TASK_GROUPER_ENTRY last,
  KHE_TASK_GROUPER tg)
{
  KHE_RESOURCE r;  KHE_TASK_GROUPER_ENTRY entry;
  KHE_GROUP_MONITOR gm;  KHE_INTERVAL in;  KHE_RESOURCE_GROUP domain;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_MARK mark;  KHE_COST res;
  KHE_TASK_SET ts;  KHE_TASK task;  int i;
  ** KHE_TASK_GROUP_DOMAIN_TYPE type; **

  ** history groups are not monitored so have zero cost **
  ** if( last->type == KHE_TASK_GROUPER_ENTRY_HISTORY ) **
  if( last->task == NULL )
    return 0;

  ** find a suitable resource to assign the tasks to **
  HnAssert(last != NULL,
    "KheTaskGrouperEntryCost internal error:  last == NULL");
  if( last->assigned_resource != NULL )
    r = last->assigned_resource;
  else
  {
    domain = KheTaskGroupDomainValue(last->domain ** , &type **);
    HnAssert(KheResourceGroupResourceCount(domain) > 0,
      "KheTaskGrouperEntryCost internal error 1");
    r = KheResourceGroupResource(domain, 0);
  }

  ** find the interval of days_frame covered by these tasks **
  in = KheIntervalMake(KheIntervalFirst(last->interval) - 1,
    KheIntervalLast(last->interval) + 1);

  ** find the cluster busy times and limit busy times monitors during in **
  gm = KheGroupMonitorMake(tg->soln, 11, "comb grouping");
  rtm = KheResourceTimetableMonitor(tg->soln, r);
  KheResourceTimetableMonitorAddInterval(rtm, tg->days_frame,
    KheIntervalFirst(in), KheIntervalLast(in),
    last->assigned_resource != NULL, gm);

  ** make sure r is free during in **
  mark = KheMarkBegin(tg->soln);
  ts = KheTaskSetMake(tg->soln);
  KheResourceTimetableMonitorAddProperRootTasksInInterval(rtm, tg->days_frame,
    KheIntervalFirst(in), KheIntervalLast(in), true, ts);
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( KheTaskAssignIsFixed(task) )
      KheTaskAssignUnFix(task);
    if( KheTaskAsstResource(task) != NULL )
      KheTaskUnAssignResource(task);
  }
  KheTaskSetDelete(ts);

  ** assign r to the tasks of last **
  for( entry = last;  entry != NULL;  entry = entry->prev )
    if( entry->task != NULL )
    ** if( entry->type == KHE_TASK_GROUPER_ENTRY_ORDINARY ) **
    {
      HnAssert(entry->task != NULL, "KheTaskGrouperEntryCost internal error 2");
      if( KheTaskAssignIsFixed(entry->task) )
	KheTaskAssignUnFix(entry->task);
      if( KheTaskAsstResource(entry->task) != NULL )
        KheTaskUnAssignResource(entry->task);
      if( !KheTaskAssignResource(entry->task, r) )
	HnAbort("KheTaskGrouperEntryCost internal error 3");
    }

  ** work out the cost **
  res = KheMonitorCost((KHE_MONITOR) gm);
  KheGroupMonitorDelete(gm);

  ** return to the initial state, and return the cost **
  KheMarkEnd(mark, true);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskGrouperEntryMakeGroup(KHE_TASK_GROUPER_ENTRY last,       */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Make a group and return its leader task.  This must succeed.  If         */
/*  sa != NULL, record what was done in sa so that it can be undone later.   */
/*                                                                           */
/*  Implementation note.  This code does not make a special case when        */
/*  there is just one task to group, but it is easy to check that it         */
/*  does nothing in that case except return that task.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK KheTaskGrouperEntryMakeGroup(KHE_TASK_GROUPER_ENTRY last,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK_GROUPER_ENTRY entry;  bool has_fixed_child, fixed;
  KHE_RESOURCE_GROUP domain, leader_domain;
  ** KHE_TASK_GROUP_DOMAIN_TYPE type; **  KHE_TASK leader_task;
  KHE_TASK_BOUND tb;  int domain_count, leader_domain_count;

  ** find the last entry; it contains the leader task and shared parent **
  HnAssert(last != NULL,
    "KheTaskGrouperEntryMakeGroup internal error (last == NULL)");
  ** ***
  HnAssert(last->leader != NULL,
    "KheTaskGrouperEntryMakeGroup internal error (last is a history entry)");
  *** **
  if( DEBUG1 )
    fprintf(stderr, "  [ KheTaskGrouperEntryMakeGroup:\n");

  ** (0) find a leader task - one with a domain of minimum size **
  leader_task = NULL;
  leader_domain = NULL;
  leader_domain_count = INT_MAX;
  for( entry = last;  entry != NULL;  entry = entry->prev )
    if( entry->task != NULL )
    ** if( entry->type == KHE_TASK_GROUPER_ENTRY_ORDINARY ) **
    {
      domain = KheTaskDomain(entry->task);
      domain_count = KheResourceGroupResourceCount(domain);
      if( KheResourceGroupResourceCount(domain) < leader_domain_count )
      {
	leader_task = entry->task;
	leader_domain = domain;
	leader_domain_count = domain_count;
      }
    }
  if( leader_task == NULL )
    return NULL;

  ** (1) make sure that the leader task has the correct domain **
  domain = KheTaskGroupDomainValue(last->domain ** , &type **);
  if( KheResourceGroupResourceCount(domain) !=
      KheResourceGroupResourceCount(leader_domain) )
  {
    tb = KheTaskGroupDomainTaskBound(last->domain);
    KheSolnAdjusterTaskAddTaskBound(sa, leader_task, tb);
  }

  ** (2) move followers to the leader, unfixing and refixing if needed. **
  ** We call KheSolnAdjusterTaskGroup rather than KheSolnAdjusterTaskMove **
  ** so that during undo, entry->task gets moved to its parent's parent **
  has_fixed_child = false;
  for( entry = last;  entry != NULL;  entry = entry->prev )
    ** if( entry->type == KHE_TASK_GROUPER_ENTRY_ORDINARY && **
    if( entry->task != NULL && entry->task != leader_task )
    {
      fixed = KheTaskAssignIsFixed(entry->task);
      if( fixed )
      {
	has_fixed_child = true;
	KheSolnAdjusterTaskAssignUnFix(sa, entry->task);
      }
      if( DEBUG1 )
	fprintf(stderr, "    grouping %s under %s\n",
	  KheTaskId(entry->task), KheTaskId(leader_task));
      if( !KheSolnAdjusterTaskGroup(sa, entry->task, leader_task) )
	HnAbort("KheTaskGrouperEntryMakeGroup internal error:  cannot group");
      if( fixed )
	KheSolnAdjusterTaskAssignFix(sa, entry->task);
    }

  ** (3) move the leader task to the assigned resource, if not already done **
  ** We don't inform sa about this, so during undo we do nothing here **
  if( KheTaskAsstResource(leader_task) != last->assigned_resource )
  {
    HnAssert(!KheTaskAssignIsFixed(leader_task),
     "KheTaskGrouperMakeGroup internal error:  fixed leader needs move");
    if( !KheTaskAssignResource(leader_task, last->assigned_resource) )
      HnAbort("KheTaskGrouperMakeGroup internal error:  cannot move leader");
  }

  ** (3) fix the leader task's assignment, if there is a fixed follower **
  if( has_fixed_child && !KheTaskAssignIsFixed(leader_task) )
    KheSolnAdjusterTaskAssignFix(sa, leader_task);

  ** all done, return leader task **
  if( DEBUG1 )
    fprintf(stderr, "  ] KheTaskGrouperEntryMakeGroup returning\n");
  return leader_task;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperEntryAddDummy(KHE_TASK_GROUPER_ENTRY *prev,           */
/*    KHE_TASK_GROUPER_ENTRY *next)                                          */
/*                                                                           */
/*  Add a dummy entry into the sequence.  It has the same attributes as      */
/*  prev (which must be non-NULL), but it does not add a new task and it     */
/*  is ignored when making groups.                                           */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
void KheTaskGrouperEntryAddDummy(KHE_TASK_GROUPER_ENTRY prev,
  KHE_TASK_GROUPER_ENTRY next)
{
  HnAssert(prev != NULL,
    "KheTaskGrouperEntryAddDummy internal error (no prev entry)");
  HnAssert(prev->type != KHE_TASK_GROUPER_ENTRY_HISTORY,
    "KheTaskGrouperEntryAddDummy internal error (prev is a history entry)");
  KheTaskGrouperEntryInit(next, prev->task, ** prev->leader, ** prev->domain,
    prev->assigned_resource, prev, prev->interval,
    KHE_TASK_GROUPER_ENTRY_DUMMY, prev->has_fixed_unassigned);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY_TYPE KheTaskGrouperEntryType(                     */
/*    KHE_TASK_GROUPER_ENTRY tge)                                            */
/*                                                                           */
/*  Return the type of tge.                                                  */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
KHE_TASK_GROUPER_ENTRY_TYPE KheTaskGrouperEntryType(
  KHE_TASK_GROUPER_ENTRY tge)
{
  return tge->type;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryPrev(                          */
/*    KHE_TASK_GROUPER_ENTRY tge)                                            */
/*                                                                           */
/*  Return the predecessor of tge.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK_GROUPER_ENTRY KheTaskGrouperEntryPrev(KHE_TASK_GROUPER_ENTRY tge)
{
  return tge->prev;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskGrouperEntryTask(KHE_TASK_GROUPER_ENTRY entry)           */
/*                                                                           */
/*  Return the task of entry, which must not be a dummy entry.               */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK KheTaskGrouperEntryTask(KHE_TASK_GROUPER_ENTRY entry)
{
  ** *** no, we are now using this call to distinguish history cases
  HnAssert(entry->type != KHE_TASK_GROUPER_ENTRY_HISTORY,
    "KheTaskGrouperEntryTask internal error (entry is a history entry)");
  *** **
  return entry->task;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheTaskGrouperEntryInterval(KHE_TASK_GROUPER_ENTRY last)    */
/*                                                                           */
/*  Return the interval covered by the task group ending at last.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_INTERVAL KheTaskGrouperEntryInterval(KHE_TASK_GROUPER_ENTRY last)
{
  HnAssert(last != NULL,
    "KheTaskGrouperEntryInterval internal error:  last == NULL");
  return last->interval;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP_DOMAIN KheTaskGrouperEntryDomain(                         */
/*    KHE_TASK_GROUPER_ENTRY tge)                                            */
/*                                                                           */
/*  Return the task group domain of tge.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TASK_GROUP_DOMAIN KheTaskGrouperEntryDomain(KHE_TASK_GROUPER_ENTRY tge)
{
  return tge->domain;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheTaskGrouperEntryAssignedResource(                        */
/*    KHE_TASK_GROUPER_ENTRY tge)                                            */
/*                                                                           */
/*  Return the assigned resource of tge (possibly NULL).                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_RESOURCE KheTaskGrouperEntryAssignedResource(KHE_TASK_GROUPER_ENTRY tge)
{
  return tge->assigned_resource;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperEntryDebug(KHE_TASK_GROUPER_ENTRY tge,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of tge onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskGrouperEntryDebug(KHE_TASK_GROUPER_ENTRY tge,
  int verbosity, int indent, FILE *fp)
{
  if( indent < 0 )
  {
    while( tge != NULL )
    {
      if( tge->task != NULL )
	fprintf(fp, "%s", KheTaskId(tge->task));
      else
	fprintf(fp, "History(%s, %d)\n",
	  KheResourceId(tge->assigned_resource),
	  KheIntervalLength(tge->interval));
      tge = tge->prev;
      if( tge != NULL )
	fprintf(fp, " -> ");
    }
  }
  else
  {
    fprintf(fp, "%*s[ TaskGrouperEntry\n", indent, "");
    while( tge != NULL )
    {
      if( tge->task != NULL )
      {
	if( !KheTaskIsProperRoot(tge->task) )
	  fprintf(fp, "%*snon-proper-root task:\n", indent + 2, "");
	KheTaskDebug(tge->task, verbosity, indent + 2, fp);
      }
      else
	fprintf(fp, "%*sHistory(%s, %d)\n", indent + 2, "",
	  KheResourceId(tge->assigned_resource),
	  KheIntervalLength(tge->interval));
      tge = tge->prev;
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperEntryHasFixedUnassigned(KHE_TASK_GROUPER_ENTRY tge)   */
/*                                                                           */
/*  Return the has_fixed_unassigned attribute of tge.                        */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
bool KheTaskGrouperEntryHasFixedUnassigned(KHE_TASK_GROUPER_ENTRY tge)
{
  return tge->has_fixed_unassigned;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "separate group testing"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperSeparateAddTask(KHE_TASK_GROUPER tg,                  */
/*    KHE_INTERVAL prev_interval, KHE_TASK_GROUP_DOMAIN prev_domain,         */
/*    KHE_RESOURCE prev_assigned_resource, KHE_TASK task,                    */
/*    KHE_INTERVAL *new_interval, KHE_TASK_GROUP_DOMAIN *new_domain,         */
/*    KHE_RESOURCE *new_assigned_resource)                                   */
/*                                                                           */
/*  If task can be added to an existing group with attributes prev_interval, */
/*  prev_domain, and prev_assigned_resource, then set *new_interval,         */
/*  *new_domain, and *new_assigned_resource to the attributes of the new     */
/*  task group that would result from this addition and return true.         */
/*  Otherwise return false with the three return parameters set to           */
/*  don't care values.                                                       */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGrouperSeparateAddTask(KHE_TASK_GROUPER tg,
  KHE_INTERVAL prev_interval, KHE_TASK_GROUP_DOMAIN prev_domain,
  KHE_RESOURCE prev_assigned_resource, KHE_TASK task,
  KHE_INTERVAL *new_interval, KHE_TASK_GROUP_DOMAIN *new_domain,
  KHE_RESOURCE *new_assigned_resource)
{
  KHE_RESOURCE_GROUP task_domain, new_rg;  KHE_INTERVAL task_in;
  KHE_RESOURCE task_assigned_resource;

  /***************************************************************************/
  /*                                                                         */
  /*  The task must be non-NULL and a proper root task                       */
  /*                                                                         */
  /***************************************************************************/

  HnAssert(task != NULL,
    "KheTaskGrouperSeparateAddTask internal error:  task is NULL");
  if( DEBUG3 && !KheTaskIsProperRoot(task) )
  {
    fprintf(stderr, "KheTaskGrouperSeparateAddTask failing (proper root)\n");
    fprintf(stderr, "task:\n");
    KheTaskDebug(task, 2, 2, stderr);
    fprintf(stderr, "task's proper root:\n");
    KheTaskDebug(KheTaskProperRoot(task), 2, 2, stderr);
  }
  HnAssert(KheTaskIsProperRoot(task),
    "KheTaskGrouperSeparateAddTask internal error:  task not proper root task");

  /* get some basic facts about task */
  task_in = KheTaskInterval(task, tg->days_frame),
  task_domain = KheTaskDomain(task);
  task_assigned_resource = KheTaskAsstResource(task);

  /* make sure result variables are well-defined even if result is false */
  *new_interval = KheIntervalUnion(prev_interval, task_in);
  *new_domain = KheTaskGroupDomainMake(tg->domain_finder,
    prev_domain, task_domain);
  if( prev_assigned_resource == NULL )
    *new_assigned_resource = task_assigned_resource;
  else
    *new_assigned_resource = prev_assigned_resource;


  /*************************************************************************/
  /*                                                                       */
  /*  1. Task cannot have an empty domain.                                 */
  /*                                                                       */
  /*************************************************************************/

  if( KheResourceGroupResourceCount(task_domain) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(task has empty domain)\n");
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  2. Task cannot have a fixed non-assignment.                          */
  /*                                                                       */
  /*************************************************************************/

  if( KheTaskAssignIsFixed(task) && task_assigned_resource == NULL )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(task has fixed non-assignment)\n");
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  3. Interval of days must be disjoint from prev.                      */
  /*                                                                       */
  /*************************************************************************/

  if( !KheIntervalDisjoint(prev_interval, task_in) )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(task interval %s not disjoint from prev %s)\n",
	KheIntervalShow(task_in, tg->days_frame),
	KheIntervalShow(prev_interval, tg->days_frame));
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  4. The intersection of task's domain with the others is non-empty.   */
  /*                                                                       */
  /*************************************************************************/

  new_rg = KheTaskGroupDomainValue(*new_domain);
  if( KheResourceGroupResourceCount(new_rg) == 0 )
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(domain intersection is empty)\n");
      fprintf(stderr, "  prev_domain: ");
      KheTaskGroupDomainDebug(prev_domain, 2, 0, stderr);
      fprintf(stderr, "  task_domain: ");
      KheResourceGroupDebug(task_domain, 2, 0, stderr);
    }
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  5. Can't have conflicting resource assignments.                      */
  /*                                                                       */
  /*     If either the existing tasks or the new task have an assigned     */
  /*     resource, that is the new shared assigned resource.   If both     */
  /*     have it, they must be equal, otherwise there is no suitable       */
  /*     shared assigned resource, so return false.                        */
  /*                                                                       */
  /*     NB this works correctly when prev is a history entry:  in that    */
  /*     case, prev->assigned_resource != NULL.                            */
  /*                                                                       */
  /*************************************************************************/

  /* if both are non-NULL, they must be equal */
  if( prev_assigned_resource != NULL && task_assigned_resource != NULL &&
      prev_assigned_resource != task_assigned_resource )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(conflicting resource assignments: prev %s, task %s)\n",
	KheResourceId(prev_assigned_resource),
	KheResourceId(task_assigned_resource));
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  6. Intersection of domains must include any assigned resource.       */
  /*                                                                       */
  /*************************************************************************/

  if( *new_assigned_resource != NULL &&
      !KheResourceGroupContains(new_rg, *new_assigned_resource) )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	"(domain does not include assignment)\n");
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  7. Can't have any interference.                                      */
  /*                                                                       */
  /*************************************************************************/

  if( *new_assigned_resource != NULL )
  {
    if( prev_assigned_resource != NULL && task_assigned_resource == NULL )
    {
      /* there must be no interference in task's interval */
      if( KheResourceIsBusy(prev_assigned_resource, task_in,
	    tg->days_frame, tg->soln) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	    "(interference 1)\n");
	return false;
      }
    }
    else if( prev_assigned_resource == NULL && task_assigned_resource != NULL )
    {
      /* there must be no interference in prev's interval */
      if( KheResourceIsBusy(task_assigned_resource, prev_interval,
	    tg->days_frame, tg->soln) )
      {
	if( DEBUG2 )
	  fprintf(stderr, "  KheTaskGrouperSeparateAddTask returning false "
	    "(interference 2)\n");
	return false;
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /*  All good so return true.                                             */
  /*                                                                       */
  /*************************************************************************/

  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGrouperSeparateAddInitialTask(KHE_TASK_GROUPER tg,           */
/*    KHE_TASK task, KHE_INTERVAL *new_interval,                             */
/*    KHE_TASK_GROUP_DOMAIN *new_domain, KHE_RESOURCE *new_assigned_resource)*/
/*                                                                           */
/*  Like KheTaskGrouperSeparateAddTask but with no previous group.           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGrouperSeparateAddInitialTask(KHE_TASK_GROUPER tg,
  KHE_TASK task, KHE_INTERVAL *new_interval,
  KHE_TASK_GROUP_DOMAIN *new_domain, KHE_RESOURCE *new_assigned_resource)
{
  KHE_RESOURCE_GROUP task_domain;


  /***************************************************************************/
  /*                                                                         */
  /*  The task must be non-NULL and a proper root task                       */
  /*                                                                         */
  /***************************************************************************/

  HnAssert(task != NULL,
    "KheTaskGrouperSeparateAddInitialTask internal error:  task is NULL");
  HnAssert(KheTaskIsProperRoot(task),
    "KheTaskGrouperSeparateAddInitialTask internal error:  task is not "
    "a proper root task");

  /* get some basic facts about task */
  *new_interval = KheTaskInterval(task, tg->days_frame),
  task_domain = KheTaskDomain(task);
  *new_assigned_resource = KheTaskAsstResource(task);


  /*************************************************************************/
  /*                                                                       */
  /*  1. Task cannot have an empty domain.                                 */
  /*                                                                       */
  /*************************************************************************/

  if( KheResourceGroupResourceCount(task_domain) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddInitialTask returning false "
	"(task has empty domain)\n");
    return false;
  }
  *new_domain = KheTaskGroupDomainMake(tg->domain_finder, NULL, task_domain);


  /*************************************************************************/
  /*                                                                       */
  /*  2. Task cannot have a fixed non-assignment.                          */
  /*                                                                       */
  /*************************************************************************/

  if( KheTaskAssignIsFixed(task) && *new_assigned_resource == NULL )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheTaskGrouperSeparateAddInitialTask returning false "
	"(task has fixed non-assignment)\n");
    return false;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  3. Interval of days must be disjoint from prev.                      */
  /*                                                                       */
  /*************************************************************************/

  /* nothing to do here */


  /*************************************************************************/
  /*                                                                       */
  /*  4. The intersection of task's domain with prev is non-empty.         */
  /*                                                                       */
  /*************************************************************************/

  /* nothing to do here */


  /*************************************************************************/
  /*                                                                       */
  /*  5. Can't have conflicting resource assignments.                      */
  /*                                                                       */
  /*************************************************************************/

  /* nothing to do here */


  /*************************************************************************/
  /*                                                                       */
  /*  6. Intersection of domains must include any assigned resource.       */
  /*                                                                       */
  /*************************************************************************/

  /* nothing to do here */


  /*************************************************************************/
  /*                                                                       */
  /*  7. Can't have any interference.                                      */
  /*                                                                       */
  /*************************************************************************/

  /* nothing to do here */


  /*************************************************************************/
  /*                                                                       */
  /*  All good so return true.                                             */
  /*                                                                       */
  /*************************************************************************/

  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGrouperSeparateAddHistory(KHE_TASK_GROUPER tg,               */
/*    KHE_RESOURCE r, int durn, KHE_INTERVAL *new_interval,                  */
/*    KHE_TASK_GROUP_DOMAIN *new_domain,                                     */
/*    KHE_RESOURCE *new_assigned_resource)                                   */
/*                                                                           */
/*  Return the three attributes of a separate task group consisting          */
/*  entirely of the history entry (r, durn).                                 */
/*                                                                           */
/*****************************************************************************/

void KheTaskGrouperSeparateAddHistory(KHE_TASK_GROUPER tg,
  KHE_RESOURCE r, int durn, KHE_INTERVAL *new_interval,
  KHE_TASK_GROUP_DOMAIN *new_domain,
  KHE_RESOURCE *new_assigned_resource)
{
  *new_interval = KheIntervalMake(-durn, -1);
  *new_domain = KheTaskGroupDomainMake(tg->domain_finder, NULL,
    KheResourceSingletonResourceGroup(r));
  *new_assigned_resource = r;
}

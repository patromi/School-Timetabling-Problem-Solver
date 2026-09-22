
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
/*  FILE:         khe_resource_in_soln.c                                     */
/*  DESCRIPTION:  A resource as viewed from within a solution                */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#include <float.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_WORKLOAD_REQUIREMENT - one workload requirement                      */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
struct khe_workload_requirement_rec {
  int			count;			** number required           **
  KHE_TIME_GROUP	time_group;		** time group required       **
  KHE_MONITOR		originating_monitor;	** originating monitor       **
  ARRAY_KHE_WORKLOAD_REQUIREMENT children;	** child requirements        **
};
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN - a resource monitor                                */
/*                                                                           */
/*  Implementation note.  All monitors, attached and unattached, and         */
/*  including workload demand monitors, go on the all_monitors list.         */
/*  But only attached non-workload monitors go on the attached_monitors      */
/*  list.  Attached workload demand monitors go only into their chunks.      */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_in_soln_rec {
  KHE_SOLN			soln;			/* encl. solution    */
  KHE_RESOURCE			resource;		/* monitored resource*/
  ARRAY_KHE_TASK		tasks;			/* assigned resource */
  KHE_TASK_BOUND		singleton_task_bound;	/* task bound just r */
  int				busy_times;		/* of tasks          */
  float				workload;		/* of tasks          */
  int				max_busy_times;		/* cached            */
  float				max_workload;		/* cached            */
  KHE_RESOURCE_TIMETABLE_MONITOR timetable_monitor;	/* its timetable     */
  ARRAY_KHE_MONITOR		all_monitors;		/* all monitors      */
  ARRAY_KHE_MONITOR		attached_monitors;	/* attached monitors */
  /* ARRAY_KHE_WORKLOAD_REQUIREMENT	workload_requirements; */
  ARRAY_KHE_TIME_GROUP			workload_time_groups;
  ARRAY_KHE_MATCHING_DEMAND_CHUNK	workload_demand_chunks;
  /* bool				workload_requirements_active; */
  KHE_RESOURCE_IN_SOLN		copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* KHE_RESOURCE_IN_SOLN KheResourceInSolnMake(KHE_SOLN soln, KHE_RESOURCE r) */
/*                                                                           */
/*  Make a new resource in soln for soln that monitors r, initially with     */
/*  no monitors except the timetable, and that is unattached anyway.         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheResourceInSolnMake(KHE_SOLN soln, KHE_RESOURCE r)
{
  KHE_RESOURCE_IN_SOLN res;  HA_ARENA a;
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->soln = soln;
  res->resource = r;
  HaArrayInit(res->tasks, a);
  res->singleton_task_bound = NULL;
  res->busy_times = 0;
  res->workload = 0.0;
  res->max_busy_times = -1;	/* negative signals not calculated yet */
  res->max_workload = -1.0;	/* negative signals not calculated yet */
  res->timetable_monitor = KheResourceTimetableMonitorMake(soln, res);
  HaArrayInit(res->all_monitors, a);
  HaArrayInit(res->attached_monitors, a);
  /* HaArrayInit(res->workload_requirements, a); */
  HaArrayInit(res->workload_time_groups, a);
  HaArrayInit(res->workload_demand_chunks, a);
  /* res->workload_requirements_active = false; */
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheResourceInSolnCopyPhase1(KHE_RESOURCE_IN_SOLN rs,*/
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of copying rs.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_WORKLOAD_REQUIREMENT KheWorkloadRequirementCopy(
  KHE_WORKLOAD_REQUIREMENT wr, HA_ARENA a)
{
  KHE_WORKLOAD_REQUIREMENT res;
  HaMake(res, a);
  res->count = wr->count;
  res->time_group = KheTimeGroupCopy(wr->time_group, a);
  res->originating_monitor = (wr->originating_monitor == NULL ? NULL :
    KheMonitorCopyPhase1(wr->originating_monitor, a));
  HaArrayInit(res->children, a);
  return res;
}
*** */

KHE_RESOURCE_IN_SOLN KheResourceInSolnCopyPhase1(KHE_RESOURCE_IN_SOLN rs,
  HA_ARENA a)
{
  KHE_RESOURCE_IN_SOLN copy;  KHE_MONITOR m;  KHE_TASK task;  int i;
  /* KHE_WORKLOAD_REQUIREMENT wr; */  KHE_MATCHING_DEMAND_CHUNK dc;
  KHE_TIME_GROUP tg;
  if( rs->copy == NULL )
  {
    HaMake(copy, a);
    rs->copy = copy;
    copy->soln = KheSolnCopyPhase1(rs->soln);
    copy->resource = rs->resource;
    HaArrayInit(copy->tasks, a);
    HaArrayForEach(rs->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, a));
    copy->singleton_task_bound = (rs->singleton_task_bound == NULL ? NULL :
      KheTaskBoundCopyPhase1(rs->singleton_task_bound, a));
    copy->busy_times = rs->busy_times;
    copy->workload = rs->workload;
    copy->timetable_monitor =
      KheResourceTimetableMonitorCopyPhase1(rs->timetable_monitor, a);
    HaArrayInit(copy->all_monitors, a);
    HaArrayForEach(rs->all_monitors, m, i)
      HaArrayAddLast(copy->all_monitors, KheMonitorCopyPhase1(m, a));
    HaArrayInit(copy->attached_monitors, a);
    HaArrayForEach(rs->attached_monitors, m, i)
      HaArrayAddLast(copy->attached_monitors, KheMonitorCopyPhase1(m, a));
    /* ***
    HaArrayInit(copy->workload_requirements, a);
    HaArrayForEach(rs->workload_requirements, wr, i)
      HaArrayAddLast(copy->workload_requirements,
	KheWorkloadRequirementCopy(wr, a));
    *** */
    HaArrayInit(copy->workload_time_groups, a);
    HaArrayForEach(rs->workload_time_groups, tg, i)
      HaArrayAddLast(copy->workload_time_groups,
	KheTimeGroupCopy(tg, a));
    HaArrayInit(copy->workload_demand_chunks, a);
    HaArrayForEach(rs->workload_demand_chunks, dc, i)
      HaArrayAddLast(copy->workload_demand_chunks,
	KheMatchingDemandChunkCopyPhase1(dc, a));
    /* ***
    HnAssert(!rs->workload_requirements_active,
      "KheSolnCopy called when currently adding workload requirements");
    copy->workload_requirements_active = false;
    *** */
    copy->copy = NULL;
  }
  return rs->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnCopyPhase2(KHE_RESOURCE_IN_SOLN rs)                */
/*                                                                           */
/*  Carry out Phase 2 of copying rs.                                         */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnCopyPhase2(KHE_RESOURCE_IN_SOLN rs)
{
  KHE_TASK task;  KHE_MONITOR m;  int i;  KHE_MATCHING_DEMAND_CHUNK dc;
  /* KHE_TIME_GROUP tg; */
  if( rs->copy != NULL )
  {
    rs->copy = NULL;
    HaArrayForEach(rs->tasks, task, i)
      KheTaskCopyPhase2(task);
    if( rs->singleton_task_bound != NULL )
      KheTaskBoundCopyPhase2(rs->singleton_task_bound);
    KheResourceTimetableMonitorCopyPhase2(rs->timetable_monitor);
    HaArrayForEach(rs->all_monitors, m, i)
      KheMonitorCopyPhase2(m);
    HaArrayForEach(rs->attached_monitors, m, i)
      KheMonitorCopyPhase2(m);
    /* ***
    HaArrayForEach(rs->workload_time_groups, tg, i)
      KheTimeGroupCopyPhase2(tg);
    *** */
    HaArrayForEach(rs->workload_demand_chunks, dc, i)
      KheMatchingDemandChunkCopyPhase2(dc);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheResourceInSolnSoln(KHE_RESOURCE_IN_SOLN rs)                  */
/*                                                                           */
/*  Return the solution that rs is a part of.                                */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheResourceInSolnSoln(KHE_RESOURCE_IN_SOLN rs)
{
  return rs->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceInSolnResource(KHE_RESOURCE_IN_SOLN rs)          */
/*                                                                           */
/*  Return the resource that rs is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheResourceInSolnResource(KHE_RESOURCE_IN_SOLN rs)
{
  return rs->resource;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceInSolnId(KHE_RESOURCE_IN_SOLN rs)                       */
/*                                                                           */
/*  Return the Id of rs's resource, or "-" if none.                          */
/*                                                                           */
/*****************************************************************************/

char *KheResourceInSolnId(KHE_RESOURCE_IN_SOLN rs)
{
  return KheResourceId(rs->resource)==NULL ? "-" : KheResourceId(rs->resource);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheResourceInSolnSingletonTaskBound(                      */
/*    KHE_RESOURCE_IN_SOLN rs)                                               */
/*                                                                           */
/*  Return a task bound whose resource group contains just rs->resource.     */
/*  Make this task bound the first time it's needed.                         */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheResourceInSolnSingletonTaskBound(KHE_RESOURCE_IN_SOLN rs)
{
  KHE_RESOURCE_GROUP rg;
  if( rs->singleton_task_bound == NULL )
  {
    rg = KheResourceSingletonResourceGroup(rs->resource);
    rs->singleton_task_bound = KheTaskBoundMake(rs->soln, rg);
  }
  return rs->singleton_task_bound;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnDelete(KHE_RESOURCE_IN_SOLN rs)                    */
/*                                                                           */
/*  Delete rs.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceInSolnWorkloadRequirementDelete(
  KHE_WORKLOAD_REQUIREMENT wr);

void KheResourceInSolnDelete(KHE_RESOURCE_IN_SOLN rs)
{
  int i;  KHE_MATCHING_DEMAND_CHUNK dc;

  ** tasks should be deleted by now **
  HnAssert(HaArrayCount(rs->tasks) == 0,
    "KheResourceInSolnDelete internal error");
  MArrayFree(rs->tasks);

  ** monitors deleted separately; just free the arrays here **
  MArrayFree(rs->all_monitors);
  MArrayFree(rs->attached_monitors);

  ** free the workload requirements **
  while( HaArrayCount(rs->workload_requirements) > 0 )
    KheResourceInSolnWorkloadRequirementDelete(
      MArrayRemoveLast(rs->workload_requirements));

  ** free the workload demand chunks (each should be empty by now) **
  HaArrayForEach(rs->workload_demand_chunks, dc, i)
    KheMatchingDemandChunkDelete(dc);
  MArrayFree(rs->workload_demand_chunks);

  MFree(rs);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnAttachMonitor(KHE_RESOURCE_IN_SOLN rs,             */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Attach m (not a workload demand monitor) to rs.                          */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnAttachMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m)
{
  KHE_TASK task;  int i;
  switch( KheMonitorTag(m) )
  {
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */

      /* attach directly to rs */
      HaArrayAddLast(rs->attached_monitors, m);
      HaArrayForEach(rs->tasks, task, i)
	KheMonitorAssignResource(m, task, rs->resource);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      HnAbort("KheResourceInSolnAttachMonitor not accepting limit workload");
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      /* attach to timetable, which will be attached first */
      KheResourceTimetableMonitorAttachAvoidClashesMonitor(
	rs->timetable_monitor, (KHE_AVOID_CLASHES_MONITOR) m);
      /* *** this bit done by KheTimetableMonitorAttachMonitor
      if( !KheMonitorAttachedToSoln((KHE_MONITOR) rs->timetable_monitor) )
        KheMonitorAttachToSoln((KHE_MONITOR) rs->timetable_monitor);
      KheTimetableMonitorAttachMonitor(rs->timetable_monitor, m);
      *** */
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    default:

      /* unexpected monitor type */
      HnAbort("KheResourceInSolnAttachMonitor unexpected monitor tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnDetachMonitor(KHE_RESOURCE_IN_SOLN rs,             */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Detach m (not a workload demand monitor) from rs.                        */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnDetachMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m)
{
  KHE_TASK task;  int i, pos;
  switch( KheMonitorTag(m) )
  {
    case KHE_RESOURCE_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */

      /* detach directly from rs */
      if( !HaArrayContains(rs->attached_monitors, m, &pos) )
	HnAbort("KheResourceInSolnDetachMonitor internal error");
      HaArrayDeleteAndShift(rs->attached_monitors, pos);
      HaArrayForEach(rs->tasks, task, i)
	KheMonitorUnAssignResource(m, task, rs->resource);
      break;

    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:

      HnAbort("KheResourceInSolnDetachMonitor not accepting limit workload");
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:

      /* detach from timetable monitor */
      KheResourceTimetableMonitorDetachAvoidClashesMonitor(
	rs->timetable_monitor, (KHE_AVOID_CLASHES_MONITOR) m);
      /* KheTimetableMonitorDetachMonitor(rs->timetable_monitor, m); */
      break;

    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_LINK_EVENTS_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    default:

      HnAbort("KheResourceInSolnDetachMonitor unexpected monitor tag");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnSplitTask(KHE_RESOURCE_IN_SOLN rs,                 */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Inform rs that task1 and task2 (which rs's resource is assigned to) are  */
/*  splitting.  Set the assign index of task2.                               */
/*                                                                           */
/*  Implementation note.  Splitting does not change the number of busy       */
/*  times or the total workload of rs's tasks.                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnSplitTask(KHE_RESOURCE_IN_SOLN rs,
  KHE_TASK task1, KHE_TASK task2)
{
  KHE_MONITOR m;  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceInSolnSplitTask(%s)\n",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-");
  HaArrayAddLast(rs->tasks, task2);
  KheTaskSetAssignedIndex(task2, HaArrayCount(rs->tasks) - 1);
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorSplitTask(m, task1, task2);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnSplitTask\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnMergeTask(KHE_RESOURCE_IN_SOLN rs,                 */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Inform rs that task1 and task2 (which rs's resource is assigned to) are  */
/*  merging.                                                                 */
/*                                                                           */
/*  Implementation note.  Merging does not change the number of busy         */
/*  times or the total workload of rs's tasks.                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnMergeTask(KHE_RESOURCE_IN_SOLN rs,
  KHE_TASK task1, KHE_TASK task2)
{
  KHE_MONITOR m;  int i, pos;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceInSolnMergeTask(%s)\n",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-");
  pos = KheTaskAssignedIndex(task2);
  HnAssert(HaArray(rs->tasks, pos) == task2,
    "KheResourceInSolnMergeTask internal error");
  HaArrayDeleteAndPlug(rs->tasks, pos);
  if( pos < HaArrayCount(rs->tasks) )
    KheTaskSetAssignedIndex(HaArray(rs->tasks, pos), pos);
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorMergeTask(m, task1, task2);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnMergeTask\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnAssignResource(KHE_RESOURCE_IN_SOLN rs,            */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Inform rs that its resource has just been assigned to task.              */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnAssignResource(KHE_RESOURCE_IN_SOLN rs, KHE_TASK task)
{
  KHE_MONITOR m;  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceInSolnAssignResource(%s)\n",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-");
  HaArrayAddLast(rs->tasks, task);
  KheTaskSetAssignedIndex(task, HaArrayCount(rs->tasks) - 1);
  rs->busy_times += KheTaskDuration(task);
  rs->workload += KheTaskWorkload(task);
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorAssignResource(m, task, rs->resource);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnAssignResource\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnUnAssignResource(KHE_RESOURCE_IN_SOLN rs,          */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Inform rs that its resource has just been unassigned from task.          */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnUnAssignResource(KHE_RESOURCE_IN_SOLN rs, KHE_TASK task)
{
  KHE_MONITOR m;  int i, pos;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceInSolnUnAssignResource(%s)\n",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-");
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorUnAssignResource(m, task, rs->resource);
  pos = KheTaskAssignedIndex(task);
  HnAssert(HaArray(rs->tasks, pos) == task,
    "KheResourceInSolnUnAssignResource internal error");
  HnAssert(pos >= 0, "KheResourceInSolnUnAssignResource internal error");
  HaArrayDeleteAndPlug(rs->tasks, pos);
  if( pos < HaArrayCount(rs->tasks) )
    KheTaskSetAssignedIndex(HaArray(rs->tasks, pos), pos);
  rs->busy_times -= KheTaskDuration(task);
  rs->workload -= KheTaskWorkload(task);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnUnssignResource\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnAssignTime(KHE_RESOURCE_IN_SOLN rs,                */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Inform rs that the solution event above task has just been assigned the  */
/*  time with this index, while task is currently assigned rs's resource.    */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnAssignTime(KHE_RESOURCE_IN_SOLN rs,
  KHE_TASK task, int assigned_time_index)
{
  KHE_MONITOR m;  int i;
  if( DEBUG1 )
    fprintf(stderr, "[ KheResourceInSolnAssignTime(%s, task, %d)\n",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-",
      assigned_time_index);
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorTaskAssignTime(m, task, assigned_time_index);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnAssignTime\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnUnAssignTime(KHE_RESOURCE_IN_SOLN rs,              */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Inform rs that the solution event above task has just been unassigned    */
/*  the time with this index, while task is currently assigned rs's resource.*/
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnUnAssignTime(KHE_RESOURCE_IN_SOLN rs,
  KHE_TASK task, int assigned_time_index)
{
  KHE_MONITOR m;  int i;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheResourceInSolnUnAssignTime(%s, ",
      KheResourceId(rs->resource) != NULL ? KheResourceId(rs->resource) : "-");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  HaArrayForEach(rs->attached_monitors, m, i)
    KheMonitorTaskUnAssignTime(m, task, assigned_time_index);
  if( DEBUG1 )
    fprintf(stderr, "] KheResourceInSolnUnAssignTime\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "matching"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInSolnWorkloadRequirementCount(KHE_RESOURCE_IN_SOLN rs)   */
/*                                                                           */
/*  Return the number of workload requirements stored in rs.                 */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
int KheResourceInSolnWorkloadRequirementCount(KHE_RESOURCE_IN_SOLN rs)
{
  return HaArrayCount(rs->workload_requirements);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnWorkloadRequirement(KHE_RESOURCE_IN_SOLN rs,       */
/*    int i, int *num, KHE_TIME_GROUP *tg, KHE_MONITOR *m)                   */
/*                                                                           */
/*  Return the i'th workload requirement stored in rs.                       */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
void KheResourceInSolnWorkloadRequirement(KHE_RESOURCE_IN_SOLN rs,
  int i, int *num, KHE_TIME_GROUP *tg, KHE_MONITOR *m)
{
  KHE_WORKLOAD_REQUIREMENT wr;
  wr = HaArray(rs->workload_requirements, i);
  *num = wr->count;
  *tg = wr->time_group;
  *m = wr->originating_monitor;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnWorkloadRequirementDelete(                         */
/*    KHE_WORKLOAD_REQUIREMENT wr)                                           */
/*                                                                           */
/*  Delete wr.                                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheResourceInSolnWorkloadRequirementDelete(
  KHE_WORKLOAD_REQUIREMENT wr)
{
  MArrayFree(wr->children);
  MFree(wr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnBeginWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs) */
/*                                                                           */
/*  Begin adding workload requirements to rs.                                */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
void KheResourceInSolnBeginWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs)
{
  HnAssert(!rs->workload_requirements_active,
    "KheSolnMatchingBeginWorkloadRequirements called out of order");
  KheResourceInSolnDeleteWorkloadRequirements(rs);
  ** ***
  while( HaArrayCount(rs->workload_requirements) > 0 )
    KheSolnAddWorkloadRequirementToFreeList(rs->soln,
      HaArrayLastAndDelete(rs->workload_requirements));
  *** **
  rs->workload_requirements_active = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnAddWorkloadRequirement(KHE_RESOURCE_IN_SOLN rs,    */
/*    int num, KHE_TIME_GROUP tg, KHE_MONITOR m)                             */
/*                                                                           */
/*  Append a workload requirement to the workload requirements stored in rs. */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
void KheResourceInSolnAddWorkloadRequirement(KHE_RESOURCE_IN_SOLN rs,
  int num, KHE_TIME_GROUP tg, KHE_MONITOR m)
{
  KHE_WORKLOAD_REQUIREMENT wr;  HA_ARENA a;
  HnAssert(rs->workload_requirements_active,
    "KheSolnMatchingAddWorkloadRequirement called out of order");
  HnAssert(num >= 0 && num <= KheTimeGroupTimeCount(tg),
    "KheSolnMatchingAddWorkloadRequirement invalid num");
  wr = KheSolnGetWorkloadRequirementFromFreeList(rs->soln);
  if( wr == NULL )
  {
    a = KheSolnArena(rs->soln);
    HaMake(wr, a);
    HaArrayInit(wr->children, a);
  }
  else
    HaArrayClear(wr->children);
  wr->count = num;
  wr->time_group = tg;
  wr->originating_monitor = m;
  HaArrayAddLast(rs->workload_requirements, wr);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWorkloadRequirementInsertIntoForest(                             */
/*    ARRAY_KHE_WORKLOAD_REQUIREMENT *roots, KHE_WORKLOAD_REQUIREMENT wr)    */
/*                                                                           */
/*  Insert wr into roots.                                                    */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
static void KheWorkloadRequirementInsertIntoForest(
  ARRAY_KHE_WORKLOAD_REQUIREMENT *roots, KHE_WORKLOAD_REQUIREMENT wr)
{
  int touch_count, superset_count, i;  KHE_WORKLOAD_REQUIREMENT touch_wr, wr2;

  ** find out how many roots wr touches and is a superset of **
  touch_count = 0;
  touch_wr = NULL;
  superset_count = 0;
  HaArrayForEach(*roots, wr2, i)
    if( !KheTimeGroupDisjoint(wr->time_group, wr2->time_group) )
    {
      touch_count++;
      touch_wr = wr2;
      if( KheTimeGroupSubset(wr2->time_group, wr->time_group) )
	superset_count++;
    }

  if( superset_count == touch_count )
  {
    ** wr is a superset of everything it touches; move them to below wr **
    HaArrayForEach(*roots, wr2, i)
      if( KheTimeGroupSubset(wr2->time_group, wr->time_group) )
      {
	HaArrayAddLast(wr->children, wr2);
	HaArrayDeleteAndShift(*roots, i);
	i--;
      }
    HaArrayAddLast(*roots, wr);
  }
  else if( touch_count == 1 &&
      KheTimeGroupSubset(wr->time_group, touch_wr->time_group) )
  {
    ** wr belongs below touch_wr **
    KheWorkloadRequirementInsertIntoForest(&touch_wr->children, wr);
  }
  else
  {
    ** no other legal possibilities, so we abandon wr here **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheWorkloadRequirementToDemandNodes(KHE_WORKLOAD_REQUIREMENT wr,     */
/*    KHE_RESOURCE_IN_SOLN rs)                                               */
/*                                                                           */
/*  Traverse the tree rooted at wr in postorder, adding demand chunks and    */
/*  nodes to rs as required, and returning the number of nodes added by      */
/*  wr and its descendants.                                                  */
/*                                                                           */
/*  Implementation note.  There is one chunk for each workload requirement   */
/*  giving rise to at least one demand node:                                 */
/*                                                                           */
/*     dc->impl       time group of requirement                              */
/*     dc->base       0                                                      */
/*     dc->increment  resource_count                                         */
/*     dc->domain     indexes of time group times                            */
/*                                                                           */
/*     dn->impl       resource                                               */
/*     dn->domain     indexes of resource's singleton resource group         */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
static int KheWorkloadRequirementToDemandNodes(KHE_WORKLOAD_REQUIREMENT wr,
  KHE_RESOURCE_IN_SOLN rs)
{
  int child_count, root_count, i, resource_count;  KHE_MATCHING m;
  KHE_WORKLOAD_REQUIREMENT child_wr;  KHE_MATCHING_DEMAND_CHUNK dc;
  KHE_WORKLOAD_DEMAND_MONITOR wdm;

  ** build demand nodes in proper descendants and count how many **
  child_count = 0;
  HaArrayForEach(wr->children, child_wr, i)
    child_count += KheWorkloadRequirementToDemandNodes(child_wr, rs);

  ** find out how many we need here at the root **
  root_count = KheTimeGroupTimeCount(wr->time_group) - wr->count - child_count;
  if( root_count < 0 )
    root_count = 0;

  ** add one chunk with root_count demand nodes, if root_count > 0 **
  if( root_count > 0 )
  {
    m = KheSolnMatching(rs->soln);
    HnAssert(m != NULL, "KheWorkloadRequirementToDemandNodes: no matching");
    resource_count = KheInstanceResourceCount(KheSolnInstance(rs->soln));
    dc = KheMatchingDemandChunkMake(m, NULL, 0, resource_count,
      KheTimeGroupTimeSet(wr->time_group));
    HaArrayAddLast(rs->workload_demand_chunks, dc);
    for( i = 0;  i < root_count;  i++ )
    {
      wdm = KheWorkloadDemandM onitorMake(rs->soln, dc, rs, wr->time_group,
	wr->originating_monitor);
      KheGroupMonitorAddChildMonitor((KHE_GROUP_MONITOR) rs->soln,
	(KHE_MONITOR) wdm);
      KheMonitorAttachToSoln((KHE_MONITOR) wdm);
    }
  }

  ** return the total number of demand nodes added at wr or below it **
  return child_count + root_count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnEndWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs)   */
/*                                                                           */
/*  End adding workload requirements to rs.  This is the signal for          */
/*  calculating the demand chunks and nodes.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
void KheResourceInSolnEndWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs)
{
  ARRAY_KHE_WORKLOAD_REQUIREMENT roots;  int i;  KHE_WORKLOAD_REQUIREMENT wr;
  KHE_MONITOR m;  HA_ARENA a;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheResourceInSolnEndWorkloadRequirements(%s)\n",
      KheResourceId(rs->resource) == NULL ? "-" : KheResourceId(rs->resource));
    fprintf(stderr, "  initial monitors:\n");
    HaArrayForEach(rs->all_monitors, m, i)
      KheMonitorDebug(m, 2, 4, stderr);
    fprintf(stderr, "  initial workload requirements:\n");
    HaArrayForEach(rs->workload_requirements, wr, i)
    {
      fprintf(stderr, "    %d ", wr->count);
      KheTimeGroupDebug(wr->time_group, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
  }
  HnAssert(KheSolnMatching(rs->soln) != NULL,
    "KheResourceInSolnEndWorkloadRequirements called when no matching");
  HnAssert(rs->workload_requirements_active,
    "KheSolnMatchingEndWorkloadRequirements called out of order");
  rs->workload_requirements_active = false;

  ** build the forest **
  a = KheSolnArena(rs->soln);
  HaArrayInit(roots, a);
  HaArrayForEach(rs->workload_requirements, wr, i)
  {
    HaArrayClear(wr->children);
    KheWorkloadRequirementInsertIntoForest(&roots, wr);
  }

  ** traverse the forest, adding workload demand chunks and nodes **
  HaArrayForEach(roots, wr, i)
    KheWorkloadRequirementToDemandNodes(wr, rs);
  HaArrayFree(roots);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final monitors:\n");
    HaArrayForEach(rs->all_monitors, m, i)
      KheMonitorDebug(m, 2, 4, stderr);
    fprintf(stderr, "] KheResourceInSolnEndWorkloadRequirements returning\n");
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnDeleteWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs)*/
/*                                                                           */
/*  Delete rs's workload requirements and workload demand nodes.             */
/*                                                                           */
/*****************************************************************************/

/* *** moved to khe_sm_matching.c
void KheResourceInSolnDeleteWorkloadRequirements(KHE_RESOURCE_IN_SOLN rs)
{
  KHE_MONITOR m;  int i;

  ** delete old workload requirements **
  while( HaArrayCount(rs->workload_requirements) > 0 )
    KheSolnAddWorkloadRequirementToFreeList(rs->soln,
      HaArrayLastAndDelete(rs->workload_requirements));

  ** delete old workload demand monitors **
  HaArrayForEach(rs->all_monitors, m, i)
    if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
    {
      KheWorkload DemandMonitorDelete((KHE_WORKLOAD_DEMAND_MONITOR) m);
      i--;
    }

  ** delete old workload demand chunks **
  while( HaArrayCount(rs->workload_demand_chunks) > 0 )
    KheMatchingDemandChunkDelete(HaArrayLastAndDelete(rs->workload_demand_chunks));

}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MATCHING_DEMAND_CHUNK KheResourceInSolnDemandChunk(                  */
/*    KHE_RESOURCE_IN_SOLN rs, KHE_TIME_GROUP tg)                            */
/*                                                                           */
/*  Return a demand chunk for rs and tg.  Make a new one if necessary.       */
/*                                                                           */
/*****************************************************************************/

KHE_MATCHING_DEMAND_CHUNK KheResourceInSolnDemandChunk(
  KHE_RESOURCE_IN_SOLN rs, KHE_TIME_GROUP tg)
{
  KHE_MATCHING_DEMAND_CHUNK dc;  int i, resource_count;  KHE_TIME_GROUP tg2;
  KHE_MATCHING m;

  /* make sure all good */
  m = KheSolnMatching(rs->soln);
  HnAssert(m != NULL, "KheWorkloadDemandMonitorMake: no matching");

  /* return an existing chunk, if there is one for tg */
  HaArrayForEach(rs->workload_time_groups, tg2, i)
    if( KheTimeGroupEqual(tg, tg2) )
      return HaArray(rs->workload_demand_chunks, i);

  /* no existing chunk, so add one and return it */
  HaArrayAddLast(rs->workload_time_groups, tg);
  resource_count = KheInstanceResourceCount(KheSolnInstance(rs->soln));
  dc = KheMatchingDemandChunkMake(m, NULL, 0, resource_count,
    KheTimeGroupTimeSet(tg));
  HaArrayAddLast(rs->workload_demand_chunks, dc);
  return dc;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnDeleteDemandChunks(KHE_RESOURCE_IN_SOLN rs)        */
/*                                                                           */
/*  Delete the demand chunks of rs.  They should all be empty by now.        */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnDeleteDemandChunks(KHE_RESOURCE_IN_SOLN rs)
{
  KHE_MATCHING_DEMAND_CHUNK wdc;
  while( HaArrayCount(rs->workload_demand_chunks) > 0 )
  {
    wdc = HaArrayLastAndDelete(rs->workload_demand_chunks);
    KheMatchingDemandChunkDelete(wdc);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnMatchingSetWeight(KHE_RESOURCE_IN_SOLN rs,         */
/*    KHE_COST new_weight)                                                   */
/*                                                                           */
/*  Update the attached matching demand monitors of rs to reflect the fact   */
/*  that their weight is changing to new_weight.                             */
/*                                                                           */
/*  Implementation note.  Array all_monitors contains all monitors, but we   */
/*  want just the attached demand monitors, so we traverse their chunks.     */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnMatchingSetWeight(KHE_RESOURCE_IN_SOLN rs,
  KHE_COST new_weight)
{
  int i, j;  KHE_MATCHING_DEMAND_CHUNK dc;  KHE_WORKLOAD_DEMAND_MONITOR m;
  HaArrayForEach(rs->workload_demand_chunks, dc, i)
    for( j = 0;  j < KheMatchingDemandChunkNodeCount(dc);  j++ )
    {
      m = (KHE_WORKLOAD_DEMAND_MONITOR) KheMatchingDemandChunkNode(dc, j);
      KheWorkloadDemandMonitorSetWeight(m, new_weight);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "assigned tasks"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInSolnAssignedTaskCount(KHE_RESOURCE_IN_SOLN rs)          */
/*                                                                           */
/*  Return the number of assigned tasks in rs.                               */
/*                                                                           */
/*****************************************************************************/

int KheResourceInSolnAssignedTaskCount(KHE_RESOURCE_IN_SOLN rs)
{
  return HaArrayCount(rs->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheResourceInSolnAssignedTask(KHE_RESOURCE_IN_SOLN rs, int i)   */
/*                                                                           */
/*  Return the i'th assigned task of rs.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheResourceInSolnAssignedTask(KHE_RESOURCE_IN_SOLN rs, int i)
{
  return HaArray(rs->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "resource availability"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceInSolnMaxBusyTimes(KHE_RESOURCE_IN_SOLN rs, int *res)    */
/*                                                                           */
/*  If there is a non-trivial maximum limit on the total number of busy      */
/*  times of rs in its instance, return true and set *res to that number,    */
/*  otherwise return false and set *res to INT_MAX.                          */
/*                                                                           */
/*  Implementation note.  This function depends only on the instance, so     */
/*  its value is cached in the max_busy_times field of rs.                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceInSolnMaxBusyTimes(KHE_RESOURCE_IN_SOLN rs,
  KHE_AVAIL_SOLVER as, int *res)
{
  if( rs->max_busy_times < 0 )
  {
    /* value not calculated yet, do it now */
    /* KheAvailSolverSetResource(as, rs->resource); */
    KheAvailSolverMaxBusyTimes(as, rs->resource, &rs->max_busy_times);
  }
  *res = rs->max_busy_times;
  return *res < INT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceInSolnMaxWorkload(KHE_RESOURCE_IN_SOLN rs, float *res)   */
/*                                                                           */
/*  If there is a non-trivial maximum limit on the total workload of rs      */
/*  in its instance, return true and set *res to that number, otherwise      */
/*  return false and set *res to FLT_MAX.                                    */
/*                                                                           */
/*  Implementation note.  This function depends only on the instance, so     */
/*  its value is cached in the max_workload field of rs.                     */
/*                                                                           */
/*****************************************************************************/

bool KheResourceInSolnMaxWorkload(KHE_RESOURCE_IN_SOLN rs,
  KHE_AVAIL_SOLVER as, float *res)
{
  if( DEBUG3 )
    fprintf(stderr, "[ KheResourceInSolnMaxWorkload(%s, as, -)\n",
      KheResourceId(rs->resource));
  if( rs->max_workload < 0.0 )
  {
    /* value not calculated yet, do it now */
    if( DEBUG3 )
      fprintf(stderr, "  calling KheAvailSolverSetResource(as, %s)\n",
        KheResourceId(rs->resource));
    /* KheAvailSolverSetResource(as, rs->resource); */
    KheAvailSolverMaxWorkload(as, rs->resource, &rs->max_workload);
  }
  *res = rs->max_workload;
  if( DEBUG3 )
  {
    if( *res < FLT_MAX )
      fprintf(stderr, "] KheResourceInSolnMaxWorkload returning true (%.1f)\n",
	*res);
    else
      fprintf(stderr, "] KheResourceInSolnMaxWorkload returning false\n");
  }
  return *res < FLT_MAX;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInSolnBusyTimes(KHE_RESOURCE_IN_SOLN rs)                  */
/*                                                                           */
/*  Return the total duration of the tasks assigned rs.                      */
/*                                                                           */
/*****************************************************************************/

int KheResourceInSolnBusyTimes(KHE_RESOURCE_IN_SOLN rs)
{
  return rs->busy_times;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheResourceInSolnWorkload(KHE_RESOURCE_IN_SOLN rs)                 */
/*                                                                           */
/*  Return the total workload of the tasks assigned rs.                      */
/*                                                                           */
/*****************************************************************************/

float KheResourceInSolnWorkload(KHE_RESOURCE_IN_SOLN rs)
{
  return rs->workload;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceInSolnAvailableBusyTimes(KHE_RESOURCE_IN_SOLN rs,        */
/*    KHE_AVAIL_SOLVER as, int *res)                                         */
/*                                                                           */
/*  Return the available busy times of rs.                                   */
/*                                                                           */
/*****************************************************************************/

bool KheResourceInSolnAvailableBusyTimes(KHE_RESOURCE_IN_SOLN rs,
  KHE_AVAIL_SOLVER as, int *res)
{
  int max_busy_times;
  if( KheResourceInSolnMaxBusyTimes(rs, as, &max_busy_times) )
    return *res = max_busy_times - rs->busy_times, true;
  else
    return *res = INT_MAX, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceInSolnAvailableWorkload(KHE_RESOURCE_IN_SOLN rs,         */
/*    KHE_AVAIL_SOLVER as, float *res)                                       */
/*                                                                           */
/*  Return the available workload of rs.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheResourceInSolnAvailableWorkload(KHE_RESOURCE_IN_SOLN rs,
  KHE_AVAIL_SOLVER as, float *res)
{
  float max_workload;
  if( KheResourceInSolnMaxWorkload(rs, as, &max_workload) )
    return *res = max_workload - rs->workload, true;
  else
    return *res = FLT_MAX, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "user monitors, cost, and timetables"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnAddMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m) */
/*                                                                           */
/*  Add m to the user-accessible list of monitors of rs.                     */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnAddMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m)
{
  HaArrayAddLast(rs->all_monitors, m);
}


/*****************************************************************************/
/*                                                                           */
/*void KheResourceInSolnDeleteMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m)*/
/*                                                                           */
/*  Delete m from rs's all_monitors list.                                    */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnDeleteMonitor(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR m)
{
  int pos;
  if( !HaArrayContains(rs->all_monitors, m, &pos) )
    HnAbort("KheResourceInSolnDeleteMonitor internal error");
  HaArrayDeleteAndShift(rs->all_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceInSolnMonitorCount(KHE_RESOURCE_IN_SOLN rs)               */
/*                                                                           */
/*  Return the number of user-accessible monitors in rs.                     */
/*                                                                           */
/*****************************************************************************/

int KheResourceInSolnMonitorCount(KHE_RESOURCE_IN_SOLN rs)
{
  return HaArrayCount(rs->all_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheResourceInSolnMonitor(KHE_RESOURCE_IN_SOLN rs, int i)     */
/*                                                                           */
/*  Return the i'th user-accessible monitor of rs.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheResourceInSolnMonitor(KHE_RESOURCE_IN_SOLN rs, int i)
{
  return HaArray(rs->all_monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnMonitorSort(KHE_RESOURCE_IN_SOLN rs,               */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the user-accessible monitors in rs using compar.                    */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnMonitorSort(KHE_RESOURCE_IN_SOLN rs,
  int(*compar)(const void *, const void *))
{
  HaArraySort(rs->all_monitors, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheResourceInSolnCost(KHE_RESOURCE_IN_SOLN rs)                  */
/*                                                                           */
/*  Return the total cost of all monitors.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheResourceInSolnCost(KHE_RESOURCE_IN_SOLN rs)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(rs->all_monitors, m, i)
    res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheResourceInSolnMonitorCost(KHE_RESOURCE_IN_SOLN rs,           */
/*    KHE_MONITOR_TAG tag)                                                   */
/*                                                                           */
/*  Return the total cost of monitors with this tag.                         */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheResourceInSolnMonitorCost(KHE_RESOURCE_IN_SOLN rs, KHE_MONITOR_TAG tag)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(rs->all_monitors, m, i)
    if( KheMonitorTag(m) == tag )
      res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheResourceInSolnTimetableMonitor(        */
/*    KHE_RESOURCE_IN_SOLN rs)                                               */
/*                                                                           */
/*  Return the timetable of rs.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TIMETABLE_MONITOR KheResourceInSolnTimetableMonitor(
  KHE_RESOURCE_IN_SOLN rs)
{
  return rs->timetable_monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceInSolnDebug(KHE_RESOURCE_IN_SOLN rs, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of resource monitor rs onto fp with the given verbosity      */
/*  and indent.                                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceInSolnDebug(KHE_RESOURCE_IN_SOLN rs, int verbosity,
  int indent, FILE *fp)
{
  KHE_MONITOR m;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
    {
      fprintf(fp, "%*s[ Resource %s\n", indent, "", KheResourceInSolnId(rs));
      HaArrayForEach(rs->attached_monitors, m, i)
	KheMonitorDebug(m, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, "%s", KheResourceInSolnId(rs));
  }
}

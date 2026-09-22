
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
/*  FILE:         khe_sr_group_by_resource.c                                 */
/*  DESCRIPTION:  Group by resource                                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define DEBUG1 0

typedef enum {
  KHE_RGR_STATE_NOT_IN_RUN,
  KHE_RGR_STATE_IN_RUN
} KHE_RGR_STATE;


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupContainsTask(KHE_RESOURCE_TIMETABLE_MONITOR rtm,        */
/*    KHE_TIME_GROUP tg, KHE_TASK *task)                                     */
/*                                                                           */
/*  If rtm contains a task running during time group tg, set *task to        */
/*  one such task and return true.  Otherwise set *task to NULL and          */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupContainsTask(KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_TIME_GROUP tg, KHE_TASK *task)
{
  int i;  KHE_TIME time;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    time = KheTimeGroupTime(tg, i);
    if( KheResourceTimetableMonitorTimeTaskCount(rtm, time) > 0 )
    {
      *task = KheResourceTimetableMonitorTimeTask(rtm, time, 0);
      return *task = KheTaskProperRoot(*task), true;
    }
  }
  return *task = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RGR_STATE KheStartRun(KHE_TASK_GROUPER tg, KHE_TASK task)            */
/*                                                                           */
/*  Start a run and return the in-run state.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_RGR_STATE KheStartRun(KHE_TASK_GROUPER tg, KHE_TASK task)
{
  KheTaskGrouperClear(tg);
  KheTaskGrouperAddTask(tg, task);  /* first task must succeed */
  return KHE_RGR_STATE_IN_RUN;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RGR_STATE KheStopRun(KHE_TASK_GROUPER tg, KHE_SOLN_ADJUSTER sa)      */
/*                                                                           */
/*  Stop a run and return the not-in-run state.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_RGR_STATE KheStopRun(KHE_TASK_GROUPER tg, KHE_SOLN_ADJUSTER sa)
{
  KheTaskGrouperMakeGroup(tg, sa);
  return KHE_RGR_STATE_NOT_IN_RUN;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceGroupByResource(KHE_SOLN soln, KHE_RESOURCE r,           */
/*    KHE_FRAME frame, KHE_TASK_SET ts, bool *soln_changed)                  */
/*                                                                           */
/*  Find maximal sets of tasks assigned to r in soln which lie in            */
/*  adjacent time groups of frame, and ensure that for each set the          */
/*  tasks of that set are assigned to a shared leader task.                  */
/*                                                                           */
/*  If any assignments were changed, set *soln_changed to true.  If          */
/*  ts != NULL, add to ts every task that is assigned to a leader task.      */
/*                                                                           */
/*****************************************************************************/

static void KheResourceGroupByResource(KHE_SOLN soln, KHE_RESOURCE r,
  KHE_FRAME frame, KHE_TASK_GROUPER tg, KHE_SOLN_ADJUSTER sa,
  bool *soln_changed)
{
  KHE_TASK prev_task, task;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  KHE_TIME_GROUP time_group;  int i;  KHE_RGR_STATE state;
  rtm = KheResourceTimetableMonitor(soln, r);
  prev_task = NULL;
  state = KHE_RGR_STATE_NOT_IN_RUN;
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    time_group = KheFrameTimeGroup(frame, i);
    switch( state )
    {
      case KHE_RGR_STATE_NOT_IN_RUN:

	if( KheTimeGroupContainsTask(rtm, time_group, &task) )
	{
	  /* starting a run here */
          state = KheStartRun(tg, task);
	  prev_task = task;
	}
	break;

      case KHE_RGR_STATE_IN_RUN:

	if( !KheTimeGroupContainsTask(rtm, time_group, &task) )
	{
	  /* ending a run here */
          state = KheStopRun(tg, sa);
	}
	else if( task == prev_task )
	{
	  /* in run but task is already added, so do nothing and keep in run */
	}
	else if( KheTaskGrouperAddTask(tg, task) )
	{
	  /* in run and added task, so keep in run */
	  prev_task = task;
	}
	else
	{
	  /* failed to add task, so end this run and start another */
          state = KheStopRun(tg, sa);
          state = KheStartRun(tg, task);
	  prev_task = task;
	}
	break;

      default:

	HnAbort("KheResourceGroupByResource internal error");
	break;
    }
  }
  if( state == KHE_RGR_STATE_IN_RUN )
    state = KheStopRun(tg, sa);
}


/* *** old version
static void KheResourceGroupByResource(KHE_SOLN soln, KHE_RESOURCE r,
KHE_FRAME frame, KHE_SOLN_ADJUSTER sa, bool *soln_changed)
{
KHE_TASK leader_task, task;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
KHE_TIME_GROUP tg;  KHE_TIME t;  KHE_RESOURCE r2;
int i, j, k, tg_task_count;
rtm = KheResourceTimetableMonitor(soln, r);
leader_task = NULL;
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    tg_task_count = 0;
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	task = KheTaskProperRoot(task);
	if( !KheTaskIsPreassigned(task, &r2) && task != leader_task )
	{
	  if( leader_task == NULL )
	  {
	    tg_task_count++;
	    leader_task = task;
	  }
	  else if( KheSolnAdjusterTaskGroup(sa, task, leader_task) )
	  {
	    tg_task_count++;
	    *soln_changed = true;
	  }
	  else if( KheSolnAdjusterTaskGroup(sa, leader_task, task) )
	  {
	    leader_task = task;
	    tg_task_count++;
	    *soln_changed = true;
	  }
	}
      }
    }
    if( tg_task_count == 0 )
      leader_task = NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeGroupByResource(KHE_SOLN soln,                       */
/*    KHE_RESOURCE_TYPE rt, KHE_FRAME frame, KHE_TASK_GROUPER tg,            */
/*    KHE_SOLN_ADJUSTER sa, bool *changed)                                   */
/*                                                                           */
/*  Carry out grouping by resource for each resource in rt.                  */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTypeGroupByResource(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, KHE_FRAME frame, KHE_TASK_GROUPER tg,
  KHE_SOLN_ADJUSTER sa, bool *changed)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    KheResourceGroupByResource(soln, r, frame, tg, sa, changed);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheGroupByResource(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,             */
/*    KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)                             */
/*                                                                           */
/*  Group the tasks of soln of type rt by resource.  If sa != NULL, add any  */
/*  changes to sa so that they can be undone later.  Return true if any      */
/*  assignments were made.                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheGroupByResource(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, KHE_SOLN_ADJUSTER sa)
{
  KHE_INSTANCE ins;  KHE_FRAME common_frame;  bool changed;  int i;
  HA_ARENA a;  KHE_TASK_GROUPER tg;
  ins = KheSolnInstance(soln);
  if( DEBUG1 )
    fprintf(stderr, "[ KheGroupByResource(%s %s, -)\n",
      KheInstanceId(ins),
      rt == NULL ? "all resource types" : KheResourceTypeId(rt));
  common_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  changed = false;
  a = KheSolnArenaBegin(soln);
  tg = KheTaskGrouperMake(a);
  if( rt != NULL )
    KheResourceTypeGroupByResource(soln, rt, common_frame, tg, sa, &changed);
  else
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      KheResourceTypeGroupByResource(soln, rt, common_frame, tg, sa, &changed);
    }
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheGroupByResource returning %s\n",
      changed ? "true" : "false");
  return changed;
}

/* *** old version with old-style interfacce
bool KheTaskingGro upByResource(KHE_TASKING tasking, KHE_OPTIONS options,
  KHE_TASK_SET ts)
{
  KHE_SOLN soln;  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  KHE_FRAME common_frame;  bool soln_changed;  int i;
  soln = KheTaskingSoln(tasking);
  ins = KheSolnInstance(soln);
  rt = KheTaskingResourceType(tasking);
  if( DEBUG1 )
    fprintf(stderr, "[ KheTaskingGr oupByResource(%s %s, -)\n",
      KheInstanceId(ins),
      rt == NULL ? "all resource types" : KheResourceTypeId(rt));
  common_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  soln_changed = false;
  if( rt != NULL )
    KheResourceTypeGroupByResource(soln, rt, common_frame, ts, &soln_changed);
  else
    for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
    {
      rt = KheInstanceResourceType(ins, i);
      KheResourceTypeGroupByResource(soln, rt, common_frame, ts, &soln_changed);
    }
  if( DEBUG1 )
    fprintf(stderr, "] KheTask ingGroupByResource returning %s\n",
      soln_changed ? "true" : "false");
  return soln_changed;
}
*** */

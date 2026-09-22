
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
  KHE_FRAME frame, KHE_TASK_SET ts, bool *soln_changed)
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
	  else if( KheTaskMove(task, leader_task) )
	  {
	    tg_task_count++;
	    *soln_changed = true;
	    if( ts != NULL )
	      KheTaskSetAddTask(ts, task);
	  }
	  else if( KheTaskMove(leader_task, task) )
	  {
	    leader_task = task;
	    tg_task_count++;
	    *soln_changed = true;
	    if( ts != NULL )
	      KheTaskSetAddTask(ts, task);
	  }
	}
      }
    }
    if( tg_task_count == 0 )
      leader_task = NULL;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTypeGroupByResource(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, */
/*    KHE_FRAME frame, KHE_TASK_SET ts, bool *soln_changed)                  */
/*                                                                           */
/*  Carry out grouping by resource for each resource in rt.                  */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTypeGroupByResource(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_FRAME frame, KHE_TASK_SET ts, bool *soln_changed)
{
  int i;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    KheResourceGroupByResource(soln, r, frame, ts, soln_changed);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingGroupByResource(KHE_TASKING tasking, KHE_OPTIONS options, */
/*    KHE_TASK_SET ts)                                                       */
/*                                                                           */
/*  Group the tasks of tasking by resource.  If ts != NULL, add any tasks    */
/*  that get assigned to ts.  Return true if any assignments were made.      */
/*                                                                           */
/*****************************************************************************/

bool KheTaskingGroupByResource(KHE_TASKING tasking, KHE_OPTIONS options,
  KHE_TASK_SET ts)
{
  KHE_SOLN soln;  KHE_RESOURCE_TYPE rt;  KHE_INSTANCE ins;
  KHE_FRAME common_frame;  bool soln_changed;  int i;
  soln = KheTaskingSoln(tasking);
  ins = KheSolnInstance(soln);
  rt = KheTaskingResourceType(tasking);
  if( DEBUG1 )
    fprintf(stderr, "[ KheTaskingGroupByResource(%s %s, -)\n",
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
    fprintf(stderr, "] KheTaskingGroupByResource returning %s\n",
      soln_changed ? "true" : "false");
  return soln_changed;
}

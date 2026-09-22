
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
/*  FILE:         khe_sr_unnecessary.c                                       */
/*  DESCRIPTION:  Moving unnecessary assignments                             */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_MOVE_UNNECESSARY_SOLVER - solver for moving unnecessary assignments  */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

typedef struct khe_move_unnecessary_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_OPTIONS			options;
  KHE_FRAME			days_frame;
  ARRAY_KHE_RESOURCE		underloaded_resources;
  ARRAY_KHE_RESOURCE		overloaded_resources;
} *KHE_MOVE_UNNECESSARY_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MOVE_UNNECESSARY_SOLVER KheMoveUnnecessarySolverMake(                */
/*    KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)  */
/*                                                                           */
/*  Make a move unnecessary assignment solver with these attributes.         */
/*                                                                           */
/*****************************************************************************/

static KHE_MOVE_UNNECESSARY_SOLVER KheMoveUnnecessarySolverMake(
  KHE_SOLN soln, KHE_RESOURCE_TYPE rt, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_MOVE_UNNECESSARY_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  HaArrayInit(res->underloaded_resources, a);
  HaArrayInit(res->overloaded_resources, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsOverloaded(KHE_SOLN soln, KHE_RESOURCE r)              */
/*                                                                           */
/*  Return true if r is overloaded.                                          */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsOverloaded(KHE_SOLN soln, KHE_RESOURCE r)
{
  int avail_busy_times;  float avail_workload;
  if( KheResourceAvailableBusyTimes(soln, r, &avail_busy_times) &&
      avail_busy_times < 0 )
    return true;
  if( KheResourceAvailableWorkload(soln, r, &avail_workload) &&
      avail_workload < 0.0 )
    return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTrySwap(KHE_TASK task, KHE_RESOURCE task_r,                      */
/*    KHE_TASK_SET ts, KHE_RESOURCE ts_r)                                    */
/*                                                                           */
/*  Try swapping task, which is initially assigned task_r, with ts, which    */
/*  initially assigned ts_r; return true if successful.                      */
/*                                                                           */
/*****************************************************************************/

static bool KheTrySwap(KHE_TASK task, KHE_RESOURCE task_r,
  KHE_TASK_SET ts, KHE_RESOURCE ts_r)
{
  KHE_COST init_cost;  KHE_SOLN soln;  KHE_MARK mark;  bool success;
  soln = KheTaskSoln(task);
  init_cost = KheSolnCost(soln);
  mark = KheMarkBegin(soln);
  success = KheTaskMoveResource(task, ts_r) &&
    KheTaskSetMoveResource(ts, task_r) &&
    KheSolnCost(soln) <= init_cost;
  KheMarkEnd(mark, !success);
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryToMoveTask(KHE_TASK task, KHE_RESOURCE r,                     */
/*    KHE_MOVE_UNNECESSARY_SOLVER mus)                                       */
/*                                                                           */
/*  Try to move task (which is unnecessarily assigned non-overloaded         */
/*  resource r) to an overloaded resource.                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTryToMoveTask(KHE_TASK task, KHE_RESOURCE r,
  KHE_MOVE_UNNECESSARY_SOLVER mus)
{
  KHE_INTERVAL in;  KHE_RESOURCE r2;  int i;  KHE_TASK_SET ts;

  in = KheTaskInterval(task, mus->days_frame);
  ts = KheTaskSetMake(mus->soln);
  HaArrayForEach(mus->overloaded_resources, r2, i)
  {
    KheTaskSetClear(ts);
    KheAddResourceProperRootTasksInInterval(r2, in, mus->soln,
      mus->days_frame, ts);
    if( KheTrySwap(task, r, ts, r2) )
    {
      if( DEBUG1 )
      {
	fprintf(stderr, "  KheMoveUnnecessary (%s, %s): %s <--> ",
	  KheResourceId(r), KheResourceId(r2), KheTaskId(task));
	KheTaskSetDebug(ts, 1, 0, stderr);
      }
      return true;
    }
  }
  KheTaskSetDelete(ts);

  /* if we get here, no luck */
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMoveUnnecessaryAssignments(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,  */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Move unnecessary assignments from resources that are not overloaded      */
/*  to resources that are overloaded.                                        */
/*                                                                           */
/*****************************************************************************/

void KheMoveUnnecessaryAssignments(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  HA_ARENA a;  KHE_MOVE_UNNECESSARY_SOLVER mus;  KHE_RESOURCE r;  int i, j;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TASK_SET ts;  KHE_TASK task;
  KHE_TIME_GROUP full_tg;

  /* get an arena and a new solver */
  if( DEBUG1 )
    fprintf(stderr, "[ KheMoveUnnecessaryAssignments(soln, %s, options)\n",
      KheResourceTypeId(rt));
  a = KheSolnArenaBegin(soln);
  mus = KheMoveUnnecessarySolverMake(soln, rt, options, a);

  /* sort out non-overloaded and overloaded resources */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( KheResourceIsOverloaded(soln, r) )
      HaArrayAddLast(mus->overloaded_resources, r);
    else
      HaArrayAddLast(mus->underloaded_resources, r);
  }

  /* try to swap unnecessary assignments from each non-overloaded resource */
  ts = KheTaskSetMake(soln);
  full_tg = KheInstanceFullTimeGroup(KheSolnInstance(soln));
  HaArrayForEach(mus->underloaded_resources, r, i)
  {
    rtm = KheResourceTimetableMonitor(soln, r);
    KheTaskSetClear(ts);
    KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm,
      full_tg, true, ts);
    for( j = 0;  j < KheTaskSetTaskCount(ts);  j++ )
    {
      task = KheTaskSetTask(ts, j);
      if( !KheTaskNeedsAssignment(task) )
      {
	if( KheTryToMoveTask(task, r, mus) )
	{
	  KheTaskSetDeleteTask(ts, task);
	  j--;
	}
      }
    }
  }
  KheTaskSetDelete(ts);

  /* all done */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheMoveUnnecessaryAssignments returning\n");
}

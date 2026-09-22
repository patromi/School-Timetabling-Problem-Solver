
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
/*  FILE:         khe_sr_kempe.c                                             */
/*  DESCRIPTION:  Kempe and ejecting task moves                              */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_priqueue.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheTaskSetMove"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r)                     */
/*                                                                           */
/*  Move the tasks of ts to r.  But don't move any preassigned tasks.        */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r)
{
  int i;  KHE_TASK task;  ** KHE_RESOURCE r2; **
  if( KheTaskSetTaskCount(ts) == 0 )
    return false;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    ** ***
    if( KheTaskIsPreassigned(task, &r2) )
      return false;
    task = KheTaskFirstUnFixed(task);
    *** **
    if( !KheTaskMoveResource(task, r) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheTaskUnAssignClashing and KheTaskUnAssignClashingFrame"     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeUnAssignClashing(KHE_TIME t, KHE_TASK task,                  */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,                  */
/*    KHE_TASK_SET ts, int ts_mark)                                          */
/*                                                                           */
/*  Unassign all tasks running at time t in rtm.  Abort if any of these      */
/*  are task.  Return true if successful, or false if allow_eject is         */
/*  false and at least one task was unassigned, or a task from before        */
/*  ts_mark in ts was unassigned.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeUnAssignClashing(KHE_TIME t, KHE_TASK task,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,
  KHE_TASK_SET ts, int ts_mark)
{
  KHE_TASK task2;  int pos;
  while( KheResourceTimetableMonitorTimeTaskCount(rtm, t) > 0 )
  {
    if( !allow_eject )
      return false;
    task2 = KheResourceTimetableMonitorTimeTask(rtm, t, 0);
    task2 = KheTaskProperRoot(task2);
    /* task2 = KheTaskFirstUnFixed(task2); */
    HnAssert(task2 != task, "KheTaskUnAssignClashingFrame internal error");
    if( task2 == NULL || KheTaskIsPreassigned(task2, NULL) ||
	!KheTaskUnAssign(task2) )
      return false;
    if( DEBUG1 )
    {
      fprintf(stderr, "  unassigning ");
      KheTaskDebug(task2, 1, 0, stderr);
    }
    if( ts != NULL )
    {
      if( !KheTaskSetContainsTask(ts, task2, &pos) )
	KheTaskSetAddTask(ts, task2);
      else if( pos < ts_mark )
	return false;
      /* else do nothing */
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssignClashing(KHE_TASK task,                              */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,                  */
/*    KHE_TASK_SET ts, int ts_mark)                                          */
/*                                                                           */
/*  Unassign every task in rtm that clashes with task, and return true;      */
/*  or if one of those tasks is preassigned r or cannot be unassigned,       */
/*  return false.                                                            */
/*                                                                           */
/*  If ts != NULL, ensure that every unassigned task lies in ts.  If it is   */
/*  already in ts, then if its index precedes ts_mark, return false; it has  */
/*  been moved previously and cannot move again.                             */
/*                                                                           */
/*  It is a precondition that task cannot lie in rtm.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskUnAssignClashing(KHE_TASK task,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,
  KHE_TASK_SET ts, int ts_mark)
{
  int i;  KHE_TASK child_task;  KHE_MEET meet;  KHE_TIME t, time;

  /* unassign tasks that clash with task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAsstTime(meet) != NULL )
  {
    /* clear every time of meet's times */
    time = KheMeetAsstTime(meet);
    for( i = 0;  i < KheMeetDuration(meet);  i++ )
    {
      t = KheTimeNeighbour(time, i);
      if( !KheTimeUnAssignClashing(t, task, rtm, allow_eject, ts, ts_mark) )
	return false;
    }
  }

  /* unassign tasks that clash with task's child tasks */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheTaskUnAssignClashing(child_task, rtm, allow_eject, ts, ts_mark) )
      return false;
  }

  /* return true since no problems arose */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskUnAssignClashingFrame(KHE_TASK task,                         */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,                  */
/*    KHE_FRAME frame, KHE_TASK_SET ts, int ts_mark)                         */
/*                                                                           */
/*  Unassign every task in rtm that clashes with task, and return true;      */
/*  or if one of those tasks is preassigned r or cannot be unassigned,       */
/*  return false.                                                            */
/*                                                                           */
/*  If ts != NULL, ensure that every unassigned task lies in ts.  If it is   */
/*  already in ts, then if its index precedes ts_mark, return false; it has  */
/*  been moved previously and cannot move again.                             */
/*                                                                           */
/*  It is a precondition that task cannot lie in rtm.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskUnAssignClashingFrame(KHE_TASK task,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, bool allow_eject,
  KHE_FRAME frame, KHE_TASK_SET ts, int ts_mark)
{
  int i, j, index;  KHE_TASK child_task;  KHE_MEET meet;
  KHE_TIME t, time;  KHE_TIME_GROUP tg;

  /* unassign tasks that clash with task itself */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAsstTime(meet) != NULL )
  {
    /* clear every time that meet's times share a time group with in frame */
    time = KheMeetAsstTime(meet);
    for( i = 0;  i < KheMeetDuration(meet);  i++ )
    {
      index = KheFrameTimeIndex(frame, KheTimeNeighbour(time, i));
      tg = KheFrameTimeGroup(frame, index);
      for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
      {
	t = KheTimeGroupTime(tg, j);
	if( !KheTimeUnAssignClashing(t, task, rtm, allow_eject, ts, ts_mark) )
	{
	  if( DEBUG2 )
	    fprintf(stderr, "  KheTaskUnAssignClashingFrame ret. false\n");
	  return false;
	}
      }
    }
  }

  /* unassign tasks that clash with task's child tasks */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheTaskUnAssignClashingFrame(child_task, rtm, allow_eject, frame,
	  ts, ts_mark) )
    {
      if( DEBUG2 )
	fprintf(stderr, "  KheTaskUnAssignClashingFrame ret. false (2)\n");
      return false;
    }
  }

  /* return true since no problems arose */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ejecting task and task set moves"                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectingTaskMove(KHE_TASK task, KHE_RESOURCE r, bool allow_eject)*/
/*                                                                           */
/*  Carry out an ejecting task move of task to r.                            */
/*                                                                           */
/*****************************************************************************/

bool KheEjectingTaskMove(KHE_TASK task, KHE_RESOURCE r, bool allow_eject)
{
  bool res;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheEjectingTaskMove(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %s, %s)\n", KheResourceId(r), bool_show(allow_eject));
  }

  /* fail if r is currently assigned to task */
  /* task = KheTaskFirstUnFixed(task); */
  if( KheTaskAsstResource(task) == r )
  {
    if( DEBUG5 )
      fprintf(stderr, "] KheEjectingTaskMove ret. false (nochange)\n");
    return false;
  }

  /* if r is NULL, just an unassignment */
  if( r == NULL )
  {
    res = KheTaskUnAssign(task);
    if( DEBUG5 )
      fprintf(stderr, "] KheEjectingTaskMove returning %s (unassign)\n",
	bool_show(res));
    return res;
  }

  /* find r's timetable monitor and ensure that it is attached */
  rtm = KheResourceTimetableMonitor(KheTaskSoln(task), r);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm);

  /* do the move */
  res = KheTaskUnAssignClashing(task, rtm, allow_eject, NULL, 0)
    && KheTaskMoveResource(task, r);
  if( DEBUG5 )
    fprintf(stderr, "] KheEjectingTaskMove returning %s\n",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectingTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,             */
/*    bool allow_eject, KHE_FRAME frame)                                     */
/*                                                                           */
/*  Carry out an ejecting task move of task to r, using frame to identify    */
/*  incompatible tasks.                                                      */
/*                                                                           */
/*****************************************************************************/

bool KheEjectingTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,
  bool allow_eject, KHE_FRAME frame)
{
  bool res;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  if( DEBUG5 )
  {
    fprintf(stderr, "[ KheEjectingTaskMoveFrame(");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %s, %s, frame)\n", KheResourceId(r),
      bool_show(allow_eject));
  }

  /* fail if r is currently assigned to task */
  /* task = KheTaskFirstUnFixed(task); */
  if( KheTaskAsstResource(task) == r )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheEjectingTaskMoveFrame ret. false (nochange)\n");
    if( DEBUG5 )
      fprintf(stderr, "] KheEjectingTaskMoveFrame ret. false (nochange)\n");
    return false;
  }

  /* if r is NULL, just an unassignment */
  if( r == NULL )
  {
    res = KheTaskUnAssign(task);
    if( DEBUG2 && !res )
      fprintf(stderr, "  KheEjectingTaskMoveFrame ret. false (unassign)\n");
    if( DEBUG5 )
      fprintf(stderr, "] KheEjectingTaskMoveFrame returning %s (unassign)\n",
	bool_show(res));
    return res;
  }

  /* find r's timetable monitor and ensure that it is attached */
  rtm = KheResourceTimetableMonitor(KheTaskSoln(task), r);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm);

  /* do the move */
  res = KheTaskUnAssignClashingFrame(task, rtm, allow_eject, frame, NULL, 0)
    && KheTaskMoveResource(task, r);
  if( DEBUG2 && !res )
    fprintf(stderr, "  KheEjectingTaskMoveFrame ret. false (final)\n");
  if( DEBUG5 )
    fprintf(stderr, "] KheEjectingTaskMoveFrame returning %s\n",
      bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectingTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,             */
/*    bool allow_eject)                                                      */
/*                                                                           */
/*  Carry out ejecting task moves of the tasks of ts and return true         */
/*  if all successful.                                                       */
/*                                                                           */
/*****************************************************************************/

bool KheEjectingTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,
  bool allow_eject)
{
  int i;  KHE_TASK task;
  if( KheTaskSetTaskCount(ts) == 0 )
    return false;
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( !KheEjectingTaskMove(task, r, allow_eject) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEjectingTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,        */
/*    bool allow_eject, KHE_FRAME frame)                                     */
/*                                                                           */
/*  Carry out ejecting task moves of the tasks of ts and return true         */
/*  if all successful.                                                       */
/*                                                                           */
/*****************************************************************************/

bool KheEjectingTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,
  bool allow_eject, KHE_FRAME frame)
{
  int i;  KHE_TASK task;
  if( KheTaskSetTaskCount(ts) == 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "  KheEjectingTaskSetMoveFrame ret. false (empty)\n");
    return false;
  }
  for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    if( !KheEjectingTaskMoveFrame(task, r, allow_eject, frame) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Kempe task and task set moves"                                */
/*                                                                           */
/*****************************************************************************/
#define swap(a, b, tmp) ((tmp) = (a), (a) = (b), (b) = (tmp))

/*****************************************************************************/
/*                                                                           */
/*  bool KheKempeTaskMove(KHE_TASK task, KHE_RESOURCE r1)                    */
/*                                                                           */
/*  Carry out a Kempe task move of task to r1.                               */
/*                                                                           */
/*****************************************************************************/

bool KheKempeTaskMove(KHE_TASK task, KHE_RESOURCE r1)
{
  KHE_TASK_SET ts;  bool res;
  ts = KheTaskSetMake(KheTaskSoln(task));
  KheTaskSetAddTask(ts, task);
  res = KheKempeTaskSetMove(ts, r1);
  KheTaskSetDelete(ts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheKempeTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r1,               */
/*    KHE_FRAME frame)                                                       */
/*                                                                           */
/*  Carry out a Kempe task move of task to r, using frame to identify        */
/*  incompatible tasks.                                                      */
/*                                                                           */
/*****************************************************************************/

bool KheKempeTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r1, KHE_FRAME frame)
{
  KHE_TASK_SET ts;  bool res;
  ts = KheTaskSetMake(KheTaskSoln(task));
  KheTaskSetAddTask(ts, task);
  res = KheKempeTaskSetMoveFrame(ts, r1, frame);
  KheTaskSetDelete(ts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheKempeTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r1)               */
/*                                                                           */
/*  Kempe-move ts to r1.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheKempeTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r1)
{
  KHE_RESOURCE r2, tmp_r;  int i, orig_ts_count, ts_mark, new_ts_mark;
  KHE_TASK task;  KHE_RESOURCE_TIMETABLE_MONITOR rtm1, rtm2, tmp_rtm;
  bool res;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheKempeTaskSetMove(");
    KheTaskSetDebug(ts, 1, -1, stderr);
    fprintf(stderr, ", %s)\n", KheResourceId(r1));
  }

  /* fail if no tasks */
  if( KheTaskSetTaskCount(ts) == 0 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheEjectingTaskSetMove ret. false (no tasks)\n");
    return false;
  }

  /* if r1 is NULL, just an unassignment */
  if( r1 == NULL )
  {
    res = KheTaskSetUnAssignResource(ts);
    if( DEBUG5 )
      fprintf(stderr, "] KheKempeTaskSetMove returning %s (unassign)\n",
	res ? "true" : "false");
    return res;
  }

  /* get r2, the resource assigned to the first task */
  /* task = KheTaskFirstUnFixed(KheTaskSetTask(ts, 0)); */
  task = KheTaskSetTask(ts, 0);
  r2 = KheTaskAsstResource(task);

  /* abort if tasks differ in their assignments */
  for( i = 1;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    HnAssert(KheTaskAsstResource(task) == r2,
      "KheEjectingTaskSetMove: tasks assigned different resources");
  }

  /* fail if the common assignment is NULL or r1 */
  if( r2 == NULL || r2 == r1 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheEjectingTaskSetMove ret. false (nochange)\n");
    return false;
  }

  /* get the timetable monitors of r1 and r2 */
  rtm1 = KheResourceTimetableMonitor(KheTaskSoln(task), r1);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm1) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm1);
  rtm2 = KheResourceTimetableMonitor(KheTaskSoln(task), r2);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm2) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm2);

  /* initialize ts and ts_mark */
  orig_ts_count = KheTaskSetTaskCount(ts);
  ts_mark = 0;

  /* do the Kempe move */
  do
  {
    new_ts_mark = KheTaskSetTaskCount(ts);
    for( i = ts_mark;  i < new_ts_mark;  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( !KheTaskUnAssignClashing(task, rtm1, true, ts, ts_mark)
	  || !KheTaskMoveResource(task, r1) )
	return false;
    }
    swap(r1, r2, tmp_r);
    swap(rtm1, rtm2, tmp_rtm);
    ts_mark = new_ts_mark;
  }
  while( ts_mark < KheTaskSetTaskCount(ts) );

  /* clear ts back to its original value and return */
  KheTaskSetDropFromEnd(ts, KheTaskSetTaskCount(ts) - orig_ts_count);
  if( DEBUG1 )
    fprintf(stderr, "] KheKempeTaskSetMove returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheKempeTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r1,          */
/*    KHE_FRAME frame)                                                       */
/*                                                                           */
/*  Kempe-move ts to r1.                                                     */
/*                                                                           */
/*****************************************************************************/

bool KheKempeTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r1,
  KHE_FRAME frame)
{
  KHE_RESOURCE r2, tmp_r;  int i, orig_ts_count, ts_mark, new_ts_mark;
  KHE_TASK task;  KHE_RESOURCE_TIMETABLE_MONITOR rtm1, rtm2, tmp_rtm;
  bool res;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheKempeTaskSetMoveFrame(");
    KheTaskSetDebug(ts, 2, -1, stderr);
    fprintf(stderr, ", %s, frame)\n", KheResourceId(r1));
  }

  /* fail if no tasks */
  if( KheTaskSetTaskCount(ts) == 0 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheEjectingTaskSetMoveFrame ret. false (no tasks)\n");
    return false;
  }

  /* if r1 is NULL, just an unassignment */
  if( r1 == NULL )
  {
    res = KheTaskSetUnAssignResource(ts);
    if( DEBUG5 )
      fprintf(stderr, "] KheKempeTaskSetMoveFrame returning %s (unassign)\n",
	res ? "true" : "false");
    return res;
  }

  /* get r2, the resource assigned to the first task */
  /* task = KheTaskFirstUnFixed(KheTaskSetTask(ts, 0)); */
  task = KheTaskSetTask(ts, 0);
  r2 = KheTaskAsstResource(task);

  /* abort if tasks differ in their assignments */
  for( i = 1;  i < KheTaskSetTaskCount(ts);  i++ )
  {
    task = KheTaskSetTask(ts, i);
    HnAssert(KheTaskAsstResource(task) == r2,
      "KheEjectingTaskSetMoveFrame: tasks assigned different resources");
  }

  /* fail if the common assignment is NULL or r1 */
  if( r2 == NULL || r2 == r1 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheEjectingTaskSetMoveFrame ret. false (nochange)\n");
    return false;
  }

  /* get the timetable monitors of r1 and r2 */
  rtm1 = KheResourceTimetableMonitor(KheTaskSoln(task), r1);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm1) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm1);
  rtm2 = KheResourceTimetableMonitor(KheTaskSoln(task), r2);
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm2) )
    KheMonitorAttachToSoln((KHE_MONITOR) rtm2);

  /* initialize ts and ts_mark */
  orig_ts_count = KheTaskSetTaskCount(ts);
  ts_mark = 0;

  /* do the Kempe move */
  do
  {
    new_ts_mark = KheTaskSetTaskCount(ts);
    for( i = ts_mark;  i < new_ts_mark;  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( !KheTaskUnAssignClashingFrame(task, rtm1, true, frame, ts, ts_mark)
	  || !KheTaskMoveResource(task, r1) )
	return false;
    }
    swap(r1, r2, tmp_r);
    swap(rtm1, rtm2, tmp_rtm);
    ts_mark = new_ts_mark;
  }
  while( ts_mark < KheTaskSetTaskCount(ts) );

  /* clear ts back to its original value and return */
  KheTaskSetDropFromEnd(ts, KheTaskSetTaskCount(ts) - orig_ts_count);
  if( DEBUG1 )
    fprintf(stderr, "] KheKempeTaskSetMoveFrame returning true\n");
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "typed task and task set moves"                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheTypedTaskMove(KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE mt)   */
/*                                                                           */
/*  Make a move of type mt of task to r.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheTypedTaskMove(KHE_TASK task, KHE_RESOURCE r, KHE_MOVE_TYPE mt)
{
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:	return KheTaskMoveResource(task, r);
    case KHE_MOVE_CHECKED:	return KheEjectingTaskMove(task, r, false);
    case KHE_MOVE_EJECTING:	return KheEjectingTaskMove(task, r, true);
    case KHE_MOVE_KEMPE:	return KheKempeTaskMove(task, r);

    default:
      HnAbort("KheTypedTaskMove internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTypedTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,                */
/*    KHE_MOVE_TYPE mt, KHE_FRAME frame)                                     */
/*                                                                           */
/*  Make a framed move of type mt of task to r, which may be NULL.           */
/*                                                                           */
/*****************************************************************************/

bool KheTypedTaskMoveFrame(KHE_TASK task, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt, KHE_FRAME frame)
{
  HnAssert(task != NULL, "KheTypedTaskMoveFrame:  task is NULL");
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:
      return KheTaskMoveResource(task, r);

    case KHE_MOVE_CHECKED:
      return KheEjectingTaskMoveFrame(task, r, false, frame);

    case KHE_MOVE_EJECTING:
      return KheEjectingTaskMoveFrame(task, r, true, frame);

    case KHE_MOVE_KEMPE:
      return KheKempeTaskMoveFrame(task, r, frame);

    default:
      HnAbort("KheTypedTaskMoveFrame internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTypedTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,                */
/*    KHE_MOVE_TYPE mt)                                                      */
/*                                                                           */
/*  Make a move of type mt of ts to r.                                       */
/*                                                                           */
/*****************************************************************************/

bool KheTypedTaskSetMove(KHE_TASK_SET ts, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt)
{
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:
      return KheTaskSetUnAssignResource(ts);

    case KHE_MOVE_CHECKED:
      return KheEjectingTaskSetMove(ts, r, false);

    case KHE_MOVE_EJECTING:
      return KheEjectingTaskSetMove(ts, r, true);

    case KHE_MOVE_KEMPE:
      return KheKempeTaskSetMove(ts, r);

    default:
      HnAbort("KheTypedTaskSetMove internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTypedTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,           */
/*    KHE_MOVE_TYPE mt, KHE_FRAME frame)                                     */
/*                                                                           */
/*  Make a framed move of type mt of ts to r.                                */
/*                                                                           */
/*****************************************************************************/

bool KheTypedTaskSetMoveFrame(KHE_TASK_SET ts, KHE_RESOURCE r,
  KHE_MOVE_TYPE mt, KHE_FRAME frame)
{
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:
      return KheTaskSetMoveResource(ts, r);

    case KHE_MOVE_CHECKED:
      return KheEjectingTaskSetMoveFrame(ts, r, false, frame);

    case KHE_MOVE_EJECTING:
      return KheEjectingTaskSetMoveFrame(ts, r, true, frame);

    case KHE_MOVE_KEMPE:
      return KheKempeTaskSetMoveFrame(ts, r, frame);

    default:
      HnAbort("KheTypedTaskSetMoveFrame internal error");
      return false;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMoveTypeShow(KHE_MOVE_TYPE mt)                                  */
/*                                                                           */
/*  Show mt.                                                                 */
/*                                                                           */
/*****************************************************************************/

char *KheMoveTypeShow(KHE_MOVE_TYPE mt)
{
  switch( mt )
  {
    case KHE_MOVE_UNCHECKED:	return "unchecked";
    case KHE_MOVE_CHECKED:	return "checked";
    case KHE_MOVE_EJECTING:	return "ejecting";
    case KHE_MOVE_KEMPE:	return "kempe";

    default:
      HnAbort("KheMoveTypeShow internal error");
      return NULL;  /* keep compiler happy */
  }
}

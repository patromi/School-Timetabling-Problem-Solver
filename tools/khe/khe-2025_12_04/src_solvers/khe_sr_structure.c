
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
/*  FILE:         khe_sr_structure.c                                         */
/*  DESCRIPTION:  Structural resource solvers                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0

/* ***
typedef HA_ARRAY(KHE_RESOURCE_TYPE) ARRAY_KHE_RESOURCE_TYPE;
typedef HA_ARRAY(KHE_CONSTRAINT) ARRAY_KHE_CONSTRAINT;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingAllowSplitAssignments(KHE_TASKING tasking,                */
/*    bool unassigned_only)                                                  */
/*                                                                           */
/*  Allow split assignments by converting the followers of the tasks of      */
/*  tasking into leaders.  If unassigned_only is true, do this only for      */
/*  the unassigned tasks of tasking.  The followers are made into tasks      */
/*  of tasking.  Any assignments to cycle tasks are preserved.               */
/*                                                                           */
/*  Implementation note.  At first glance this code appears to visit         */
/*  only the children of each leader task, not its proper descendants.       */
/*  However, each child becomes a leader and gets added to the end of        */
/*  tasking's list of tasks, so it is visited later and its children         */
/*  made into leaders, etc.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheAllowSplitAssignments(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool unassigned_only)
{
  KHE_TASK task, child_task;  int i;
  if( DEBUG7 )
  {
    KHE_RESOURCE_TYPE rt;
    fprintf(stderr, "[ KheTaskingAllowSplitAssignments(%s, %s)\n",
      rt == NULL ? "NULL" : KheResourceTypeName(rt) == NULL ? "?" :
      KheResourceTypeName(rt), unassigned_only ? "true" : "false");
    fprintf(stderr, "  before:\n");
    /* KheTaskingDebug(tasking, 2, 2, stderr); */
  }

  /* do the reconstructing */
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskResourceType(task) == rt )
    {
      if( !unassigned_only || KheTaskAsst(task) == NULL )
	while( KheTaskAssignedToCount(task) > 0 )
	{
	  child_task = KheTaskAssignedTo(task, 0);
	  KheTaskAssignUnFix(child_task);
	  KheTaskUnAssign(child_task);
	  /* KheTaskingAddTask(tasking, child_task); */
	  if( KheTaskAsst(task) != NULL )
	    if( !KheTaskAssign(child_task, KheTaskAsst(task)) )
	      HnAbort("KheTaskingAllowSplitAssignments internal error");
	}
    }
  }

  /* apply KheMonitorAttachCheck to affected avoid split assignments monitors */
  /* ***
  KheTaskingMonitorAttachCheck(tasking,
    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG, unassigned_only);
  *** */

  if( DEBUG7 )
  {
    fprintf(stderr, "  after:\n");
    /* KheTaskingDebug(tasking, 2, 2, stderr); */
    fprintf(stderr, "] KheTaskingAllowSplitAssignments returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoEnlargeDomains(KHE_TASK task)                              */
/*                                                                           */
/*  Unfix the domains of task and its descendants, and enlarge their         */
/*  domains by removing all task bounds.                                     */
/*                                                                           */
/*  Implementation note.  The tasks are visited in postorder, since          */
/*  otherwise some of the task bound deletions could fail.                   */
/*                                                                           */
/*****************************************************************************/

static void KheTaskDoEnlargeDomains(KHE_TASK task)
{
  int i;  KHE_TASK_BOUND tb;

  /* child tasks first */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    KheTaskDoEnlargeDomains(KheTaskAssignedTo(task, i));

  /* now task itself, if not preassigned */
  /* KheTaskDomainUnFix(task); */
  if( !KheTaskIsPreassigned(task, NULL) )
    while( KheTaskTaskBoundCount(task) > 0 )
    {
      tb = KheTaskTaskBound(task, 0);
      if( !KheTaskDeleteTaskBound(task, tb) )
	HnAbort("KheTaskDoEnlargeDomains internal error");
      if( KheTaskBoundTaskCount(tb) == 0 )
	KheTaskBoundDelete(tb);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingEnlargeDomains(KHE_TASKING tasking, bool unassigned_only) */
/*                                                                           */
/*  Enlarge the domains of the tasks of tasking (or of just the unassigned   */
/*  tasks, if unassigned_only is true) to all resources of their type.       */
/*                                                                           */
/*  But take care not to enlarge the domain of any preassigned task.         */
/*                                                                           */
/*****************************************************************************/

void KheEnlargeDomains(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  bool unassigned_only)
{
  KHE_TASK task;  int i;
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( KheTaskResourceType(task) == rt )
    {
      if( !unassigned_only || KheTaskAsst(task) == NULL )
	KheTaskDoEnlargeDomains(task);
    }
  }
}


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
/*  FILE:         khe_avoid_split_assignments_monitor.c                      */
/*  DESCRIPTION:  An avoid split assignments monitor                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR - an avoid split assignments monitor */
/*                                                                           */
/*  The deviation is HaArrayCount(m->multiplicities) - 1, or 0 if that       */
/*  number is negative.                                                      */
/*                                                                           */
/*****************************************************************************/

struct khe_avoid_split_assignments_monitor_rec {
  INHERIT_MONITOR(unused, eg_index)
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT constraint;	/* the constraint    */
  ARRAY_KHE_RESOURCE			resources;	/* assigned now      */
  HA_ARRAY_INT				multiplicities;	/* multiplicities    */
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR	copy;		/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR KheAvoidSplitAssignmentsMonitorMake( */
/*    KHE_SOLN soln, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index) */
/*                                                                           */
/*  Make a new avoid split assignments monitor with these attributes.        */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR KheAvoidSplitAssignmentsMonitorMake(
  KHE_SOLN soln, KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, int eg_index)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR res;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_EVENT_RESOURCE er;  int i, count;  HA_ARENA a;
  if( DEBUG1 )
    fprintf(stderr, "[ KheAvoidSplitAssignmentsMonitorMake(%s.%d)\n",
      KheConstraintId((KHE_CONSTRAINT) c) == NULL ? "-" :
      KheConstraintId((KHE_CONSTRAINT) c), eg_index);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->constraint = c;
  res->eg_index = eg_index;
  HaArrayInit(res->resources, a);
  HaArrayInit(res->multiplicities, a);
  res->copy = NULL;

  /* add to (but not attach to) all the applicable event resources */
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c,res->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, res->eg_index, i);
    ers = KheSolnEventResourceInSoln(soln, er);
    KheEventResourceInSolnAddMonitor(ers, (KHE_MONITOR) res);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheAvoidSplitAssignmentsMonitorMake\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR                                      */
/*    KheAvoidSplitAssignmentsMonitorCopyPhase1(                             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, HA_ARENA a)                     */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR KheAvoidSplitAssignmentsMonitorCopyPhase1(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, HA_ARENA a)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR copy;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->constraint = m->constraint;
    copy->eg_index = m->eg_index;
    HaArrayInit(copy->resources, a);
    HaArrayAppend(copy->resources, m->resources, i);
    HaArrayInit(copy->multiplicities, a);
    HaArrayAppend(copy->multiplicities, m->multiplicities, i);
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorCopyPhase2(                          */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorCopyPhase2(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorDelete(                              */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidSplitAssignmentsMonitorDelete(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_EVENT_RESOURCE er;  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c;
  if( m->attached )
    KheAvoidSplitAssignmentsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  c = m->constraint;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(c, m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(c, m->eg_index, i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnDeleteMonitor(ers, (KHE_MONITOR) m);
  }
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MArrayFree(m->resources);
  MArrayFree(m->multiplicities);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT                                   */
/*    KheAvoidSplitAssignmentsMonitorConstraint(                             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT
  KheAvoidSplitAssignmentsMonitorConstraint(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorEventGroupIndex(                      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the event group index of m.                                       */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsMonitorEventGroupIndex(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  return m->eg_index;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorResourceCount(                        */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the number of distinct resources currently assigned to the tasks  */
/*  monitored by m.                                                          */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsMonitorResourceCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  return HaArrayCount(m->resources);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheAvoidSplitAssignmentsMonitorResource(                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return the i'th distinct resource current assigned to the tasks          */
/*  monitored by m.                                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheAvoidSplitAssignmentsMonitorResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)
{
  return HaArray(m->resources, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorResourceMultiplicity(                 */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return the number of tasks that the i'th distinct resource is assigned.  */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsMonitorResourceMultiplicity(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)
{
  return HaArray(m->multiplicities, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0, because when unlinked the monitor monitors  */
/*  no tasks, hence there are no resources and no multiplicities.            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorAttachToSoln(                        */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorAttachToSoln(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  m->attached = true;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnAttachMonitor(ers, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorDetachFromSoln(                      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorDetachFromSoln(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0; i < count; i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnDetachMonitor(ers, (KHE_MONITOR) m);
  }
  HnAssert(HaArrayCount(m->multiplicities) == 0,
    "KheAvoidSplitAssignmentsMonitorDetachFromSoln internal error");
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorAttachCheck(                         */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*  This will ensure that m is detached from the solution if and only if     */
/*  every task that it monitors is assigned, directly or indirectly, to      */
/*  the same leader task.                                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAvoidSplitAssignmentsMonitorAttachCheck(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  KHE_TASK task, leader_task;  int count, i, j;  KHE_EVENT_RESOURCE er;
  leader_task = NULL;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0; i < count; i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, i);
    for( j = 0;  j < KheEventResourceTaskCount(m->soln, er);  j++ )
    {
      task = KheTaskLeader(KheEventResourceTask(m->soln, er, j));
      if( leader_task == NULL )
	leader_task = task;
      else if( leader_task != task )
      {
	** distinct leaders; make sure m is attached, and exit **
	if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
	  KheMonitorAttachToSoln((KHE_MONITOR) m);
	return;
      }
    }
  }

  ** no distinct leaders; make sure m is detached, and exit **
  if( KheMonitorAttachedToSoln((KHE_MONITOR) m) )
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsMonitorTimeRange(                           */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME *first_time,           */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Implement KheMonitorTimeRange for avoid split assts monitor m.           */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsMonitorTimeRange(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, i);
    if( !KheEventResourceTimeRange(er, m->soln, &ft, &lt) )
      return false;
    if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
      *first_time = ft;
    if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
      *last_time = lt;
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorSetSweepTime(                        */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME t)                     */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for avoid split assignments monitor m.  */
/*                                                                           */
/*  Implementation note.  Installing a cutoff time makes no difference       */
/*  to this monitor, because an unassigned task has the same effect on       */
/*  it as an open task, so there is nothing to do here.                      */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorSetSweepTime(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsMonitorSweepTimeRange(                      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME *first_time,           */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for avoid split assts monitor m.      */
/*                                                                           */
/*  Implementation note.  Installing a cutoff time makes no difference       */
/*  to this monitor, because an unassigned task has the same effect on       */
/*  it as an open task, so there is nothing to do here.                      */
/*                                                                           */
/*****************************************************************************/

bool KheAvoidSplitAssignmentsMonitorSweepTimeRange(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TIME *first_time,
  KHE_TIME *last_time)
{
  /* nothing to do */
  return *first_time = *last_time = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "fix and unfix"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsMonitorMustHaveZeroCost(                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return true if m must have zero cost, because the tasks it monitors      */
/*  all have the same fixed leader task.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidSplitAssignmentsMonitorMustHaveZeroCost(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  KHE_TASK task, leader_task;  int count, i, j;  KHE_EVENT_RESOURCE er;
  leader_task = NULL;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0; i < count; i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, i);
    for( j = 0;  j < KheEventResourceTaskCount(m->soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(m->soln, er, j));
      if( task == NULL )
	return false;
      else if( leader_task == NULL )
	leader_task = task;
      else if( leader_task != task )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorTaskAssignFix(                       */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  A TaskAssignFix relevant to m has just occurred.                         */
/*                                                                           */
/*  This function assumes that m is attached with cost 0.                    */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorTaskAssignFix(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  if( KheAvoidSplitAssignmentsMonitorMustHaveZeroCost(m) )
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  detaching ");
      KheAvoidSplitAssignmentsMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorTaskAssignUnFix(                     */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  A TaskAssignUnFix relevant to m has just occurred.                       */
/*                                                                           */
/*  This function assumes that m is detached.                                */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorTaskAssignUnFix(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  if( !KheAvoidSplitAssignmentsMonitorMustHaveZeroCost(m) )
  {
    if( DEBUG2 )
    {
      fprintf(stderr, "  attaching ");
      KheAvoidSplitAssignmentsMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
    KheMonitorAttachToSoln((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorAddTask(                             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TASK task)                  */
/*                                                                           */
/*  Monitor the effect of adding task.                                       */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorAddTask(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TASK task)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task);
  if( r != NULL )
  {
    /* multiplicy increases, just like assigning a resource */
    KheAvoidSplitAssignmentsMonitorAssignResource(m, task, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorDeleteTask(                          */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TASK task)                  */
/*                                                                           */
/*  Monitor the effect of deleting task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorDeleteTask(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, KHE_TASK task)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task);
  if( r != NULL )
  {
    /* multiplicy decreases, just like deleting a resource */
    KheAvoidSplitAssignmentsMonitorUnAssignResource(m, task, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorSplitTask(                           */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,                                 */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that a solution resource has just split into task1 and task2. */
/*  Either both solution resources are assigned the same resource, or        */
/*  they are both unassigned.                                                */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorSplitTask(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task2);
  if( r != NULL )
  {
    /* multiplicity increases, just like assigning a resource */
    KheAvoidSplitAssignmentsMonitorAssignResource(m, task2, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorMergeTask(                           */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,                                 */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that task1 and task2 are just about to be merged.             */
/*  Either both solution resources are assigned the same resource, or        */
/*  they are both unassigned.                                                */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorMergeTask(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  KHE_RESOURCE r;
  r = KheTaskAsstResource(task2);
  if( r != NULL )
  {
    /* multiplicy decreases, just like deleting a resource */
    KheAvoidSplitAssignmentsMonitorUnAssignResource(m, task2, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorAssignResource(                      */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,                                 */
/*    KHE_TASK task, KHE_RESOURCE r)                                         */
/*                                                                           */
/*  Let m know that task has just been assigned resource r.                  */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorAssignResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  int pos;
  if( HaArrayContains(m->resources, r, &pos) )
    ++HaArray(m->multiplicities, pos);
  else
  {
    HaArrayAddLast(m->resources, r);
    HaArrayAddLast(m->multiplicities, 1);
    if( HaArrayCount(m->resources) >= 2 )
      KheMonitorChangeCost((KHE_MONITOR) m,
	KheMonitorDevToCost((KHE_MONITOR) m, HaArrayCount(m->resources) - 1));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorUnAssignResource(                    */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,                                 */
/*    KHE_TASK task, KHE_RESOURCE r)                                         */
/*                                                                           */
/*  Let m know that task has just been unassigned resource r.                */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorUnAssignResource(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  int pos, tmpm;  KHE_RESOURCE tmpr;
  if( !HaArrayContains(m->resources, r, &pos) )
    HnAbort("KheAvoidSplitAssignmentsMonitorUnAssignResource internal error");
  if( HaArray(m->multiplicities, pos) >= 2 )
    --HaArray(m->multiplicities, pos);
  else
  {
    tmpr = HaArrayLastAndDelete(m->resources);
    tmpm = HaArrayLastAndDelete(m->multiplicities);
    if( tmpr != r )
    {
      HaArrayPut(m->resources, pos, tmpr);
      HaArrayPut(m->multiplicities, pos, tmpm);
    }
    if( HaArrayCount(m->resources) >= 1 )
      KheMonitorChangeCost((KHE_MONITOR) m,
	KheMonitorDevToCost((KHE_MONITOR) m, HaArrayCount(m->resources) - 1));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorDeviation(                            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheAvoidSplitAssignmentsMonitorDeviation(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  return HaArrayCount(m->resources) <= 1 ? 0 : HaArrayCount(m->resources) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidSplitAssignmentsMonitorDeviationDescription(               */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidSplitAssignmentsMonitorDeviationDescription(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i;  KHE_RESOURCE r;  char *name;  HA_ARENA a;
  if( HaArrayCount(m->resources) <= 1 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d too many of ", HaArrayCount(m->resources) - 1);
    HaArrayForEach(m->resources, r, i)
    {
      if( i > 0 )
	HnStringAdd(&ac, ", ");
      name = KheResourceName(r);
      HnStringAdd(&ac, name != NULL ? name : "?");
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorDeviationCount(                       */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAvoidSplitAssignmentsMonitorDeviationCount(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvoidSplitAssignmentsMonitorDeviation(                            */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAvoidSplitAssignmentsMonitorDeviation(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheAvoidSplitAssignmentsMonitorDeviation: i out of range");
  return HaArrayCount(m->resources) >= 2 ? HaArrayCount(m->resources) - 1 : 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidSplitAssignmentsMonitorDeviationDescription(               */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheAvoidSplitAssignmentsMonitorDeviationDescription(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int i)
{
  HnAssert(i == 0,
    "KheAvoidSplitAssignmentsMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidSplitAssignmentsMonitorPointOfApplication(                 */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidSplitAssignmentsMonitorPointOfApplication(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;  int egi;  KHE_EVENT_GROUP eg;
  egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(m);
  asac = KheAvoidSplitAssignmentsMonitorConstraint(m);
  eg = KheAvoidSplitAssignmentsConstraintEventGroup(asac, egi);
  return KheEventGroupName(eg);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAvoidSplitAssignmentsMonitorId(                                 */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheAvoidSplitAssignmentsMonitorId(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m)
{
  int egi;  KHE_EVENT_GROUP eg;  char *constraint_id, *eg_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    egi = KheAvoidSplitAssignmentsMonitorEventGroupIndex(m);
    eg = KheAvoidSplitAssignmentsConstraintEventGroup(m->constraint, egi);
    if( eg == NULL )
      eg_id = "?";
    else if( KheEventGroupId(eg) != NULL )
      eg_id = KheEventGroupId(eg);
    else if( KheEventGroupEventCount(eg) == 1 )
      eg_id = KheEventId(KheEventGroupEvent(eg, 0));
    else
      eg_id = "?";
    a = KheSolnArena(m->soln);
    m->id = HnStringMake(a, "%s/%s", constraint_id, eg_id);
  }
  return m->id;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAvoidSplitAssignmentsMonitorDebug(                               */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAvoidSplitAssignmentsMonitorDebug(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR m, int verbosity, int indent, FILE *fp)
{
  char *id;  KHE_RESOURCE r;  int i, count;  KHE_EVENT_RESOURCE er;
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    if( verbosity >= 3 )
    {
      id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
      fprintf(fp, " %s.%d", id != NULL ? id : "-", m->eg_index);
    }
    count = KheAvoidSplitAssignmentsConstraintEventResourceCount(m->constraint,
      m->eg_index);
    HnAssert(count >= 1, "KheAvoidSplitAssignmentsMonitorDebug internal error");
    er = KheAvoidSplitAssignmentsConstraintEventResource(m->constraint,
      m->eg_index, 0);
    fprintf(fp, " ");
    KheEventResourceDebug(er, 1, -1, fp);
    fprintf(fp, " := (");
    for( i = 0;  i < HaArrayCount(m->resources);  i++ )
    {
      if( i > 0 )
	fprintf(fp, ", ");
      r = HaArray(m->resources, i);
      fprintf(fp, "%d %s", HaArray(m->multiplicities, i),
	KheResourceId(r) != NULL ? KheResourceId(r) : "-");
    }
    fprintf(fp, ")");
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

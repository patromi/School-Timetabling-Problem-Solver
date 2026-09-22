
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
/*  FILE:         khe_event_resource_in_soln.c                               */
/*  DESCRIPTION:  A soln object that monitors one event resource             */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_IN_SOLN                                               */
/*                                                                           */
/*****************************************************************************/

struct khe_event_resource_in_soln_rec {
  KHE_EVENT_IN_SOLN		event_in_soln;		/* encl es           */
  KHE_EVENT_RESOURCE		event_resource;		/* original er       */
  ARRAY_KHE_TASK		tasks;			/* its tasks         */
  ARRAY_KHE_MONITOR		all_monitors;		/* all monitors      */
  ARRAY_KHE_MONITOR		attached_monitors;      /* attached monitors */
  KHE_EVENT_RESOURCE_IN_SOLN	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_IN_SOLN KheEventResourceInSolnMake(                   */
/*    KHE_EVENT_IN_SOLN es, KHE_EVENT_RESOURCE er)                           */
/*                                                                           */
/*  Make a new event resource monitor for er in soln.                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_IN_SOLN KheEventResourceInSolnMake(KHE_EVENT_IN_SOLN es,
  KHE_EVENT_RESOURCE er)
{
  KHE_EVENT_RESOURCE_IN_SOLN res;  HA_ARENA a;  KHE_SOLN soln;
  soln = KheEventInSolnSoln(es);
  a = KheSolnArena(soln);
  HaMake(res, a);
  res->event_in_soln = es;
  res->event_resource = er;
  HaArrayInit(res->tasks, a);
  HaArrayInit(res->all_monitors, a);
  HaArrayInit(res->attached_monitors, a);
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN KheEventResourceInSolnEventInSoln(                     */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers)                                        */
/*                                                                           */
/*  Return the event_in_soln attribute of ers.                               */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_IN_SOLN KheEventResourceInSolnEventInSoln(
  KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  return ers->event_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE KheEventResourceInSolnEventResource(                  */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers)                                        */
/*                                                                           */
/*  Return the event resource attribute of ers.                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheEventResourceInSolnEventResource(
  KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  return ers->event_resource;
}


/*****************************************************************************/
/*                                                                           */
/*  char *EventResourceInSolnId(KHE_EVENT_RESOURCE_IN_SOLN ers)              */
/*                                                                           */
/*  Return a rough and ready identifier for ers.                             */
/*                                                                           */
/*****************************************************************************/

char *EventResourceInSolnId(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  return KheEventInSolnId(ers->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnDelete(KHE_EVENT_RESOURCE_IN_SOLN ers)        */
/*                                                                           */
/*  Free ers.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEventResourceInSolnDelete(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  ** soln resources should be deleted already **
  HnAssert(HaArrayCount(ers->tasks) == 0,
    "KheEventResourceInSolnDelete internal error");
  MArrayFree(ers->tasks);

  ** monitors are deleted separately; just free the arrays here **
  MArrayFree(ers->all_monitors);
  MArrayFree(ers->attached_monitors);

  MFree(ers);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_IN_SOLN KheEventResourceInSolnCopyPhase1(             */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers, HA_ARENA a)                            */
/*                                                                           */
/*  Carry out Phase 1 of copying ers.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_IN_SOLN KheEventResourceInSolnCopyPhase1(
  KHE_EVENT_RESOURCE_IN_SOLN ers, HA_ARENA a)
{
  KHE_EVENT_RESOURCE_IN_SOLN copy;  KHE_TASK task;  int i;  KHE_MONITOR m;
  if( ers->copy == NULL )
  {
    HaMake(copy, a);
    ers->copy = copy;
    copy->event_in_soln = KheEventInSolnCopyPhase1(ers->event_in_soln, a);
    copy->event_resource = ers->event_resource;
    HaArrayInit(copy->tasks, a);
    HaArrayForEach(ers->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, a));
    HaArrayInit(copy->all_monitors, a);
    HaArrayForEach(ers->all_monitors, m, i)
      HaArrayAddLast(copy->all_monitors, KheMonitorCopyPhase1(m, a));
    HaArrayInit(copy->attached_monitors, a);
    HaArrayForEach(ers->attached_monitors, m, i)
      HaArrayAddLast(copy->attached_monitors, KheMonitorCopyPhase1(m, a));
    copy->copy = NULL;
  }
  return ers->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnCopyPhase2(KHE_EVENT_RESOURCE_IN_SOLN ers)    */
/*                                                                           */
/*  Carry out Phase 2 of copying ers.                                        */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnCopyPhase2(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  int i;  KHE_MONITOR m;
  if( ers->copy != NULL )
  {
    ers->copy = NULL;
    KheEventInSolnCopyPhase2(ers->event_in_soln);
    HaArrayForEach(ers->all_monitors, m, i)
      KheMonitorCopyPhase2(m);
    HaArrayForEach(ers->attached_monitors, m, i)
      KheMonitorCopyPhase2(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "soln resources"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnAddTask(KHE_EVENT_RESOURCE_IN_SOLN ers,       */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Add task to ers.                                                         */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnAddTask(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task)
{
  KHE_MONITOR m;  int i;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEventResourceInSolnAddTask(");
    KheEventResourceDebug(ers->event_resource, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  HaArrayAddLast(ers->tasks, task);
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorAddTask(m, task);
  if( DEBUG1 )
    fprintf(stderr, "] KheEventResourceInSolnAddTask returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnDeleteTask(KHE_EVENT_RESOURCE_IN_SOLN ers,    */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Delete task from ers.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnDeleteTask(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task)
{
  KHE_MONITOR m;  KHE_TASK x;  int i;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheEventResourceInSolnDeleteTask(");
    KheEventResourceDebug(ers->event_resource, 1, -1, stderr);
    fprintf(stderr, ", ");
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorDeleteTask(m, task);
  x = HaArrayLastAndDelete(ers->tasks);
  if( x != task )
  {
    if( !HaArrayContains(ers->tasks, task, &i) )
      HnAbort("KheEventResourceInSolnDeleteTask internal error");
    HaArrayPut(ers->tasks, i, x);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheEventResourceInSolnDeleteTask returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnSplitTask(KHE_EVENT_RESOURCE_IN_SOLN ers,     */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Inform ers that task1, one of its tasks, is splitting into task1 and     */
/*  task2.  The new durations are already installed.                         */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnSplitTask(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task1, KHE_TASK task2)
{
  int i;  KHE_MONITOR m;
  HaArrayAddLast(ers->tasks, task2);
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorSplitTask(m, task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnMergeTask(KHE_EVENT_RESOURCE_IN_SOLN ers,     */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Inform ers that its solution resources task1 and task2 are merging.      */
/*  They currently have their pre-merging durations; task2 is going.         */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnMergeTask(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task1, KHE_TASK task2)
{
  KHE_MONITOR m;  int i;  KHE_TASK x;
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorMergeTask(m, task1, task2);
  x = HaArrayLastAndDelete(ers->tasks);
  if( x != task2 )
  {
    if( !HaArrayContains(ers->tasks, task2, &i) )
      HnAbort("KheEventInSolnMergeSolnEvent internal error");
    HaArrayPut(ers->tasks, i, x);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnAssignResource(                               */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers, KHE_TASK task, KHE_RESOURCE r)         */
/*                                                                           */
/*  Inform ers that its soln resource, task, has been assigned r.            */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnAssignResource(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task, KHE_RESOURCE r)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorAssignResource(m, task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnUnAssignResource(                             */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers, KHE_TASK task, KHE_RESOURCE r)         */
/*                                                                           */
/*  Inform ers that its soln resource, task, has been unassigned r.          */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnUnAssignResource(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_TASK task, KHE_RESOURCE r)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(ers->attached_monitors, m, i)
    KheMonitorUnAssignResource(m, task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceInSolnTaskCount(KHE_EVENT_RESOURCE_IN_SOLN ers)      */
/*                                                                           */
/*  Return the number of solution resources in ers.                          */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceInSolnTaskCount(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  return HaArrayCount(ers->tasks);
}


/*****************************************************************************/
/*                                                                           */
/* KHE_TASK KheEventResourceInSolnTask(KHE_EVENT_RESOURCE_IN_SOLN ers, int i)*/
/*                                                                           */
/*  Return the i'th solution resource of ers.                                */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheEventResourceInSolnTask(KHE_EVENT_RESOURCE_IN_SOLN ers, int i)
{
  return HaArray(ers->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnAttachMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers, */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Attach m to ers.                                                         */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnAttachMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_MONITOR m)
{
  KHE_TASK task;  int i;
  HaArrayAddLast(ers->attached_monitors, m);
  HaArrayForEach(ers->tasks, task, i)
    KheMonitorAddTask(m, task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnDetachMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers, */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Detach m from ers.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnDetachMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_MONITOR m)
{
  KHE_TASK task;  int i, pos;
  HaArrayForEach(ers->tasks, task, i)
    KheMonitorDeleteTask(m, task);
  if( !HaArrayContains(ers->attached_monitors, m, &pos) )
    HnAbort("KheEventResourceInSolnDetachMonitor internal error");
  HaArrayDeleteAndShift(ers->attached_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "fix and unfix"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnTaskAssignFix(KHE_EVENT_RESOURCE_IN_SOLN ers) */
/*                                                                           */
/*  A relevant call to KheTaskAssignFix has occurred.                        */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnTaskAssignFix(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(ers->all_monitors, m, i)
    if( KheMonitorTag(m) == KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG &&
	KheMonitorAttachedToSoln(m) && KheMonitorCost(m) == 0 )
      KheAvoidSplitAssignmentsMonitorTaskAssignFix(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnTaskAssignUnFix(                              */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers)                                        */
/*                                                                           */
/*  A relevant call to KheTaskAssignUnFix has occurred.                      */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnTaskAssignUnFix(
  KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(ers->all_monitors, m, i)
    if( KheMonitorTag(m) == KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG &&
	!KheMonitorAttachedToSoln(m) )
      KheAvoidSplitAssignmentsMonitorTaskAssignUnFix(
	(KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "user monitors and cost"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnAddMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,    */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Add m to the user-accessible list of monitors of ers.                    */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnAddMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_MONITOR m)
{
  HaArrayAddLast(ers->all_monitors, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnDeleteMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers, */
/*    KHE_MONITOR m)                                                         */
/*                                                                           */
/*  Delete m from the user-accessible list of monitors of ers.               */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnDeleteMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_MONITOR m)
{
  int pos;
  if( !HaArrayContains(ers->all_monitors, m, &pos) )
    HnAbort("KheEventResourceInSolnDeleteMonitor internal error");
  HaArrayDeleteAndShift(ers->all_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventResourceInSolnMonitorCount(KHE_EVENT_RESOURCE_IN_SOLN ers)   */
/*                                                                           */
/*  Return the number of user-accessible monitors in ers.                    */
/*                                                                           */
/*****************************************************************************/

int KheEventResourceInSolnMonitorCount(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  return HaArrayCount(ers->all_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheEventResourceInSolnMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers,*/
/*    int i)                                                                 */
/*                                                                           */
/*  Return the i'th user-accessible monitor of ers.                          */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheEventResourceInSolnMonitor(KHE_EVENT_RESOURCE_IN_SOLN ers, int i)
{
  return HaArray(ers->all_monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEventResourceInSolnCost(KHE_EVENT_RESOURCE_IN_SOLN ers)      */
/*                                                                           */
/*  Return the total cost of all monitors.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEventResourceInSolnCost(KHE_EVENT_RESOURCE_IN_SOLN ers)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(ers->all_monitors, m, i)
    res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEventResourceInSolnMonitorCost(                              */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers, KHE_MONITOR_TAG tag)                   */
/*                                                                           */
/*  Return the total cost of monitors with this tag.                         */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEventResourceInSolnMonitorCost(KHE_EVENT_RESOURCE_IN_SOLN ers,
  KHE_MONITOR_TAG tag)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(ers->all_monitors, m, i)
    if( KheMonitorTag(m) == tag )
      res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventResourceInSolnDebug(KHE_EVENT_RESOURCE_IN_SOLN ers,         */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ers onto fp with the given verbosity and indent,          */
/*  including its monitors.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheEventResourceInSolnDebug(KHE_EVENT_RESOURCE_IN_SOLN ers,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_TASK task;  KHE_MONITOR m;  char *role;
  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
    {
      fprintf(fp, "%*s[ Event Resource In Soln\n", indent, "");
      HaArrayForEach(ers->tasks, task, i)
      {
	fprintf(fp, "%*s  soln_resource\n", indent, "");
	KheTaskDebug(task, 1, -1, fp);
      }
      HaArrayForEach(ers->attached_monitors, m, i)
	KheMonitorDebug(m, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
    {
      /* just a quick identifier */
      role = KheEventResourceRole(ers->event_resource);
      if( ers->event_resource != NULL )
      {
	rt = KheEventResourceResourceType(ers->event_resource);
	if( KheResourceTypeId(rt) != NULL )
	  fprintf(fp, "%s:", KheResourceTypeId(rt));
      }
      fprintf(fp, "%s:%s", KheEventInSolnId(ers->event_in_soln),
	role != NULL ? role : "~");
      fprintf(fp, " asst (");
      HaArrayForEach(ers->tasks, task, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	r = KheTaskAsstResource(task);
	fprintf(fp, "%s", r == NULL ? "-" : KheResourceId(r));
      }
      fprintf(fp, ")");
    }
  }
}

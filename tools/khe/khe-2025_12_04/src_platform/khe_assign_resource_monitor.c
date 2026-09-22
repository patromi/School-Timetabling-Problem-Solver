
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
/*  FILE:         khe_assign_resource_monitor.c                              */
/*  DESCRIPTION:  An assign resource monitor                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DBID "ARC:A=s1000:1/1Mon:D/A=s1000:1"
#define DEBUG1(m) (false && strcmp(KheAssignResourceMonitorId(m), DBID) == 0)

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_RESOURCE_MONITOR - an assign resource monitor                 */
/*                                                                           */
/*****************************************************************************/

struct khe_assign_resource_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_EVENT_RESOURCE_IN_SOLN	event_resource_in_soln;	/* enclosing ers     */
  KHE_ASSIGN_RESOURCE_CONSTRAINT constraint;		/* constraint        */
  KHE_ASSIGN_RESOURCE_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_RESOURCE_MONITOR KheAssignResourceMonitorMake(                */
/*    KHE_EVENT_RESOURCE_IN_SOLN ers, KHE_ASSIGN_RESOURCE_CONSTRAINT c)      */
/*                                                                           */
/*  Make a new assign resource monitor with these attributes.                */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_RESOURCE_MONITOR KheAssignResourceMonitorMake(
  KHE_EVENT_RESOURCE_IN_SOLN ers, KHE_ASSIGN_RESOURCE_CONSTRAINT c)
{
  KHE_ASSIGN_RESOURCE_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheEventInSolnSoln(KheEventResourceInSolnEventInSoln(ers));
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_ASSIGN_RESOURCE_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->deviation = 0;
  res->event_resource_in_soln = ers;
  res->constraint = c;
  res->copy = NULL;
  KheEventResourceInSolnAddMonitor(ers, (KHE_MONITOR) res);
  if( DEBUG1(res) )
    fprintf(stderr, "%s = KheAssignResourceMonitorMake(...)\n",
      KheAssignResourceMonitorId(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_RESOURCE_MONITOR KheAssignResourceMonitorCopyPhase1(          */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, HA_ARENA a)                             */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_RESOURCE_MONITOR KheAssignResourceMonitorCopyPhase1(
  KHE_ASSIGN_RESOURCE_MONITOR m, HA_ARENA a)
{
  KHE_ASSIGN_RESOURCE_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->deviation = m->deviation;
    copy->event_resource_in_soln =
      KheEventResourceInSolnCopyPhase1(m->event_resource_in_soln, a);
    copy->constraint = m->constraint;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorCopyPhase2(KHE_ASSIGN_RESOURCE_MONITOR m)   */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorCopyPhase2(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheEventResourceInSolnCopyPhase2(m->event_resource_in_soln);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorDelete(KHE_ASSIGN_RESOURCE_MONITOR m)       */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignResourceMonitorDelete(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  if( m->attached )
    KheAssignResourceMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheEventResourceInSolnDeleteMonitor(m->event_resource_in_soln,
    (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_RESOURCE_CONSTRAINT KheAssignResourceMonitorConstraint(       */
/*    KHE_ASSIGN_RESOURCE_MONITOR m)                                         */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_RESOURCE_CONSTRAINT KheAssignResourceMonitorConstraint(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheAssignResourceMonitorEvent(KHE_ASSIGN_RESOURCE_MONITOR m)   */
/*                                                                           */
/*  Return the event that m is monitoring.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE KheAssignResourceMonitorEventResource(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  return KheEventResourceInSolnEventResource(m->event_resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0, because when unlinked, an assign resource   */
/*  monitor is monitoring no event resources, hence no unassigned ones.      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorAttachToSoln(KHE_ASSIGN_RESOURCE_MONITOR m) */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorAttachToSoln(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  m->attached = true;
  KheEventResourceInSolnAttachMonitor(m->event_resource_in_soln,
    (KHE_MONITOR) m);
  if( DEBUG1(m) )
    fprintf(stderr, "KheAssignResourceMonitorAttachToSoln(%s), dev %d\n",
      KheAssignResourceMonitorId(m), m->deviation);
}


/*****************************************************************************/
/*                                                                           */
/* void KheAssignResourceMonitorDetachFromSoln(KHE_ASSIGN_RESOURCE_MONITOR m)*/
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorDetachFromSoln(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  if( DEBUG1(m) )
    fprintf(stderr, "KheAssignResourceMonitorDetachFromSoln(%s), dev %d\n",
      KheAssignResourceMonitorId(m), m->deviation);
  KheEventResourceInSolnDetachMonitor(m->event_resource_in_soln,
    (KHE_MONITOR) m);
  if( DEBUG1(m) && m->deviation != 0)
    fprintf(stderr, "KheAssignResourceMonitorDetachFromSoln(%s) failing\n",
      KheAssignResourceMonitorId(m));
  HnAssert(m->deviation == 0,
    "KheAssignResourceMonitorDetachFromSoln internal error");
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorAttachCheck(KHE_ASSIGN_RESOURCE_MONITOR m)  */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignResourceMonitorAttachCheck(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
    KheMonitorAttachToSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignResourceMonitorTimeRange(KHE_ASSIGN_RESOURCE_MONITOR m,    */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for assign resource monitor m.             */
/*                                                                           */
/*****************************************************************************/

bool KheAssignResourceMonitorTimeRange(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT_RESOURCE er;
  er = KheAssignResourceMonitorEventResource(m);
  return KheEventResourceTimeRange(er, m->soln, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorSetSweepTime(                               */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for assign resource monitor m.          */
/*                                                                           */
/*  Implementation note.  Implementing this function by attaching and        */
/*  detaching is not a great way to do it:  if a monitor is detached for     */
/*  some other reason, it goes wrong.  But it's good enough to go on with.   */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorSetSweepTime(
  KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)
{
  KHE_TIME first_time, last_time;
  if( KheAssignResourceMonitorTimeRange(m, &first_time, &last_time)
      && (t == NULL || KheTimeIndex(t) < KheTimeIndex(last_time)) )
  {
    /* m needs to be cut off */
    if( m->attached )
      KheAssignResourceMonitorDetachFromSoln(m);
  }
  else
  {
    /* m needs to be not cut off */
    if( !m->attached )
      KheAssignResourceMonitorAttachToSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignResourceMonitorSweepTimeRange(                             */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME *first_time,                   */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for assign resource monitor m.        */
/*                                                                           */
/*****************************************************************************/

bool KheAssignResourceMonitorSweepTimeRange(
  KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheAssignResourceMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorAddTask(KHE_ASSIGN_RESOURCE_MONITOR m,      */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Monitor the effect of adding task.                                       */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorAddTask(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task)
{
  if( DEBUG1(m) )
    fprintf(stderr, "KheAssignResourceMonitorAddTask(%s, %s task), dev %d\n",
      KheAssignResourceMonitorId(m),
      KheTaskAsstResource(task) == NULL ? "unassigned" : "assigned",
      m->deviation);
  if( KheTaskAsstResource(task) == NULL )
  {
    m->deviation += KheTaskDuration(task);
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorDeleteTask(KHE_ASSIGN_RESOURCE_MONITOR m,   */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Monitor the effect of deleting task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorDeleteTask(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task)
{
  if( DEBUG1(m) )
    fprintf(stderr, "KheAssignResourceMonitorDeleteTask(%s, %s task), dev %d\n",
      KheAssignResourceMonitorId(m),
      KheTaskAsstResource(task) == NULL ? "unassigned" : "assigned", m->deviation);
  if( KheTaskAsstResource(task) == NULL )
  {
    m->deviation -= KheTaskDuration(task);
    HnAssert(m->deviation >= 0,
      "KheAssignResourceMonitorDeleteTask internal error");
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorSplitTask(KHE_ASSIGN_RESOURCE_MONITOR m,    */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that a task has just split into task1 and task2.              */
/*  Either both tasks are assigned, or they aren't.                          */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorSplitTask(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  /* no change in cost is possible */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorMergeTask(KHE_ASSIGN_RESOURCE_MONITOR m,    */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that task1 and task2 are just about to be merged.  It might   */
/*  seem that there is a problem with this code if task1 is assigned         */
/*  a resource and task2 is not.  However, this code is called only by       */
/*  KheTaskMerge, and it is a precondition of that function that             */
/*  task1 and task2 must either both be unassigned or else both assigned to  */
/*  the same resource.                                                       */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorMergeTask(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  /* no change in cost is possible */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorAssignResource(                             */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TASK task, KHE_RESOURCE r)          */
/*                                                                           */
/*  Let m know that task has just been assigned resource r.                  */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorAssignResource(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  if( DEBUG1(m) )
    fprintf(stderr,"KheAssignResourceMonitorAssignResource(%s, task), dev %d\n",
      KheAssignResourceMonitorId(m), m->deviation);
  m->deviation -= KheTaskDuration(task);
  HnAssert(m->deviation >= 0,
    "KheAssignResourceMonitorAssignTime internal error");
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignResourceMonitorUnAssignResource(                           */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TASK task, KHE_RESOURCE r)          */
/*                                                                           */
/*  Let m know that task has just been unassigned resource r.                */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorUnAssignResource(KHE_ASSIGN_RESOURCE_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  if( DEBUG1(m) )
    fprintf(stderr,"KheAssignResourceMonitorUnAssignResource(%s, t), dev %d\n",
      KheAssignResourceMonitorId(m), m->deviation);
  m->deviation += KheTaskDuration(task);
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceMonitorDeviation(KHE_ASSIGN_RESOURCE_MONITOR m)     */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheAssignResourceMonitorDeviation(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceMonitorUnassignedTaskCount(                         */
/*    KHE_ASSIGN_RESOURCE_MONITOR m)                                         */
/*                                                                           */
/*  Return the number of unassigned tasks monitored by m.                    */
/*                                                                           */
/*****************************************************************************/

/* *** not currently used
static int KheAssignResourceMonitorUnassignedTaskCount(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_TASK task;
  er = KheAssignResourceMonitorEventResource(m);
  count = 0;
  for( i = 0;  i < KheEventResourceTaskCount(m->soln, er);  i++ )
  {
    task = KheEventResourceTask(m->soln, er, i);
    if( KheTaskAsst(task) == NULL )
      count++;
  }
  return count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignResourceMonitorDeviationDescription(                      */
/*    KHE_ASSIGN_RESOURCE_MONITOR m)                                         */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheAssignResourceMonitorDeviationDescription(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count;  KHE_EVENT_RESOURCE er;  KHE_TASK task;
  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    /* MStringPrintf(ac, 100, "%d: ", m->deviation); */
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    er = KheAssignResourceMonitorEventResource(m);
    count = 0;
    for( i = 0;  i < KheEventResourceTaskCount(m->soln, er);  i++ )
    {
      task = KheEventResourceTask(m->soln, er, i);
      if( KheTaskAsstResource(task) == NULL )
      {
	if( count > 0 )
          HnStringAdd(&ac, "; ");
	HnStringAdd(&ac, "%d %s", KheTaskDuration(task),
	  KheEventName(KheEventResourceEvent(er)));
	count++;
      }
    }
  }
  return HnStringEnd(ac);
}

/* *** old version, too uninformative for resource timetable listings
char *KheAssignResourceMonitorDeviationDescription(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  ARRAY_CHAR ac;  int i, count;  KHE_EVENT_RESOURCE er;  KHE_TASK task;
  MStringInit(ac);
  if( m->deviation == 0 )
    MStringAddString(ac, "0");
  else if( KheAssignResourceMonitorUnassignedTaskCount(m) == 1 )
    MStringAddInt(ac, m->deviation);
  else
  {
    MStringPrintf(ac, 100, "%d: ", m->deviation);
    er = KheAssignResourceMonitorEventResource(m);
    count = 0;
    for( i = 0;  i < KheEventResourceTaskCount(m->soln, er);  i++ )
    {
      task = KheEventResourceTask(m->soln, er, i);
      if( KheTaskAsstResource(task) == NULL )
      {
	if( count > 0 )
          MStringAddString(ac, "; ");
	MStringAddInt(ac, KheTaskDuration(task));
	count++;
      }
    }
  }
  return MStringVal(ac);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceMonitorDeviationCount(KHE_ASSIGN_RESOURCE_MONITOR m)*/
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAssignResourceMonitorDeviationCount(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignResourceMonitorDeviation(                                   */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, int i)                                  */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAssignResourceMonitorDeviation(KHE_ASSIGN_RESOURCE_MONITOR m, int i)
{
  HnAssert(i == 0, "KheAssignResourceMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignResourceMonitorDeviationDescription(                      */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, int i)                                  */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheAssignResourceMonitorDeviationDescription(
  KHE_ASSIGN_RESOURCE_MONITOR m, int i)
{
  HnAssert(i==0, "KheAssignResourceMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignResourceMonitorPointOfApplication(                        */
/*    KHE_ASSIGN_RESOURCE_MONITOR m)                                         */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheAssignResourceMonitorPointOfApplication(
  KHE_ASSIGN_RESOURCE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  char *role, *event_name;
  er = KheAssignResourceMonitorEventResource(m);
  event_name = KheEventName(KheEventResourceEvent(er));
  role = KheEventResourceRole(er);
  return HnStringMake(KheSolnArena(m->soln), "%s role %s", event_name,
    role != NULL ? role : "?");
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignResourceMonitorId(KHE_ASSIGN_RESOURCE_MONITOR m)          */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheAssignResourceMonitorId(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  char *constraint_id, *event_id, *role;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    er = KheAssignResourceMonitorEventResource(m);
    event_id = KheEventId(KheEventResourceEvent(er));
    role = KheEventResourceRole(er);
    m->id = HnStringMake(KheSolnArena(m->soln), "%s/%s/%s",
      constraint_id, event_id, role != NULL ? role : "?");
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
/*  void KheAssignResourceMonitorDebug(KHE_ASSIGN_RESOURCE_MONITOR m,        */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAssignResourceMonitorDebug(KHE_ASSIGN_RESOURCE_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " ");
    KheEventResourceInSolnDebug(m->event_resource_in_soln, 1, -1, fp);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

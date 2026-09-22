
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
/*  FILE:         khe_limit_resources_monitor.c                              */
/*  DESCRIPTION:  A limit resources monitor                                  */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
/* ***
#define DEBUG2 1
#define DEBUG2_ID "DemandConstraint:18A/1Wed:E"
*** */

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_RESOURCES_MONITOR - a limit resources monitor                  */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_resources_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_LIMIT_RESOURCES_CONSTRAINT	constraint;	/* the constraint    */
  int					eg_index;       /* event group index */
  int					minimum;	/* from constraint   */
  int					maximum;	/* from constraint   */
  int					active_durn;	/* active duration   */
  KHE_LIMIT_RESOURCES_MONITOR		copy;		/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_RESOURCES_MONITOR KheLimitResourcesMonitorMake(KHE_SOLN soln,  */
/*    KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index)                        */
/*                                                                           */
/*  Make a limit resources constraint with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_RESOURCES_MONITOR KheLimitResourcesMonitorMake(KHE_SOLN soln,
  KHE_LIMIT_RESOURCES_CONSTRAINT c, int eg_index)
{
  KHE_LIMIT_RESOURCES_MONITOR res;  KHE_EVENT_RESOURCE er;
  KHE_EVENT_RESOURCE_IN_SOLN ers;  int i, count;  HA_ARENA a;

  /* make the object */
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_LIMIT_RESOURCES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->constraint = c;
  res->eg_index = eg_index;
  res->minimum = KheLimitResourcesConstraintMinimum(c);
  res->maximum = KheLimitResourcesConstraintMaximum(c);
  res->active_durn = 0;
  res->deviation = 0;
  res->copy = NULL;

  /* add it to all the event resource in solution objects it monitors */
  count = KheLimitResourcesConstraintEventResourceCount(c, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(c, eg_index, i);
    ers = KheSolnEventResourceInSoln(soln, er);
    KheEventResourceInSolnAddMonitor(ers, (KHE_MONITOR) res);
  }
  /* KheLimitResourcesMonitorCheck(res, "end make"); */
  if( DEBUG1 )
    fprintf(stderr, "KheLimitResourcesMonitorMake returning %s\n",
      KheMonitorId((KHE_MONITOR) res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_RESOURCES_CONSTRAINT KheLimitResourcesMonitorConstraint(       */
/*    KHE_LIMIT_RESOURCES_MONITOR m)                                         */
/*                                                                           */
/*  Return m's constraint.                                                   */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_RESOURCES_CONSTRAINT KheLimitResourcesMonitorConstraint(
  KHE_LIMIT_RESOURCES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/* int KheLimitResourcesMonitorEventGroupIndex(KHE_LIMIT_RESOURCES_MONITOR m)*/
/*                                                                           */
/*  Return the index in m's constraint of the event group which is m's       */
/*  point of application.                                                    */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesMonitorEventGroupIndex(KHE_LIMIT_RESOURCES_MONITOR m)
{
  return m->eg_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorActiveDuration(                             */
/*    KHE_LIMIT_RESOURCES_MONITOR m, int *minimum, int *maximum,             */
/*    int *active_durn)                                                      */
/*                                                                           */
/*  Return the minimum, maximum, and active_durn attributes of m.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorActiveDuration(KHE_LIMIT_RESOURCES_MONITOR m,
  int *minimum, int *maximum, int *active_durn)
{
  *minimum = m->minimum;
  *maximum = m->maximum;
  *active_durn = m->active_durn;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_RESOURCES_MONITOR KheLimitResourcesMonitorCopyPhase1(          */
/*    KHE_LIMIT_RESOURCES_MONITOR m, HA_ARENA a)                             */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_RESOURCES_MONITOR KheLimitResourcesMonitorCopyPhase1(
  KHE_LIMIT_RESOURCES_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_RESOURCES_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->constraint = m->constraint;
    copy->eg_index = m->eg_index;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->active_durn = m->active_durn;
    copy->deviation = m->deviation;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorCopyPhase2(KHE_LIMIT_RESOURCES_MONITOR m)   */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorCopyPhase2(KHE_LIMIT_RESOURCES_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorDelete(KHE_LIMIT_RESOURCES_MONITOR m)       */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitResourcesMonitorDelete(KHE_LIMIT_RESOURCES_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_EVENT_RESOURCE er;  KHE_LIMIT_RESOURCES_CONSTRAINT c;
  if( m->attached )
    KheLimitResourcesMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  c = m->constraint;
  count = KheLimitResourcesConstraintEventResourceCount(c, m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(c, m->eg_index, i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnDeleteMonitor(ers, (KHE_MONITOR) m);
  }
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesDev(KHE_LIMIT_RESOURCES_MONITOR m, int active_durn) */
/*                                                                           */
/*  Return the deviation of m if the active duration (the total duration of  */
/*  monitored tasks which are assigned relevant resources) is active_durn.   */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesDev(KHE_LIMIT_RESOURCES_MONITOR m, int active_durn)
{
  if( active_durn < m->minimum )
    return m->minimum - active_durn;
  else if( active_durn > m->maximum )
    return active_durn - m->maximum;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorFlush(KHE_LIMIT_RESOURCES_MONITOR m)        */
/*                                                                           */
/*  Assuming that active_durn has changed, flush m.                          */
/*                                                                           */
/*****************************************************************************/

static void KheLimitResourcesMonitorFlush(KHE_LIMIT_RESOURCES_MONITOR m)
{
  int new_dev;  KHE_COST new_cost;
  new_dev = KheLimitResourcesDev(m, m->active_durn);
  if( m->deviation != new_dev )
  {
    new_cost = KheMonitorDevToCost((KHE_MONITOR) m, new_dev);
    KheMonitorChangeCost((KHE_MONITOR) m, new_cost);
    m->deviation = new_dev;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorAttachToSoln(KHE_LIMIT_RESOURCES_MONITOR m) */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorAttachToSoln(KHE_LIMIT_RESOURCES_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  m->attached = true;
  HnAssert(m->deviation == 0,
    "KheLimitResourcesMonitorAttachToSoln internal error 1");
  HnAssert(m->active_durn == 0,
    "KheLimitResourcesMonitorAttachToSoln internal error 2");
  KheLimitResourcesMonitorFlush(m);
  count = KheLimitResourcesConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(m->constraint, m->eg_index,i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnAttachMonitor(ers, (KHE_MONITOR) m);
  }
  /* KheLimitResourcesMonitorCheck(m, "end attach"); */
}


/*****************************************************************************/
/*                                                                           */
/* void KheLimitResourcesMonitorDetachFromSoln(KHE_LIMIT_RESOURCES_MONITOR m)*/
/*                                                                           */
/*  Detach m.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorDetachFromSoln(KHE_LIMIT_RESOURCES_MONITOR m)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  count = KheLimitResourcesConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(m->constraint, m->eg_index,i);
    ers = KheSolnEventResourceInSoln(m->soln, er);
    KheEventResourceInSolnDetachMonitor(ers, (KHE_MONITOR) m);
  }
  HnAssert(m->active_durn == 0,
    "KheLimitResourcesMonitorDetachFromSoln internal error 2");
  KheMonitorChangeCost((KHE_MONITOR) m, 0);
  m->deviation = 0;
  m->attached = false;
  /* KheLimitResourcesMonitorCheck(m, "end detach"); */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorRightResource(                              */
/*    KHE_LIMIT_RESOURCES_MONITOR m, KHE_RESOURCE r)                         */
/*                                                                           */
/*  Return true when the presence of r adds to the active duration.          */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorRightResource(
  KHE_LIMIT_RESOURCES_MONITOR m, KHE_RESOURCE r)
{
  return r != NULL && KheResourceGroupContains(
    KheLimitResourcesConstraintDomain(m->constraint), r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorAddTask(KHE_LIMIT_RESOURCES_MONITOR m,      */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Monitor the effect of adding task.                                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorAddTask(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TASK task)
{
  if( KheLimitResourcesMonitorRightResource(m, KheTaskAsstResource(task)) )
  {
    m->active_durn += KheTaskDuration(task);
    KheLimitResourcesMonitorFlush(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorDeleteTask(KHE_LIMIT_RESOURCES_MONITOR m,   */
/*    KHE_TASK task)                                                         */
/*                                                                           */
/*  Monitor the effect of deleting task.                                     */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorDeleteTask(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TASK task)
{
  if( KheLimitResourcesMonitorRightResource(m, KheTaskAsstResource(task)) )
  {
    m->active_durn -= KheTaskDuration(task);
    HnAssert(m->active_durn >= 0,
      "KheLimitResourcesMonitorDeleteTask internal error");
    KheLimitResourcesMonitorFlush(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorSplitTask(KHE_LIMIT_RESOURCES_MONITOR m,    */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that a solution resource has just split into task1 and task2. */
/*  Either both solution resources are assigned the same resource, or        */
/*  they are both unassigned; so this can change nothing.                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorSplitTask(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  /* nothing to do - splitting does not change the active duration */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorMergeTask(KHE_LIMIT_RESOURCES_MONITOR m,    */
/*    KHE_TASK task1, KHE_TASK task2)                                        */
/*                                                                           */
/*  Let m know that task1 and task2 are just about to be merged.             */
/*  Either both solution resources are assigned the same resource, or        */
/*  they are both unassigned.                                                */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorMergeTask(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TASK task1, KHE_TASK task2)
{
  /* nothing to do - merging does not change the active duration */
}


/*****************************************************************************/
/*                                                                           */
/* void KheLimitResourcesMonitorAssignResource(KHE_LIMIT_RESOURCES_MONITOR m,*/
/*    KHE_TASK task, KHE_RESOURCE r)                                         */
/*                                                                           */
/*  Let m know that task has just been assigned resource r.                  */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorAssignResource(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  if( KheLimitResourcesMonitorRightResource(m, r) )
  {
    m->active_durn += KheTaskDuration(task);
    KheLimitResourcesMonitorFlush(m);
  }
  /* KheLimitResourcesMonitorCheck(m, "end assign resource"); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorUnAssignResource(                           */
/*    KHE_LIMIT_RESOURCES_MONITOR m, KHE_TASK task, KHE_RESOURCE r)          */
/*                                                                           */
/*  Let m know that task has just been unassigned resource r.                */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorUnAssignResource(
  KHE_LIMIT_RESOURCES_MONITOR m, KHE_TASK task, KHE_RESOURCE r)
{
  if( KheLimitResourcesMonitorRightResource(m, r) )
  {
    m->active_durn -= KheTaskDuration(task);
    HnAssert(m->active_durn >= 0,
      "KheLimitResourcesMonitorUnAssignResource internal error");
    KheLimitResourcesMonitorFlush(m);
  }
  /* KheLimitResourcesMonitorCheck(m, "end unassign resource"); */
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviation and point of application"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesMonitorDeviation(KHE_LIMIT_RESOURCES_MONITOR m)     */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitResourcesMonitorDeviation(KHE_LIMIT_RESOURCES_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  void AddEntry(KHE_EVENT curr_event, int curr_durn, int curr_count,       */
/*    HA_ARRAY_NCHAR *ac, char sign, bool *not_first)                        */
/*                                                                           */
/*  Add an entry to *ac.                                                     */
/*                                                                           */
/*****************************************************************************/

static void AddEntry(KHE_EVENT curr_event, int curr_durn, int curr_count,
  HA_ARRAY_NCHAR *ac, char sign, bool *not_first)
{
  char buff1[10], buff2[30];

  /* handle sign and *not_first */
  if( *not_first )
    sprintf(buff1, "  %c  ", sign);
  else
    sprintf(buff1, "%s", "");
  *not_first = true;

  /* prepare curr_count and curr_durn */
  if( curr_count > 1 && curr_durn > 1 )
    sprintf(buff2, "%d * %d ", curr_count, curr_durn);
  else if( curr_count > 1 )
    sprintf(buff2, "%d ", curr_count);
  else
    sprintf(buff2, "%d ", curr_durn);

  /* print the entry proper */
  HnStringAdd(ac, "%s%s%s", buff1, buff2, KheEventName(curr_event));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddActiveDurations(KHE_LIMIT_RESOURCES_MONITOR m,                */
/*    HA_ARRAY_NCHAR *ac, char sign, bool *not_first)                        */
/*                                                                           */
/*  Add active durations to the deviation description produced by            */
/*  KheLimitResourcesMonitorDeviationDescription just below.                 */
/*                                                                           */
/*****************************************************************************/

static void KheAddActiveDurations(KHE_LIMIT_RESOURCES_MONITOR m,
  HA_ARRAY_NCHAR *ac, char sign, bool *not_first)
{
  int i, j, count;  KHE_EVENT_RESOURCE er;  KHE_TASK task;
  KHE_EVENT curr_event;  int curr_durn;  int curr_count;
  count = KheLimitResourcesConstraintEventResourceCount(m->constraint,
    m->eg_index);
  curr_event = NULL;
  curr_durn = -1;
  curr_count = 0;
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(m->constraint, m->eg_index,i);
    for( j = 0;  j < KheEventResourceTaskCount(m->soln, er);  j++ )
    {
      task = KheEventResourceTask(m->soln, er, j);
      if( KheLimitResourcesMonitorRightResource(m, KheTaskAsstResource(task)) )
      {
	if( KheEventResourceEvent(er) == curr_event &&
	    KheTaskDuration(task) == curr_durn )
	  curr_count++;
	else
	{
	  /* finish off and print current entry */
	  if( curr_event != NULL )
            AddEntry(curr_event, curr_durn, curr_count, ac, sign, not_first);

	  /* start new entry */
	  curr_event = KheEventResourceEvent(er);
	  curr_durn = KheTaskDuration(task);
	  curr_count = 1;
	}
      }
    }
  }

  /* finish off and print the last entry, if any */
  if( curr_event != NULL )
    AddEntry(curr_event, curr_durn, curr_count, ac, sign, not_first);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitResourcesMonitorDeviationDescription(                      */
/*    KHE_LIMIT_RESOURCES_MONITOR m)                                         */
/*                                                                           */
/*  Return a description of m's deviation.                                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitResourcesMonitorDeviationDescription(
  KHE_LIMIT_RESOURCES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  bool not_first;  HA_ARENA a;
  if( m->active_durn < m->minimum )
  {
    /* m->minimum - m->active_durn */
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d", m->minimum);
    not_first = true;
    KheAddActiveDurations(m, &ac, '-', &not_first);
    return HnStringEnd(ac);
  }
  else if( m->active_durn > m->maximum )
  {
    /* m->active_durn - m->maximum */
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    not_first = false;
    KheAddActiveDurations(m, &ac, '+', &not_first);
    HnStringAdd(&ac, "  -  %d", m->maximum);
    return HnStringEnd(ac);
  }
  else
    return "0";
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitResourcesMonitorPointOfApplication(                        */
/*    KHE_LIMIT_RESOURCES_MONITOR m)                                         */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitResourcesMonitorPointOfApplication(KHE_LIMIT_RESOURCES_MONITOR m)
{
  KHE_EVENT_GROUP eg;
  eg = KheLimitResourcesConstraintEventGroup(m->constraint, m->eg_index);
  if( eg == NULL )
    return "(not an event group)";
  else if( KheEventGroupName(eg) != NULL )
    return KheEventGroupName(eg);
  else if( KheEventGroupEventCount(eg) == 1 )
    return KheEventName(KheEventGroupEvent(eg, 0));
  else
    return "(nameless event group)";
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitResourcesMonitorId(KHE_LIMIT_RESOURCES_MONITOR m)          */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitResourcesMonitorId(KHE_LIMIT_RESOURCES_MONITOR m)
{
  KHE_EVENT_GROUP eg;  char *constraint_id, *eg_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    eg = KheLimitResourcesConstraintEventGroup(m->constraint, m->eg_index);
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
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorTimeRange(KHE_LIMIT_RESOURCES_MONITOR m,    */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for limit resources monitor m.             */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesMonitorTimeRange(KHE_LIMIT_RESOURCES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, count;  KHE_EVENT_RESOURCE er;  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  count = KheLimitResourcesConstraintEventResourceCount(m->constraint,
    m->eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(m->constraint, m->eg_index,i);
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
/*  void KheLimitResourcesMonitorSetSweepTime(                               */
/*    KHE_LIMIT_RESOURCES_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for limit resources monitor m.          */
/*                                                                           */
/*  Implementation note.  Implementing this function by attaching and        */
/*  detaching is not a great way to do it:  if a monitor is detached for     */
/*  some other reason, it goes wrong, and there is no recognition that       */
/*  some of the monitored event resources could be before the cutoff and     */
/*  others could be after it.  But it's good enough to go on with.           */
/*                                                                           */
/*****************************************************************************/

void KheLimitResourcesMonitorSetSweepTime(
  KHE_LIMIT_RESOURCES_MONITOR m, KHE_TIME t)
{
  KHE_TIME first_time, last_time;
  if( KheLimitResourcesMonitorSweepTimeRange(m, &first_time, &last_time)
      && (t == NULL || KheTimeIndex(t) < KheTimeIndex(last_time)) )
  {
    /* m needs to be cut off */
    if( m->attached )
      KheLimitResourcesMonitorDetachFromSoln(m);
  }
  else
  {
    /* m needs to be not cut off */
    if( !m->attached )
      KheLimitResourcesMonitorAttachToSoln(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorSweepTimeRange(                             */
/*    KHE_LIMIT_RESOURCES_MONITOR m, KHE_TIME *first_time,                   */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for limit resources monitor m.        */
/*                                                                           */
/*****************************************************************************/

bool KheLimitResourcesMonitorSweepTimeRange(
  KHE_LIMIT_RESOURCES_MONITOR m, KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheLimitResourcesMonitorTimeRange(m, first_time, last_time);
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

void KheLimitResourcesMonitorDebug(KHE_LIMIT_RESOURCES_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  char *id;
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    if( verbosity >= 3 )
    {
      id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
      fprintf(fp, " %s.%d", id != NULL ? id : "-", m->eg_index);
    }
    fprintf(fp, " LRM (");
    if( m->minimum > 0 )
      fprintf(fp, "min %d, ", m->minimum);
    if( m->maximum < INT_MAX )
    fprintf(fp, "max %d, ", m->maximum);
    fprintf(fp, "curr %d)", m->active_durn);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitResourcesMonitorCheck(KHE_LIMIT_RESOURCES_MONITOR lrm)      */
/*                                                                           */
/*  Check a case where lrm is failing.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitResourcesMonitorCheck(KHE_LIMIT_RESOURCES_MONITOR lrm,
  char *pos)
{
  int i;  KHE_GROUP_MONITOR gm;  KHE_MONITOR m;
  m = (KHE_MONITOR) lrm;
  if( DEBUG2 && strcmp(KheMonitorId(m), DEBUG2_ID) == 0 )
  {
    fprintf(stderr, "  %s: ", pos);
    KheLimitResourcesMonitorDebug(lrm, 2, 0, stderr);
    for( i = 0;  i < KheMonitorParentMonitorCount(m);  i++ )
    {
      gm = KheMonitorParentMonitor(m, i);
      KheGroupMonitorDebug(gm, 1, 4, stderr);
    }
  }
}
*** */

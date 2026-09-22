
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
/*  FILE:         khe_limit_workload_monitor.c                               */
/*  DESCRIPTION:  A limit workload monitor                                   */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DN "Welfare07"
#define DO_DEBUG1(m) (DEBUG1 && 0 ==					\
 strcmp(KheResourceName(KheResourceInSolnResource((m)->resource_in_soln)),DN))
#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG3_ID "Constraint:11/Nurse11"
#define DEBUG3(m) (false && \
  strcmp(KheMonitorId((KHE_MONITOR) m), DEBUG3_ID) == 0)


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_WORKLOAD_MONITOR                                               */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_workload_monitor_rec {
  INHERIT_MONITOR(allow_zero, deviation)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_LIMIT_WORKLOAD_CONSTRAINT	constraint;		/* monitoring this   */
  int				offset;			/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  int				ceiling;		/* user-defined      */
  float				max_workload_per_time;	/* max wk per time   */
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups; /* monitored_tg's    */
  ARRAY_KHE_MONITORED_TIME_GROUP defective_time_groups;	/* defective tg's    */
  KHE_TIME			first_time;		/* time range        */
  KHE_TIME			last_time;		/* time range        */
  KHE_LIMIT_WORKLOAD_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorSetLowerBound(KHE_LIMIT_WORKLOAD_MONITOR m)  */
/*                                                                           */
/*  If m's lower bound is greater than 0, set m->lower_bound to its value.   */
/*                                                                           */
/*  This code only does a serious job when monitoring the whole cycle.       */
/*                                                                           */
/*****************************************************************************/

static void KheLimitWorkloadMonitorSetLowerBound(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_RESOURCE r;  KHE_EVENT_RESOURCE er;  int i, workload;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;

  if( HaArrayCount(m->monitored_time_groups) == 1 )
  {
    tg = KheMonitoredTimeGroupTimeGroup(HaArrayFirst(m->monitored_time_groups));
    ins = KheTimeGroupInstance(tg);
    if( KheTimeGroupTimeCount(tg) == KheInstanceTimeCount(ins) )
    {
      /* find the total workload of preassigned event resources */
      workload = 0;
      r = KheLimitWorkloadMonitorResource(m);
      for( i = 0;  i < KheResourcePreassignedEventResourceCount(r);  i++ )
      {
	er = KheResourcePreassignedEventResource(r, i);
	workload += KheEventResourceWorkload(er);
      }

      /* set the lower bound if non-zero */
      if( workload > m->maximum )
	m->lower_bound = KheMonitorDevToCost((KHE_MONITOR) m,
	  workload - m->maximum);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_WORKLOAD_MONITOR KheLimitWorkloadMonitorMake(                  */
/*    KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_WORKLOAD_CONSTRAINT c)  */
/*                                                                           */
/*  Make a new limit workload monitor object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_WORKLOAD_MONITOR KheLimitWorkloadMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_WORKLOAD_CONSTRAINT c)
{
  KHE_LIMIT_WORKLOAD_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i;
  KHE_RESOURCE_TYPE rt;

  /* make and initialize the monitor object */
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, KheResourceInSolnSoln(rs),
    KHE_LIMIT_WORKLOAD_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  HnAssert(rs != NULL, "KheLimitWorkloadMonitorMake internal error");
  res->resource_in_soln = rs;
  res->constraint = c;
  res->deviation = 0;
  res->offset = offset;
  res->minimum = KheLimitWorkloadConstraintMinimum(c);
  res->maximum = KheLimitWorkloadConstraintMaximum(c);
  res->allow_zero = KheLimitWorkloadConstraintAllowZero(c);
  rt = KheResourceResourceType(KheResourceInSolnResource(rs));
  res->max_workload_per_time = KheResourceTypeMaxWorkloadPerTime(rt);
  res->ceiling = INT_MAX;
  /* res->workload = 0.0; */
  res->copy = NULL;

  /* initialize the monitored time groups and defective ones */
  rtm = KheResourceInSolnTimetableMonitor(rs);
  HaArrayInit(res->monitored_time_groups, a);
  for( i = 0;  i < KheLimitWorkloadConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitWorkloadConstraintTimeGroup(c, i, offset);
    HaArrayAddLast(res->monitored_time_groups,
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg));
  }
  HaArrayInit(res->defective_time_groups, a);
  res->first_time = NULL;
  res->last_time = NULL;

  /* add to rs and set lower bound */
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  KheLimitWorkloadMonitorSetLowerBound(res);
  if( DO_DEBUG1(res) )
  {
    fprintf(stderr, "  ");
    KheLimitWorkloadMonitorDebug(res, 1, -1, stderr);
    fprintf(stderr, "  after make\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_WORKLOAD_MONITOR KheLimitWorkloadMonitorCopyPhase1(            */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, HA_ARENA a)                              */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_WORKLOAD_MONITOR KheLimitWorkloadMonitorCopyPhase1(
  KHE_LIMIT_WORKLOAD_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_WORKLOAD_MONITOR copy;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->resource_in_soln =
      KheResourceInSolnCopyPhase1(m->resource_in_soln, a);
    copy->constraint = m->constraint;
    copy->deviation = m->deviation;
    copy->offset = m->offset;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->allow_zero = m->allow_zero;
    copy->ceiling = m->ceiling;
    /* copy->workload = m->workload; */
    HaArrayInit(copy->monitored_time_groups, a);
    HaArrayForEach(m->monitored_time_groups, mtg, i)
      HaArrayAddLast(copy->monitored_time_groups,
	KheMonitoredTimeGroupCopyPhase1(mtg, a));
    HaArrayInit(copy->defective_time_groups, a);
    HaArrayForEach(m->defective_time_groups, mtg, i)
      HaArrayAddLast(copy->defective_time_groups,
	KheMonitoredTimeGroupCopyPhase1(mtg, a));
    copy->first_time = m->first_time;
    copy->last_time = m->last_time;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorCopyPhase2(KHE_LIMIT_WORKLOAD_MONITOR m)     */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorCopyPhase2(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
    HaArrayForEach(m->monitored_time_groups, mtg, i)
      KheMonitoredTimeGroupCopyPhase2(mtg);
    HaArrayForEach(m->defective_time_groups, mtg, i)
      KheMonitoredTimeGroupCopyPhase2(mtg);
    if( DO_DEBUG1(m) )
    {
      fprintf(stderr, "  ");
      KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "  after copy\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDelete(KHE_LIMIT_WORKLOAD_MONITOR m)         */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitWorkloadMonitorDelete(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  if( m->attached )
    KheLimitWorkloadMonitorDetachFromSoln(m);
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
    fprintf(stderr, "  before delete\n");
  }
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheResourceInSolnDeleteMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MArrayFree(m->monitored_time_groups);
  MArrayFree(m->defective_time_groups);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheLimitWorkloadMonitorResourceInSoln(              */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheLimitWorkloadMonitorResourceInSoln(
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->resource_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_WORKLOAD_CONSTRAINT KheLimitWorkloadMonitorConstraint(         */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Return the constraint monitored by m.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_WORKLOAD_CONSTRAINT KheLimitWorkloadMonitorConstraint(
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/* KHE_RESOURCE KheLimitWorkloadMonitorResource(KHE_LIMIT_WORKLOAD_MONITOR m)*/
/*                                                                           */
/*  Return the resource monitored by m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitWorkloadMonitorResource(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorOffset(KHE_LIMIT_WORKLOAD_MONITOR m)          */
/*                                                                           */
/*  Return the offset attribute of m.                                        */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadMonitorOffset(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorTimeGroupCount(KHE_LIMIT_WORKLOAD_MONITOR m)  */
/*                                                                           */
/*  Return the number of time groups of m.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadMonitorTimeGroupCount(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return HaArrayCount(m->monitored_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitWorkloadMonitorTimeGroup(                         */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int i, float *workload)                  */
/*                                                                           */
/*  Return the i'th time group of m.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitWorkloadMonitorTimeGroup(
  KHE_LIMIT_WORKLOAD_MONITOR m, int i, float *workload)
{
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  mtg = HaArray(m->monitored_time_groups, i);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  *workload = busy_and_idle->workload;
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorRange(KHE_LIMIT_WORKLOAD_MONITOR m,          */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLimitWorkloadMonitorSweepTimeRange
void KheLimitWorkloadMonitorRange(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    HaArrayForEach(m->monitored_time_groups, mtg, i)
    {
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      count = KheTimeGroupTimeCount(tg);
      if( count > 0 )
      {
	index = KheTimeIndex(KheTimeGroupTime(tg, 0));
	if( index < first_index )
          first_index = index;
	index = KheTimeIndex(KheTimeGroupTime(tg, count - 1));
	if( index > last_index )
          last_index = index;
      }
    }
    if( first_index < INT_MAX )
    {
      ins = KheSolnInstance(m->soln);
      m->first_time = KheInstanceTime(ins, first_index);
      m->last_time = KheInstanceTime(ins, last_index);
    }
  }
  *first_time = m->first_time;
  *last_time = m->last_time;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheLimitWorkloadMonitorWorkload(KHE_LIMIT_WORKLOAD_MONITOR m)      */
/*                                                                           */
/*  Return the workload of the resource monitored by m.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheLimitWorkloadMonitorWorkload(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->workload;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorWorkloadAndLimits(                           */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, float *workload,                         */
/*    int *minimum, int *maximum)                                            */
/*                                                                           */
/*  Return the workload, minimum, and maximum.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitWorkloadMonitorWorkloadAndLimits(
  KHE_LIMIT_WORKLOAD_MONITOR m, float *workload,
  int *minimum, int *maximum)
{
  *workload = m->workload;
  *minimum = KheLimitWorkloadConstraintMinimum(m->constraint);
  *maximum = KheLimitWorkloadConstraintMaximum(m->constraint);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When unattached, there are no time group monitors and so any sum         */
/*  over the set of these monitors will be 0.                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorAttachToSoln(KHE_LIMIT_WORKLOAD_MONITOR m)   */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorAttachToSoln(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HnAssert(m->deviation == 0,
    "KheLimitWorkloadMonitorAttachToSoln internal error 1");
  m->attached = true;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupAttachMonitor(mtg, (KHE_MONITOR) m, i);
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
    fprintf(stderr, "  after attach\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDetachFromSoln(KHE_LIMIT_WORKLOAD_MONITOR m) */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorDetachFromSoln(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupDetachMonitor(mtg, (KHE_MONITOR) m, i);
  HnAssert(m->deviation == 0,
    "KheLimitWorkloadMonitorDetachFromSoln internal error 1");
  m->attached = false;
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
    fprintf(stderr, "  after detach\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadMonitorTimeRange(KHE_LIMIT_WORKLOAD_MONITOR m,      */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadMonitorTimeRange(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  int i, first_index, last_index, index, count;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->first_time == NULL )
  {
    first_index = INT_MAX;
    last_index = 0;
    HaArrayForEach(m->monitored_time_groups, mtg, i)
    {
      tg = KheMonitoredTimeGroupTimeGroup(mtg);
      count = KheTimeGroupTimeCount(tg);
      if( count > 0 )
      {
	index = KheTimeIndex(KheTimeGroupTime(tg, 0));
	if( index < first_index )
          first_index = index;
	index = KheTimeIndex(KheTimeGroupTime(tg, count - 1));
	if( index > last_index )
          last_index = index;
      }
    }
    if( first_index < INT_MAX )
    {
      ins = KheSolnInstance(m->soln);
      m->first_time = KheInstanceTime(ins, first_index);
      m->last_time = KheInstanceTime(ins, last_index);
    }
  }
  *first_time = m->first_time;
  *last_time = m->last_time;
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorSetSweepTime(                                */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, KHE_TIME t)                              */
/*                                                                           */
/*  Set the sweep time of m.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorSetSweepTime(
  KHE_LIMIT_WORKLOAD_MONITOR m, KHE_TIME t)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupSetSweepTime(mtg, t);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitWorkloadMonitorSweepTimeRange(KHE_LIMIT_WORKLOAD_MONITOR m, */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the sweep time range of m.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheLimitWorkloadMonitorSweepTimeRange(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheLimitWorkloadMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ceiling"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorSetCeiling(KHE_LIMIT_WORKLOAD_MONITOR m,     */
/*    int ceiling)                                                           */
/*                                                                           */
/*  Set m's ceiling attribute.                                               */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorSetCeiling(KHE_LIMIT_WORKLOAD_MONITOR m,
  int ceiling)
{
  HnAssert(ceiling >= 0, "KheLimitWorkloadMonitorSetCeiling: negative ceiling",
    ceiling);
  if( m->attached )
  {
    KheLimitWorkloadMonitorDetachFromSoln(m);
    m->ceiling = ceiling;
    KheLimitWorkloadMonitorAttachToSoln(m);
  }
  else
    m->ceiling = ceiling;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorCeiling(KHE_LIMIT_WORKLOAD_MONITOR m)         */
/*                                                                           */
/*  Return m's ceiling attribute.                                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadMonitorCeiling(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->ceiling;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls (old)"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorAssignResource(                              */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, KHE_TASK task, KHE_RESOURCE r)           */
/*                                                                           */
/*  Inform m that its resource r is being assigned to task.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitWorkloadMonitorAssignResource(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  float workload, new_workload, new_deviation;
  workload = KheTaskWorkload(task);
  if( workload != 0.0 )
  {
    new_workload = m->workload + workload;
    new_deviation = KheLimitWorkloadMonitorDev(m, new_workload);
    if( new_deviation != m->deviation )
    {
      KheMonitorChangeCost((KHE_MONITOR) m,
        KheMonitorDevToCost((KHE_MONITOR) m, new_deviation));
      m->deviation = new_deviation;
    }
    m->workload = new_workload;
    if( DO_DEBUG1(m) )
    {
      fprintf(stderr, "  ");
      KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "  after assign ");
      KheTaskDebug(task, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorUnAssignResource(                            */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, KHE_TASK task, KHE_RESOURCE r)           */
/*                                                                           */
/*  Inform m that its resource r is being unassigned from task.              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitWorkloadMonitorUnAssignResource(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_TASK task, KHE_RESOURCE r)
{
  float workload, new_workload, new_deviation;
  workload = KheTaskWorkload(task);
  if( workload != 0.0 )
  {
    new_workload = m->workload - workload;
    ** *** have observed this being triggered on 0.000016!
    HnAssert(new_workload >= -0.00001,
      "KheLimitWorkloadMonitorDeleteTask internal error (workload %.6f)",
      new_workload);
    *** **
    new_deviation = KheLimitWorkloadMonitorDev(m, new_workload);
    if( new_deviation != m->deviation )
    {
      KheMonitorChangeCost((KHE_MONITOR) m,
        KheMonitorDevToCost((KHE_MONIOR) m, new_deviation));
      m->deviation = new_deviation;
    }
    m->workload = new_workload;
    if( DO_DEBUG1(m) )
    {
      fprintf(stderr, "  ");
      KheLimitWorkloadMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "  after unassign ");
      KheTaskDebug(task, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDebugOp(KHE_LIMIT_WORKLOAD_MONITOR m,        */
/*    char *op, KHE_BUSY_AND_IDLE old_busy_and_idle,                         */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle, KHE_MONITORED_TIME_GROUP mtg)     */
/*                                                                           */
/*  Debug print of op on mtg.                                                */
/*                                                                           */
/*****************************************************************************/

static void KheLimitWorkloadMonitorDebugOp(KHE_LIMIT_WORKLOAD_MONITOR m,
  char *op, KHE_BUSY_AND_IDLE old_busy_and_idle, int old_dev,
  KHE_BUSY_AND_IDLE new_busy_and_idle, int new_dev,
  KHE_MONITORED_TIME_GROUP mtg)
{
  if( DEBUG3(m) )
    fprintf(stderr, "  %s %-6s [%6.1f %2d] %4d -> [%6.1f %2d] %4d %s\n",
      KheMonitorId((KHE_MONITOR) m), op,
      old_busy_and_idle == NULL ? 0.0 : old_busy_and_idle->workload,
      old_busy_and_idle == NULL ? 0   : old_busy_and_idle->open_count,
      old_dev,
      new_busy_and_idle == NULL ? 0.0 : new_busy_and_idle->workload,
      new_busy_and_idle == NULL ? 0   : new_busy_and_idle->open_count,
      new_dev,
      KheTimeGroupId(KheMonitoredTimeGroupTimeGroup(mtg)));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorDev(KHE_LIMIT_WORKLOAD_MONITOR m,             */
/*    float workload, int open_count)                                        */
/*                                                                           */
/*  Work out the deviation caused by this workload and open_count.           */
/*                                                                           */
/*  Because there is no clear correspondence between open_count and the      */
/*  amount of workload that could possibly be accumulated during open        */
/*  times, all we can say is that if open_count > 0 then violations of       */
/*  the minimum limit can't be taken as certain.                             */
/*                                                                           */
/*****************************************************************************/

static int KheLimitWorkloadMonitorDev(KHE_LIMIT_WORKLOAD_MONITOR m,
  float workload, int open_count)
{
  float adjusted_workload;
  adjusted_workload = workload + open_count * m->max_workload_per_time;
  if( (m->allow_zero && workload < 0.001) || workload > m->ceiling + 0.001 )
    return 0;
  else if( adjusted_workload < m->minimum - 0.001 )
    return (int) ceil(m->minimum - 0.001 - adjusted_workload);
  else if( workload > m->maximum + 0.001 )
    return (int) ceil(workload - m->maximum - 0.001);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorAddDefective(KHE_LIMIT_WORKLOAD_MONITOR m,   */
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Add mtg to m's list of defective time groups, in sorted order.           */
/*                                                                           */
/*****************************************************************************/

static void KheLimitWorkloadMonitorAddDefective(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_MONITORED_TIME_GROUP mtg)
{
  KHE_MONITORED_TIME_GROUP mtg2;  int i;  KHE_TIME_GROUP tg, tg2;
  tg = KheMonitoredTimeGroupTimeGroup(mtg);
  HaArrayAddLast(m->defective_time_groups, NULL);
  for( i = HaArrayCount(m->defective_time_groups) - 2;  i >= 0;  i-- )
  {
    mtg2 = HaArray(m->defective_time_groups, i);
    tg2 = KheMonitoredTimeGroupTimeGroup(mtg2);
    if( KheTimeGroupTypedCmp(tg2, tg) )
      break;
    HaArrayPut(m->defective_time_groups, i + 1, mtg2);
  }
  HaArrayPut(m->defective_time_groups, i + 1, mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDeleteDefective(KHE_LIMIT_WORKLOAD_MONITOR m,*/
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Delete mtg from m's list of defective time groups.  It must be there.    */
/*                                                                           */
/*****************************************************************************/

static void KheLimitWorkloadMonitorDeleteDefective(KHE_LIMIT_WORKLOAD_MONITOR m,
  KHE_MONITORED_TIME_GROUP mtg)
{
  int pos;
  if( !HaArrayContains(m->defective_time_groups, mtg, &pos) )
    HnAbort("KheLimitWorkloadMonitorDeleteDefective internal error");
  HaArrayDeleteAndShift(m->defective_time_groups, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorAddBusyAndIdle(                              */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int index,                               */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add a workload for one monitored time group.                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorAddBusyAndIdle(
  KHE_LIMIT_WORKLOAD_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  int dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  dev = KheLimitWorkloadMonitorDev(m, busy_and_idle->workload,
    busy_and_idle->open_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitWorkloadMonitorAddBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    KheLimitWorkloadMonitorDebugOp(m, "add", NULL, 0, busy_and_idle, dev, mtg);
    HnAssert(!HaArrayContains(m->defective_time_groups, mtg, &pos),
      "KheLimitWorkloadMonitorAddBusyAndIdle internal error 2");
    /* HaArrayAddLast(m->defective_time_groups, mtg); */
    KheLimitWorkloadMonitorAddDefective(m, mtg);
    m->deviation += dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDeleteBusyAndIdle(                           */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int index,                               */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Remove the workload of one monitored time group.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorDeleteBusyAndIdle(
  KHE_LIMIT_WORKLOAD_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  int dev;  KHE_MONITORED_TIME_GROUP mtg;
  dev = KheLimitWorkloadMonitorDev(m, busy_and_idle->workload,
    busy_and_idle->open_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitWorkloadMonitorDeleteBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    KheLimitWorkloadMonitorDebugOp(m, "del", busy_and_idle, dev, NULL, 0, mtg);
    KheLimitWorkloadMonitorDeleteDefective(m, mtg);
    /* ***
    HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
      "KheLimitWorkloadMonitorDeleteBusyAndIdle internal error 2");
    HaArrayDeleteAndShift(m->defective_time_groups, pos);
    *** */
    m->deviation -= dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorChangeBusyAndIdle(                           */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int index,                               */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change the workload of one monitored time group.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorChangeBusyAndIdle(
  KHE_LIMIT_WORKLOAD_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle,
  KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  int old_dev, new_dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  old_dev = KheLimitWorkloadMonitorDev(m, old_busy_and_idle->workload,
    old_busy_and_idle->open_count);
  new_dev = KheLimitWorkloadMonitorDev(m, new_busy_and_idle->workload,
    new_busy_and_idle->open_count);
  if( old_dev != new_dev )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitWorkloadMonitorChangeBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    if( old_dev == 0 )
    {
      KheLimitWorkloadMonitorDebugOp(m, "cadd", old_busy_and_idle, old_dev,
	new_busy_and_idle, new_dev, mtg);
      HnAssert(!HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitWorkloadMonitorChangeBusyAndIdle internal error 2 (%s)",
	KheMonitorId((KHE_MONITOR) m));
      KheLimitWorkloadMonitorAddDefective(m, mtg);
      /* HaArrayAddLast(m->defective_time_groups, mtg); */
    }
    else if( new_dev == 0 )
    {
      KheLimitWorkloadMonitorDebugOp(m, "cdel", old_busy_and_idle, old_dev,
	new_busy_and_idle, new_dev, mtg);
      KheLimitWorkloadMonitorDeleteDefective(m, mtg);
      /* ***
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitWorkloadMonitorChangeBusyAndIdle internal error 3");
      HaArrayDeleteAndShift(m->defective_time_groups, pos);
      *** */
    }
    else
    {
      KheLimitWorkloadMonitorDebugOp(m, "ckeep", old_busy_and_idle, old_dev,
	new_busy_and_idle, new_dev, mtg);
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitWorkloadMonitorChangeBusyAndIdle internal error 4");
    }
    m->deviation += (new_dev - old_dev);
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorDefectiveTimeGroupCount(                      */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Return the number of defective time groups.                              */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return HaArrayCount(m->defective_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDefectiveTimeGroup(                          */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int i, KHE_TIME_GROUP *tg,               */
/*    float *workload, int *minimum, int *maximum, bool *allow_zero)         */
/*                                                                           */
/*  Return the i'th defective time group.                                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorDefectiveTimeGroup(
  KHE_LIMIT_WORKLOAD_MONITOR m, int i, KHE_TIME_GROUP *tg,
  float *workload, int *open_count, int *minimum, int *maximum,
  bool *allow_zero)
{
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  HnAssert(0 <= i && i < HaArrayCount(m->defective_time_groups),
    "KheLimitWorkloadMonitorDefectiveTimeGroup: i (%d) out of range (0 .. %d)",
    i, HaArrayCount(m->defective_time_groups) - 1);
  mtg = HaArray(m->defective_time_groups, i);
  *tg = KheMonitoredTimeGroupTimeGroup(mtg);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  *workload = busy_and_idle->workload;
  *open_count = busy_and_idle->open_count;
  *minimum = m->minimum;
  *maximum = m->maximum;
  *allow_zero = m->allow_zero;
  if( DEBUG2 )
    fprintf(stderr, "  KheLimitWorkloadMonitorDefectiveTimeGroup(%s, %d) = "
      "tg %s, wk %.1f, op %d, min %d, max %d, az %s\n",
      KheLimitWorkloadMonitorId(m), i, KheTimeGroupId(*tg), *workload,
      *open_count, *minimum, *maximum, bool_show(*allow_zero));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitWorkloadMonitorDeviation(KHE_LIMIT_WORKLOAD_MONITOR m)       */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitWorkloadMonitorDeviation(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitWorkloadMonitorDeviationShowOneTimeGroup(                   */
/*    KHE_LIMIT_WORKLOAD_MONITOR m, int i, HA_ARRAY_NCHAR *ac)               */
/*                                                                           */
/*  Show one time group in the deviation description of m.                   */
/*                                                                           */
/*****************************************************************************/

static void KheLimitWorkloadMonitorDeviationShowOneTimeGroup(
  KHE_LIMIT_WORKLOAD_MONITOR m, int i, HA_ARRAY_NCHAR *ac)
{
  float workload, adjusted_workload;  int minimum, maximum, open_count;
  KHE_TIME_GROUP tg; char *name; bool allow_zero;
  KheLimitWorkloadMonitorDefectiveTimeGroup(m, i, &tg, &workload,
    &open_count, &minimum, &maximum, &allow_zero);
  name = KheTimeGroupName(tg);
  adjusted_workload = workload + open_count * m->max_workload_per_time;
  if( adjusted_workload < minimum )
    HnStringAdd(ac, "%d - %s%.1f %s", minimum,
      open_count == 0 ? "" : "(adjusted) ", adjusted_workload,
      name == NULL ? "?" : name);
  else if( workload > maximum )
    HnStringAdd(ac, "%.1f %s - %d", workload,
      name == NULL ? "?" : name, maximum);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitWorkloadMonitorDeviationDescription(                       */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitWorkloadMonitorDeviationDescription(
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    for( i = 0;  i < KheLimitWorkloadMonitorDefectiveTimeGroupCount(m);  i++ )
    {
      if( i > 0 )
	HnStringAdd(&ac, "; ");
      KheLimitWorkloadMonitorDeviationShowOneTimeGroup(m, i, &ac);
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitWorkloadMonitorPointOfApplication(                         */
/*    KHE_LIMIT_WORKLOAD_MONITOR m)                                          */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitWorkloadMonitorPointOfApplication(
  KHE_LIMIT_WORKLOAD_MONITOR m)
{
  char *name;  HA_ARENA a;
  name = KheResourceName(KheLimitWorkloadMonitorResource(m));
  if( KheLimitWorkloadConstraintAppliesToTimeGroup(m->constraint) != NULL )
  {
    a = KheSolnArena(m->soln);
    return HnStringMake(a, "%s offset %d", name, m->offset);
  }
  else
    return name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitWorkloadMonitorId(KHE_LIMIT_WORKLOAD_MONITOR m)            */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitWorkloadMonitorId(KHE_LIMIT_WORKLOAD_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheLimitWorkloadMonitorResource(m));
    a = KheSolnArena(m->soln);
    if( m->offset == 0 )
      m->id = HnStringMake(a, "%s/%s", constraint_id, resource_id);
    else
      m->id = HnStringMake(a, "%s/%s/%d", constraint_id, resource_id,m->offset);
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
/*  void KheLimitWorkloadMonitorDebug(KHE_LIMIT_WORKLOAD_MONITOR m,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitWorkloadMonitorDebug(KHE_LIMIT_WORKLOAD_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  char buff[20];  int i, minimum, maximum, open_count;  KHE_TIME_GROUP tg;
  float workload;  bool allow_zero;
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    /* *** shown in Monitor Id now
    fprintf(fp, " %s", KheResourceInSolnId(m->resource_in_soln));
    if( m->offset != 0 )
      fprintf(fp, "/%d", m->offset);
    *** */
    if( m->ceiling < INT_MAX )
      snprintf(buff, 20, ", ceil %d", m->ceiling);
    else
      snprintf(buff, 20, "%s", "");
    fprintf(fp, " LWKM (min %d, max %d%s, allow_zero %s, dev %d)",
      m->minimum, m->maximum, buff, m->allow_zero ? "true" : "false",
      m->deviation);
    for( i = 0;  i < KheLimitWorkloadMonitorDefectiveTimeGroupCount(m);  i++ )
    {
      KheLimitWorkloadMonitorDefectiveTimeGroup(m, i, &tg,
	&workload, &open_count, &minimum, &maximum, &allow_zero);
      fprintf(fp, " %.1f", workload);
    }
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

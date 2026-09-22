
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
/*  FILE:         khe_limit_effort_monitor.c                                 */
/*  DESCRIPTION:  A limit effor monitor (limit busy times or limit workload) */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DN "Welfare07"
#define DO_DEBUG1(m) (DEBUG1 && 0 ==					\
 strcmp(KheResourceName(KheResourceInSolnResource((m)->resource_in_soln)),DN))
#define bool_show(x) ((x) ? "true" : "false")


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_EFFORT_MONITOR                                                 */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_effort_monitor_rec {
  INHERIT_MONITOR(allow_zero, deviation)
  bool				is_workload;
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_LIMIT_EFFORT_CONSTRAINT	constraint;		/* monitoring this   */
  int				offset;			/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  int				ceiling;		/* user-defined      */
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups; /* monitored_tg's    */
  ARRAY_KHE_MONITORED_TIME_GROUP defective_time_groups;	/* defective tg's    */
  KHE_TIME			first_time;		/* time range        */
  KHE_TIME			last_time;		/* time range        */
  KHE_LIMIT_EFFORT_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorSetLowerBound(KHE_LIMIT_EFFORT_MONITOR m)      */
/*                                                                           */
/*  If m's lower bound is greater than 0, set m->lower_bound to its value.   */
/*                                                                           */
/*  This code only does a serious job when monitoring the whole cycle.       */
/*                                                                           */
/*****************************************************************************/

static void KheLimitEffortMonitorSetLowerBound(KHE_LIMIT_EFFORT_MONITOR m)
{
  KHE_RESOURCE r;  KHE_EVENT_RESOURCE er;  int i, workload;  KHE_TIME_GROUP tg;
  KHE_INSTANCE ins;  int events_durn, min_in_domain, max_in_domain;
  KHE_TIME_GROUP domain;  KHE_CONSTRAINT mc;

  r = KheLimitEffortMonitorResource(m);
  if( m->is_workload )
  {
    if( HaArrayCount(m->monitored_time_groups) == 1 )
    {
      tg = KheMonitoredTimeGroupTimeGroup(HaArrayFirst(m->monitored_time_groups));
      ins = KheTimeGroupInstance(tg);
      if( KheTimeGroupTimeCount(tg) == KheInstanceTimeCount(ins) )
      {
	/* find the total workload of preassigned event resources */
	workload = 0;
	for( i = 0;  i < KheResourcePreassignedEventResourceCount(r);  i++ )
	{
	  er = KheResourcePreassignedEventResource(r, i);
	  workload += KheEventResourceWorkload(er);
	}

	/* set the lower bound if non-zero */
	if( workload > m->maximum )
	  m->lower_bound = KheConstraintCost((KHE_CONSTRAINT) m->constraint,
	    workload - m->maximum);
      }
    }
  }
  else
  {
    /* if r does not have a highly weighted avoid clashes constraint, quit */
    mc = (KHE_CONSTRAINT) m->constraint;
    if( !KheResourceHasAvoidClashesConstraint(r,
	  KheConstraintCombinedWeight(mc)))
      return;  /* no highly weighted avoid clashes constraint */

    /* find the duration of highly weighted preassigned events */
    events_durn = KheResourcePreassignedEventsDuration(r,
      KheConstraintCombinedWeight(mc));

    /* min_in_domain = the min number of domain times occupied by workload */
    ins = KheResourceInstance(r);
    domain = KheLimitEffortConstraintDomain(m->constraint);
    min_in_domain = events_durn -
      (KheInstanceTimeCount(ins) - KheTimeGroupTimeCount(domain));

    /* max_in_domain = the max number of cost-free occupied domain times */
    max_in_domain = m->maximum *
      KheLimitEffortConstraintTimeGroupCount(m->constraint);
    if( min_in_domain > max_in_domain )
    {
      m->lower_bound = KheConstraintCost((KHE_CONSTRAINT) m->constraint,
	min_in_domain - max_in_domain);
      if( DEBUG2 )
	fprintf(stderr, "  set limit busy times lower bound of %s to %.5f\n",
	  KheResourceId(r), KheCostShow(m->lower_bound));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_EFFORT_MONITOR KheLimitEffortMonitorMake(                      */
/*    KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_EFFORT_CONSTRAINT c)    */
/*                                                                           */
/*  Make a new limit workload monitor object with these attributes.          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_EFFORT_MONITOR KheLimitEffortMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_EFFORT_CONSTRAINT c)
{
  KHE_LIMIT_EFFORT_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i;

  /* make and initialize the monitor object */
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, KheResourceInSolnSoln(rs),
    KHE_LIMIT_EFFORT_MONITOR_TAG);
  HnAssert(rs != NULL, "KheLimitEffortMonitorMake internal error");
  res->is_workload = KheLimitEffortConstraintIsWorkload(c);
  res->resource_in_soln = rs;
  res->constraint = c;
  res->deviation = 0;
  res->offset = offset;
  res->minimum = KheLimitEffortConstraintMinimum(c);
  res->maximum = KheLimitEffortConstraintMaximum(c);
  res->allow_zero = KheLimitEffortConstraintAllowZero(c);
  res->ceiling = INT_MAX;
  /* res->workload = 0.0; */
  res->copy = NULL;

  /* initialize the monitored time groups and defective ones */
  rtm = KheResourceInSolnTimetableMonitor(rs);
  HaArrayInit(res->monitored_time_groups, a);
  for( i = 0;  i < KheLimitEffortConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitEffortConstraintTimeGroup(c, i, offset);
    HaArrayAddLast(res->monitored_time_groups,
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg));
  }
  HaArrayInit(res->defective_time_groups, a);
  res->first_time = NULL;
  res->last_time = NULL;

  /* add to rs and set lower bound */
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  KheLimitEffortMonitorSetLowerBound(res);
  if( DO_DEBUG1(res) )
  {
    fprintf(stderr, "  ");
    KheLimitEffortMonitorDebug(res, 1, -1, stderr);
    fprintf(stderr, "  after make\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_EFFORT_MONITOR KheLimitEffortMonitorCopyPhase1(                */
/*    KHE_LIMIT_EFFORT_MONITOR m, HA_ARENA a)                                */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_EFFORT_MONITOR KheLimitEffortMonitorCopyPhase1(
  KHE_LIMIT_EFFORT_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_EFFORT_MONITOR copy;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->is_workload = m->is_workload;
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
/*  void KheLimitEffortMonitorCopyPhase2(KHE_LIMIT_EFFORT_MONITOR m)         */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorCopyPhase2(KHE_LIMIT_EFFORT_MONITOR m)
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
      KheLimitEffortMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "  after copy\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorDelete(KHE_LIMIT_EFFORT_MONITOR m)             */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitEffortMonitorDelete(KHE_LIMIT_EFFORT_MONITOR m)
{
  if( m->attached )
    KheLimitEffortMonitorDetachFromSoln(m);
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitEffortMonitorDebug(m, 1, -1, stderr);
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
/*  KHE_RESOURCE_IN_SOLN KheLimitEffortMonitorResourceInSoln(                */
/*    KHE_LIMIT_EFFORT_MONITOR m)                                            */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheLimitEffortMonitorResourceInSoln(
  KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->resource_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_EFFORT_CONSTRAINT KheLimitEffortMonitorConstraint(             */
/*    KHE_LIMIT_EFFORT_MONITOR m)                                            */
/*                                                                           */
/*  Return the constraint monitored by m.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_EFFORT_CONSTRAINT KheLimitEffortMonitorConstraint(
  KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/* KHE_RESOURCE KheLimitEffortMonitorResource(KHE_LIMIT_EFFORT_MONITOR m)    */
/*                                                                           */
/*  Return the resource monitored by m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitEffortMonitorResource(KHE_LIMIT_EFFORT_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorOffset(KHE_LIMIT_EFFORT_MONITOR m)              */
/*                                                                           */
/*  Return the offset attribute of m.                                        */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortMonitorOffset(KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorTimeGroupCount(KHE_LIMIT_EFFORT_MONITOR m)      */
/*                                                                           */
/*  Return the number of time groups of m.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortMonitorTimeGroupCount(KHE_LIMIT_EFFORT_MONITOR m)
{
  return HaArrayCount(m->monitored_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitEffortMonitorTimeGroup(                           */
/*    KHE_LIMIT_EFFORT_MONITOR m, int i, float *workload)                    */
/*                                                                           */
/*  Return the i'th time group of m.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitEffortMonitorTimeGroup(
  KHE_LIMIT_EFFORT_MONITOR m, int i, int *busy_count, float *workload)
{
  KHE_MONITORED_TIME_GROUP mtg;
  mtg = HaArray(m->monitored_time_groups, i);
  *busy_count = KheMonitoredTimeGroupBusyCount(mtg);
  *workload = KheMonitoredTimeGroupWorkload(mtg);
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorRange(KHE_LIMIT_EFFORT_MONITOR m,              */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorRange(KHE_LIMIT_EFFORT_MONITOR m,
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


/*****************************************************************************/
/*                                                                           */
/*  float KheLimitEffortMonitorWorkload(KHE_LIMIT_EFFORT_MONITOR m)          */
/*                                                                           */
/*  Return the workload of the resource monitored by m.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
float KheLimitEffortMonitorWorkload(KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->workload;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorWorkloadAndLimits(                             */
/*    KHE_LIMIT_EFFORT_MONITOR m, float *workload,                           */
/*    int *minimum, int *maximum)                                            */
/*                                                                           */
/*  Return the workload, minimum, and maximum.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitEffortMonitorWorkloadAndLimits(
  KHE_LIMIT_EFFORT_MONITOR m, float *workload,
  int *minimum, int *maximum)
{
  *workload = m->workload;
  *minimum = KheLimitEffortConstraintMinimum(m->constraint);
  *maximum = KheLimitEffortConstraintMaximum(m->constraint);
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
/*  void KheLimitEffortMonitorAttachToSoln(KHE_LIMIT_EFFORT_MONITOR m)       */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorAttachToSoln(KHE_LIMIT_EFFORT_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HnAssert(m->deviation == 0,
    "KheLimitEffortMonitorAttachToSoln internal error 1");
  m->attached = true;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupAttachMonitor(mtg, (KHE_MONITOR) m, i);
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitEffortMonitorDebug(m, 1, -1, stderr);
    fprintf(stderr, "  after attach\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorDetachFromSoln(KHE_LIMIT_EFFORT_MONITOR m)     */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorDetachFromSoln(KHE_LIMIT_EFFORT_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupDetachMonitor(mtg, (KHE_MONITOR) m, i);
  HnAssert(m->deviation == 0,
    "KheLimitEffortMonitorDetachFromSoln internal error 1");
  m->attached = false;
  if( DO_DEBUG1(m) )
  {
    fprintf(stderr, "  ");
    KheLimitEffortMonitorDebug(m, 1, -1, stderr);
    fprintf(stderr, "  after detach\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ceiling"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorSetCeiling(KHE_LIMIT_EFFORT_MONITOR m,         */
/*    int ceiling)                                                           */
/*                                                                           */
/*  Set m's ceiling attribute.                                               */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorSetCeiling(KHE_LIMIT_EFFORT_MONITOR m,
  int ceiling)
{
  HnAssert(ceiling >= 0, "KheLimitEffortMonitorSetCeiling: negative ceiling",
    ceiling);
  if( m->attached )
  {
    KheLimitEffortMonitorDetachFromSoln(m);
    m->ceiling = ceiling;
    KheLimitEffortMonitorAttachToSoln(m);
  }
  else
    m->ceiling = ceiling;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorCeiling(KHE_LIMIT_EFFORT_MONITOR m)             */
/*                                                                           */
/*  Return m's ceiling attribute.                                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortMonitorCeiling(KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->ceiling;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorDev(KHE_LIMIT_EFFORT_MONITOR m,                 */
/*    float workload)                                                        */
/*                                                                           */
/*  Work out the deviation caused by this much workload.                     */
/*                                                                           */
/*****************************************************************************/

static int KheLimitEffortMonitorWorkloadDev(KHE_LIMIT_EFFORT_MONITOR m,
  float workload)
{

  if( (m->allow_zero && workload < 0.001) || workload > m->ceiling + 0.001 )
    return 0;
  else if( workload < m->minimum - 0.001 )
    return (int) ceil(m->minimum - 0.001 - workload);
  else if( workload > m->maximum + 0.001 )
    return (int) ceil(workload - m->maximum - 0.001);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorBusyTimesDev(KHE_LIMIT_EFFORT_MONITOR m,        */
/*    int busy_count)                                                        */
/*                                                                           */
/*  Work out the deviations caused by this busy_count.                       */
/*                                                                           */
/*****************************************************************************/

static int KheLimitEffortMonitorBusyTimesDev(KHE_LIMIT_EFFORT_MONITOR m,
  int busy_count)
{
  if( (m->allow_zero && busy_count == 0) || busy_count > m->ceiling )
    return 0;
  else if( busy_count < m->minimum )
    return m->minimum - busy_count;
  else if( busy_count > m->maximum )
    return busy_count - m->maximum;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorAddBusyAndIdle(                                */
/*    KHE_LIMIT_EFFORT_MONITOR m, int index,                                 */
/*    int busy_count, int idle_count, float workload)                        */
/*                                                                           */
/*  Add a workload for one monitored time group.                             */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorAddBusyAndIdle(
  KHE_LIMIT_EFFORT_MONITOR m, int index,
  int busy_count, int idle_count, float workload)
{
  int dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->is_workload )
    dev = KheLimitEffortMonitorWorkloadDev(m, workload);
  else
    dev = KheLimitEffortMonitorBusyTimesDev(m, busy_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitEffortMonitorAddBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    HnAssert(!HaArrayContains(m->defective_time_groups, mtg, &pos),
      "KheLimitEffortMonitorAddBusyAndIdle internal error 2");
    HaArrayAddLast(m->defective_time_groups, mtg);
    m->deviation += dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheConstraintCost((KHE_CONSTRAINT) m->constraint, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorDeleteBusyAndIdle(                             */
/*    KHE_LIMIT_EFFORT_MONITOR m, int index,                                 */
/*    int busy_count, int idle_count, float workload)                        */
/*                                                                           */
/*  Remove the workload of one monitored time group.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorDeleteBusyAndIdle(
  KHE_LIMIT_EFFORT_MONITOR m, int index,
  int busy_count, int idle_count, float workload)
{
  int dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->is_workload )
    dev = KheLimitEffortMonitorWorkloadDev(m, workload);
  else
    dev = KheLimitEffortMonitorBusyTimesDev(m, busy_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitEffortMonitorDeleteBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
      "KheLimitEffortMonitorDeleteBusyAndIdle internal error 2");
    HaArrayDeleteAndShift(m->defective_time_groups, pos);
    m->deviation -= dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheConstraintCost((KHE_CONSTRAINT) m->constraint, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorChangeBusyAndIdle(                             */
/*    KHE_LIMIT_EFFORT_MONITOR m, int index,                                 */
/*    int old_busy_count, int new_busy_count,                                */
/*    int old_idle_count, int new_idle_count,                                */
/*    float old_workload, float new_workload)                                */
/*                                                                           */
/*  Change the workload of one monitored time group.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorChangeBusyAndIdle(
  KHE_LIMIT_EFFORT_MONITOR m, int index,
  int old_busy_count, int new_busy_count,
  int old_idle_count, int new_idle_count,
  float old_workload, float new_workload)
{
  int old_dev, new_dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  if( m->is_workload )
  {
    old_dev = KheLimitEffortMonitorWorkloadDev(m, old_workload);
    new_dev = KheLimitEffortMonitorWorkloadDev(m, new_workload);
  }
  else
  {
    old_dev = KheLimitEffortMonitorBusyTimesDev(m, old_busy_count);
    new_dev = KheLimitEffortMonitorBusyTimesDev(m, new_busy_count);
  }
  if( old_dev != new_dev )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitEffortMonitorChangeBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    if( old_dev == 0 )
    {
      HnAssert(!HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitEffortMonitorChangeBusyAndIdle internal error 2");
      HaArrayAddLast(m->defective_time_groups, mtg);
    }
    else if( new_dev == 0 )
    {
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitEffortMonitorChangeBusyAndIdle internal error 3");
      HaArrayDeleteAndShift(m->defective_time_groups, pos);
    }
    else
    {
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitEffortMonitorChangeBusyAndIdle internal error 4");
    }
    m->deviation += (new_dev - old_dev);
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheConstraintCost((KHE_CONSTRAINT) m->constraint, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorDefectiveTimeGroupCount(                        */
/*    KHE_LIMIT_EFFORT_MONITOR m)                                            */
/*                                                                           */
/*  Return the number of defective time groups.                              */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_EFFORT_MONITOR m)
{
  return HaArrayCount(m->defective_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorDefectiveTimeGroup(                            */
/*    KHE_LIMIT_EFFORT_MONITOR m, int i, KHE_TIME_GROUP *tg,                 */
/*    float *workload, int *minimum, int *maximum, bool *allow_zero)         */
/*                                                                           */
/*  Return the i'th defective time group.                                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorDefectiveTimeGroup(
  KHE_LIMIT_EFFORT_MONITOR m, int i, KHE_TIME_GROUP *tg,
  int *busy_count, float *workload, int *minimum, int *maximum,
  bool *allow_zero)
{
  KHE_MONITORED_TIME_GROUP mtg;
  HnAssert(0 <= i && i < HaArrayCount(m->defective_time_groups),
    "KheLimitEffortMonitorDefectiveTimeGroup: i (%d) out of range (0 .. %d)",
    i, HaArrayCount(m->defective_time_groups) - 1);
  mtg = HaArray(m->defective_time_groups, i);
  *tg = KheMonitoredTimeGroupTimeGroup(mtg);
  *busy_count = KheMonitoredTimeGroupBusyCount(mtg);
  *workload = KheMonitoredTimeGroupWorkload(mtg);
  *minimum = m->minimum;
  *maximum = m->maximum;
  *allow_zero = m->allow_zero;
  if( DEBUG2 )
    fprintf(stderr, "  KheLimitEffortMonitorDefectiveTimeGroup(%s, %d) = "
      "tg %s, bc %d, wk %.1f, min %d, max %d, az %s\n",
      KheLimitEffortMonitorId(m), i, KheTimeGroupId(*tg), *busy_count,
      *workload, *minimum, *maximum, bool_show(allow_zero));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitEffortMonitorDeviation(KHE_LIMIT_EFFORT_MONITOR m)           */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitEffortMonitorDeviation(KHE_LIMIT_EFFORT_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitEffortMonitorDeviationShowOneTimeGroup(                     */
/*    KHE_LIMIT_EFFORT_MONITOR m, int i, HA_ARRAY_NCHAR *ac)                 */
/*                                                                           */
/*  Show one time group in the deviation description of m.                   */
/*                                                                           */
/*****************************************************************************/

static void KheLimitEffortMonitorDeviationShowOneTimeGroup(
  KHE_LIMIT_EFFORT_MONITOR m, int i, HA_ARRAY_NCHAR *ac)
{
  float workload;  int minimum, maximum, busy_count;
  KHE_TIME_GROUP tg;  char *name;  bool allow_zero;
  KheLimitEffortMonitorDefectiveTimeGroup(m, i, &tg, &busy_count,
    &workload, &minimum, &maximum, &allow_zero);
  name = KheTimeGroupName(tg);
  if( m->is_workload )
  {
    if( workload < minimum )
      HnStringAdd(ac, "%d - %.1f %s", minimum, workload,
	name == NULL ? "?" : name);
    else if( workload > maximum )
      HnStringAdd(ac, "%.1f %s - %d", workload,
	name == NULL ? "?" : name, maximum);
  }
  else
  {
    if( busy_count < minimum )
      HnStringAdd(ac, "%d - %d %s", minimum, busy_count,
	name != NULL ? name : KheTimeGroupTimeCount(tg) == 1 ?
	KheTimeName(KheTimeGroupTime(tg, 0)) : "?");
    else if( busy_count > maximum )
      HnStringAdd(ac, "%d %s - %d", busy_count,
	name != NULL ? name : KheTimeGroupTimeCount(tg) == 1 ?
	KheTimeName(KheTimeGroupTime(tg, 0)) : "?", maximum);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitEffortMonitorDeviationDescription(                         */
/*    KHE_LIMIT_EFFORT_MONITOR m)                                            */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitEffortMonitorDeviationDescription(
  KHE_LIMIT_EFFORT_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    for( i = 0;  i < KheLimitEffortMonitorDefectiveTimeGroupCount(m);  i++ )
    {
      if( i > 0 )
	HnStringAdd(&ac, "; ");
      KheLimitEffortMonitorDeviationShowOneTimeGroup(m, i, &ac);
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitEffortMonitorPointOfApplication(                           */
/*    KHE_LIMIT_EFFORT_MONITOR m)                                            */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitEffortMonitorPointOfApplication(
  KHE_LIMIT_EFFORT_MONITOR m)
{
  char *name;  HA_ARENA a;
  name = KheResourceName(KheLimitEffortMonitorResource(m));
  if( KheLimitEffortConstraintAppliesToTimeGroup(m->constraint) != NULL )
  {
    a = KheSolnArena(m->soln);
    return HnStringMake(a, "%s offset %d", name, m->offset);
  }
  else
    return name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitEffortMonitorId(KHE_LIMIT_EFFORT_MONITOR m)                */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitEffortMonitorId(KHE_LIMIT_EFFORT_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheLimitEffortMonitorResource(m));
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
/*  void KheLimitEffortMonitorDebug(KHE_LIMIT_EFFORT_MONITOR m,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitEffortMonitorDebug(KHE_LIMIT_EFFORT_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  char buff[20];  int i, minimum, maximum, busy_count;  KHE_TIME_GROUP tg;
  float workload;  bool allow_zero;
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    if( m->ceiling < INT_MAX )
      snprintf(buff, 20, ", ceil %d", m->ceiling);
    else
      snprintf(buff, 20, "%s", "");
    fprintf(fp, " LEM (wk %s, min %d, max %d%s, allow_zero %s, dev %d)",
      bool_show(m->is_workload), m->minimum, m->maximum, buff,
      bool_show(m->allow_zero), m->deviation);
    for( i = 0;  i < KheLimitEffortMonitorDefectiveTimeGroupCount(m);  i++ )
    {
      KheLimitEffortMonitorDefectiveTimeGroup(m, i, &tg,
	&busy_count, &workload, &minimum, &maximum, &allow_zero);
      if( m->is_workload )
	fprintf(fp, " %.1f", workload);
      else
	fprintf(fp, " %d", busy_count);
    }
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

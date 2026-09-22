
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
/*  FILE:         khe_limit_busy_times_monitor.c                             */
/*  DESCRIPTION:  A time group monitor                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG1_RESOURCE "Nurse36"
#define DEBUG1_R(rs) ( DEBUG3 || (DEBUG1 &&				\
  strcmp(KheResourceId(KheResourceInSolnResource(rs)), DEBUG1_RESOURCE)==0) )


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_BUSY_TIMES_MONITOR - monitors busy times                       */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_busy_times_monitor_rec {
  INHERIT_MONITOR(allow_zero, deviation)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT constraint;		/* monitoring this   */
  int				offset;			/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  int				ceiling;		/* user-defined      */
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups; /* monitored_tg's    */
  ARRAY_KHE_MONITORED_TIME_GROUP defective_time_groups;	/* defective tg's    */
  KHE_TIME			first_time;		/* time range        */
  KHE_TIME			last_time;		/* time range        */
  KHE_LIMIT_BUSY_TIMES_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorDebugCost(KHE_LIMIT_BUSY_TIMES_MONITOR m,   */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print the cost of m onto fp.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheLimitBusyTimesMonitorDebugCost(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  FILE *fp)
{
  fprintf(fp, "cost of %p (", (void *) m);
  fprintf(fp, "%d", m->deviation);
  fprintf(fp, ") %.5f\n", KheCostShow(m->cost));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorSetLowerBound(                              */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Set the lower bound of m.                                                */
/*                                                                           */
/*  Implementation note.  Given that offsets only apply when they do         */
/*  not invalidate any times, KheTimeGroupTimeCount(domain) is the same      */
/*  for every offset.  So the lower bound is independent of the offset.      */
/*                                                                           */
/*****************************************************************************/

static void KheLimitBusyTimesMonitorSetLowerBound(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_RESOURCE r;  int events_durn, min_in_domain, max_in_domain;
  KHE_TIME_GROUP domain;  KHE_INSTANCE ins;  /* KHE_CONSTRAINT mc; */

  /* if r does not have a highly weighted avoid clashes constraint, quit */
  r = KheLimitBusyTimesMonitorResource(m);
  /* mc = (KHE_CONSTRAINT) m->constraint; */
  if( !KheResourceHasAvoidClashesConstraint(r, m->combined_weight))
    return;  /* no highly weighted avoid clashes constraint */

  /* find the duration of highly weighted preassigned events */
  events_durn = KheResourcePreassignedEventsDuration(r, m->combined_weight);
    /* Khe ConstraintCombinedWeight(mc)); */

  /* min_in_domain = the min number of domain times occupied by workload */
  ins = KheResourceInstance(r);
  domain = KheLimitBusyTimesConstraintDomain(m->constraint);
  min_in_domain = events_durn -
    (KheInstanceTimeCount(ins) - KheTimeGroupTimeCount(domain));

  /* max_in_domain = the max number of cost-free occupied domain times */
  max_in_domain = m->maximum *
    KheLimitBusyTimesConstraintTimeGroupCount(m->constraint);
  if( min_in_domain > max_in_domain )
  {
    m->lower_bound = KheMonitorDevToCost((KHE_MONITOR) m,
      min_in_domain - max_in_domain);
    if( DEBUG2 )
      fprintf(stderr, "  setting limit busy times lower bound of %s to %.5f\n",
	KheResourceId(r), KheCostShow(m->lower_bound));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_BUSY_TIMES_MONITOR KheLimitBusyTimesMonitorMake(               */
/*    KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)*/
/*                                                                           */
/*  Make a new limit busy times monitor for rs.                              */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_BUSY_TIMES_MONITOR KheLimitBusyTimesMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, int offset, KHE_LIMIT_BUSY_TIMES_CONSTRAINT c)
{
  KHE_LIMIT_BUSY_TIMES_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_TIME_GROUP tg;  int i;
  if( DEBUG1_R(rs) )
    fprintf(stderr, "[ KheLimitBusyTimesMonitorMake(%s, %s)\n",
      KheResourceId(KheResourceInSolnResource(rs)),
      KheConstraintId( (KHE_CONSTRAINT) c));

  /* make and initialize the monitor object */
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_LIMIT_BUSY_TIMES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->resource_in_soln = rs;
  res->constraint = c;
  res->deviation = 0;
  res->offset = offset;
  res->minimum = KheLimitBusyTimesConstraintMinimum(c);
  res->maximum = KheLimitBusyTimesConstraintMaximum(c);
  res->allow_zero = KheLimitBusyTimesConstraintAllowZero(c);
  res->ceiling = INT_MAX;
  res->copy = NULL;

  /* initialize the monitored time groups and defective ones */
  rtm = KheResourceInSolnTimetableMonitor(rs);
  HaArrayInit(res->monitored_time_groups, a);
  for( i = 0;  i < KheLimitBusyTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitBusyTimesConstraintTimeGroup(c, i, offset);
    HaArrayAddLast(res->monitored_time_groups,
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg));
  }
  HaArrayInit(res->defective_time_groups, a);
  res->first_time = NULL;
  res->last_time = NULL;

  /* add to rs and set lower bound */
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  KheLimitBusyTimesMonitorSetLowerBound(res);
  if( DEBUG1_R(rs) )
  {
    fprintf(stderr, "] KheLimitBusyTimesMonitorMake returning, ");
    KheLimitBusyTimesMonitorDebugCost(res, stderr);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_BUSY_TIMES_MONITOR KheLimitBusyTimesMonitorCopyPhase1(         */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, HA_ARENA a)                            */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_BUSY_TIMES_MONITOR KheLimitBusyTimesMonitorCopyPhase1(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_BUSY_TIMES_MONITOR copy;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->resource_in_soln = KheResourceInSolnCopyPhase1(m->resource_in_soln,a);
    copy->constraint = m->constraint;
    copy->deviation = m->deviation;
    copy->offset = m->offset;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->allow_zero = m->allow_zero;
    copy->ceiling = m->ceiling;
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
/* void KheLimitBusyTimesMonitorCopyPhase2(KHE_LIMIT_BUSY_TIMES_MONITOR m)   */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorCopyPhase2(KHE_LIMIT_BUSY_TIMES_MONITOR m)
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
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheLimitBusyTimesMonitorResourceInSoln(             */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheLimitBusyTimesMonitorResourceInSoln(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return m->resource_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_BUSY_TIMES_CONSTRAINT KheLimitBusyTimesMonitorConstraint(      */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the contraint that m is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_BUSY_TIMES_CONSTRAINT KheLimitBusyTimesMonitorConstraint(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitBusyTimesMonitorResource(                           */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the resource monitored by m.                                      */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitBusyTimesMonitorResource(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesMonitorOffset(KHE_LIMIT_BUSY_TIMES_MONITOR m)       */
/*                                                                           */
/*  Return the offset attribute of m.                                        */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesMonitorOffset(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return m->offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesMonitorTimeGroupCount(                              */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the number of time groups of m.                                   */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesMonitorTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return HaArrayCount(m->monitored_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitBusyTimesMonitorTimeGroup(                        */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int i)                                 */
/*                                                                           */
/*  Return the i'th time group of m.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitBusyTimesMonitorTimeGroup(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, int *busy_count)
{
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  mtg = HaArray(m->monitored_time_groups, i);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  *busy_count = busy_and_idle->busy_count;
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorRange(KHE_LIMIT_BUSY_TIMES_MONITOR m,       */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheLimitBusyTimesMonitorSweepTimeRange
void KheLimitBusyTimesMonitorRange(KHE_LIMIT_BUSY_TIMES_MONITOR m,
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
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When unattached, there are no time group monitors and so any sum         */
/*  over the set of these monitors will be 0.                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorAttachToSoln(KHE_LIMIT_BUSY_TIMES_MONITOR m)*/
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorAttachToSoln(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "  KheLimitBusyTimesMonitorAttachToSoln(m) ");
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
  HnAssert(m->deviation == 0,
    "KheLimitBusyTimesMonitorAttachToSoln internal error 1");
  m->attached = true;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupAttachMonitor(mtg, (KHE_MONITOR) m, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorDetachFromSoln(                             */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorDetachFromSoln(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupDetachMonitor(mtg, (KHE_MONITOR) m, i);
  HnAssert(m->deviation == 0,
    "KheLimitBusyTimesMonitorDetachFromSoln internal error 1");
  m->attached = false;
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "  KheLimitBusyTimesMonitorDetachFromSoln(m) ");
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesMonitorTimeRange(KHE_LIMIT_BUSY_TIMES_MONITOR m,   */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Return the time range of m.                                              */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesMonitorTimeRange(KHE_LIMIT_BUSY_TIMES_MONITOR m,
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
/*  void KheLimitBusyTimesMonitorSetSweepTime(                               */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_TIME t)                            */
/*                                                                           */
/*  Set the sweep time of m.                                                 */
/*                                                                           */
/*  Implementation note.  This simply passes t on to each monitored time     */
/*  group, which calls back if its value (including open count) changes.     */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorSetSweepTime(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_TIME t)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupSetSweepTime(mtg, t);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitBusyTimesMonitorSweepTimeRange(                             */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_TIME *first_time,                  */
/*    KHE_TIME *last_time)                                                   */
/*                                                                           */
/*  Return the sweep time range of m.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheLimitBusyTimesMonitorSweepTimeRange(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheLimitBusyTimesMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "ceiling"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorSetCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m,  */
/*    int ceiling)                                                           */
/*                                                                           */
/*  Set m's ceiling attribute.                                               */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorSetCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  int ceiling)
{
  HnAssert(ceiling >= 0, "KheLimitBusyTimesMonitorSetCeiling: negative ceiling",
    ceiling);
  if( m->attached )
  {
    KheLimitBusyTimesMonitorDetachFromSoln(m);
    m->ceiling = ceiling;
    KheLimitBusyTimesMonitorAttachToSoln(m);
  }
  else
    m->ceiling = ceiling;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesMonitorCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m)      */
/*                                                                           */
/*  Return m's ceiling attribute.                                            */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesMonitorCeiling(KHE_LIMIT_BUSY_TIMES_MONITOR m)
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
/*  int KheLimitBusyTimesMonitorDev(KHE_LIMIT_BUSY_TIMES_MONITOR m,          */
/*    int busy_count, int open_count)                                        */
/*                                                                           */
/*  Work out the deviations caused by this busy_count and open_count.        */
/*                                                                           */
/*****************************************************************************/

static int KheLimitBusyTimesMonitorDev(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  int busy_count, int open_count)
{
  if( (m->allow_zero && busy_count == 0) || busy_count > m->ceiling )
    return 0;
  else if( busy_count > m->maximum )
    return busy_count - m->maximum;
  else if( busy_count + open_count < m->minimum )
    return m->minimum - (busy_count + open_count);
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorAddDefective(                               */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_MONITORED_TIME_GROUP mtg)          */
/*                                                                           */
/*  Add mtg to m's list of defective time groups, in sorted order.           */
/*                                                                           */
/*****************************************************************************/

static void KheLimitBusyTimesMonitorAddDefective(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_MONITORED_TIME_GROUP mtg)
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
/*  void KheLimitBusyTimesMonitorDeleteDefective(                            */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_MONITORED_TIME_GROUP mtg)          */
/*                                                                           */
/*  Delete mtg from m's list of defective time groups.  It must be there.    */
/*                                                                           */
/*****************************************************************************/

static void KheLimitBusyTimesMonitorDeleteDefective(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, KHE_MONITORED_TIME_GROUP mtg)
{
  int pos;
  if( !HaArrayContains(m->defective_time_groups, mtg, &pos) )
    HnAbort("KheLimitBusyTimesMonitorDeleteDefective internal error");
  HaArrayDeleteAndShift(m->defective_time_groups, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorAddBusyAndIdle(                             */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,                             */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add a busy and idle value for one monitored time group.                  */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorAddBusyAndIdle(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  int dev;  KHE_MONITORED_TIME_GROUP mtg;
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "[ KheLimitBusyTimesMonitorAddBusyAndIdle(m, %d, %d), ",
      busy_and_idle->busy_count, busy_and_idle->idle_count);
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
  dev = KheLimitBusyTimesMonitorDev(m, busy_and_idle->busy_count,
    busy_and_idle->open_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitBusyTimesMonitorAddBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    KheLimitBusyTimesMonitorAddDefective(m, mtg);
    /* HaArrayAddLast(m->defective_time_groups, mtg); */
    m->deviation += dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "] KheLimitBusyTimesMonitorAddBusyAndIdle returning, ");
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorDeleteBusyAndIdle(                          */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,                             */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Remove the busy value of one monitored time group.                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorDeleteBusyAndIdle(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  int dev;  KHE_MONITORED_TIME_GROUP mtg;
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "[ KheLimitBusyTimesMonitorDeleteBusyAndIdle(m, %d, %d) ",
      busy_and_idle->busy_count, busy_and_idle->idle_count);
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
  dev = KheLimitBusyTimesMonitorDev(m, busy_and_idle->busy_count,
    busy_and_idle->open_count);
  if( dev != 0 )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitBusyTimesMonitorDeleteBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    KheLimitBusyTimesMonitorDeleteDefective(m, mtg);
    /* ***
    HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
      "KheLimitBusyTimesMonitorDeleteBusyAndIdle internal error 2");
    HaArrayDeleteAndShift(m->defective_time_groups, pos);
    *** */
    m->deviation -= dev;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "] KheLimitBusyTimesMonitorDeleteBusyAndIdle returning, ");
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorChangeBusyAndIdle(                          */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,                             */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change the idle value of one monitored time group.                       */
/*                                                                           */
/*  Implementation note.  This code is correct when old_busy_count and       */
/*  new_busy_count are equal, as is possible, so no special case is made.    */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorChangeBusyAndIdle(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle,
  KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  int old_dev, new_dev, pos;  KHE_MONITORED_TIME_GROUP mtg;
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "[ %s(m, %d, %d, %d, %d), ",
      "KheLimitBusyTimesMonitorChangeBusyAndIdle",
      old_busy_and_idle->busy_count, new_busy_and_idle->busy_count,
      old_busy_and_idle->idle_count, new_busy_and_idle->idle_count);
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
  old_dev = KheLimitBusyTimesMonitorDev(m, old_busy_and_idle->busy_count,
    old_busy_and_idle->open_count);
  new_dev = KheLimitBusyTimesMonitorDev(m, new_busy_and_idle->busy_count,
    new_busy_and_idle->open_count);
  if( old_dev != new_dev )
  {
    HnAssert(index >= 0 && index < HaArrayCount(m->monitored_time_groups),
      "KheLimitBusyTimesMonitorChangeBusyAndIdle internal error 1");
    mtg = HaArray(m->monitored_time_groups, index);
    if( old_dev == 0 )
    {
      HnAssert(!HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitBusyTimesMonitorChangeBusyAndIdle internal error 2");
      KheLimitBusyTimesMonitorAddDefective(m, mtg);
      /* HaArrayAddLast(m->defective_time_groups, mtg); */
    }
    else if( new_dev == 0 )
    {
      KheLimitBusyTimesMonitorDeleteDefective(m, mtg);
      /* ***
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitBusyTimesMonitorChangeBusyAndIdle internal error 3");
      HaArrayDeleteAndShift(m->defective_time_groups, pos);
      *** */
    }
    else
    {
      HnAssert(HaArrayContains(m->defective_time_groups, mtg, &pos),
	"KheLimitBusyTimesMonitorChangeBusyAndIdle internal error 4");
    }
    m->deviation += (new_dev - old_dev);
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
  if( DEBUG1_R(m->resource_in_soln) )
  {
    fprintf(stderr, "] KheLimitBusyTimesMonitorChangeBusyAndIdle returning, ");
    KheLimitBusyTimesMonitorDebugCost(m, stderr);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesMonitorDefectiveTimeGroupCount(                     */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the number of defective time groups.                              */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesMonitorDefectiveTimeGroupCount(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return HaArrayCount(m->defective_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorDefectiveTimeGroup(                         */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,             */
/*    int *busy_count, int *minimum, int *maximum, bool *allow_zero)         */
/*                                                                           */
/*  Return the i'th defective time group.                                    */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorDefectiveTimeGroup(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, KHE_TIME_GROUP *tg,
  int *busy_count, int *open_count, int *minimum, int *maximum,
  bool *allow_zero)
{
  KHE_MONITORED_TIME_GROUP mtg;  KHE_BUSY_AND_IDLE busy_and_idle;
  mtg = HaArray(m->defective_time_groups, i);
  *tg = KheMonitoredTimeGroupTimeGroup(mtg);
  busy_and_idle = KheMonitoredTimeGroupBusyAndIdle(mtg);
  *busy_count = busy_and_idle->busy_count;
  *open_count = busy_and_idle->open_count;
  *minimum = m->minimum;
  *maximum = m->maximum;
  *allow_zero = m->allow_zero;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitBusyTimesMonitorDeviation(KHE_LIMIT_BUSY_TIMES_MONITOR m)    */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitBusyTimesMonitorDeviation(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitBusyTimesMonitorDeviationShowOneTimeGroup(                  */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, HA_ARRAY_NCHAR *ac)             */
/*                                                                           */
/*  Show one time group in the deviation description of m.                   */
/*                                                                           */
/*****************************************************************************/

static void KheLimitBusyTimesMonitorDeviationShowOneTimeGroup(
  KHE_LIMIT_BUSY_TIMES_MONITOR m, int i, HA_ARRAY_NCHAR *ac)
{
  int busy_count, open_count, minimum, maximum;  KHE_TIME_GROUP tg;
  char *name;  bool allow_zero;
  KheLimitBusyTimesMonitorDefectiveTimeGroup(m, i, &tg, &busy_count,
    &open_count, &minimum, &maximum, &allow_zero);
  name = KheTimeGroupName(tg);
  if( busy_count > maximum )
    HnStringAdd(ac, "%d %s - %d", busy_count,
      name != NULL ? name : KheTimeGroupTimeCount(tg) == 1 ?
      KheTimeName(KheTimeGroupTime(tg, 0)) : "?", maximum);
  else if( busy_count + open_count < minimum )
  {
    if( open_count == 0 )
      HnStringAdd(ac, "%d - %d %s", minimum, busy_count,
	name != NULL ? name : KheTimeGroupTimeCount(tg) == 1 ?
	KheTimeName(KheTimeGroupTime(tg, 0)) : "?");
    else
      HnStringAdd(ac, "%d - (%d busy + %d open) %s", minimum, busy_count,
	open_count, name != NULL ? name : KheTimeGroupTimeCount(tg) == 1 ?
	KheTimeName(KheTimeGroupTime(tg, 0)) : "?");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitBusyTimesMonitorDeviationDescription(                      */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitBusyTimesMonitorDeviationDescription(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    for( i = 0;  i < KheLimitBusyTimesMonitorDefectiveTimeGroupCount(m);  i++ )
    {
      if( i > 0 )
	HnStringAdd(&ac, "; ");
      KheLimitBusyTimesMonitorDeviationShowOneTimeGroup(m, i, &ac);
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitBusyTimesMonitorPointOfApplication(                        */
/*    KHE_LIMIT_BUSY_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitBusyTimesMonitorPointOfApplication(
  KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  char *name;  HA_ARENA a;
  name = KheResourceName(KheLimitBusyTimesMonitorResource(m));
  if( KheLimitBusyTimesConstraintAppliesToTimeGroup(m->constraint) != NULL )
  {
    a = KheSolnArena(m->soln);
    return HnStringMake(a, "%s offset %d", name, m->offset);
  }
  else
    return name;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitBusyTimesMonitorId(KHE_LIMIT_BUSY_TIMES_MONITOR m)         */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitBusyTimesMonitorId(KHE_LIMIT_BUSY_TIMES_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheLimitBusyTimesMonitorResource(m));
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
/*  void KheLimitBusyTimesMonitorDebug(KHE_LIMIT_BUSY_TIMES_MONITOR m,       */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLimitBusyTimesMonitorDebug(KHE_LIMIT_BUSY_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  char buff[20];
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    /* *** shown in MonitorId now
    fprintf(fp, " %s", KheResourceInSolnId(m->resource_in_soln));
    if( m->offset != 0 )
      fprintf(fp, "/%d", m->offset);
    *** */
    if( m->ceiling < INT_MAX )
      snprintf(buff, 20, ", ceil %d", m->ceiling);
    else
      snprintf(buff, 20, "%s", "");
    fprintf(fp, " LBTM (min %d, max %d%s, allow_zero %s, dev %d)",
      m->minimum, m->maximum, buff, m->allow_zero ? "true" : "false",
      m->deviation);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

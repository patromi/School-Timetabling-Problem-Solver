
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
/*  FILE:         khe_limit_idle_times_monitor.c                             */
/*  DESCRIPTION:  A limit idle times monitor                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1(m) (false && strcmp(KheMonitorId((KHE_MONITOR) m), "S4/t24")==0)

/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_IDLE_TIMES_MONITOR - monitors idle times                       */
/*                                                                           */
/*****************************************************************************/

struct khe_limit_idle_times_monitor_rec {
  INHERIT_MONITOR(unused, idle_count)
  KHE_RESOURCE_IN_SOLN		resource_in_soln;	/* enclosing rs      */
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups;	/* monitored tg's    */
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT constraint;		/* monitoring this   */
  int				minimum;		/* from constraint   */
  int				maximum;		/* from constraint   */
  KHE_LIMIT_IDLE_TIMES_MONITOR	copy;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* int KheLimitIdleTimesMonitorTimeGroupCount(KHE_LIMIT_IDLE_TIMES_MONITOR m)*/
/*                                                                           */
/*  Return the number of time groups monitored by m.                         */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesMonitorTimeGroupCount(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return HaArrayCount(m->monitored_time_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheLimitIdleTimesMonitorTimeGroup(                        */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, int i)                                 */
/*                                                                           */
/*  Return the i'th time group of m.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitIdleTimesMonitorTimeGroup(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int i)
{
  KHE_MONITORED_TIME_GROUP mtg;
  mtg = HaArray(m->monitored_time_groups, i);
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorTimeGroupState(                             */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, int i, int *busy_count, int *idle_count*/
/*    KHE_TIME extreme_busy_times[2], int *extreme_busy_times_count)         */
/*                                                                           */
/*  Return the state of the i'th time group of m.                            */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheLimitIdleTimesMonitorTimeGroupState(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int i, int *busy_count, int *idle_count,
  KHE_TIME extreme_busy_times[2], int *extreme_busy_times_count)
{
  KHE_MONITORED_TIME_GROUP mtg;
  mtg = HaArray(m->monitored_time_groups, i);
  KheMonitoredTimeGroupIdleState(mtg, busy_count, idle_count,
    extreme_busy_times, extreme_busy_times_count);
  return KheMonitoredTimeGroupTimeGroup(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_IDLE_TIMES_MONITOR KheLimitIdleTimesMonitorMake(               */
/*    KHE_RESOURCE_IN_SOLN rs, KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)            */
/*                                                                           */
/*  Make a new limit idle times monitor for rs.                              */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_IDLE_TIMES_MONITOR KheLimitIdleTimesMonitorMake(
  KHE_RESOURCE_IN_SOLN rs, KHE_LIMIT_IDLE_TIMES_CONSTRAINT c)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  int i;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  soln = KheResourceInSolnSoln(rs);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_LIMIT_IDLE_TIMES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->resource_in_soln = rs;
  rtm = KheResourceInSolnTimetableMonitor(rs);
  HaArrayInit(res->monitored_time_groups, a);
  for( i = 0;  i < KheLimitIdleTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitIdleTimesConstraintTimeGroup(c, i);
    HaArrayAddLast(res->monitored_time_groups,
      KheResourceTimetableMonitorAddMonitoredTimeGroup(rtm, tg));
  }
  res->constraint = c;
  res->minimum = KheLimitIdleTimesConstraintMinimum(c);
  res->maximum = KheLimitIdleTimesConstraintMaximum(c);
  res->idle_count = 0;
  res->copy = NULL;
  KheResourceInSolnAddMonitor(rs, (KHE_MONITOR) res);
  if( DEBUG1(res) )
    fprintf(stderr, "  KheLimitIdleTimesMonitorMake returning %s, %d idle\n",
      KheMonitorId((KHE_MONITOR) res), res->idle_count);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_IDLE_TIMES_MONITOR KheLimitIdleTimesMonitorCopyPhase1(         */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, HA_ARENA a)                            */
/*                                                                           */
/*  Carry out Phase 1 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_IDLE_TIMES_MONITOR KheLimitIdleTimesMonitorCopyPhase1(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_LIMIT_IDLE_TIMES_MONITOR copy;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->resource_in_soln =
      KheResourceInSolnCopyPhase1(m->resource_in_soln, a);
    HaArrayInit(copy->monitored_time_groups, a);
    HaArrayForEach(m->monitored_time_groups, mtg, i)
      HaArrayAddLast(copy->monitored_time_groups,
	KheMonitoredTimeGroupCopyPhase1(mtg, a));
    copy->constraint = m->constraint;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->idle_count = m->idle_count;
    /* copy->new_idle_count = m->new_idle_count; */
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorCopyPhase2(KHE_LIMIT_IDLE_TIMES_MONITOR m)  */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorCopyPhase2(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheResourceInSolnCopyPhase2(m->resource_in_soln);
    HaArrayForEach(m->monitored_time_groups, mtg, i)
      KheMonitoredTimeGroupCopyPhase2(mtg);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorDelete(KHE_LIMIT_IDLE_TIMES_MONITOR m)      */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLimitIdleTimesMonitorDelete(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  if( m->attached )
    KheLimitIdleTimesMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheResourceInSolnDeleteMonitor(m->resource_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_IN_SOLN KheLimitIdleTimesMonitorResourceInSoln(             */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the resource monitor holding m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_IN_SOLN KheLimitIdleTimesMonitorResourceInSoln(
  KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return m->resource_in_soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LIMIT_IDLE_TIMES_CONSTRAINT KheLimitIdleTimesMonitorConstraint(      */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the contraint that m is monitoring.                               */
/*                                                                           */
/*****************************************************************************/

KHE_LIMIT_IDLE_TIMES_CONSTRAINT KheLimitIdleTimesMonitorConstraint(
  KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheLimitIdleTimesMonitorResource(                           */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return the resource that m is monitoring.                                */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheLimitIdleTimesMonitorResource(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return KheResourceInSolnResource(m->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When unlinked, there are no time group monitors and so any sum over      */
/*  them, including the unlinked deviation, must be 0.  So the unlinked      */
/*  deviation is m->minimum.                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorAttachToSoln(KHE_LIMIT_IDLE_TIMES_MONITOR m)*/
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorAttachToSoln(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  m->attached = true;
  if( m->minimum > 0 )
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->minimum));
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupAttachMonitor(mtg, (KHE_MONITOR) m, i);
  /* KheResourceInSolnAttachMonitor(m->resource_in_soln, (KHE_MONITOR) m); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorDetachFromSoln(                             */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorDetachFromSoln(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i;
  HaArrayForEach(m->monitored_time_groups, mtg, i)
    KheMonitoredTimeGroupDetachMonitor(mtg, (KHE_MONITOR) m, i);
  /* KheResourceInSolnDetachMonitor(m->resource_in_soln, (KHE_MONITOR) m); */
  if( m->minimum > 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, 0);
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesMonitorTimeRange(KHE_LIMIT_IDLE_TIMES_MONITOR m,   */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesMonitorTimeRange(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_LIMIT_IDLE_TIMES_CONSTRAINT c;  int i;  KHE_TIME_GROUP tg;
  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  c = KheLimitIdleTimesMonitorConstraint(m);
  for( i = 0;  i < KheLimitIdleTimesConstraintTimeGroupCount(c);  i++ )
  {
    tg = KheLimitIdleTimesConstraintTimeGroup(c, i);
    if( KheTimeGroupTimeCount(tg) > 0 )
    {
      ft = KheTimeGroupTime(tg, 0);
      lt = KheTimeGroupTime(tg, KheTimeGroupTimeCount(tg) - 1);
      if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
	*first_time = ft;
      if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
	*last_time = lt;
    }
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorSetSweepTime(KHE_LIMIT_IDLE_TIMES_MONITOR m,*/
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorSetSweepTime(KHE_LIMIT_IDLE_TIMES_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitIdleTimesMonitorSweepTimeRange(                             */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m,                                        */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KheLimitIdleTimesMonitorSweepTimeRange(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return KheLimitIdleTimesMonitorTimeRange(m, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesMonitorDev(KHE_LIMIT_IDLE_TIMES_MONITOR m,          */
/*    int idle_count)                                                        */
/*                                                                           */
/*  Work out the deviation caused by this many idle times.                   */
/*                                                                           */
/*****************************************************************************/

static int KheLimitIdleTimesMonitorDev(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  int idle_count)
{
  if( idle_count < m->minimum )
    return m->minimum - idle_count;
  else if( idle_count > m->maximum )
    return idle_count - m->maximum;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorFlush(KHE_LIMIT_IDLE_TIMES_MONITOR m)       */
/*                                                                           */
/*  Flush m.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheLimitIdleTimesMonitorFlush(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  int delta)
{
  int old_dev, new_dev;
  if( delta != 0 )
  {
    old_dev = KheLimitIdleTimesMonitorDev(m, m->idle_count);
    new_dev = KheLimitIdleTimesMonitorDev(m, m->idle_count + delta);
    if( old_dev != new_dev )
      KheMonitorChangeCost((KHE_MONITOR) m,
        KheMonitorDevToCost((KHE_MONITOR) m, new_dev));
    m->idle_count += delta;
    if( DEBUG1(m) )
      fprintf(stderr, "  KheLimitIdleTimesMonitorFlush %s, %d = %d + %d\n",
	KheMonitorId((KHE_MONITOR) m), m->idle_count, m->idle_count - delta,
	delta);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorAddBusyAndIdle(                             */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,                             */
/*    KHE_BUSY_AND_IDLE busy_and_idle)                                       */
/*                                                                           */
/*  Add an idle value for one monitored time group.                          */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorAddBusyAndIdle(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  if( DEBUG1(m) )
    fprintf(stderr,
      "  KheLimitIdleTimesMonitorAddBusyAndIdle(%s, index %d, %d)\n",
      KheMonitorId((KHE_MONITOR) m), index, busy_and_idle->idle_count);
  KheLimitIdleTimesMonitorFlush(m, busy_and_idle->idle_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorDeleteBusyAndIdle(                          */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,                             */
/*    int busy_count, int idle_count, float workload)                        */
/*                                                                           */
/*  Remove the idle value of one monitored time group.                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorDeleteBusyAndIdle(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE busy_and_idle)
{
  if( DEBUG1(m) )
    fprintf(stderr,
      "  KheLimitIdleTimesMonitorDeleteBusyAndIdle(%s, index %d, %d)\n",
      KheMonitorId((KHE_MONITOR) m), index, busy_and_idle->idle_count);
  KheLimitIdleTimesMonitorFlush(m, - busy_and_idle->idle_count);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLimitIdleTimesMonitorChangeBusyAndIdle(                          */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,                             */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle,                                   */
/*    KHE_BUSY_AND_IDLE new_busy_and_idle)                                   */
/*                                                                           */
/*  Change the idle value of one monitored time group.                       */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorChangeBusyAndIdle(
  KHE_LIMIT_IDLE_TIMES_MONITOR m, int index,
  KHE_BUSY_AND_IDLE old_busy_and_idle,
  KHE_BUSY_AND_IDLE new_busy_and_idle)
{
  if( DEBUG1(m) )
    fprintf(stderr, "  KheLimitIdleTimesMonitorChangeBusyAndIdle"
      "(%s, index %d, old %d, new %d)\n", KheMonitorId((KHE_MONITOR) m),
      index, old_busy_and_idle->idle_count, new_busy_and_idle->idle_count);
  KheLimitIdleTimesMonitorFlush(m,
    new_busy_and_idle->idle_count - old_busy_and_idle->idle_count);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLimitIdleTimesMonitorDeviation(KHE_LIMIT_IDLE_TIMES_MONITOR m)    */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLimitIdleTimesMonitorDeviation(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return KheLimitIdleTimesMonitorDev(m, m->idle_count);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitIdleTimesMonitorDeviationDescription(                      */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLimitIdleTimesMonitorDeviationDescription(
  KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count;  char *name;  KHE_TIME_GROUP tg;
  int busy_count, idle_count, extreme_busy_times_count;  HA_ARENA a;
  KHE_TIME extreme_busy_times[2];
  if( KheLimitIdleTimesMonitorDeviation(m) == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d too %s in ", KheLimitIdleTimesMonitorDeviation(m),
      m->idle_count < m->minimum ? "few" : m->maximum > 0 ? "many" : "?");
    count = 0;
    for( i = 0;  i < KheLimitIdleTimesMonitorTimeGroupCount(m);  i++ )
    {
      tg = KheLimitIdleTimesMonitorTimeGroupState(m, i, &busy_count,
	&idle_count, extreme_busy_times, &extreme_busy_times_count);
      if( idle_count > 0 )
      {
	if( count > 0 )
          HnStringAdd(&ac, ", ");
	name = KheTimeGroupName(tg);
	HnStringAdd(&ac, "%s", name == NULL ? "?" : name);
	count++;
      }
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitIdleTimesMonitorPointOfApplication(                        */
/*    KHE_LIMIT_IDLE_TIMES_MONITOR m)                                        */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLimitIdleTimesMonitorPointOfApplication(
  KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  return KheResourceName(KheLimitIdleTimesMonitorResource(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLimitIdleTimesMonitorId(KHE_LIMIT_IDLE_TIMES_MONITOR m)         */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLimitIdleTimesMonitorId(KHE_LIMIT_IDLE_TIMES_MONITOR m)
{
  char *constraint_id, *resource_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    resource_id = KheResourceId(KheLimitIdleTimesMonitorResource(m));
    a = KheSolnArena(m->soln);
    m->id = HnStringMake(a, "%s/%s", constraint_id, resource_id);
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
/*  void KheLimitIdleTimesMonitorDebug(KHE_LIMIT_IDLE_TIMES_MONITOR m,       */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp at the given indent.                            */
/*                                                                           */
/*****************************************************************************/

void KheLimitIdleTimesMonitorDebug(KHE_LIMIT_IDLE_TIMES_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    if( m->minimum > 0 )
      fprintf(fp, " LITM (min %d, max %d, idle %d)", m->minimum, m->maximum,
	m->idle_count);
    else
      fprintf(fp, " LITM (max %d, idle %d)", m->maximum, m->idle_count);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

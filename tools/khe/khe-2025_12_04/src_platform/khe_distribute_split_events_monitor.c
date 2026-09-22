
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
/*  FILE:         khe_distribute_split_events_monitor.c                      */
/*  DESCRIPTION:  A distribute split events monitor                          */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR - an distribute split events monitor */
/*                                                                           */
/*****************************************************************************/

struct khe_distribute_split_events_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  int				meet_count;		/* with duration     */
  KHE_EVENT_IN_SOLN		event_in_soln;		/* enclosing es      */
  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT constraint;	/* constraint        */
  int				duration;		/* Duration          */
  int				minimum;		/* Minimum           */
  int				maximum;		/* Maximum           */
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR copy;		/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR KheDistributeSplitEventsMonitorMake( */
/*    KHE_EVENT_IN_SOLN es, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)        */
/*                                                                           */
/*  Make a new distribute split events monitor with these attributes.        */
/*                                                                           */
/*****************************************************************************/

KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR KheDistributeSplitEventsMonitorMake(
  KHE_EVENT_IN_SOLN es, KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT c)
{
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheEventInSolnSoln(es);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->event_in_soln = es;
  res->constraint = c;
  res->duration = KheDistributeSplitEventsConstraintDuration(c);
  res->minimum = KheDistributeSplitEventsConstraintMinimum(c);
  res->maximum = KheDistributeSplitEventsConstraintMaximum(c);
  res->meet_count = 0;
  res->deviation = 0;
  res->copy = NULL;
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR                                      */
/*    KheDistributeSplitEventsMonitorCopyPhase1(                             */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, HA_ARENA a)                     */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR KheDistributeSplitEventsMonitorCopyPhase1(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, HA_ARENA a)
{
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->event_in_soln = KheEventInSolnCopyPhase1(m->event_in_soln, a);
    copy->constraint = m->constraint;
    copy->duration = m->duration;
    copy->minimum = m->minimum;
    copy->maximum = m->maximum;
    copy->meet_count = m->meet_count;
    copy->deviation = m->deviation;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorCopyPhase2(                          */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorCopyPhase2(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheEventInSolnCopyPhase2(m->event_in_soln);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorDelete(                              */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheDistributeSplitEventsMonitorDelete(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  if( m->attached )
    KheDistributeSplitEventsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheEventInSolnDeleteMonitor(m->event_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT                                   */
/*    KheDistributeSplitEventsMonitorConstraint(                             */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_DISTRIBUTE_SPLIT_EVENTS_CONSTRAINT
  KheDistributeSplitEventsMonitorConstraint(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheDistributeSplitEventsMonitorEvent(                          */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the event that m is monitoring.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheDistributeSplitEventsMonitorEvent(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  return KheEventInSolnEvent(m->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeEventsMonitorLimits(                                   */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,                                 */
/*    int *duration, int *minimum, int *maximum, int *meet_count)            */
/*                                                                           */
/*  Return these attributes of m.                                            */
/*                                                                           */
/*****************************************************************************/

void KheDistributeEventsMonitorLimits(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,
  int *duration, int *minimum, int *maximum, int *meet_count)
{
  *duration = m->duration;
  *minimum = m->minimum;
  *maximum = m->maximum;
  *meet_count = m->meet_count;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  When m is unlinked, it is monitoring 0 events, so there are no events    */
/*  of the wanted duration, and the unlinked deviation is m->minimum.        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorAttachToSoln(                        */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorAttachToSoln(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  m->attached = true;
  m->deviation = m->minimum;
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  KheEventInSolnAttachMonitor(m->event_in_soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorDetachFromSoln(                      */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorDetachFromSoln(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  KheEventInSolnDetachMonitor(m->event_in_soln, (KHE_MONITOR) m);
  HnAssert(m->deviation == m->minimum,
    "KheDistributeSplitEventsMonitorDetach internal error");
  m->deviation = 0;
  KheMonitorChangeCost((KHE_MONITOR) m, 0);
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorAttachCheck(                         */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Check the attachment of m.  Since event splitting is considered to be    */
/*  structural, the way to do this is to ensure it is attached, and then     */
/*  detach it if its cost is 0.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheDistributeSplitEventsMonitorAttachCheck(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
    KheMonitorAttachToSoln((KHE_MONITOR) m);
  if( m->cost == 0 )
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
/*  bool KheDistributeSplitEventsMonitorTimeRange(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,                                 */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheDistributeSplitEventsMonitorTimeRange(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT e;
  e = KheDistributeSplitEventsMonitorEvent(m);
  return KheEventTimeRange(e, m->soln, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorSetSweepTime(                        */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_TIME t)                     */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorSetSweepTime(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDistributeSplitEventsMonitorSweepTimeRange(                      */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,                                 */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KheDistributeSplitEventsMonitorSweepTimeRange(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return *first_time = *last_time = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsMonitorAddOneMeet(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Do what's required to add one meet of the correct duration, and return   */
/*  the change in deviation.                                                 */
/*                                                                           */
/*****************************************************************************/

static int KheDistributeSplitEventsMonitorAddOneMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  m->meet_count++;
  if( m->meet_count <= m->minimum )
    return -1;
  else if( m->meet_count > m->maximum )
    return 1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsMonitorDeleteOneMeet(                        */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Do what's required to delete one meet of the correct duration, and       */
/*  return the change in deviation.                                          */
/*                                                                           */
/*****************************************************************************/

static int KheDistributeSplitEventsMonitorDeleteOneMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  m->meet_count--;
  if( m->meet_count < m->minimum )
    return 1;
  else if( m->meet_count >= m->maximum )
    return -1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorAddMeet(                             */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet)                  */
/*                                                                           */
/*  Monitor the effect of adding meet.s                                      */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorAddMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet)
{
  int delta_deviation;
  if( KheMeetDuration(meet) == m->duration )
  {
    /* update meet_count and find the change in deviation */
    delta_deviation = KheDistributeSplitEventsMonitorAddOneMeet(m);

    /* do the update if deviations have changed */
    if( delta_deviation != 0 )
    {
      m->deviation += delta_deviation;
      KheMonitorChangeCost((KHE_MONITOR) m,
	KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorDeleteMeet(                          */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet)                  */
/*                                                                           */
/*  Monitor the effect of deleting meet.                                     */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorDeleteMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet)
{
  int delta_deviation;
  if( KheMeetDuration(meet) == m->duration )
  {
    /* update meet_count and find the change in deviation */
    delta_deviation = KheDistributeSplitEventsMonitorDeleteOneMeet(m);

    /* do the update if deviations have changed */
    if( delta_deviation != 0 )
    {
      m->deviation += delta_deviation;
      KheMonitorChangeCost((KHE_MONITOR) m,
	KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorSplitMeet(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet1, KHE_MEET meet2) */
/*                                                                           */
/*  Let m know that a meet has just split into meet1 and meet2.              */
/*  Either both meets are assigned times, or they aren't.                    */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorSplitMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2, delta_deviation;

  /* update meet_count and find the change in deviation */
  durn1 = KheMeetDuration(meet1);
  durn2 = KheMeetDuration(meet2);
  delta_deviation = 0;
  if( durn1 + durn2 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorDeleteOneMeet(m);
  if( durn1 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorAddOneMeet(m);
  if( durn2 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorAddOneMeet(m);

  /* do the update if deviations have changed */
  if( delta_deviation != 0 )
  {
    m->deviation += delta_deviation;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDistributeSplitEventsMonitorMergeMeet(                           */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet1, KHE_MEET meet2) */
/*                                                                           */
/*  Let m know that meet1 and meet2 are just about to be merged.             */
/*                                                                           */
/*  It might seem that there is a problem with this code of meet is assigned */
/*  a time and merge_meet is not.  However, this code is called only by      */
/*  KheMeetMerge, and it is a precondition of that function that             */
/*  meet and merge_meet must either both be unassigned (and hence both not   */
/*  assigned a time) or else assigned to the same meet (and hence            */
/*  both either assigned a time or not).                                     */
/*                                                                           */
/*  KheMeetMerge could allow meet to be assigned and merge_meet to not be,   */
/*  at least when the combined duration is acceptable; but it doesn't.       */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorMergeMeet(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2, delta_deviation;

  /* update meet_count and find the change in deviation */
  durn1 = KheMeetDuration(meet1);
  durn2 = KheMeetDuration(meet2);
  delta_deviation = 0;
  if( durn1 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorDeleteOneMeet(m);
  if( durn2 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorDeleteOneMeet(m);
  if( durn1 + durn2 == m->duration )
    delta_deviation += KheDistributeSplitEventsMonitorAddOneMeet(m);

  /* do the update if deviations have changed */
  if( delta_deviation != 0 )
  {
    m->deviation += delta_deviation;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsMonitorDeviation(                            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheDistributeSplitEventsMonitorDeviation(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDistributeSplitEventsMonitorDeviationDescription(               */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheDistributeSplitEventsMonitorDeviationDescription(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  char *n;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    n = KheEventName(KheDistributeSplitEventsMonitorEvent(m));
    if( m->meet_count < m->minimum )
      return HnStringMake(a, "%d too few in %s", m->minimum - m->meet_count, n);
    else if( m->meet_count > m->maximum )
      return HnStringMake(a, "%d too many in %s", m->meet_count - m->maximum,n);
    else
    {
      HnAbort("KheDistributeSplitEventsMonitorDeviationDescription"
	" internal error");
      return NULL;  /* keep compiler happy */
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsMonitorDeviationCount(                       */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheDistributeSplitEventsMonitorDeviationCount(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDistributeSplitEventsMonitorDeviation(                            */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheDistributeSplitEventsMonitorDeviation(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheDistributeSplitEventsMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheDistributeSplitEventsMonitorDeviationDescription(               */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int i)                          */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheDistributeSplitEventsMonitorDeviationDescription(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int i)
{
  KHE_EVENT e;
  HnAssert(i == 0,
    "KheDistributeSplitEventsMonitorDeviationDescription: i out of range");
  e = KheDistributeSplitEventsMonitorEvent(m);
  return KheEventName(e);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheDistributeSplitEventsMonitorPointOfApplication(                 */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheDistributeSplitEventsMonitorPointOfApplication(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  return KheEventName(KheDistributeSplitEventsMonitorEvent(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDistributeSplitEventsMonitorId(                                 */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)                                 */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheDistributeSplitEventsMonitorId(KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m)
{
  char *constraint_id, *event_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    a = KheSolnArena(m->soln);
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    event_id = KheEventId(KheDistributeSplitEventsMonitorEvent(m));
    m->id = HnStringMake(a, "%s/%s", constraint_id, event_id);
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
/*  void KheDistributeSplitEventsMonitorDebug(                               */
/*    KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int verbosity,                  */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheDistributeSplitEventsMonitorDebug(
  KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR m, int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " ");
    KheEventInSolnDebug(m->event_in_soln, 1, -1, fp);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}


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
/*  FILE:         khe_split_events_monitor.c                                 */
/*  DESCRIPTION:  A split events monitor                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_EVENTS_MONITOR - a split events monitor                        */
/*                                                                           */
/*****************************************************************************/

struct khe_split_events_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_EVENT_IN_SOLN		event_in_soln;		/* enclosing es      */
  KHE_SPLIT_EVENTS_CONSTRAINT	constraint;		/* constraint        */
  int				min_duration;		/* MinimumDuration   */
  int				max_duration;		/* MaximumDuration   */
  int				min_amount;		/* MinimumAmount     */
  int				max_amount;		/* MaximumAmount     */
  int				meet_count;		/* number of meets   */
  KHE_SPLIT_EVENTS_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_EVENTS_MONITOR KheSplitEventsMonitorMake(KHE_EVENT_IN_SOLN es, */
/*    KHE_SPLIT_EVENTS_CONSTRAINT c)                                         */
/*                                                                           */
/*  Make a new split events monitor with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_EVENTS_MONITOR KheSplitEventsMonitorMake(KHE_EVENT_IN_SOLN es,
  KHE_SPLIT_EVENTS_CONSTRAINT c)
{
  KHE_SPLIT_EVENTS_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheEventInSolnSoln(es);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res,
    soln, KHE_SPLIT_EVENTS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->event_in_soln = es;
  res->constraint = c;
  res->min_duration = KheSplitEventsConstraintMinDuration(c);
  res->max_duration = KheSplitEventsConstraintMaxDuration(c);
  res->min_amount = KheSplitEventsConstraintMinAmount(c);
  res->max_amount = KheSplitEventsConstraintMaxAmount(c);
  res->meet_count = 0;
  res->deviation = 0;  /* while unattached */
  res->copy = NULL;
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  if( DEBUG1 )
    fprintf(stderr, "  KheSplitEventsMonitorMake %p (soln %p)\n",
      (void *) res, (void *) res->soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_EVENTS_MONITOR KheSplitEventsMonitorCopyPhase1(                */
/*    KHE_SPLIT_EVENTS_MONITOR m, HA_ARENA a)                                */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_EVENTS_MONITOR KheSplitEventsMonitorCopyPhase1(
  KHE_SPLIT_EVENTS_MONITOR m, HA_ARENA a)
{
  KHE_SPLIT_EVENTS_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->event_in_soln = KheEventInSolnCopyPhase1(m->event_in_soln, a);
    copy->constraint = m->constraint;
    copy->min_duration = m->min_duration;
    copy->max_duration = m->max_duration;
    copy->min_amount = m->min_amount;
    copy->max_amount = m->max_amount;
    copy->meet_count = m->meet_count;
    copy->deviation = m->deviation;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorCopyPhase2(KHE_SPLIT_EVENTS_MONITOR m)         */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorCopyPhase2(KHE_SPLIT_EVENTS_MONITOR m)
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
/*  void KheSplitEventsMonitorDelete(KHE_SPLIT_EVENTS_MONITOR m)             */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitEventsMonitorDelete(KHE_SPLIT_EVENTS_MONITOR m)
{
  if( m->attached )
    KheSplitEventsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheEventInSolnDeleteMonitor(m->event_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_SPLIT_EVENTS_CONSTRAINT KheSplitEventsMonitorConstraint(             */
/*    KHE_SPLIT_EVENTS_MONITOR m)                                            */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_SPLIT_EVENTS_CONSTRAINT KheSplitEventsMonitorConstraint(
  KHE_SPLIT_EVENTS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheSplitEventsMonitorEvent(KHE_SPLIT_EVENTS_MONITOR m)         */
/*                                                                           */
/*  Return the event that m is monitoring.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheSplitEventsMonitorEvent(KHE_SPLIT_EVENTS_MONITOR m)
{
  return KheEventInSolnEvent(m->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorLimits(KHE_SPLIT_EVENTS_MONITOR m,             */
/*    int *min_duration, int *max_duration, int *min_amount, int *max_amount)*/
/*                                                                           */
/*  Return these four attributes of m.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorLimits(KHE_SPLIT_EVENTS_MONITOR m,
  int *min_duration, int *max_duration, int *min_amount, int *max_amount)
{
  *min_duration = m->min_duration;
  *max_duration = m->max_duration;
  *min_amount = m->min_amount;
  *max_amount = m->max_amount;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is m->min_amount, because initially there are     */
/*  no meets.  There is therefore no violation of the duration limits.       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorAttachToSoln(KHE_SPLIT_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorAttachToSoln(KHE_SPLIT_EVENTS_MONITOR m)
{
  m->attached = true;
  m->deviation = m->min_amount;
  if( m->deviation > 0 )
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  KheEventInSolnAttachMonitor(m->event_in_soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorDetachFromSoln(KHE_SPLIT_EVENTS_MONITOR m)     */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorDetachFromSoln(KHE_SPLIT_EVENTS_MONITOR m)
{
  KheEventInSolnDetachMonitor(m->event_in_soln, (KHE_MONITOR) m);
  HnAssert(m->deviation == m->min_amount,
    "KheSplitEventsMonitorAttachToSoln internal error");
  m->deviation = 0;
  if( m->min_amount > 0 )
    KheMonitorChangeCost((KHE_MONITOR) m, 0);
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorAttachCheck(KHE_SPLIT_EVENTS_MONITOR m)        */
/*                                                                           */
/*  Check the attachment of m.  Since event splitting is considered to be    */
/*  structural, the way to do this is to ensure it is attached, and then     */
/*  detach it if its cost is 0.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheSplitEventsMonitorAttachCheck(KHE_SPLIT_EVENTS_MONITOR m)
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
/*  bool KheSplitEventsMonitorTimeRange(KHE_SPLIT_EVENTS_MONITOR m,          */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheSplitEventsMonitorTimeRange(KHE_SPLIT_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT e;
  e = KheSplitEventsMonitorEvent(m);
  return KheEventTimeRange(e, m->soln, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorSetSweepTime(KHE_SPLIT_EVENTS_MONITOR m,       */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorSetSweepTime(KHE_SPLIT_EVENTS_MONITOR m,
  KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSplitEventsMonitorSweepTimeRange(KHE_SPLIT_EVENTS_MONITOR m,     */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KheSplitEventsMonitorSweepTimeRange(KHE_SPLIT_EVENTS_MONITOR m,
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
/*  void KheSplitEventsMonitorAddMeet(KHE_SPLIT_EVENTS_MONITOR m,            */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of adding meet.                                       */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorAddMeet(KHE_SPLIT_EVENTS_MONITOR m, KHE_MEET meet)
{
  int durn;

  /* update the number of meets, and record deviation change there */
  m->meet_count++;
  if( m->meet_count <= m->min_amount )
    m->deviation--;
  else if( m->meet_count > m->max_amount )
    m->deviation++;

  /* update the number of bad meets */
  durn = KheMeetDuration(meet);
  if( durn < m->min_duration || durn > m->max_duration )
    m->deviation++;

  /* update the cost */
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorDeleteMeet(KHE_SPLIT_EVENTS_MONITOR m,         */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of deleting meet.                                     */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorDeleteMeet(KHE_SPLIT_EVENTS_MONITOR m,
  KHE_MEET meet)
{
  int durn;

  /* update the number of meets, and record deviation change there */
  m->meet_count--;
  if( m->meet_count < m->min_amount )
    m->deviation++;
  else if( m->meet_count >= m->max_amount )
    m->deviation--;

  /* update the number of bad meets */
  durn = KheMeetDuration(meet);
  if( durn < m->min_duration || durn > m->max_duration )
    m->deviation--;

  /* update the cost */
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorSplitMeet(KHE_SPLIT_EVENTS_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Let m know that a meet has just split into meet1 and meet2.              */
/*  Either both meets are assigned times, or they aren't.                    */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorSplitMeet(KHE_SPLIT_EVENTS_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2;

  /* update the number of meets, and record deviation change there */
  m->meet_count++;
  if( m->meet_count <= m->min_amount )
    m->deviation--;
  else if( m->meet_count > m->max_amount )
    m->deviation++;

  /* update the number of bad meets */
  durn1 = KheMeetDuration(meet1);
  durn2 = KheMeetDuration(meet2);
  if( durn1 < m->min_duration || durn1 > m->max_duration )
    m->deviation++;
  if( durn2 < m->min_duration || durn2 > m->max_duration )
    m->deviation++;
  if( durn1 + durn2 < m->min_duration || durn1 + durn2 > m->max_duration )
    m->deviation--;

  /* update the cost */
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSplitEventsMonitorMergeMeet(KHE_SPLIT_EVENTS_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Let m know that meet1 and meet2 are just about to be merged.             */
/*                                                                           */
/*  It might seem that there is a problem with this code of meet is          */
/*  assigned a time and merge_meet is not.  However, this code is called     */
/*  only by KheMeetMerge, and it is a precondition of that function that     */
/*  meet and merge_meet must either both be unassigned (and hence both not   */
/*  assigned a time) or else assigned to the same meet (and hence            */
/*  both either assigned a time or not).                                     */
/*                                                                           */
/*  KheMeetMerge could allow meet to be assigned and merge_meet to not be,   */
/*  at least when the combined duration is acceptable; but it doesn't.       */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorMergeMeet(KHE_SPLIT_EVENTS_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2;

  /* update the number of meets, and record deviation change there */
  m->meet_count--;
  if( m->meet_count < m->min_amount )
    m->deviation++;
  else if( m->meet_count >= m->max_amount )
    m->deviation--;

  /* update the number of bad meets */
  durn1 = KheMeetDuration(meet1);
  durn2 = KheMeetDuration(meet2);
  if( durn1 < m->min_duration || durn1 > m->max_duration )
    m->deviation--;
  if( durn2 < m->min_duration || durn2 > m->max_duration )
    m->deviation--;
  if( durn1 + durn2 < m->min_duration || durn1 + durn2 > m->max_duration )
    m->deviation++;

  /* do the update if deviations have changed */
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
/*  int KheSplitEventsMonitorDeviation(KHE_SPLIT_EVENTS_MONITOR m)           */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheSplitEventsMonitorDeviation(KHE_SPLIT_EVENTS_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSplitEventsMonitorDeviationDescription(                         */
/*    KHE_SPLIT_EVENTS_MONITOR m)                                            */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheSplitEventsMonitorDeviationDescription(KHE_SPLIT_EVENTS_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int too_many_count, too_few_count, bad_durn_count;
  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    /* analyse into parts */
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    if( m->meet_count < m->min_amount )
    {
       too_few_count = m->min_amount - m->meet_count;
       too_many_count = 0;
       bad_durn_count = m->deviation - too_few_count;
    }
    else if( m->meet_count > m->max_amount )
    {
       too_few_count = 0;
       too_many_count = m->meet_count - m->max_amount;
       bad_durn_count = m->deviation - too_many_count;
    }
    else
    {
       too_few_count = 0;
       too_many_count = 0;
       bad_durn_count = m->deviation;
    }

    if( too_few_count + too_many_count == 0 )
    {
      /* bad duration item only */
      HnStringAdd(&ac, "%d sub-events of unwanted duration",
	bad_durn_count);
    }
    else if( bad_durn_count == 0 )
    {
      /* too few or too many item only */
      if( too_few_count > 0 )
	HnStringAdd(&ac, "%d too few sub-events", too_few_count);
      else
	HnStringAdd(&ac, "%d too many sub-events", too_many_count);
    }
    else
    {
      /* both items */
      HnStringAdd(&ac, "%d: ", m->deviation);
      if( too_few_count > 0 )
	HnStringAdd(&ac, "%d too few sub-events", too_few_count);
      else
	HnStringAdd(&ac, "%d too many sub-events", too_many_count);
      HnStringAdd(&ac, "; %d sub-events of unwanted duration",
	bad_durn_count);
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsMonitorDeviationCount(KHE_SPLIT_EVENTS_MONITOR m)      */
/*                                                                           */
/*  Return the number of deviations.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheSplitEventsMonitorDeviationCount(KHE_SPLIT_EVENTS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheSplitEventsMonitorDeviation(KHE_SPLIT_EVENTS_MONITOR m, int i)    */
/*                                                                           */
/*  Return the i'th deviation.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheSplitEventsMonitorDeviation(KHE_SPLIT_EVENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheSplitEventsMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheSplitEventsMonitorDeviationDescription(                         */
/*    KHE_SPLIT_EVENTS_MONITOR m, int i)                                     */
/*                                                                           */
/*  Return a description of the i'th deviation.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheSplitEventsMonitorDeviationDescription(
  KHE_SPLIT_EVENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheSplitEventsMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheSplitEventsMonitorPointOfApplication(KHE_SPLIT_EVENTS_MONITOR m)*/
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheSplitEventsMonitorPointOfApplication(KHE_SPLIT_EVENTS_MONITOR m)
{
  return KheEventName(KheSplitEventsMonitorEvent(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheSplitEventsMonitorId(KHE_SPLIT_EVENTS_MONITOR m)                */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheSplitEventsMonitorId(KHE_SPLIT_EVENTS_MONITOR m)
{
  char *constraint_id, *event_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    a = KheSolnArena(m->soln);
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    event_id = KheEventId(KheSplitEventsMonitorEvent(m));
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
/*  void KheSplitEventsMonitorDebug(KHE_SPLIT_EVENTS_MONITOR m,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheSplitEventsMonitorDebug(KHE_SPLIT_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " ");
    KheEventInSolnDebug(m->event_in_soln, 1, -1, fp);
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

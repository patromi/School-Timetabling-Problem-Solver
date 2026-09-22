
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
/*  FILE:         khe_order_events_monitor.c                                 */
/*  DESCRIPTION:  An order events monitor                                    */
/*                                                                           */
/*****************************************************************************/
#include <stdarg.h>
#include "khe_interns.h"
#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDER_EVENTS_MONITOR - an order events monitor                       */
/*                                                                           */
/*****************************************************************************/

struct khe_order_events_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_ORDER_EVENTS_CONSTRAINT	constraint;		/* constraint        */
  KHE_EVENT			first_event;		/* first event       */
  KHE_EVENT			second_event;		/* second event      */
  int				min_separation;		/* min separation    */
  int				max_separation;		/* max separation    */
  int				unassigned_meets;	/* unassigned meets  */
  KHE_ORDER_EVENTS_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDER_EVENTS_MONITOR KheOrderEventsMonitorMake(KHE_SOLN soln,        */
/*    KHE_ORDER_EVENTS_CONSTRAINT c, KHE_EVENT first_event,                  */
/*    KHE_EVENT second_event, int min_separation, int max_separation)        */
/*                                                                           */
/*  Make a new order events monitor with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_ORDER_EVENTS_MONITOR KheOrderEventsMonitorMake(KHE_SOLN soln,
  KHE_ORDER_EVENTS_CONSTRAINT c, KHE_EVENT first_event,
  KHE_EVENT second_event, int min_separation, int max_separation)
{
  KHE_ORDER_EVENTS_MONITOR res;  KHE_EVENT_IN_SOLN es;  HA_ARENA a;

  /* make the monitor */
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_ORDER_EVENTS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->constraint = c;
  res->first_event = first_event;
  res->second_event = second_event;
  res->min_separation = min_separation;
  res->max_separation = max_separation;
  res->unassigned_meets = 0;
  res->deviation = 0;
  res->copy = NULL;

  /* add (but don't attach) to the monitored events in soln objects */
  es = KheSolnEventInSoln(soln, res->first_event);
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  es = KheSolnEventInSoln(soln, res->second_event);
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDER_EVENTS_CONSTRAINT KheOrderEventsMonitorConstraint(             */
/*    KHE_ORDER_EVENTS_MONITOR m)                                            */
/*                                                                           */
/*  Return the constraint that m was derived from.                           */
/*                                                                           */
/*****************************************************************************/

KHE_ORDER_EVENTS_CONSTRAINT KheOrderEventsMonitorConstraint(
  KHE_ORDER_EVENTS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheOrderEventsMonitorFirstEvent(KHE_ORDER_EVENTS_MONITOR m)    */
/*                                                                           */
/*  Return the first event attribute of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheOrderEventsMonitorFirstEvent(KHE_ORDER_EVENTS_MONITOR m)
{
  return m->first_event;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheOrderEventsMonitorSecondEvent(KHE_ORDER_EVENTS_MONITOR m)   */
/*                                                                           */
/*  Return the second event attribute of m.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheOrderEventsMonitorSecondEvent(KHE_ORDER_EVENTS_MONITOR m)
{
  return m->second_event;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorMinSeparation(KHE_ORDER_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Return the min_separation attribute of m.                                */
/*                                                                           */
/*****************************************************************************/

int KheOrderEventsMonitorMinSeparation(KHE_ORDER_EVENTS_MONITOR m)
{
  return m->min_separation;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorMaxSeparation(KHE_ORDER_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Return the max_separation attribute of m.                                */
/*                                                                           */
/*****************************************************************************/

int KheOrderEventsMonitorMaxSeparation(KHE_ORDER_EVENTS_MONITOR m)
{
  return m->max_separation;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ORDER_EVENTS_MONITOR KheOrderEventsMonitorCopyPhase1(                */
/*    KHE_ORDER_EVENTS_MONITOR m, HA_ARENA a)                                */
/*                                                                           */
/*  Carry out Phase 1 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_ORDER_EVENTS_MONITOR KheOrderEventsMonitorCopyPhase1(
  KHE_ORDER_EVENTS_MONITOR m, HA_ARENA a)
{
  KHE_ORDER_EVENTS_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->constraint = m->constraint;
    copy->first_event = m->first_event;
    copy->second_event = m->second_event;
    copy->min_separation = m->min_separation;
    copy->max_separation = m->max_separation;
    copy->unassigned_meets = m->unassigned_meets;
    copy->deviation = m->deviation;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorCopyPhase2(KHE_ORDER_EVENTS_MONITOR m)         */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorCopyPhase2(KHE_ORDER_EVENTS_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorDelete(KHE_ORDER_EVENTS_MONITOR m)             */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOrderEventsMonitorDelete(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT_IN_SOLN es;
  if( m->attached )
    KheOrderEventsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  es = KheSolnEventInSoln(m->soln, m->first_event);
  KheEventInSolnDeleteMonitor(es, (KHE_MONITOR) m);
  es = KheSolnEventInSoln(m->soln, m->second_event);
  KheEventInSolnDeleteMonitor(es, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0.                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorDev(KHE_ORDER_EVENTS_MONITOR m)                 */
/*                                                                           */
/*  Calculate the deviation.  This is not particularly efficient in          */
/*  general, so it's best to call it only when something has changed.        */
/*                                                                           */
/*****************************************************************************/

static int KheOrderEventsMonitorDev(KHE_ORDER_EVENTS_MONITOR m)
{
  int gap;  KHE_EVENT_IN_SOLN es1, es2;
  if( m->unassigned_meets > 0 )
    return 0;
  es1 = KheSolnEventInSoln(m->soln, m->first_event);
  es2 = KheSolnEventInSoln(m->soln, m->second_event);
  if( KheEventInSolnMeetCount(es1) == 0 || KheEventInSolnMeetCount(es2) == 0 )
    return 0;
  gap = KheEventInSolnMinTimeIndex(es2) -
    KheEventInSolnMaxTimeIndexPlusDuration(es1);
  if( gap < m->min_separation )
    return m->min_separation - gap;
  else if( gap > m->max_separation )
    return gap - m->max_separation;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorAttachToSoln(KHE_ORDER_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorAttachToSoln(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT_IN_SOLN es;
  m->attached = true;
  es = KheSolnEventInSoln(m->soln, m->first_event);
  KheEventInSolnAttachMonitor(es, (KHE_MONITOR) m);
  es = KheSolnEventInSoln(m->soln, m->second_event);
  KheEventInSolnAttachMonitor(es, (KHE_MONITOR) m);
  m->deviation = KheOrderEventsMonitorDev(m);
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorDetachFromSoln(KHE_ORDER_EVENTS_MONITOR m)     */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorDetachFromSoln(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT_IN_SOLN es;
  es = KheSolnEventInSoln(m->soln, m->first_event);
  KheEventInSolnDetachMonitor(es, (KHE_MONITOR) m);
  es = KheSolnEventInSoln(m->soln, m->second_event);
  KheEventInSolnDetachMonitor(es, (KHE_MONITOR) m);
  KheMonitorChangeCost((KHE_MONITOR) m, 0);
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsMonitorMustHaveZeroCost(KHE_ORDER_EVENTS_MONITOR m)   */
/*                                                                           */
/*  Return true if m must have zero cost, because its events have one meet   */
/*  each, with the same leader meet, and their separation is in range.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheOrderEventsMonitorMustHaveZeroCost(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT_IN_SOLN es1, es2;  KHE_MEET meet1, meet2, leader1, leader2;
  int offset1, offset2, sep;
  es1 = KheSolnEventInSoln(m->soln, m->first_event);
  es2 = KheSolnEventInSoln(m->soln, m->second_event);
  if( KheEventInSolnMeetCount(es1) != 1 || KheEventInSolnMeetCount(es2) != 1 )
    return false;
  meet1 = KheEventInSolnMeet(es1, 0);
  meet2 = KheEventInSolnMeet(es2, 0);
  leader1 = KheMeetFirstUnFixed(meet1, &offset1);
  leader2 = KheMeetFirstUnFixed(meet2, &offset2);
  if( leader1 == NULL || leader1 != leader2 )
    return false;
  sep = offset2 - (offset1 + KheMeetDuration(meet1));
  return sep >= m->min_separation && sep <= m->max_separation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorAttachCheck(KHE_ORDER_EVENTS_MONITOR m)        */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheOrderEventsMonitorAttachCheck(KHE_ORDER_EVENTS_MONITOR m)
{
  if( KheOrderEventsMonitorMustHaveZeroCost(m) )
  {
    if( KheMonitorAttachedToSoln((KHE_MONITOR) m) )
      KheMonitorDetachFromSoln((KHE_MONITOR) m);
  }
  else
  {
    if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
      KheMonitorAttachToSoln((KHE_MONITOR) m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time ranges and sweep times"                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsMonitorTimeRange(KHE_ORDER_EVENTS_MONITOR m,          */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheOrderEventsMonitorTimeRange(KHE_ORDER_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT e;  KHE_TIME ft, lt;
  *first_time = *last_time = NULL;
  e = KheOrderEventsMonitorFirstEvent(m);
  if( KheEventTimeRange(e, m->soln, &ft, &lt) )
  {
    if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
      *first_time = ft;
    if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
      *last_time = ft;
  }
  e = KheOrderEventsMonitorSecondEvent(m);
  if( KheEventTimeRange(e, m->soln, &ft, &lt) )
  {
    if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
      *first_time = ft;
    if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
      *last_time = ft;
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorSetSweepTime(KHE_ORDER_EVENTS_MONITOR m,       */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorSetSweepTime(KHE_ORDER_EVENTS_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsMonitorSweepTimeRange(KHE_ORDER_TIMES_MONITOR m,      */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KheOrderEventsMonitorSweepTimeRange(KHE_ORDER_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  return *first_time = *last_time = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "fix and unfix"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheOrderEventsMonitorHasFixedZeroCost(KHE_ORDER_EVENTS_MONITOR m)   */
/*                                                                           */
/*  Return true if m must have zero cost, because its events have one meet   */
/*  each, with the same leader meet, and their separation is in range.       */
/*                                                                           */
/*****************************************************************************/

static bool KheOrderEventsMonitorHasFixedZeroCost(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT_IN_SOLN es1, es2;  KHE_MEET meet1, meet2, leader1, leader2;
  int offset1, offset2, sep;
  es1 = KheSolnEventInSoln(m->soln, m->first_event);
  es2 = KheSolnEventInSoln(m->soln, m->second_event);
  if( KheEventInSolnMeetCount(es1) != 1 || KheEventInSolnMeetCount(es2) != 1 )
    return false;
  meet1 = KheEventInSolnMeet(es1, 0);
  meet2 = KheEventInSolnMeet(es2, 0);
  leader1 = KheMeetLastFixed(meet1, &offset1);
  leader2 = KheMeetLastFixed(meet2, &offset2);
  if( leader1 != leader2 )
    return false;
  sep = offset2 - (offset1 + KheMeetDuration(meet1));
  return sep >= m->min_separation && sep <= m->max_separation;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorMeetAssignFix(KHE_ORDER_EVENTS_MONITOR m)      */
/*                                                                           */
/*  A meet monitored by m has just had its assignment fixed, or at least     */
/*  its fixed assignment path is longer than it was.  Check whether m        */
/*  needs to be attached.                                                    */
/*                                                                           */
/*  This function assumes that m is attached and has cost 0.                 */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorMeetAssignFix(KHE_ORDER_EVENTS_MONITOR m)
{
  if( KheOrderEventsMonitorHasFixedZeroCost(m) )
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorMeetAssignUnFix(KHE_ORDER_EVENTS_MONITOR m)    */
/*                                                                           */
/*  A meet monitored by m has just had its assignment unfixed, or at least   */
/*  its fixed assignment path is shorter than it was.  Check whether m       */
/*  needs to be attached.                                                    */
/*                                                                           */
/*  This function assumes that m is detached.                                */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorMeetAssignUnFix(KHE_ORDER_EVENTS_MONITOR m)
{
  if( !KheOrderEventsMonitorHasFixedZeroCost(m) )
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorAddMeet(KHE_ORDER_EVENTS_MONITOR m,            */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Inform m that meet is being added.                                       */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorAddMeet(KHE_ORDER_EVENTS_MONITOR m, KHE_MEET meet)
{
  int new_dev;
  if( KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX )
    m->unassigned_meets++;
  new_dev = KheOrderEventsMonitorDev(m);
  if( new_dev != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, new_dev));
    m->deviation = new_dev;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorDeleteMeet(KHE_ORDER_EVENTS_MONITOR m,         */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Inform m that meet is being deleted.                                     */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorDeleteMeet(KHE_ORDER_EVENTS_MONITOR m, KHE_MEET meet)
{
  int new_dev;
  if( KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX )
    m->unassigned_meets--;
  new_dev = KheOrderEventsMonitorDev(m);
  if( new_dev != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, new_dev));
    m->deviation = new_dev;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorSplitMeet(KHE_ORDER_EVENTS_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Inform m that a split into meet1 and meet2 is occurring.                 */
/*                                                                           */
/*  Implementation note.  Whether a meet is assigned or not, splitting       */
/*  does not change the value of this monitors, so this code does            */
/*  not evaluate deviations or change cost.                                  */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorSplitMeet(KHE_ORDER_EVENTS_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  if( KheMeetAssignedTimeIndex(meet1) == NO_TIME_INDEX )
    m->unassigned_meets++;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorMergeMeet(KHE_ORDER_EVENTS_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Inform m that a merge of meet1 and meet2 is occurring.                   */
/*                                                                           */
/*  Implementation note.  Whether a meet is assigned or not, merging         */
/*  does not change the value of this monitor, so this code does             */
/*  not evaluate deviations or change cost.                                  */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorMergeMeet(KHE_ORDER_EVENTS_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  if( KheMeetAssignedTimeIndex(meet1) == NO_TIME_INDEX )
    m->unassigned_meets--;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorAssignTime(KHE_ORDER_EVENTS_MONITOR m,         */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Inform m that a time assignment of meet is occurring.                    */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorAssignTime(KHE_ORDER_EVENTS_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  int new_dev;
  m->unassigned_meets--;
  new_dev = KheOrderEventsMonitorDev(m);
  if( new_dev != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, new_dev));
    m->deviation = new_dev;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheOrderEventsMonitorUnAssignTime(KHE_ORDER_EVENTS_MONITOR m,       */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Inform m that a time unassignment of meet is occurring.                  */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorUnAssignTime(KHE_ORDER_EVENTS_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  m->unassigned_meets++;
  if( 0 != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m, 0);
    m->deviation = 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorDeviation(KHE_ORDER_EVENTS_MONITOR m)           */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

int KheOrderEventsMonitorDeviation(KHE_ORDER_EVENTS_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrderEventsMonitorDeviationDescription(                         */
/*    KHE_ORDER_EVENTS_MONITOR m)                                            */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

char *KheOrderEventsMonitorDeviationDescription(KHE_ORDER_EVENTS_MONITOR m)
{
  int gap;  KHE_EVENT_IN_SOLN es1, es2;  HA_ARENA a;
  if( m->deviation == 0 || m->unassigned_meets > 0 )
    return "0";
  else
  {
    es1 = KheSolnEventInSoln(m->soln, m->first_event);
    es2 = KheSolnEventInSoln(m->soln, m->second_event);
    if( KheEventInSolnMeetCount(es1)==0 || KheEventInSolnMeetCount(es2)==0 )
      return "0";
    else
    {
      a = KheSolnArena(m->soln);
      gap = KheEventInSolnMinTimeIndex(es2) -
	KheEventInSolnMaxTimeIndexPlusDuration(es1);
      if( gap < m->min_separation )
	return HnStringMake(a, "%d too small", m->min_separation - gap);
      else if( gap > m->max_separation )
	return HnStringMake(a, "%d too large", gap - m->max_separation);
      else
	return "0";
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorDeviationCount(KHE_ORDER_EVENTS_MONITOR m)      */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheOrderEventsMonitorDeviationCount(KHE_ORDER_EVENTS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheOrderEventsMonitorDeviation(KHE_ORDER_EVENTS_MONITOR m, int i)    */
/*                                                                           */
/*  Return the i'th deviation.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheOrderEventsMonitorDeviation(KHE_ORDER_EVENTS_MONITOR m, int i)
{
  MAssert(i == 0, "KheOrderEventsMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrderEventsMonitorDeviationDescription(                         */
/*    KHE_ORDER_EVENTS_MONITOR m, int i)                                     */
/*                                                                           */
/*  Return a description of the i'th deviation.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheOrderEventsMonitorDeviationDescription(
  KHE_ORDER_EVENTS_MONITOR m, int i)
{
  MAssert(i == 0, "KheOrderEventsMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrderEventsMonitorPointOfApplication(KHE_ORDER_EVENTS_MONITOR m)*/
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheOrderEventsMonitorPointOfApplication(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT e1, e2;  HA_ARENA a;
  a = KheSolnArena(m->soln);
  e1 = KheOrderEventsMonitorFirstEvent(m);
  e2 = KheOrderEventsMonitorSecondEvent(m);
  return HnStringMake(a, "%s : %s", KheEventName(e1), KheEventName(e2));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheOrderEventsMonitorId(KHE_ORDER_EVENTS_MONITOR m)                */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheOrderEventsMonitorId(KHE_ORDER_EVENTS_MONITOR m)
{
  KHE_EVENT e1, e2;  char *constraint_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    e1 = KheOrderEventsMonitorFirstEvent(m);
    e2 = KheOrderEventsMonitorSecondEvent(m);
    a = KheSolnArena(m->soln);
    m->id = HnStringMake(a, "%s/%s/%s", constraint_id, KheEventId(e1),
      KheEventId(e2));
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
/*  void KheOrderEventsMonitorDebug(KHE_ORDER_EVENTS_MONITOR m,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheOrderEventsMonitorDebug(KHE_ORDER_EVENTS_MONITOR m,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) m, indent, fp);
    fprintf(fp, " %s",
      KheConstraintId((KHE_CONSTRAINT) m->constraint) == NULL ? "-" :
      KheConstraintId((KHE_CONSTRAINT) m->constraint));
    KheMonitorDebugEnd((KHE_MONITOR) m, true, indent, fp);
  }
}

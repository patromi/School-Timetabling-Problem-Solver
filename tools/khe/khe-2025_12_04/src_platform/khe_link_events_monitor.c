
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
/*  FILE:         khe_link_events_monitor.c                                  */
/*  DESCRIPTION:  An link events monitor                                     */
/*                                                                           */
/*****************************************************************************/
#include <stdarg.h>
#include "khe_interns.h"

#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_LINK_EVENTS_MONITOR - a link events monitor                          */
/*                                                                           */
/*****************************************************************************/

struct khe_link_events_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_LINK_EVENTS_CONSTRAINT	constraint;		/* constraint        */
  KHE_EVENT_GROUP		event_group;		/* event group       */
  int				wanted_multiplicity;	/* want this many    */
  HA_ARRAY_INT			multiplicities;		/* indexed by time   */
  int				new_deviation;		/* new deviation     */
  ARRAY_KHE_MEET		tmp_leader_meets;	/* temporaries       */
  HA_ARRAY_INT			tmp_offsets;
  HA_ARRAY_INT			tmp_durations;
  HA_ARRAY_INT			tmp_multiplicities;
  KHE_LINK_EVENTS_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_LINK_EVENTS_MONITOR KheLinkEventsMonitorMake(KHE_SOLN soln,          */
/*    KHE_LINK_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg)                      */
/*                                                                           */
/*  Make a new link events monitor with these attributes.                    */
/*                                                                           */
/*****************************************************************************/

KHE_LINK_EVENTS_MONITOR KheLinkEventsMonitorMake(KHE_SOLN soln,
  KHE_LINK_EVENTS_CONSTRAINT c, KHE_EVENT_GROUP eg)
{
  KHE_LINK_EVENTS_MONITOR res;  KHE_INSTANCE ins;
  KHE_EVENT e;  KHE_EVENT_IN_SOLN es;  int i;  HA_ARENA a;

  /* make the monitor */
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_LINK_EVENTS_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->constraint = c;
  res->event_group = eg;
  res->wanted_multiplicity = KheEventGroupEventCount(eg);
  ins = KheSolnInstance(soln);
  HaArrayInit(res->multiplicities, a);
  HaArrayFill(res->multiplicities, KheInstanceTimeCount(ins), 0);
  res->deviation = 0;
  res->new_deviation = 0;
  HaArrayInit(res->tmp_leader_meets, a);
  HaArrayInit(res->tmp_offsets, a);
  HaArrayInit(res->tmp_durations, a);
  HaArrayInit(res->tmp_multiplicities, a);
  res->copy = NULL;

  /* add (but don't attach) to the monitored event in soln objects */
  for( i = 0;  i < KheEventGroupEventCount(res->event_group);  i++ )
  {
    e = KheEventGroupEvent(res->event_group, i);
    es = KheSolnEventInSoln(soln, e);
    KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LINK_EVENTS_CONSTRAINT KheLinkEventsMonitorConstraint(               */
/*    KHE_LINK_EVENTS_MONITOR m)                                             */
/*                                                                           */
/*  Return the constraint that m was derived from.                           */
/*                                                                           */
/*****************************************************************************/

KHE_LINK_EVENTS_CONSTRAINT KheLinkEventsMonitorConstraint(
  KHE_LINK_EVENTS_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsMonitorEventGroupIndex(KHE_LINK_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Return the event group within m's constraint that m monitors.            */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_GROUP KheLinkEventsMonitorEventGroup(KHE_LINK_EVENTS_MONITOR m)
{
  return m->event_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_LINK_EVENTS_MONITOR KheLinkEventsMonitorCopyPhase1(                  */
/*    KHE_LINK_EVENTS_MONITOR m, HA_ARENA a)                                 */
/*                                                                           */
/*  Carry out Phase 1 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_LINK_EVENTS_MONITOR KheLinkEventsMonitorCopyPhase1(
  KHE_LINK_EVENTS_MONITOR m, HA_ARENA a)
{
  KHE_LINK_EVENTS_MONITOR copy;  int i;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->constraint = m->constraint;
    copy->event_group = m->event_group;
    copy->wanted_multiplicity = m->wanted_multiplicity;
    HaArrayInit(copy->multiplicities, a);
    HaArrayAppend(copy->multiplicities, m->multiplicities, i);
    copy->deviation = m->deviation;
    copy->new_deviation = m->new_deviation;
    HaArrayInit(copy->tmp_leader_meets, a);
    HaArrayInit(copy->tmp_offsets, a);
    HaArrayInit(copy->tmp_durations, a);
    HaArrayInit(copy->tmp_multiplicities, a);
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorCopyPhase2(KHE_LINK_EVENTS_MONITOR m)           */
/*                                                                           */
/*  Carry out Phase 2 of the copying of m.                                   */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorCopyPhase2(KHE_LINK_EVENTS_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorDelete(KHE_LINK_EVENTS_MONITOR m)               */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLinkEventsMonitorDelete(KHE_LINK_EVENTS_MONITOR m)
{
  int i;  KHE_EVENT e;  KHE_EVENT_IN_SOLN es;
  if( m->attached )
    KheLinkEventsMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  for( i = 0;  i < KheEventGroupEventCount(m->event_group);  i++ )
  {
    e = KheEventGroupEvent(m->event_group, i);
    es = KheSolnEventInSoln(m->soln, e);
    KheEventInSolnDeleteMonitor(es, (KHE_MONITOR) m);
  }
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MArrayFree(m->multiplicities);
  MArrayInit(m->tmp_leader_meets);
  MArrayInit(m->tmp_offsets);
  MArrayInit(m->tmp_durations);
  MArrayInit(m->tmp_multiplicities);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is evidently zero, there are no problems when     */
/*  you are monitoring no events.                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorAttachToSoln(KHE_LINK_EVENTS_MONITOR m)         */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorAttachToSoln(KHE_LINK_EVENTS_MONITOR m)
{
  int i;  KHE_EVENT e;  KHE_EVENT_IN_SOLN es;
  m->attached = true;
  for( i = 0;  i < KheEventGroupEventCount(m->event_group);  i++ )
  {
    e = KheEventGroupEvent(m->event_group, i);
    es = KheSolnEventInSoln(m->soln, e);
    KheEventInSolnAttachMonitor(es, (KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorDetachFromSoln(KHE_LINK_EVENTS_MONITOR m)       */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorDetachFromSoln(KHE_LINK_EVENTS_MONITOR m)
{
  int i;  KHE_EVENT e;  KHE_EVENT_IN_SOLN es;
  for( i = 0;  i < KheEventGroupEventCount(m->event_group);  i++ )
  {
    e = KheEventGroupEvent(m->event_group, i);
    es = KheSolnEventInSoln(m->soln, e);
    KheEventInSolnDetachMonitor(es, (KHE_MONITOR) m);
  }
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLinkEventsMonitorMustHaveZeroCost(KHE_LINK_EVENTS_MONITOR m)     */
/*                                                                           */
/*  Return true if m must have zero cost, because its events' meets have     */
/*  equal durations, leader meets, and offsets into leader meets.            */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheLinkEventsMonitorMustHaveZeroCost(KHE_LINK_EVENTS_MONITOR m)
{
  ARRAY_KHE_MEET leader_meets;  KHE_MEET leader_meet, meet;  KHE_EVENT e;
  HA_ARRAY_INT offsets, durations, multiplicities;  int i, j, k, offset;

  ** if there are no events, then there is no cost **
  if( KheEventGroupEventCount(m->event_group) == 0 )
    return true;

  ** find the leader meets, durations, and offsets of the first event **
  MArrayInit(durations);
  MArrayInit(leader_meets);
  MArrayInit(offsets);
  MArrayInit(multiplicities);
  e = KheEventGroupEvent(m->event_group, 0);
  for( j = 0;  j < KheEventMeetCount(m->soln, e);  j++ )
  {
    meet = KheEventMeet(m->soln, e, j);
    leader_meet = KheMeetLeader(meet, &offset);
    if( leader_meet == NULL )
    {
      ** fail if meet has no leader meet **
      MArrayFree(durations);
      MArrayFree(leader_meets);
      MArrayFree(offsets);
      MArrayFree(multiplicities);
      return false;
    }
    HaArrayAddLast(durations, KheMeetDuration(meet));
    HaArrayAddLast(leader_meets, leader_meet);
    HaArrayAddLast(offsets, offset);
    HaArrayAddLast(multiplicities, 1);
  }

  ** check consistency of the leader meets and offsets of the other events **
  for( i = 1;  i < KheEventGroupEventCount(m->event_group);  i++ )
  {
    e = KheEventGroupEvent(m->event_group, i);

    ** fail if e has a different number of meets from the the first e **
    if( KheEventMeetCount(m->soln, e) != HaArrayCount(leader_meets) )
    {
      MArrayFree(durations);
      MArrayFree(leader_meets);
      MArrayFree(offsets);
      MArrayFree(multiplicities);
      return false;
    }

    ** fail if any of e's meets don't match up, including having no leader **
    for( j = 0;  j < KheEventMeetCount(m->soln, e);  j++ )
    {
      meet = KheEventMeet(m->soln, e, j);

      ** search for a leader meet that meet matches with, not already nabbed **
      leader_meet = KheMeetRoot(meet, &offset);
      for( k = 0;  k < HaArrayCount(leader_meets);  k++ )
	if( KheMeetDuration(meet) == HaArray(durations, k) &&
	    leader_meet == HaArray(leader_meets, k) &&
	    offset == HaArray(offsets, k) &&
	    i == HaArray(multiplicities, k) )
	  break;

      ** if no luck, clean up and exit with failure **
      if( k >= HaArrayCount(leader_meets) )
      {
	MArrayFree(durations);
	MArrayFree(leader_meets);
	MArrayFree(offsets);
	MArrayFree(multiplicities);
	return false;
      }

      ** success, but nab index k so it isn't matched twice by e's meets **
      MArrayInc(multiplicities, k);
    }
  }

  ** all correct **
  MArrayFree(durations);
  MArrayFree(leader_meets);
  MArrayFree(offsets);
  MArrayFree(multiplicities);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorAttachCheck(KHE_LINK_EVENTS_MONITOR m)          */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheLinkEventsMonitorAttachCheck(KHE_LINK_EVENTS_MONITOR m)
{
  if( KheLinkEventsMonitorMustHaveZeroCost(m) )
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
/*  bool KheLinkEventsMonitorTimeRange(KHE_LINK_EVENTS_MONITOR m,            */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheLinkEventsMonitorTimeRange(KHE_LINK_EVENTS_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT_GROUP eg;  KHE_EVENT e;  KHE_TIME ft, lt;  int i;
  eg = KheLinkEventsMonitorEventGroup(m);
  *first_time = *last_time = NULL;
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    if( KheEventTimeRange(e, m->soln, &ft, &lt) )
    {
      if( *first_time == NULL || KheTimeIndex(ft) < KheTimeIndex(*first_time) )
	*first_time = ft;
      if( *last_time == NULL || KheTimeIndex(lt) > KheTimeIndex(*last_time) )
	*last_time = ft;
    }
  }
  return *first_time != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorSetSweepTime(KHE_LINK_EVENTS_MONITOR m,         */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorSetSweepTime(KHE_LINK_EVENTS_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLinkEventsMonitorSweepTimeRange(KHE_LINK_EVENTS_MONITOR m,       */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KheLinkEventsMonitorSweepTimeRange(KHE_LINK_EVENTS_MONITOR m,
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
/*  bool KheLinkEventsMonitorHasFixedZeroCost(KHE_LINK_EVENTS_MONITOR m)     */
/*                                                                           */
/*  Return true if m has fixed zero cost, because its events' meets have     */
/*  equal durations, leader meets, and offsets into leader meets.            */
/*                                                                           */
/*****************************************************************************/

static bool KheLinkEventsMonitorHasFixedZeroCost(KHE_LINK_EVENTS_MONITOR m)
{
  KHE_MEET leader_meet, meet;  KHE_EVENT e;  int i, j, k, offset;

  /* if there are no events, then there is no cost */
  if( KheEventGroupEventCount(m->event_group) == 0 )
    return true;

  /* find the leader meets, durations, and offsets of the first event */
  HaArrayClear(m->tmp_durations);
  HaArrayClear(m->tmp_leader_meets);
  HaArrayClear(m->tmp_offsets);
  HaArrayClear(m->tmp_multiplicities);
  e = KheEventGroupEvent(m->event_group, 0);
  for( j = 0;  j < KheEventMeetCount(m->soln, e);  j++ )
  {
    meet = KheEventMeet(m->soln, e, j);
    leader_meet = KheMeetLastFixed(meet, &offset);
    /* ***
    leader_meet = KheMeetFirstUnFixed(meet, &offset);
    if( leader_meet == NULL )
      return false;
    *** */
    HaArrayAddLast(m->tmp_durations, KheMeetDuration(meet));
    HaArrayAddLast(m->tmp_leader_meets, leader_meet);
    HaArrayAddLast(m->tmp_offsets, offset);
    HaArrayAddLast(m->tmp_multiplicities, 1);
  }

  /* check consistency of the leader meets and offsets of the other events */
  for( i = 1;  i < KheEventGroupEventCount(m->event_group);  i++ )
  {
    /* fail if e has a different number of meets from the the first e */
    e = KheEventGroupEvent(m->event_group, i);
    if( KheEventMeetCount(m->soln, e) != HaArrayCount(m->tmp_leader_meets) )
      return false;

    /* fail if any of e's meets don't match up, including having no leader */
    for( j = 0;  j < KheEventMeetCount(m->soln, e);  j++ )
    {
      meet = KheEventMeet(m->soln, e, j);

      /* search for a leader meet that meet matches with, not already nabbed */
      leader_meet = KheMeetLastFixed(meet, &offset);
      /* ***
      leader_meet = KheMeetFirstUnFixed(meet, &offset);
      if( leader_meet == NULL )
	return false;
      *** */
      for( k = 0;  k < HaArrayCount(m->tmp_leader_meets);  k++ )
	if( KheMeetDuration(meet) == HaArray(m->tmp_durations, k) &&
	    leader_meet == HaArray(m->tmp_leader_meets, k) &&
	    offset == HaArray(m->tmp_offsets, k) &&
	    i == HaArray(m->tmp_multiplicities, k) )
	  break;

      /* if no luck, exit with failure */
      if( k >= HaArrayCount(m->tmp_leader_meets) )
	return false;

      /* success, but nab index k so it isn't matched twice by e's meets */
      --HaArray(m->tmp_multiplicities, k);
    }
  }

  /* all correct */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorMeetAssignFix(KHE_LINK_EVENTS_MONITOR m)        */
/*                                                                           */
/*  A meet monitored by m has just had its assignment fixed, or at least     */
/*  its fixed assignment path is longer than it was.  Check whether m        */
/*  needs to be attached.                                                    */
/*                                                                           */
/*  This function assumes that m is attached and has cost 0.                 */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorMeetAssignFix(KHE_LINK_EVENTS_MONITOR m)
{
  if( KheLinkEventsMonitorHasFixedZeroCost(m) )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  detaching ");
      KheLinkEventsMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorMeetAssignUnFix(KHE_LINK_EVENTS_MONITOR m)      */
/*                                                                           */
/*  A meet monitored by m has just had its assignment unfixed, or at least   */
/*  its fixed assignment path is shorter than it was.  Check whether m       */
/*  needs to be attached.                                                    */
/*                                                                           */
/*  This function assumes that m is detached.                                */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorMeetAssignUnFix(KHE_LINK_EVENTS_MONITOR m)
{
  if( !KheLinkEventsMonitorHasFixedZeroCost(m) )
  {
    if( DEBUG1 )
    {
      fprintf(stderr, "  attaching ");
      KheLinkEventsMonitorDebug(m, 1, -1, stderr);
      fprintf(stderr, "\n");
    }
    KheMonitorAttachToSoln((KHE_MONITOR) m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int ValDeviations(KHE_LINK_EVENTS_MONITOR m, int val)                    */
/*                                                                           */
/*  Return the number of deviations associated with val.  This will be 0     */
/*  if val is 0 or the wanted multiplicity, and 1 otherwise.                 */
/*                                                                           */
/*****************************************************************************/

static int ValDeviations(KHE_LINK_EVENTS_MONITOR m, int val)
{
  return val != 0 && val != m->wanted_multiplicity ? 1 : 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorAssignNonClash(KHE_LINK_EVENTS_MONITOR m,       */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform m that one of the events it is monitoring has been assigned       */
/*  this time for the first time (i.e. it's not a clash).                    */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorAssignNonClash(KHE_LINK_EVENTS_MONITOR m,
  int assigned_time_index)
{
  int old_val, new_val;
  old_val = HaArray(m->multiplicities, assigned_time_index);
  new_val = old_val + 1;
  HnAssert(new_val <= m->wanted_multiplicity,
    "KheLinkEventsMonitorAssignNonClash internal error");
  HaArrayPut(m->multiplicities, assigned_time_index, new_val);
  m->new_deviation += (ValDeviations(m, new_val) - ValDeviations(m, old_val));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorUnAssignNonClash(KHE_LINK_EVENTS_MONITOR m,     */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform m that one of the events it is monitoring has been unassigned     */
/*  this time for the first time (i.e. it's not a clash).                    */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorUnAssignNonClash(KHE_LINK_EVENTS_MONITOR m,
  int assigned_time_index)
{
  int old_val, new_val;
  old_val = HaArray(m->multiplicities, assigned_time_index);
  new_val = old_val - 1;
  HnAssert(new_val >= 0, "KheLinkEventsMonitorUnAssignNonClash internal error");
  HaArrayPut(m->multiplicities, assigned_time_index, new_val);
  m->new_deviation += (ValDeviations(m, new_val) - ValDeviations(m, old_val));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorFlush(KHE_LINK_EVENTS_MONITOR m)                */
/*                                                                           */
/*  Flush m.                                                                 */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorFlush(KHE_LINK_EVENTS_MONITOR m)
{
  if( m->new_deviation != m->deviation )
  {
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->new_deviation));
    m->deviation = m->new_deviation;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "deviations"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsMonitorDeviation(KHE_LINK_EVENTS_MONITOR m)             */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheLinkEventsMonitorDeviation(KHE_LINK_EVENTS_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLinkEventsMonitorDeviationDescription(KHE_LINK_EVENTS_MONITOR m)*/
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheLinkEventsMonitorDeviationDescription(KHE_LINK_EVENTS_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count, val;  char *name;  KHE_INSTANCE ins;
  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d: ", m->deviation);
    ins = KheEventGroupInstance(m->event_group);
    count = 0;
    HaArrayForEach(m->multiplicities, val, i)
      if( ValDeviations(m, val) != 0 )
      {
	if( count > 0 )
	  HnStringAdd(&ac, "; ");
	name = KheTimeName(KheInstanceTime(ins, i));
        HnStringAdd(&ac, name == NULL ? "?" : name);
	count++;
      }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsMonitorDeviationCount(KHE_LINK_EVENTS_MONITOR m)        */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheLinkEventsMonitorDeviationCount(KHE_LINK_EVENTS_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheLinkEventsMonitorDeviation(KHE_LINK_EVENTS_MONITOR m, int i)      */
/*                                                                           */
/*  Return the i'th deviation.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheLinkEventsMonitorDeviation(KHE_LINK_EVENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheLinkEventsMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheLinkEventsMonitorDeviationDescription(                          */
/*    KHE_LINK_EVENTS_MONITOR m, int i)                                      */
/*                                                                           */
/*  Return a description of the i'th deviation.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheLinkEventsMonitorDeviationDescription(
  KHE_LINK_EVENTS_MONITOR m, int i)
{
  HnAssert(i == 0, "KheLinkEventsMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheLinkEventsMonitorPointOfApplication(KHE_LINK_EVENTS_MONITOR m)  */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheLinkEventsMonitorPointOfApplication(KHE_LINK_EVENTS_MONITOR m)
{
  return KheEventGroupName(KheLinkEventsMonitorEventGroup(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheLinkEventsMonitorId(KHE_LINK_EVENTS_MONITOR m)                  */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheLinkEventsMonitorId(KHE_LINK_EVENTS_MONITOR m)
{
  KHE_EVENT_GROUP eg;  char *constraint_id, *eg_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    eg = KheLinkEventsMonitorEventGroup(m);
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
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheLinkEventsMonitorDebug(KHE_LINK_EVENTS_MONITOR m,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheLinkEventsMonitorDebug(KHE_LINK_EVENTS_MONITOR m,
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

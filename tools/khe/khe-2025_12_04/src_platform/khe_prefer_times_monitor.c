
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
/*  FILE:         khe_prefer_times_monitor.c                                 */
/*  DESCRIPTION:  An prefer times monitor                                    */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_TIMES_MONITOR - an prefer times monitor                       */
/*                                                                           */
/*****************************************************************************/

struct khe_prefer_times_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_EVENT_IN_SOLN		event_in_soln;		/* enclosing es      */
  KHE_PREFER_TIMES_CONSTRAINT	constraint;		/* constraint        */
  KHE_PREFER_TIMES_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_TIMES_MONITOR KhePreferTimesMonitorMake(KHE_EVENT_IN_SOLN es, */
/*    KHE_PREFER_TIMES_CONSTRAINT c)                                         */
/*                                                                           */
/*  Make a new prefer times monitor with these attributes.                   */
/*                                                                           */
/*****************************************************************************/

KHE_PREFER_TIMES_MONITOR KhePreferTimesMonitorMake(KHE_EVENT_IN_SOLN es,
  KHE_PREFER_TIMES_CONSTRAINT c)
{
  KHE_PREFER_TIMES_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheEventInSolnSoln(es);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_PREFER_TIMES_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->deviation = 0;
  res->event_in_soln = es;
  res->constraint = c;
  res->copy = NULL;
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_TIMES_MONITOR KhePreferTimesMonitorCopyPhase1(                */
/*    KHE_PREFER_TIMES_MONITOR m, HA_ARENA a)                                */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_PREFER_TIMES_MONITOR KhePreferTimesMonitorCopyPhase1(
  KHE_PREFER_TIMES_MONITOR m, HA_ARENA a)
{
  KHE_PREFER_TIMES_MONITOR copy;
  if( m->copy == NULL )
  {
    HaMake(copy, a);
    m->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) m, a);
    copy->deviation = m->deviation;
    copy->event_in_soln = KheEventInSolnCopyPhase1(m->event_in_soln, a);
    copy->constraint = m->constraint;
    copy->copy = NULL;
  }
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorCopyPhase2(KHE_PREFER_TIMES_MONITOR m)         */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorCopyPhase2(KHE_PREFER_TIMES_MONITOR m)
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
/*  void KhePreferTimesMonitorDelete(KHE_PREFER_TIMES_MONITOR m)             */
/*                                                                           */
/*  Free m.                                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePreferTimesMonitorDelete(KHE_PREFER_TIMES_MONITOR m)
{
  if( m->attached )
    KhePreferTimesMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheEventInSolnDeleteMonitor(m->event_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_PREFER_TIMES_CONSTRAINT KhePreferTimesMonitorConstraint(             */
/*    KHE_PREFER_TIMES_MONITOR m)                                            */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_PREFER_TIMES_CONSTRAINT KhePreferTimesMonitorConstraint(
  KHE_PREFER_TIMES_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KhePreferTimesMonitorEvent(KHE_PREFER_TIMES_MONITOR m)         */
/*                                                                           */
/*  Return the event that m is monitoring.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KhePreferTimesMonitorEvent(KHE_PREFER_TIMES_MONITOR m)
{
  return KheEventInSolnEvent(m->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  Unlinked deviation is 0.                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorAttachToSoln(KHE_PREFER_TIMES_MONITOR m)       */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorAttachToSoln(KHE_PREFER_TIMES_MONITOR m)
{
  m->attached = true;
  KheEventInSolnAttachMonitor(m->event_in_soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorDetachFromSoln(KHE_PREFER_TIMES_MONITOR m)     */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorDetachFromSoln(KHE_PREFER_TIMES_MONITOR m)
{
  KheEventInSolnDetachMonitor(m->event_in_soln, (KHE_MONITOR) m);
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool PreferTimesMonitorMustHaveZeroCost(KHE_PREFER_TIMES_MONITOR m)      */
/*                                                                           */
/*  Return true when m has provably zero cost.  This will be so when the     */
/*  following holds for every meet derived from the event that m monitors:   */
/*                                                                           */
/*  * Either the meet's splits are fixed and its duration is not one         */
/*    of those monitored by m;                                               */
/*                                                                           */
/*  * Or the meet's domain is fixed and is a subset of the time group        */
/*    that m constrains meets to lie in.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KhePreferTimesMonitorMustHaveZeroCost(KHE_PREFER_TIMES_MONITOR m)
{
  KHE_MEET meet;  KHE_TIME_GROUP tg;  int i;  KHE_EVENT e;
  KHE_PREFER_TIMES_CONSTRAINT c;
  c = KhePreferTimesMonitorConstraint(m);
  tg = KhePreferTimesConstraintDomain(c);
  e = KhePreferTimesMonitorEvent(m);
  for( i = 0;  i < KheEventMeetCount(m->soln, e);  i++ )
  {
    meet = KheEventMeet(m->soln, e, i);
    if( KheMeetSplitIsFixed(meet) &&
	KhePreferTimesConstraintDuration(c) != KHE_ANY_DURATION &&
        KhePreferTimesConstraintDuration(c) != KheMeetDuration(meet) )
    {
      ** this is the first case above **
      continue;
    }
    else if( KheMeetDomainIsFixed(meet) &&
	KheTimeGroupSubset(KheMeetDomain(meet), tg) )
    {
      ** this is the second case above **
      continue;
    }
    else
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorAttachCheck(KHE_PREFER_TIMES_MONITOR m)        */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePreferTimesMonitorAttachCheck(KHE_PREFER_TIMES_MONITOR m)
{
  if( KhePreferTimesMonitorMustHaveZeroCost(m) )
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
/*  void KhePreferTimesMonitorFix(KHE_PREFER_TIMES_MONITOR m)                */
/*                                                                           */
/*  A meet split or domain fix has occurred.  Check whether m now has        */
/*  provably zero fixed cost and detach it if so.  (This function assumes    */
/*  that m is attached; m also has cost 0, although that fact is not used.)  */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePreferTimesMonitorFix(KHE_PREFER_TIMES_MONITOR m)
{
  if( KhePreferTimesMonitorMustHaveZeroCost(m) )
    KheMonitorDetachFromSoln((KHE_MONITOR) m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorUnFix(KHE_PREFER_TIMES_MONITOR m)              */
/*                                                                           */
/*  A meet split or domain unfix has occurred.  Check whether m now has      */
/*  provably zero fixed cost, and attach it if not.  (This function assumes  */
/*  that m is detached.)                                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KhePreferTimesMonitorUnFix(KHE_PREFER_TIMES_MONITOR m)
{
  if( !KhePreferTimesMonitorMustHaveZeroCost(m) )
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
/*  bool KhePreferTimesMonitorTimeRange(KHE_PREFER_TIMES_MONITOR m,          */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KhePreferTimesMonitorTimeRange(KHE_PREFER_TIMES_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT e;
  e = KhePreferTimesMonitorEvent(m);
  return KheEventTimeRange(e, m->soln, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorSetSweepTime(KHE_PREFER_TIMES_MONITOR m,       */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorSetSweepTime(KHE_PREFER_TIMES_MONITOR m, KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesMonitorSweepTimeRange(KHE_PREFER_TIMES_MONITOR m,     */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for m.                                */
/*                                                                           */
/*****************************************************************************/

bool KhePreferTimesMonitorSweepTimeRange(KHE_PREFER_TIMES_MONITOR m,
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
/*  bool KhePreferTimesMonitorApplies(KHE_PREFER_TIMES_MONITOR m, int durn)  */
/*                                                                           */
/*  Return true if m is interested in meets of duration durn.                */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesMonitorApplies(KHE_PREFER_TIMES_MONITOR m, int durn)
{
  int d;
  d = KhePreferTimesConstraintDuration(m->constraint);
  return d == durn || d == KHE_ANY_DURATION;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesMonitorWrongTime(KHE_PREFER_TIMES_MONITOR m,          */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Return true when assigning t should attract a cost.                      */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesMonitorWrongTime(KHE_PREFER_TIMES_MONITOR m,
  KHE_TIME t)
{
  int pos;
  return t != NULL && !KheTimeGroupContains(
    KhePreferTimesConstraintDomain(m->constraint), t, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KhePreferTimesMonitorWrongTimeIndex(KHE_PREFER_TIMES_MONITOR m,     */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Like KhePreferTimesMonitorWrongTime except we are sure that there is     */
/*  a time, and we are given its index.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KhePreferTimesMonitorWrongTimeIndex(KHE_PREFER_TIMES_MONITOR m,
  int assigned_time_index)
{
  int pos;
  return !KheTimeGroupContainsIndex(
    KhePreferTimesConstraintDomain(m->constraint), assigned_time_index, &pos);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorAddMeet(KHE_PREFER_TIMES_MONITOR m,            */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of adding meet.                                       */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorAddMeet(KHE_PREFER_TIMES_MONITOR m, KHE_MEET meet)
{
  int durn;
  durn = KheMeetDuration(meet);
  if( KhePreferTimesMonitorApplies(m, durn) &&
      KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet)) )
  {
    m->deviation += durn;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorDeleteMeet(KHE_PREFER_TIMES_MONITOR m,         */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of deleting meet.                                     */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorDeleteMeet(KHE_PREFER_TIMES_MONITOR m, KHE_MEET meet)
{
  int durn;
  durn = KheMeetDuration(meet);
  if( KhePreferTimesMonitorApplies(m, durn) &&
      KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet)) )
  {
    m->deviation -= durn;
    HnAssert(m->deviation >= 0,
      "KhePreferTimesMonitorDeleteMeet internal error");
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorSplitMeet(KHE_PREFER_TIMES_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Let m know that a meet has just split into meet1 and meet2.              */
/*  Either both meets are assigned times, or they aren't.                    */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorSplitMeet(KHE_PREFER_TIMES_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2;
  if( KheMeetAsstTime(meet1) != NULL )
  {
    durn1 = KheMeetDuration(meet1);
    durn2 = KheMeetDuration(meet2);

    /* remove old deviation */
    if( KhePreferTimesMonitorApplies(m, durn1 + durn2) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet1)) )
      m->deviation -= durn1 + durn2;

    /* add meet1 deviation */
    if( KhePreferTimesMonitorApplies(m, durn1) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet1)) )
      m->deviation += durn1;

    /* add meet2 deviation */
    if( KhePreferTimesMonitorApplies(m, durn2) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet2)) )
      m->deviation += durn2;

    /* report any change in cost */
    HnAssert(m->deviation >= 0,
      "KhePreferTimesMonitorSplitMeet internal error");
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorMergeMeet(KHE_PREFER_TIMES_MONITOR m,          */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
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

void KhePreferTimesMonitorMergeMeet(KHE_PREFER_TIMES_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int durn1, durn2;
  if( KheMeetAsstTime(meet1) != NULL )
  {
    durn1 = KheMeetDuration(meet1);
    durn2 = KheMeetDuration(meet2);

    /* remove meet1 cost */
    if( KhePreferTimesMonitorApplies(m, durn1) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet1)) )
      m->deviation -= durn1;

    /* remove meet2 cost */
    if( KhePreferTimesMonitorApplies(m, durn2) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet2)) )
      m->deviation -= durn2;

    /* add merged cost */
    if( KhePreferTimesMonitorApplies(m, durn1 + durn2) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet1)) )
      m->deviation += durn1 + durn2;

    /* report any change in cost */
    HnAssert(m->deviation >= 0,
      "KhePreferTimesMonitorMergeMeet internal error");
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorAssignTime(KHE_PREFER_TIMES_MONITOR m,         */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Let m know that meet has just been assigned the time with this index.    */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorAssignTime(KHE_PREFER_TIMES_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  int durn;
  durn = KheMeetDuration(meet);
  if( KhePreferTimesMonitorApplies(m, durn) &&
      KhePreferTimesMonitorWrongTimeIndex(m, assigned_time_index) )
  {
    m->deviation += durn;
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePreferTimesMonitorUnAssignTime(KHE_PREFER_TIMES_MONITOR m,       */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Let m know that meet is about to be unassigned the time with this index. */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorUnAssignTime(KHE_PREFER_TIMES_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  int durn;
  durn = KheMeetDuration(meet);
  if( KhePreferTimesMonitorApplies(m, durn) &&
      KhePreferTimesMonitorWrongTimeIndex(m, assigned_time_index) )
  {
    m->deviation -= durn;
    HnAssert(m->deviation >= 0,
      "KhePreferTimesMonitorUnAssignTime internal error");
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
/*  int KhePreferTimesMonitorDeviation(KHE_PREFER_TIMES_MONITOR m)           */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KhePreferTimesMonitorDeviation(KHE_PREFER_TIMES_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesMonitorUnpreferredMeetCount(KHE_PREFER_TIMES_MONITOR m)*/
/*                                                                           */
/*  Return the number of meets with unpreferred assignments monitored by m.  */
/*                                                                           */
/*****************************************************************************/

static int KhePreferTimesMonitorUnpreferredMeetCount(KHE_PREFER_TIMES_MONITOR m)
{
  int i, count;  KHE_EVENT e;  KHE_MEET meet;
  e = KhePreferTimesMonitorEvent(m);
  count = 0;
  for( i = 0;  i < KheEventMeetCount(m->soln, e);  i++ )
  {
    meet = KheEventMeet(m->soln, e, i);
    if( KhePreferTimesMonitorApplies(m, KheMeetDuration(meet)) &&
	KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet)) )
      count++;
  }
  return count;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePreferTimesMonitorDeviationDescription(                         */
/*    KHE_PREFER_TIMES_MONITOR m)                                            */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KhePreferTimesMonitorDeviationDescription(KHE_PREFER_TIMES_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count;  KHE_EVENT e;  KHE_MEET meet;  HA_ARENA a;
  a = KheSolnArena(m->soln);
  if( m->deviation == 0 )
    return "0";
  else if( KhePreferTimesMonitorUnpreferredMeetCount(m) == 1 )
    return HnStringMake(a, "%d", m->deviation);
  else
  {
    HnStringBegin(ac, a);
    HnStringAdd(&ac, "%d: ", m->deviation);
    e = KhePreferTimesMonitorEvent(m);
    count = 0;
    for( i = 0;  i < KheEventMeetCount(m->soln, e);  i++ )
    {
      meet = KheEventMeet(m->soln, e, i);
      if( KhePreferTimesMonitorApplies(m, KheMeetDuration(meet)) &&
          KhePreferTimesMonitorWrongTime(m, KheMeetAsstTime(meet)) )
      {
	if( count > 0 )
          HnStringAdd(&ac, "; ");
	HnStringAdd(&ac, "%d", KheMeetDuration(meet));
	count++;
      }
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesMonitorDeviationCount(KHE_PREFER_TIMES_MONITOR m)      */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KhePreferTimesMonitorDeviationCount(KHE_PREFER_TIMES_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KhePreferTimesMonitorDeviation(KHE_PREFER_TIMES_MONITOR m, int i)    */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KhePreferTimesMonitorDeviation(KHE_PREFER_TIMES_MONITOR m, int i)
{
  HnAssert(i == 0, "KhePreferTimesMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KhePreferTimesMonitorDeviationDescription(                         */
/*    KHE_PREFER_TIMES_MONITOR m, int i)                                     */
/*                                                                           */
/*  Return a description of the i'th deviation of m.                         */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KhePreferTimesMonitorDeviationDescription(
  KHE_PREFER_TIMES_MONITOR m, int i)
{
  HnAssert(i == 0, "KhePreferTimesMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KhePreferTimesMonitorPointOfApplication(KHE_PREFER_TIMES_MONITOR m)*/
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KhePreferTimesMonitorPointOfApplication(KHE_PREFER_TIMES_MONITOR m)
{
  return KheEventName(KhePreferTimesMonitorEvent(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KhePreferTimesMonitorId(KHE_PREFER_TIMES_MONITOR m)                */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KhePreferTimesMonitorId(KHE_PREFER_TIMES_MONITOR m)
{
  char *constraint_id, *event_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    a = KheSolnArena(m->soln);
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    event_id = KheEventId(KhePreferTimesMonitorEvent(m));
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
/*  void KhePreferTimesMonitorDebug(KHE_PREFER_TIMES_MONITOR m,              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KhePreferTimesMonitorDebug(KHE_PREFER_TIMES_MONITOR m,
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

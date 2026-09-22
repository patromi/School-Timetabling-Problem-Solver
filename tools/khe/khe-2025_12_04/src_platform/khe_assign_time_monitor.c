
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
/*  FILE:         khe_assign_time_monitor.c                                  */
/*  DESCRIPTION:  An assign time monitor                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG1 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_TIME_MONITOR - an assign time monitor                         */
/*                                                                           */
/*****************************************************************************/

struct khe_assign_time_monitor_rec {
  INHERIT_MONITOR(unused, deviation)
  KHE_EVENT_IN_SOLN		event_in_soln;		/* enclosing es      */
  KHE_ASSIGN_TIME_CONSTRAINT	constraint;		/* constraint        */
  KHE_ASSIGN_TIME_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_TIME_MONITOR KheAssignTimeMonitorMake(KHE_EVENT_IN_SOLN es,   */
/*    KHE_ASSIGN_TIME_CONSTRAINT c)                                          */
/*                                                                           */
/*  Make a new assign time monitor with these attributes and add it to es.   */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_TIME_MONITOR KheAssignTimeMonitorMake(KHE_EVENT_IN_SOLN es,
  KHE_ASSIGN_TIME_CONSTRAINT c)
{
  KHE_ASSIGN_TIME_MONITOR res;  KHE_SOLN soln;  HA_ARENA a;
  soln = KheEventInSolnSoln(es);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_ASSIGN_TIME_MONITOR_TAG,
    KheConstraintCostFunction((KHE_CONSTRAINT) c),
    KheConstraintCombinedWeight((KHE_CONSTRAINT) c));
  res->deviation = 0;
  res->event_in_soln = es;
  res->constraint = c;
  res->copy = NULL;
  KheEventInSolnAddMonitor(es, (KHE_MONITOR) res);
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorMake(es, c) = %p\n", (void *) res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_TIME_MONITOR KheAssignTimeMonitorCopyPhase1(                  */
/*    KHE_ASSIGN_TIME_MONITOR m, HA_ARENA a)                                 */
/*                                                                           */
/*  Carry out Phase 1 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_TIME_MONITOR KheAssignTimeMonitorCopyPhase1(
  KHE_ASSIGN_TIME_MONITOR m, HA_ARENA a)
{
  KHE_ASSIGN_TIME_MONITOR copy;
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
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorCopyPhase1(%p) = %p\n",
      (void *) m, (void *) m->copy);
  return m->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorCopyPhase2(KHE_ASSIGN_TIME_MONITOR m)           */
/*                                                                           */
/*  Carry out Phase 2 of copying m.                                          */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorCopyPhase2(KHE_ASSIGN_TIME_MONITOR m)
{
  if( m->copy != NULL )
  {
    m->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) m);
    KheEventInSolnCopyPhase2(m->event_in_soln);
  }
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorCopyPhase2(%p)\n", (void *) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorDelete(KHE_ASSIGN_TIME_MONITOR m)               */
/*                                                                           */
/*  Delete m.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignTimeMonitorDelete(KHE_ASSIGN_TIME_MONITOR m)
{
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorDelete(%p)\n", (void *) m);
  if( m->attached )
    KheAssignTimeMonitorDetachFromSoln(m);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) m);
  KheEventInSolnDeleteMonitor(m->event_in_soln, (KHE_MONITOR) m);
  KheSolnDeleteMonitor(m->soln, (KHE_MONITOR) m);
  MFree(m);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_ASSIGN_TIME_CONSTRAINT KheAssignTimeMonitorConstraint(               */
/*    KHE_ASSIGN_TIME_MONITOR m)                                             */
/*                                                                           */
/*  Return the constraint that m is monitoring.                              */
/*                                                                           */
/*****************************************************************************/

KHE_ASSIGN_TIME_CONSTRAINT KheAssignTimeMonitorConstraint(
  KHE_ASSIGN_TIME_MONITOR m)
{
  return m->constraint;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheAssignTimeMonitorEvent(KHE_ASSIGN_TIME_MONITOR m)           */
/*                                                                           */
/*  Return the event that m is monitoring.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheAssignTimeMonitorEvent(KHE_ASSIGN_TIME_MONITOR m)
{
  return KheEventInSolnEvent(m->event_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*  The unlinked deviation is 0, because when unlinked, an assign time       */
/*  monitor is monitoring no meets, hence no unassigned ones.                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorAttachToSoln(KHE_ASSIGN_TIME_MONITOR m)         */
/*                                                                           */
/*  Attach m.  It is known to be currently detached with cost 0.             */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorAttachToSoln(KHE_ASSIGN_TIME_MONITOR m)
{
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorAttachToSoln(%p)\n", (void *) m);
  m->attached = true;
  KheEventInSolnAttachMonitor(m->event_in_soln, (KHE_MONITOR) m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorDetachFromSoln(KHE_ASSIGN_TIME_MONITOR m)       */
/*                                                                           */
/*  Detach m.  It is known to be currently attached.                         */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorDetachFromSoln(KHE_ASSIGN_TIME_MONITOR m)
{
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorDetachFromSoln(%p)\n", (void *) m);
  KheEventInSolnDetachMonitor(m->event_in_soln, (KHE_MONITOR) m);
  HnAssert(m->deviation == 0,
    "KheAssignTimeMonitorDetachFromSoln internal error");
  m->attached = false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorAttachCheck(KHE_ASSIGN_TIME_MONITOR m)          */
/*                                                                           */
/*  Check the attachment of m.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheAssignTimeMonitorAttachCheck(KHE_ASSIGN_TIME_MONITOR m)
{
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) m) )
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
/*  bool KheAssignTimeMonitorTimeRange(KHE_ASSIGN_TIME_MONITOR m,            */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorTimeRange for m.                                     */
/*                                                                           */
/*****************************************************************************/

bool KheAssignTimeMonitorTimeRange(KHE_ASSIGN_TIME_MONITOR m,
  KHE_TIME *first_time, KHE_TIME *last_time)
{
  KHE_EVENT e;
  e = KheAssignTimeMonitorEvent(m);
  return KheEventTimeRange(e, m->soln, first_time, last_time);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorSetSweepTime(KHE_ASSIGN_TIME_MONITOR m,         */
/*    KHE_ASSIGN_RESOURCE_MONITOR m, KHE_TIME t)                             */
/*                                                                           */
/*  Implement KheMonitorSetSweepTime for m.                                  */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorSetSweepTime(KHE_ASSIGN_TIME_MONITOR m,
  KHE_TIME t)
{
  /* nothing to do */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAssignTimeMonitorSweepTimeRange(KHE_ASSIGN_TIME_MONITOR m,       */
/*    KHE_TIME *first_time, KHE_TIME *last_time)                             */
/*                                                                           */
/*  Implement KheMonitorSweepTimeRange for assign resource monitor m.        */
/*                                                                           */
/*****************************************************************************/

bool KheAssignTimeMonitorSweepTimeRange(KHE_ASSIGN_TIME_MONITOR m,
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
/*  void KheAssignTimeMonitorAddMeet(KHE_ASSIGN_TIME_MONITOR m,              */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of adding meet.                                       */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorAddMeet(KHE_ASSIGN_TIME_MONITOR m, KHE_MEET meet)
{
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorAddMeet(%p dv %d, %p drn %d %s)\n",
      (void *) m, m->deviation, (void *) meet, KheMeetDuration(meet),
      KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX ? "unass" : "ass");
  if( KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX )
  {
    m->deviation += KheMeetDuration(meet);
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorDeleteMeet(KHE_ASSIGN_TIME_MONITOR m,           */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Monitor the effect of deleting meet.                                     */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorDeleteMeet(KHE_ASSIGN_TIME_MONITOR m, KHE_MEET meet)
{
  if( DEBUG1 )
    fprintf(stderr, "KheAssignTimeMonitorDeleteMeet(%p dv %d, %p drn %d %s)\n",
      (void *) m, m->deviation, (void *) meet, KheMeetDuration(meet),
      KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX ? "unass" : "ass");
  if( KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX )
  {
    m->deviation -= KheMeetDuration(meet);
    HnAssert(m->deviation >= 0,
      "KheAssignTimeMonitorDeleteMeet internal error");
    KheMonitorChangeCost((KHE_MONITOR) m,
      KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorSplitMeet(KHE_ASSIGN_TIME_MONITOR m,            */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Let m know that a meet has just split into meet1 and meet2.              */
/*  Either both meets are assigned times, or they aren't.                    */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorSplitMeet(KHE_ASSIGN_TIME_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  /* no change in cost is possible */
  if( DEBUG1 )
    fprintf(stderr,
      "KheAssignTimeMonitorSplitMeet(%p dv %d, %p drn %d %s, %p drn %d %s)\n",
      (void *) m, m->deviation, (void *) meet1, KheMeetDuration(meet1),
      KheMeetAssignedTimeIndex(meet1) == NO_TIME_INDEX ? "unass" : "ass",
      (void *) meet2, KheMeetDuration(meet2),
      KheMeetAssignedTimeIndex(meet2) == NO_TIME_INDEX ? "unass" : "ass");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorMergeMeet(KHE_ASSIGN_TIME_MONITOR m,            */
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

void KheAssignTimeMonitorMergeMeet(KHE_ASSIGN_TIME_MONITOR m,
  KHE_MEET meet1, KHE_MEET meet2)
{
  /* no change in cost is possible */
  if( DEBUG1 )
    fprintf(stderr,
      "KheAssignTimeMonitorMergeMeet(%p dv %d, %p drn %d %s, %p drn %d %s)\n",
      (void *) m, m->deviation, (void *) meet1, KheMeetDuration(meet1),
      KheMeetAssignedTimeIndex(meet1) == NO_TIME_INDEX ? "unass" : "ass",
      (void *) meet2, KheMeetDuration(meet2),
      KheMeetAssignedTimeIndex(meet2) == NO_TIME_INDEX ? "unass" : "ass");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorAssignTime(KHE_ASSIGN_TIME_MONITOR m,           */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Let m know that meet has just been assigned the time with this index.    */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorAssignTime(KHE_ASSIGN_TIME_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  if( DEBUG1 )
    fprintf(stderr,
      "KheAssignTimeMonitorAssignTime(%p dv %d, %p drn %d %s, %d)\n",
      (void *) m, m->deviation, (void *) meet, KheMeetDuration(meet),
      KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX ? "unass" : "ass",
      assigned_time_index);
  m->deviation -= KheMeetDuration(meet);
  HnAssert(m->deviation >= 0,
    "KheAssignTimeMonitorAssignTime internal error");
  KheMonitorChangeCost((KHE_MONITOR) m,
    KheMonitorDevToCost((KHE_MONITOR) m, m->deviation));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAssignTimeMonitorUnAssignTime(KHE_ASSIGN_TIME_MONITOR m,         */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Let m know that meet is about to be unassigned the time with this index. */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorUnAssignTime(KHE_ASSIGN_TIME_MONITOR m,
  KHE_MEET meet, int assigned_time_index)
{
  if( DEBUG1 )
    fprintf(stderr,
      "KheAssignTimeMonitorUnAssignTime(%p dv %d, %p drn %d %s, %d)\n",
      (void *) m, m->deviation, (void *) meet, KheMeetDuration(meet),
      KheMeetAssignedTimeIndex(meet) == NO_TIME_INDEX ? "unass" : "ass",
      assigned_time_index);
  m->deviation += KheMeetDuration(meet);
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
/*  int KheAssignTimeMonitorDeviation(KHE_ASSIGN_TIME_MONITOR m)             */
/*                                                                           */
/*  Return the deviation of m.                                               */
/*                                                                           */
/*****************************************************************************/

int KheAssignTimeMonitorDeviation(KHE_ASSIGN_TIME_MONITOR m)
{
  return m->deviation;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignMeetMonitorUnassignedMeetCount(                             */
/*    KHE_ASSIGN_TIME_MONITOR m)                                             */
/*                                                                           */
/*  Return the number of unassigned meets monitored by m.                    */
/*                                                                           */
/*****************************************************************************/

static int KheAssignMeetMonitorUnassignedMeetCount(KHE_ASSIGN_TIME_MONITOR m)
{
  int i, count;  KHE_EVENT e;  KHE_MEET meet;
  e = KheAssignTimeMonitorEvent(m);
  count = 0;
  for( i = 0;  i < KheEventMeetCount(m->soln, e);  i++ )
  {
    meet = KheEventMeet(m->soln, e, i);
    if( KheMeetAsstTime(meet) == NULL )
      count++;
  }
  return count;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignTimeMonitorDeviationDescription(KHE_ASSIGN_TIME_MONITOR m)*/
/*    KHE_ASSIGN_TIME_MONITOR m)                                             */
/*                                                                           */
/*  Return a description of the deviation of m in heap memory.               */
/*                                                                           */
/*****************************************************************************/

char *KheAssignTimeMonitorDeviationDescription(KHE_ASSIGN_TIME_MONITOR m)
{
  HA_ARRAY_NCHAR ac;  int i, count;  KHE_EVENT e;  KHE_MEET meet;  HA_ARENA a;
  if( m->deviation == 0 )
    return "0";
  else
  {
    a = KheSolnArena(m->soln);
    HnStringBegin(ac, a);
    if( KheAssignMeetMonitorUnassignedMeetCount(m) == 1 )
      HnStringAdd(&ac, "%d", m->deviation);
    else
    {
      HnStringAdd(&ac, "%d: ", m->deviation);
      e = KheAssignTimeMonitorEvent(m);
      count = 0;
      for( i = 0;  i < KheEventMeetCount(m->soln, e);  i++ )
      {
	meet = KheEventMeet(m->soln, e, i);
	if( KheMeetAsstTime(meet) == NULL )
	{
	  if( count > 0 )
	    HnStringAdd(&ac, "; ");
	  HnStringAdd(&ac, "%d", KheMeetDuration(meet));
	  count++;
	}
      }
    }
    return HnStringEnd(ac);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeMonitorDeviationCount(KHE_ASSIGN_TIME_MONITOR m)        */
/*                                                                           */
/*  Return the number of deviations of m.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAssignTimeMonitorDeviationCount(KHE_ASSIGN_TIME_MONITOR m)
{
  return 1;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAssignTimeMonitorDeviation(KHE_ASSIGN_TIME_MONITOR m, int i)      */
/*                                                                           */
/*  Return the i'th deviation of m.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
int KheAssignTimeMonitorDeviation(KHE_ASSIGN_TIME_MONITOR m, int i)
{
  HnAssert(i == 0, "KheAssignTimeMonitorDeviation: i out of range");
  return m->deviation;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignTimeMonitorDeviationDescription(KHE_ASSIGN_TIME_MONITOR m,*/
/*    int i)                                                                 */
/*                                                                           */
/*  Return a description of the i'th deviation.                              */
/*                                                                           */
/*****************************************************************************/

/* ***
char *KheAssignTimeMonitorDeviationDescription(KHE_ASSIGN_TIME_MONITOR m,
  int i)
{
  HnAssert(i == 0, "KheAssignTimeMonitorDeviationDescription: i out of range");
  return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignTimeMonitorPointOfApplication(KHE_ASSIGN_TIME_MONITOR m)  */
/*                                                                           */
/*  Return a description of the point of application of m.                   */
/*                                                                           */
/*****************************************************************************/

char *KheAssignTimeMonitorPointOfApplication(KHE_ASSIGN_TIME_MONITOR m)
{
  return KheEventName(KheAssignTimeMonitorEvent(m));
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheAssignTimeMonitorId(KHE_ASSIGN_TIME_MONITOR m)                  */
/*                                                                           */
/*  Return the Id of m.                                                      */
/*                                                                           */
/*****************************************************************************/

char *KheAssignTimeMonitorId(KHE_ASSIGN_TIME_MONITOR m)
{
  char *constraint_id, *event_id;  HA_ARENA a;
  if( m->id == NULL )
  {
    a = KheSolnArena(m->soln);
    constraint_id = KheConstraintId((KHE_CONSTRAINT) m->constraint);
    event_id = KheEventId(KheAssignTimeMonitorEvent(m));
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
/*  void KheAssignTimeMonitorDebug(KHE_ASSIGN_TIME_MONITOR m,                */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of m onto fp with the given verbosity and indent.            */
/*                                                                           */
/*****************************************************************************/

void KheAssignTimeMonitorDebug(KHE_ASSIGN_TIME_MONITOR m,
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

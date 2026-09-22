
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
/*  FILE:         khe_timetable_monitor.c                                    */
/*  DESCRIPTION:  A timetable monitor                                        */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_CELL - one time of a timetable                                  */
/*                                                                           */
/*  NB avoid clashes and link events monitors monitor every cell, but we     */
/*  don't store them in every cell; we just assume they're there.            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_cell_rec *KHE_TIME_CELL;
typedef HA_ARRAY(KHE_TIME_CELL) ARRAY_KHE_TIME_CELL;

struct khe_time_cell_rec {
  KHE_TIME			time;			/* monitored time    */
  ARRAY_KHE_MEET		meets;			/* incidences        */
  KHE_TIME_CELL			copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR - an event timetable monitor                 */
/*                                                                           */
/*****************************************************************************/

struct khe_event_timetable_monitor_rec {
  INHERIT_MONITOR(unused, attached_monitor_count)
  ARRAY_KHE_EVENT_IN_SOLN	events_in_soln;		/* event             */
  ARRAY_KHE_TIME_CELL		time_cells;		/* time cells        */
  ARRAY_KHE_LINK_EVENTS_MONITOR	link_events_monitors;	/* link events m's   */
  KHE_EVENT_TIMETABLE_MONITOR	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time cells" (private)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_CELL KheTimeCellMake(KHE_TIME t)                                */
/*                                                                           */
/*  Make and initialize a new time cell object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_CELL KheTimeCellMake(KHE_TIME t, HA_ARENA a)
{
  KHE_TIME_CELL res;
  HaMake(res, a);
  res->time = t;
  HaArrayInit(res->meets, a);
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_CELL KheTimeCellCopyPhase1(KHE_TIME_CELL tc, HA_ARENA a)        */
/*                                                                           */
/*  Carry out Phase 1 of copying tc.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_CELL KheTimeCellCopyPhase1(KHE_TIME_CELL tc, HA_ARENA a)
{
  KHE_TIME_CELL copy;  KHE_MEET meet;  int i;
  if( tc->copy == NULL )
  {
    HaMake(copy, a);
    tc->copy = copy;
    copy->time = tc->time;
    HaArrayInit(copy->meets, a);
    HaArrayForEach(tc->meets, meet, i)
      HaArrayAddLast(copy->meets, KheMeetCopyPhase1(meet, a));
    copy->copy = NULL;
  }
  return tc->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeCellCopyPhase2(KHE_TIME_CELL tc)                             */
/*                                                                           */
/*  Carry out Phase 2 of copying tc.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheTimeCellCopyPhase2(KHE_TIME_CELL tc)
{
  KHE_MEET meet;  int i;
  if( tc->copy != NULL )
  {
    tc->copy = NULL;
    HaArrayForEach(tc->meets, meet, i)
      KheMeetCopyPhase2(meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeCellDelete(KHE_TIME_CELL tc)                                 */
/*                                                                           */
/*  Delete tc.  The elements of the arrays don't have to be deleted.         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTimeCellDelete(KHE_TIME_CELL tc)
{
  MArrayFree(tc->meets);
  MFree(tc);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeCellDebug(KHE_TIME_CELL tc, int verbosity, int indent,       */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of tc onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheTimeCellDebug(KHE_TIME_CELL tc, int verbosity,
  int indent, FILE *fp)
{
  KHE_MEET meet;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ %s", KheTimeId(tc->time) == NULL ? "-" :
      KheTimeId(tc->time));
    HaArrayForEach(tc->meets, meet, i)
    {
      fprintf(fp, i == 0 ? ": " : " ");
      KheMeetDebug(meet, 1, -1, fp);
    }
    fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorAddTimeCells(                               */
/*    KHE_EVENT_TIMETABLE_MONITOR etm)                                       */
/*                                                                           */
/*  Add time cells to etm, if there are none already.  Time cells are added  */
/*  only when the monitor is first attached, as an optimization.             */
/*                                                                           */
/*****************************************************************************/

static void KheEventTimetableMonitorAddTimeCells(
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_INSTANCE ins;  int i;  KHE_TIME_CELL tc;  HA_ARENA a;
  if( HaArrayCount(etm->time_cells) == 0 )
  {
    a = KheSolnArena(etm->soln);
    ins = KheSolnInstance(etm->soln);
    for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
    {
      tc = KheTimeCellMake(KheInstanceTime(ins, i), a);
      HaArrayAddLast(etm->time_cells, tc);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitorMake(KHE_SOLN soln,  */
/*    KHE_RESOURCE_IN_SOLN rs, KHE_EVENT_IN_SOLN es)                         */
/*                                                                           */
/*  Make a new timetable monitor for soln, monitoring either rs or es.       */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitorMake(KHE_SOLN soln,
  KHE_EVENT_GROUP eg)
{
  KHE_EVENT_TIMETABLE_MONITOR res;  int i;  KHE_EVENT e;  HA_ARENA a;
  if( DEBUG3 )
    fprintf(stderr, "  calling KheEventTimetableMonitorMake(soln, eg %d)\n",
      KheEventGroupEventCount(eg));
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_EVENT_TIMETABLE_MONITOR_TAG, KHE_LINEAR_COST_FUNCTION, 0);
  HaArrayInit(res->events_in_soln, a);
  for( i = 0;  i < KheEventGroupEventCount(eg);  i++ )
  {
    e = KheEventGroupEvent(eg, i);
    HaArrayAddLast(res->events_in_soln, KheSolnEventInSoln(soln, e));
  }
  /* res->event_in_soln = es; */
  HaArrayInit(res->time_cells, a);
  HaArrayInit(res->link_events_monitors, a);
  res->attached_monitor_count = 0;
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitorCopyPhase1(          */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, HA_ARENA a)                           */
/*                                                                           */
/*  Carry out Phase 1 of copying etm.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_TIMETABLE_MONITOR KheEventTimetableMonitorCopyPhase1(
  KHE_EVENT_TIMETABLE_MONITOR etm, HA_ARENA a)
{
  KHE_TIME_CELL tc;  KHE_LINK_EVENTS_MONITOR lem;  int i;
  KHE_EVENT_TIMETABLE_MONITOR copy;  KHE_EVENT_IN_SOLN es;
  if( etm->copy == NULL )
  {
    HaMake(copy, a);
    etm->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) etm, a);
    HaArrayInit(copy->events_in_soln, a);
    HaArrayForEach(etm->events_in_soln, es, i)
      HaArrayAddLast(copy->events_in_soln, KheEventInSolnCopyPhase1(es, a));
    /* copy->event_in_soln = KheEventInSolnCopyPhase1(etm->event_in_soln); */
    HaArrayInit(copy->time_cells, a);
    HaArrayForEach(etm->time_cells, tc, i)
      HaArrayAddLast(copy->time_cells, KheTimeCellCopyPhase1(tc, a));
    HaArrayInit(copy->link_events_monitors, a);
    HaArrayForEach(etm->link_events_monitors, lem, i)
      HaArrayAddLast(copy->link_events_monitors,
	KheLinkEventsMonitorCopyPhase1(lem, a));
    copy->attached_monitor_count = etm->attached_monitor_count;
    copy->copy = NULL;
  }
  return etm->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorCopyPhase2(KHE_EVENT_TIMETABLE_MONITOR etm) */
/*                                                                           */
/*  Carry out Phase 2 of copying etm.                                        */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorCopyPhase2(KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_TIME_CELL tc;  KHE_LINK_EVENTS_MONITOR lem;  int i;  KHE_EVENT_IN_SOLN es;
  if( etm->copy != NULL )
  {
    etm->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) etm);
    HaArrayForEach(etm->events_in_soln, es, i)
      KheEventInSolnCopyPhase2(es);
    HaArrayForEach(etm->time_cells, tc, i)
      KheTimeCellCopyPhase2(tc);
    HaArrayForEach(etm->link_events_monitors, lem, i)
      KheLinkEventsMonitorCopyPhase2(lem);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDelete(KHE_EVENT_TIMETABLE_MONITOR etm)     */
/*                                                                           */
/*  Delete etm.  The monitors attached to it will be deleted separately.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEventTimetableMonitorDoDelete(KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_TIME_CELL tc;  int i;
  if( etm->attached )
    KheEventTimetableMonitorDetachFromSoln(etm);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) etm);
  KheSolnDeleteMonitor(etm->soln, (KHE_MONITOR) etm);
  MArrayFree(etm->events_in_soln);
  MArrayFree(etm->link_events_monitors);
  HaArrayForEach(etm->time_cells, tc, i)
    KheTimeCellDelete(tc);
  MArrayFree(etm->time_cells);
  MFree(etm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDelete(KHE_EVENT_TIMETABLE_MONITOR etm)     */
/*                                                                           */
/*  Delete etm, but only if it was created by the user.                      */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEventTimetableMonitorDelete(KHE_EVENT_TIMETABLE_MONITOR etm)
{
  KHE_EVENT_TIMETABLE_MONITOR etm2;
  if( HaArrayCount(etm->events_in_soln) == 1 )
  {
    etm2 = KheEventInSolnTimetableMonitor(MArrayFirst(etm->events_in_soln));
    HnAssert(etm2 != etm, "KheEventTimetableMonitorDelete: etm was created "
      "automatically and can only be deleted when the solution is deleted");
  }
  KheEventTimetableMonitorDoDelete(etm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reporting state"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheEventTimetableMonitorTimeMeetCount(                               */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME time)                        */
/*                                                                           */
/*  Return the number of meets running at time.                              */
/*                                                                           */
/*****************************************************************************/

int KheEventTimetableMonitorTimeMeetCount(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_TIME time)
{
  KHE_TIME_CELL tc;
  KheEventTimetableMonitorAddTimeCells(etm);
  tc = HaArray(etm->time_cells, KheTimeIndex(time));
  if( DEBUG2 )
  {
    KHE_MEET m;  int i;
    fprintf(stderr,"[ KheEventTimetableMonitorTimeMeetCount(etm, %s, %d meets):\n",
      KheTimeId(time), HaArrayCount(tc->meets));
    HaArrayForEach(tc->meets, m, i)
      fprintf(stderr, "  meet %s\n", KheMeetId(m));
    fprintf(stderr,"]\n");
  }
  return HaArrayCount(tc->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheEventTimetableMonitorTimeMeet(                               */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME time, int i)                 */
/*                                                                           */
/*  Return the i'th meet running at time.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheEventTimetableMonitorTimeMeet(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_TIME time, int i)
{
  KHE_TIME_CELL tc;
  KheEventTimetableMonitorAddTimeCells(etm);
  tc = HaArray(etm->time_cells, KheTimeIndex(time));
  return HaArray(tc->meets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventTimetableMonitorTimeAvailable(                              */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_MEET meet, KHE_TIME time)         */
/*                                                                           */
/*  Return true if adding meet to etm with the given starting time, or       */
/*  moving it within etm to that time, would cause no clashes and would      */
/*  not place any part of meet off the end of the timetable.                 */
/*                                                                           */
/*****************************************************************************/

bool KheEventTimetableMonitorTimeAvailable(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet, KHE_TIME time)
{
  KHE_TIME_CELL tc;  int i, j, time_index, durn;  KHE_MEET m2;
  KheEventTimetableMonitorAddTimeCells(etm);
  time_index = KheTimeIndex(time);
  durn = KheMeetDuration(meet);
  if( time_index + durn > HaArrayCount(etm->time_cells) )
    return false;
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(etm->time_cells, time_index + i);
    HaArrayForEach(tc->meets, m2, j)
      if( m2 != meet )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* void KheEventTimetableMonitorAttachToSoln(KHE_EVENT_TIMETABLE_MONITOR etm)*/
/*                                                                           */
/*  Attach etm.  It is known to be currently detached.                       */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorAttachToSoln(KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int i;  KHE_EVENT_IN_SOLN es;
  KheEventTimetableMonitorAddTimeCells(etm);
  etm->attached = true;
  HaArrayForEach(etm->events_in_soln, es, i)
    KheEventInSolnAttachMonitor(es, (KHE_MONITOR) etm);
  /* KheEventInSolnAttachMonitor(etm->event_in_soln, (KHE_MONITOR) etm); */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDetachFromSoln(                             */
/*    KHE_EVENT_TIMETABLE_MONITOR etm)                                       */
/*                                                                           */
/*  Detach etm.  It is known to be currently attached.  But unusually, it    */
/*  only acts sometimes:  when no other monitors are attached to it.         */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorDetachFromSoln(KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int i;  KHE_EVENT_IN_SOLN es;
  if( etm->attached_monitor_count == 0 )
  {
    HaArrayForEach(etm->events_in_soln, es, i)
      KheEventInSolnDetachMonitor(es, (KHE_MONITOR) etm);
    /* KheEventInSolnDetachMonitor(etm->event_in_soln, (KHE_MONITOR) etm); */
    etm->attached = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorInternalAttach(                             */
/*    KHE_EVENT_TIMETABLE_MONITOR etm)                                       */
/*                                                                           */
/*  This function is called when etm might need to be attached because some  */
/*  other monitor is about to be attached to it.                             */
/*                                                                           */
/*****************************************************************************/

static void KheEventTimetableMonitorInternalAttach(
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  /* KheEventTimetableMonitorAddTimeCells(etm); */
  etm->attached_monitor_count++;
  if( !etm->attached )
    KheEventTimetableMonitorAttachToSoln(etm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorInternalDetach(                             */
/*    KHE_EVENT_TIMETABLE_MONITOR etm)                                       */
/*                                                                           */
/*  This function is called when etm might need to be detached because some  */
/*  other monitor has just been detached from it.                            */
/*                                                                           */
/*****************************************************************************/

static void KheEventTimetableMonitorInternalDetach(
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  etm->attached_monitor_count--;
  if( etm->attached_monitor_count == 0 )
    KheEventTimetableMonitorDetachFromSoln(etm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorAttachLinkEventsMonitor(                    */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_LINK_EVENTS_MONITOR lem)          */
/*                                                                           */
/*  Attach link events monitor m to etm.                                     */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorAttachLinkEventsMonitor(
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_LINK_EVENTS_MONITOR lem)
{
  int i;  KHE_TIME_CELL tc;
  KheEventTimetableMonitorInternalAttach(etm);
  HaArrayAddLast(etm->link_events_monitors, lem);
  for( i = 0;  i < HaArrayCount(etm->time_cells);  i++ )
  {
    tc = HaArray(etm->time_cells, i);
    if( HaArrayCount(tc->meets) > 0 )
      KheLinkEventsMonitorAssignNonClash(lem, i);
  }
  KheLinkEventsMonitorFlush(lem);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDetachLinkEventsMonitor(                    */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_LINK_EVENTS_MONITOR lem)          */
/*                                                                           */
/*  Detach link events monitor m from etm.                                   */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorDetachLinkEventsMonitor(
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_LINK_EVENTS_MONITOR lem)
{
  int i, pos;  KHE_TIME_CELL tc;
  if( !HaArrayContains(etm->link_events_monitors, lem, &pos) )
    HnAbort("KheEventTimetableMonitorDetachLinkEventsMonitor internal error 1");
  HaArrayDeleteAndShift(etm->link_events_monitors, pos);
  for( i = 0;  i < HaArrayCount(etm->time_cells);  i++ )
  {
    tc = HaArray(etm->time_cells, i);
    if( HaArrayCount(tc->meets) > 0 )
      KheLinkEventsMonitorUnAssignNonClash(lem, i);
  }
  KheLinkEventsMonitorFlush(lem);
  KheEventTimetableMonitorInternalDetach(etm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls from KHE_EVENT_IN_SOLN"                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorAddMeet(KHE_EVENT_TIMETABLE_MONITOR etm,    */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Add meet to etm, if assigned.                                            */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorAddMeet(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet)
{
  if( KheMeetAssignedTimeIndex(meet) != NO_TIME_INDEX )
  {
    if( DEBUG3 )
      fprintf(stderr, "  KheEventTimetableMonitorAddMeet(etm, %s) calling "
	"KheEventTimetableMonitorAssignTime\n", KheMeetId(meet));
    KheEventTimetableMonitorAssignTime(etm, meet,
      KheMeetAssignedTimeIndex(meet));
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDeleteMeet(KHE_EVENT_TIMETABLE_MONITOR etm, */
/*    KHE_MEET meet)                                                         */
/*                                                                           */
/*  Delete meet from etm, if assigned.                                       */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorDeleteMeet(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet)
{
  if( KheMeetAssignedTimeIndex(meet) != NO_TIME_INDEX )
    KheEventTimetableMonitorUnAssignTime(etm, meet,
      KheMeetAssignedTimeIndex(meet));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorSplitMeet(KHE_EVENT_TIMETABLE_MONITOR etm,  */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Inform etm that meet1 and meet2 are splitting.                           */
/*                                                                           */
/*  Implementation note.  For the timetable, this means that some            */
/*  occurrences of meet1 have to be replaced by meet2.  Since this does not  */
/*  change the number of meets at any time, no propagation of these changes  */
/*  to the monitors attached to tm is needed.                                */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorSplitMeet(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int i, pos;  KHE_TIME_CELL tc;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventTimetableMonitorSplitMeet(tm, %p: ",
      (void *) meet1);
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", %p: ", (void *) meet2);
    KheMeetDebug(meet2, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  if( KheMeetAssignedTimeIndex(meet2) != NO_TIME_INDEX )
    for( i = 0;  i < KheMeetDuration(meet2);  i++ )
    {
      tc = HaArray(etm->time_cells, KheMeetAssignedTimeIndex(meet2) + i);
      if( !HaArrayContains(tc->meets, meet1, &pos) )
	HnAbort("KheEventTimetableMonitorSplitMeet internal error");
      HaArrayPut(tc->meets, pos, meet2);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorMergeMeet(KHE_EVENT_TIMETABLE_MONITOR etm,  */
/*    KHE_MEET meet1, KHE_MEET meet2)                                        */
/*                                                                           */
/*  Inform etm that meet1 and meet2 are merging.                             */
/*                                                                           */
/*  Implementation note.  Similarly to splitting, this means that some       */
/*  occurrences of meet2 have to be replaced by meet1, without propagation.  */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorMergeMeet(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet1, KHE_MEET meet2)
{
  int i, pos;  KHE_TIME_CELL tc;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventTimetableMonitorMergeMeet(tm, %p: ",
      (void *) meet1);
    KheMeetDebug(meet1, 1, -1, stderr);
    fprintf(stderr, ", %p: ", (void *) meet2);
    KheMeetDebug(meet2, 1, -1, stderr);
    fprintf(stderr, ")\n");
  }
  if( KheMeetAssignedTimeIndex(meet2) != NO_TIME_INDEX )
    for( i = 0;  i < KheMeetDuration(meet2);  i++ )
    {
      tc = HaArray(etm->time_cells, KheMeetAssignedTimeIndex(meet2) + i);
      if( !HaArrayContains(tc->meets, meet2, &pos) )
	HnAbort("KheEventTimetableMonitorMergeMeet internal error");
      HaArrayPut(tc->meets, pos, meet1);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorAssignTime(KHE_EVENT_TIMETABLE_MONITOR etm, */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Update etm to add meet at assigned_time_index and on for its duration.   */
/*                                                                           */
/*  Implementation note.  At each time we either inform the monitors         */
/*  attached to that time that the resource has become busy then, or else    */
/*  we inform all avoid clashes monitors that the resource has a clash then. */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorAssignTime(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet, int assigned_time_index)
{
  int i, j, durn;  KHE_TIME_CELL tc;  KHE_LINK_EVENTS_MONITOR lem;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventTimetableMonitorAssignTime(tm, %p: ",
      (void *) meet);
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  HnAssert(0 <= assigned_time_index,
    "KheEventTimetableMonitorAssignTime internal error 1");
  durn = KheMeetDuration(meet);
  HnAssert(assigned_time_index + durn <= HaArrayCount(etm->time_cells),
    "KheEventTimetableMonitorAssignTime internal error 2 (%d + %d <= %d)",
    assigned_time_index, durn, HaArrayCount(etm->time_cells));
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(etm->time_cells, assigned_time_index + i);
    if( HaArrayCount(tc->meets) == 0 )
    {
      HaArrayForEach(etm->link_events_monitors, lem, j)
	KheLinkEventsMonitorAssignNonClash(lem, assigned_time_index + i);
    }
    if( DEBUG3 )
      fprintf(stderr, "KheEventTimetableMonitorAssignTime(%s, %d) at tc %d"
	" (%d meets before)\n", KheMeetId(meet), assigned_time_index,
	assigned_time_index + i, HaArrayCount(tc->meets));
    HaArrayAddLast(tc->meets, meet);
  }
  HaArrayForEach(etm->link_events_monitors, lem, i)
    KheLinkEventsMonitorFlush(lem);
}


/*****************************************************************************/
/*                                                                           */
/* void KheEventTimetableMonitorUnAssignTime(KHE_EVENT_TIMETABLE_MONITOR etm,*/
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Update etm to delete meet at assigned_time_index and on for its duration.*/
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorUnAssignTime(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_MEET meet, int assigned_time_index)
{
  int i, j, pos, durn;  KHE_TIME_CELL tc;  KHE_LINK_EVENTS_MONITOR lem;
  KHE_MEET m;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventTimetableMonitorUnAssignTime(tm, %p: ",
      (void *) meet);
    KheMeetDebug(meet, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  durn = KheMeetDuration(meet);
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(etm->time_cells, assigned_time_index + i);
    HaArrayForEachReverse(tc->meets, m, pos)
      if( m == meet )
	break;
    if( pos < 0 )
    {
      if( DEBUG5 )
      {
	fprintf(stderr, "KheEventTimetableMonitorUnAssignTime failing:\n");
        KheEventTimetableMonitorDebug(etm, 2, 2, stderr);
      }
      HnAbort("KheEventTimetableMonitorUnAssignTime internal error 1");
    }
    if( DEBUG3 )
      fprintf(stderr, "KheEventTimetableMonitorUnAssignTime(%s, %d) at tc %d"
	" (%d meets before)\n", KheMeetId(meet), assigned_time_index,
	assigned_time_index + i, HaArrayCount(tc->meets));
    HaArrayDeleteAndShift(tc->meets, pos);
    if( HaArrayCount(tc->meets) == 0 )
    {
      HaArrayForEach(etm->link_events_monitors, lem, j)
	KheLinkEventsMonitorUnAssignNonClash(lem, assigned_time_index + i);
    }
  }
  HaArrayForEach(etm->link_events_monitors, lem, i)
    KheLinkEventsMonitorFlush(lem);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_week_rec {
  KHE_TIME_GROUP	week_tg;		/* optional; defines week    */
  int			max_times;		/* on any one day            */
  ARRAY_KHE_TIME_GROUP	days;			/* days of the week          */
} *KHE_WEEK;

typedef HA_ARRAY(KHE_WEEK) ARRAY_KHE_WEEK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WEEK KheWeekMake(KHE_TIME_GROUP week_tg, HA_ARENA a)                 */
/*                                                                           */
/*  Make a new week object with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_WEEK KheWeekMake(KHE_TIME_GROUP week_tg, HA_ARENA a)
{
  KHE_WEEK res;
  HaMake(res, a);
  res->week_tg = week_tg;
  res->max_times = 0;
  HaArrayInit(res->days, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWeekDelete(KHE_WEEK week)                                        */
/*                                                                           */
/*  Delete week.                                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWeekDelete(KHE_WEEK week)
{
  MArrayFree(week->days);
  MFree(week);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePrint(char *str, int cell_width, FILE *fp)                       */
/*                                                                           */
/*  Print str onto fp, with a margin, taking care not to overrun.            */
/*                                                                           */
/*****************************************************************************/

static void KhePrint(char *str, bool in_cell, int cell_width, FILE *fp)
{
  char buff[100];
  snprintf(buff, cell_width - 3, "%s", str);
  fprintf(fp, "%c %-*s ", in_cell ? '|' : ' ', cell_width - 3, buff);
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePrintRule(int cell_width, FILE *fp)                              */
/*                                                                           */
/*  Print a rule of the given cell_width onto fp.                            */
/*                                                                           */
/*****************************************************************************/

static void KhePrintRule(int cell_width, FILE *fp)
{
  int i;
  fprintf(fp, "+");
  for( i = 0;  i < cell_width - 1;  i++ )
    fprintf(fp, "-");
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePrintRuleLine(int cells, int cell_width, int indent, FILE *fp)   */
/*                                                                           */
/*  Print a full-width rule, for this many cells of this width, onto fp      */
/*  with the given indent.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KhePrintRuleLine(int cells, int cell_width, int indent, FILE *fp)
{
  int i;
  fprintf(fp, "%*s", indent, "");
  for( i = 0;  i < cells;  i++ )
    KhePrintRule(cell_width, fp);
  fprintf(fp, "+\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePrintBlankLine(int cells, int cell_width, int indent, FILE *fp)  */
/*                                                                           */
/*  Print a full-width rule, for this many cells of this width, onto fp      */
/*  with the given indent.                                                   */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KhePrintBlankLine(int cells, int cell_width, int indent, FILE *fp)
{
  int i;
  fprintf(fp, "%*s", indent, "");
  for( i = 0;  i < cells;  i++ )
    KhePrint("", true, cell_width, fp);
  fprintf(fp, "|\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorPrintRow(KHE_EVENT_TIMETABLE_MONITOR etm,   */
/*    KHE_WEEK week, int time_index, int cell_width, int indent, FILE *fp)   */
/*                                                                           */
/*  Print one row of the timetable, the one for time time_index of week.     */
/*                                                                           */
/*****************************************************************************/

static void KheEventTimetableMonitorPrintRow(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_WEEK week, int time_index, int cell_width, int indent, FILE *fp)
{
  int i, j, content_lines;  KHE_TIME_GROUP day;  KHE_TIME t;  char *str;
  KHE_MEET meet;

  /* print the top line and the blank line just under it */
  KhePrintRuleLine(HaArrayCount(week->days), cell_width, indent, fp);
  /* KhePrintBlankLine(HaArrayCount(week->days), cell_width, indent, fp); */

  /* find the number of content lines to print */
  content_lines = 0;
  HaArrayForEach(week->days, day, i)
    if( time_index < KheTimeGroupTimeCount(day) )
    {
      t = KheTimeGroupTime(day, time_index);
      if( content_lines < KheEventTimetableMonitorTimeMeetCount(etm, t) )
	content_lines = KheEventTimetableMonitorTimeMeetCount(etm, t);
    }

  /* if there are any content lines, print them plus a blank line */
  if( content_lines > 0 )
  {
    for( j = 0;  j < content_lines;  j++ )
    {
      fprintf(fp, "%*s", indent, "");
      HaArrayForEach(week->days, day, i)
      {
	/* print something for this day/j, even if just a blank line */
	if( time_index >= KheTimeGroupTimeCount(day) )
	  str = "";
	else
	{
	  t = KheTimeGroupTime(day, time_index);
	  if( j >= KheEventTimetableMonitorTimeMeetCount(etm, t) )
	    str = "";
	  else
	  {
	    meet = KheEventTimetableMonitorTimeMeet(etm, t, j);
	    str = KheMeetEvent(meet) == NULL ? "?" :
	      KheEventId(KheMeetEvent(meet)) == NULL ? "-" :
	      KheEventId(KheMeetEvent(meet));
	  }
	}
	KhePrint(str, true, cell_width, fp);
      }
      fprintf(fp, "|\n");
    }
    /* KhePrintBlankLine(HaArrayCount(week->days), cell_width, indent, fp); */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorPrintTimetable(                             */
/*    KHE_EVENT_TIMETABLE_MONITOR etm,                                       */
/*    int cell_width, int indent, FILE *fp)                                  */
/*                                                                           */
/*  Print etm onto fp with the given indent, in a format that looks like     */
/*  an actual timetable (there must be Days, at least, in the instance).     */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorPrintTimetable(KHE_EVENT_TIMETABLE_MONITOR etm,
  int cell_width, int indent, FILE *fp)
{
  ARRAY_KHE_WEEK weeks;  KHE_WEEK week;  KHE_INSTANCE ins;  int i, j, count;
  KHE_TIME_GROUP tg;  HA_ARENA a;

  /* find the weeks; make just one if there are no weeks */
  ins = KheSolnInstance(etm->soln);
  a = KheSolnArenaBegin(etm->soln);
  HaArrayInit(weeks, a);
  for( i = 0;  i < KheInstanceTimeGroupCount(ins);  i++ )
  {
    tg = KheInstanceTimeGroup(ins, i);
    if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_WEEK )
      HaArrayAddLast(weeks, KheWeekMake(tg, a));
  }
  if( HaArrayCount(weeks) == 0 )
    HaArrayAddLast(weeks, KheWeekMake(NULL, a));

  /* add the days to the weeks */
  for( i = 0;  i < KheInstanceTimeGroupCount(ins);  i++ )
  {
    tg = KheInstanceTimeGroup(ins, i);
    if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_DAY )
    {
      count = 0;
      HaArrayForEach(weeks, week, j)
	if( week->week_tg == NULL || KheTimeGroupSubset(tg, week->week_tg) )
	{
	  HaArrayAddLast(week->days, tg);
	  count++;
	  if( week->max_times < KheTimeGroupTimeCount(tg) )
	    week->max_times = KheTimeGroupTimeCount(tg);
	}
      if( count != 1 )
      {
	fprintf(fp, "%*sKheEventTimetableMonitorPrintTimetable: day ",
	  indent, "");
	KheTimeGroupDebug(tg, 1, -1, fp);
	fprintf(fp, " lies in %d weeks\n", count);
      }
    }
  }

  /* print the timetable, week by week */
  HaArrayForEach(weeks, week, i)
  {
    /* blank line between weeks */
    if( i > 0 )
      fprintf(fp, "\n");

    /* header line containing the names of the days */
    fprintf(fp, "%*s", indent, "");
    HaArrayForEach(week->days, tg, j)
      KhePrint(KheTimeGroupId(tg) == NULL ? "-" : KheTimeGroupId(tg),
	false, cell_width, fp);
    fprintf(fp, "\n");

    /* one row for each time of the day */
    for( j = 0;  j < week->max_times;  j++ )
      KheEventTimetableMonitorPrintRow(etm, week, j, cell_width, indent, fp);

    /* and a finishing rule */
    KhePrintRuleLine(HaArrayCount(week->days), cell_width, indent, fp);
  }

  /* delete the memory used */
  KheSolnArenaEnd(etm->soln, a);
  /* ***
  while( HaArrayCount(weeks) > 0 )
    KheWeekDelete(MArrayRemoveLast(weeks));
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventTimetableMonitorDebug(KHE_EVENT_TIMETABLE_MONITOR etm,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of timetable etm onto fp with this verbosity and indent.     */
/*                                                                           */
/*****************************************************************************/

void KheEventTimetableMonitorDebug(KHE_EVENT_TIMETABLE_MONITOR etm,
  int verbosity, int indent, FILE *fp)
{
  KHE_TIME_CELL tc;  KHE_LINK_EVENTS_MONITOR lem;  int i;
  KHE_EVENT_IN_SOLN es;
  if( indent >= 0 && verbosity >= 1 )
  {
    KheMonitorDebugBegin((KHE_MONITOR) etm, indent, fp);
    HaArrayForEach(etm->events_in_soln, es, i)
    {
      if( i > 10 )
      {
	fprintf(fp, ", ...");
	break;
      }
      else
	fprintf(fp, "%s %s", i > 0 ? "," : "", KheEventInSolnId(es));
    }
    fprintf(fp, "\n");
    /* fprintf(fp, " %s\n", KheEventInSolnId(etm->event_in_soln)); */
    HaArrayForEach(etm->link_events_monitors, lem, i)
      KheLinkEventsMonitorDebug(lem, 1, indent + 2, fp);
    HaArrayForEach(etm->time_cells, tc, i)
      if( verbosity >= 3 || HaArrayCount(tc->meets) > 0 )
	KheTimeCellDebug(tc, verbosity, indent + 2, fp);
    KheMonitorDebugEnd((KHE_MONITOR) etm, false, indent, fp);
  }
}

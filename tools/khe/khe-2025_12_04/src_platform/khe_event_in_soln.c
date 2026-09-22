
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
/*  FILE:         khe_event_in_soln.c                                        */
/*  DESCRIPTION:  A soln object that monitors everything about one event     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN                                                        */
/*                                                                           */
/*****************************************************************************/

struct khe_event_in_soln_rec {
  KHE_SOLN			soln;			/* encl solution     */
  KHE_EVENT			event;			/* original event    */
  ARRAY_KHE_MEET		meets;			/* its meets         */
  ARRAY_KHE_EVENT_RESOURCE_IN_SOLN event_resources_in_soln;
  KHE_EVENT_TIMETABLE_MONITOR	timetable_monitor;	/* timetable         */
  ARRAY_KHE_MONITOR		all_monitors;		/* all monitors      */
  ARRAY_KHE_MONITOR		attached_monitors;	/* attached monitors */
  ARRAY_KHE_MONITOR	attached_time_asst_monitors;	/* time asst mon's   */
  KHE_EVENT_IN_SOLN		copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnMeetsDebug(KHE_EVENT_IN_SOLN es, FILE *fp)            */
/*                                                                           */
/*  Debug print of es and its meets.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheEventInSolnMeetsDebug(KHE_EVENT_IN_SOLN es, FILE *fp)
{
  int i;  KHE_MEET m;
  fprintf(fp, "%d meets[", HaArrayCount(es->meets));
  HaArrayForEach(es->meets, m, i)
    fprintf(fp, "%s%s%p", i == 0 ? "" : ", ", KheMeetId(m), (void *) m);
  fprintf(fp, "]");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN KheEventInSolnMake(KHE_SOLN soln, KHE_EVENT e)         */
/*                                                                           */
/*  Make a new event monitor for soln, corresponding to e, initially with    */
/*  no meets, but with one event_resource_in_soln object for each event      */
/*  resource of e.                                                           */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_IN_SOLN KheEventInSolnMake(KHE_SOLN soln, KHE_EVENT e
  /* , bool with_monitors */)
{
  KHE_EVENT_IN_SOLN res;  int i;  KHE_EVENT_RESOURCE_IN_SOLN ers;  HA_ARENA a;
  a = KheSolnArena(soln);
  HaMake(res, a);
  HnAssert(e != NULL, "KheEventInSolnMake: internal error (e is NULL)");
  res->soln = soln;
  res->event = e;
  HaArrayInit(res->meets, a);
  HaArrayInit(res->event_resources_in_soln, a);
  for( i = 0;  i < KheEventResourceCount(e);  i++ )
  {
    ers = KheEventResourceInSolnMake(res, KheEventResource(e, i));
    HaArrayAddLast(res->event_resources_in_soln, ers);
  }
  res->timetable_monitor = NULL;  /* done immediately after, see below */
  /* ***
  res->timetable_monitor = (with_monitors ?
    KheTimetableMonitorMake(soln, NULL, res) : NULL);
  *** */
  HaArrayInit(res->all_monitors, a);
  HaArrayInit(res->attached_monitors, a);
  HaArrayInit(res->attached_time_asst_monitors, a);
  res->copy = NULL;
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventInSolnMake(%p, %s %p) = ", (void *) soln,
      KheEventId(e), (void *) e);
    KheEventInSolnMeetsDebug(res, stderr);
    fprintf(stderr, "\n");
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnAddTimetableMonitor(KHE_EVENT_IN_SOLN es)             */
/*                                                                           */
/*  Add a timetable monitor to es.  This is called immediately after         */
/*  KheEventInSolnMake, but it can't be folded into it because of            */
/*  sequencing problems.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnAddTimetableMonitor(KHE_EVENT_IN_SOLN es)
{
  es->timetable_monitor = KheEventTimetableMonitorMake(es->soln,
    KheEventSingletonEventGroup(es->event));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnTotalDuration(KHE_EVENT_IN_SOLN es)                    */
/*                                                                           */
/*  Return the total duration of the meets of es.                            */
/*                                                                           */
/*****************************************************************************/

static int KheEventInSolnTotalDuration(KHE_EVENT_IN_SOLN es)
{
  int res, i;  KHE_MEET meet;
  res = 0;
  HaArrayForEach(es->meets, meet, i)
    res += KheMeetDuration(meet);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheEventInSolnMakeCompleteRepresentation(KHE_EVENT_IN_SOLN es,      */
/*    KHE_EVENT *problem_event)                                              */
/*                                                                           */
/*  Carry out the specification of KheSolnMakeCompleteRepresentation on es.  */
/*                                                                           */
/*****************************************************************************/

bool KheEventInSolnMakeCompleteRepresentation(KHE_EVENT_IN_SOLN es,
  KHE_EVENT *problem_event)
{
  int j, k, total_durn;  KHE_MEET meet;  KHE_TASK task;
  KHE_EVENT_RESOURCE er;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheEventInSolnMakeCompleteRepresentation(%s)\n",
      KheEventId(es->event) != NULL ? KheEventId(es->event) : "-");
    fprintf(stderr, "  event: duration %d; es: %d sub-events, %d duration\n",
      KheEventDuration(es->event), HaArrayCount(es->meets),
      KheEventInSolnTotalDuration(es));
    KheEventInSolnDebug(es, 2, 2, stderr);
  }

  /* make sure es has the correct total duration */
  total_durn = KheEventInSolnTotalDuration(es);
  if( total_durn > KheEventDuration(es->event) )
  {
    *problem_event = es->event;
    return false;
  }
  else if( total_durn < KheEventDuration(es->event) )
    KheMeetMake(es->soln, KheEventDuration(es->event) - total_durn, es->event);

  /* make sure each meet has the required tasks */
  HaArrayForEach(es->meets, meet, j)
  {
    HnAssert(KheMeetSoln(meet) == es->soln,
      "KheEventInSolnMakeCompleteRepresentation internal error (%p != %p)",
      (void *) KheMeetSoln(meet), (void *) es->soln);
    for( k = 0;  k < KheEventResourceCount(es->event);  k++ )
    {
      er = KheEventResource(es->event, k);
      if( !KheMeetFindTask(meet, er, &task) )
      {
	/* must add a task corresponding to er */
	task = KheTaskMake(KheMeetSoln(meet),
	  KheEventResourceResourceType(er), meet, er);
      }
    }
  }
  if( DEBUG2 )
  {
    fprintf(stderr, "] KheEventInSolnMakeCompleteRepresentation ret. true\n");
    KheEventInSolnDebug(es, 2, 2, stderr);
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheEventInSolnSoln(KHE_EVENT_IN_SOLN es)                        */
/*                                                                           */
/*  Return the soln attribute of es.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheEventInSolnSoln(KHE_EVENT_IN_SOLN es)
{
  return es->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT KheEventInSolnEvent(KHE_EVENT_IN_SOLN es)                      */
/*                                                                           */
/*  Return the event attribute of es.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT KheEventInSolnEvent(KHE_EVENT_IN_SOLN es)
{
  return es->event;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheEventInSolnId(KHE_EVENT_IN_SOLN es)                             */
/*                                                                           */
/*  Return the Id of the event that es is for, or "-" if none.               */
/*                                                                           */
/*****************************************************************************/

char *KheEventInSolnId(KHE_EVENT_IN_SOLN es)
{
  return KheEventId(es->event) == NULL ? "-" : KheEventId(es->event);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnDelete(KHE_EVENT_IN_SOLN es)                          */
/*                                                                           */
/*  Delete es and everything below it.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheEventInSolnDelete(KHE_EVENT_IN_SOLN es)
{
  KHE_EVENT_RESOURCE_IN_SOLN ers;  int i;

  ** meets should be already deleted **
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventInSolnDelete(");
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, ")\n");
  }
  HnAssert(HaArrayCount(es->meets) == 0,
    "KheEventInSolnDelete internal error");
  MArrayFree(es->meets);

  ** delete event resource in soln objects **
  HaArrayForEach(es->event_resources_in_soln, ers, i)
    KheEventResourceInSolnDelete(ers);

  ** timetables and other monitors deleted separately; but free arrays now **
  MArrayFree(es->all_monitors);
  MArrayFree(es->attached_monitors);
  MArrayFree(es->attached_time_asst_monitors);

  MFree(es);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_IN_SOLN KheEventInSolnCopyPhase1(KHE_EVENT_IN_SOLN es,         */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Carry out Phase 1 of the operation of copying es.                        */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_IN_SOLN KheEventInSolnCopyPhase1(KHE_EVENT_IN_SOLN es, HA_ARENA a)
{
  KHE_EVENT_IN_SOLN copy;  KHE_MEET meet;  int i;
  KHE_EVENT_RESOURCE_IN_SOLN ers;  KHE_MONITOR m;
  if( es->copy == NULL )
  {
    HaMake(copy, a);
    es->copy = copy;
    copy->soln = KheSolnCopyPhase1(es->soln);
    copy->event = es->event;
    HaArrayInit(copy->meets, a);
    HaArrayForEach(es->meets, meet, i)
      HaArrayAddLast(copy->meets, KheMeetCopyPhase1(meet, a));
    HaArrayInit(copy->event_resources_in_soln, a);
    HaArrayForEach(es->event_resources_in_soln, ers, i)
      HaArrayAddLast(copy->event_resources_in_soln,
	KheEventResourceInSolnCopyPhase1(ers, a));
    copy->timetable_monitor = (es->timetable_monitor == NULL ? NULL :
      KheEventTimetableMonitorCopyPhase1(es->timetable_monitor, a));
    HaArrayInit(copy->all_monitors, a);
    HaArrayForEach(es->all_monitors, m, i)
      HaArrayAddLast(copy->all_monitors, KheMonitorCopyPhase1(m, a));
    HaArrayInit(copy->attached_monitors, a);
    HaArrayForEach(es->attached_monitors, m, i)
      HaArrayAddLast(copy->attached_monitors, KheMonitorCopyPhase1(m, a));
    HaArrayInit(copy->attached_time_asst_monitors, a);
    HaArrayForEach(es->attached_time_asst_monitors, m, i)
      HaArrayAddLast(copy->attached_time_asst_monitors,
	KheMonitorCopyPhase1(m, a));
    copy->copy = NULL;
    if( DEBUG4 )
    {
      fprintf(stderr, "KheEventInSolnCopyPhase1(");
      KheEventInSolnMeetsDebug(es, stderr);
      fprintf(stderr, ") = \n");
      KheEventInSolnMeetsDebug(copy, stderr);
      fprintf(stderr, ") = ");
    }
  }
  return es->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnCopyPhase2(KHE_EVENT_IN_SOLN es)                      */
/*                                                                           */
/*  Carry out Phase 2 of the operation of copying es.                        */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnCopyPhase2(KHE_EVENT_IN_SOLN es)
{
  int i;  KHE_EVENT_RESOURCE_IN_SOLN ers;  KHE_MONITOR m;  KHE_MEET meet;
  if( es->copy != NULL )
  {
    es->copy = NULL;
    HaArrayForEach(es->meets, meet, i)
      KheMeetCopyPhase2(meet);
    HaArrayForEach(es->event_resources_in_soln, ers, i)
      KheEventResourceInSolnCopyPhase2(ers);
    if( es->timetable_monitor != NULL )
      KheEventTimetableMonitorCopyPhase2(es->timetable_monitor);
    HaArrayForEach(es->all_monitors, m, i)
      KheMonitorCopyPhase2(m);
    HaArrayForEach(es->attached_monitors, m, i)
      KheMonitorCopyPhase2(m);
    HaArrayForEach(es->attached_time_asst_monitors, m, i)
      KheMonitorCopyPhase2(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "meets"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnAddMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet)          */
/*                                                                           */
/*  Add meet to es.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnAddMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet)
{
  int i;  KHE_MONITOR m;
  if( DEBUG1 )
    fprintf(stderr, "[ KheEventInSolnAddMeet(es, \"%s\" d%d, meet d%d\n",
      KheEventId(es->event), KheEventDuration(es->event),
      KheMeetDuration(meet));
  HnAssert(KheMeetDuration(meet) > 0,
    "KheEventInSolnAddMeet internal error");
  HaArrayAddLast(es->meets, meet);
  HaArrayForEach(es->attached_monitors, m, i)
    KheMonitorAddMeet(m, meet);
  if( DEBUG4 )
  {
    fprintf(stderr, "KheEventInSolnAddMeet(%p, %p) = ",
      (void *) es, (void *) meet);
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, "\n");
  }
  if( DEBUG1 )
  {
    KheEventInSolnDebug(es, 2, 2, stderr);
    fprintf(stderr, "] KheEventInSolnAddMeet returning\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnDeleteMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet)       */
/*                                                                           */
/*  Delete meet from es.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnDeleteMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet)
{
  KHE_MEET x;  int i, pos;  KHE_MONITOR m;
  if( !HaArrayContains(es->meets, meet, &pos) )
    HnAbort("KheEventInSolnDeleteMeet:  meet absent (already deleted?)");
  if( DEBUG4 )
  {
    fprintf(stderr, "[ KheEventInSolnDeleteMeet(");
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, ", %p)\n", (void *) meet);
  }
  HaArrayForEach(es->attached_monitors, m, i)
    KheMonitorDeleteMeet(m, meet);
  x = HaArrayLastAndDelete(es->meets);
  if( x != meet )
    HaArrayPut(es->meets, pos, x);
  if( DEBUG4 )
  {
    fprintf(stderr, "] KheEventInSolnDeleteMeet returning, es = ");
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnSplitMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet1,       */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Inform es that its meet meet1 is splitting into meet1 and meet2.  The    */
/*  new durations are already installed.                                     */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnSplitMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet1,
  KHE_MEET meet2)
{
  int i;  KHE_MONITOR m;
  HnAssert(KheMeetDuration(meet1) > 0,
    "KheEventInSolnSplitMeet internal error 1");
  HnAssert(KheMeetDuration(meet2) > 0,
    "KheEventInSolnSplitMeet internal error 2");
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheEventInSolnSplitMeet(%s ", KheEventId(es->event));
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, ", %s %p, %s %p)\n",
      KheMeetId(meet1), (void *) meet1, KheMeetId(meet2), (void *) meet2);
  }
  HaArrayAddLast(es->meets, meet2);
  HaArrayForEach(es->attached_monitors, m, i)
    KheMonitorSplitMeet(m, meet1, meet2);
  if( DEBUG6 )
  {
    fprintf(stderr, "] KheEventInSolnSplitMeet returning, es = ");
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnMergeMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet1,       */
/*    KHE_MEET meet2)                                                        */
/*                                                                           */
/*  Inform es that its meets meet1 and meet2 are merging.  They currently    */
/*  have their pre-merging durations; meet2 is the one going.                */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnMergeMeet(KHE_EVENT_IN_SOLN es, KHE_MEET meet1,
  KHE_MEET meet2)
{
  KHE_MEET x;  int i;  KHE_MONITOR m;
  if( DEBUG6 )
  {
    fprintf(stderr, "[ KheEventInSolnMergeMeet(%s ", KheEventId(es->event));
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, ", %s %p, %s %p)\n",
      KheMeetId(meet1), (void *) meet1, KheMeetId(meet2), (void *) meet2);
  }
  HaArrayForEach(es->attached_monitors, m, i)
    KheMonitorMergeMeet(m, meet1, meet2);
  x = HaArrayLastAndDelete(es->meets);
  if( x != meet2 )
  {
    if( !HaArrayContains(es->meets, meet2, &i) )
      HnAbort("KheEventInSolnMergeMeet internal error");
    HaArrayPut(es->meets, i, x);
  }
  if( DEBUG6 )
  {
    fprintf(stderr, "] KheEventInSolnMergeMeet returning, es = ");
    KheEventInSolnMeetsDebug(es, stderr);
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnAssignTime(KHE_EVENT_IN_SOLN es,                      */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Inform es that meet has been assigned this time.                         */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnAssignTime(KHE_EVENT_IN_SOLN es,
  KHE_MEET meet, int assigned_time_index)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(es->attached_time_asst_monitors, m, i)
  {
    if( DEBUG5 )
      fprintf(stderr, "  KheEventInSolnAssignTime(es, %s, %d) calling "
	"KheMonitorAssignTime\n", KheMeetId(meet), assigned_time_index);
    KheMonitorAssignTime(m, meet, assigned_time_index);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnUnAssignTime(KHE_EVENT_IN_SOLN es,                    */
/*    KHE_MEET meet, int assigned_time_index)                                */
/*                                                                           */
/*  Inform es that meet has been unassigned this time.                       */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnUnAssignTime(KHE_EVENT_IN_SOLN es,
  KHE_MEET meet, int assigned_time_index)
{
  KHE_MONITOR m;  int i;
  if( DEBUG3 )
    fprintf(stderr, "[ KheEventInSolnUnAssignTime(es, meet, %d)\n",
      assigned_time_index);
  HaArrayForEach(es->attached_time_asst_monitors, m, i)
    KheMonitorUnAssignTime(m, meet, assigned_time_index);
  if( DEBUG3 )
    fprintf(stderr, "] KheEventInSolnUnAssignTime returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnMeetCount(KHE_EVENT_IN_SOLN es)                        */
/*                                                                           */
/*  Return the number of meets in es.                                        */
/*                                                                           */
/*****************************************************************************/

int KheEventInSolnMeetCount(KHE_EVENT_IN_SOLN es)
{
  return HaArrayCount(es->meets);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MEET KheEventInSolnMeet(KHE_EVENT_IN_SOLN es, int i)                 */
/*                                                                           */
/*  Return the i'th meet of es.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_MEET KheEventInSolnMeet(KHE_EVENT_IN_SOLN es, int i)
{
  return HaArray(es->meets, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnMinTimeIndex(KHE_EVENT_IN_SOLN es)                     */
/*                                                                           */
/*  Return the minimum, over the meets of es, of the assigned time index.    */
/*                                                                           */
/*  This function assumes that es has at least one meet, and that every      */
/*  meet of es has an assigned time index.  Thus, the result is always       */
/*  well defined.                                                            */
/*                                                                           */
/*****************************************************************************/

int KheEventInSolnMinTimeIndex(KHE_EVENT_IN_SOLN es)
{
  KHE_MEET meet;  int i, ti, res;
  res = INT_MAX;
  HaArrayForEach(es->meets, meet, i)
  {
    ti = KheMeetAssignedTimeIndex(meet);
    if( ti < res )
      res = ti;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnMaxTimeIndexPlusDuration(KHE_EVENT_IN_SOLN es)         */
/*                                                                           */
/*  Return the maximum, over the meets of es, of the assigned time index     */
/*  plus the duration.                                                       */
/*                                                                           */
/*  This function assumes that es has at least one meet, and that every      */
/*  meet of es has an assigned time index.  Thus, the result is always       */
/*  well defined.                                                            */
/*                                                                           */
/*****************************************************************************/

int KheEventInSolnMaxTimeIndexPlusDuration(KHE_EVENT_IN_SOLN es)
{
  KHE_MEET meet;  int i, tid, res;
  res = 0;
  HaArrayForEach(es->meets, meet, i)
  {
    tid = KheMeetAssignedTimeIndex(meet) + KheMeetDuration(meet);
    if( tid > res )
      res = tid;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "fix and unfix"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventInSolnAllMeetsAssignFixed(KHE_EVENT_IN_SOLN es)             */
/*                                                                           */
/*  Return true if every meet in es is assign-fixed.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheEventInSolnAllMeetsAssignFixed(KHE_EVENT_IN_SOLN es)
{
  KHE_MEET meet;  int i;
  HaArrayForEach(es->meets, meet, i)
    if( !KheMeetAssignIsFixed(meet) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnMeetAssignFix(KHE_EVENT_IN_SOLN es)                   */
/*                                                                           */
/*  A meet of es has just changed from assign-unfixed to assign-fixed; or,   */
/*  more generally, its fixed assignment path is longer than it was.  If     */
/*  all meets are assign-fixed, report this to all attached link events and  */
/*  order events monitors of cost 0; they might choose to detach themselves. */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnMeetAssignFix(KHE_EVENT_IN_SOLN es)
{
  KHE_MONITOR m;  int i;
  if( KheEventInSolnAllMeetsAssignFixed(es) )
    HaArrayForEach(es->all_monitors, m, i)
      if( KheMonitorAttachedToSoln(m) && KheMonitorCost(m) == 0 )
	switch( KheMonitorTag(m) )
	{
	  case KHE_LINK_EVENTS_MONITOR_TAG:

	    KheLinkEventsMonitorMeetAssignFix((KHE_LINK_EVENTS_MONITOR) m);
	    break;

	  case KHE_ORDER_EVENTS_MONITOR_TAG:

	    KheOrderEventsMonitorMeetAssignFix((KHE_ORDER_EVENTS_MONITOR) m);
	    break;

	  default:

	    /* not interested */
	    break;
	}
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnMeetAssignUnFix(KHE_EVENT_IN_SOLN es)                 */
/*                                                                           */
/*  A meet of es has just changed from assign-fixed to assign-unfixed; or,   */
/*  more generally, its fixed assignment path is shorter than it was.        */
/*  Report this to all unattached link events and order events monitors;     */
/*  they might then choose to attach themselves.                             */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnMeetAssignUnFix(KHE_EVENT_IN_SOLN es)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(es->all_monitors, m, i)
    if( !KheMonitorAttachedToSoln(m) )
      switch( KheMonitorTag(m) )
      {
	case KHE_LINK_EVENTS_MONITOR_TAG:

	  KheLinkEventsMonitorMeetAssignUnFix((KHE_LINK_EVENTS_MONITOR) m);
	  break;

	case KHE_ORDER_EVENTS_MONITOR_TAG:

	  KheOrderEventsMonitorMeetAssignUnFix((KHE_ORDER_EVENTS_MONITOR) m);
	  break;

	default:

	  /* not interested */
	  break;
      }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "event resources in soln"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnEventResourceInSolnCount(KHE_EVENT_IN_SOLN es)         */
/*                                                                           */
/*  Return the number of event resources in soln containing in es.  This     */
/*  will be the same as the number of event resources in es's event.         */
/*                                                                           */
/*****************************************************************************/

int KheEventInSolnEventResourceInSolnCount(KHE_EVENT_IN_SOLN es)
{
  return HaArrayCount(es->event_resources_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_RESOURCE_IN_SOLN KheEventInSolnEventResourceInSoln(            */
/*    KHE_EVENT_IN_SOLN es, int i)                                           */
/*                                                                           */
/*  Return the i'th event resource in soln of es.                            */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_RESOURCE_IN_SOLN KheEventInSolnEventResourceInSoln(
  KHE_EVENT_IN_SOLN es, int i)
{
  return HaArray(es->event_resources_in_soln, i);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitors"                                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnAttachMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)    */
/*                                                                           */
/*  Attach m to es.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnAttachMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)
{
  KHE_MEET meet;  int i;
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:

      /* first add to es's time asst monitor list */
      HaArrayAddLast(es->attached_time_asst_monitors, m);
      /* NB NO BREAK */

    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      /* attach directly to es */
      HaArrayAddLast(es->attached_monitors, m);
      HaArrayForEach(es->meets, meet, i)
	KheMonitorAddMeet(m, meet);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      /* attach to timetable, which much itself be attached first */
      KheEventTimetableMonitorAttachLinkEventsMonitor(es->timetable_monitor,
	(KHE_LINK_EVENTS_MONITOR) m);
      /* ***
      if( !KheMonitorAttachedToSoln((KHE_MONITOR) es->timetable_monitor) )
        KheMonitorAttachToSoln((KHE_MONITOR) es->timetable_monitor);
      KheTimetableMonitorAttachMonitor(es->timetable_monitor, m);
      *** */
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    default:

      /* unexpected monitor type */
      HnAbort("KheEventInSolnAttachMonitor internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnDetachMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)    */
/*                                                                           */
/*  Detach m from es.                                                        */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnDetachMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)
{
  KHE_MEET meet;  int i, pos;
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_TIME_MONITOR_TAG:
    case KHE_PREFER_TIMES_MONITOR_TAG:
    case KHE_EVENT_TIMETABLE_MONITOR_TAG:
    /* case KHE_TIMETABLE_MONITOR_TAG: */
    case KHE_SPREAD_EVENTS_MONITOR_TAG:
    case KHE_ORDER_EVENTS_MONITOR_TAG:

      /* first remove from es's time asst monitor list */
      if( !HaArrayContains(es->attached_time_asst_monitors, m, &pos) )
	HnAbort("KheEventInSolnDetachMonitor internal error 1");
      HaArrayDeleteAndShift(es->attached_time_asst_monitors, pos);
      /* NB NO BREAK */

    case KHE_SPLIT_EVENTS_MONITOR_TAG:
    case KHE_DISTRIBUTE_SPLIT_EVENTS_MONITOR_TAG:

      /* detach directly from es */
      HaArrayForEach(es->meets, meet, i)
	KheMonitorDeleteMeet(m, meet);
      if( !HaArrayContains(es->attached_monitors, m, &pos) )
	HnAbort("KheEventInSolnDetachMonitor internal error 2");
      HaArrayDeleteAndShift(es->attached_monitors, pos);
      break;

    case KHE_LINK_EVENTS_MONITOR_TAG:

      /* detach from timetable */
      KheEventTimetableMonitorDetachLinkEventsMonitor(es->timetable_monitor,
	(KHE_LINK_EVENTS_MONITOR) m);
      /* KheTimetableMonitorDetachMonitor(es->timetable_monitor, m); */
      break;

    case KHE_AVOID_CLASHES_MONITOR_TAG:
    case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:
    case KHE_LIMIT_IDLE_TIMES_MONITOR_TAG:
    case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:
    case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:
    case KHE_PREFER_RESOURCES_MONITOR_TAG:
    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:
    case KHE_LIMIT_WORKLOAD_MONITOR_TAG:
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:
    case KHE_ORDINARY_DEMAND_MONITOR_TAG:
    case KHE_WORKLOAD_DEMAND_MONITOR_TAG:
    default:

      /* unexpected monitor type */
      HnAbort("KheEventInSolnDetachMonitor internal error");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "user monitors and cost"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnAddMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)       */
/*                                                                           */
/*  Add m to the user-accessible list of monitors of es.                     */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnAddMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)
{
  HaArrayAddLast(es->all_monitors, m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnDeleteMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)    */
/*                                                                           */
/*  Delete m from the user-accessible list of monitors of es.                */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnDeleteMonitor(KHE_EVENT_IN_SOLN es, KHE_MONITOR m)
{
  int pos;
  if( !HaArrayContains(es->all_monitors, m, &pos) )
    HnAbort("KheEventInSolnDeleteMonitor internal error");
  HaArrayDeleteAndShift(es->all_monitors, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheEventInSolnMonitorCount(KHE_EVENT_IN_SOLN es)                     */
/*                                                                           */
/*  Return the number of user-accessible monitors in es.                     */
/*                                                                           */
/*****************************************************************************/

int KheEventInSolnMonitorCount(KHE_EVENT_IN_SOLN es)
{
  return HaArrayCount(es->all_monitors);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR KheEventInSolnMonitor(KHE_EVENT_IN_SOLN es, int i)           */
/*                                                                           */
/*  Return the i'th user-accessible monitor of es.                           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITOR KheEventInSolnMonitor(KHE_EVENT_IN_SOLN es, int i)
{
  return HaArray(es->all_monitors, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEventInSolnCost(KHE_EVENT_IN_SOLN es)                        */
/*                                                                           */
/*  Return the total cost of all monitors.                                   */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEventInSolnCost(KHE_EVENT_IN_SOLN es)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(es->all_monitors, m, i)
    res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheEventInSolnMonitorCost(KHE_EVENT_IN_SOLN es,                 */
/*    KHE_MONITOR_TAG tag)                                                   */
/*                                                                           */
/*  Return the total cost of monitors with this tag.                         */
/*                                                                           */
/*****************************************************************************/

KHE_COST KheEventInSolnMonitorCost(KHE_EVENT_IN_SOLN es, KHE_MONITOR_TAG tag)
{
  KHE_MONITOR m;  int i;  KHE_COST res;
  res = 0;
  HaArrayForEach(es->all_monitors, m, i)
    if( KheMonitorTag(m) == tag )
      res += KheMonitorCost(m);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_EVENT_TIMETABLE_MONITOR KheEventInSolnTimetableMonitor(              */
/*    KHE_EVENT_IN_SOLN es)                                                  */
/*                                                                           */
/*  Return the timetable of es.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_EVENT_TIMETABLE_MONITOR KheEventInSolnTimetableMonitor(KHE_EVENT_IN_SOLN es)
{
  return es->timetable_monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reading and writing"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnWrite(KHE_EVENT_IN_SOLN es, KML_FILE kf,              */
/*    bool *event_written)                                                   */
/*                                                                           */
/*  Write the meets of es to kf.  Set *event_written to true if at least     */
/*  one event is written.                                                    */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnWrite(KHE_EVENT_IN_SOLN es, KML_FILE kf, bool *event_written)
{
  KHE_MEET meet;  int i;
  HaArraySort(es->meets, &KheMeetAssignedTimeCmp);
  HaArrayForEach(es->meets, meet, i)
  {
    KheMeetCheckForWriting(meet);
    if( KheMeetMustWrite(meet) )
    {
      if( !*event_written )
      {
	KmlBegin(kf, "Events");
	*event_written = true;
      }
      KheMeetWrite(meet, kf);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheEventInSolnDebug(KHE_EVENT_IN_SOLN es, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of es onto fp with the given verbosity and indent,           */
/*  including its constraints.                                               */
/*                                                                           */
/*****************************************************************************/

void KheEventInSolnDebug(KHE_EVENT_IN_SOLN es, int verbosity,
  int indent, FILE *fp)
{
  int i;  KHE_MEET meet;  KHE_EVENT_RESOURCE_IN_SOLN ers;
  KHE_MONITOR m;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
    {
      fprintf(fp, "%*s[ Event Monitor \"%s\"\n", indent, "",
	KheEventId(es->event) != NULL ? KheEventId(es->event) : "-");
      HaArrayForEach(es->meets, meet, i)
	fprintf(fp, "%*s  meet dur %d %p\n", indent, "",
	  KheMeetDuration(meet), (void *) meet);
      HaArrayForEach(es->event_resources_in_soln, ers, i)
	KheEventResourceInSolnDebug(ers, verbosity, indent + 2, fp);
      HaArrayForEach(es->attached_monitors, m, i)
	KheMonitorDebug(m, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
    {
      fprintf(fp, "\"%s\"", KheEventId(es->event) != NULL ?
	KheEventId(es->event) : "-");
    }
  }
}

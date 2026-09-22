
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
/*  it under the terms of the GNU Generar Public License as published by     */
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
/*  FILE:         khe_sr_time_sweep.c                                        */
/*  DESCRIPTION:  Time sweep resource assignment                             */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#define min(a, b) ((a) < (b) ? (a) : (b))

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0	/* print timetables for DEBUG7_ID   */
#define DEBUG7_ID "Nurse3"
#define DEBUG8 0  /* clash checking */
#define DEBUG9 0
#define DEBUG10 0
#define DEBUG11 0

/* ***
typedef HA_ARRAY(KHE_RESOURCE_MATCHING_DEMAND_SET)
  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET;
*** */

/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_FRAME                                                        */
/*                                                                           */
/*  A monitor frame is a partition of the monitors which need cutoffs,       */
/*  so that all the monitors whose initial cutoff time lies in a given       */
/*  part of the frame are grouped together under the index of that part.     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_monitor_frame_part_rec {
  ARRAY_KHE_MONITOR	resource_monitors;
  ARRAY_KHE_MONITOR	task_monitors;
} *KHE_MONITOR_FRAME_PART;

typedef HA_ARRAY(KHE_MONITOR_FRAME_PART) ARRAY_KHE_MONITOR_FRAME_PART;

typedef struct khe_monitor_frame_rec {
  KHE_FRAME			frame;
  ARRAY_KHE_MONITOR		active_resource_monitors;
  ARRAY_KHE_MONITOR_FRAME_PART	parts;
} *KHE_MONITOR_FRAME;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "checks"                                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheTimeGroupAssignments(KHE_EVENT_TIMETABLE_MONITOR etm,             */
/*    KHE_TIME_GROUP tg)                                                     */
/*                                                                           */
/*  Return the number of assigned tasks running at the times of tg.          */
/*                                                                           */
/*****************************************************************************/

static int KheTimeGroupAssignments(KHE_EVENT_TIMETABLE_MONITOR etm,
  KHE_TIME_GROUP tg)
{
  int res, i, j, k;  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;
  res = 0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(etm, t);  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(etm, t, j);
      for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
      {
	task = KheMeetTask(meet, k);
	if( KheTaskAsstResource(task) != NULL )
	  res++;
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheFrameAssignmentsDebug(KHE_FRAME frame, int indent, FILE *fp)     */
/*                                                                           */
/*  Print the number of assigned tasks in each time group of frame.          */
/*                                                                           */
/*****************************************************************************/

static void KheFrameAssignmentsDebug(KHE_FRAME frame, KHE_OPTIONS options,
  int indent, FILE *fp)
{
  KHE_EVENT_TIMETABLE_MONITOR etm;  KHE_TIME_GROUP tg;  int i;
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  fprintf(fp, "%*s[", indent, "");
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    if( i > 0 )
      fprintf(fp, " ");
    fprintf(fp, "%d", KheTimeGroupAssignments(etm, tg));
  }
  fprintf(fp, "]\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDebugTimetable(KHE_SOLN soln, char *fmt, ...)                    */
/*                                                                           */
/*  If DEBUG7, print the timetable of resource DEBUG7_ID, plus header.       */
/*                                                                           */
/*****************************************************************************/

static void KheDebugTimetable(KHE_SOLN soln, char *fmt, ...)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_INSTANCE ins;  KHE_RESOURCE r;
  va_list args;
  if( DEBUG7 )
  {
    ins = KheSolnInstance(soln);
    if( KheInstanceRetrieveResource(ins, DEBUG7_ID, &r) )
    {
      fprintf(stderr, "  [ %s timetable ", KheResourceId(r));
      if( fmt != NULL )
      {
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
      }
      fprintf(stderr, "\n");
      rtm = KheResourceTimetableMonitor(soln, r);
      KheResourceTimetableMonitorPrintTimetable(rtm, 10, 4, stderr);
      fprintf(stderr, "  ]\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitor frames (private)"                                     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_FRAME KheMonitorFrameMake(KHE_FRAME frame)                   */
/*                                                                           */
/*  Make a new, empty monitor frame suitable for use with frame.             */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_FRAME KheMonitorFrameMake(KHE_FRAME frame, HA_ARENA a)
{
  KHE_MONITOR_FRAME res;  KHE_MONITOR_FRAME_PART part;  int i;
  HaMake(res, a);
  res->frame = frame;
  HaArrayInit(res->active_resource_monitors, a);
  HaArrayInit(res->parts, a);
  for( i = 0;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    HaMake(part, a);
    HaArrayInit(part->resource_monitors, a);
    HaArrayInit(part->task_monitors, a);
    HaArrayAddLast(res->parts, part);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameAddResourceMonitor(KHE_MONITOR_FRAME mf,             */
/*    KHE_MONITOR m, KHE_TIME time)                                          */
/*                                                                           */
/*  Add resource monitor m to mf at the position where time occurs in        */
/*  frame.  Or if time is NULL, do nothing.                                  */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameAddResourceMonitor(KHE_MONITOR_FRAME mf,
  KHE_MONITOR m, KHE_TIME first_time, KHE_TIME last_time)
{
  int first_index, last_index, index;  KHE_MONITOR_FRAME_PART part;
  first_index = KheFrameTimeIndex(mf->frame, first_time);
  last_index = KheFrameTimeIndex(mf->frame, last_time);
  for( index = first_index;  index <= last_index;  index++ )
  {
    part = HaArray(mf->parts, index);
    HaArrayAddLast(part->resource_monitors, m);
  }
}

/* old version
static void KheMonitorFrameAddResourceMonitor(KHE_MONITOR_FRAME mf,
  KHE_MONITOR m, KHE_TIME time)
{
  int index;  KHE_MONITOR_FRAME_PART part;
  if( time != NULL )
  {
    index = KheFrameTimeIndex(mf->frame, time);
    part = HaArray(mf->parts, index);
    HaArrayAddLast(part->resource_monitors, m);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameAddAndDetachTaskMonitor(KHE_MONITOR_FRAME mf,        */
/*    KHE_MONITOR m, KHE_TIME time)                                          */
/*                                                                           */
/*  Add task monitor m to mf at the position where time occurs in            */
/*  frame, and detach it.  Or if time is NULL, do nothing.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameAddAndDetachTaskMonitor(KHE_MONITOR_FRAME mf,
  KHE_MONITOR m, KHE_TIME time)
{
  int index, pos;  KHE_MONITOR_FRAME_PART part;
  if( time != NULL )
  {
    index = KheFrameTimeIndex(mf->frame, time);
    part = HaArray(mf->parts, index);
    if( !HaArrayContains(part->task_monitors, m, &pos) )
    {
      HaArrayAddLast(part->task_monitors, m);
      KheMonitorDetachFromSoln(m);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameAddAndCutOffAllResourceMonitors(KHE_MONITOR_FRAME mf,*/
/*    KHE_RESOURCE_GROUP rg, KHE_SOLN soln)                                  */
/*                                                                           */
/*  Add all relevant monitors to mf, and cut them all off at index 0.        */
/*                                                                           */
/*  Cluster busy times monitors which request specific busy times are not    */
/*  cut off, since that would remove the rationale for assigning requested   */
/*  resources induce time sweep to wrongly unassign them during rematching.  */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameAddAndCutOffAllResourceMonitors(KHE_MONITOR_FRAME mf,
  KHE_RESOURCE_GROUP rg, KHE_SOLN soln)
{
  KHE_TIME first_time, last_time;  int i, j;  KHE_RESOURCE r;  KHE_MONITOR m;
  if( DEBUG6 )
    fprintf(stderr, "[ KheMonitorFrameAddAndCutOffAllResourceMonitors(...):\n");
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorCutoffTimeRange(m, &first_time, &last_time) )
      {
	KheMonitorSetCutoffTime(m, NULL);
        KheMonitorFrameAddResourceMonitor(mf, m, first_time, last_time);
      }
      /* ***
      if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
      {
	laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
	KheLimitActiveIntervalsMonitorSetCutoffIndex(laim, 0);
        t = KheLimitActiveIntervalsMonitorInitialCutoffTime(laim);
	KheMonitorFrameAddResourceMonitor(mf, m, t);
	if( DEBUG6 )
	  KheMonitorDebug(m, 2, 2, stderr);
      }
      else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG &&
	  !KheMonitorRequestsSpecificBusyTimes(m) )
      {
	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	KheClusterBusyTimesMonitorSetCutoffIndex(cbtm, 0);
        t = KheClusterBusyTimesMonitorInitialCutoffTime(cbtm);
	KheMonitorFrameAddResourceMonitor(mf, m, t);
	if( DEBUG6 )
	  KheMonitorDebug(m, 2, 2, stderr);
      }
      *** */
    }
  }
  if( DEBUG6 )
    fprintf(stderr,
      "] KheMonitorFrameAddAndCutOffAllResourceMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheEventResourceLastTime(KHE_EVENT_RESOURCE er, KHE_SOLN soln)  */
/*                                                                           */
/*  Return the last time that er is running in soln, or NULL if none.        */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheEventResourceLastTime(KHE_EVENT_RESOURCE er, KHE_SOLN soln)
{
  KHE_TIME time, res;  int i;  KHE_MEET meet;  KHE_EVENT e;
  res = NULL;
  e = KheEventResourceEvent(er);
  for( i = 0;  i < KheEventMeetCount(soln, e);  i++ )
  {
    meet = KheEventMeet(soln, e, i);
    time = KheMeetAsstTime(meet);
    if( time != NULL )
    {
      time = KheTimeNeighbour(time, KheEventDuration(e) - 1);
      if( res == NULL || KheTimeIndex(res) < KheTimeIndex(time) )
	res = time;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheAssignResourceMonitorLastTime(KHE_ASSIGN_RESOURCE_MONITOR m) */
/*                                                                           */
/*  Return the largest time of the meets of the tasks monitored by m.        */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheAssignResourceMonitorLastTime(KHE_ASSIGN_RESOURCE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;
  er = KheAssignResourceMonitorEventResource(m);
  return KheEventResourceLastTime(er, KheMonitorSoln((KHE_MONITOR) m));
}


/*****************************************************************************/
/*                                                                           */
/* KHE_TIME KhePreferResourcesMonitorLastTime(KHE_PREFER_RESOURCES_MONITOR m)*/
/*                                                                           */
/*  Return the largest time of the meets of the tasks monitored by m.        */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_TIME KhePreferResourcesMonitorLastTime(
  KHE_PREFER_RESOURCES_MONITOR m)
{
  KHE_EVENT_RESOURCE er;
  er = KhePreferResourcesMonitorEventResource(m);
  return KheEventResourceLastTime(er, KheMonitorSoln((KHE_MONITOR) m));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheLimitResourcesMonitorLastTime(KHE_LIMIT_RESOURCES_MONITOR m) */
/*                                                                           */
/*  Return the largest time of the meets of the tasks monitored by m.        */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME KheLimitResourcesMonitorLastTime(KHE_LIMIT_RESOURCES_MONITOR m)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;  int ix, i;  KHE_TIME time, res;
  KHE_EVENT_RESOURCE er;
  lrc = KheLimitResourcesMonitorConstraint(m);
  ix = KheLimitResourcesMonitorEventGroupIndex(m);
  res = NULL;
  for( i = 0; i < KheLimitResourcesConstraintEventResourceCount(lrc, ix); i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, ix, i);
    time = KheEventResourceLastTime(er, KheMonitorSoln((KHE_MONITOR) m));
    if( time != NULL )
    {
      if( res == NULL || KheTimeIndex(res) < KheTimeIndex(time) )
	res = time;
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameAddAndDetachTaskMonitors(KHE_MONITOR_FRAME mf,       */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Add all relevant task monitors to mf, and detach them.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameAddAndDetachTaskMonitors(KHE_MONITOR_FRAME mf,
  KHE_SOLN soln)
{
  KHE_TIME t;  int i, j;  KHE_ASSIGN_RESOURCE_MONITOR arm;
  /* KHE_PREFER_RESOURCES_MONITOR prm; */  KHE_LIMIT_RESOURCES_MONITOR lrm;
  KHE_MONITOR m;  KHE_INSTANCE ins;  KHE_EVENT_RESOURCE er;
  if( DEBUG6 )
    fprintf(stderr, "[ KheMonitorFrameAddAndDetachTaskMonitors(...):\n");
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    for( j = 0;  j < KheSolnEventResourceMonitorCount(soln, er);  j++ )
    {
      m = KheSolnEventResourceMonitor(soln, er, j);
      if( KheMonitorAttachedToSoln(m) ) switch( KheMonitorTag(m) )
      {
	case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

	  arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
	  t = KheAssignResourceMonitorLastTime(arm);
	  KheMonitorFrameAddAndDetachTaskMonitor(mf, m, t);
	  if( DEBUG6 )
	    KheMonitorDebug(m, 2, 2, stderr);
	  break;

	/* *** leaving these ones attached now
	case KHE_PREFER_RESOURCES_MONITOR_TAG:

	  prm = (KHE_PREFER_RESOURCES_MONITOR) m;
	  t = KhePreferResourcesMonitorLastTime(prm);
	  KheMonitorFrameAddAndDetachTaskMonitor(mf, m, t);
	  if( DEBUG6 )
	    KheMonitorDebug(m, 2, 2, stderr);
	  break;
	*** */

	case KHE_LIMIT_RESOURCES_MONITOR_TAG:

	  lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
	  t = KheLimitResourcesMonitorLastTime(lrm);
	  KheMonitorFrameAddAndDetachTaskMonitor(mf, m, t);
	  if( DEBUG6 )
	    KheMonitorDebug(m, 2, 2, stderr);
	  break;

	default:

	  /* do nothing */
	  break;
      }
    }
  }
  if( DEBUG6 )
    fprintf(stderr, "] KheMonitorFrameAddAndDetachTaskMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameUpdateResourceMonitorCutoffs(KHE_MONITOR_FRAME mf,   */
/*    int frame_index)                                                       */
/*                                                                           */
/*  We are now starting on position frame_index of the frame, so add         */
/*  in the monitors that start there, cut off all monitors at the last       */
/*  time of that frame part, and remove from the current list all            */
/*  monitors that don't need cutting off after here.                         */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameUpdateResourceMonitorCutoffs(KHE_MONITOR_FRAME mf,
  int frame_index)
{
  KHE_MONITOR_FRAME_PART part;  int i, tg_count;  KHE_TIME_GROUP tg;
  KHE_TIME t;  bool still_active;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  KHE_MONITOR m;
  int initial, adding, dropping;

  /*  add in the monitors that start at frame_index */
  if( DEBUG6 )
    fprintf(stderr, "[ KheMonitorFrameResourceUpdateMonitorCutoffs(mf, %d):\n",
      frame_index);
  part = HaArray(mf->parts, frame_index);
  initial = HaArrayCount(mf->active_resource_monitors);
  adding = HaArrayCount(part->resource_monitors);
  HaArrayAppend(mf->active_resource_monitors, part->resource_monitors, i);

  /* find the time we need to be cutting off at */
  tg = KheFrameTimeGroup(mf->frame, frame_index);
  tg_count = KheTimeGroupTimeCount(tg);
  HnAssert(tg_count > 0, "KheTimeSweepAssignResources: empty part");
  t = KheTimeGroupTime(tg, tg_count - 1);

  /* cut off all active monitors, simultaneously removing from the active */
  /* list those that don't need to be cut off again */
  dropping = 0;
  for( i = 0;  i < HaArrayCount(mf->active_resource_monitors);  i++ )
  {
    m = HaArray(mf->active_resource_monitors, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      still_active = KheLimitActiveIntervalsMonitorSetCutoffTime(laim, t);
    }
    else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      still_active = KheClusterBusyTimesMonitorSetCutoffTime(cbtm, t);
    }
    else
    {
      HnAbort("KheMonitorFrameUpdateResourceMonitorCutoffs internal error");
      still_active = false;  /* keep compiler happy */
    }
    if( !still_active )
    {
      HaArrayDeleteAndPlug(mf->active_resource_monitors, i);
      i--;
      dropping++;
    }
  }
  if( DEBUG6 )
  {
    HaArrayForEach(mf->active_resource_monitors, m, i)
      KheMonitorDebug(m, 2, 2, stderr);
    fprintf(stderr, "  %d + %d = %d - %d = %d:\n", initial,
      adding, initial + adding, dropping,
      HaArrayCount(mf->active_resource_monitors));
    fprintf(stderr, "] KheMonitorFrameUpdateResourceMonitorCutoffs ret.\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameAttachTaskMonitors(KHE_MONITOR_FRAME mf,             */
/*    int frame_index)                                                       */
/*                                                                           */
/*  Attach the task monitors at position frame_index of mf.                  */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameAttachTaskMonitors(KHE_MONITOR_FRAME mf,
  int frame_index)
{
  KHE_MONITOR_FRAME_PART part;  int i;  KHE_MONITOR m;
  if( DEBUG9 )
    fprintf(stderr, "[ KheMonitorFrameAttachTaskMonitors(mf, %d)\n",
      frame_index);
  part = HaArray(mf->parts, frame_index);
  HaArrayForEach(part->task_monitors, m, i)
  {
    KheMonitorAttachToSoln(m);
    if( DEBUG9 )
    {
      KheMonitorDebug(m, 2, 2, stderr);
      if( KheMonitorTag(m) == KHE_ASSIGN_RESOURCE_MONITOR_TAG )
      {
	KHE_TASK task;  KHE_ASSIGN_RESOURCE_MONITOR arm;
	KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  int j;
	soln = KheMonitorSoln(m);
	arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
	er = KheAssignResourceMonitorEventResource(arm);
	for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
	{
	  task = KheEventResourceTask(soln, er, j);
	  KheTaskDebug(task, 4, 4, stderr);
	}
      }
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheMonitorFrameAttachTaskMonitors returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameRemoveResourceMonitorCutoffs(KHE_MONITOR_FRAME mf)   */
/*                                                                           */
/*  Remove cutoffs from the remaining active monitors.                       */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameRemoveResourceMonitorCutoffs(KHE_MONITOR_FRAME mf)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  KHE_MONITOR m;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  int i;
  HaArrayForEach(mf->active_resource_monitors, m, i)
  {
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laim = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      KheLimitActiveIntervalsMonitorSetCutoffTime(laim, NULL);
    }
    else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    {
      cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
      KheClusterBusyTimesMonitorSetCutoffTime(cbtm, NULL);
    }
    else
      HnAbort("KheMonitorFrameRemoveResourceMonitorCutoffs internal errorg");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitorFrameDebug(KHE_MONITOR_FRAME mf, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of mf onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void KheMonitorFrameDebug(KHE_MONITOR_FRAME mf, int indent, FILE *fp)
{
  int i, j;  KHE_TIME_GROUP tg;
  KHE_MONITOR_FRAME_PART part;  KHE_MONITOR m;
  fprintf(fp, "%*s[ Monitor frame (%d parts):\n", indent, "",
    KheFrameTimeGroupCount(mf->frame));
  for( i = 0;  i < KheFrameTimeGroupCount(mf->frame);  i++ )
  {
    tg = KheFrameTimeGroup(mf->frame, i);
    part = HaArray(mf->parts, i);
    fprintf(fp, "%*s[ ", indent + 2, "");
    KheTimeGroupDebug(tg, 1, 0, fp);
    HaArrayForEach(part->resource_monitors, m, j)
      KheMonitorDebug(m, 2, indent + 4, fp);
    HaArrayForEach(part->task_monitors, m, j)
      KheMonitorDebug(m, 2, indent + 4, fp);
    fprintf(fp, "%*s]\n", indent + 2, "");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheDefaultRematchMaxGroups(KHE_FRAME frame)                          */
/*                                                                           */
/*  Return a default maximum number of rematch time groups.  The larger      */
/*  the frame size, the smaller the number of time groups, for speed.        */
/*                                                                           */
/*****************************************************************************/

static int KheDefaultRematchMaxGroups(KHE_FRAME frame)
{
  int count;
  count = KheFrameTimeGroupCount(frame);
  if( count <= 4 * 7 )
    return 7;
  else if( count <= 26 * 7 )
    return 4;
  else
    return 1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSaveDemandSets(KHE_FRAME frame)                                  */
/*                                                                           */
/*  Return true if we are saving demand sets.  This is only feasible on      */
/*  fairly short frames.                                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheSaveDemandSets(KHE_FRAME frame)
{
  return KheFrameTimeGroupCount(frame) <= 4 * 7;
}


/*****************************************************************************/
/*                                                                           */
/*  void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, KHE_FRAME frame,     */
/*    int first_index, int last_index, char *etc)                            */
/*                                                                           */
/*  Brief report of what is starting, for debugging.                         */
/*                                                                           */
/*****************************************************************************/

static void DebugStart(KHE_SOLN soln, KHE_OPTIONS options, KHE_FRAME frame,
  int first_index, int last_index, char *etc)
{
  KHE_INSTANCE ins;  KHE_TIMER timer;  char buff[20];  int i;
  KHE_TIME_GROUP tg;
  if( DEBUG2 )
  {
    ins = KheSolnInstance(soln);
    fprintf(stderr, "  %s#%d: ", KheInstanceId(ins), KheSolnDiversifier(soln));
    if( KheOptionsContainsTimer(options, "global", &timer) )
      fprintf(stderr, " after %s",
	KheTimeShow(KheTimerElapsedTime(timer), buff));
    fprintf(stderr, " starting KheTimeSweepAssignResources");
    for( i = first_index;  i <= last_index;  i++ )
    {
      tg = KheFrameTimeGroup(frame, i);
      fprintf(stderr, " %s", KheTimeGroupId(tg));
    }
    if( etc != NULL )
      fprintf(stderr, " %s", etc);
    fprintf(stderr, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_SWEEP_OPTIONS - options for controlling time sweep              */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_sweep_options_rec {
  KHE_FRAME	frame;
  int		frame_len;
  float		daily_time_limit;
  /* KHE_TIMER	daily_timer; */
  KHE_TIMER	st_cutoffs;
  float		time_cutoffs;
  KHE_TIMER	st_matches;
  float		time_matches;
  bool		edge_adjust1_off;
  bool		edge_adjust2_off;
  bool		edge_adjust3_off;
  bool		edge_adjust4_off;
  bool		ejection_off;
  int		lookahead;
  bool		preserve_existing;
  bool		cutoff_off;
  bool		redo_off;
  bool		rematch_off;
  int		rematch_max_groups;
} *KHE_TIME_SWEEP_OPTIONS;


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeSweepDoAssignResources(KHE_SOLN soln, KHE_RESOURCE_GROUP rg, */
/*    KHE_TIME_SWEEP_OPTIONS tso, KHE_OPTIONS options, HA_ARENA a)           */
/*                                                                           */
/*  Do most of the work of KheTimeSweepAssignResources.                      */
/*                                                                           */
/*****************************************************************************/

static void KheTimeSweepDoAssignResources(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_TIME_SWEEP_OPTIONS tso, KHE_OPTIONS options, HA_ARENA a)
{
  KHE_RESOURCE_MATCHING_SOLVER rms;  KHE_TIME_GROUP tg;
  int frame_pos, init_len;  KHE_TIMER daily_timer;
  int count, p, i, redo_count;  KHE_COST init_cost;
  bool rematch_redo;  ARRAY_KHE_RESOURCE_MATCHING_DEMAND_SET demand_sets;
  KHE_RESOURCE_MATCHING_DEMAND_SET rmds;  KHE_MONITOR_FRAME mf;
  rms = KheResourceMatchingSolverMake(soln, rg, a);

  if( DEBUG8 )
    KheFrameAssertNoClashes(tso->frame);

  /* install cutoffs, if required */
  if( !tso->cutoff_off )
  {
    mf = KheMonitorFrameMake(tso->frame, a);
    KheMonitorFrameAddAndCutOffAllResourceMonitors(mf, rg, soln);
    for( frame_pos = 0;  frame_pos < tso->lookahead;  frame_pos++ )
      KheMonitorFrameUpdateResourceMonitorCutoffs(mf, frame_pos);
    KheMonitorFrameAddAndDetachTaskMonitors(mf, soln);
    if( DEBUG5 )
      KheMonitorFrameDebug(mf, 2, stderr);
  }
  else
    mf = NULL;  /* keep compiler happy */
  rematch_redo = false;

  /* create timer for daily time limit, if required */
  if( tso->daily_time_limit >= 0.0 )
  {
    daily_timer = KheOptionsAddTimer(options, "daily",
      tso->daily_time_limit);
    if( DEBUG2 )
      fprintf(stderr,
	"  KheTimeSweepAssignResources daily time limit %.1f secs\n",
        tso->daily_time_limit);
  }
  else
    daily_timer = NULL;

  /* get demand sets up to just before position lookahead */
  HaArrayInit(demand_sets, a);
  init_len = min(tso->lookahead, tso->frame_len);
  for( frame_pos = 0;  frame_pos < init_len;  frame_pos++ )
  {
    rmds = KheResourceMatchingDemandSetMake(rms, tso->preserve_existing);
    tg = KheFrameTimeGroup(tso->frame, frame_pos);
    KheResourceMatchingDemandSetAddTimeGroup(rmds, tg);
    HaArrayAddLast(demand_sets, rmds);
  }

  /* do one solve for each time group of frame */
  for( frame_pos = 0;  frame_pos < tso->frame_len;  frame_pos++ )
  {
    DebugStart(soln, options, tso->frame, frame_pos, frame_pos, "construct");
    if( DEBUG8 )
      KheFrameAssertNoClashes(tso->frame);

    /* start daily time limit, if requested */
    if( tso->daily_time_limit >= 0.0 )
      KheTimerResetStartTime(daily_timer);

    /* get demand set for position frame_pos + lookahead  */
    if( frame_pos + tso->lookahead < tso->frame_len )
    {
      rmds = KheResourceMatchingDemandSetMake(rms, tso->preserve_existing);
      tg = KheFrameTimeGroup(tso->frame, frame_pos + tso->lookahead);
      KheResourceMatchingDemandSetAddTimeGroup(rmds, tg);
      HaArrayAddLast(demand_sets, rmds);
    }

    /* install cutoffs, unless cutoff_off */
    if( !tso->cutoff_off )
    {
      if( DEBUG3 )
	KheTimerResetStartTime(tso->st_cutoffs);
      if( frame_pos + tso->lookahead < tso->frame_len )
	KheMonitorFrameUpdateResourceMonitorCutoffs(mf,
	  frame_pos + tso->lookahead);
      KheMonitorFrameAttachTaskMonitors(mf, frame_pos);
      if( DEBUG3 )
	tso->time_cutoffs += KheTimerElapsedTime(tso->st_cutoffs);
    }

    /* do the solve */
    if( DEBUG3 )
      KheTimerResetStartTime(tso->st_matches);
    DebugStart(soln, options, tso->frame, frame_pos, frame_pos, "solve");
    KheResourceMatchingSolverSolveWithLookahead(rms, &demand_sets, frame_pos,
      min(frame_pos + tso->lookahead, tso->frame_len - 1),
      tso->edge_adjust1_off, tso->edge_adjust2_off, tso->edge_adjust3_off,
      tso->edge_adjust4_off, tso->ejection_off, /* nocost_off, */ options);
    DebugStart(soln, options, tso->frame, frame_pos, frame_pos, "after-solve");
    KheDebugTimetable(soln, "after solving %d", frame_pos);
    if( DEBUG8 )
      KheFrameAssertNoClashes(tso->frame);

    /* clear demand set for frame_pos, if not being saved */
    if( !KheSaveDemandSets(tso->frame) )
    {
      rmds = HaArray(demand_sets, frame_pos);
      KheResourceMatchingDemandSetDelete(rmds);
      HaArrayPut(demand_sets, frame_pos, NULL);
    }
    if( DEBUG3 )
      tso->time_matches += KheTimerElapsedTime(tso->st_matches);

    /* rematch, unless rematch_off */
    if( !tso->rematch_off && !KheOptionsTimeLimitReached(options) )
    {
      rmds = KheResourceMatchingDemandSetMake(rms, false);
      count = min(tso->rematch_max_groups, frame_pos + 1);
      do
      {
	if( DEBUG4 )
	  fprintf(stderr, "  and rematching 2 .. %d parts\n", count);
	init_cost = KheSolnCost(soln);
	for( p = 2;  p <= count && !KheOptionsTimeLimitReached(options);  p++ )
	{
	  for( i = 0;  i < p;  i++ )
	  {
	    tg = KheFrameTimeGroup(tso->frame, frame_pos - i);
	    KheResourceMatchingDemandSetAddTimeGroup(rmds, tg);
	  }
	  DebugStart(soln, options, tso->frame, frame_pos - p + 1, frame_pos,
	    "rematch");
	  if( DEBUG3 )
	    KheTimerResetStartTime(tso->st_matches);
	  if( DEBUG8 )
	    KheFrameAssertNoClashes(tso->frame);
	  KheResourceMatchingSolverSolve(rms, rmds, tso->edge_adjust1_off,
	    tso->edge_adjust2_off, tso->edge_adjust3_off, tso->edge_adjust4_off,
	    tso->ejection_off, /* nocost_off, */ options);
          KheDebugTimetable(soln, "after rematch %d .. %d",
	    frame_pos - p + 1, frame_pos);
	  if( DEBUG8 )
	    KheFrameAssertNoClashes(tso->frame);
          KheResourceMatchingDemandSetClear(rmds);
	  if( DEBUG3 )
	    tso->time_matches += KheTimerElapsedTime(tso->st_matches);
	}
      } while( rematch_redo && KheSolnCost(soln) < init_cost );
      KheResourceMatchingDemandSetDelete(rmds);
    }
    if( DEBUG11 )
    {
      fprintf(stderr, "  after time sweep of time group %d:\n", frame_pos);
      KheFrameAssignmentsDebug(tso->frame, options, 2, stderr);
    }
  }
  if( tso->daily_time_limit >= 0.0 )
    KheOptionsDeleteTimer(options, daily_timer);

  /* remove cutoffs, unless cutoff_off */
  if( !tso->cutoff_off )
  {
    if( DEBUG3 )
      KheTimerResetStartTime(tso->st_cutoffs);
    KheMonitorFrameRemoveResourceMonitorCutoffs(mf);
    if( DEBUG3 )
      tso->time_cutoffs += KheTimerElapsedTime(tso->st_cutoffs);
  }

  /* if time permits, redo while improving (no cutoffs, no rematch) */
  /* although redo no more than twice, because of diminishing returns */
  if( !tso->redo_off && !KheOptionsTimeLimitReached(options) )
  {
    redo_count = 0;
    do
    {
      init_cost = KheSolnCost(soln);
      if( tso->daily_time_limit >= 0.0 )
	daily_timer = KheOptionsAddTimer(options, "daily",
	  tso->daily_time_limit/2);
      for( frame_pos = 0;  frame_pos < tso->frame_len;  frame_pos++ )
      {
	/* daily time limit, if requested */
	if( tso->daily_time_limit >= 0.0 )
	  KheTimerResetStartTime(daily_timer);

	/* get time group and demand set for frame_pos */
	tg = KheFrameTimeGroup(tso->frame, frame_pos);
	if( KheSaveDemandSets(tso->frame) )
	  rmds = HaArray(demand_sets, frame_pos);
	else
	{
	  rmds = KheResourceMatchingDemandSetMake(rms, tso->preserve_existing);
	  KheResourceMatchingDemandSetAddTimeGroup(rmds, tg);
	}

	/* solve tg */
	DebugStart(soln, options, tso->frame, frame_pos, frame_pos, "redo");
	if( DEBUG3 )
	  KheTimerResetStartTime(tso->st_matches);
	if( DEBUG8 )
	  KheFrameAssertNoClashes(tso->frame);
	KheResourceMatchingSolverSolve(rms, rmds, tso->edge_adjust1_off,
	  tso->edge_adjust2_off, tso->edge_adjust3_off, tso->edge_adjust4_off,
	  tso->ejection_off, /* nocost_off, */ options);
	if( DEBUG8 )
	  KheFrameAssertNoClashes(tso->frame);
	redo_count++;
	if( !KheSaveDemandSets(tso->frame) )
	  KheResourceMatchingDemandSetClear(rmds);
	if( DEBUG3 )
	  tso->time_matches += KheTimerElapsedTime(tso->st_matches);
      }
      if( tso->daily_time_limit >= 0.0 )
	KheOptionsDeleteTimer(options, daily_timer);
      if( DEBUG11 )
      {
	fprintf(stderr, "  after time sweep of time group %d:\n", frame_pos);
	KheFrameAssignmentsDebug(tso->frame, options, 2, stderr);
      }
    } while( redo_count < 2 && KheSolnCost(soln) < init_cost &&
	!KheOptionsTimeLimitReached(options) );
  }

  /* debug, reclaim all memory, and return */
  KheResourceMatchingSolverDelete(rms);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeSweepAssignResources(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,   */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out time sweep assignment of the resources of rg in soln.          */
/*                                                                           */
/*****************************************************************************/

bool KheTimeSweepAssignResources(KHE_SOLN soln, KHE_RESOURCE_GROUP rg,
  KHE_OPTIONS options)
{
  struct khe_time_sweep_options_rec tsor;  KHE_RESOURCE_GROUP rg1, rg2;
  KHE_COST global_init_cost;  bool two_phase;  KHE_TIMER st_all;  HA_ARENA a;
  float time_all;

  if( DEBUG1 )
    fprintf(stderr, "[ KheTimeSweepAssignResources(soln, rg, options)\n");
  if( DEBUG10 )
  {
    fprintf(stderr, "at start of KheTimeSweepAssignResources, arena set is:\n");
    HaArenaSetDebug(KheSolnArenaSet(soln), 1, 2, stderr);
  }
  KheDebugTimetable(soln, "at start of KheTimeSweepAssignResources");

  /* get an arena */
  a = KheSolnArenaBegin(soln);

  /* initialize for debugging running time, if wanted */
  if( DEBUG3 )
  {
    st_all = KheTimerMake("ts_all", KHE_NO_TIME, a);
    tsor.st_cutoffs = KheTimerMake("ts_cutoffs", KHE_NO_TIME, a);
    tsor.st_matches = KheTimerMake("ts_matches", KHE_NO_TIME, a);
  }
  else
    st_all = tsor.st_cutoffs = tsor.st_matches = NULL;
  time_all = tsor.time_cutoffs = tsor.time_matches = 0.0;

  /* get the time sweep options */
  tsor.frame = KheOptionsFrame(options, "gs_common_frame", soln);
  tsor.frame_len = KheFrameTimeGroupCount(tsor.frame);
  tsor.daily_time_limit = KheTimeFromString(KheOptionsGet(options,
    "rs_time_sweep_daily_time_limit", "3"));
  /* tsor.daily_timer = NULL; */
  tsor.edge_adjust1_off = KheOptionsGetBool(options,
    "rs_time_sweep_edge_adjust1_off", false);
  tsor.edge_adjust2_off = KheOptionsGetBool(options,
    "rs_time_sweep_edge_adjust2_off", false);
  tsor.edge_adjust3_off = KheOptionsGetBool(options,
    "rs_time_sweep_edge_adjust3_off", false);
  tsor.edge_adjust4_off = KheOptionsGetBool(options,
    "rs_time_sweep_edge_adjust4_off", false);
  tsor.ejection_off = KheOptionsGetBool(options,
    "rs_time_sweep_ejection_off", false);
  tsor.lookahead = KheOptionsGetInt(options,
    "rs_time_sweep_lookahead", 0);
  tsor.preserve_existing = ! KheOptionsGetBool(options,
    "rs_time_sweep_preserve_existing_off", false);
  tsor.cutoff_off = KheOptionsGetBool(options,
    "rs_time_sweep_cutoff_off", false);
  tsor.redo_off = KheOptionsGetBool(options,
    "rs_time_sweep_redo_off", false);
  tsor.rematch_off = KheOptionsGetBool(options,
    "rs_time_sweep_rematch_off", false);
  tsor.rematch_max_groups = KheOptionsGetInt(options,
    "rs_time_sweep_rematch_max_groups", KheDefaultRematchMaxGroups(tsor.frame));
  two_phase = KheOptionsGetBool(options,
    "rs_time_sweep_two_phase", false);

  /* do the actual solving */
  global_init_cost = KheSolnCost(soln);
  if( two_phase && KheClassifyResourcesByWorkload(soln, rg, &rg1, &rg2) )
  {
    KheTimeSweepDoAssignResources(soln, rg1, &tsor, options, a);
    KheTimeSweepDoAssignResources(soln, rg2, &tsor, options, a);
  }
  else
    KheTimeSweepDoAssignResources(soln, rg, &tsor, options, a);
  if( DEBUG3 )
  {
    time_all += KheTimerElapsedTime(st_all);
    fprintf(stderr, "  timing:  all = %.1f, cutoffs = %.1f, matches = %.1f\n",
      time_all, tsor.time_cutoffs, tsor.time_matches);
  }

  /* give up the arena */
  KheSolnArenaEnd(soln, a);

  /* debug print and exit */
  KheDebugTimetable(soln, "at end of KheTimeSweepAssignResources");
  if( DEBUG10 )
  {
    fprintf(stderr, "at end of KheTimeSweepAssignResources, arena set is:\n");
    HaArenaSetDebug(KheSolnArenaSet(soln), 1, 2, stderr);
  }
  if( DEBUG11 )
  {
    fprintf(stderr, "  at end of time sweep:\n");
    KheFrameAssignmentsDebug(tsor.frame, options, 2, stderr);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheTimeSweepAssignResources ret. %s (%.5f -> %.5f)\n",
      KheSolnCost(soln) < global_init_cost ? "true" : "false",
      KheCostShow(global_init_cost), KheCostShow(KheSolnCost(soln)));
  return KheSolnCost(soln) < global_init_cost;
}

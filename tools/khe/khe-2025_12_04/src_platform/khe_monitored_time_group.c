
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
/*  FILE:         khe_monitored_time_group.c                                 */
/*  DESCRIPTION:  A monitored time group                                     */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG1(m) (false && strcmp(KheMonitorId((KHE_MONITOR) m), "S4/t24")==0)

#define DEBUG2(mtg)	\
  (false && strcmp(KheTimeGroupId((mtg)->time_group), "1-3Mon-Sun") == 0 && \
   strcmp(KheResourceId(KheMonitoredTimeGroupResource(mtg)), "Nurse11") == 0)

#define DEBUG3 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP - one time group monitored by a timetable.      */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_MTG_STATE_UNATTACHED,		/* not attached to timetable monitor */
  KHE_MTG_STATE_ATTACHED_BUSY,		/* attached, monitor busy only       */
  KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE	/* attached, monitor busy and idle   */
} KHE_MTG_STATE;

struct khe_monitored_time_group_rec {
  KHE_TIME_GROUP		time_group;		/* monitored tg      */
  KHE_RESOURCE_TIMETABLE_MONITOR timetable_monitor;	/* enclosing tm      */
  ARRAY_KHE_MONITOR		monitors;		/* attached monitors */
  HA_ARRAY_INT			monitor_indexes;	/* id in monitors    */
  KHE_MTG_STATE			state;			/* attach state      */
  struct khe_busy_and_idle_rec	busy_and_idle;		/* busy, idle, etc.  */
  /* int			busy_count; */		/* busy times count  */
  /* float			workload; */		/* workload in tg    */
  /* int			idle_count; */		/* idle times count  */
  SSET				busy_set;		/* busy times lset   */
  KHE_TIME			sweep_time;		/* sweep time        */
  int				sweep_index;		/* sweep index       */
  KHE_MONITORED_TIME_GROUP	copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "construction and query"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheMonitoredTimeGroupResource(KHE_MONITORED_TIME_GROUP mtg) */
/*                                                                           */
/*  Return the resource monitored by mtg.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE KheMonitoredTimeGroupResource(KHE_MONITORED_TIME_GROUP mtg)
{
  return KheResourceTimetableMonitorResource(mtg->timetable_monitor);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP KheMonitoredTimeGroupMake(KHE_TIME_GROUP tg,    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Make a new monitored time group object for rtm, monitoring tg.           */
/*                                                                           */
/*****************************************************************************/

KHE_MONITORED_TIME_GROUP KheMonitoredTimeGroupMake(KHE_TIME_GROUP tg,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KHE_MONITORED_TIME_GROUP res;  HA_ARENA a;  int count;
  a = KheSolnArena(KheMonitorSoln((KHE_MONITOR) rtm));
  HaMake(res, a);
  res->time_group = tg;
  res->timetable_monitor = rtm;
  HaArrayInit(res->monitors, a);
  HaArrayInit(res->monitor_indexes, a);
  res->state = KHE_MTG_STATE_UNATTACHED;
  res->busy_and_idle.busy_count = 0;
  res->busy_and_idle.workload = 0.0;
  res->busy_and_idle.idle_count = 0;
  res->busy_and_idle.open_count = 0;
  SSetInit(res->busy_set, a);
  count = KheTimeGroupTimeCount(tg);
  if( count > 0 )
    res->sweep_time = KheTimeGroupTime(tg, count - 1);
  else
    res->sweep_time = NULL;
  res->sweep_index = count;
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP KheMonitoredTimeGroupCopyPhase1(                */
/*    KHE_MONITORED_TIME_GROUP mtg, HA_ARENA a)                              */
/*                                                                           */
/*  Carry out Phase 1 of the copying of mtg.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_MONITORED_TIME_GROUP KheMonitoredTimeGroupCopyPhase1(
  KHE_MONITORED_TIME_GROUP mtg, HA_ARENA a)
{
  KHE_MONITORED_TIME_GROUP copy;  int i, mi;  KHE_MONITOR m, m_copy;
  if( mtg->copy == NULL )
  {
    if( DEBUG3 )
      fprintf(stderr, "[ KheMonitoredTimeGroupCopyPhase1(%p, a)\n",
	(void *) mtg);
    HaMake(copy, a);
    mtg->copy = copy;
    copy->time_group = KheTimeGroupCopy(mtg->time_group, a);
    copy->timetable_monitor =
      KheResourceTimetableMonitorCopyPhase1(mtg->timetable_monitor, a);
    HaArrayInit(copy->monitors, a);
    if( DEBUG3 )
      fprintf(stderr, "  KheMonitoredTimeGroupCopyPhase1(%p) copy = %p, "
	"&copy->monitors = %p\n", (void *) mtg, (void *) copy,
	(void *) &copy->monitors);
    HaArrayForEach(mtg->monitors, m, i)
    {
      if( DEBUG3 )
      {
	fprintf(stderr,
	  "  KheMonitoredTimeGroupCopyPhase1(%p) copying monitor ",
	  (void *) mtg);
	KheMonitorDebug(m, 2, 0, stderr);
      }
      m_copy = KheMonitorCopyPhase1(m, a);
      if( DEBUG3 )
      {
	fprintf(stderr,
	  "  KheMonitoredTimeGroupCopyPhase1(%p) copied monitor %p\n",
	  (void *) mtg, (void *) m_copy);
	/* *** can't do this, m_copy might not be ready
	KheMonitorDebug(m_copy, 2, 0, stderr);
	*** */
      }
      HaArrayAddLast(copy->monitors, m_copy);
    }
    HaArrayInit(copy->monitor_indexes, a);
    HaArrayForEach(mtg->monitor_indexes, mi, i)
      HaArrayAddLast(copy->monitor_indexes, mi);
    copy->state = mtg->state;
    copy->busy_and_idle = mtg->busy_and_idle;
    SSetCopy(copy->busy_set, mtg->busy_set, a);
    copy->sweep_time = mtg->sweep_time;
    copy->sweep_index = mtg->sweep_index;
    copy->copy = NULL;
    if( DEBUG3 )
      fprintf(stderr, "] KheMonitoredTimeGroupCopyPhase1 returning %p\n",
	(void *) mtg->copy);
  }
  return mtg->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTimeGroupMonitorCopyPhase2(KHE_TIME_GROUP_MONITOR tgm)           */
/*                                                                           */
/*  Carry out Phase 2 of the copying of tgm.                                 */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupCopyPhase2(KHE_MONITORED_TIME_GROUP mtg)
{
  int i;  KHE_MONITOR m;
  if( mtg->copy != NULL )
  {
    mtg->copy = NULL;
    KheResourceTimetableMonitorCopyPhase2(mtg->timetable_monitor);
    HaArrayForEach(mtg->monitors, m, i)
      KheMonitorCopyPhase2(m);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP KheMonitoredTimeGroupTimeGroup(                           */
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Return the time group being monitored by mtg.                            */
/*                                                                           */
/*****************************************************************************/

KHE_TIME_GROUP KheMonitoredTimeGroupTimeGroup(KHE_MONITORED_TIME_GROUP mtg)
{
  return mtg->time_group;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheMonitoredTimeGroupTimetableMonitor(    */
/*    KHE_MONITORED_TIME_GROUP tgm)                                          */
/*                                                                           */
/*  Return the enclosing timetable monitor of tgm.                           */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TIMETABLE_MONITOR KheMonitoredTimeGroupTimetableMonitor(
  KHE_MONITORED_TIME_GROUP tgm)
{
  return tgm->timetable_monitor;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "state changes (private)"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupUnattachedToBusy(KHE_MONITORED_TIME_GROUP mtg) */
/*                                                                           */
/*  Change state from unattached to monitoring busy only.                    */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupUnattachedToBusy(KHE_MONITORED_TIME_GROUP mtg)
{
  HnAssert(mtg->state == KHE_MTG_STATE_UNATTACHED,
    "KheMonitoredTimeGroupUnattachedToBusy internal error 1");
  HnAssert(mtg->busy_and_idle.busy_count == 0,
    "KheMonitoredTimeGroupUnattachedToBusy internal error 2");
  KheResourceTimetableMonitorAttachMonitoredTimeGroup(
    mtg->timetable_monitor, mtg, &mtg->busy_and_idle.busy_count,
    &mtg->busy_and_idle.workload);
  mtg->state = KHE_MTG_STATE_ATTACHED_BUSY;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupBusyToUnattached(KHE_MONITORED_TIME_GROUP mtg) */
/*                                                                           */
/*  Change state from monitoring busy only to unattached.                    */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupBusyToUnattached(KHE_MONITORED_TIME_GROUP mtg)
{
  HnAssert(mtg->state == KHE_MTG_STATE_ATTACHED_BUSY,
    "KheMonitoredTimeGroupBusyToUnattached internal error 1");
  KheResourceTimetableMonitorDetachMonitoredTimeGroup(
    mtg->timetable_monitor, mtg);
  mtg->busy_and_idle.busy_count = 0;
  mtg->busy_and_idle.workload = 0.0;
  mtg->state = KHE_MTG_STATE_UNATTACHED;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheIdleTimes(KHE_MONITORED_TIME_GROUP mtg)                           */
/*                                                                           */
/*  Return the number of idle times, assuming we are monitoring idle times.  */
/*                                                                           */
/*****************************************************************************/

static int KheIdleTimes(KHE_MONITORED_TIME_GROUP mtg)
{
  if( mtg->busy_and_idle.busy_count > 1 )
    return SSetMax(mtg->busy_set) - SSetMin(mtg->busy_set) + 1 -
      mtg->busy_and_idle.busy_count;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupBusyToBusyAndIdle(KHE_MONITORED_TIME_GROUP mtg)*/
/*                                                                           */
/*  Change state from monitoring busy only to monitoring busy and idle.      */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupBusyToBusyAndIdle(KHE_MONITORED_TIME_GROUP mtg)
{
  int i;  KHE_TIME t;  KHE_RESOURCE_TIMETABLE_MONITOR tm;
  HnAssert(mtg->state == KHE_MTG_STATE_ATTACHED_BUSY,
    "KheMonitoredTimeGroupBusyToBusyAndIdle internal error");
  HnAssert(mtg->busy_and_idle.idle_count == 0,
    "KheMonitoredTimeGroupBusyToBusyAndIdle internal error 2");
  SSetClear(mtg->busy_set);
  tm = mtg->timetable_monitor;
  for( i = 0;  i < KheTimeGroupTimeCount(mtg->time_group);  i++ )
  {
    t = KheTimeGroupTime(mtg->time_group, i);
    if( KheResourceTimetableMonitorTimeTaskCount(tm, t) > 0 )
      SSetInsert(mtg->busy_set, i);  /* NB *position* in time group */
  }
  mtg->busy_and_idle.idle_count = KheIdleTimes(mtg);
  mtg->state = KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupBusyAndIdleToBusy(KHE_MONITORED_TIME_GROUP mtg)*/
/*                                                                           */
/*  Finish monitoring idle times in mtg.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupBusyAndIdleToBusy(KHE_MONITORED_TIME_GROUP mtg)
{
  HnAssert(mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE,
    "KheMonitoredTimeGroupBusyAndIdleToBusy internal error");
  SSetClear(mtg->busy_set);
  mtg->busy_and_idle.idle_count = 0;
  mtg->state = KHE_MTG_STATE_ATTACHED_BUSY;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupAttachMonitor(KHE_MONITORED_TIME_GROUP mtg,    */
/*    KHE_MONITOR m, int index)                                              */
/*                                                                           */
/*  Attach m to mtg.  This means that m is becoming an attached monitor,     */
/*  and that mtg must be attached to tm and tm must be attached.             */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupAttachMonitor(KHE_MONITORED_TIME_GROUP mtg,
  KHE_MONITOR m, int index)
{
  /* make sure mtg is monitoring busy times at least */
  if( mtg->state == KHE_MTG_STATE_UNATTACHED )
    KheMonitoredTimeGroupUnattachedToBusy(mtg);

  /* make sure mtg is monitoring idle times, if required */
  if( KheMonitorTag(m) == KHE_LIMIT_IDLE_TIMES_MONITOR_TAG &&
      mtg->state != KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
    KheMonitoredTimeGroupBusyToBusyAndIdle(mtg);

  /* add m and its index to mtg */
  HaArrayAddLast(mtg->monitors, m);
  HaArrayAddLast(mtg->monitor_indexes, index);

  /* report busy and idle back to m */
  KheMonitorAddBusyAndIdle(m, index, &mtg->busy_and_idle);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitoredTimeGroupHasLimitIdleTimesMonitor(                      */
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Return true when at least one of the monitors of mtg is a limit idle     */
/*  times monitor.                                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitoredTimeGroupHasLimitIdleTimesMonitor(
  KHE_MONITORED_TIME_GROUP mtg)
{
  KHE_MONITOR m;  int i;
  HaArrayForEach(mtg->monitors, m, i)
    if( KheMonitorTag(m) == KHE_LIMIT_IDLE_TIMES_MONITOR_TAG )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupDetachMonitor(KHE_MONITORED_TIME_GROUP mtg,    */
/*    KHE_MONITOR m, int index)                                              */
/*                                                                           */
/*  Detach m from mtg.  This means that m is becoming a unattached monitor.  */
/*                                                                           */
/*  NB mtg must be attached, because it is attached whenever at least one    */
/*  monitor is attached to it, and m is such a monitor.                      */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupDetachMonitor(KHE_MONITORED_TIME_GROUP mtg,
  KHE_MONITOR m, int index)
{
  int pos;

  /* delete busy and idle from m */
  HnAssert(mtg->state != KHE_MTG_STATE_UNATTACHED,
    "KheMonitoredTimeGroupDetachMonitor: internal error");
  KheMonitorDeleteBusyAndIdle(m, index, &mtg->busy_and_idle);

  /* remove m and its index from mtg */
  if( !HaArrayContains(mtg->monitors, m, &pos) )
    HnAbort("KheMonitoredTimeGroupDetachMonitor: m not present");
  HaArrayDeleteAndShift(mtg->monitors, pos);
  HaArrayDeleteAndShift(mtg->monitor_indexes, pos);

  /* make sure mtg is no longer monitoring idle times, if not required */
  if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE &&
      !KheMonitoredTimeGroupHasLimitIdleTimesMonitor(mtg) )
    KheMonitoredTimeGroupBusyAndIdleToBusy(mtg);

  /* make sure mtg is no longer attached, if not requred */
  if( HaArrayCount(mtg->monitors) == 0 )
    KheMonitoredTimeGroupBusyToUnattached(mtg);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring"                                                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupFlush(KHE_MONITORED_TIME_GROUP mtg,            */
/*    KHE_BUSY_AND_IDLE old_busy_and_idle)                                   */
/*                                                                           */
/*  Report a change of state in mtg to its attached monitors.                */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupFlush(KHE_MONITORED_TIME_GROUP mtg,
  KHE_BUSY_AND_IDLE old_busy_and_idle)
{
  int i;  KHE_MONITOR m;
  HaArrayForEach(mtg->monitors, m, i)
  {
    if( DEBUG1(m) )
      fprintf(stderr, "KheMonitoredTimeGroupFlush(%s, %s, idle %d)\n",
	KheTimeGroupId(mtg->time_group), SSetShow(mtg->busy_set),
	mtg->busy_and_idle.idle_count);
    KheMonitorChangeBusyAndIdle(m, HaArray(mtg->monitor_indexes, i),
      old_busy_and_idle, &mtg->busy_and_idle);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupAssignNonClash(KHE_MONITORED_TIME_GROUP mtg,   */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform mtg that in the enclosing timetable, the time with this index     */
/*  number (which lies in mtg's time group) has changed from being           */
/*  unoccupied to being occupied.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitoredTimeGroupAssign below
void KheMonitoredTimeGroupAssignNonClash(KHE_MONITORED_TIME_GROUP mtg,
  int assigned_time_index)
{
  int old_busy_count, old_idle_count;
  old_busy_count = mtg->busy_count;
  old_idle_count = mtg->idle_count;
  mtg->busy_count++;
  if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
  {
    SSetInsert(mtg->busy_set,
      KheTimeGroupTimePos(mtg->time_group, assigned_time_index));
    mtg->idle_count = KheIdleTimes(mtg);
  }
  KheMonitoredTimeGroupFlush(mtg, old_busy_count, old_idle_count);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupUnAssignNonClash(KHE_MONITORED_TIME_GROUP mtg, */
/*    int assigned_time_index)                                               */
/*                                                                           */
/*  Inform mtg that in the enclosing timetable, the time with this index     */
/*  number (which lies in mtg's time group) has changed from being           */
/*  occupied to being unoccupied.                                            */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitoredTimeGroupUnAssign below
void KheMonitoredTimeGroupUnAssignNonClash(KHE_MONITORED_TIME_GROUP mtg,
  int assigned_time_index)
{
  int old_busy_count, old_idle_count;
  HnAssert(mtg->busy_count > 0,
    "KheMonitoredTimeGroupUnAssignNonClash internal error");
  old_busy_count = mtg->busy_count;
  old_idle_count = mtg->idle_count;
  mtg->busy_count--;
  if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
  {
    LSetDelete(mtg->busy_set,
      KheTimeGroupTimePos(mtg->time_group, assigned_time_index));
    mtg->idle_count = KheIdleTimes(mtg);
  }
  KheMonitoredTimeGroupFlush(mtg, old_busy_count, old_idle_count);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupAssign(KHE_MONITORED_TIME_GROUP mtg,           */
/*    KHE_TASK task, int assigned_time_index, int busy_count_before)         */
/*                                                                           */
/*  Inform mtg that in the enclosing timetable, the time with this index     */
/*  number (which lies in mtg's time group) has been assigned task.  Here    */
/*  busy_count_before is the number of tasks that were running at this       */
/*  time before task was assigned.                                           */
/*                                                                           */
/*  Implementation note.  For attached monitors other than limit workload    */
/*  monitors, this change is only of interest when the time goes from        */
/*  unoccupied to occupied (that is, when busy_count_before == 0).           */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupAssign(KHE_MONITORED_TIME_GROUP mtg,
  KHE_TASK task, int assigned_time_index, int busy_count_before)
{
  struct khe_busy_and_idle_rec old_busy_and_idle;
  old_busy_and_idle = mtg->busy_and_idle;
  mtg->busy_and_idle.workload += KheTaskWorkloadPerTime(task);
  if( busy_count_before == 0 )
  {
    mtg->busy_and_idle.busy_count++;
    if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
    {
      SSetInsert(mtg->busy_set,
	KheTimeGroupTimePos(mtg->time_group, assigned_time_index));
      mtg->busy_and_idle.idle_count = KheIdleTimes(mtg);
    }
  }
  KheMonitoredTimeGroupFlush(mtg, &old_busy_and_idle);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupUnAssign(KHE_MONITORED_TIME_GROUP mtg,         */
/*    KHE_TASK task, int assigned_time_index, int busy_count_after)          */
/*                                                                           */
/*  Inform mtg that in the enclosing timetable, the time with this index     */
/*  number (which lies in mtg's time group) has been unassigned task.  Here  */
/*  busy_count_after is the number of tasks that are running at this         */
/*  time after task is unassigned.                                           */
/*                                                                           */
/*  Implementation note.  For attached monitors other than limit workload    */
/*  monitors, this change is only of interest when the time goes from        */
/*  occupied to unoccupied (that is, when busy_count_after == 0).            */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupUnAssign(KHE_MONITORED_TIME_GROUP mtg,
  KHE_TASK task, int assigned_time_index, int busy_count_after)
{
  struct khe_busy_and_idle_rec old_busy_and_idle;
  old_busy_and_idle = mtg->busy_and_idle;
  mtg->busy_and_idle.workload -= KheTaskWorkloadPerTime(task);
  if( busy_count_after == 0 )
  {
    HnAssert(mtg->busy_and_idle.busy_count > 0,
      "KheMonitoredTimeGroupUnAssignNonClash internal error");
    mtg->busy_and_idle.busy_count--;
    if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
    {
      SSetDelete(mtg->busy_set,
	KheTimeGroupTimePos(mtg->time_group, assigned_time_index));
      mtg->busy_and_idle.idle_count = KheIdleTimes(mtg);
    }
  }
  KheMonitoredTimeGroupFlush(mtg, &old_busy_and_idle);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupSetSweepTime(KHE_MONITORED_TIME_GROUP mtg,     */
/*    KHE_TIME t)                                                            */
/*                                                                           */
/*  Set the sweep time of mtg to t.  This will cause a flush if it           */
/*  makes a change.                                                          */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupSetSweepTime(KHE_MONITORED_TIME_GROUP mtg,
  KHE_TIME t)
{
  KHE_TIME t2;  int i, count;  struct khe_busy_and_idle_rec old_busy_and_idle;
  if( t != mtg->sweep_time )
  {
    /* find i, the index of the first time after t */
    count = KheTimeGroupTimeCount(mtg->time_group);
    if( t == NULL )
    {
      /* this means "before first time" */
      i = 0;
    }
    else
    {
      /* initialize search spot */
      if( mtg->sweep_time != NULL &&
	  KheTimeIndex(t) > KheTimeIndex(mtg->sweep_time) )
	i = mtg->sweep_index;
      else
	i = 0;

      /* search forwards for the right time and index */
      for( ;  i < count;  i++ )
      {
	t2 = KheTimeGroupTime(mtg->time_group, i);
	if( KheTimeIndex(t2) > KheTimeIndex(t) )
	  break;
      }
    }

    /* update sweep index, sweep time, and open count */
    old_busy_and_idle = mtg->busy_and_idle;
    mtg->sweep_index = i;
    mtg->sweep_time = t;
    mtg->busy_and_idle.open_count = count - i;

    /* and flush */
    if( DEBUG2(mtg) )
      fprintf(stderr, "  KheMonitoredTimeGroupSetSweepTime(%-5s) %s, %s flush "
	"old(%3d, %6.1f, %3d) -> new(%3d, %6.1f, %3d)\n",
	t == NULL ? "NULL" : KheTimeId(t),
	KheResourceId(KheMonitoredTimeGroupResource(mtg)),
	KheTimeGroupId(mtg->time_group),
        old_busy_and_idle.busy_count, old_busy_and_idle.workload,
        old_busy_and_idle.open_count, mtg->busy_and_idle.busy_count,
        mtg->busy_and_idle.workload, mtg->busy_and_idle.open_count);
    KheMonitoredTimeGroupFlush(mtg, &old_busy_and_idle);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_BUSY_AND_IDLE KheMonitoredTimeGroupBusyAndIdle(                      */
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Return the current busy and idle value of mtg.                           */
/*                                                                           */
/*****************************************************************************/

KHE_BUSY_AND_IDLE KheMonitoredTimeGroupBusyAndIdle(
  KHE_MONITORED_TIME_GROUP mtg)
{
  return &mtg->busy_and_idle;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitoredTimeGroupBusyCount(KHE_MONITORED_TIME_GROUP mtg)         */
/*                                                                           */
/*  Return the current number of busy times.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitoredTimeGroupBusyAndIdle
int KheMonitoredTimeGroupBusyCount(KHE_MONITORED_TIME_GROUP mtg)
{
  return mtg->busy_and_idle.busy_count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitoredTimeGroupOpenCount(KHE_MONITORED_TIME_GROUP mtg)         */
/*                                                                           */
/*  Return the current number of open times.                                 */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitoredTimeGroupBusyAndIdle
int KheMonitoredTimeGroupOpenCount(KHE_MONITORED_TIME_GROUP mtg)
{
  return mtg->busy_and_idle.open_count;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  float KheMonitoredTimeGroupWorkload(KHE_MONITORED_TIME_GROUP mtg)        */
/*                                                                           */
/*  Return the current workload of mtg.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheMonitoredTimeGroupBusyAndIdle
float KheMonitoredTimeGroupWorkload(KHE_MONITORED_TIME_GROUP mtg)
{
  return mtg->busy_and_idle.workload;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupIdleState(KHE_MONITORED_TIME_GROUP mtg,        */
/*    int *busy_count, int *idle_count,                                      */
/*    KHE_TIME extreme_busy_times[2], int *extreme_busy_times_count)         */
/*                                                                           */
/*  Report the state of mtg when it is monitoring busy and idle:  the        */
/*  number of busy times and idle times, and the first and last busy times.  */
/*  This is only called when monitoring busy and idle.                       */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupIdleState(KHE_MONITORED_TIME_GROUP mtg,
  int *busy_count, int *idle_count,
  KHE_TIME extreme_busy_times[2], int *extreme_busy_times_count)
{
  KHE_TIME first, last;
  HnAssert( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE,
    "KheMonitoredTimeGroupIdleState: mtg not monitoring idle");
  *busy_count = mtg->busy_and_idle.busy_count;
  *idle_count = mtg->busy_and_idle.idle_count;
  *extreme_busy_times_count = 0;
  if( mtg->busy_and_idle.busy_count > 0 )
  {
    first = KheTimeGroupTime(mtg->time_group, SSetMin(mtg->busy_set));
    last  = KheTimeGroupTime(mtg->time_group, SSetMax(mtg->busy_set));
    extreme_busy_times[(*extreme_busy_times_count)++] = first;
    if( last != first )
      extreme_busy_times[(*extreme_busy_times_count)++] = last;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "at max limit count"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheMonitoredTimeGroupAtMaxLimitCount(KHE_MONITORED_TIME_GROUP mtg)   */
/*                                                                           */
/*  Return the total at max limit count of the monitors of mtg.              */
/*                                                                           */
/*****************************************************************************/

int KheMonitoredTimeGroupAtMaxLimitCount(KHE_MONITORED_TIME_GROUP mtg)
{
  int res, i;  KHE_MONITOR m;
  res = 0;
  HaArrayForEach(mtg->monitors, m, i)
    switch( KheMonitorTag(m) )
    {
      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	res += KheClusterBusyTimesMonitorAtMaxLimitCount(
	  (KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
	break;

      case KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG:

	res += KheLimitActiveIntervalsMonitorAtMaxLimitCount(
	  (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m);
	break;

      default:

	break;
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitoredTimeGroupAvailable(KHE_MONITORED_TIME_GROUP mtg)        */
/*                                                                           */
/*  Return true if there are no avoid unavailable times monitors.            */
/*                                                                           */
/*****************************************************************************/

bool KheMonitoredTimeGroupAvailable(KHE_MONITORED_TIME_GROUP mtg)
{
  int i;  KHE_MONITOR m;
  HaArrayForEach(mtg->monitors, m, i)
    if( KheMonitorTag(m) == KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "range"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool InRange(KHE_TIME ft, KHE_TIME lt, int first_time_index,             */
/*    int last_time_index)                                                   */
/*                                                                           */
/*  Return true if [ft .. lt] lies in [first_time_index .. last_time_index]. */
/*                                                                           */
/*****************************************************************************/

static bool InRange(KHE_TIME ft, KHE_TIME lt, int first_time_index,
  int last_time_index)
{
  return first_time_index <= KheTimeIndex(ft) &&
    KheTimeIndex(lt) <= last_time_index;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupAddRange(KHE_MONITORED_TIME_GROUP mtg,         */
/*    KHE_RESOURCE_TYPE rt, int first_time_index, int last_time_index,       */
/*    KHE_GROUP_MONITOR gm)                                                  */
/*                                                                           */
/*  Add as children to gm all limit busy times and cluster busy times        */
/*  monitors that are derived from constraints that monitor all resources    */
/*  of type rt, and monitor mtg at times limited to between first_time_index */
/*  and last_time_index.  Do not add any monitors that are already there.    */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupAddRange(KHE_MONITORED_TIME_GROUP mtg,
  KHE_RESOURCE_TYPE rt, int first_time_index, int last_time_index,
  bool include_avoid_unavailable_times, KHE_GROUP_MONITOR gm)
{
  int i, count;  KHE_MONITOR m;  KHE_TIME ft, lt;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;
  count = KheResourceTypeResourceCount(rt) / 2;
  HaArrayForEach(mtg->monitors, m, i)
    switch( KheMonitorTag(m) )
    {
      case KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG:

	cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	if( KheClusterBusyTimesConstraintResourceOfTypeCount(cbtc,rt) >= count
	    && KheClusterBusyTimesMonitorSweepTimeRange(cbtm, &ft, &lt)
	    && InRange(ft, lt, first_time_index, last_time_index)
	    && !KheGroupMonitorHasChildMonitor(gm, m) )
	  KheGroupMonitorAddChildMonitor(gm, m);
	break;

      case KHE_LIMIT_BUSY_TIMES_MONITOR_TAG:

	lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
	if( KheLimitBusyTimesConstraintResourceOfTypeCount(lbtc, rt) >= count
	    && KheLimitBusyTimesMonitorSweepTimeRange(lbtm, &ft, &lt)
	    && InRange(ft, lt, first_time_index, last_time_index)
	    && !KheGroupMonitorHasChildMonitor(gm, m) )
	  KheGroupMonitorAddChildMonitor(gm, m);
	break;

      case KHE_AVOID_UNAVAILABLE_TIMES_MONITOR_TAG:

	if( include_avoid_unavailable_times &&
	    !KheGroupMonitorHasChildMonitor(gm, m) )
	  KheGroupMonitorAddChildMonitor(gm, m);
	break;

      default:

	break;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupDebug(KHE_MONITORED_TIME_GROUP mtg,            */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of mtg onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMonitoredTimeGroupDebug(KHE_MONITORED_TIME_GROUP mtg,
  int verbosity, int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheTimeGroupDebug(mtg->time_group, 1, -1, fp);
    fprintf(fp, " (%d busy", mtg->busy_and_idle.busy_count);
    if( mtg->state == KHE_MTG_STATE_ATTACHED_BUSY_AND_IDLE )
      fprintf(fp, ", %d idle", mtg->busy_and_idle.idle_count);
    fprintf(fp, ") ]");
  }
}


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
/*  FILE:         khe_resource_timetable_monitor.c                           */
/*  DESCRIPTION:  A resource timetable monitor                               */
/*                                                                           */
/*****************************************************************************/
#include "khe_interns.h"

#define DEBUG_CELL 1
#define DEBUG4 0
#define DEBUG4_RESOURCE "T5"
#define DEBUG5 0

#define DO_DEBUG4 (DEBUG4 && rtm->resource_in_soln != NULL &&		\
  strcmp(KheResourceInSolnId(rtm->resource_in_soln), DEBUG4_RESOURCE) == 0)


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
  ARRAY_KHE_TASK		tasks;			/* incidences        */
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups;	/* monitored tg's    */
#if DEBUG_CELL
  bool				debug;			/* debug this cell   */
#endif
  KHE_TIME_CELL			copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  MONITORED_TIME_GROUP_TABLE                                               */
/*                                                                           */
/*  A table of monitored time groups.  Each monitored time group is          */
/*  indexed by the first time in its time group, or if the time group        */
/*  is empty its index is 0.                                                 */
/*                                                                           */
/*  Important point (i.e. it caused a bug):  an element of mtgt->entries     */
/*  may be NULL, meaning that there are no time groups monitoring that time. */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtgt_entry_rec {
  ARRAY_KHE_MONITORED_TIME_GROUP monitored_time_groups;
} *KHE_MTGT_ENTRY;

typedef HA_ARRAY(KHE_MTGT_ENTRY) ARRAY_KHE_MTGT_ENTRY;

typedef struct khe_monitored_time_group_table_rec {
  ARRAY_KHE_MTGT_ENTRY	entries;
} *KHE_MONITORED_TIME_GROUP_TABLE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR - a resource timetable monitor            */
/*                                                                           */
/*  Attached_monitor_count counts both monitors and monitored time groups    */
/*                                                                           */
/*****************************************************************************/

struct khe_resource_timetable_monitor_rec {
  INHERIT_MONITOR(unused1, unused2)
  KHE_RESOURCE_IN_SOLN 		resource_in_soln;	/* resource          */
  ARRAY_KHE_TIME_CELL		time_cells;		/* time cells        */
  ARRAY_KHE_AVOID_CLASHES_MONITOR avoid_clashes_monitors; /* avoid clash m's */
  KHE_MONITORED_TIME_GROUP_TABLE mtg_table;		/* monitored tg's    */
  ARRAY_KHE_TIME		clashing_times;		/* clashing times    */
  int				attached_monitor_count;	/* attached monitors */
  KHE_RESOURCE_TIMETABLE_MONITOR copy;			/* used when copying */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitored time group table" (private)                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP_TABLE KheMonitoredTimeGroupTableMake(void)      */
/*                                                                           */
/*  Make a new, empty monitored time group table.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITORED_TIME_GROUP_TABLE KheMonitoredTimeGroupTableMake(HA_ARENA a)
{
  KHE_MONITORED_TIME_GROUP_TABLE res;
  HaMake(res, a);
  HaArrayInit(res->entries, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITORED_TIME_GROUP_TABLE KheMonitoredTimeGroupTableCopyPhase1(     */
/*    KHE_MONITORED_TIME_GROUP_TABLE mtgt, HA_ARENA a)                       */
/*                                                                           */
/*  Carry out Phase 1 of copying mtgt.                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITORED_TIME_GROUP_TABLE KheMonitoredTimeGroupTableCopyPhase1(
  KHE_MONITORED_TIME_GROUP_TABLE mtgt, HA_ARENA a)
{
  KHE_MONITORED_TIME_GROUP_TABLE copy;  KHE_MONITORED_TIME_GROUP mtg, mtg_copy;
  int i, j;  KHE_MTGT_ENTRY entry, entry_copy;
  HaMake(copy, a);
  HaArrayInit(copy->entries, a);
  HaArrayForEach(mtgt->entries, entry, i)
  {
    if( entry != NULL )
    {
      HaMake(entry_copy, a);
      HaArrayInit(entry_copy->monitored_time_groups, a);
      HaArrayForEach(entry->monitored_time_groups, mtg, j)
      {
	mtg_copy = KheMonitoredTimeGroupCopyPhase1(mtg, a);
	HaArrayAddLast(entry_copy->monitored_time_groups, mtg_copy);
      }
    }
    else
      entry_copy = NULL;
    HaArrayAddLast(copy->entries, entry_copy);  /* JeffK bug fix here */
  }
  return copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupTableCopyPhase2(                               */
/*    KHE_MONITORED_TIME_GROUP_TABLE mtgt)                                   */
/*                                                                           */
/*  Carry out Phase 2 of copying mtgt.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupTableCopyPhase2(
  KHE_MONITORED_TIME_GROUP_TABLE mtgt)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i, j;  KHE_MTGT_ENTRY entry;
  HaArrayForEach(mtgt->entries, entry, i)
    if( entry != NULL )
      HaArrayForEach(entry->monitored_time_groups, mtg, j)
	KheMonitoredTimeGroupCopyPhase2(mtg);
}


/*****************************************************************************/
/*                                                                           */
/* void KheMonitoredTimeGroupTableDelete(KHE_MONITORED_TIME_GROUP_TABLE mtgt)*/
/*                                                                           */
/*  Delete mtgt, including deleting the monitored time groups it holds.      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMonitoredTimeGroupTableDelete(
  KHE_MONITORED_TIME_GROUP_TABLE mtgt)
{
  KHE_MONITORED_TIME_GROUP mtg;  int i, j;  KHE_MTGT_ENTRY entry;
  HaArrayForEach(mtgt->entries, entry, i)
    if( entry != NULL )
    {
      HaArrayForEach(entry->monitored_time_groups, mtg, j)
	KheMonitoredTimeGroupDelete(mtg);
      MArrayFree(entry->monitored_time_groups);
      MFree(entry);
    }
  MArrayFree(mtgt->entries);
  MFree(mtgt);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitoredTimeGroupTableKey(KHE_TIME_GROUP tg)                     */
/*                                                                           */
/*  Return the key for searching for tg in a monitored time group table.     */
/*                                                                           */
/*****************************************************************************/

static int KheMonitoredTimeGroupTableKey(KHE_TIME_GROUP tg)
{
  return KheTimeGroupTimeCount(tg) == 0 ? 0 :
    KheTimeIndex(KheTimeGroupTime(tg, 0));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitoredTimeGroupTableRetrieve(                                 */
/*    KHE_MONITORED_TIME_GROUP_TABLE mtgt, KHE_TIME_GROUP tg,                */
/*    KHE_MONITORED_TIME_GROUP *mtg)                                         */
/*                                                                           */
/*  Retrieve tg from mtgt.                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitoredTimeGroupTableRetrieve(
  KHE_MONITORED_TIME_GROUP_TABLE mtgt, KHE_TIME_GROUP tg,
  KHE_MONITORED_TIME_GROUP *mtg)
{
  int key, i;  KHE_MTGT_ENTRY entry;  KHE_MONITORED_TIME_GROUP mtg2;
  key = KheMonitoredTimeGroupTableKey(tg);
  if( key < HaArrayCount(mtgt->entries) )
  {
    entry = HaArray(mtgt->entries, key);
    if( entry != NULL )
      HaArrayForEach(entry->monitored_time_groups, mtg2, i)
	if( KheTimeGroupEqual(KheMonitoredTimeGroupTimeGroup(mtg2), tg) )
	  return *mtg = mtg2, true;
  }
  return *mtg = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMonitoredTimeGroupTableInsert(                                   */
/*    KHE_MONITORED_TIME_GROUP_TABLE mtgt, KHE_TIME_GROUP tg,                */
/*    KHE_MONITORED_TIME_GROUP mtg)                                          */
/*                                                                           */
/*  Insert mtg with key tg into mtgt.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheMonitoredTimeGroupTableInsert(
  KHE_MONITORED_TIME_GROUP_TABLE mtgt, KHE_TIME_GROUP tg,
  KHE_MONITORED_TIME_GROUP mtg, HA_ARENA a)
{
  int key;  KHE_MTGT_ENTRY entry;
  key = KheMonitoredTimeGroupTableKey(tg);
  HaArrayFill(mtgt->entries, key + 1, NULL);
  entry = HaArray(mtgt->entries, key);
  if( entry == NULL )
  {
    HaMake(entry, a);
    HaArrayInit(entry->monitored_time_groups, a);
    HaArrayPut(mtgt->entries, key, entry);
  }
  HaArrayAddLast(entry->monitored_time_groups, mtg);
}


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
  HaArrayInit(res->tasks, a);
  HaArrayInit(res->monitored_time_groups, a);
#if DEBUG_CELL
  res->debug = false;
#endif
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
  KHE_TIME_CELL copy;  KHE_TASK task;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( tc->copy == NULL )
  {
    HaMake(copy, a);
    tc->copy = copy;
    copy->time = tc->time;
    HaArrayInit(copy->tasks, a);
    HaArrayForEach(tc->tasks, task, i)
      HaArrayAddLast(copy->tasks, KheTaskCopyPhase1(task, a));
    HaArrayInit(copy->monitored_time_groups, a);
    HaArrayForEach(tc->monitored_time_groups, mtg, i)
      HaArrayAddLast(copy->monitored_time_groups,
	KheMonitoredTimeGroupCopyPhase1(mtg, a));
#if DEBUG_CELL
    copy->debug = tc->debug;
#endif
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
  KHE_TASK task;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( tc->copy != NULL )
  {
    tc->copy = NULL;
    HaArrayForEach(tc->tasks, task, i)
      KheTaskCopyPhase2(task);
    HaArrayForEach(tc->monitored_time_groups, mtg, i)
      KheMonitoredTimeGroupCopyPhase2(mtg);
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
  MArrayFree(tc->tasks);
  MArrayFree(tc->monitored_time_groups);
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
  KHE_TASK task;  KHE_MONITORED_TIME_GROUP mtg;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ %s", KheTimeId(tc->time) == NULL ? "-" :
      KheTimeId(tc->time));
    HaArrayForEach(tc->tasks, task, i)
    {
      fprintf(fp, i == 0 ? ": " : " ");
      KheTaskDebug(task, 1, -1, fp);
    }
    if( indent >= 0 && HaArrayCount(tc->monitored_time_groups) > 0 )
    {
      if( HaArrayCount(tc->monitored_time_groups) > 0 )
      {
	fprintf(fp, "\n");
	if( verbosity >= 2 )
	{
	  HaArrayForEach(tc->monitored_time_groups, mtg, i)
	    KheMonitoredTimeGroupDebug(mtg, 1, indent + 2, fp);
	}
      }
      fprintf(fp, "%*s]", indent, "");
    }
    else
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
/*  void KheResourceTimetableMonitorAddTimeCells(                            */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, HA_ARENA a)                        */
/*                                                                           */
/*  Add time cells to rtm, if there are none already.  Time cells are added  */
/*  only when time cells are actually needed, as an optimization.            */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableMonitorAddTimeCells(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KHE_INSTANCE ins;  int i;  KHE_TIME_CELL tc;  HA_ARENA a;
  if( HaArrayCount(rtm->time_cells) == 0 )
  {
    ins = KheSolnInstance(rtm->soln);
    a = KheSolnArena(rtm->soln);
    for( i = 0;  i < KheInstanceTimeCount(ins);  i++ )
    {
      tc = KheTimeCellMake(KheInstanceTime(ins, i), a);
      HaArrayAddLast(rtm->time_cells, tc);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitorMake(          */
/*    KHE_SOLN soln, KHE_RESOURCE_IN_SOLN rs)                                */
/*                                                                           */
/*  Make a new timetable monitor for soln, monitoring rs.                    */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitorMake(KHE_SOLN soln,
  KHE_RESOURCE_IN_SOLN rs)
{
  KHE_RESOURCE_TIMETABLE_MONITOR res;  HA_ARENA a;
  if( DEBUG4 && rs != NULL &&
      strcmp(KheResourceInSolnId(rs), DEBUG4_RESOURCE) == 0 )
    fprintf(stderr, "KheResourceTimetableMonitorMake(soln, %s, -)\n",
      DEBUG4_RESOURCE);
  a = KheSolnArena(soln);
  HaMake(res, a);
  HaArrayInit(res->parent_links, a);
  KheMonitorInitCommonFields((KHE_MONITOR) res, soln,
    KHE_RESOURCE_TIMETABLE_MONITOR_TAG, KHE_LINEAR_COST_FUNCTION, 0);
  res->resource_in_soln = rs;
  HaArrayInit(res->time_cells, a);
  HaArrayInit(res->avoid_clashes_monitors, a);
  res->mtg_table = KheMonitoredTimeGroupTableMake(a);
  HaArrayInit(res->clashing_times, a);
  /* res->busy_times = 0; */
  res->attached_monitor_count = 0;
  res->copy = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheResourceTimetableMonitorSoln(                                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Return rtm's solution attribute.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheResourceTimetableMonitorSoln(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  return KheMonitorSoln((KHE_MONITOR) rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheResourceTimetableMonitorResource(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Return rtm's resource attribute.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheResourceTimetableMonitorResource(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  return KheResourceInSolnResource(rtm->resource_in_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitorCopyPhase1(    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, HA_ARENA a)                        */
/*                                                                           */
/*  Carry out Phase 1 of copying rtm.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TIMETABLE_MONITOR KheResourceTimetableMonitorCopyPhase1(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, HA_ARENA a)
{
  KHE_TIME t;  KHE_TIME_CELL tc;  KHE_AVOID_CLASHES_MONITOR acm;
  int i;  KHE_RESOURCE_TIMETABLE_MONITOR copy;
  if( rtm->copy == NULL )
  {
    HaMake(copy, a);
    rtm->copy = copy;
    KheMonitorCopyCommonFieldsPhase1((KHE_MONITOR) copy, (KHE_MONITOR) rtm, a);
    copy->resource_in_soln =
      KheResourceInSolnCopyPhase1(rtm->resource_in_soln, a);
    HaArrayInit(copy->time_cells, a);
    HaArrayForEach(rtm->time_cells, tc, i)
      HaArrayAddLast(copy->time_cells, KheTimeCellCopyPhase1(tc, a));
    HaArrayInit(copy->avoid_clashes_monitors, a);
    HaArrayForEach(rtm->avoid_clashes_monitors, acm, i)
      HaArrayAddLast(copy->avoid_clashes_monitors,
	KheAvoidClashesMonitorCopyPhase1(acm, a));
    copy->mtg_table = KheMonitoredTimeGroupTableCopyPhase1(rtm->mtg_table, a);
    HaArrayInit(copy->clashing_times, a);
    HaArrayForEach(rtm->clashing_times, t, i)
      HaArrayAddLast(copy->clashing_times, t);
    /* copy->busy_times = rtm->busy_times; */
    copy->attached_monitor_count = rtm->attached_monitor_count;
    copy->copy = NULL;
  }
  return rtm->copy;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorCopyPhase2(                              */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Carry out Phase 2 of copying rtm.                                        */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorCopyPhase2(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KHE_TIME_CELL tc;  KHE_AVOID_CLASHES_MONITOR acm;  int i;
  if( rtm->copy != NULL )
  {
    rtm->copy = NULL;
    KheMonitorCopyCommonFieldsPhase2((KHE_MONITOR) rtm);
    HaArrayForEach(rtm->time_cells, tc, i)
      KheTimeCellCopyPhase2(tc);
    HaArrayForEach(rtm->avoid_clashes_monitors, acm, i)
      KheAvoidClashesMonitorCopyPhase2(acm);
    KheMonitoredTimeGroupTableCopyPhase2(rtm->mtg_table);
  }
}


/*****************************************************************************/
/*                                                                           */
/* void KheResourceTimetableMonitorDelete(KHE_RESOURCE_TIMETABLE_MONITOR rtm)*/
/*                                                                           */
/*  Delete rtm.  The monitors attached to it will be deleted separately.     */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheResourceTimetableMonitorDelete(KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KHE_TIME_CELL tc;  int i;
  if( rtm->attached )
    KheResourceTimetableMonitorDetachFromSoln(rtm);
  KheMonitorDeleteAllParentMonitors((KHE_MONITOR) rtm);
  KheSolnDeleteMonitor(rtm->soln, (KHE_MONITOR) rtm);
  MArrayFree(rtm->avoid_clashes_monitors);
  KheMonitoredTimeGroupTableDelete(rtm->mtg_table);
  HaArrayForEach(rtm->time_cells, tc, i)
    KheTimeCellDelete(tc);
  MArrayFree(rtm->time_cells);
  MArrayFree(rtm->clashing_times);
  MFree(rtm);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "reporting state"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTimetableMonitorTimeTaskCount(                            */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time)                     */
/*                                                                           */
/*  Return the number of tasks running at time.                              */
/*                                                                           */
/*****************************************************************************/

int KheResourceTimetableMonitorTimeTaskCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time)
{
  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  tc = HaArray(rtm->time_cells, KheTimeIndex(time));
  return HaArrayCount(tc->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheResourceTimetableMonitorTimeTask(                            */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time, int i)              */
/*                                                                           */
/*  Return the i'th task running at time.                                    */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheResourceTimetableMonitorTimeTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME time, int i)
{
  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  tc = HaArray(rtm->time_cells, KheTimeIndex(time));
  return HaArray(tc->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTimetableMonitorB usyTimes(                               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Return the number of busy times in rtm.                                  */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete; the resource in soln object takes care of this now
int KheResourceTimetableMon itorBusyTimes(KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  return rtm->busy_times;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorTimeAvailable(                           */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MEET meet, KHE_TIME time)      */
/*                                                                           */
/*  Return true if adding meet to rtm with the given starting time, or       */
/*  moving it within tm to that time, would cause no clashes and would       */
/*  not place any part of meet off the end of the timetable.                 */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorTimeAvailable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MEET meet, KHE_TIME time)
{
  KHE_TIME_CELL tc;  int i, j, time_index, durn;  KHE_TASK t2;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  time_index = KheTimeIndex(time);
  durn = KheMeetDuration(meet);
  if( time_index + durn > HaArrayCount(rtm->time_cells) )
    return false;
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(rtm->time_cells, time_index + i);
    HaArrayForEach(tc->tasks, t2, j)
      if( KheTaskMeet(t2) != meet )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorTimeGroupAvailable(                      */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,                 */
/*    bool nocost_off, bool check_avoid_unavailable_times_monitors)          */
/*                                                                           */
/*  Return true if the resource monitored by rtm is free at tg, including    */
/*  not being subject to any avoid unavailable times constraints then.       */
/*                                                                           */
/*  Ordinarily, tasks for which non-assignment has no cost are considered    */
/*  to be free time, given that they can be unassigned at no cost.  But      */
/*  if nocost_off is true, they are considered to be not free time.          */
/*                                                                           */
/*  If check_avoid_unavailable_times_monitors is true, this function also    */
/*  searches for avoid unavailable times monitors.  If it finds any that     */
/*  monitor any of the times of tg, it returns false, since the resource     */
/*  is unavailable at that time, not because it is busy, but because of      */
/*  an avoid unavailable times constraint.                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheResourceTimetableMonitorTimeGroupAvailable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  bool nocost_off, bool check_avoid_unavailable_times_monitors)
{
  int i, j;  KHE_TIME t;  KHE_TIME_CELL tc;  KHE_TASK task;
  KHE_MONITORED_TIME_GROUP mtg;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    HaArrayForEach(tc->tasks, task, j)
      if( nocost_off || KheTaskNeedsAssignment(task) )
	return false;
    if( check_avoid_unavailable_times_monitors )
      HaArrayForEach(tc->monitored_time_groups, mtg, j)
	if( !KheMonitoredTimeGroupAvailable(mtg) )
	  return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorTaskAvailableInFrame(                    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_FRAME frame,    */
/*    KHE_TASK ignore_task)                                                  */
/*                                                                           */
/*  Return true if rtm's resource is free at the times of task, and indeed   */
/*  free at the days that task is running in frame.  If ignore_task is true, */
/*  ignore any task whose proper root is ignore_task.                        */
/*                                                                           */
/*  It is an error for any of the times involved to be outside the frame.    */
/*                                                                           */
/*  It is acceptable for frame to be NULL, in which case that part of        */
/*  the specification is ignored.                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheResourceTimetableMonitorTaskAvailableInFrame(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_FRAME frame,
  KHE_TASK ignore_task)
{
  KHE_TIME t, t2;  int durn, i, j, k, index;  KHE_TIME_GROUP tg, prev_tg;
  KHE_MEET meet;  KHE_TASK child_task, task2;  KHE_TIME_CELL tc;

  ** check availability of task itself **
  KheResourceTimetableMonitorAddTimeCells(rtm);
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      prev_tg = NULL;
      for( i = 0;  i < durn;  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	if( frame != NULL )
	{
	  index = KheFrameTimeIndex(frame, t2);
	  HnAssert(index != -1,"KheResourceTimetableMonitorTaskAvailableInFrame"
	    " internal error (time %s not in frame)", KheTimeId(t2));
	  tg = KheFrameTimeGroup(frame, index);
	  if( tg != prev_tg )
	  {
	    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	    {
	      t = KheTimeGroupTime(tg, j);
	      tc = HaArray(rtm->time_cells, KheTimeIndex(t));
	      HaArrayForEach(tc->tasks, task2, k)
		if( KheTaskProperRoot(task2) != ignore_task )
		  return false;
	    }
	    prev_tg = tg;
	  }
	}
	else
	{
	  tc = HaArray(rtm->time_cells, KheTimeIndex(t2));
	  HaArrayForEach(tc->tasks, task2, k)
	    if( KheTaskProperRoot(task2) != ignore_task )
	      return false;
	}
      }
    }
  }

  ** check availability of the child tasks of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheResourceTimetableMonitorTaskAvailableInFrame(rtm, child_task,
	  frame, ignore_task) )
      return false;
  }

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTimetableMonitorBusyTimesForTimeGroup(                    */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)                 */
/*                                                                           */
/*  Return the number of times during tg that rtm's resource is busy.        */
/*                                                                           */
/*****************************************************************************/

int KheResourceTimetableMonitorBusyTimesForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)
{
  int res, i;  KHE_TIME t;  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  res = 0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    if( HaArrayCount(tc->tasks) > 0 )
      res++;
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorFreeForTime(                             */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t,                        */
/*    KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)       */
/*                                                                           */
/*  Like KheResourceTimetableMonitorFreeForTimeGroup below, only for a       */
/*  single time.                                                             */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorFreeForTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)
{
  return KheResourceTimetableMonitorFreeForTimeGroup(rtm,
    KheTimeSingletonTimeGroup(t), frame, ignore_task, ignore_nocost_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorFreeForTimeGroup(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,                 */
/*    KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)       */
/*                                                                           */
/*  Return true if rtm's resource is free (not assigned) at the times of tg. */
/*                                                                           */
/*  If frame == NULL, to be free to be assigned to task means to be free     */
/*  at every time that task is running.  If frame != NULL, to be free to     */
/*  be assigned to task means to be free at every time of every time group   */
/*  of frame that contains a time that task is running.                      */
/*                                                                           */
/*  If ignore_task != NULL, the function behaves as though rtm contains      */
/*  no trace of ignore_task.  This affects whether it is free or not at      */
/*  the times of ignore_task.  This is useful for checking whether a         */
/*  swap is possible:  ignore_task can be set to the other task.             */
/*                                                                           */
/*  If ignore_nocost_tasks is true, then every task t for which              */
/*  KheTaskNeedsAssignment(t) return false is ignored in the manner of       */
/*  ignore_task.  The reasoning here is that the resource could be           */
/*  assigned at the times of these tasks if they were unassigned, which      */
/*  they can be at no cost.                                                  */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorFreeForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)
{
  int i, j;  KHE_TIME t;  KHE_TIME_CELL tc;  KHE_TASK task;
  KHE_TIME_GROUP tg2, prev_tg2;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  if( frame != NULL )
  {
    prev_tg2 = NULL;
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      tg2 = KheFrameTimeTimeGroup(frame, t);
      if( tg2 != prev_tg2 && !KheResourceTimetableMonitorFreeForTimeGroup(
	    rtm, tg2, NULL, ignore_task, ignore_nocost_tasks) )
	return false;
      prev_tg2 = tg2;
    }
  }
  else
  {
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      tc = HaArray(rtm->time_cells, KheTimeIndex(t));
      HaArrayForEach(tc->tasks, task, j)
      {
	task = KheTaskProperRoot(task);
	if( task == ignore_task )
	{
	  /* ignore this task */
	}
	else if( ignore_nocost_tasks && !KheTaskNeedsAssignment(task) )
	{
	  /* ignore this task */
	}
	else
	{
	  /* don't ignore this task */
	  return false;
	}
      }
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorFreeForTask(                             */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,                     */
/*    KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)       */
/*                                                                           */
/*  Return true if rtm's resource is free (not assigned) at the times that   */
/*  task is running, including tasks assigned to task, direct or indirect.   */
/*  The other parameters are as for KheResourceTimetableMonitorFreeForTask.  */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorFreeForTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,
  KHE_FRAME frame, KHE_TASK ignore_task, bool ignore_nocost_tasks)
{
  KHE_TIME t, t2;  int durn, i;  KHE_MEET meet;  KHE_TASK child_task;

  /* check free for task itself */
  KheResourceTimetableMonitorAddTimeCells(rtm);
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      for( i = 0;  i < durn;  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	if( !KheResourceTimetableMonitorFreeForTimeGroup(rtm,
	      KheTimeSingletonTimeGroup(t2), frame, ignore_task,
	      ignore_nocost_tasks) )
	  return false;
      }
    }
  }

  /* check free for the child tasks of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheResourceTimetableMonitorFreeForTask(rtm, child_task,
	  frame, ignore_task, ignore_nocost_tasks) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorAvailableForTime(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t)                        */
/*                                                                           */
/*  Like KheResourceTimetableMonitorAvailableForTimeGroup, only for a        */
/*  single time t.                                                           */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorAvailableForTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t)
{
  return KheResourceTimetableMonitorAvailableForTimeGroup(rtm,
    KheTimeSingletonTimeGroup(t));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorAvailableForTimeGroup(                   */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)                 */
/*                                                                           */
/*  Return true if rtm's resource is available (not subject to an avoid      */
/*  unavailable times constraint) at the times of tg.                        */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorAvailableForTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)
{
  int i, j;  KHE_TIME t;  KHE_TIME_CELL tc;
  KHE_MONITORED_TIME_GROUP mtg;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    HaArrayForEach(tc->monitored_time_groups, mtg, j)
      if( !KheMonitoredTimeGroupAvailable(mtg) )
	return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorAvailableForTask(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task)                     */
/*                                                                           */
/*  Return true if rtm's resource is available (not subject to an avoid      */
/*  unavailable times constraint) at the times that task is running,         */
/*  including tasks assigned to task, directly or indirectly.                */
/*                                                                           */
/*****************************************************************************/

bool KheResourceTimetableMonitorAvailableForTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task)
{
  KHE_TIME t, t2;  int durn, i, j;  KHE_MONITORED_TIME_GROUP mtg;
  KHE_MEET meet;  KHE_TASK child_task;  KHE_TIME_CELL tc;

  /* check available for task itself */
  KheResourceTimetableMonitorAddTimeCells(rtm);
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      for( i = 0;  i < durn;  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	tc = HaArray(rtm->time_cells, KheTimeIndex(t2));
	HaArrayForEach(tc->monitored_time_groups, mtg, j)
	  if( !KheMonitoredTimeGroupAvailable(mtg) )
	    return false;
      }
    }
  }

  /* check free for the child tasks of task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( !KheResourceTimetableMonitorAvailableForTask(rtm, child_task) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(           */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, KHE_TASK_SET ts)*/
/*                                                                           */
/*  Add the proper roots of the tasks of rtm that overlap with tg to ts.     */
/*  If include_preassigned is true, include preassigned tasks, otherwise     */
/*  omit them.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg,
  bool include_preassigned, KHE_TASK_SET ts)
{
  int i, j, pos;  KHE_TIME t;  KHE_TIME_CELL tc;  KHE_TASK task;
  KHE_RESOURCE r2;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  if( tg == NULL )
    tg = KheInstanceFullTimeGroup(KheSolnInstance(rtm->soln));
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    HaArrayForEach(tc->tasks, task, j)
    {
      task = KheTaskProperRoot(task);
      if( (include_preassigned || !KheTaskIsPreassigned(task, &r2)) &&
          !KheTaskSetContainsTask(ts, task, &pos) )
	KheTaskSetAddTask(ts, task);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAddProperRootTasksInTimeRange(           */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,              */
/*    int last_time_index, bool include_preassigned, KHE_TASK_SET ts)        */
/*                                                                           */
/*  Add the proper roots of the tasks of rtm that lie between these two      */
/*  time indexes.  If include_preassigned is true, include preassigned       */
/*  tasks, otherwise omit them.                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAddProperRootTasksInTimeRange(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,
  int last_time_index, bool include_preassigned, KHE_TASK_SET ts)
{
  int index, j, pos;  KHE_TIME t;  KHE_TIME_CELL tc;  KHE_TASK task;
  KHE_RESOURCE r2;  KHE_INSTANCE ins;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  ins = KheSolnInstance(KheMonitorSoln((KHE_MONITOR) rtm));
  for( index = first_time_index;  index <= last_time_index;  index++ )
  {
    t = KheInstanceTime(ins, index);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    HaArrayForEach(tc->tasks, task, j)
    {
      task = KheTaskProperRoot(task);
      if( (include_preassigned || !KheTaskIsPreassigned(task, &r2)) &&
          !KheTaskSetContainsTask(ts, task, &pos) )
	KheTaskSetAddTask(ts, task);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAddProperRootTasksInInterval(            */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,                   */
/*    int first_frame_index, int last_frame_index,                           */
/*    bool include_preassigned, KHE_TASK_SET ts)                             */
/*                                                                           */
/*  Add the proper roots of the tasks of rtm that lie between these two      */
/*  frame indexes.  If include_preassigned is true, include preassigned      */
/*  tasks, otherwise omit them.                                              */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAddProperRootTasksInInterval(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,
  int first_frame_index, int last_frame_index,
  bool include_preassigned, KHE_TASK_SET ts)
{
  int i;  KHE_TIME_GROUP tg;

  /* make sure first_frame_index and last_frame_index are within range */
  if( first_frame_index < 0 )
    first_frame_index = 0;
  if( last_frame_index > KheFrameTimeGroupCount(frame) - 1 )
    last_frame_index = KheFrameTimeGroupCount(frame) - 1;

  /* add in the tasks from each time group in the range */
  for( i = first_frame_index;  i <= last_frame_index;  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    KheResourceTimetableMonitorAddProperRootTasksInTimeGroup(rtm, tg,
      include_preassigned, ts);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_BUSY_TYPE KheResourceTimetableMonitorTaskBusyType(                   */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,                     */
/*    KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off)              */
/*                                                                           */
/*  Return a value indicating how busy rtm's resource is on the days of the  */
/*  times that task is running:                                              */
/*                                                                           */
/*     KHE_BUSY_NONE - rtm is busy on none of the days                       */
/*     KHE_BUSY_SOME - rtm is busy on some of the days but not all           */
/*     KHE_BUSY_ALL  - rtm is busy on all of the days                        */
/*                                                                           */
/*  Ordinarily, tasks for which non-assignment has no cost are considered    */
/*  to be free time, given that they can be unassigned at no cost.  But      */
/*  if nocost_off is true, they are considered to be not free time.          */
/*                                                                           */
/*  Also, add to r_ts the proper roots of the tasks running in rtm on the    */
/*  days of the times that task is running.  This includes tasks for which   */
/*  KheTaskNeedsAssignment(task) is false.                                   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete, using khe_task_finder.c now
KHE_BUSY_TYPE KheResourceTimetableMonitorTaskBusyType(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task,
  KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off)
{
  KHE_TIME t, t2;  int durn, i, j, k, index, pos;  KHE_TIME_GROUP tg, prev_tg;
  KHE_MEET meet;  KHE_TASK child_task, task2;  KHE_TIME_CELL tc;
  bool tg_busy, all_busy, all_free;  KHE_BUSY_TYPE bt;

  ** handle task itself **
  KheResourceTimetableMonitorAddTimeCells(rtm);
  all_busy = all_free = true;
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      durn = KheMeetDuration(meet);
      prev_tg = NULL;
      for( i = 0;  i < durn;  i++ )
      {
	t2 = KheTimeNeighbour(t, i);
	index = KheFrameTimeIndex(days_frame, t2);
	HnAssert(index != -1, "KheResourceTimetableMonitorTaskBusy "
	  "internal error (time %s not in frame)", KheTimeId(t2));
	tg = KheFrameTimeGroup(days_frame, index);
	if( tg != prev_tg )
	{
	  tg_busy = false;
	  for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	  {
	    t = KheTimeGroupTime(tg, j);
	    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
	    HaArrayForEach(tc->tasks, task2, k)
	    {
	      task2 = KheTaskProperRoot(task2);
	      if( !KheTaskSetContainsTask(r_ts, task2, &pos) )
		KheTaskSetAddTask(r_ts, task2);
	      if( nocost_off || KheTaskNeedsAssignment(task2) )
		tg_busy = true;
	    }
	  }
	  if( tg_busy )
	    all_free = false;
	  else
	    all_busy = false;
	  prev_tg = tg;
	}
      }
    }
  }

  ** handle the child tasks of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    bt = KheResourceTimetableMonitorTaskBusyType(rtm, child_task, days_frame,
      r_ts, nocost_off);
    if( bt != KHE_BUSY_NONE )
      all_free = false;
    if( bt != KHE_BUSY_ALL )
      all_busy = false;
  }

  ** all done **
  return all_busy ? KHE_BUSY_ALL : all_free ? KHE_BUSY_NONE : KHE_BUSY_SOME;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceTimetableMonitorTaskSetBusyType(                         */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK_SET task_set,             */
/*    KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off)              */
/*                                                                           */
/*  Like KheResourceTimetableMonitorTaskBusy except applied to all the       */
/*  tasks of task_set.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete, using khe_task_finder.c now
KHE_BUSY_TYPE KheResourceTimetableMonitorTaskSetBusyType(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK_SET task_set,
  KHE_FRAME days_frame, KHE_TASK_SET r_ts, bool nocost_off)
{
  KHE_TASK task;  int i;  KHE_BUSY_TYPE bt;  bool all_busy, all_free;
  all_busy = all_free = true;
  for( i = 0;  i < KheTaskSetTaskCount(task_set);  i++ )
  {
    task = KheTaskSetTask(task_set, i);
    bt = KheResourceTimetableMonitorTaskBusyType(rtm, task, days_frame, r_ts,
      nocost_off);
    if( bt != KHE_BUSY_NONE )
      all_free = false;
    if( bt != KHE_BUSY_ALL )
      all_busy = false;
  }
  return all_busy ? KHE_BUSY_ALL : all_free ? KHE_BUSY_NONE : KHE_BUSY_SOME;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTimetableMonitorClashingTimeCount(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Return the number of clashing times.                                     */
/*                                                                           */
/*****************************************************************************/

int KheResourceTimetableMonitorClashingTimeCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  return HaArrayCount(rtm->clashing_times);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheResourceTimetableMonitorClashingTime(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, int i)                             */
/*                                                                           */
/*  Return the ith clashing time.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheResourceTimetableMonitorClashingTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int i)
{
  return HaArray(rtm->clashing_times, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTimetableMonitorAtMaxLimitCount(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t)                        */
/*                                                                           */
/*  Return the total at max limit count of the monitors that monitor rtm     */
/*  at t.                                                                    */
/*                                                                           */
/*****************************************************************************/

int KheResourceTimetableMonitorAtMaxLimitCount(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME t)
{
  KHE_TIME_CELL tc;  KHE_MONITORED_TIME_GROUP mtg;  int i, res;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  tc = HaArray(rtm->time_cells, KheTimeIndex(t));
  res = 0;
  HaArrayForEach(tc->monitored_time_groups, mtg, i)
    res += KheMonitoredTimeGroupAtMaxLimitCount(mtg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAddRange(                                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,              */
/*    int last_time_index, KHE_GROUP_MONITOR gm)                             */
/*                                                                           */
/*  Add as children to gm all limit busy times and cluster busy times        */
/*  monitors that monitor rtm at times limited to between first_time_index   */
/*  and last_time_index, and whose constraints cover all resoures of rtm's   */
/*  resource's type.  Do not add any monitors that are already there.        */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAddRange(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int first_time_index,
  int last_time_index, bool include_avoid_unavailable_times,
  KHE_GROUP_MONITOR gm)
{
  KHE_TIME_CELL tc;  KHE_MONITORED_TIME_GROUP mtg;  int i, j;
  KHE_RESOURCE_TYPE rt;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  HnAssert(0 <= first_time_index && first_time_index <= last_time_index + 1
    && last_time_index < HaArrayCount(rtm->time_cells),
    "KheResourceTimetableMonitorAddRange internal error: (%d .. %d) not a"
    " subset of (0 .. %d)", first_time_index, last_time_index,
    HaArrayCount(rtm->time_cells) - 1);
  rt = KheResourceResourceType(KheResourceTimetableMonitorResource(rtm));
  for( i = first_time_index;  i <= last_time_index;  i++ )
  {
    tc = HaArray(rtm->time_cells, i);
    HaArrayForEach(tc->monitored_time_groups, mtg, j)
      KheMonitoredTimeGroupAddRange(mtg, rt, first_time_index,
	last_time_index, include_avoid_unavailable_times, gm);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAddInterval(                             */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,                   */
/*    KHE_INTERVAL in, KHE_GROUP_MONITOR gm)                                 */
/*                                                                           */
/*  Like KheResourceTimetableMonitorAddRange, except that it uses all        */
/*  times in interval in of frame.  Here in may go off either end of         */
/*  frame; if so, in is reduced to lie within frame.                         */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAddInterval(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME frame,
  int first_frame_index, int last_frame_index,
  bool include_avoid_unavailable_times, KHE_GROUP_MONITOR gm)
{
  int first_time_index, last_time_index;  KHE_TIME_GROUP tg;
  tg = KheFrameTimeGroup(frame, max(first_frame_index, 0));
  first_time_index = KheTimeIndex(KheTimeGroupTime(tg, 0));
  tg = KheFrameTimeGroup(frame, min(last_frame_index,
    KheFrameTimeGroupCount(frame) - 1));
  last_time_index = KheTimeIndex(KheTimeGroupTime(tg,
      KheTimeGroupTimeCount(tg) - 1));
  KheResourceTimetableMonitorAddRange(rtm, first_time_index,
    last_time_index, include_avoid_unavailable_times, gm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "attach and detach"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAttachToSoln(                            */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Attach rtm.  It is known to be currently detached.                       */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAttachToSoln(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  if( DO_DEBUG4 )
    fprintf(stderr, "KheResourceTimetableMonitorAttachToSoln()\n");
  rtm->attached = true;
  KheResourceInSolnAttachMonitor(rtm->resource_in_soln, (KHE_MONITOR) rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorDetachFromSoln(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  Detach tm.  It is known to be currently attached.  But unusually, it     */
/*  only acts sometimes:  when no other monitors are attached to it.         */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorDetachFromSoln(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  if( DO_DEBUG4 )
    fprintf(stderr, "KheResourceTimetableMonitorDetachFromSoln()\n");
  if( rtm->attached_monitor_count == 0 )
  {
    KheResourceInSolnDetachMonitor(rtm->resource_in_soln, (KHE_MONITOR) rtm);
    rtm->attached = false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorInternalAttach(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  This function is called when tm might need to be attached because some   */
/*  other monitor is about to be attached to it.                             */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableMonitorInternalAttach(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  KheResourceTimetableMonitorAddTimeCells(rtm);
  rtm->attached_monitor_count++;
  if( !rtm->attached )
    KheResourceTimetableMonitorAttachToSoln(rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorInternalDetach(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm)                                    */
/*                                                                           */
/*  This function is called when rtm might need to be detached because some  */
/*  other monitor has just been detached from it.                            */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableMonitorInternalDetach(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm)
{
  rtm->attached_monitor_count--;
  if( rtm->attached_monitor_count == 0 )
    KheResourceTimetableMonitorDetachFromSoln(rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "avoid clashes monitors"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAttachAvoidClashesMonitor(               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_AVOID_CLASHES_MONITOR m)       */
/*                                                                           */
/*  Attach avoid clashes monitor m to rtm.                                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAttachAvoidClashesMonitor(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_AVOID_CLASHES_MONITOR m)
{
  int i;  KHE_TIME_CELL tc;
  if( DO_DEBUG4 )
    fprintf(stderr, "KheResourceTimetableMonitorAttachAvoidClashesMonitor()\n");
  KheResourceTimetableMonitorInternalAttach(rtm);
  HaArrayAddLast(rtm->avoid_clashes_monitors, m);
  HaArrayForEach(rtm->time_cells, tc, i)
    if( HaArrayCount(tc->tasks) >= 2 )
      KheAvoidClashesMonitorChangeClashCount(m, 0, HaArrayCount(tc->tasks) - 1);
  KheAvoidClashesMonitorFlush(m);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorDetachAvoidClashesMonitor(               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_AVOID_CLASHES_MONITOR m)       */
/*                                                                           */
/*  Detach avoid clashes monitor m from rtm.                                 */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorDetachAvoidClashesMonitor(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_AVOID_CLASHES_MONITOR m)
{
  int i, pos;  KHE_TIME_CELL tc;
  if( DO_DEBUG4 )
    fprintf(stderr, "KheResourceTimetableMonitorDetachAvoidClashesMonitor()\n");
  KheResourceTimetableMonitorAddTimeCells(rtm);
  HaArrayForEach(rtm->time_cells, tc, i)
    if( HaArrayCount(tc->tasks) >= 2 )
      KheAvoidClashesMonitorChangeClashCount(m, HaArrayCount(tc->tasks) - 1, 0);
  KheAvoidClashesMonitorFlush(m);
  if( !HaArrayContains(rtm->avoid_clashes_monitors, m, &pos) )
    HnAbort("KheResourceTimetableMonitorDetachAvoidClashesMonitor internal err");
  HaArrayDeleteAndShift(rtm->avoid_clashes_monitors, pos);
  KheResourceTimetableMonitorInternalDetach(rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitored time groups"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* KHE_MONITORED_TIME_GROUP KheResourceTimetableMonitorAddMonitoredTimeGroup(*/
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)                 */
/*                                                                           */
/*  Ensure that tm monitors tg, and return the monitored time group          */
/*  object that does that monitoring.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_MONITORED_TIME_GROUP KheResourceTimetableMonitorAddMonitoredTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg)
{
  KHE_MONITORED_TIME_GROUP res;  HA_ARENA a;
  if( !KheMonitoredTimeGroupTableRetrieve(rtm->mtg_table, tg, &res) )
  {
    a = KheSolnArena(rtm->soln);
    res = KheMonitoredTimeGroupMake(tg, rtm);
    KheMonitoredTimeGroupTableInsert(rtm->mtg_table, tg, res, a);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAttachMonitoredTimeGroup(                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MONITORED_TIME_GROUP mtg,      */
/*    int *busy_count, float *workload)                                      */
/*                                                                           */
/*  Attach mtg to rtm.  Return the number of busy times, and the workload.   */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAttachMonitoredTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MONITORED_TIME_GROUP mtg,
  int *busy_count, float *workload)
{
  KHE_TIME t;  KHE_TIME_CELL tc;  int i, j;  KHE_TIME_GROUP tg;  KHE_TASK task;
  KheResourceTimetableMonitorInternalAttach(rtm);
  tg = KheMonitoredTimeGroupTimeGroup(mtg);
  *busy_count = 0;
  *workload = 0.0;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    HaArrayAddLast(tc->monitored_time_groups, mtg);
    if( HaArrayCount(tc->tasks) > 0 )
    {
      *busy_count += 1;
      HaArrayForEach(tc->tasks, task, j)
	*workload += KheTaskWorkloadPerTime(task);
    }
  }
  if( !KheMonitorAttachedToSoln((KHE_MONITOR) rtm) )
    KheResourceTimetableMonitorAttachToSoln(rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorDetachMonitoredTimeGroup(                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MONITORED_TIME_GROUP mtg)      */
/*                                                                           */
/*  Detach mtg from rtm.                                                     */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorDetachMonitoredTimeGroup(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_MONITORED_TIME_GROUP mtg)
{
  KHE_TIME t;  KHE_TIME_CELL tc;  int i, pos;  KHE_TIME_GROUP tg;
  tg = KheMonitoredTimeGroupTimeGroup(mtg);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    if( !HaArrayContains(tc->monitored_time_groups, mtg, &pos) )
      HnAbort("KheResourceTimetableMonitorDetachMonitoredTimeGroup internal er");
    HaArrayDeleteAndShift(tc->monitored_time_groups, pos);
  }
  KheResourceTimetableMonitorInternalDetach(rtm);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "monitoring calls from KHE_RESOURCE_IN_SOLN"                   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDebugCell(KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_CELL tc,  */
/*    char *op, KHE_TASK task)                                               */
/*                                                                           */
/*  Brief debug of a change to tc.                                           */
/*                                                                           */
/*****************************************************************************/

#if DEBUG_CELL
static void KheDebugCell(KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_CELL tc,
  char *op, KHE_TASK task)
{
  KHE_RESOURCE r;
  r = KheResourceTimetableMonitorResource(rtm);
  fprintf(stderr, "  [rtm %s/%s %s", KheResourceId(r), KheTimeId(tc->time), op);
  if( task != NULL )
  {
    fprintf(stderr, " ");
    KheTaskDebug(task, 1, -1, stderr);
  }
  fprintf(stderr, " --> %d tasks]\n", HaArrayCount(tc->tasks));
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorSplitTask(                               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task1, KHE_TASK task2)    */
/*                                                                           */
/*  Inform tm that task1 and task2 are splitting.                            */
/*                                                                           */
/*  Implementation note.  For the timetable, this means that some            */
/*  occurrences of task1 have to be replaced by task2.  Since this does not  */
/*  change the number of tasks at any time, no propagation of these changes  */
/*  to the monitors attached to rtm is needed.                               */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorSplitTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task1, KHE_TASK task2)
{
  KHE_MEET meet1, meet2;  int i, pos;  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  meet1 = KheTaskMeet(task1);
  meet2 = KheTaskMeet(task2);
  HnAssert((meet1 == NULL) == (meet2 == NULL),
    "KheResourceTimetableMonitorSplitTask internal error");
  if( meet1 != NULL )
  {
    if( KheMeetAssignedTimeIndex(meet2) != NO_TIME_INDEX )
      for( i = 0;  i < KheMeetDuration(meet2);  i++ )
      {
	tc = HaArray(rtm->time_cells, KheMeetAssignedTimeIndex(meet2) + i);
	if( !HaArrayContains(tc->tasks, task1, &pos) )
	  HnAbort("KheTimetableMonitorSplitTask internal error");
	HaArrayPut(tc->tasks, pos, task2);
#if DEBUG_CELL
	if( tc->debug )
          KheDebugCell(rtm, tc, "split", task2);
#endif
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorMergeTask(                               */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task1, KHE_TASK task2)    */
/*                                                                           */
/*  Inform rtm that task1 and task2 are merging.                             */
/*                                                                           */
/*  Implementation note.  Similarly to splitting, this means that some       */
/*  occurrences of task2 have to be replaced by task1, without propagation.  */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorMergeTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task1, KHE_TASK task2)
{
  KHE_MEET meet1, meet2;  int i, pos;  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  meet1 = KheTaskMeet(task1);
  meet2 = KheTaskMeet(task2);
  HnAssert((meet1 == NULL) == (meet2 == NULL),
    "KheResourceTimetableMonitorMergeTask internal error");
  if( meet1 != NULL )
  {
    if( KheMeetAssignedTimeIndex(meet2) != NO_TIME_INDEX )
      for( i = 0;  i < KheMeetDuration(meet2);  i++ )
      {
	tc = HaArray(rtm->time_cells, KheMeetAssignedTimeIndex(meet2) + i);
	if( !HaArrayContains(tc->tasks, task2, &pos) )
	  HnAbort("KheResourceTimetableMonitorMergeTask internal error");
	HaArrayPut(tc->tasks, pos, task1);
#if DEBUG_CELL
	if( tc->debug )
          KheDebugCell(rtm, tc, "merge", task1);
#endif
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorTaskAssignTime(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm,                                    */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Let m know that task has just been assigned this time.                   */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorTaskAssignTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_TASK task, int assigned_time_index)
{
  int i, j, durn, busy_count_before;  KHE_TIME_CELL tc;  KHE_TIME time;
  KHE_AVOID_CLASHES_MONITOR acm;  KHE_MONITORED_TIME_GROUP mtg;
  if( DO_DEBUG4 )
  {
    fprintf(stderr, "KheResourceTimetableMonitorTaskAssignTime(tm, %p: ",
      (void *) task);
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  KheResourceTimetableMonitorAddTimeCells(rtm);
  HnAssert(0 <= assigned_time_index,
    "KheResourceTimetableMonitorTaskAssignTime internal error 1");
  durn = KheTaskDuration(task);
  HnAssert(assigned_time_index + durn <= HaArrayCount(rtm->time_cells),
    "KheResourceTimetableMonitorTaskAssignTime internal error 2");
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(rtm->time_cells, assigned_time_index + i);
    busy_count_before = HaArrayCount(tc->tasks);
    HaArrayForEach(tc->monitored_time_groups, mtg, j)
      KheMonitoredTimeGroupAssign(mtg, task, assigned_time_index + i,
	busy_count_before);
    if( busy_count_before > 0 )
    {
      HaArrayForEach(rtm->avoid_clashes_monitors, acm, j)
	KheAvoidClashesMonitorChangeClashCount(acm,
	  busy_count_before - 1, busy_count_before);
      if( busy_count_before == 1 )
      {
	/* insert tc->time into rtm->clashing_times in time order */
	for( j = HaArrayCount(rtm->clashing_times) - 1;  j >= 0;  j-- )
	{
	  time = HaArray(rtm->clashing_times, j);
	  if( KheTimeIndex(time) < KheTimeIndex(tc->time) )
	    break;
	}
	HaArrayAdd(rtm->clashing_times, j + 1, tc->time);
      }
    }
    /* ***
    else
      rtm->busy_times++;
    *** */
    HaArrayAddLast(tc->tasks, task);
#if DEBUG_CELL
    if( tc->debug )
      KheDebugCell(rtm, tc, "assign", task);
#endif
  }
  HaArrayForEach(rtm->avoid_clashes_monitors, acm, i)
    KheAvoidClashesMonitorFlush(acm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorTaskUnAssignTime(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm,                                    */
/*    KHE_TASK task, int assigned_time_index)                                */
/*                                                                           */
/*  Let rtm know that task has just been unassigned this time.               */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorTaskUnAssignTime(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_TASK task, int assigned_time_index)
{
  int i, j, pos, durn, busy_count_after;  KHE_TIME_CELL tc;
  KHE_AVOID_CLASHES_MONITOR acm;  KHE_MONITORED_TIME_GROUP mtg;
  if( DO_DEBUG4 )
  {
    fprintf(stderr, "KheResourceTimetableMonitorUnAssignTime(tm, %p: ",
      (void *) task);
    KheTaskDebug(task, 1, -1, stderr);
    fprintf(stderr, ", %d)\n", assigned_time_index);
  }
  KheResourceTimetableMonitorAddTimeCells(rtm);
  durn = KheTaskDuration(task);
  for( i = 0;  i < durn;  i++ )
  {
    tc = HaArray(rtm->time_cells, assigned_time_index + i);
    if( !HaArrayContains(tc->tasks, task, &pos) )
    {
      if( DEBUG5 )
      {
	fprintf(stderr, "KheResourceTimetableMonitorUnAssignTime failing:\n");
        KheResourceTimetableMonitorDebug(rtm, 2, 2, stderr);
      }
      HnAbort("KheResourceTimetableMonitorUnAssignTime internal error 1");
    }
    HaArrayDeleteAndShift(tc->tasks, pos);
#if DEBUG_CELL
    if( tc->debug )
      KheDebugCell(rtm, tc, "unassign", task);
#endif
    busy_count_after = HaArrayCount(tc->tasks);
    HaArrayForEach(tc->monitored_time_groups, mtg, j)
      KheMonitoredTimeGroupUnAssign(mtg, task, assigned_time_index + i,
	busy_count_after);
    if( busy_count_after > 0 )
    {
      HaArrayForEach(rtm->avoid_clashes_monitors, acm, j)
	KheAvoidClashesMonitorChangeClashCount(acm,
	  busy_count_after, busy_count_after - 1);
      if( busy_count_after == 1 )
      {
	if( !HaArrayContains(rtm->clashing_times, tc->time, &pos) )
	  HnAbort("KheResourceTimetableMonitorUnAssignTime internal error 2)");
	HaArrayDeleteAndShift(rtm->clashing_times, pos);
      }
    }
    /* ***
    else
    {
      rtm->busy_times--;
      HnAssert(rtm->busy_times >= 0,
	"KheResourceTimetableMonitorUnAssignTime internal error 3");
    }
    *** */
  }
  HaArrayForEach(rtm->avoid_clashes_monitors, acm, i)
    KheAvoidClashesMonitorFlush(acm);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorAssignResource(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_RESOURCE r)     */
/*                                                                           */
/*  Add task to rtm, but only if it has an assigned time.                    */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorAssignResource(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_RESOURCE r)
{
  KHE_MEET meet;
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAssignedTimeIndex(meet) != NO_TIME_INDEX )
    KheResourceTimetableMonitorTaskAssignTime(rtm, task,
      KheMeetAssignedTimeIndex(meet));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorUnAssignResource(                        */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_RESOURCE r)     */
/*                                                                           */
/*  Delete task from rtm, but only if it has an assigned time.               */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorUnAssignResource(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TASK task, KHE_RESOURCE r)
{
  KHE_MEET meet;
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetAssignedTimeIndex(meet) != NO_TIME_INDEX )
  {
    if( DO_DEBUG4 )
      fprintf(stderr, "  calling KheResourceTimetableMonitorUnAssignTime from "
	"KheResourceTimetableMonitorUnAssignResource:\n");
    KheResourceTimetableMonitorTaskUnAssignTime(rtm, task,
      KheMeetAssignedTimeIndex(meet));
  }
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
/*  KHE_WEEK KheWeekMake(KHE_TIME_GROUP week_tg)                             */
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
/*  void KheResourceTimetableMonitorPrintRow(                                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm,                                    */
/*    KHE_WEEK week, int time_index, int cell_width, int indent, FILE *fp)   */
/*                                                                           */
/*  Print one row of the timetable, the one for time time_index of week.     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableMonitorPrintRow(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_WEEK week, int time_index, int cell_width, int indent, FILE *fp)
{
  int i, j, content_lines;  KHE_TIME_GROUP day;  KHE_TIME t;  char *str;
  KHE_MEET meet;  KHE_TASK task;

  /* print the top line and the blank line just under it */
  KhePrintRuleLine(HaArrayCount(week->days), cell_width, indent, fp);
  /* KhePrintBlankLine(HaArrayCount(week->days), cell_width, indent, fp); */

  /* find the number of content lines to print */
  content_lines = 0;
  HaArrayForEach(week->days, day, i)
    if( time_index < KheTimeGroupTimeCount(day) )
    {
      t = KheTimeGroupTime(day, time_index);
      if( content_lines < KheResourceTimetableMonitorTimeTaskCount(rtm, t) )
	content_lines = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
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
	  if( j >= KheResourceTimetableMonitorTimeTaskCount(rtm, t) )
	    str = "";
	  else
	  {
	    task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	    meet = KheTaskMeet(task);
	    str = KheMeetEvent(meet) == NULL ? "?" :
	      KheEventId(KheMeetEvent(meet)) == NULL ? "-" :
	      KheEventId(KheMeetEvent(meet));
	    if( KheTimeGroupId(day) != NULL &&
		strstr(str, KheTimeGroupId(day)) == str )
	    {
	      str = &str[strlen(KheTimeGroupId(day))];
	      if( strstr(str, ":") == str )
		str = &str[1];
	    }
	  }
	}
	KhePrint(str, true, cell_width, fp);
      }
      fprintf(fp, "|\n");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorPrintRow(                                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm,                                    */
/*    KHE_WEEK week, int time_index, int cell_width, int indent, FILE *fp)   */
/*                                                                           */
/*  Print one row of the timetable, the one for time time_index of week.     */
/*                                                                           */
/*****************************************************************************/

static void KheResourceTimetableMonitorPrintDayRow(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm,
  KHE_WEEK week, int max_days, int cell_width, int indent, FILE *fp)
{
  int i, j;  KHE_TIME_GROUP day;  KHE_TIME t;
  char *str;  KHE_MEET meet;  KHE_TASK task;

  /* print the line just above the row */
  KhePrintRuleLine(max_days, cell_width, indent, fp);

  /* print one content line */
  fprintf(fp, "%*s", indent, "");
  for( i = 0;  i < max_days;  i++ )
  {
    /* print something for this day, even if just a blank line */
    str = "";
    if( i < HaArrayCount(week->days) )
    {
      day = HaArray(week->days, i);
      for( j = 0;  j < KheTimeGroupTimeCount(day);  j++ )
      {
	t = KheTimeGroupTime(day, j);
	if( KheResourceTimetableMonitorTimeTaskCount(rtm, t) > 0 )
	{
	  task = KheResourceTimetableMonitorTimeTask(rtm, t, 0);
	  meet = KheTaskMeet(task);
	  str = KheMeetEvent(meet) == NULL ? "?" :
	    KheEventId(KheMeetEvent(meet)) == NULL ? "-" :
	    KheEventId(KheMeetEvent(meet));
	  if( KheTimeGroupId(day) != NULL &&
	      strstr(str, KheTimeGroupId(day)) == str )
	  {
	    str = &str[strlen(KheTimeGroupId(day))];
	    if( strstr(str, ":") == str )
	      str = &str[1];
	  }
	  break;
	}
      }
    }
    KhePrint(str, true, cell_width, fp);
  }
  fprintf(fp, "|\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorPrintTimetable(                          */
/*    KHE_RESOURCE_TIMETABLE_MONITOR tm, int cell_width,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Print rtm onto fp with the given indent, in a format that looks like     */
/*  an actual timetable (there must be Days, at least, in the instance).     */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorPrintTimetable(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_FRAME days_frame,
  int cell_width, int indent, FILE *fp)
{
  ARRAY_KHE_WEEK weeks;  KHE_WEEK week;  KHE_INSTANCE ins;
  int i, j, count, max_days;  KHE_TIME_GROUP tg;  HA_ARENA a;

  /* find the weeks; make just one if there are no weeks */
  ins = KheSolnInstance(rtm->soln);
  a = KheSolnArenaBegin(rtm->soln);
  HaArrayInit(weeks, a);
  for( i = 0;  i < KheInstanceTimeGroupCount(ins);  i++ )
  {
    tg = KheInstanceTimeGroup(ins, i);
    if( KheTimeGroupKind(tg) == KHE_TIME_GROUP_KIND_WEEK )
      HaArrayAddLast(weeks, KheWeekMake(tg, a));
  }
  if( HaArrayCount(weeks) == 0 )
    HaArrayAddLast(weeks, KheWeekMake(NULL, a));

  /* add the days (either from the instance, or from days_frame) to the weeks */
  if( days_frame == NULL )
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
	  fprintf(fp, "%*sKheResourceTimetableMonitorPrintTimetable: day ",
	    indent, "");
	  KheTimeGroupDebug(tg, 1, -1, fp);
	  fprintf(fp, " lies in %d weeks\n", count);
	}
      }
    }
  else
    for( i = 0;  i < KheFrameTimeGroupCount(days_frame);  i++ )
    {
      tg = KheFrameTimeGroup(days_frame, i);
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
	fprintf(fp, "%*sKheResourceTimetableMonitorPrintTimetable: day ",
	  indent, "");
	KheTimeGroupDebug(tg, 1, -1, fp);
	fprintf(fp, " lies in %d weeks\n", count);
      }
    }

  if( days_frame == NULL )
  {
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
	KheResourceTimetableMonitorPrintRow(rtm, week, j, cell_width,indent,fp);

      /* and a finishing rule */
      KhePrintRuleLine(HaArrayCount(week->days), cell_width, indent, fp);
    }
  }
  else
  {
    /* print a single timetable, one row per week */
    /* find the maximum number of days in one week */
    max_days = 0;
    HaArrayForEach(weeks, week, i)
      if( max_days < HaArrayCount(week->days) )
	max_days = HaArrayCount(week->days);

    /* print one row for each week */
    HaArrayForEach(weeks, week, i)
      KheResourceTimetableMonitorPrintDayRow(rtm, week, max_days, cell_width,
	indent, fp);

    /* and a finishing line */
    KhePrintRuleLine(max_days, cell_width, indent, fp);
  }

  /* delete the memory used */
  KheSolnArenaEnd(rtm->soln, a);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorDebug(                                   */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, int verbosity,                     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of timetable rtm onto fp with this verbosity and indent.     */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorDebug(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, int verbosity,
  int indent, FILE *fp)
{
  KHE_TIME_CELL tc;  int i;
  if( indent >= 0 && verbosity >= 1 )
  {
    KheResourceTimetableMonitorAddTimeCells(rtm);
    KheMonitorDebugBegin((KHE_MONITOR) rtm, indent, fp);
    fprintf(fp, " %s\n", KheResourceInSolnId(rtm->resource_in_soln));
    HaArrayForEach(rtm->time_cells, tc, i)
      if( verbosity >= 3 || HaArrayCount(tc->tasks) > 0 )
	KheTimeCellDebug(tc, verbosity, indent + 2, fp);
    KheMonitorDebugEnd((KHE_MONITOR) rtm, false, indent, fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheResourceTimetableMonitorSetDebug(                                */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, bool val)       */
/*                                                                           */
/*  Set the debug flag in the time cells of the times of tg to val.          */
/*                                                                           */
/*****************************************************************************/

void KheResourceTimetableMonitorSetDebug(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, bool val)
{
#if DEBUG_CELL
  int i;  KHE_TIME t;  KHE_TIME_CELL tc;
  KheResourceTimetableMonitorAddTimeCells(rtm);
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    tc = HaArray(rtm->time_cells, KheTimeIndex(t));
    tc->debug = val;
  }
#else
  HnAbort("KheResourceTimetableMonitorSetDebug not available, DEBUG_CELL is 0");
#endif
}

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
/*  FILE:         khe_sr_tgrc_dynamic_profile.c                              */
/*  DESCRIPTION:  Interval grouping using dynamic programming                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define bool_show(x) ((x) ? "true" : "false")

#define MAX_KEEP 100
#define KHE_COL_WIDTH 14

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0
#define DEBUG10 1		/* print final results as timetables */
#define DEBUG11 0
/* #define DEBUG12 0 */


/*****************************************************************************/
/*                                                                           */
/*  Forward typedefs                                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_time_group_rec *KHE_DPG_TIME_GROUP;
typedef HA_ARRAY(KHE_DPG_TIME_GROUP) ARRAY_KHE_DPG_TIME_GROUP;

typedef struct khe_dpg_task_group_rec *KHE_DPG_TASK_GROUP;
typedef HA_ARRAY(KHE_DPG_TASK_GROUP) ARRAY_KHE_DPG_TASK_GROUP;

typedef struct khe_dpg_soln_rec *KHE_DPG_SOLN;
typedef HA_ARRAY(KHE_DPG_SOLN) ARRAY_KHE_DPG_SOLN;

typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;

typedef struct khe_dpg_solver_rec *KHE_DPG_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TASK_AND_INTERVAL - a task plus its interval                */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_task_and_interval_rec {
  KHE_TASK		task;
  KHE_INTERVAL		interval;
  bool			task_asst_is_fixed;
} *KHE_DPG_TASK_AND_INTERVAL;

typedef HA_ARRAY(KHE_DPG_TASK_AND_INTERVAL) ARRAY_KHE_DPG_TASK_AND_INTERVAL;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIME_GROUP - one time group from the current monitor        */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_time_group_rec {
  KHE_DPG_SOLVER	solver;
  int			index;			/* index in solver           */
  KHE_TIME_GROUP	time_group;		/* the time group            */
  ARRAY_KHE_MTASK	starting_mtasks;	/* mtasks starting here      */
  HA_ARRAY_INT		starting_counts;	/* in starting_mtasks        */
  int			total_solns;		/* solns (dominated + undom) */
  ARRAY_KHE_DPG_SOLN	solns;			/* solns (undominated only)  */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TASK_GROUP - one task group lying in a solution             */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_task_group_rec {
  KHE_TASK		task;			/* the proper root task      */
  /* KHE_COST		non_asst_cost; */	/* cost of non-assignment    */
  /* KHE_COST		asst_cost; */		/* cost of assignment        */
  bool			optional;		/* no cost in omitting this  */
  bool			finish_here_in_best_ds; /* no successor in best_ds   */
  bool			tmp_flag;		/* temporary bool value      */
  /* int		history; */		/* history, if any           */
  int			duration;		/* duration incl. history    */
  int			extension;		/* its extension (future)    */
  KHE_RESOURCE_GROUP	group_domain;		/* its group's domain        */
  KHE_RESOURCE		group_asst;		/* optional assignment       */
  KHE_DPG_TASK_GROUP	prev_task_group;	/* prev task group, if any   */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_SOLN - one solution (a set of tasks)                        */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_soln_rec {
  ARRAY_KHE_DPG_TASK_GROUP task_groups;		/* task groups of this soln  */
  KHE_COST		cost;			/* cost of this soln         */
  KHE_DPG_SOLN		prev_ds;		/* prev soln if any          */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_EXTENDER - info used by KheDpgSolnExtend                    */
/*                                                                           */
/*  undersized_groups                                                        */
/*    The number of groups in prev_ds whose extension is 0 and whose length  */
/*    is less than the minimum limit which have not yet been extended.       */
/*                                                                           */
/*  available_tasks                                                          */
/*    The number of tasks starting in next_dtg and thus available for        */
/*    adding to the undersized groups of prev_ds, which have not yet         */
/*    been used.                                                             */
/*                                                                           */
/*  If undersized_groups >= available_tasks, then no available task can be   */
/*  used to start a new group.  It must be added to an undersized group.     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_extender_rec {
  KHE_DPG_SOLVER		solver;
  KHE_DPG_SOLN			prev_ds;
  KHE_DPG_SOLN			next_ds;
  KHE_DPG_TIME_GROUP		next_dtg;
  /* int			undersized_groups; */
  /* int			available_tasks; */
  HA_ARRAY_BOOL			task_used;
} *KHE_DPG_EXTENDER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_COST_CACHE_SUB - a part of a cost cache                     */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;

typedef struct khe_dpg_cost_cache_sub_rec {
  ARRAY_KHE_COST		costs;
} *KHE_DPG_COST_CACHE_SUB;

typedef HA_ARRAY(KHE_DPG_COST_CACHE_SUB) ARRAY_KHE_DPG_COST_CACHE_SUB;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_COST_CACHE - a cache of costs indexed by an interval        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_cost_cache_rec {
  KHE_DPG_SOLVER		solver;
  ARRAY_KHE_DPG_COST_CACHE_SUB	subs;
  ARRAY_KHE_DPG_COST_CACHE_SUB	neg_subs;   /* for when first_index < 0 */
} *KHE_DPG_COST_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIMETABLE_ENTRY - one entry of a displayed timetable        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_timetable_entry_rec {
  /* KHE_DPG_TASK_GROUP		task_group; */	/* the task                  */
  KHE_TASK			task;		/* the exact KHE task        */
  bool				continuing;	/* continuing from prev row  */
  char				durn_char;	/* over or under durn        */
} *KHE_DPG_TIMETABLE_ENTRY;

typedef HA_ARRAY(KHE_DPG_TIMETABLE_ENTRY) ARRAY_KHE_DPG_TIMETABLE_ENTRY;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIMETABLE_ROW - one row of a displayed timetable            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_timetable_row_rec {
  KHE_DPG_TIME_GROUP		time_group;	/* the time group of the row */
  ARRAY_KHE_DPG_TIMETABLE_ENTRY	entries;	/* the entries of the row    */
} *KHE_DPG_TIMETABLE_ROW;

typedef HA_ARRAY(KHE_DPG_TIMETABLE_ROW) ARRAY_KHE_DPG_TIMETABLE_ROW;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIMETABLE - an actual timetable, used for debugging         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_timetable_rec {
  KHE_DPG_SOLVER		solver;
  KHE_DPG_SOLN			soln;		/* the soln being displayed  */
  int				undersized_durn;
  int				oversized_durn;
  ARRAY_KHE_DPG_TIMETABLE_ROW	rows;		/* rows of the timetable     */
} *KHE_DPG_TIMETABLE;

typedef HA_ARRAY(KHE_DPG_TIMETABLE) ARRAY_KHE_DPG_TIMETABLE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_SOLVER - interval grouping by dynamic programming solver    */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR)
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR;

struct khe_dpg_solver_rec {

  /* free lists */
  ARRAY_KHE_DPG_TASK_AND_INTERVAL	dpg_task_and_interval_free_list;
  ARRAY_KHE_DPG_TIME_GROUP		dpg_time_group_free_list;
  ARRAY_KHE_DPG_TASK_GROUP		dpg_task_group_free_list;
  ARRAY_KHE_DPG_SOLN			dpg_soln_free_list;
  ARRAY_KHE_DPG_COST_CACHE_SUB		dpg_cost_cache_sub_free_list;
  ARRAY_KHE_DPG_TIMETABLE_ENTRY		dpg_timetable_entry_free_list;
  ARRAY_KHE_DPG_TIMETABLE_ROW		dpg_timetable_row_free_list;
  ARRAY_KHE_DPG_TIMETABLE		dpg_timetable_free_list;

  /* fields defined (and mostly constant) throughout the solve */
  HA_ARENA				arena;
  KHE_SOLN				soln;
  KHE_RESOURCE_TYPE			resource_type;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_FRAME				days_frame;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  KHE_COST				marginal_cost;
  ARRAY_KHE_LIMIT_ACTIVE_INTERVALS_MONITOR monitors;
  int					groups_count;

  /* fields defined when solving one set of equivalent monitors */
  int					first_monitor_index;
  int					last_monitor_index;
  int					min_limit;
  int					max_limit;
  int					history_before;
  int					history_after;
  ARRAY_KHE_DPG_TIME_GROUP		time_groups;
  KHE_DPG_COST_CACHE			cost_cache;

  /* scratch fields */
  ARRAY_KHE_TASK			assigned_tasks;
  ARRAY_KHE_DPG_TASK_AND_INTERVAL	tasks_and_intervals;
  KHE_DPG_EXTENDER			extender;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TASK_AND_INTERVAL"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TASK_AND_INTERVAL KheDpgTaskAndIntervalMake(KHE_TASK task,       */
/*    KHE_INTERVAL in, KHE_DPG_SOLVER dsv)                                   */
/*                                                                           */
/*  Make a new task and interval object with these attributes.               */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TASK_AND_INTERVAL KheDpgTaskAndIntervalMake(KHE_TASK task,
    KHE_INTERVAL in, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK_AND_INTERVAL res;
  if( HaArrayCount(dsv->dpg_task_and_interval_free_list) > 0 )
    res = HaArrayLastAndDelete(dsv->dpg_task_and_interval_free_list);
  else
    HaMake(res, dsv->arena);
  res->task = task;
  res->interval = in;
  res->task_asst_is_fixed = KheTaskAssignIsFixed(task);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTaskAndIntervalFree(KHE_DPG_TASK_AND_INTERVAL dti,            */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Free dti.                                                                */
/*                                                                           */
/*****************************************************************************/

/* *** correct, but currently unused
   static void KheDpgTaskAndIntervalFree(KHE_DPG_TASK_AND_INTERVAL dti,
   KHE_DPG_SOLVER dsv)
   {
   HaArrayAddLast(dsv->dpg_task_and_interval_free_list, dti);
   }
 *** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskAndIntervalTypedCmp(KHE_DPG_TASK_AND_INTERVAL dti1,        */
/*    KHE_DPG_TASK_AND_INTERVAL dti2)                                        */
/*                                                                           */
/*  Typed comparison function for sorting an array of task plus              */
/*  interval objects by increasing interval.                                 */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskAndIntervalTypedCmp(KHE_DPG_TASK_AND_INTERVAL dti1,
    KHE_DPG_TASK_AND_INTERVAL dti2)
{
  if( dti1->interval.first != dti2->interval.first )
    return dti1->interval.first - dti2->interval.first;
  else
    return dti1->interval.last - dti2->interval.last;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskAndIntervalCmp(const void *t1, const void *t2)             */
/*                                                                           */
/*  Untyped comparison function for sorting an array of task plus            */
/*  interval objects by increasing interval.                                 */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskAndIntervalCmp(const void *t1, const void *t2)
{
  KHE_DPG_TASK_AND_INTERVAL dti1 = * (KHE_DPG_TASK_AND_INTERVAL *) t1;
  KHE_DPG_TASK_AND_INTERVAL dti2 = * (KHE_DPG_TASK_AND_INTERVAL *) t2;
  return KheDpgTaskAndIntervalTypedCmp(dti1, dti2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TIME_GROUP"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TIME_GROUP KheDpgTimeGroupMake(int index, KHE_TIME_GROUP tg,     */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Make a new dpg time group object with these attributes.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TIME_GROUP KheDpgTimeGroupMake(int index, KHE_TIME_GROUP tg,
    KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TIME_GROUP res;

  if( HaArrayCount(dsv->dpg_time_group_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(dsv->dpg_time_group_free_list);
    HaArrayClear(res->starting_mtasks);
    HaArrayClear(res->starting_counts);
    HaArrayClear(res->solns);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->starting_mtasks, dsv->arena);
    HaArrayInit(res->starting_counts, dsv->arena);
    HaArrayInit(res->solns, dsv->arena);
  }
  res->solver = dsv;
  res->index = index;
  res->time_group = tg;
  res->total_solns = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupDeleteSolns(KHE_DPG_TIME_GROUP dtg,                  */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Delete dtg's solutions.                                                  */
/*                                                                           */
/*****************************************************************************/
static void KheDpgSolnFree(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv);

static void KheDpgTimeGroupDeleteSolns(KHE_DPG_TIME_GROUP dtg,
  KHE_DPG_SOLVER dsv)
{
  KHE_DPG_SOLN ds;  int i;
  dtg->total_solns = 0;
  HaArrayForEach(dtg->solns, ds, i)
    KheDpgSolnFree(ds, dsv);
  HaArrayClear(dtg->solns);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupFree(KHE_DPG_TIME_GROUP dtg, KHE_DPG_SOLVER dsv)     */
/*                                                                           */
/*  Free dtg, including freeing any solutions it may still contain.          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimeGroupFree(KHE_DPG_TIME_GROUP dtg, KHE_DPG_SOLVER dsv)
{
  KheDpgTimeGroupDeleteSolns(dtg, dsv);
  HaArrayAddLast(dsv->dpg_time_group_free_list, dtg);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTimeGroupAcceptsMTask(KHE_DPG_SOLVER dsv, int index,          */
/*    KHE_MTASK mt)                                                          */
/*                                                                           */
/*  Return true if the time group at position index accepts mt.              */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTimeGroupAcceptsMTask(KHE_DPG_SOLVER dsv, int index,
    KHE_MTASK mt)
{
  KHE_TIME_SET ts;  int i, pos;  KHE_TIME time;  KHE_DPG_TIME_GROUP dtg;

  /* mt must have no overlaps and no gaps */
  if( !KheMTaskNoOverlap(mt) )
    return false;
  if( !KheMTaskNoGaps(mt) )
    return false;

  /* mt must contain at least one assigned time */
  ts = KheMTaskTimeSet(mt);
  if( KheTimeSetTimeCount(ts) == 0 )
    return false;

  /* check time groups */
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
  {
    time = KheTimeSetTime(ts, i);
    if( index + i >= HaArrayCount(dsv->time_groups) )
      return false;
    dtg = HaArray(dsv->time_groups, index + i);
    if( !KheTimeGroupContains(dtg->time_group, time, &pos) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupAddMTasks(KHE_DPG_TIME_GROUP dtg, int index,         */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Add all admissible mtasks to dtg.                                        */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimeGroupAddMTasks(KHE_DPG_TIME_GROUP dtg, int index,
    KHE_DPG_SOLVER dsv)
{
  KHE_MTASK_SET mts;  KHE_MTASK mt;  int i, count;
  mts = KheMTaskFinderMTasksInTimeGroup(dsv->mtask_finder,
      dsv->resource_type, dtg->time_group);
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( KheDpgTimeGroupAcceptsMTask(dsv, index, mt) )
    {
      HaArrayAddLast(dtg->starting_mtasks, mt);
      count = KheMTaskAssignedTaskCount(mt) +
	KheMTaskNeedsAssignmentTaskCount(mt);
      HaArrayAddLast(dtg->starting_counts, count);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupAddSoln(KHE_DPG_TIME_GROUP dtg, KHE_DPG_SOLN ds)     */
/*                                                                           */
/*  Add ds to dtg, doing dominance testing as we go.                         */
/*                                                                           */
/*  The solution passed here, ds, is already a copy.  So if it ends up       */
/*  not being used, because it is dominated, then free it.                   */
/*                                                                           */
/*****************************************************************************/
static bool KheDpgSolnDominates(KHE_DPG_SOLN ds1, KHE_DPG_SOLN ds2,
    bool last_dtg);
static KHE_DPG_SOLN KheDpgSolnCopy(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv);

static void KheDpgTimeGroupAddSoln(KHE_DPG_TIME_GROUP dtg, KHE_DPG_SOLN ds)
{
  KHE_DPG_SOLN ds2;  int i;  bool last_dtg;

  /* if ds is dominated, free ds and return without changing anything else */
  dtg->total_solns++;
  last_dtg = (dtg->index == HaArrayCount(dtg->solver->time_groups) - 1);
  HaArrayForEach(dtg->solns, ds2, i)
    if( KheDpgSolnDominates(ds2, ds, last_dtg) )
    {
      KheDpgSolnFree(ds, dtg->solver);
      return;
    }

  /* ds is not dominated, so free anything dominated by it */
  HaArrayForEach(dtg->solns, ds2, i)
    if( KheDpgSolnDominates(ds, ds2, last_dtg) )
    {
      KheDpgSolnFree(ds2, dtg->solver);
      HaArrayDeleteAndPlug(dtg->solns, i);
      i--;
    }

  /* finally, add ds */
  HaArrayAddLast(dtg->solns, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupDebugMTasks(KHE_DPG_TIME_GROUP dtg, int verbosity,   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the mtasks of dtg onto fp with the given verbosity and    */
/*  indent.                                                                  */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimeGroupDebugMTasks(KHE_DPG_TIME_GROUP dtg, int verbosity,
    int indent, FILE *fp)
{
  KHE_MTASK mt;  int i, count;
  fprintf(fp, "%*s[ DpgTimeGroup(%s)\n", indent, "",
      KheTimeGroupId(dtg->time_group));
  HaArrayForEach(dtg->starting_mtasks, mt, i)
  {
    count = HaArray(dtg->starting_counts, i);
    fprintf(fp, "%*s  %d:\n", indent, "", count);
    KheMTaskDebug(mt, verbosity, indent + 2, fp);
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupDebugSolnsHeader(KHE_DPG_TIME_GROUP dtg, FILE *fp)   */
/*                                                                           */
/*  Print the header line of KheDpgTimeGroupDebugSolns.                      */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimeGroupDebugSolnsHeader(KHE_DPG_TIME_GROUP dtg, FILE *fp)
{
  fprintf(fp, "DpgTimeGroupSolns(%s, %d made, %d undominated)",
      KheTimeGroupId(dtg->time_group), dtg->total_solns,
      HaArrayCount(dtg->solns));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimeGroupDebugSolns(KHE_DPG_TIME_GROUP dtg, int verbosity,    */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of dtg's solutions onto fp with the given verbosity and      */
/*  indent.                                                                  */
/*                                                                           */
/*****************************************************************************/
static void KheDpgSolnDebugTimetable(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv,
    int verbosity, int indent, FILE *fp);

static void KheDpgTimeGroupDebugSolns(KHE_DPG_TIME_GROUP dtg, int verbosity,
    int indent, FILE *fp)
{
  KHE_DPG_SOLN ds;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheDpgTimeGroupDebugSolnsHeader(dtg, fp);
    if( HaArrayCount(dtg->solns) <= 10 )
    {
      HaArrayForEach(dtg->solns, ds, i)
      {
	fprintf(stderr, "\n");
	KheDpgSolnDebugTimetable(ds, dtg->solver, verbosity, indent+2, stderr);
      }
    }
    else
    {
      fprintf(fp, "\n");
      ds = HaArrayFirst(dtg->solns);
      KheDpgSolnDebugTimetable(ds, dtg->solver, verbosity, indent + 2, fp);
      fprintf(fp, "%*s  ... ... ...\n", indent, "");
      ds = HaArrayLast(dtg->solns);
      KheDpgSolnDebugTimetable(ds, dtg->solver, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheDpgTimeGroupDebugSolnsHeader(dtg, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TASK_GROUP"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TASK_GROUP KheDpgTaskGroupMake(KHE_TASK task,                    */
/*    bool optional, int history, int group_length, int extension,           */
/*    KHE_RESOURCE_GROUP group_domain, KHE_RESOURCE group_asst,              */
/*    KHE_DPG_TASK_GROUP prev_task_group, KHE_DPG_SOLVER dsv)                */
/*                                                                           */
/*  Make a task group with these attributes.                                 */
/*                                                                           */
/*  The task group is optional if the "optional" argument is true is set     */
/*  and its predecessors are all optional.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TASK_GROUP KheDpgTaskGroupMake(KHE_TASK task,
  bool optional, /* int history, int group_length, */ int duration,
  int extension, KHE_RESOURCE_GROUP group_domain, KHE_RESOURCE group_asst,
  KHE_DPG_TASK_GROUP prev_task_group, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK_GROUP res;
  if( HaArrayCount(dsv->dpg_task_group_free_list) > 0 )
    res = HaArrayLastAndDelete(dsv->dpg_task_group_free_list);
  else
    HaMake(res, dsv->arena);
  res->task = task;
  res->optional = optional &&
    (prev_task_group == NULL || prev_task_group->optional);
  res->finish_here_in_best_ds = true;  /* not really defined yet */
  res->duration = duration;
  /* ***
  res->history = history;
  res->group_length = group_length;
  *** */
  res->extension = extension;
  res->group_domain = group_domain;
  res->group_asst = group_asst;
  res->prev_task_group = prev_task_group;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TASK_GROUP KheDpgTaskGroupCopy(KHE_DPG_TASK_GROUP dt,            */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Return a copy of dt.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TASK_GROUP KheDpgTaskGroupCopy(KHE_DPG_TASK_GROUP dt,
  KHE_DPG_SOLVER dsv)
{
  return KheDpgTaskGroupMake(dt->task, dt->optional, dt->duration,
    /* dt->history, dt->group_length, */ dt->extension, dt->group_domain,
    dt->group_asst, dt->prev_task_group, dsv);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTaskGroupFree(KHE_DPG_TASK_GROUP dt, KHE_DPG_SOLVER dsv)      */
/*                                                                           */
/*  Free dt, by adding it to the free list in dsv.                           */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTaskGroupFree(KHE_DPG_TASK_GROUP dt, KHE_DPG_SOLVER dsv)
{
  HaArrayAddLast(dsv->dpg_task_group_free_list, dt);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTaskGroupExtendable(KHE_DPG_TASK_GROUP dt, KHE_TASK task,     */
/*    int task_total_durn, KHE_DPG_SOLVER dsv, KHE_DPG_TASK_GROUP *next_dt)  */
/*                                                                           */
/*  If dt can be extended by adding task, whose total duration is            */
/*  task_total_durn, then return true with *next_dt set to the resulting     */
/*  new dpg task.  Otherwise return false with *next_dt set to NULL.         */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTaskGroupExtendable(KHE_DPG_TASK_GROUP dt, KHE_TASK task,
  /* KHE_COST non_asst_cost, KHE_COST asst_cost, */ int task_total_durn,
  bool optional, KHE_DPG_SOLVER dsv, KHE_DPG_TASK_GROUP *next_dt)
{
  int duration /* group_length */;  KHE_RESOURCE task_asst, group_asst;
  KHE_RESOURCE_GROUP task_domain, group_domain;
  HnAssert(dt->extension == 0, "KheDpgTaskGroupExtendable internal error");

  /* find duration; return false if can't */
  duration = dt->duration + task_total_durn;
  if( duration > dsv->max_limit )
    return *next_dt = NULL, false;
  /* ***
  group_length = dt->group_length + task_total_durn;
  if( group_length + dt->history > dsv->max_limit )
    return *next_dt = NULL, false;
  *** */

  /* find group_asst; return false if can't */
  task_asst = KheTaskAsstResource(task);
  if( dt->group_asst != NULL )
  {
    if( task_asst != NULL )
    {
      /* both are assigned; return false if not equal */
      if( dt->group_asst != task_asst )
	return *next_dt = NULL, false;
      group_asst = task_asst;
    }
    else
    {
      /* one is assigned; return false if domain does not suit */
      if( !KheTaskAssignResourceCheck(task, dt->group_asst) )
	return *next_dt = NULL, false;
      group_asst = dt->group_asst;
    }
  }
  else
  {
    if( task_asst != NULL )
    {
      /* one is assigned; return false if domain does not suit */
      if( !KheResourceGroupContains(dt->group_domain, task_asst) )
	return *next_dt = NULL, false;
      group_asst = task_asst;
    }
    else
    {
      /* neither is assigned, all good */
      group_asst = NULL;
    }
  }

  /* find group_domain; return false if can't */
  task_domain = KheTaskDomain(task);
  if( KheResourceGroupSubset(dt->group_domain, task_domain) )
    group_domain = dt->group_domain;
  else if( KheResourceGroupSubset(task_domain, dt->group_domain) )
    group_domain = task_domain;
  else
    return *next_dt = NULL, false;

  /* all good, make *next_dt and return true */
  *next_dt = KheDpgTaskGroupMake(task, optional /*non_asst_cost <= asst_cost*/,
    duration, /* dt->history, group_length, */ task_total_durn - 1,
    group_domain, group_asst, dt, dsv);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskGroupTypedCmp(KHE_DPG_TASK_GROUP dt1,                      */
/*    KHE_DPG_TASK_GROUP dt2)                                                */
/*                                                                           */
/*  Typed comparison function for sorting an array of task groups.           */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskGroupTypedCmp(KHE_DPG_TASK_GROUP dt1,
  KHE_DPG_TASK_GROUP dt2)
{
  int cmp, domain_count1, domain_count2;

  /* sort by increasing extension */
  cmp = dt1->extension - dt2->extension;
  if( cmp != 0 )  return cmp;

  /* sort by increasing duration */
  cmp = dt1->duration - dt2->duration;
  if( cmp != 0 )  return cmp;

  /* sort by increasing group length */
  /* ***
  cmp = dt1->group_length - dt2->group_length;
  if( cmp != 0 )  return cmp;
  *** */

  /* sort by increasing history */
  /* ***
  cmp = dt1->history - dt2->history;
  if( cmp != 0 )  return cmp;
  *** */

  /* sort by domain size */
  domain_count1 = KheResourceGroupResourceCount(KheTaskDomain(dt1->task));
  domain_count2 = KheResourceGroupResourceCount(KheTaskDomain(dt2->task));
  return domain_count1 - domain_count2;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskGroupCmp(const void *t1, const void *t2)                   */
/*                                                                           */
/*  Untyped comparison function for sorting an array of dpg tasks.           */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskGroupCmp(const void *t1, const void *t2)
{
  KHE_DPG_TASK_GROUP dt1 = * (KHE_DPG_TASK_GROUP *) t1;
  KHE_DPG_TASK_GROUP dt2 = * (KHE_DPG_TASK_GROUP *) t2;
  return KheDpgTaskGroupTypedCmp(dt1, dt2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTaskGroupDebug(KHE_DPG_TASK_GROUP dt, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of dt onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* *** good, but currently unused
static void KheDpgTaskGroupDebug(KHE_DPG_TASK_GROUP dt, int verbosity,
  int indent, FILE *fp)
{
  if( indent > 0 )
  fprintf(fp, "%*s", indent, "");
  fprintf(fp, "<%s %d:%d %s", KheTaskId(dt->task), dt->group_length,
  dt->extension, KheResourceGroupId(dt->group_domain));
  if( verbosity >= 3 && dt->prev_task_group != NULL )
  {
    fprintf(fp, " ");
    KheDpgTaskGroupDebug(dt->prev_task_group, verbosity, -1, fp);
  }
  fprintf(fp, ">");
  if( indent >= 0 )
  fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_SOLN"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolnReset(KHE_DPG_SOLN ds, KHE_DPG_SOLN prev_ds,              */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Reset ds so that it has no tasks and follows on from prev_ds.            */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolnReset(KHE_DPG_SOLN ds, KHE_DPG_SOLN prev_ds,
    KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK_GROUP dt;  int i;
  HaArrayForEach(ds->task_groups, dt, i)
    KheDpgTaskGroupFree(dt, dsv);
  HaArrayClear(ds->task_groups);
  if( prev_ds == NULL )
  {
    ds->cost = 0;
    ds->prev_ds = NULL;
  }
  else
  {
    ds->cost = prev_ds->cost;
    ds->prev_ds = prev_ds;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_SOLN KheDpgSolnMake(KHE_DPG_SOLN prev_ds, KHE_DPG_SOLVER dsv)    */
/*                                                                           */
/*  Make a new soln object, initially following on from prev_ds but with     */
/*  no tasks.                                                                */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_SOLN KheDpgSolnMake(KHE_DPG_SOLN prev_ds, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_SOLN res;
  if( HaArrayCount(dsv->dpg_soln_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(dsv->dpg_soln_free_list);
    HaArrayClear(res->task_groups);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->task_groups, dsv->arena);
  }
  KheDpgSolnReset(res, prev_ds, dsv);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_SOLN KheDpgSolnCopy(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)         */
/*                                                                           */
/*  Return a copy of ds.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_SOLN KheDpgSolnCopy(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_SOLN res;  KHE_DPG_TASK_GROUP dt;  int i;
  res = KheDpgSolnMake(ds->prev_ds, dsv);
  HaArrayForEach(ds->task_groups, dt, i)
    HaArrayAddLast(res->task_groups, KheDpgTaskGroupCopy(dt, dsv));
  res->cost = ds->cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnMustExtendCount(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)       */
/*                                                                           */
/*  Return the number of tasks in ds which have 0 extension but must extend. */
/*                                                                           */
/*****************************************************************************/

/* ***
   static int KheDpgSolnMustExtendCount(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)
   {
   int res, i;  KHE_DPG_TASK_GROUP dt;
   res = 0;
   HaArrayForEach(ds->task_groups, dt, i)
   if( KheDpgTaskGroupMustExtend(dt, dsv) )
   res++;
   return res;
   }
 *** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolnFree(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)                 */
/*                                                                           */
/*  Free ds, by adding it to the free list in dsv.                           */
/*                                                                           */
/*  This also frees its tasks.                                               */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolnFree(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK_GROUP dt;  int i;
  HaArrayForEach(ds->task_groups, dt, i)
    KheDpgTaskGroupFree(dt, dsv);
  HaArrayAddLast(dsv->dpg_soln_free_list, ds);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgSolnDominates(KHE_DPG_SOLN ds1, KHE_DPG_SOLN ds2,             */
/*    bool last_dtg)                                                         */
/*                                                                           */
/*  Return true if ds1 dominates ds2.  If last_dtg is true, we are at        */
/*  the last time group and the tasks do not matter.                         */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgSolnDominates(KHE_DPG_SOLN ds1, KHE_DPG_SOLN ds2,
    bool last_dtg)
{
  KHE_DPG_TASK_GROUP dt1, dt2;  int i;
  HnAssert(HaArrayCount(ds1->task_groups) == HaArrayCount(ds2->task_groups),
      "KheDpgSolnDominates internal error 1");

  /* for dominance, the cost of ds1 must not exceed the cost of ds2 */
  if( ds1->cost > ds2->cost )
    return false;

  /* for dominance, corresponding group lengths and extensions must be */
  /*  equal, and domains must be supersets; unless last_dtg */
  if( !last_dtg )
    for( i = 0;  i < HaArrayCount(ds1->task_groups);  i++ )
    {
      dt1 = HaArray(ds1->task_groups, i);
      dt2 = HaArray(ds2->task_groups, i);
      if( dt1->duration != dt2->duration )
	return false;
      /* ***
      if( dt1->history != dt2->history )
	return false;
      if( dt1->group_length != dt2->group_length )
	return false;
      *** */
      if( dt1->extension != dt2->extension )
	return false;
      if( dt1->group_asst != dt2->group_asst )
	return false;
      if( !KheResourceGroupSubset(dt2->group_domain, dt1->group_domain) )
	return false;
    }

  /* all good, ds1 dominates ds2 */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheFindLeaderTask(KHE_DPG_TASK_GROUP dt,                        */
/*    KHE_RESOURCE_GROUP domain)                                             */
/*                                                                           */
/*  Find the leader task of the tasks linked to dt.  It is any task with     */
/*  the given domain.                                                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused (the mtf task grouper is doing it now)
static KHE_TASK KheFindLeaderTask(KHE_DPG_TASK_GROUP dt,
  KHE_RESOURCE_GROUP domain)
{
  for( ;  dt != NULL;  dt = dt->prev_task_group )
    if( KheTaskDomain(dt->task) == domain )
      return dt->task;
  HnAbort("KheFindLeaderTask internal error");
  return NULL;  ** keep compiler happy **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheClearGroup(KHE_DPG_TASK_GROUP dt)                                */
/*                                                                           */
/*  Clear out the group starting at dt.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** merged into KheBuildGroup now
static void KheClearGroup(KHE_DPG_TASK_GROUP dt)
{
  KHE_DPG_TASK_GROUP prev_dt;
  prev_dt = dt->prev_task_group;
  while( prev_dt != NULL )
  {
    dt->prev_task_group = NULL;
    dt = prev_dt;
    prev_dt = dt->prev_task_group;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupBuildAndClear(KHE_DPG_TASK_GROUP last_dt,               */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Build one task group starting at last_dt.  Also clear it out.            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupBuildAndClear(KHE_DPG_TASK_GROUP last_dt,
  KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK_GROUP dt, prev_dt;

  /* build the group */
  KheMTaskFinderTaskGrouperClear(dsv->mtask_finder);
  for( dt = last_dt;  dt != NULL;  dt = dt->prev_task_group )
  {
    if( dt->extension == 0 )
    {
      if( !KheMTaskFinderTaskGrouperAddTask(dsv->mtask_finder, dt->task) )
	HnAbort("KheTaskGroupBuildAndClear internal error");
    }
  }
  KheMTaskFinderTaskGrouperMakeGroup(dsv->mtask_finder, dsv->soln_adjuster);

  /* break up the group, to prevent it from being grouped again */
  for( dt = last_dt;  dt != NULL;  dt = prev_dt )
  {
    prev_dt = dt->prev_task_group;
    dt->prev_task_group = NULL;
  }
}

/* *** old version from before task groupers were defined and used by mtf
static void KheTaskGroupBuildAndClear(KHE_DPG_TASK_GROUP last_dt,
  KHE_DPG_SOLVER dsv)
{
  KHE_TASK leader_task;  KHE_DPG_TASK_GROUP dt, prev_dt;

  ** find the leader task **
  leader_task = KheFindLeaderTask(last_dt, last_dt->group_domain);

  ** build the group, unfixing tasks where needed **
  KheMTaskFinderGroupBegin(dsv->mtask_finder, leader_task);
  for( dt = last_dt;  dt != NULL;  dt = dt->prev_task_group )
  {
    if( dt->task != leader_task && dt->extension == 0 )
    {
      dt->tmp_flag = KheTaskAssignIsFixed(dt->task);
      if( dt->tmp_flag )
	KheTaskAssignUnFix(dt->task);
      if( !KheMTaskFinderGroupAddTask(dsv->mtask_finder, dt->task) )
	HnAbort("KheTaskGroupBuildAndClear internal error");
    }
  }
  KheMTaskFinderGroupEnd(dsv->mtask_finder, dsv->soln_adjuster);

  ** fix any tasks that were fixed initially **
  for( dt = last_dt;  dt != NULL;  dt = dt->prev_task_group )
    if( dt->task != leader_task && dt->extension == 0 && dt->tmp_flag )
      KheTaskAssignFix(dt->task);

  ** break up the group, to prevent it from being grouped again **
  for( dt = last_dt;  dt != NULL;  dt = prev_dt )
  {
    prev_dt = dt->prev_task_group;
    dt->prev_task_group = NULL;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnBuildGroups(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv,           */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int run)                      */
/*                                                                           */
/*  Build the groups defined by ds and return the number of groups made.     */
/*  Parameters laim and run are only for debugging.                          */
/*                                                                           */
/*****************************************************************************/

static int KheDpgSolnBuildGroups(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int run)
{
  KHE_DPG_TASK_GROUP dt;  int i, res;
  if( DEBUG11 )
  {
    fprintf(stderr, "  KheDpgSolnBuildGroups(%s, %s, %s) best_ds %d:\n",
      KheInstanceId(KheSolnInstance(dsv->soln)),
      KheResourceTypeId(dsv->resource_type),
      KheMonitorId((KHE_MONITOR) laim), run);
    KheDpgSolnDebugTimetable(ds, dsv, 2, 2, stderr);
  }
  res = 0;
  for( ;  ds != NULL;  ds = ds->prev_ds )
  {
    HaArrayForEach(ds->task_groups, dt, i)
      if( dt->prev_task_group != NULL )
      {
	KheTaskGroupBuildAndClear(dt, dsv);
	res++;
      }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnTypedCmp(KHE_DPG_SOLN ds1, KHE_DPG_SOLN ds2)               */
/*                                                                           */
/*  Typed comparison function for sorting an array of solutions by           */
/*  increasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheDpgSolnTypedCmp(KHE_DPG_SOLN ds1, KHE_DPG_SOLN ds2)
{
  return KheCostCmp(ds1->cost, ds2->cost);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of solutions by         */
/*  increasing cost.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheDpgSolnCmp(const void *t1, const void *t2)
{
  KHE_DPG_SOLN ds1 = * (KHE_DPG_SOLN *) t1;
  KHE_DPG_SOLN ds2 = * (KHE_DPG_SOLN *) t2;
  return KheDpgSolnTypedCmp(ds1, ds2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolnDebug(KHE_DPG_SOLN ds, int verbosity, int indent,         */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of ds onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* *** more or less superseded by KheDpgSolnDebugTimetable below
static void KheDpgSolnDebug(KHE_DPG_SOLN ds, int verbosity, int indent,
  FILE *fp)
{
  KHE_DPG_TASK_GROUP dt;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ Soln(%.5f)", KheCostShow(ds->cost));
  HaArrayForEach(ds->task_groups, dt, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheDpgTaskGroupDebug(dt, verbosity, -1, fp);
  }
  if( indent >= 0 && ds->prev_ds != NULL )
  {
    fprintf(fp, "\n");
    KheDpgSolnDebug(ds->prev_ds, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]", indent, "");
  }
  else
    fprintf(fp, " ]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolnDebugTimetable(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv,       */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of ds (in the form of a timetable) onto fp with the given    */
/*  verbosity and indent.                                                    */
/*                                                                           */
/*****************************************************************************/
static KHE_DPG_TIMETABLE KheDpgTimetableMake(KHE_DPG_SOLN ds,
    KHE_DPG_SOLVER dsv);
static void KheDpgTimetableDebug(KHE_DPG_TIMETABLE dtt, int verbosity,
    int indent, FILE *fp);
static void KheDpgTimetableFree(KHE_DPG_TIMETABLE dtt);

static void KheDpgSolnDebugTimetable(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv,
    int verbosity, int indent, FILE *fp)
{
  KHE_DPG_TIMETABLE dtt;
  dtt = KheDpgTimetableMake(ds, dsv);
  KheDpgTimetableDebug(dtt, verbosity, indent, fp);
  KheDpgTimetableFree(dtt);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_EXTENDER - calculating the cost of one group"         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheLimitActiveCost(KHE_DPG_SOLVER dsv, int len, bool last)      */
/*                                                                           */
/*  Return the cost reported by the current limit active intervals monitor   */
/*  given a sequence of length len.  If last is true, we are at the end.     */
/*                                                                           */
/*****************************************************************************/

static KHE_COST KheLimitActiveCost(KHE_DPG_SOLVER dsv, int len, bool last)
{
  int dev, i;  KHE_COST res;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;

  /* find the deviation */
  if( len < dsv->min_limit )
  {
    if( last && dsv->history_after > 0 )
      dev = 0;
    else
      dev = dsv->min_limit - len;
  }
  else if( len > dsv->max_limit )
    dev = len - dsv->max_limit;
  else
    dev = 0;

  /* find the cost */
  res = 0;
  for( i = dsv->first_monitor_index;  i <= dsv->last_monitor_index;  i++ )
  {
    laim = HaArray(dsv->monitors, i);
    res += KheMonitorDevToCost((KHE_MONITOR) laim, dev);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceIsAvailable(KHE_RESOURCE r, KHE_SOLN soln,               */
/*    int first_index, int last_index, KHE_FRAME days_frame)                 */
/*                                                                           */
/*  If r is available at time groups first_index to last_index of            */
/*  days_frame, return true.  Otherwise return false.                        */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceIsAvailable(KHE_RESOURCE r, KHE_SOLN soln,
  int first_index, int last_index, KHE_FRAME days_frame)
{
  int i;  KHE_TIME_GROUP tg;  KHE_RESOURCE_TIMETABLE_MONITOR rtm;
  rtm = KheResourceTimetableMonitor(soln, r);
  for( i = first_index;  i <= last_index;  i++)
  {
    tg = KheFrameTimeGroup(days_frame, i);
    if( !KheResourceTimetableMonitorFreeForTimeGroup(rtm, tg, NULL,
	  NULL, false) )
      return false;
    if( !KheResourceTimetableMonitorAvailableForTimeGroup(rtm, tg) )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheResourceGroupHasAvailableResource(KHE_RESOURCE_GROUP domain,     */
/*    KHE_SOLN soln, int first_index, int last_index, KHE_FRAME days_frame,  */
/*    KHE_RESOURCE *r)                                                       */
/*                                                                           */
/*  If some element of domain is available at time groups first_index to     */
/*  last_index of days_frame, return true with *r set to one such resource,  */
/*  Otherwise return false.                                                  */
/*                                                                           */
/*****************************************************************************/

static bool KheResourceGroupHasAvailableResource(KHE_RESOURCE_GROUP domain,
  KHE_SOLN soln, int first_index, int last_index, KHE_FRAME days_frame,
  KHE_RESOURCE *r)
{
  int i;
  if( first_index < 0 )
    first_index = 0;
  for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
  {
    *r = KheResourceGroupResource(domain, i);
    if( KheResourceIsAvailable(*r, soln, first_index, last_index, days_frame) )
      return true;
  }
  return *r = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheDpgExtenderGroupCost(KHE_DPG_EXTENDER de,                    */
/*    KHE_DPG_TASK_GROUP last_dt)                                            */
/*                                                                           */
/*  Return the cost to some resource r of assigning r to last_dt's group.    */
/*  As a special case, optional groups have cost 0, because we don't need    */
/*  to assign a resource to them at all.                                     */
/*                                                                           */
/*****************************************************************************/
static bool KheDpgCostCacheContainsCost(KHE_DPG_COST_CACHE dcc,
  int first_index, int last_index, KHE_COST *cost);
static void KheDpgCostCacheAddCost(KHE_DPG_COST_CACHE dcc, int first_index,
  int last_index, KHE_COST cost);

static KHE_COST KheDpgExtenderGroupCost(KHE_DPG_EXTENDER de,
  KHE_DPG_TASK_GROUP last_dt)
{
  int li, fi;  KHE_DPG_TASK_GROUP dt;  KHE_GROUP_MONITOR gm;  KHE_COST res;
  KHE_RESOURCE r, task_asst;  bool last;
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_DPG_SOLVER dsv;

  /* optional groups have cost 0 */
  if( last_dt->optional )
    return 0;

  /* find the interval of days covered by dt, including history if any */
  dsv = de->solver;
  li = de->next_dtg->index - 1;
  fi = li - last_dt->duration + 1;
  /* ***
  fi = li;
  for( dt = last_dt->prev_task_group;  dt != NULL;  dt = dt->prev_task_group )
    fi--;
  fi -= last_dt->history;
  *** */

  /* if there is a cached value, return that */
  if( KheDpgCostCacheContainsCost(dsv->cost_cache, fi, li, &res) )
    return res;

  /* find a suitable resource, r */
  if( last_dt->group_asst != NULL )
    r = last_dt->group_asst;
  else if( !KheResourceGroupHasAvailableResource(last_dt->group_domain,
    dsv->soln, fi, li, dsv->days_frame, &r) )
  {
    /* quit early with a large cost */
    return KheCost(INT_MAX / 2, INT_MAX);
  }

  /* assign r to those tasks which are not already assigned r */
  for( dt = last_dt;  dt != NULL;  dt = dt->prev_task_group )
  {
    task_asst = KheTaskAsstResource(dt->task);
    dt->tmp_flag = (task_asst != NULL);
    if( dt->tmp_flag )
      HnAssert(r == task_asst, "KheDpgExtenderGroupCost internal error:  r =="
        " %s, dt->task assigned %s\n", r == NULL ? "NULL" : KheResourceId(r),
	task_asst == NULL ? "NULL " : KheResourceId(task_asst));
    else
    {
      if( !KheTaskAssignResource(dt->task, r) )
	HnAbort("KheDpgExtenderGroupCost internal error:  cannot assign %s "
	  "to %s\n", KheResourceId(r), KheTaskId(dt->task));
    }
  }

  /* build a group monitor covering those days +- 1 and get its cost */
  gm = KheGroupMonitorMake(dsv->soln, 11, "interval grouping");
  rtm = KheResourceTimetableMonitor(dsv->soln, r);
  KheResourceTimetableMonitorAddInterval(rtm, dsv->days_frame, fi-1, li+1, gm);
  res = KheMonitorCost((KHE_MONITOR) gm);
  KheGroupMonitorDelete(gm);

  /* unassign r from those tasks we assigned it to */
  for( dt = last_dt;  dt != NULL;  dt = dt->prev_task_group )
  {
    if( !dt->tmp_flag )
    {
      if( !KheTaskUnAssignResource(dt->task) )
	HnAbort("KheDpgExtenderGroupCost internal error:  cannot unassign %s "
	  "from %s\n", KheResourceId(r), KheTaskId(dt->task));
    }
  }

  /* add in the cost of dsv's limit active intervals monitor */
  last = (li == HaArrayCount(dsv->time_groups) - 1);
  res += KheLimitActiveCost(dsv, li - fi + 1, last);

  /* add in the marginal cost, if not optional */
  if( !last_dt->optional )
    res += dsv->marginal_cost * (li - fi + 1);

  /* cache the result and return it */
  KheDpgCostCacheAddCost(dsv->cost_cache, fi, li, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_EXTENDER and the extension operation"                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheUndersizedGroups(KHE_DPG_EXTENDER de)                             */
/*                                                                           */
/*  Return the number of undersized groups in ds.                            */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheUndersizedGroups(KHE_DPG_SOLN ds, int min_limit)
{
  KHE_DPG_TASK_GROUP dt;  int i, res;
  res = 0;
  if( ds != NULL )
    HaArrayForEach(ds->task_groups, dt, i)
      if( dt->extension == 0 && dt->group_length < min_limit )
        res++;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailableTasks(KHE_DPG_TIME_GROUP dtg)                            */
/*                                                                           */
/*  Return the number of admissible tasks starting in dtg, and hence         */
/*  available for assignment.                                                */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static int KheAvailableTasks(KHE_DPG_TIME_GROUP dtg)
{
  int res, i;  KHE_MTASK mt;
  res = 0;
  HaArrayForEach(dtg->starting_mtasks, mt, i)
    res += KheMTaskNeedsAssignmentTaskCount(mt);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderReset(KHE_DPG_EXTENDER de, KHE_DPG_SOLVER dsv,        */
/*    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg)                     */
/*                                                                           */
/*  Reset de.  This function assumes that de->solver is set correctly.       */
/*                                                                           */
/*****************************************************************************/

static void KheDpgExtenderReset(KHE_DPG_EXTENDER de,
    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg)
{
  int count;
  de->prev_ds = prev_ds;
  KheDpgSolnReset(de->next_ds, prev_ds, de->solver);
  de->next_dtg = next_dtg;
  HaArrayClear(de->task_used);
  count = (prev_ds != NULL ? HaArrayCount(prev_ds->task_groups) : 0);
  HaArrayFill(de->task_used, count, false);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_EXTENDER KheDpgExtenderMake(KHE_DPG_SOLVER dsv,                  */
/*    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg,                     */
/*                                                                           */
/*  Make a new extender object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_EXTENDER KheDpgExtenderMake(KHE_DPG_SOLVER dsv,
    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg)
{
  KHE_DPG_EXTENDER res;
  HaMake(res, dsv->arena);
  res->solver = dsv;
  res->next_ds = KheDpgSolnMake(NULL, dsv);
  HaArrayInit(res->task_used, dsv->arena);
  KheDpgExtenderReset(res, prev_ds, next_dtg);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskHistory(KHE_TASK task, KHE_DPG_EXTENDER de)                   */
/*                                                                           */
/*  Return the history of a new group beginning with task.  This will        */
/*  possibly be non-zero when task is assigned a resource and we are         */
/*  at the first time group.                                                 */
/*                                                                           */
/*****************************************************************************/

static int KheTaskHistory(KHE_TASK task, KHE_DPG_EXTENDER de)
{
  KHE_RESOURCE r;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT laic;
  int res, i, history;  KHE_DPG_SOLVER dsv;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;
  r = KheTaskAsstResource(task);
  res = 0;
  if( r != NULL && de->next_dtg->index == 0 )
  {
    dsv = de->solver;
    for( i = dsv->first_monitor_index;  i <= dsv->last_monitor_index;  i++ )
    {
      laim = HaArray(dsv->monitors, i);
      laic = KheLimitActiveIntervalsMonitorConstraint(laim);
      history = KheLimitActiveIntervalsConstraintHistory(laic, r);
      res = max(res, history);
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTaskGroupUndersized(KHE_DPG_TASK_GROUP dt, KHE_DPG_SOLVER dsv)*/
/*                                                                           */
/*  Return true if dt is undersized.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTaskGroupUndersized(KHE_DPG_TASK_GROUP dt, KHE_DPG_SOLVER dsv)
{
  return dt->duration < dsv->min_limit;
  /* return dt->group_length + dt->history < dsv->min_limit; */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderTryStartingTasksB(KHE_DPG_EXTENDER de,                */
/*    int mtask_index, int task_index, int prev_i)                           */
/*                                                                           */
/*  Try all ways to add one starting task to de->next_ds and recurse.        */
/*                                                                           */
/*****************************************************************************/
static void KheDpgExtenderTryStartingTasksA(KHE_DPG_EXTENDER de,
    int mtask_index);

static void KheDpgExtenderTryStartingTasksB(KHE_DPG_EXTENDER de,
    int mtask_index, int task_index, int prev_i,
    int skipped_undersized_count)
{
  KHE_MTASK mt;  KHE_COST non_asst_cost, asst_cost;
  KHE_DPG_TASK_GROUP prev_dt, next_dt;  KHE_DPG_SOLVER dsv;
  KHE_TASK task;  int i, task_total_durn, count;  bool optional;

  mt = HaArray(de->next_dtg->starting_mtasks, mtask_index);
  count = HaArray(de->next_dtg->starting_counts, mtask_index);
  if( task_index >= count )
  {
    /* finished with this mtask, go on to the next mtask */
    KheDpgExtenderTryStartingTasksA(de, mtask_index + 1);
  }
  else
  {
    /* try all groupings of task with a task from de->prev_ds */
    dsv = de->solver;
    task = KheMTaskTask(mt, task_index, &non_asst_cost, &asst_cost);
    optional = (non_asst_cost <= asst_cost);
    task_total_durn = KheTaskTotalDuration(task);
    i = prev_i + 1;
    if( de->prev_ds != NULL )
    {
      for( ;  i < HaArrayCount(de->prev_ds->task_groups);  i++ )
      {
	if( !HaArray(de->task_used, i) )
	{
	  prev_dt = HaArray(de->prev_ds->task_groups, i);
	  if( prev_dt->extension > 0 )  break;
	  if( KheDpgTaskGroupExtendable(prev_dt, task,
		/* non_asst_cost, asst_cost, */
	        task_total_durn, optional, dsv, &next_dt) )
	  {
	    HaArrayAddLast(de->next_ds->task_groups, next_dt);
	    HaArrayPut(de->task_used, i, true);
	    KheDpgExtenderTryStartingTasksB(de, mtask_index,
		task_index + 1, i, skipped_undersized_count);
	    if( !prev_dt->optional && KheDpgTaskGroupUndersized(prev_dt, dsv) )
	      skipped_undersized_count++;
	    HaArrayPut(de->task_used, i, false);
	    HaArrayDeleteLast(de->next_ds->task_groups);
	    KheDpgTaskGroupFree(next_dt, dsv);
	  }
	}
      }
    }

    /* try a fresh start for task, if permissible */
    if( skipped_undersized_count == 0 )
    {
      next_dt = KheDpgTaskGroupMake(task, non_asst_cost <= asst_cost,
	task_total_durn + KheTaskHistory(task, de), task_total_durn - 1,
	KheTaskDomain(task), KheTaskAsstResource(task), NULL, dsv);
      HaArrayAddLast(de->next_ds->task_groups, next_dt);
      KheDpgExtenderTryStartingTasksB(de, mtask_index, task_index + 1, i,
	skipped_undersized_count);
      HaArrayDeleteLast(de->next_ds->task_groups);
      KheDpgTaskGroupFree(next_dt, dsv);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderFinishedCostAdd(KHE_DPG_SOLN ds, KHE_DPG_EXTENDER de) */
/*                                                                           */
/*  Finish off all unused tasks.  This amounts to adding their costs         */
/*  to the cost of ds.  Optional tasks have cost 0.                          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgExtenderFinishedCostAdd(KHE_DPG_SOLN ds, KHE_DPG_EXTENDER de)
{
  int i;  KHE_DPG_TASK_GROUP prev_dt;
  if( de->prev_ds != NULL )
    for( i = 0;  i < HaArrayCount(de->prev_ds->task_groups);  i++ )
      if( !HaArray(de->task_used, i) )
      {
	prev_dt = HaArray(de->prev_ds->task_groups, i);
	HnAssert(prev_dt->extension == 0,
	  "KheDpgExtenderFinishedCostAdd internal error");
	ds->cost += KheDpgExtenderGroupCost(de, prev_dt);
      }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderTryStartingTasksA(KHE_DPG_EXTENDER de,                */
/*    int mtask_index)                                                       */
/*                                                                           */
/*  Try all ways to add to de->next_ds the tasks of one starting mtask,      */
/*  and recurse.                                                             */
/*                                                                           */
/*****************************************************************************/

static void KheDpgExtenderTryStartingTasksA(KHE_DPG_EXTENDER de,
    int mtask_index)
{
  KHE_DPG_SOLN ds;
  if( DEBUG7 )
    fprintf(stderr, "[ TryStartingTasksA(de, %d)%s\n", mtask_index,
     mtask_index >= HaArrayCount(de->next_dtg->starting_mtasks) ? " base" : "");
  if( mtask_index >= HaArrayCount(de->next_dtg->starting_mtasks) )
  {
    /* base of recursion; copy de->next_ds, sort its tasks, find its cost */
    ds = KheDpgSolnCopy(de->next_ds, de->solver);
    HaArraySort(ds->task_groups, &KheDpgTaskGroupCmp);
    KheDpgExtenderFinishedCostAdd(ds, de);
    KheDpgTimeGroupAddSoln(de->next_dtg, ds);
  }
  else
  {
    /* explore assignments to the first task of mt */
    KheDpgExtenderTryStartingTasksB(de, mtask_index, 0, -1, 0);
  }
  if( DEBUG7 )
    fprintf(stderr, "] TryStartingTasksA returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgExtenderAddContinuingTasks(KHE_DPG_EXTENDER de)                */
/*                                                                           */
/*  Add to de->next_ds the continuing tasks from de->prev_ds (those whose    */
/*  extension is non-zero).  Return the number of tasks added.               */
/*                                                                           */
/*****************************************************************************/

static int KheDpgExtenderAddContinuingTasks(KHE_DPG_EXTENDER de)
{
  KHE_DPG_TASK_GROUP dt, new_dt;  int i, res;
  res = 0;
  if( de->prev_ds != NULL )
    HaArrayForEachReverse(de->prev_ds->task_groups, dt, i)
    {
      if( dt->extension == 0 )
	break;
      new_dt = KheDpgTaskGroupMake(dt->task, dt->optional,
	/* dt->history, dt->group_length, */ dt->duration,
	dt->extension - 1, dt->group_domain, dt->group_asst, dt, de->solver);
      HaArrayAddLast(de->next_ds->task_groups, new_dt);
      HaArrayPut(de->task_used, i, true);
      if( DEBUG6 )
	fprintf(stderr, "  continuing task %s (duration %d)\n",
	  KheTaskId(dt->task), dt->duration /*dt->history, dt->group_length*/);
      res++;
    }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderDeleteContinuingTasks(KHE_DPG_EXTENDER de, int num)   */
/*                                                                           */
/*  Delete the continuing tasks of de.  There are num of them.               */
/*                                                                           */
/*****************************************************************************/

static void KheDpgExtenderDeleteContinuingTasks(KHE_DPG_EXTENDER de, int num)
{
  int i;  KHE_DPG_TASK_GROUP dt;
  for( i = 0;  i < num;  i++ )
  {
    dt = HaArrayLastAndDelete(de->next_ds->task_groups);
    KheDpgTaskGroupFree(dt, de->solver);
    /* HaArrayPut(de->task_used, i, false); not quite accurate; not needed */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgExtenderExtend(KHE_DPG_EXTENDER de,                           */
/*    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg)                     */
/*                                                                           */
/*  Using de, extend prev_ds in all possible ways into next_dtg.             */
/*                                                                           */
/*****************************************************************************/

static void KheDpgExtenderExtend(KHE_DPG_EXTENDER de,
    KHE_DPG_SOLN prev_ds, KHE_DPG_TIME_GROUP next_dtg)
{
  int continuing_count;
  KheDpgExtenderReset(de, prev_ds, next_dtg);
  continuing_count = KheDpgExtenderAddContinuingTasks(de);
  KheDpgExtenderTryStartingTasksA(de, 0);
  KheDpgExtenderDeleteContinuingTasks(de, continuing_count);
  HnAssert(HaArrayCount(de->next_ds->task_groups) == 0,
    "KheDpgExtenderExtend internal error");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_COST_CACHE_SUB"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_COST_CACHE_SUB KheDpgCostCacheSubMake(KHE_DPG_SOLVER dsv)        */
/*                                                                           */
/*  Make a new cost cache sub object.                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_COST_CACHE_SUB KheDpgCostCacheSubMake(KHE_DPG_SOLVER dsv)
{
  KHE_DPG_COST_CACHE_SUB res;
  if( HaArrayCount(dsv->dpg_cost_cache_sub_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(dsv->dpg_cost_cache_sub_free_list);
    HaArrayClear(res->costs);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->costs, dsv->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheSubFree(KHE_DPG_COST_CACHE_SUB sub,                  */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Free sub,                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheDpgCostCacheSubFree(KHE_DPG_COST_CACHE_SUB sub,
  KHE_DPG_SOLVER dsv)
{
  HaArrayAddLast(dsv->dpg_cost_cache_sub_free_list, sub);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheSubDebug(KHE_DPG_COST_CACHE_SUB sub, int first_index,*/
/*    KHE_DPG_SOLVER dsv, int verbosity, int indent, FILE *fp)               */
/*                                                                           */
/*  Debug print of the cache sub indexed by first_index.  NB first_index     */
/*  could be negative.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheDpgCostCacheSubDebug(KHE_DPG_COST_CACHE_SUB sub, int first_index,
  KHE_DPG_SOLVER dsv, int verbosity, int indent, FILE *fp)
{
  KHE_COST cost;  int last_index;
  KHE_DPG_TIME_GROUP dtg1, dtg2;
  if( sub != NULL )
    HaArrayForEach(sub->costs, cost, last_index)
      if( cost != -1 )
      {
	if( first_index >= 0 )
	{
	  dtg1 = HaArray(dsv->time_groups, first_index);
	  dtg2 = HaArray(dsv->time_groups, last_index);
	  fprintf(fp, "%*s  %s-%s: %.5f\n", indent, "",
	      KheTimeGroupId(dtg1->time_group),
	      KheTimeGroupId(dtg2->time_group), KheCostShow(cost));
	}
	else
	{
	  dtg2 = HaArray(dsv->time_groups, last_index);
	  fprintf(fp, "%*s  (%d)-%s: %.5f\n", indent, "", first_index,
	      KheTimeGroupId(dtg2->time_group), KheCostShow(cost));
	}
      }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_COST_CACHE"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_COST_CACHE KheDpgCostCacheMake(KHE_DPG_SOLVER dsv)               */
/*                                                                           */
/*  Make a new, empty cost cache object for dsv.                             */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_COST_CACHE KheDpgCostCacheMake(KHE_DPG_SOLVER dsv)
{
  KHE_DPG_COST_CACHE res;
  HaMake(res, dsv->arena);
  res->solver = dsv;
  HaArrayInit(res->subs, dsv->arena);
  HaArrayInit(res->neg_subs, dsv->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheClear(KHE_DPG_COST_CACHE dcc)                        */
/*                                                                           */
/*  Clear dcc back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheDpgCostCacheClear(KHE_DPG_COST_CACHE dcc)
{
  KHE_DPG_COST_CACHE_SUB sub;  int i;

  /* clear dcc->subs */
  HaArrayForEach(dcc->subs, sub, i)
    KheDpgCostCacheSubFree(sub, dcc->solver);
  HaArrayClear(dcc->subs);

  /* clear dcc->neg_subs */
  HaArrayForEach(dcc->neg_subs, sub, i)
    KheDpgCostCacheSubFree(sub, dcc->solver);
  HaArrayClear(dcc->neg_subs);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheAddCost(KHE_DPG_COST_CACHE dcc, int first_index,     */
/*    int last_index, KHE_COST cost)                                         */
/*                                                                           */
/*  Add cost to dcc, indexed by (first_index, last_index).                   */
/*                                                                           */
/*****************************************************************************/

static void KheDpgCostCacheAddCost(KHE_DPG_COST_CACHE dcc, int first_index,
  int last_index, KHE_COST cost)
{
  KHE_DPG_COST_CACHE_SUB sub;

  /* get sub, either existing or newly made */
  if( first_index < 0 )
  {
    first_index = - first_index;
    HaArrayFill(dcc->neg_subs, first_index + 1, NULL);
    sub = HaArray(dcc->neg_subs, first_index);
    if( sub == NULL )
    {
      sub = KheDpgCostCacheSubMake(dcc->solver);
      HaArrayPut(dcc->neg_subs, first_index, sub);
    }
  }
  else
  {
    HaArrayFill(dcc->subs, first_index + 1, NULL);
    sub = HaArray(dcc->subs, first_index);
    if( sub == NULL )
    {
      sub = KheDpgCostCacheSubMake(dcc->solver);
      HaArrayPut(dcc->subs, first_index, sub);
    }
  }

  /* add the cost to sub */
  HaArrayFill(sub->costs, last_index + 1, -1 /* means NULL */);
  HaArrayPut(sub->costs, last_index, cost);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgCostCacheContainsCost(KHE_DPG_COST_CACHE dcc,                 */
/*    int first_index, int last_index, KHE_COST *cost)                       */
/*                                                                           */
/*  If dcc contains a cost at index (first_index, last_index), set *cost     */
/*  to that cost and return true.  Otherwise return false.  Note that        */
/*  first_index could be negative.                                           */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgCostCacheContainsCost(KHE_DPG_COST_CACHE dcc,
  int first_index, int last_index, KHE_COST *cost)
{
  KHE_DPG_COST_CACHE_SUB sub;
  if( first_index < 0 )
  {
    /* use the negative of first_index as the index */
    first_index = - first_index;
    if( first_index < HaArrayCount(dcc->neg_subs) )
    {
      sub = HaArray(dcc->neg_subs, first_index);
      if( sub == NULL )
	return *cost = 0, false;
      else if( 0 <= last_index && last_index < HaArrayCount(sub->costs) )
      {
	*cost = HaArray(sub->costs, last_index);
	if( *cost == -1 )
	  return *cost = 0, false;
	else
	  return true;
      }
      else
	return *cost = 0, false;
    }
    else
      return *cost = 0, false;
  }
  else if( 0 <= first_index && first_index < HaArrayCount(dcc->subs) )
  {
    sub = HaArray(dcc->subs, first_index);
    if( sub == NULL )
      return *cost = 0, false;
    else if( 0 <= last_index && last_index < HaArrayCount(sub->costs) )
    {
      *cost = HaArray(sub->costs, last_index);
      if( *cost == -1 )
	return *cost = 0, false;
      else
	return true;
    }
    else
      return *cost = 0, false;
  }
  else
    return *cost = 0, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheDebug(KHE_DPG_COST_CACHE dcc, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of dcc onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgCostCacheDebug(KHE_DPG_COST_CACHE dcc, int verbosity,
    int indent, FILE *fp)
{
  int first_index;  KHE_DPG_SOLVER dsv;  KHE_DPG_COST_CACHE_SUB sub;
  if( indent >= 0 )
  {
    dsv = dcc->solver;
    fprintf(fp, "%*s[ CostCache\n", indent, "");
    HaArrayForEachReverse(dcc->neg_subs, sub, first_index)
      KheDpgCostCacheSubDebug(sub, - first_index, dsv, verbosity, indent, fp);
    HaArrayForEach(dcc->subs, sub, first_index)
      KheDpgCostCacheSubDebug(sub, first_index, dsv, verbosity, indent, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "CostCache");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TIMETABLE_ENTRY"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TIMETABLE_ENTRY KheDpgTimetableEntryMake(KHE_DPG_SOLVER dsv,     */
/*    KHE_DPG_TASK_GROUP dt)                                                 */
/*                                                                           */
/*  Make an empty timetable entry.                                           */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TIMETABLE_ENTRY KheDpgTimetableEntryMake(KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TIMETABLE_ENTRY res;
  if( HaArrayCount(dsv->dpg_timetable_entry_free_list) > 0 )
    res = HaArrayLastAndDelete(dsv->dpg_timetable_entry_free_list);
  else
    HaMake(res, dsv->arena);
  res->task = NULL;
  res->continuing = false;
  res->durn_char = ' ';
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableEntryFree(KHE_DPG_TIMETABLE_ENTRY dtte,              */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Free dtte.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableEntryFree(KHE_DPG_TIMETABLE_ENTRY dtte,
    KHE_DPG_SOLVER dsv)
{
  HaArrayAddLast(dsv->dpg_timetable_entry_free_list, dtte);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableEntrySet(KHE_DPG_TIMETABLE_ENTRY dtte,               */
/*    KHE_DPG_TASK_GROUP dpg_task_group, KHE_TASK task, char durn_char)      */
/*                                                                           */
/*  Set the attributes of dtte to these values.                              */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableEntrySet(KHE_DPG_TIMETABLE_ENTRY dtte,
    KHE_TASK task, bool continuing, char durn_char)
{
  HnAssert(task != NULL, "KheDpgTimetableEntrySet internal error 2");
  dtte->task = task;
  dtte->continuing = continuing;
  dtte->durn_char = durn_char;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TIMETABLE_ROW"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TIMETABLE_ROW KheDpgTimetableRowMake(KHE_DPG_TIME_GROUP dtg,     */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Make a timetable row with these attributes.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TIMETABLE_ROW KheDpgTimetableRowMake(KHE_DPG_TIME_GROUP dtg,
    KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TIMETABLE_ROW res;
  if( HaArrayCount(dsv->dpg_timetable_row_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(dsv->dpg_timetable_row_free_list);
    HaArrayClear(res->entries);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->entries, dsv->arena);
  }
  res->time_group = dtg;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableRowFree(KHE_DPG_TIMETABLE_ROW dttr,                  */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Free dttr.  Also free its entries.                                       */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableRowFree(KHE_DPG_TIMETABLE_ROW dttr,
    KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TIMETABLE_ENTRY dtte;  int i;

  /* free the entries */
  HaArrayForEach(dttr->entries, dtte, i)
    KheDpgTimetableEntryFree(dtte, dsv);

  /* free the row */
  HaArrayAddLast(dsv->dpg_timetable_row_free_list, dttr);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_TIMETABLE"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableAddEmptyColumn(KHE_DPG_TIMETABLE dtt)                */
/*                                                                           */
/*  Add an empty column to the end of dtt.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableAddEmptyColumn(KHE_DPG_TIMETABLE dtt)
{
  KHE_DPG_TIMETABLE_ROW dttr;  int i;  KHE_DPG_TIMETABLE_ENTRY dtte;
  HaArrayForEach(dtt->rows, dttr, i)
  {
    dtte = KheDpgTimetableEntryMake(dtt->solver);
    HaArrayAddLast(dttr->entries, dtte);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTimetableColumnAvail(KHE_DPG_TIMETABLE dtt, int col_index,    */
/*    int first_row_index, int last_row_index)                               */
/*                                                                           */
/*  If the column with index col_index is available from first_row_index     */
/*  to last_row_index inclusive, then return true, else return false.        */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTimetableColumnAvail(KHE_DPG_TIMETABLE dtt, int col_index,
    int first_row_index, int last_row_index)
{
  int i;  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_TIMETABLE_ENTRY dtte;
  for( i = first_row_index;  i <= last_row_index;  i++ )
  {
    dttr = HaArray(dtt->rows, i);
    dtte = HaArray(dttr->entries, col_index);
    if( dtte->task != NULL )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTimetableFindOrMakeFreeColumn(KHE_DPG_TIMETABLE dtt,           */
/*    int first_row_index, int last_row_index)                               */
/*                                                                           */
/*  Find or make a free column from first_row_index to last_row_index.       */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTimetableFindOrMakeFreeColumn(KHE_DPG_TIMETABLE dtt,
    int first_row_index, int last_row_index)
{
  KHE_DPG_TIMETABLE_ROW dttr;  int col_count, i;

  /* find out the current number of columns */
  if( HaArrayCount(dtt->rows) == 0 )
    col_count = 0;
  else
  {
    dttr = HaArrayFirst(dtt->rows);
    col_count = HaArrayCount(dttr->entries);
  }

  /* if an existing column will do the job, return its index */
  for( i = 0;  i < col_count;  i++ )
    if( KheDpgTimetableColumnAvail(dtt, i, first_row_index, last_row_index) )
      return i;

  /* otherwise add an empty column and return its index */
  KheDpgTimetableAddEmptyColumn(dtt);
  return col_count;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskGroupGroupDuration(KHE_DPG_TASK_GROUP dt)                  */
/*                                                                           */
/*  Return the duration of dt.                                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheDpgTaskGroupGroupDuration(KHE_DPG_TASK_GROUP dt)
{
  int res;
  res = 0;
  while( dt != NULL )
    res++, dt = dt->prev_task_group;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char KheDurnChar(KHE_DPG_TIMETABLE dtt, int durn, bool optional)         */
/*                                                                           */
/*  Return a single character indicating how dc compares with the            */
/*  expected duration.                                                       */
/*                                                                           */
/*****************************************************************************/

static char KheDurnChar(KHE_DPG_TIMETABLE dtt, int durn, bool optional,
  bool at_end, bool asst_is_fixed)
{
  if( durn < dtt->solver->min_limit && !optional && !at_end )
  {
    dtt->undersized_durn += (dtt->solver->min_limit - durn);
    return '#';
  }
  else if( durn > dtt->solver->max_limit )
  {
    dtt->oversized_durn += (durn - dtt->solver->min_limit);
    return '$';
  }
  else if( asst_is_fixed )
    return '@';
  else
    return ' ';
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMeetIsRunningDuringTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)   */
/*                                                                           */
/*  Return true if meet is running during tg.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheMeetIsRunningDuringTimeGroup(KHE_MEET meet, KHE_TIME_GROUP tg)
{
  KHE_TIME start_time;  int durn, i, pos;
  start_time = KheMeetAsstTime(meet);
  if( start_time != NULL )
  {
    durn = KheMeetDuration(meet);
    for( i = 0;  i < durn;  i++ )
      if( KheTimeGroupContains(tg, KheTimeNeighbour(start_time, i), &pos) )
	return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)        */
/*                                                                           */
/*  Return the descendant of task running during tg, or NULL if none.        */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheTaskDuringTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TASK res, child_task;  int i;

  /* do it for task */
  meet = KheTaskMeet(task);
  if( meet != NULL && KheMeetIsRunningDuringTimeGroup(meet, tg) )
    return task;

  /* do it for task's proper descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    res = KheTaskDuringTimeGroup(child_task, tg);
    if( res != NULL )
      return res;
  }

  /* no luck, return NULL */
  if( DEBUG4 )
    fprintf(stderr, "  KheTaskDuringTimeGroup(%s, %s) = NULL\n",
      KheTaskId(task), KheTimeGroupId(tg));
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableAddDpgTask(KHE_DPG_TIMETABLE dtt,                    */
/*    KHE_DPG_TASK_GROUP dt, int last_row_index)                             */
/*                                                                           */
/*  Add dt to dtt.  Its last row has index last_row_index.                   */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableAddDpgTask(KHE_DPG_TIMETABLE dtt,
  KHE_DPG_TASK_GROUP dt, int last_row_index)
{
  int first_row_index, col_index, i;  char durn_char;
  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_TIMETABLE_ENTRY dtte;
  KHE_TASK task;  KHE_DPG_TIME_GROUP dtg;  bool continuing;

  /* find the duration and durn_char */
  /* durn = KheDpgTaskGroupGroupDuration(dt);  not including history */
  durn_char = KheDurnChar(dtt, dt->duration, dt->optional,
    last_row_index == HaArrayCount(dtt->solver->time_groups) - 1,
    KheTaskAssignIsFixed(dt->task));

  /* find the indexes delimiting where dt will go */
  first_row_index = max(0, last_row_index - dt->duration + 1);
  col_index = KheDpgTimetableFindOrMakeFreeColumn(dtt,
    first_row_index, last_row_index);

  /* add dt at those indexes */
  for( i = last_row_index;  i >= first_row_index;  i-- )
  {
    dttr = HaArray(dtt->rows, i);
    dtte = HaArray(dttr->entries, col_index);
    dtg = HaArray(dtt->solver->time_groups, i);
    task = KheTaskDuringTimeGroup(dt->task, dtg->time_group);
    continuing = (dt->prev_task_group != NULL);
    KheDpgTimetableEntrySet(dtte, task, continuing, durn_char);
    dt = dt->prev_task_group;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnTimeGroupCount(KHE_DPG_SOLN ds)                            */
/*                                                                           */
/*  Return the number of time groups covered by ds.  This is just the        */
/*  length of the chain of solutions leading back from ds.                   */
/*                                                                           */
/*****************************************************************************/

static int KheDpgSolnTimeGroupCount(KHE_DPG_SOLN ds)
{
  int res;
  res = 0;
  while( ds != NULL )
    res++, ds = ds->prev_ds;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTaskGroupIsPredecessor(KHE_DPG_TASK_GROUP dt,                 */
/*    KHE_DPG_SOLN next_ds)                                                  */
/*                                                                           */
/*  Return true if dt is the predecessor of a task in next_ds, or false      */
/*  if nor or if next_ds is NULL.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTaskGroupIsPredecessor(KHE_DPG_TASK_GROUP dt,
  KHE_DPG_SOLN next_ds)
{
  KHE_DPG_TASK_GROUP dt2;  int i;
  if( next_ds != NULL )
    HaArrayForEach(next_ds->task_groups, dt2, i)
      if( dt2->prev_task_group == dt )
	return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableAddSoln(KHE_DPG_TIMETABLE dtt, KHE_DPG_SOLN ds,      */
/*    int last_row_index, KHE_DPG_SOLN next_ds)                              */
/*                                                                           */
/*  Add ds to dtt at last_row_index.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableAddSoln(KHE_DPG_TIMETABLE dtt, KHE_DPG_SOLN ds,
  int last_row_index, KHE_DPG_SOLN next_ds)
{
  KHE_DPG_TASK_GROUP dt;  int i;
  HaArrayForEach(ds->task_groups, dt, i)
    if( !KheDpgTaskGroupIsPredecessor(dt, next_ds) )
      KheDpgTimetableAddDpgTask(dtt, dt, last_row_index);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TIMETABLE KheDpgTimetableMake(KHE_DPG_SOLN ds,                   */
/*    KHE_DPG_SOLVER dsv)                                                    */
/*                                                                           */
/*  Make a timetable holding ds.                                             */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TIMETABLE KheDpgTimetableMake(KHE_DPG_SOLN ds,
  KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TIMETABLE res;  int i, tg_count;  KHE_DPG_TIME_GROUP dtg;
  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_SOLN next_ds;

  /* make the basic object */
  if( HaArrayCount(dsv->dpg_timetable_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(dsv->dpg_timetable_free_list);
    HaArrayClear(res->rows);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->rows, dsv->arena);
  }
  res->solver = dsv;
  res->soln = ds;
  res->undersized_durn = 0;
  res->oversized_durn = 0;

  /* add one row for each time group covered by soln */
  tg_count = KheDpgSolnTimeGroupCount(ds);
  for( i = 0;  i < tg_count;  i++ )
  {
    dtg = HaArray(dsv->time_groups, i);
    dttr = KheDpgTimetableRowMake(dtg, dsv);
    HaArrayAddLast(res->rows, dttr);
  }

  /* add solns */
  next_ds = NULL;
  for( i = tg_count - 1;  i >= 0;  i-- )
  {
    KheDpgTimetableAddSoln(res, ds, i, next_ds);
    next_ds = ds;
    ds = ds->prev_ds;
  }
  HnAssert(ds == NULL, "KheDpgTimetableMake internal error");

  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableFree(KHE_DPG_TIMETABLE dtt, KHE_DPG_SOLVER dsv)      */
/*                                                                           */
/*  Free dtt.                                                                */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableFree(KHE_DPG_TIMETABLE dtt)
{
  KHE_DPG_TIMETABLE_ROW dttr;  int i;

  /* free the rows */
  HaArrayForEach(dtt->rows, dttr, i)
    KheDpgTimetableRowFree(dttr, dtt->solver);

  /* free dtt */
  HaArrayAddLast(dtt->solver->dpg_timetable_free_list, dtt);
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheDpgTimetableEntryShow2(KHE_DPG_TIMETABLE_ENTRY dtte,            */
/*    char buff[KHE_COL_WIDTH + 1])                                          */
/*                                                                           */
/*  Show dtte's second row in buff and return buff.                          */
/*                                                                           */
/*****************************************************************************/

static char *KheDpgTimetableEntryShow2(KHE_DPG_TIMETABLE_ENTRY dtte,
  char buff[KHE_COL_WIDTH + 1])
{
  KHE_COST non_asst_cost, asst_cost;  KHE_RESOURCE_GROUP domain;
  if( dtte->task == NULL )
    snprintf(buff, KHE_COL_WIDTH, "%s", "");
  else if( KheTaskAsstResource(dtte->task) != NULL )
  {
    /* print the assigned resource */
    snprintf(buff, KHE_COL_WIDTH, "%s%c",
      KheResourceId(KheTaskAsstResource(dtte->task)),
      dtte->durn_char);
  }
  else
  {
    domain = KheTaskDomain(dtte->task);
    KheTaskNonAsstAndAsstCost(dtte->task, &non_asst_cost, &asst_cost);
    if( non_asst_cost <= asst_cost )
    {
      /* print domain id (or just a count) and soft cost */
      if( KheResourceGroupId(domain) == NULL )
	snprintf(buff, KHE_COL_WIDTH, "(%d)@%d%c",
	  KheResourceGroupResourceCount(domain),
	  KheSoftCost(asst_cost),
	  dtte->durn_char);
      else
	snprintf(buff, KHE_COL_WIDTH, "%s@%d%c",
	  KheResourceGroupId(domain),
	  KheSoftCost(asst_cost),
	  dtte->durn_char);
    }
    else if( non_asst_cost >= KheCost(1, 0) )
    {
      /* print domain and "H" for hard cost */
      snprintf(buff, KHE_COL_WIDTH, "%s/H%c",
	KheResourceGroupId(domain),
	dtte->durn_char);
    }
    else
    {
      /* print domain and soft cost */
      snprintf(buff, KHE_COL_WIDTH, "%s/%d%c",
	KheResourceGroupId(domain),
	KheSoftCost(non_asst_cost),
	dtte->durn_char);
    }
  }
  return buff;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableDebug(KHE_DPG_TIMETABLE dtt, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of dtt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableDebug(KHE_DPG_TIMETABLE dtt, int verbosity,
  int indent, FILE *fp)
{
  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_TIMETABLE_ENTRY dtte;  int i, j;
  char spaces[KHE_COL_WIDTH + 1], dashes[KHE_COL_WIDTH + 1];
  char buff[KHE_COL_WIDTH + 1];
  char *id, *id2;

  /* make the spaces and dashes */
  for( i = 0;  i < KHE_COL_WIDTH;  i++ )
  {
    spaces[i] = ' ';
    dashes[i] = '-';
  }
  spaces[i] = '\0';
  dashes[i] = '\0';

  fprintf(fp, "%*s[ Soln(cost %.5f, undersized_durn %d, oversized_durn %d)\n",
    indent, "", KheCostShow(dtt->soln->cost), dtt->undersized_durn,
    dtt->oversized_durn);
  HaArrayForEach(dtt->rows, dttr, i)
  {
    /* print the separator line above the row */
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    HaArrayForEach(dttr->entries, dtte, j)
      fprintf(fp, "+%s", dtte->continuing ? spaces : dashes);
    fprintf(fp, "+\n");

    /* print the task name line of the row */
    fprintf(fp, "%*s%-*s", indent + 2, "", KHE_COL_WIDTH,
      KheTimeGroupId(dttr->time_group->time_group));
    HaArrayForEach(dttr->entries, dtte, j)
    {
      if( dtte->task == NULL )
	fprintf(fp, "|%*s", KHE_COL_WIDTH, "");
      else
      {
	id = KheTaskId(dtte->task);
	id2 = strstr(id, ":");
	id = (id2 != NULL ? id2 + 1 : id);
	fprintf(fp, "|%*s%c", KHE_COL_WIDTH - 1, id, dtte->durn_char);
      }
    }
    fprintf(fp, "|\n");

    /* print the domain and cost row */
    fprintf(fp, "%*s%-*s", indent + 2, "", KHE_COL_WIDTH, "");
    HaArrayForEach(dttr->entries, dtte, j)
      fprintf(fp, "|%*s", KHE_COL_WIDTH, KheDpgTimetableEntryShow2(dtte, buff));
    fprintf(fp, "|\n");
  }

  /* print the final separator line */
  if( HaArrayCount(dtt->rows) > 0 )
  {
    dttr = HaArrayFirst(dtt->rows);
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    HaArrayForEach(dttr->entries, dtte, j)
      fprintf(fp, "+%s", dashes);
    fprintf(fp, "+\n");
  }

  /* finish off */
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_DPG_SOLVER"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_SOLVER KheDpgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,     */
/*    KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa, HA_ARENA a)                */
/*                                                                           */
/*  Make a dpg solver object with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_SOLVER KheDpgSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_MTASK_FINDER mtf, KHE_SOLN_ADJUSTER sa, HA_ARENA a)
{
  KHE_DPG_SOLVER res;  KHE_BALANCE_SOLVER bs;
  HaMake(res, a);

  /* free lists */
  HaArrayInit(res->dpg_task_and_interval_free_list, a);
  HaArrayInit(res->dpg_time_group_free_list, a);
  HaArrayInit(res->dpg_task_group_free_list, a);
  HaArrayInit(res->dpg_soln_free_list, a);
  HaArrayInit(res->dpg_cost_cache_sub_free_list, a);
  HaArrayInit(res->dpg_timetable_entry_free_list, a);
  HaArrayInit(res->dpg_timetable_row_free_list, a);
  HaArrayInit(res->dpg_timetable_free_list, a);

  /* fields defined (and mostly constant) throughout the solve */
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->mtask_finder = mtf;
  res->days_frame = KheMTaskFinderDaysFrame(mtf);
  res->soln_adjuster = sa;
  bs = KheBalanceSolverMake(res->soln, rt, res->days_frame, a);
  res->marginal_cost = KheBalanceSolverMarginalCost(bs);
  if( DEBUG9 )
    fprintf(stderr, "  KheBalanceSolverMarginalCost = %.5f\n",
      KheCostShow(res->marginal_cost));
  HaArrayInit(res->monitors, a);
  res->groups_count = 0;

  /* fields defined when solving one set of equivalent monitors */
  res->first_monitor_index = -1;
  res->last_monitor_index = -1;
  res->min_limit = -1;
  res->max_limit = -1;
  res->history_before = -1;
  res->history_after = -1;
  HaArrayInit(res->time_groups, a);
  res->cost_cache = KheDpgCostCacheMake(res);

  /* scratch fields */
  HaArrayInit(res->assigned_tasks, a);
  HaArrayInit(res->tasks_and_intervals, a);
  res->extender = KheDpgExtenderMake(res, NULL, NULL);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverAddTimeGroup(KHE_DPG_SOLVER dsv, KHE_TIME_GROUP tg)     */
/*                                                                           */
/*  Add tg to dsv.                                                           */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverAddTimeGroup(KHE_DPG_SOLVER dsv, KHE_TIME_GROUP tg)
{
  KHE_DPG_TIME_GROUP dtg;
  dtg = KheDpgTimeGroupMake(HaArrayCount(dsv->time_groups), tg, dsv);
  HaArrayAddLast(dsv->time_groups, dtg);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverDebug(KHE_DPG_SOLVER dsv, int verbosity, int indent,    */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of dsv onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverDebug(KHE_DPG_SOLVER dsv, int verbosity, int indent,
  FILE *fp)
{
  KHE_DPG_TIME_GROUP dtg;  int i;
  fprintf(fp, "%*s[ DpgSolver(%s)\n", indent, "",
    KheResourceTypeId(dsv->resource_type));
  HaArrayForEach(dsv->time_groups, dtg, i)
    KheDpgTimeGroupDebugMTasks(dtg, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "initial grouping of assigned tasks"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheInitDomainAndLeaderTask(KHE_TASK task,                           */
/*    KHE_RESOURCE_GROUP *domain, KHE_TASK *leader_task)                     */
/*                                                                           */
/*  Initialize *domain and *leader_task to reflect the discovery of task.    */
/*                                                                           */
/*****************************************************************************/

static void KheInitDomainAndLeaderTask(KHE_TASK task,
  KHE_RESOURCE_GROUP *domain, KHE_TASK *leader_task)
{
  *domain = KheTaskDomain(task);
  *leader_task = task;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheUpdateDomainAndLeaderTask(KHE_TASK task,                         */
/*    KHE_RESOURCE_GROUP *domain, KHE_TASK *leader_task)                     */
/*                                                                           */
/*  Update *domain and *leader_task to reflect the discovery of task,        */
/*  and return true if that worked out.                                      */
/*                                                                           */
/*****************************************************************************/

static bool KheUpdateDomainAndLeaderTask(KHE_TASK task,
  KHE_RESOURCE_GROUP *domain, KHE_TASK *leader_task)
{
  if( KheResourceGroupSubset(*domain, KheTaskDomain(task)) )
  {
    /* all good as is */
    return true;
  }
  else if( KheResourceGroupSubset(KheTaskDomain(task), *domain) )
  {
    /* task becomes the leader task */
    *domain = KheTaskDomain(task);
    *leader_task = task;
    return true;
  }
  else
  {
    /* no luck */
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverMakeAssignedGroup(KHE_DPG_SOLVER dsv, KHE_RESOURCE r,   */
/*    KHE_TASK leader_task, int first_index, int last_index)                 */
/*                                                                           */
/*  Make a group from first_index to last_index.                             */
/*                                                                           */
/*  This code includes a horrible patch for carrying on when some of         */
/*  the tasks in the group have fixed assignments.                           */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverMakeAssignedGroup(KHE_DPG_SOLVER dsv, KHE_RESOURCE r,
  int first_index, int last_index)
{
  KHE_DPG_TASK_AND_INTERVAL dti;  int i;
  if( last_index - first_index + 1 >= 2 )
  {
    KheMTaskFinderTaskGrouperClear(dsv->mtask_finder);
    for( i = first_index;  i <= last_index;  i++ )
    {
      dti = HaArray(dsv->tasks_and_intervals, i);
      if( !KheMTaskFinderTaskGrouperAddTask(dsv->mtask_finder, dti->task) )
	HnAbort("KheDpgSolverMakeAssignedGroup internal error");
    }
    KheMTaskFinderTaskGrouperMakeGroup(dsv->mtask_finder, dsv->soln_adjuster);
  }
}

/* ***
static void KheDpgSolverMakeAssignedGroup(KHE_DPG_SOLVER dsv, KHE_RESOURCE r,
  KHE_TASK leader_task, int first_index, int last_index)
{
  KHE_DPG_TASK_AND_INTERVAL dti;  int i;
  if( last_index - first_index + 1 >= 2 )
  {
    KheMTaskFinderGroupBegin(dsv->mtask_finder, leader_task);
    for( i = first_index;  i <= last_index;  i++ )
    {
      dti = HaArray(dsv->tasks_and_intervals, i);
      if( dti->task != leader_task )
      {
	if( dti->task_asst_is_fixed )
	  KheTaskAssignUnFix(dti->task);
	if( !KheMTaskFinderGroupAddTask(dsv->mtask_finder, dti->task) )
	  HnAbort("KheDpgSolverMakeAssignedGroup internal error");
      }
    }
    KheMTaskFinderGroupEnd(dsv->mtask_finder, dsv->soln_adjuster);
    for( i = first_index;  i <= last_index;  i++ )
    {
      dti = HaArray(dsv->tasks_and_intervals, i);
      if( dti->task != leader_task && dti->task_asst_is_fixed )
	KheTaskAssignFix(dti->task);
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)                      */
/*                                                                           */
/*  Typed comparison function for sorting an array of tasks to bring         */
/*  duplicates together.                                                     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

static int KheTaskTypedCmp(KHE_TASK task1, KHE_TASK task2)
{
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskCmp(const void *t1, const void *t2)                           */
/*                                                                           */
/*  Untyped comparison function for sorting an array of tasks to bring       */
/*  duplicates together.                                                     */
/*                                                                           */
/*****************************************************************************/

static int KheTaskCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  return KheTaskTypedCmp(task1, task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverGroupAssignedTasksForResource(KHE_DPG_SOLVER dsv,       */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Carry out an initial grouping of assigned tasks for resource r.          */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverGroupAssignedTasksForResource(KHE_DPG_SOLVER dsv,
  KHE_RESOURCE r)
{
  int i, j, count;  KHE_TASK task, leader_task;  KHE_RESOURCE_GROUP domain;
  KHE_INTERVAL in;  KHE_DPG_TASK_AND_INTERVAL dti, dti_i, dti_j;

  count = KheResourceAssignedTaskCount(dsv->soln, r);
  if( count >= 2 )
  {
    /* find the proper root tasks assigned r and uniqueify them */
    HaArrayClear(dsv->assigned_tasks);
    for( i = 0;  i < count;  i++ )
    {
      task = KheResourceAssignedTask(dsv->soln, r, i);
      HaArrayAddLast(dsv->assigned_tasks, KheTaskProperRoot(task));
    }
    HaArraySortUnique(dsv->assigned_tasks, &KheTaskCmp);

    /* for each proper root task, make one task and interval object */
    /* and sort by interval */
    HaArrayClear(dsv->tasks_and_intervals);
    HaArrayForEach(dsv->assigned_tasks, task, i)
    {
      in = KheTaskInterval(task, dsv->days_frame);
      dti = KheDpgTaskAndIntervalMake(task, in, dsv);
      HaArrayAddLast(dsv->tasks_and_intervals, dti);
    }
    HaArraySort(dsv->tasks_and_intervals, &KheDpgTaskAndIntervalCmp);

    /* traverse the sorted tasks, grouping adjacent ones */
    for( i = 0;  i < HaArrayCount(dsv->tasks_and_intervals);  i = j )
    {
      /* look for a group starting at i */
      dti_i = HaArray(dsv->tasks_and_intervals, i);
      KheInitDomainAndLeaderTask(dti_i->task, &domain, &leader_task);
      for( j = i + 1;  j < HaArrayCount(dsv->tasks_and_intervals);  j++ )
      {
	dti_j = HaArray(dsv->tasks_and_intervals, j);
	if( dti_j->interval.first != dti_i->interval.last + 1 ||
	    !KheUpdateDomainAndLeaderTask(task, &domain, &leader_task) )
	  break;
      }

      /* now we have a group starting at i and finishing at j-1 */
      KheDpgSolverMakeAssignedGroup(dsv, r, i, j - 1);
    }

    /* free the tasks and intervals */
    HaArrayAppend(dsv->dpg_task_and_interval_free_list,
      dsv->tasks_and_intervals, i);
    HaArrayClear(dsv->tasks_and_intervals);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverGroupAssignedTasks(KHE_DPG_SOLVER dsv)                  */
/*                                                                           */
/*  Carry out an initial grouping of assigned tasks.                         */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverGroupAssignedTasks(KHE_DPG_SOLVER dsv)
{
  KHE_RESOURCE r;  int i;
  for( i = 0;  i < KheResourceTypeResourceCount(dsv->resource_type);  i++ )
  {
    r = KheResourceTypeResource(dsv->resource_type, i);
    KheDpgSolverGroupAssignedTasksForResource(dsv, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgSolverDoSolve(KHE_DPG_SOLVER dsv, KHE_DPG_SOLN *best_ds)      */
/*                                                                           */
/*  Carry one the actual solve, assuming everything has been set up.         */
/*  If successful, return true with *best_ds set to the best solution;       */
/*  otherwise return false with *best_ds set to NULL.                        */
/*                                                                           */
/*****************************************************************************/

bool KheDpgSolverDoSolve(KHE_DPG_SOLVER dsv, KHE_DPG_SOLN *best_ds)
{
  KHE_DPG_TIME_GROUP prev_dtg, next_dtg, last_dtg;  int i, j;
  KHE_DPG_SOLN prev_ds;

  /* clear out any previous solutions */
  HaArrayForEach(dsv->time_groups, next_dtg, i)
    KheDpgTimeGroupDeleteSolns(next_dtg, dsv);

  /* do the solve */
  prev_dtg = NULL;
  HaArrayForEach(dsv->time_groups, next_dtg, i)
  {
    if( prev_dtg == NULL )
      KheDpgExtenderExtend(dsv->extender, NULL, next_dtg);
    else
      HaArrayForEach(prev_dtg->solns, prev_ds, j)
	KheDpgExtenderExtend(dsv->extender, prev_ds, next_dtg);
    HaArraySort(next_dtg->solns, &KheDpgSolnCmp);
    if( DEBUG2 )
      KheDpgTimeGroupDebugSolns(next_dtg, 2, 2, stderr);
    while( HaArrayCount(next_dtg->solns) > MAX_KEEP )
    {
      prev_ds = HaArrayLastAndDelete(next_dtg->solns);
      KheDpgSolnFree(prev_ds, dsv);
    }
    prev_dtg = next_dtg;
  }

  /* if there is a best solution, build its groups */
  last_dtg = HaArrayLast(dsv->time_groups);
  HnAssert(HaArrayCount(last_dtg->solns) <= 1,
    "KheDpgSolverDoSolve internal error");
  if( HaArrayCount(last_dtg->solns) == 1 )
  {
    *best_ds = HaArrayFirst(last_dtg->solns);
    if( DEBUG10 )
    {
      fprintf(stderr, "  KheDpgSolverDoSolve(%s, %s) best_ds:\n",
	KheInstanceId(KheSolnInstance(dsv->soln)),
	KheResourceTypeId(dsv->resource_type));
      KheDpgSolnDebugTimetable(*best_ds, dsv, 2, 2, stderr);
    }
    return true;
  }
  else
    return *best_ds = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindBestMTask(KHE_DPG_TIME_GROUP dtg,                            */
/*    KHE_RESOURCE_GROUP domain, int *index)                                 */
/*                                                                           */
/*  If there is a way to increase our options at dtg for tasks with the      */
/*  given domain, return the index of the mtask with the best options.       */
/*                                                                           */
/*****************************************************************************/

static bool KheFindBestMTask(KHE_DPG_TIME_GROUP dtg,
  KHE_RESOURCE_GROUP domain, bool length_one_only, int *index)
{
  KHE_MTASK mt;  int i, best_index, rg_count, best_rg_count;
  KHE_RESOURCE_GROUP rg;
  if( dtg == NULL )
    return *index = -1, false;
  best_index = -1;
  best_rg_count = INT_MAX;
  HaArrayForEach(dtg->starting_mtasks, mt, i)
  {
    rg = KheMTaskDomain(mt);
    if( (!length_one_only || KheMTaskTotalDuration(mt) == 1) &&
	KheResourceGroupSubset(domain, rg) &&
	HaArray(dtg->starting_counts, i) < KheMTaskTaskCount(mt) )
    {
      rg_count = KheResourceGroupResourceCount(rg);
      if( rg_count < best_rg_count )
      {
	best_index = i;
	best_rg_count = rg_count;
      }
    }
  }
  if( DEBUG8 && best_index != -1 )
  {
    mt = HaArray(dtg->starting_mtasks, best_index);
    fprintf(stderr, "  KheFindBestMTask(%s, %s, %s, -) returning %s (%s)\n",
      KheTimeGroupId(dtg->time_group), KheResourceGroupId(domain),
      bool_show(length_one_only), KheMTaskId(mt),
      KheResourceGroupId(KheMTaskDomain(mt)));
  }
  return *index = best_index, (best_index != -1);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAddOptionalTasks(KHE_DPG_SOLVER dsv, KHE_DPG_SOLN best_ds)       */
/*                                                                           */
/*  Try adding optional tasks based on best_ds, and return true if any       */
/*  were added.  Otherwise return false.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheAddOptionalTasks(KHE_DPG_SOLVER dsv, KHE_DPG_SOLN best_ds)
{
  KHE_DPG_SOLN ds;  int i, j, mtask_index, k;  KHE_DPG_TASK_GROUP dt, dt2;
  KHE_DPG_TIME_GROUP dtg, succ_dtg, pred_dtg;  bool res;

  /* set all dt->finish_here_in_best_ds to true */
  for( ds = best_ds;  ds != NULL;  ds = ds->prev_ds )
    HaArrayForEach(ds->task_groups, dt, j)
      dt->finish_here_in_best_ds = true;

  /* set some dt->finish_here_in_best_ds to false */
  for( ds = best_ds;  ds != NULL;  ds = ds->prev_ds )
    HaArrayForEach(ds->task_groups, dt, j)
      if( dt->prev_task_group != NULL )
	dt->prev_task_group->finish_here_in_best_ds = false;

  /* look for time groups that will suit us */
  res = false;
  ds = best_ds;
  succ_dtg = NULL;
  HaArrayForEachReverse(dsv->time_groups, dtg, i)
  {
    HaArrayForEach(ds->task_groups, dt, j)
    {
      if( dt->finish_here_in_best_ds && !dt->optional &&
	  KheDpgTaskGroupUndersized(dt, dsv) )
      {
	/* undersized, try increasing its length at the end */
	if( KheFindBestMTask(succ_dtg, dt->group_domain, false, &mtask_index) )
	{
	  HaArray(succ_dtg->starting_counts, mtask_index)++;
	  res = true;
	}

	/* undersized, try increasing its length at the start */
	k = i;
	for( dt2 = dt->prev_task_group; dt2 != NULL; dt2 = dt2->prev_task_group)
	  k--;
	if( k > 0 )
	{
	  pred_dtg = HaArray(dsv->time_groups, k - 1);
	  if( KheFindBestMTask(pred_dtg, dt->group_domain, true, &mtask_index) )
	  {
	    HaArray(pred_dtg->starting_counts, mtask_index)++;
	    res = true;
	  }
	}
      }
    }
    succ_dtg = dtg;
    ds = ds->prev_ds;
  }

  /* all done */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)    */
/*                                                                           */
/*  Return true if the time groups of m are all singletons.                  */
/*                                                                           */
/*****************************************************************************/

static bool KheTimeGroupsAllSingletons(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(m);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(m, i, &po);
    if( KheTimeGroupTimeCount(tg) != 1 )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverSolveBegin(KHE_DPG_SOLVER dsv)                          */
/*                                                                           */
/*  Prepare for solving by adding time groups and mtasks to dsv.             */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverSolveBegin(KHE_DPG_SOLVER dsv)
{
  int i;  KHE_TIME_GROUP tg;  KHE_DPG_TIME_GROUP dtg;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  KHE_POLARITY po;

  /* add the time groups to dsv */
  laim = HaArray(dsv->monitors, dsv->first_monitor_index);
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
    KheDpgSolverAddTimeGroup(dsv, tg);
  }

  /* add the mtasks to the time groups (all time groups must be present) */
  HaArrayForEach(dsv->time_groups, dtg, i)
    KheDpgTimeGroupAddMTasks(dtg, i, dsv);
  if( false && DEBUG1 )
    KheDpgSolverDebug(dsv, 2, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgSolverSolveEnd(KHE_DPG_SOLVER dsv)                            */
/*                                                                           */
/*  End solving by deleting the time groups and clearing the cost cache.     */
/*                                                                           */
/*****************************************************************************/

static void KheDpgSolverSolveEnd(KHE_DPG_SOLVER dsv)
{
  int i;  KHE_DPG_TIME_GROUP dtg;

  /* delete the time groups */
  HaArrayForEach(dsv->time_groups, dtg, i)
    KheDpgTimeGroupFree(dtg, dsv);
  HaArrayClear(dsv->time_groups);

  /* clear the cost cache */
  KheDpgCostCacheClear(dsv->cost_cache);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheIntervalGroupingForMonitorRange(KHE_DPG_SOLVER dsv,              */
/*    int first_index, int last_index)                                       */
/*                                                                           */
/*  Carry out interval grouping for monitors[first_index .. last_index].     */
/*                                                                           */
/*****************************************************************************/

static void KheIntervalGroupingForMonitorRange(KHE_DPG_SOLVER dsv,
  int first_index, int last_index)
{
  KHE_DPG_SOLN best_ds, best_ds2;  KHE_TIMER timer;  char buff[20];
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim;  char *laim_id;  int i;

  if( DEBUG8 )
  {
    laim = HaArray(dsv->monitors, first_index);
    laim_id = KheMonitorId((KHE_MONITOR) laim);
    fprintf(stderr, "[ KheIntervalGroupingForMonitorRange(dsv, %s, %d-%d)\n",
      laim_id, first_index, last_index);
    timer = KheTimerMake(laim_id, KHE_NO_TIME, dsv->arena);
  }

  /* fields constant when solving one set of equivalent monitors */
  HnAssert(first_index <= last_index,
    "KheIntervalGroupingForMonitorRange internal error");
  dsv->first_monitor_index = first_index;
  dsv->last_monitor_index = last_index;
  dsv->min_limit = 1;
  dsv->max_limit = INT_MAX;
  dsv->history_before = false;
  dsv->history_after = false;
  for( i = first_index;  i <= last_index;  i++ )
  {
    laim = HaArray(dsv->monitors, i);
    if( KheLimitActiveIntervalsMonitorMinimum(laim) > dsv->min_limit )
      dsv->min_limit = KheLimitActiveIntervalsMonitorMinimum(laim);
    if( KheLimitActiveIntervalsMonitorMaximum(laim) < dsv->max_limit )
      dsv->max_limit = KheLimitActiveIntervalsMonitorMaximum(laim);
    if( KheLimitActiveIntervalsMonitorHistoryBefore(laim) > 0 )
      dsv->history_before = true;
    if( KheLimitActiveIntervalsMonitorHistoryAfter(laim) > 0 )
      dsv->history_after = true;
  }

  /* no use if min_limit is 1 */
  if( dsv->min_limit <= 1 )
  {
    if( DEBUG8 )
      fprintf(stderr, "] KheIntervalGroupingForMonitorRange returning early"
	" (dsv->min_limit == %d)\n", dsv->min_limit);
    return;
  }

  /* do it if all time groups are singletons, or limits are equal */
  laim = HaArray(dsv->monitors, first_index);
  if( KheTimeGroupsAllSingletons(laim) && dsv->min_limit >= dsv->max_limit - 1 )
  {
    /* temporary test */
    /* ***
    if( DEBUG12 && dsv->min_limit == 4 && dsv->max_limit == 5 )
    {
      fprintf(stderr, "  increasing %s min_limit from 4 to 5\n",
	KheMonitorId((KHE_MONITOR) laim));
      dsv->min_limit = 5;
    }
    *** */

    /* group assigned tasks that a relevant to laim, at the start */
    KheDpgSolverGroupAssignedTasks(dsv);

    /* add the time groups and their mtasks to dsv */
    KheDpgSolverSolveBegin(dsv);

    /* main algorithm: build solutions for each time group in turn */
    if( !KheDpgSolverDoSolve(dsv, &best_ds) )
      dsv->groups_count += 0;
    else if( KheAddOptionalTasks(dsv, best_ds) &&
	KheDpgSolverDoSolve(dsv, &best_ds2) )
      dsv->groups_count += KheDpgSolnBuildGroups(best_ds2, dsv, laim, 2);
    else
      dsv->groups_count += KheDpgSolnBuildGroups(best_ds2, dsv, laim, 1);

    /* delete the time groups and mtasks, and clear the cost cache */
    if( DEBUG5 )
      KheDpgCostCacheDebug(dsv->cost_cache, 2, 2, stderr);
    KheDpgSolverSolveEnd(dsv);
  }

  if( DEBUG8 )
    fprintf(stderr, "] KheIntervalGroupingForMonitorRange returning "
      "(groups_count = %d, %s)\n", dsv->groups_count,
      KheTimeShow(KheTimerElapsedTime(timer), buff));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveTypedCmp(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)                              */
/*                                                                           */
/*  Typed comparison function for sorting an array of limit active           */
/*  intervals monitors so that monitors with the same time groups            */
/*  are brought together, and monitors whose time groups have fewer          */
/*  times come before monitors whose time groups have more times.  This      */
/*  function assumes (as it may) that all time groups are positive.          */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveTypedCmp(KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim2)
{
  KHE_TIME_GROUP tg1, tg2;  KHE_POLARITY po1, po2;
  int i, cmp, time_count1, time_count2, tg_count1, tg_count2;

  /* if first time groups differ in size, smallest comes first */
  tg_count1 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim1);
  tg_count2 = KheLimitActiveIntervalsMonitorTimeGroupCount(laim2);
  if( tg_count1 > 0 && tg_count2 > 0 )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, 0, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, 0, &po2);
    time_count1 = KheTimeGroupTimeCount(tg1);
    time_count2 = KheTimeGroupTimeCount(tg2);
    if( time_count1 != time_count2 )
      return time_count1 - time_count2;
  }

  /* time groups must be equal; polarities are known to be all positive */
  if( tg_count1 != tg_count2 )
    return tg_count1 - tg_count2;
  for( i = 0;  i < tg_count1;  i++ )
  {
    tg1 = KheLimitActiveIntervalsMonitorTimeGroup(laim1, i, &po1);
    tg2 = KheLimitActiveIntervalsMonitorTimeGroup(laim2, i, &po2);
    cmp = KheTimeGroupTypedCmp(tg1, tg2);
    if( cmp != 0 ) return cmp;
  }

  /* *** wrong !!!
  ** min limits must be equal **
  lim1 = KheLimitActiveIntervalsMonitorMinimum(laim1);
  lim2 = KheLimitActiveIntervalsMonitorMinimum(laim2);
  cmp = lim1 - lim2;
  if( cmp != 0 )
    return cmp;

  ** max limits must be equal **
  lim1 = KheLimitActiveIntervalsMonitorMaximum(laim1);
  lim2 = KheLimitActiveIntervalsMonitorMaximum(laim2);
  cmp = lim1 - lim2;
  if( cmp != 0 )
    return cmp;
  *** */

  /* everything that matters here is equal */
  return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitActiveCmp(const void *t1, const void *t2)                    */
/*                                                                           */
/*  Untyped comparison function for sorting an array of limit active         */
/*  intervals monitors so that monitors with the same time groups            */
/*  are brought together.  This function assumes that all time groups        */
/*  are positive.                                                            */
/*                                                                           */
/*****************************************************************************/

static int KheLimitActiveCmp(const void *t1, const void *t2)
{
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim1, laim2;
  laim1 = * (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR *) t1;
  laim2 = * (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR *) t2;
  return KheLimitActiveTypedCmp(laim1, laim2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorSuitsIntervalGrouping(                                    */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_RESOURCE_TYPE rt)            */
/*                                                                           */
/*  Return true if m suits interval grouping:  if it has at least two time   */
/*  groups, its constraint applies to all resources of type rt, and its      */
/*  time groups are all positive.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorSuitsIntervalGrouping(
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR m, KHE_RESOURCE_TYPE rt)
{
  int rt_count;  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c;

  /* m must have at least two time groups */
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(m) < 2 )
    return false;

  /* m must monitor all resources of type rt */
  rt_count = KheResourceTypeResourceCount(rt);
  c = KheLimitActiveIntervalsMonitorConstraint(m);
  if( KheLimitActiveIntervalsConstraintResourceOfTypeCount(c, rt) < rt_count )
    return false;

  /* m's time groups must be all positive */
  if( !KheLimitActiveIntervalsConstraintAllPositive(c) )
    return false;

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheintervalGrouping(KHE_MTASK_FINDER mtf, KHE_RESOURCE_TYPE rt,      */
/*    KHE_SOLN_ADJUSTER sa)                                                  */
/*                                                                           */
/*  Carry out interval grouping for rt.  Return the number of groups made.   */
/*                                                                           */
/*****************************************************************************/

int KheintervalGrouping(KHE_MTASK_FINDER mtf, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa)
{
  KHE_MONITOR m;  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laimi, laimj;  int i, j;
  KHE_RESOURCE r;  KHE_DPG_SOLVER dsv;  HA_ARENA a;  KHE_SOLN soln;

  if( DEBUG1 )
    fprintf(stderr, "[ KheIntervalGrouping(mtf, %s, sa)\n",
      KheResourceTypeId(rt));

  /* find suitable limit active intervals monitors */
  if( KheResourceTypeResourceCount(rt) == 0 )
    fprintf(stderr, "] KheIntervalGrouping returning early (no resources)\n");
  soln = KheMTaskFinderSoln(mtf);
  a = KheSolnArenaBegin(soln);
  dsv = KheDpgSolverMake(soln, rt, mtf, sa, a);
  r = KheResourceTypeResource(rt, 0);
  for( i = 0;  i < KheSolnResourceMonitorCount(soln, r);  i++ )
  {
    m = KheSolnResourceMonitor(soln, r, i);
    if( KheMonitorTag(m) == KHE_LIMIT_ACTIVE_INTERVALS_MONITOR_TAG )
    {
      laimi = (KHE_LIMIT_ACTIVE_INTERVALS_MONITOR) m;
      if( KheMonitorSuitsIntervalGrouping(laimi, rt) )
      {
	if( DEBUG7 )
	  fprintf(stderr, "  found suitable monitor %s\n", KheMonitorId(m));
	HaArrayAddLast(dsv->monitors, laimi);
      }
    }
  }
  HaArraySort(dsv->monitors, &KheLimitActiveCmp);

  /* find ranges of suitable monitors and do the job for each range */
  for( i = 0;  i < HaArrayCount(dsv->monitors);  i = j )
  {
    laimi = HaArray(dsv->monitors, i);
    for( j = i + 1;  j < HaArrayCount(dsv->monitors);  j++ )
    {
      laimj = HaArray(dsv->monitors, j);
      if( KheLimitActiveTypedCmp(laimi, laimj) != 0 )
	break;
    }
    KheIntervalGroupingForMonitorRange(dsv, i, j - 1);
  }

  /* return the number of groups made */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheIntervalGrouping returning %d\n", dsv->groups_count);
  return dsv->groups_count;
}

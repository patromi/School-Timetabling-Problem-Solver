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
/*  DESCRIPTION:  Profile grouping using dynamic programming                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "khe_sr_tgrc.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define MAX_KEEP 100

#define DEBUG1 1
#define DEBUG2 1
#define DEBUG3 0

/*****************************************************************************/
/*                                                                           */
/*  Forward typedefs                                                         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_time_group_rec *KHE_DPG_TIME_GROUP;
typedef HA_ARRAY(KHE_DPG_TIME_GROUP) ARRAY_KHE_DPG_TIME_GROUP;

typedef struct khe_dpg_task_rec *KHE_DPG_TASK;
typedef HA_ARRAY(KHE_DPG_TASK) ARRAY_KHE_DPG_TASK;

typedef struct khe_dpg_soln_rec *KHE_DPG_SOLN;
typedef HA_ARRAY(KHE_DPG_SOLN) ARRAY_KHE_DPG_SOLN;

typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;

typedef struct khe_dpg_solver_rec *KHE_DPG_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIME_GROUP - one time group from the monitor being profiled */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_time_group_rec {
  KHE_DPG_SOLVER	solver;
  int			index;			/* index in solver           */
  KHE_TIME_GROUP	time_group;		/* the time group            */
  ARRAY_KHE_MTASK	starting_mtasks;	/* mtasks starting here      */
  int			total_solns;		/* solns (dominated + undom) */
  ARRAY_KHE_DPG_SOLN	solns;			/* solns (undominated only)  */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TASK - one task lying in a solution                         */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_task_rec {
  KHE_TASK		task;			/* the proper root task      */
  int			group_length;		/* its group length          */
  int			extension;		/* its extension (future)    */
  KHE_RESOURCE_GROUP	group_domain;		/* its group's domain        */
  KHE_DPG_TASK		prev_dpg_task;		/* previous task if any      */
};


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_SOLN - one solution (a set of tasks)                        */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_soln_rec {
  ARRAY_KHE_DPG_TASK	dpg_tasks;		/* the tasks of this soln    */
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
  int				undersized_groups;
  int				available_tasks;
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
} *KHE_DPG_COST_CACHE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_DPG_TIMETABLE_ENTRY - one entry of a displayed timetable        */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_dpg_timetable_entry_rec {
  KHE_DPG_TASK			dpg_task;	/* the task                  */
  KHE_TASK			task;		/* the exact KHE task        */
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
  ARRAY_KHE_DPG_TIMETABLE_ROW	rows;		/* rows of the timetable     */
} *KHE_DPG_TIMETABLE;

typedef HA_ARRAY(KHE_DPG_TIMETABLE) ARRAY_KHE_DPG_TIMETABLE;


/*****************************************************************************/
/*                                                                           */
/*  type KHE_DPG_SOLVER - solver for profile grouping by dynamic programming */
/*                                                                           */
/*****************************************************************************/

struct khe_dpg_solver_rec {

  /* free lists */
  ARRAY_KHE_DPG_TIME_GROUP		dpg_time_group_free_list;
  ARRAY_KHE_DPG_TASK			dpg_task_free_list;
  ARRAY_KHE_DPG_SOLN			dpg_soln_free_list;
  ARRAY_KHE_DPG_COST_CACHE_SUB		dpg_cost_cache_sub_free_list;
  ARRAY_KHE_DPG_TIMETABLE_ENTRY		dpg_timetable_entry_free_list;
  ARRAY_KHE_DPG_TIMETABLE_ROW		dpg_timetable_row_free_list;
  ARRAY_KHE_DPG_TIMETABLE		dpg_timetable_free_list;

  /* other fields */
  HA_ARENA				arena;
  KHE_COMB_GROUPER			comb_grouper;
  KHE_MTASK_FINDER			mtask_finder;
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR	monitor;
  KHE_RESOURCE_TYPE			resource_type;
  int					min_limit;
  int					max_limit;
  int					history_before;
  int					history_after;
  KHE_SOLN_ADJUSTER			soln_adjuster;
  ARRAY_KHE_DPG_TIME_GROUP		dpg_time_groups;
  KHE_DPG_COST_CACHE			cost_cache;
  KHE_DPG_EXTENDER			extender;
};


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
    HaArrayClear(res->solns);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->starting_mtasks, dsv->arena);
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

  /* mt must have no overlaps and no gaps, and must need assignment */
  if( !KheMTaskNoOverlap(mt) )
    return false;
  if( !KheMTaskNoGaps(mt) )
    return false;
  if( !KheMTaskNeedsAssignment(mt) )
    return false;

  /* mt must contain at least one assigned time */
  ts = KheMTaskTimeSet(mt);
  if( KheTimeSetTimeCount(ts) == 0 )
    return false;

  /* check time groups */
  for( i = 0;  i < KheTimeSetTimeCount(ts);  i++ )
  {
    time = KheTimeSetTime(ts, i);
    if( index + i >= HaArrayCount(dsv->dpg_time_groups) )
      return false;
    dtg = HaArray(dsv->dpg_time_groups, index + i);
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
  KHE_MTASK_SET mts;  KHE_MTASK mt;  int i;
  mts = KheMTaskFinderMTasksInTimeGroup(dsv->mtask_finder,
    dsv->resource_type, dtg->time_group);
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( KheDpgTimeGroupAcceptsMTask(dsv, index, mt) )
      HaArrayAddLast(dtg->starting_mtasks, mt);
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
static void KheDpgSolnFree(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv);
static KHE_DPG_SOLN KheDpgSolnCopy(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv);

static void KheDpgTimeGroupAddSoln(KHE_DPG_TIME_GROUP dtg, KHE_DPG_SOLN ds)
{
  KHE_DPG_SOLN ds2;  int i;  bool last_dtg;

  /* if ds is dominated, free ds and return without changing anything else */
  dtg->total_solns++;
  last_dtg = (dtg->index == HaArrayCount(dtg->solver->dpg_time_groups) - 1);
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
  KHE_MTASK mt;  int i;
  fprintf(fp, "%*s[ DpgTimeGroup(%s)\n", indent, "",
    KheTimeGroupId(dtg->time_group));
  HaArrayForEach(dtg->starting_mtasks, mt, i)
    KheMTaskDebug(mt, verbosity, indent + 2, fp);
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
/* ***
static void KheDpgSolnDebug(KHE_DPG_SOLN ds, int verbosity, int indent,
  FILE *fp);
*** */
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
	KheDpgSolnDebugTimetable(ds, dtg->solver, verbosity, indent + 2, stderr);
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
/*  Submodule "KHE_DPG_TASK"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TASK KheDpgTaskMake(KHE_TASK task, int group_length,             */
/*    int extension, KHE_RESOURCE_GROUP group_domain,                        */
/*    KHE_DPG_TASK prev_dpg_task)                                            */
/*                                                                           */
/*  Make a dpg task with these attributes.                                   */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TASK KheDpgTaskMake(KHE_TASK task, int group_length,
  int extension, KHE_RESOURCE_GROUP group_domain,
  KHE_DPG_TASK prev_dpg_task, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK res;
  if( HaArrayCount(dsv->dpg_task_free_list) > 0 )
    res = HaArrayLastAndDelete(dsv->dpg_task_free_list);
  else
    HaMake(res, dsv->arena);
  res->task = task;
  res->group_length = group_length;
  res->extension = extension;
  res->group_domain = group_domain;
  res->prev_dpg_task = prev_dpg_task;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_DPG_TASK KheDpgTaskCopy(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)         */
/*                                                                           */
/*  Return a copy of dt.                                                     */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_TASK KheDpgTaskCopy(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)
{
  return KheDpgTaskMake(dt->task, dt->group_length, dt->extension,
    dt->group_domain, dt->prev_dpg_task, dsv);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTaskFree(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)                 */
/*                                                                           */
/*  Free dt, by adding it to the free list in dsv.                           */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTaskFree(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)
{
  HaArrayAddLast(dsv->dpg_task_free_list, dt);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgTaskExtendable(KHE_DPG_TASK dt, KHE_TASK task,                */
/*    int task_total_durn, KHE_DPG_SOLVER dsv, int *group_length,            */
/*    int *extension, KHE_RESOURCE_GROUP *group_domain)                      */
/*                                                                           */
/*  If dt can be extended by adding task (with total duration total_durn),   */
/*  return true with *group_length, *extension, and *group_domain set to     */
/*  the resulting values.  Otherwise return false.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTaskExtendable(KHE_DPG_TASK dt, KHE_TASK task,
  int task_total_durn, KHE_DPG_SOLVER dsv, int *group_length,
  int *extension, KHE_RESOURCE_GROUP *group_domain)
{
  KHE_RESOURCE_GROUP domain;
  HnAssert(dt->extension == 0, "KheDpgTaskExtendable internal error");
  /* ***
  if( dt->extension > 0 )
  {
    if( dt->task == task )
    {
      ** this will work because we just keep using the same task **
      return *group_length = dt->group_length, *extension = dt->extension - 1,
	*group_domain = dt->group_domain, true;
    }
    else
    {
      ** this won't work because task would clash with dt->task **
      return *group_length = 0, *extension = 0, *group_domain = NULL, false;
    }
  }
  else
  {
  *** */

  /* return false if adding task makes the group length too long */
  if( dt->group_length + task_total_durn > dsv->max_limit )
    return *group_length = 0, *extension = 0, *group_domain = NULL, false;

  /* return false if there is no easily calculated suitable domain */
  domain = KheTaskDomain(task);
  if( KheResourceGroupSubset(dt->group_domain, domain) )
    return *group_length = dt->group_length + task_total_durn,
      *extension = task_total_durn - 1, *group_domain = dt->group_domain, true;
  else if( KheResourceGroupSubset(domain, dt->group_domain) )
    return *group_length = dt->group_length + task_total_durn,
      *extension = task_total_durn - 1, *group_domain = domain, true;
  else
    return *group_length = 0, *extension = 0, *group_domain = NULL, false;

  /* ***
  }
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgTaskTypedCmp(KHE_DPG_TASK dt1, KHE_DPG_TASK dt2)               */
/*                                                                           */
/*  Typed comparison function for sorting an array of dpg tasks.             */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskTypedCmp(KHE_DPG_TASK dt1, KHE_DPG_TASK dt2)
{
  int cmp, domain_count1, domain_count2;

  /* sort by increasing extension */
  cmp = dt1->extension - dt2->extension;
  if( cmp != 0 )  return cmp;

  /* sort by increasing group length */
  cmp = dt1->group_length - dt2->group_length;
  if( cmp != 0 )  return cmp;

  /* sort by domain size */
  domain_count1 = KheResourceGroupResourceCount(KheTaskDomain(dt1->task));
  domain_count2 = KheResourceGroupResourceCount(KheTaskDomain(dt2->task));
  return domain_count1 - domain_count2;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDgpTaskCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of dpg tasks.           */
/*                                                                           */
/*****************************************************************************/

static int KheDgpTaskCmp(const void *t1, const void *t2)
{
  KHE_DPG_TASK dt1 = * (KHE_DPG_TASK *) t1;
  KHE_DPG_TASK dt2 = * (KHE_DPG_TASK *) t2;
  return KheDpgTaskTypedCmp(dt1, dt2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTaskDebug(KHE_DPG_TASK dt, int verbosity, int indent,         */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of dt onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

/* *** good, but currently unused
static void KheDpgTaskDebug(KHE_DPG_TASK dt, int verbosity, int indent,
  FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "<%s %d:%d %s", KheTaskId(dt->task), dt->group_length,
    dt->extension, KheResourceGroupId(dt->group_domain));
  if( verbosity >= 3 && dt->prev_dpg_task != NULL )
  {
    fprintf(fp, " ");
    KheDpgTaskDebug(dt->prev_dpg_task, verbosity, -1, fp);
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
  KHE_DPG_TASK dt;  int i;
  HaArrayForEach(ds->dpg_tasks, dt, i)
    KheDpgTaskFree(dt, dsv);
  HaArrayClear(ds->dpg_tasks);
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
    HaArrayClear(res->dpg_tasks);
  }
  else
  {
    HaMake(res, dsv->arena);
    HaArrayInit(res->dpg_tasks, dsv->arena);
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
  KHE_DPG_SOLN res;  KHE_DPG_TASK dt;  int i;
  res = KheDpgSolnMake(ds->prev_ds, dsv);
  HaArrayForEach(ds->dpg_tasks, dt, i)
    HaArrayAddLast(res->dpg_tasks, KheDpgTaskCopy(dt, dsv));
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
  int res, i;  KHE_DPG_TASK dt;
  res = 0;
  HaArrayForEach(ds->dpg_tasks, dt, i)
    if( KheDpgTaskMustExtend(dt, dsv) )
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
  KHE_DPG_TASK dt;  int i;
  HaArrayForEach(ds->dpg_tasks, dt, i)
    KheDpgTaskFree(dt, dsv);
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
  KHE_DPG_TASK dt1, dt2;  int i;
  HnAssert(HaArrayCount(ds1->dpg_tasks) == HaArrayCount(ds2->dpg_tasks),
    "KheDpgSolnDominates internal error 1");

  /* for dominance, the cost of ds1 must not exceed the cost of ds2 */
  if( ds1->cost > ds2->cost )
    return false;

  /* for dominance, corresponding group lengths and extensions must be */
  /*  equal, and domains must be supersets; unless last_dtg */
  if( !last_dtg )
    for( i = 0;  i < HaArrayCount(ds1->dpg_tasks);  i++ )
    {
      dt1 = HaArray(ds1->dpg_tasks, i);
      dt2 = HaArray(ds2->dpg_tasks, i);
      if( dt1->group_length != dt2->group_length )
	return false;
      if( dt1->extension != dt2->extension )
	return false;
      if( !KheResourceGroupSubset(dt2->group_domain, dt1->group_domain) )
	return false;
    }

  /* all good, ds1 dominates ds2 */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheFindLeaderTask(KHE_DPG_TASK dt, KHE_RESOURCE_GROUP domain)   */
/*                                                                           */
/*  Find the leader task of the tasks linked to dt.  It is any task with     */
/*  the given domain.                                                        */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheFindLeaderTask(KHE_DPG_TASK dt, KHE_RESOURCE_GROUP domain)
{
  for( ;  dt != NULL;  dt = dt->prev_dpg_task )
    if( KheTaskDomain(dt->task) == domain )
      return dt->task;
  HnAbort("KheFindLeaderTask internal error");
  return NULL;  /* keep compiler happy */
}


/*****************************************************************************/
/*                                                                           */
/*  void KheClearGroup(KHE_DPG_TASK dt)                                      */
/*                                                                           */
/*  Clear out the group starting at dt.                                      */
/*                                                                           */
/*****************************************************************************/

/* *** merged into KheBuildGroup now
static void KheClearGroup(KHE_DPG_TASK dt)
{
  KHE_DPG_TASK prev_dt;
  prev_dt = dt->prev_dpg_task;
  while( prev_dt != NULL )
  {
    dt->prev_dpg_task = NULL;
    dt = prev_dt;
    prev_dt = dt->prev_dpg_task;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheBuildAndClearGroup(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)          */
/*                                                                           */
/*  Build one group starting at dt.  Also clear it out.                      */
/*                                                                           */
/*****************************************************************************/

static void KheBuildAndClearGroup(KHE_DPG_TASK dt, KHE_DPG_SOLVER dsv)
{
  KHE_TASK leader_task;  KHE_DPG_TASK prev_dt;
  leader_task = KheFindLeaderTask(dt, dt->group_domain);
  KheMTaskFinderGroupBegin(dsv->mtask_finder, leader_task);
  prev_dt = NULL;
  while( dt != NULL )
  {
    if( dt->task != leader_task && dt->extension == 0 )
      KheMTaskFinderGroupAddTask(dsv->mtask_finder, dt->task);
    prev_dt = dt;
    dt = dt->prev_dpg_task;
    prev_dt->prev_dpg_task = NULL;
  }
  KheMTaskFinderGroupEnd(dsv->mtask_finder, dsv->soln_adjuster);
  /* KheClearGroup(dt); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheDpgSolnBuildGroups(KHE_DPG_SOLN ds, KHE_MTASK_FINDER mtf)         */
/*                                                                           */
/*  Build the groups defined by ds.                                          */
/*                                                                           */
/*****************************************************************************/

static int KheDpgSolnBuildGroups(KHE_DPG_SOLN ds, KHE_DPG_SOLVER dsv)
{
  KHE_DPG_TASK dt;  int i, res;
  res = 0;
  for( ;  ds != NULL;  ds = ds->prev_ds )
  {
    HaArrayForEach(ds->dpg_tasks, dt, i)
      if( dt->prev_dpg_task != NULL )
      {
        KheBuildAndClearGroup(dt, dsv);
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
  KHE_DPG_TASK dt;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ Soln(%.5f)", KheCostShow(ds->cost));
  HaArrayForEach(ds->dpg_tasks, dt, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    KheDpgTaskDebug(dt, verbosity, -1, fp);
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

static int KheUndersizedGroups(KHE_DPG_SOLN ds, int min_limit)
{
  KHE_DPG_TASK dt;  int i, res;
  res = 0;
  if( ds != NULL )
    HaArrayForEach(ds->dpg_tasks, dt, i)
      if( dt->extension == 0 && dt->group_length < min_limit )
	res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAvailableTasks(KHE_DPG_TIME_GROUP dtg)                            */
/*                                                                           */
/*  Return the number of admissible tasks starting in dtg, and hence         */
/*  available for assignment.                                                */
/*                                                                           */
/*****************************************************************************/

static int KheAvailableTasks(KHE_DPG_TIME_GROUP dtg)
{
  int res, i;  KHE_MTASK mt;
  res = 0;
  HaArrayForEach(dtg->starting_mtasks, mt, i)
    res += KheMTaskNeedsAssignmentTaskCount(mt);
  return res;
}


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
  de->undersized_groups = KheUndersizedGroups(prev_ds, de->solver->min_limit);
  de->available_tasks = (next_dtg != NULL ? KheAvailableTasks(next_dtg) : 0);
  HaArrayClear(de->task_used);
  count = (prev_ds != NULL ? HaArrayCount(prev_ds->dpg_tasks) : 0);
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
/*  void KheDpgExtenderTryStartingTasksB(KHE_DPG_EXTENDER de,                */
/*    int mtask_index, int task_index, int prev_i)                           */
/*                                                                           */
/*  Try all ways to add one starting task to de->next_ds and recurse.        */
/*                                                                           */
/*****************************************************************************/
static void KheDpgExtenderTryStartingTasksA(KHE_DPG_EXTENDER de,
  int mtask_index);

static void KheDpgExtenderTryStartingTasksB(KHE_DPG_EXTENDER de,
  int mtask_index, int task_index, int prev_i)
{
  KHE_MTASK mt;  KHE_TASK task;  KHE_COST non_asst_cost, asst_cost;
  KHE_DPG_TASK prev_dt, next_dt;  KHE_RESOURCE_GROUP group_domain;
  int i, group_length, extension, task_total_durn;
  bool undersized, task_utilized;

  mt = HaArray(de->next_dtg->starting_mtasks, mtask_index);
  if( task_index >= KheMTaskNeedsAssignmentTaskCount(mt) )
  {
    /* finished with this mtask, go on to next mtask */
    KheDpgExtenderTryStartingTasksA(de, mtask_index + 1);
  }
  else
  {
    /* try all groupings of task with a task from de->prev_ds */
    task = KheMTaskNeedsAssignmentTask(mt, task_index,
      &non_asst_cost, &asst_cost);
    task_total_durn = KheMTaskTotalDuration(mt);
    i = prev_i + 1;
    task_utilized = false;
    if( de->prev_ds != NULL )
      for( ;  i < HaArrayCount(de->prev_ds->dpg_tasks);  i++ )
      {
	if( !HaArray(de->task_used, i) )
	{
	  prev_dt = HaArray(de->prev_ds->dpg_tasks, i);
	  if( prev_dt->extension > 0 )  break;
	  if( KheDpgTaskExtendable(prev_dt, task, task_total_durn,
		de->solver, &group_length, &extension, &group_domain) )
	  {
	    next_dt = KheDpgTaskMake(task, group_length, extension,
	      group_domain, prev_dt, de->solver);
	    HaArrayAddLast(de->next_ds->dpg_tasks, next_dt);
	    HaArrayPut(de->task_used, i, true);
	    undersized = (prev_dt->group_length < de->solver->min_limit);
	    if( undersized )
	      de->undersized_groups--;
	    de->available_tasks--;
	    KheDpgExtenderTryStartingTasksB(de, mtask_index,
	      task_index + 1, i);
	    task_utilized = true;
	    de->available_tasks++;
	    if( undersized )
	      de->undersized_groups++;
	    HaArrayPut(de->task_used, i, false);
	    HaArrayDeleteLast(de->next_ds->dpg_tasks);
	    KheDpgTaskFree(next_dt, de->solver);
	  }
	}
      }

    /* try a fresh start for task, if permissible */
    if( !task_utilized || de->undersized_groups < de->available_tasks )
    {
      next_dt = KheDpgTaskMake(task, task_total_durn, task_total_durn - 1,
	KheTaskDomain(task), NULL, de->solver);
      HaArrayAddLast(de->next_ds->dpg_tasks, next_dt);
      de->available_tasks--;
      KheDpgExtenderTryStartingTasksB(de, mtask_index, task_index + 1, i);
      de->available_tasks++;
      HaArrayDeleteLast(de->next_ds->dpg_tasks);
      KheDpgTaskFree(next_dt, de->solver);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgExtenderFinishedCostAdd(KHE_DPG_SOLN ds, KHE_DPG_EXTENDER de) */
/*                                                                           */
/*  Finish off all unused tasks.  This amounts to adding their costs         */
/*  to the cost of ds.  If this is successful, return true.  Otherwise       */
/*  free ds and return false.                                                */
/*                                                                           */
/*****************************************************************************/
static bool KheDpgSolverSequenceCost(KHE_DPG_SOLVER dsv,
  int first_index, int last_index, KHE_COST *cost);

static bool KheDpgExtenderFinishedCostAdd(KHE_DPG_SOLN ds, KHE_DPG_EXTENDER de)
{
  int i, li, fi;  KHE_DPG_TASK prev_dt;
  KHE_COST cost;  KHE_DPG_TASK dt;

  if( de->prev_ds != NULL )
    for( i = 0;  i < HaArrayCount(de->prev_ds->dpg_tasks);  i++ )
    {
      if( !HaArray(de->task_used, i) )
      {
	prev_dt = HaArray(de->prev_ds->dpg_tasks, i);
	HnAssert(prev_dt->extension == 0,
	  "KheDpgExtenderFinishedCostAdd internal error");

	/* find the cost of ending a group at prev_dt and add it to ds->cost */
	li = de->next_dtg->index - 1;
	fi = li;
	for( dt = prev_dt->prev_dpg_task; dt != NULL; dt = dt->prev_dpg_task )
	  fi--;
	if( !KheDpgSolverSequenceCost(de->solver, fi, li, &cost) )
	{
	  KheDpgSolnFree(ds, de->solver);
	  return false;
	}
	ds->cost += cost;
      }
    }
  return true;
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
  if( mtask_index >= HaArrayCount(de->next_dtg->starting_mtasks) )
  {
    /* base of recursion; start by copying de->next_ds */
    ds = KheDpgSolnCopy(de->next_ds, de->solver);
    if( KheDpgExtenderFinishedCostAdd(ds, de) )
    {
      /* sort ds's tasks, and add ds in or delete it */
      HaArraySort(ds->dpg_tasks, &KheDgpTaskCmp);
      KheDpgTimeGroupAddSoln(de->next_dtg, ds);
    }
  }
  else
  {
    /* explore assignments to the first task of mt */
    KheDpgExtenderTryStartingTasksB(de, mtask_index, 0, -1);
  }
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
  KHE_DPG_TASK dt, new_dt;  int i, res;
  res = 0;
  if( de->prev_ds != NULL )
    HaArrayForEachReverse(de->prev_ds->dpg_tasks, dt, i)
    {
      if( dt->extension == 0 )
	break;
      new_dt = KheDpgTaskMake(dt->task, dt->group_length, dt->extension - 1,
	dt->group_domain, dt, de->solver);
      HaArrayAddLast(de->next_ds->dpg_tasks, new_dt);
      HaArrayPut(de->task_used, i, true);
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
  int i;  KHE_DPG_TASK dt;
  for( i = 0;  i < num;  i++ )
  {
    dt = HaArrayLastAndDelete(de->next_ds->dpg_tasks);
    KheDpgTaskFree(dt, de->solver);
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
  if( DEBUG3 )
    fprintf(stderr, "  extending (undersized_groups %d, available_tasks %d)\n",
      de->undersized_groups, de->available_tasks);
  continuing_count = KheDpgExtenderAddContinuingTasks(de);
  KheDpgExtenderTryStartingTasksA(de, 0);
  KheDpgExtenderDeleteContinuingTasks(de, continuing_count);
  HnAssert(HaArrayCount(de->next_ds->dpg_tasks) == 0,
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

/* *** currently unused
static void KheDpgCostCacheSubFree(KHE_DPG_COST_CACHE_SUB sub,
  KHE_DPG_SOLVER dsv)
{
  HaArrayAddLast(dsv->dpg_cost_cache_sub_free_list, sub);
}
*** */


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
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgCostCacheClear(KHE_DPG_COST_CACHE dcc)                        */
/*                                                                           */
/*  Clear dcc back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static void KheDpgCostCacheClear(KHE_DPG_COST_CACHE dcc)
{
  KHE_DPG_COST_CACHE_SUB sub;  int i;
  HaArrayForEach(dcc->subs, sub, i)
    KheDpgCostCacheSubFree(sub, dcc->solver);
  HaArrayClear(dcc->subs);
}
*** */


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
  HaArrayFill(dcc->subs, first_index + 1, NULL);
  sub = HaArray(dcc->subs, first_index);
  if( sub == NULL )
  {
    sub = KheDpgCostCacheSubMake(dcc->solver);
    HaArrayPut(dcc->subs, first_index, sub);
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
/*  to that cost and return true.  Otherwise return false.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgCostCacheContainsCost(KHE_DPG_COST_CACHE dcc,
  int first_index, int last_index, KHE_COST *cost)
{
  KHE_DPG_COST_CACHE_SUB sub;
  if( 0 <= first_index && first_index < HaArrayCount(dcc->subs) )
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
  int first_index, last_index;  KHE_DPG_SOLVER dsv;  KHE_COST cost;
  KHE_DPG_COST_CACHE_SUB sub;  KHE_DPG_TIME_GROUP dtg1, dtg2;
  if( indent >= 0 )
  {
    dsv = dcc->solver;
    fprintf(fp, "%*s[ CostCache\n", indent, "");
    HaArrayForEach(dcc->subs, sub, first_index)
      if( sub != NULL )
	HaArrayForEach(sub->costs, cost, last_index)
	  if( cost != -1 )
	  {
	    dtg1 = HaArray(dsv->dpg_time_groups, first_index);
	    dtg2 = HaArray(dsv->dpg_time_groups, last_index);
	    fprintf(fp, "%*s  %s-%s: %.5f\n", indent, "",
	      KheTimeGroupId(dtg1->time_group),
	      KheTimeGroupId(dtg2->time_group), KheCostShow(cost));
	  }
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
/*    KHE_DPG_TASK dt)                                                       */
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
  res->dpg_task = NULL;
  res->task = NULL;
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
/*    KHE_DPG_TASK dpg_task, KHE_TASK task, char durn_char)                  */
/*                                                                           */
/*  Set the attributes of dtte to these values.                              */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableEntrySet(KHE_DPG_TIMETABLE_ENTRY dtte,
  KHE_DPG_TASK dpg_task, KHE_TASK task, char durn_char)
{
  dtte->dpg_task = dpg_task;
  dtte->task = task;
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
    if( dtte->dpg_task != NULL )
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
/*  int KheDpgTaskGroupDuration(KHE_DPG_TASK dt)                             */
/*                                                                           */
/*  Return the duration of dt.                                               */
/*                                                                           */
/*****************************************************************************/

static int KheDpgTaskGroupDuration(KHE_DPG_TASK dt)
{
  int res;
  res = 0;
  while( dt != NULL )
    res++, dt = dt->prev_dpg_task;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char KheDurnChar(KHE_DPG_TIMETABLE dtt, int durn)                        */
/*                                                                           */
/*  Return a single character indicating how dc compares with the            */
/*  expected duration.                                                       */
/*                                                                           */
/*****************************************************************************/

static char KheDurnChar(KHE_DPG_TIMETABLE dtt, int durn)
{
  return durn < dtt->solver->min_limit ? '#' :
    durn > dtt->solver->max_limit ? '$' : ' ';
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
  KHE_MEET meet;  KHE_TASK res, child_task;  KHE_TIME time;  int i, pos;

  /* do it for task */
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    time = KheMeetAsstTime(meet);
    if( time != NULL && KheTimeGroupContains(tg, time, &pos) )
      return task;
  }

  /* do it for task's proper descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    res = KheTaskDuringTimeGroup(child_task, tg);
    if( res != NULL )
      return res;
  }

  /* no luck, return NULL */
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableAddDpgTask(KHE_DPG_TIMETABLE dtt, KHE_DPG_TASK dt,   */
/*    int last_row_index)                                                    */
/*                                                                           */
/*  Add dt to dtt.  Its last row has index last_row_index.                   */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableAddDpgTask(KHE_DPG_TIMETABLE dtt, KHE_DPG_TASK dt,
  int last_row_index)
{
  int first_row_index, col_index, i, durn;  char durn_char;
  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_TIMETABLE_ENTRY dtte;
  KHE_TASK task;  KHE_DPG_TIME_GROUP dtg;

  /* find the duration and durn_char */
  durn = KheDpgTaskGroupDuration(dt);
  durn_char = KheDurnChar(dtt, durn);

  /* find the indexes delimiting where dt will go */
  first_row_index = last_row_index - durn + 1;
  col_index = KheDpgTimetableFindOrMakeFreeColumn(dtt,
    first_row_index, last_row_index);

  /* add dt at those indexes */
  for( i = last_row_index;  i >= first_row_index;  i-- )
  {
    dttr = HaArray(dtt->rows, i);
    dtte = HaArray(dttr->entries, col_index);
    dtg = HaArray(dtt->solver->dpg_time_groups, i);
    task = KheTaskDuringTimeGroup(dt->task, dtg->time_group);
    KheDpgTimetableEntrySet(dtte, dt, task, durn_char);
    dt = dt->prev_dpg_task;
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
/*  bool KheDpgTaskIsPredecessor(KHE_DPG_TASK dt, KHE_DPG_SOLN next_ds)      */
/*                                                                           */
/*  Return true if dt is the predecessor of a task in next_ds, or false      */
/*  if nor or if next_ds is NULL.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgTaskIsPredecessor(KHE_DPG_TASK dt, KHE_DPG_SOLN next_ds)
{
  KHE_DPG_TASK dt2;  int i;
  if( next_ds != NULL )
    HaArrayForEach(next_ds->dpg_tasks, dt2, i)
      if( dt2->prev_dpg_task == dt )
	return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheDpgTimetableAddSoln(KHE_DPG_TIMETABLE dtt, KHE_DPG_SOLN ds,      */
/*    int last_row_index)                                                    */
/*                                                                           */
/*  Add ds to dtt at last_row_index.                                         */
/*                                                                           */
/*****************************************************************************/

static void KheDpgTimetableAddSoln(KHE_DPG_TIMETABLE dtt, KHE_DPG_SOLN ds,
  int last_row_index, KHE_DPG_SOLN next_ds)
{
  KHE_DPG_TASK dt;  int i;
  HaArrayForEach(ds->dpg_tasks, dt, i)
    if( !KheDpgTaskIsPredecessor(dt, next_ds) )
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

  /* add one row for each time group covered by soln */
  tg_count = KheDpgSolnTimeGroupCount(ds);
  for( i = 0;  i < tg_count;  i++ )
  {
    dtg = HaArray(dsv->dpg_time_groups, i);
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
/*  void KheDpgTimetableDebug(KHE_DPG_TIMETABLE dtt, int verbosity,          */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of dtt onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/
#define KHE_COL_WIDTH 14

static void KheDpgTimetableDebug(KHE_DPG_TIMETABLE dtt, int verbosity,
  int indent, FILE *fp)
{
  KHE_DPG_TIMETABLE_ROW dttr;  KHE_DPG_TIMETABLE_ENTRY dtte;  int i, j;
  char spaces[KHE_COL_WIDTH + 1], dashes[KHE_COL_WIDTH + 1];
  char buff[KHE_COL_WIDTH + 1];
  KHE_COST non_asst_cost, asst_cost;  char *id, *id2;

  /* make the spaces and dashes */
  for( i = 0;  i < KHE_COL_WIDTH;  i++ )
  {
    spaces[i] = ' ';
    dashes[i] = '-';
  }
  spaces[i] = '\0';
  dashes[i] = '\0';

  fprintf(fp, "%*s[ Soln(cost %.5f)\n", indent, "",
    KheCostShow(dtt->soln->cost));
  HaArrayForEach(dtt->rows, dttr, i)
  {
    /* print the separator line above the row */
    fprintf(fp, "%*s%s", indent + 2, "", spaces);
    HaArrayForEach(dttr->entries, dtte, j)
      fprintf(fp, "+%s", dtte->dpg_task != NULL &&
	dtte->dpg_task->prev_dpg_task != NULL ? spaces : dashes);
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
    {
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
	KheTaskNonAsstAndAsstCost(dtte->task, &non_asst_cost, &asst_cost);
	if( non_asst_cost >= KheCost(1, 0) )
	{
	  /* print domain and "H" for hard cost */
	  snprintf(buff, KHE_COL_WIDTH, "%s/H%c",
	    KheResourceGroupId(KheTaskDomain(dtte->task)),
	    dtte->durn_char);
	}
	else
	{
	  /* print domain and soft cost */
	  snprintf(buff, KHE_COL_WIDTH, "%s/%d%c",
	    KheResourceGroupId(KheTaskDomain(dtte->task)),
	    KheSoftCost(non_asst_cost),
	    dtte->durn_char);
	}
      }
      fprintf(fp, "|%*s", KHE_COL_WIDTH, buff);
    }
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
/*  KHE_DPG_SOLVER KheDpgSolverMake(KHE_COMB_GROUPER cg,                     */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_RESOURCE_TYPE rt,         */
/*    int min_limit, int max_limit, int history_before, int history_after,   */
/*    KHE_SOLN_ADJUSTER sa, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a dpg solver object with these attributes.                          */
/*                                                                           */
/*****************************************************************************/

static KHE_DPG_SOLVER KheDpgSolverMake(KHE_COMB_GROUPER cg,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, KHE_RESOURCE_TYPE rt,
  int min_limit, int max_limit, int history_before, int history_after,
  KHE_SOLN_ADJUSTER sa, HA_ARENA a)
{
  KHE_DPG_SOLVER res;
  HaMake(res, a);

  /* free lists */
  HaArrayInit(res->dpg_time_group_free_list, a);
  HaArrayInit(res->dpg_task_free_list, a);
  HaArrayInit(res->dpg_soln_free_list, a);
  HaArrayInit(res->dpg_cost_cache_sub_free_list, a);
  HaArrayInit(res->dpg_timetable_entry_free_list, a);
  HaArrayInit(res->dpg_timetable_row_free_list, a);
  HaArrayInit(res->dpg_timetable_free_list, a);

  /* other fields */
  res->arena = a;
  res->comb_grouper = cg;
  res->mtask_finder = KheCombGrouperMTaskFinder(cg);
  res->monitor = laim;
  res->resource_type = rt;
  res->min_limit = min_limit;
  res->max_limit = max_limit;
  res->history_before = history_before;
  res->history_after = history_after;
  res->soln_adjuster = sa;
  HaArrayInit(res->dpg_time_groups, a);
  res->cost_cache = KheDpgCostCacheMake(res);
  res->extender = KheDpgExtenderMake(res, NULL, NULL /* , 0, 0 */);
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
  dtg = KheDpgTimeGroupMake(HaArrayCount(dsv->dpg_time_groups), tg, dsv);
  HaArrayAddLast(dsv->dpg_time_groups, dtg);
}


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
  int dev;
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
  return KheMonitorDevToCost((KHE_MONITOR) dsv->monitor, dev);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgSolverFindSequenceCost(KHE_DPG_SOLVER dsv,                    */
/*    int first_index, int last_index, KHE_COST *cost)                       */
/*                                                                           */
/*  Find the cost of a sequence from first_index to last_index.              */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgSolverFindSequenceCost(KHE_DPG_SOLVER dsv,
  int first_index, int last_index, KHE_COST *cost)
{
  KHE_DPG_TIME_GROUP dtg;  KHE_MTASK_GROUP mg;  bool res, last;  int i;

  /* add requirements */
  KheCombGrouperClearRequirements(dsv->comb_grouper);
  if( first_index - 1 >= 0 )
  {
    dtg = HaArray(dsv->dpg_time_groups, first_index - 1);
    KheCombGrouperAddTimeGroupRequirement(dsv->comb_grouper,
      dtg->time_group, KHE_COMB_COVER_NO);
  }
  for( i = first_index;  i <= last_index;  i++ )
  {
    dtg = HaArray(dsv->dpg_time_groups, i);
    KheCombGrouperAddTimeGroupRequirement(dsv->comb_grouper,
      dtg->time_group, KHE_COMB_COVER_YES);
  }
  if( last_index + 1 < HaArrayCount(dsv->dpg_time_groups) )
  {
    dtg = HaArray(dsv->dpg_time_groups, last_index + 1);
    KheCombGrouperAddTimeGroupRequirement(dsv->comb_grouper,
      dtg->time_group, KHE_COMB_COVER_NO);
    last = false;
  }
  else
    last = true;

  /* solve (but don't group) and get cost */
  mg = KheMTaskGroupMake(dsv->comb_grouper);
  res = KheCombGrouperSolve(dsv->comb_grouper, KHE_COMB_VARIANT_MIN, mg) &&
    KheMTaskGroupHasCost(mg, cost);
  KheMTaskGroupDelete(mg);

  /* add in the cost of length being out of range (if it is out of range) */
  if( res )
    *cost += KheLimitActiveCost(dsv, last_index - first_index + 1, last);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDpgSolverSequenceCost(KHE_DPG_SOLVER dsv,                        */
/*    int first_index, int last_index, KHE_COST *cost)                       */
/*                                                                           */
/*  Return the cost of the best sequence from first_index to last_index.     */
/*  These values are cached.                                                 */
/*                                                                           */
/*****************************************************************************/

static bool KheDpgSolverSequenceCost(KHE_DPG_SOLVER dsv,
  int first_index, int last_index, KHE_COST *cost)
{
  /* if there is a cached value, return that */
  if( KheDpgCostCacheContainsCost(dsv->cost_cache, first_index,
	last_index, cost) )
    return true;

  /* else if we can find a cost, cache it and return it */
  if( KheDpgSolverFindSequenceCost(dsv, first_index, last_index, cost) )
  {
    KheDpgCostCacheAddCost(dsv->cost_cache, first_index,
      last_index, *cost);
    return true;
  }

  /* else no luck */
  return false;
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
  HaArrayForEach(dsv->dpg_time_groups, dtg, i)
    KheDpgTimeGroupDebugMTasks(dtg, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheProfileGroupingByDynamicProgramming(KHE_COMB_GROUPER cg,          */
/*    KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int min_limit, int max_limit, */
/*    int history_before, int history_after, KHE_SOLN_ADJUSTER sa,           */
/*    bool *progressing)                                                     */
/*                                                                           */
/*  Carry out profile grouping using dynamic programming on laim, with       */
/*  the given limits and history.  Return the number of groups made, and     */
/*  set *progressing to true if we got somewhere.                            */
/*                                                                           */
/*****************************************************************************/

int KheProfileGroupingByDynamicProgramming(KHE_COMB_GROUPER cg,
  KHE_LIMIT_ACTIVE_INTERVALS_MONITOR laim, int min_limit, int max_limit,
  int history_before, int history_after, KHE_SOLN_ADJUSTER sa,
  bool *progressing)
{
  KHE_DPG_SOLVER dsv;  int i, j, res;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  KHE_DPG_TIME_GROUP prev_dtg, next_dtg, last_dtg;  KHE_RESOURCE_TYPE rt;
  KHE_DPG_SOLN prev_ds, best_ds;  KHE_SOLN soln;  HA_ARENA a;  KHE_TIMER timer;
  char buff[20];

  if( DEBUG1 )
    fprintf(stderr, "[ KheProfileGroupingByDynamicProgramming(%s, %d-%d)\n",
      KheMonitorId((KHE_MONITOR) laim), min_limit, max_limit);

  /* make sure that laim has at least two time groups */
  if( KheLimitActiveIntervalsMonitorTimeGroupCount(laim) < 2 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheProfileGroupingByDynamicProgramming ret. 0 (1)\n");
    return 0;
  }

  /* make the dpg solver and add the time groups */
  soln = KheCombGrouperSoln(cg);
  a = KheSolnArenaBegin(soln);
  if( DEBUG1 )
    timer = KheTimerMake(KheMonitorId((KHE_MONITOR) laim), KHE_NO_TIME, a);
  rt = KheResourceResourceType(KheLimitActiveIntervalsMonitorResource(laim));
  dsv = KheDpgSolverMake(cg, laim, rt, min_limit, max_limit, history_before,
    history_after, sa, a);

  /* add the time groups to dsv */
  for( i = 0;  i < KheLimitActiveIntervalsMonitorTimeGroupCount(laim);  i++ )
  {
    tg = KheLimitActiveIntervalsMonitorTimeGroup(laim, i, &po);
    KheDpgSolverAddTimeGroup(dsv, tg);
  }

  /* add the mtasks to the time groups (all time groups must be present) */
  HaArrayForEach(dsv->dpg_time_groups, next_dtg, i)
    KheDpgTimeGroupAddMTasks(next_dtg, i, dsv);
  if( false && DEBUG1 )
    KheDpgSolverDebug(dsv, 2, 2, stderr);

  /* main algorithm: build solutions for each time group in turn */
  prev_dtg = NULL;
  HaArrayForEach(dsv->dpg_time_groups, next_dtg, i)
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
  last_dtg = HaArrayLast(dsv->dpg_time_groups);
  HnAssert(HaArrayCount(last_dtg->solns) <= 1,
    "KheProfileGroupingByDynamicProgramming internal error");
  if( HaArrayCount(last_dtg->solns) == 1 )
  {
    /* have a solution, so build its groups */
    best_ds = HaArrayFirst(last_dtg->solns);
    /* ***
    if( DEBUG2 )
    {
      fprintf(stderr, "  overall best soln:\n");
      KheDpgSolnDebug(best_ds, 2, 2, stderr);
    }
    *** */
    res = KheDpgSolnBuildGroups(best_ds, dsv);
  }
  else
  {
    /* no solution */
    res = 0;
  }

  /* all done */
  KheSolnArenaEnd(soln, a);
  if( false && DEBUG2 )
    KheDpgCostCacheDebug(dsv->cost_cache, 2, 2, stderr);
  if( DEBUG1 )
    fprintf(stderr, "] KheProfileGroupingByDynamicProgramming ret. %d (%s)\n",
      res, KheTimeShow(KheTimerElapsedTime(timer), buff));
  return res;
}

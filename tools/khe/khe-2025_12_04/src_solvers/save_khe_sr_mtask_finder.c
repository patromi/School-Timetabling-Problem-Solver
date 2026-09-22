
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
/*  FILE:         khe_sr_mtask_finder.c                                      */
/*  DESCRIPTION:  Multi-tasks                                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST - one task with its associated costs                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_and_cost_rec {
  KHE_TASK			task;			/* a task            */
  KHE_COST			non_asst_cost;		/* its non-asst cost */
  KHE_COST			asst_cost;		/* its asst cost     */
} *KHE_TASK_AND_COST;

typedef HA_ARRAY(KHE_TASK_AND_COST) ARRAY_KHE_TASK_AND_COST;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND - classifies monitors into one of four kinds            */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_MONITOR_IGNORE,
  KHE_MONITOR_INSEPARABLE,
  KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT,
  KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT
} KHE_MONITOR_KIND;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG - what similarity of atomic tasks is based on        */
/*                                                                           */
/*****************************************************************************/
typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;

typedef struct khe_atomic_task_sig_rec {
  bool				fixed_times;
  union {
    KHE_TIME			first_time;
    KHE_MEET			encl_meet;
  } u;
  int				duration;
  float				workload_per_time;
  ARRAY_KHE_MONITOR		separable_rd_monitors;
  ARRAY_KHE_MONITOR		inseparable_monitors;
} *KHE_ATOMIC_TASK_SIG;

typedef HA_ARRAY(KHE_ATOMIC_TASK_SIG) ARRAY_KHE_ATOMIC_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_ROOT_TASK_SIG - what similarity of proper root tasks is based on     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_root_task_sig_rec {
  KHE_RESOURCE_GROUP		root_domain;
  KHE_RESOURCE			root_preasst;
  bool				root_asst_is_fixed;
  int				total_duration;
  float				total_workload;
  ARRAY_KHE_ATOMIC_TASK_SIG	atomic_sigs;
} *KHE_ROOT_TASK_SIG;

typedef HA_ARRAY(KHE_ROOT_TASK_SIG) ARRAY_KHE_ROOT_TASK_SIG;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_TIME - what an mtask is doing at one time                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_mtask_time_rec {
  KHE_TIME			time;			/* the time          */
  float				workload_per_time;	/* workload at time  */
} *KHE_MTASK_TIME;

typedef HA_ARRAY(KHE_MTASK_TIME) ARRAY_KHE_MTASK_TIME;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK                                                                */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_rec {
  KHE_MTASK_FINDER		mtask_finder;
  KHE_ROOT_TASK_SIG		sig;
  ARRAY_KHE_TASK_AND_COST	tasks_and_costs;
  int				asst_count;
  bool				fixed_times;  /* not degenerate & fixed times */
  bool				fixed_times_no_overlap;
  bool				fixed_times_no_gaps;
  KHE_INTERVAL			fixed_times_interval;
  ARRAY_KHE_MTASK_TIME		fixed_times_by_day;
};

typedef HA_ARRAY(KHE_MTASK) ARRAY_KHE_MTASK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET                                                            */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_set_rec {
  KHE_INTERVAL			interval;     /* interval covered by mtasks */
  ARRAY_KHE_MTASK		mtasks;
};

typedef HA_ARRAY(KHE_MTASK_SET) ARRAY_KHE_MTASK_SET;


/*****************************************************************************/
/*                                                                           */
/*  NEEDS_ASST_TYPE - says whether task should need assignment or not        */
/*                                                                           */
/*****************************************************************************/

/* ***
typedef enum {
  NEEDS_ASST_FALSE,
  NEEDS_ASST_TRUE,
  NEEDS_ASST_DONT_CARE
} NEEDS_ASST_TYPE;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART_FROM - what we need to know about what from_r is doing         */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
typedef struct khe_mpart_from_rec {
  KHE_MTASK_SET			mts;
  KHE_INTERVAL			in;
} *KHE_MPART_FROM;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART_TO - what we need to know about what to_r is doing             */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
typedef struct khe_mpart_to_rec {
  KHE_MTASK_SET			mts;
  int				durn;
  bool				effectively_free;
} *KHE_MPART_TO;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART                                                                */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
typedef struct khe_mpart_rec {
  struct khe_mpart_from_rec		from;
  struct khe_mpart_to_rec		to;
  bool					curr_is_swap;
  bool					best_is_swap;
} *KHE_MPART;

typedef HA_ARRAY(KHE_MPART) ARRAY_KHE_MPART;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_MTASK_SET - a widened mtask set, with a core and wings       */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
struct khe_widened_mtask_set_rec {
  KHE_MTASK_FINDER			mtask_finder;
  KHE_RESOURCE				from_r;
  bool					to_r_is_set;
  KHE_RESOURCE				to_r;
  struct khe_mpart_rec			core;
  ARRAY_KHE_MPART			left_wing;
  ARRAY_KHE_MPART			right_wing;
  int					to_r_max_left_wing_count;
  int					to_r_max_right_wing_count;

  ** optimal moves **
  ARRAY_KHE_MPART			swappable_wings;
  KHE_COST				best_cost;
  int					best_defects;
};

typedef HA_ARRAY(KHE_WIDENED_MTASK_SET) ARRAY_KHE_WIDENED_MTASK_SET;
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET - cache of mtask sets indexed by time group     */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_time_group_mtask_set_rec {
  KHE_RESOURCE_TYPE		rt;
  KHE_TIME_GROUP		tg;
  KHE_MTASK_SET			mts;
} *KHE_TIME_GROUP_MTASK_SET;

typedef HA_ARRAY(KHE_TIME_GROUP_MTASK_SET) ARRAY_KHE_TIME_GROUP_MTASK_SET;

typedef struct khe_time_group_mtask_set_set_rec {
  ARRAY_KHE_TIME_GROUP_MTASK_SET	mtask_sets;
} *KHE_TIME_GROUP_MTASK_SET_SET;

typedef HA_ARRAY(KHE_TIME_GROUP_MTASK_SET_SET)
  ARRAY_KHE_TIME_GROUP_MTASK_SET_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET - cache of mtask sets indexed by interval         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_interval_mtask_set_rec {
  KHE_RESOURCE_TYPE		rt;
  KHE_INTERVAL			in;
  KHE_MTASK_SET			mts;
} *KHE_INTERVAL_MTASK_SET;

typedef HA_ARRAY(KHE_INTERVAL_MTASK_SET) ARRAY_KHE_INTERVAL_MTASK_SET;

typedef struct khe_interval_mtask_set_set_rec {
  ARRAY_KHE_INTERVAL_MTASK_SET	mtask_sets;
} *KHE_INTERVAL_MTASK_SET_SET;

typedef HA_ARRAY(KHE_INTERVAL_MTASK_SET_SET) ARRAY_KHE_INTERVAL_MTASK_SET_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER                                                         */
/*                                                                           */
/*****************************************************************************/

struct khe_mtask_finder_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_RESOURCE_TYPE		resource_type;
  KHE_FRAME			days_frame;
  KHE_EVENT_TIMETABLE_MONITOR	etm;
  bool				fixed_times;
  /* bool			make_group_monitors; */
  ARRAY_KHE_MTASK		mtasks;			/* the mtasks        */
  ARRAY_KHE_MTASK_SET		mtasks_at_time;	       	/* mtasks at time    */
  ARRAY_KHE_MTASK_SET		mtasks_at_meet;		/* mtasks at meet    */
  ARRAY_KHE_MTASK		mtasks_by_task;		/* indexed by task   */
  ARRAY_KHE_TIME_GROUP_MTASK_SET_SET mtasks_in_tg;
  ARRAY_KHE_INTERVAL_MTASK_SET_SET   mtasks_in_in;
  KHE_MTASK_SET			empty_mtask_set;
  ARRAY_KHE_TASK_AND_COST	free_tasks_and_costs;	/* free tasks & costs*/
  ARRAY_KHE_ATOMIC_TASK_SIG	free_atomic_task_sigs;	/* free atomic sigs  */
  ARRAY_KHE_ROOT_TASK_SIG	free_root_task_sigs;	/* free task sigs    */
  /* ARRAY_KHE_MTASK_TIME	free_mtask_times; */	/* free class times  */
  /* ARRAY_KHE_MTASK		free_mtasks; */		/* free mtasks       */
  ARRAY_KHE_MTASK_SET		free_mtask_sets;	/* free mtask sets   */
  /* struct khe_mpart_from_rec	scratch_mfrom; */
  /* struct khe_mpart_to_rec	scratch_mto; */
  /* ARRAY_KHE_WIDENED_MTASK_SET free_widened_mtask_sets; */
  /* ARRAY_KHE_MPART		free_mparts; */
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Multi-task finders"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFinderFindMTask(KHE_MTASK_FINDER mtf,                       */
/*    KHE_ROOT_TASK_SIG sig, KHE_MTASK *res, KHE_MTASK_SET *mts)             */
/*                                                                           */
/*  If mtf contains an mtask with this sig, set *res to that mtask and       */
/*  return true.  Otherwise set *res to NULL and return false.               */
/*                                                                           */
/*  But either way, also return in *mts the mtask set that an mtask with     */
/*  this signature should be in, or NULL if sig is degenerate.               */
/*                                                                           */
/*****************************************************************************/
static bool KheRootTaskSigIsNonDegenerate(KHE_ROOT_TASK_SIG sig);
static bool KheRootTaskSigFixedTimes(KHE_ROOT_TASK_SIG sig, int *sort_index);
static bool KheRootTaskSigSimilar(KHE_ROOT_TASK_SIG sig1,
  KHE_ROOT_TASK_SIG sig2);

static bool KheMTaskFinderFindMTask(KHE_MTASK_FINDER mtf,
  KHE_ROOT_TASK_SIG sig, KHE_MTASK *res, KHE_MTASK_SET *mts)
{
  KHE_MTASK mt;  int i, sort_index;

  /* get the mtask set that an mtask with this sig should be in */
  if( KheRootTaskSigIsNonDegenerate(sig) )
  {
    if( KheRootTaskSigFixedTimes(sig, &sort_index) )
    {
      /* search for sig among mtasks_at_time with this sort index */
      HaArrayFill(mtf->mtasks_at_time, sort_index + 1, NULL);
      *mts = HaArray(mtf->mtasks_at_time, sort_index);
      if( *mts == NULL )
      {
	*mts = KheMTaskSetMake(mtf);
	HaArrayPut(mtf->mtasks_at_time, sort_index, *mts);
      }
    }
    else
    {
      /* search for sig among mtasks_at_meet with this sort index */
      HaArrayFill(mtf->mtasks_at_meet, sort_index + 1, NULL);
      *mts = HaArray(mtf->mtasks_at_meet, sort_index);
      if( *mts == NULL )
      {
	*mts = KheMTaskSetMake(mtf);
	HaArrayPut(mtf->mtasks_at_meet, sort_index, *mts);
      }
    }

    /* search *mts for an mtask with this signature */
    HaArrayForEach((*mts)->mtasks, mt, i)
      if( KheRootTaskSigSimilar(sig, mt->sig) )
	return *res = mt, true;
  }
  else
    *mts = NULL;
  return *res = NULL, false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)          */
/*                                                                           */
/*  Add task to mtf, if suitable.                                            */
/*                                                                           */
/*****************************************************************************/
static KHE_ROOT_TASK_SIG KheRootTaskSigMake(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost);
static bool KheMTaskContainsTask(KHE_MTASK mt, KHE_TASK task);
static KHE_MTASK KheMTaskMake(KHE_MTASK_FINDER mtf, KHE_ROOT_TASK_SIG sig);
static void KheRootTaskSigDelete(KHE_ROOT_TASK_SIG sig, KHE_MTASK_FINDER mtf);
static void KheMTaskAddTask(KHE_MTASK mt, KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf);

static void KheMTaskFinderAddTask(KHE_MTASK_FINDER mtf, KHE_TASK task)
{
  KHE_MTASK mt;  KHE_ROOT_TASK_SIG sig;  KHE_COST non_asst_cost, asst_cost;
  KHE_MTASK_SET mts;

  if( KheTaskIsProperRoot(task) && (mtf->resource_type == NULL ||
	KheTaskResourceType(task) == mtf->resource_type) )
  {
    sig = KheRootTaskSigMake(mtf, task, &non_asst_cost, &asst_cost);
    if( KheMTaskFinderFindMTask(mtf, sig, &mt, &mts) )
    {
      /* an mt with this sig exists, so delete sig; we'll add to mt */
      HnAssert(!KheMTaskContainsTask(mt, task),
	"KheMTaskFinderAddTask internal error:  task already added");
      KheRootTaskSigDelete(sig, mtf);
    }
    else
    {
      /* no mtask for task, so make one and add it to mts and mtf */
      mt = KheMTaskMake(mtf, sig);
      if( mts != NULL )
	KheMTaskSetAddMTask(mts, mt);
      HaArrayAddLast(mtf->mtasks, mt);
    }
    if( DEBUG1 )
      fprintf(stderr, "  adding task %s\n", KheTaskId(task));
    KheMTaskAddTask(mt, task, non_asst_cost, asst_cost, mtf);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_FINDER KheMTaskFinderMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt, */
/*    KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,                 */
/*    bool fixed_times, HA_ARENA a)                                          */
/*                                                                           */
/*  Make a new mtask finder object with these attributes.                    */
/*                                                                           */
/*****************************************************************************/
/* static void KheMPartFromInit(KHE_MPART_FROM pf, KHE_MTASK_SET mts); */
/* static void KheMPartToInit(KHE_MPART_TO pt, KHE_MTASK_SET mts); */

KHE_MTASK_FINDER KheMTaskFinderMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm,
  bool fixed_times, HA_ARENA a)
{
  KHE_MTASK_FINDER res;  int i;  KHE_TASK task;

  if( DEBUG1 )
    fprintf(stderr, "[ KheMTaskFinderMake(soln of %s, rt %s, frame, etm, %s)\n",
      KheInstanceId(KheSolnInstance(soln)),
      rt == NULL ? "-" : KheResourceTypeId(rt),
      bool_show(fixed_times));

  /* make the basic object */
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->days_frame = days_frame;
  res->etm = etm;
  res->fixed_times = fixed_times;
  /* res->make_group_monitors = make_group_monitors; */
  HaArrayInit(res->mtasks, a);
  HaArrayInit(res->mtasks_at_time, a);
  HaArrayInit(res->mtasks_at_meet, a);
  HaArrayInit(res->mtasks_by_task, a);
  HaArrayInit(res->mtasks_in_tg, a);
  HaArrayInit(res->mtasks_in_in, a);
  HaArrayInit(res->free_tasks_and_costs, a);
  HaArrayInit(res->free_atomic_task_sigs, a);
  HaArrayInit(res->free_root_task_sigs, a);
  /* HaArrayInit(res->free_mtask_times, a); */
  /* HaArrayInit(res->free_mtasks, a); */
  HaArrayInit(res->free_mtask_sets, a);
  res->empty_mtask_set = KheMTaskSetMake(res);  /* must follow prv line */
  /* KheMPartFromInit(&res->scratch_mfrom, KheMTaskSetMake(res)); */
  /* KheMPartToInit(&res->scratch_mto, KheMTaskSetMake(res)); */
  /* HaArrayInit(res->free_widened_mtask_sets, a); */
  /* HaArrayInit(res->free_mparts, a); */

  /* add all tasks */
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    KheMTaskFinderAddTask(res, task);
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheMTaskFinderMake returning\n");
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderLastIndex(KHE_MTASK_FINDER mtf)                        */
/*                                                                           */
/*  Return the last index of mtf.                                            */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderLastIndex(KHE_MTASK_FINDER mtf)
{
  return KheFrameTimeGroupCount(mtf->days_frame) - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderMTaskCount(KHE_MTASK_FINDER mtf)                       */
/*                                                                           */
/*  Return the number of mtasks in mtf.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskCount(KHE_MTASK_FINDER mtf)
{
  return HaArrayCount(mtf->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTask(KHE_MTASK_FINDER mtf, int i)               */
/*                                                                           */
/*  Return the i'th mtask of mtf.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTask(KHE_MTASK_FINDER mtf, int i)
{
  return HaArray(mtf->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderMTaskSort(KHE_MTASK_FINDER mtf,                       */
/*    int (*compar)(const void *, const void *))                             */
/*                                                                           */
/*  Sort the mtasks of mtf using comparison function compar.                 */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderMTaskSort(KHE_MTASK_FINDER mtf,
  int (*compar)(const void *, const void *))
{
  HaArraySort(mtf->mtasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskDecreasingDurationTypedCmp(KHE_MTASK mt1, KHE_MTASK mt2)     */
/*                                                                           */
/*  Typed comparison function for sorting mtasks by decreasing duration.     */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskDecreasingDurationTypedCmp(KHE_MTASK mt1, KHE_MTASK mt2)
{
  return mt2->sig->total_duration - mt1->sig->total_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskDecreasingDurationCmp(const void *t1, const void *t2)        */
/*                                                                           */
/*  Untyped comparison function for sorting mtasks by decreasing duration.   */
/*                                                                           */
/*****************************************************************************/

int KheMTaskDecreasingDurationCmp(const void *t1, const void *t2)
{
  KHE_MTASK mt1 = * (KHE_MTASK *) t1;
  KHE_MTASK mt2 = * (KHE_MTASK *) t2;
  return KheMTaskDecreasingDurationTypedCmp(mt1, mt2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFinderMTaskAtTimeCount(KHE_MTASK_FINDER mtf, KHE_TIME time)  */
/*                                                                           */
/*  Return the number of mtasks in mtf whose start time is time.             */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskAtTimeCount(KHE_MTASK_FINDER mtf, KHE_TIME time)
{
  KHE_MTASK_SET mts;  int index;
  index = KheTimeIndex(time);
  if( index < HaArrayCount(mtf->mtasks_at_time) )
  {
    mts = HaArray(mtf->mtasks_at_time, index);
    return HaArrayCount(mts->mtasks);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTaskAtTime(KHE_MTASK_FINDER mtf,                */
/*    KHE_TIME time, int i)                                                  */
/*                                                                           */
/*  Return the i'th mtask of mtf whose start time is time.                   */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTaskAtTime(KHE_MTASK_FINDER mtf,
  KHE_TIME time, int i)
{
  KHE_MTASK_SET mts;  int index;
  index = KheTimeIndex(time);
  HnAssert(index < HaArrayCount(mtf->mtasks_at_time),
    "KheMTaskFinderMTaskAtTime internal error: i out of range");
  mts = HaArray(mtf->mtasks_at_time, index);
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/* int KheMTaskFinderMTaskFromMeetCount(KHE_MTASK_FINDER mtf, KHE_MEET meet) */
/*                                                                           */
/*  Return the number of mtasks in mtf whose meet is meet.                   */
/*                                                                           */
/*****************************************************************************/

int KheMTaskFinderMTaskFromMeetCount(KHE_MTASK_FINDER mtf, KHE_MEET meet)
{
  KHE_MTASK_SET mts;  int index;
  index = KheMeetSolnIndex(meet);
  if( index < HaArrayCount(mtf->mtasks_at_meet) )
  {
    mts = HaArray(mtf->mtasks_at_meet, index);
    return HaArrayCount(mts->mtasks);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderMTaskFromMeet(KHE_MTASK_FINDER mtf,              */
/*    KHE_MEET meet, int i)                                                  */
/*                                                                           */
/*  Return the i'th task derived from meet.                                  */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderMTaskFromMeet(KHE_MTASK_FINDER mtf,
  KHE_MEET meet, int i)
{
  KHE_MTASK_SET mts;  int index;
  index = KheMeetSolnIndex(meet);
  mts = HaArray(mtf->mtasks_at_meet, index);
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskFinderTaskToMTask(KHE_MTASK_FINDER mtf, KHE_TASK t)    */
/*                                                                           */
/*  Return the mtask corresponding to the proper root of task t.             */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskFinderTaskToMTask(KHE_MTASK_FINDER mtf, KHE_TASK t)
{
  int index;  KHE_MTASK res;
  index = KheTaskSolnIndex(KheTaskProperRoot(t));
  HaArrayFill(mtf->mtasks_by_task, index + 1, NULL);
  res = HaArray(mtf->mtasks_by_task, index);
  HnAssert(res != NULL,
    "KheMTaskFinderTaskToMTask: task %s (proper root %s with index %d) has "
    "no mtask", KheTaskId(t), KheTaskId(KheTaskProperRoot(t)), index);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderDebug(KHE_MTASK_FINDER mtf, int verbosity,            */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mtf onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderDebug(KHE_MTASK_FINDER mtf, int verbosity,
  int indent, FILE *fp)
{
  KHE_MTASK mt;  int i, j;  KHE_MTASK_SET mts;
  fprintf(fp, "%*s[ MTaskFinder(%s, %s)\n", indent, "",
    KheInstanceId(KheSolnInstance(mtf->soln)),
    KheResourceTypeId(mtf->resource_type));
  HaArrayForEach(mtf->mtasks_at_time, mts, i)
  {
    fprintf(fp, "%*s  [ time %d:\n", indent, "", i);
    HaArrayForEach(mts->mtasks, mt, j)
      KheMTaskDebug(mt, verbosity, indent + 4, fp);
    fprintf(fp, "%*s  ]\n", indent, "");
  }
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "time group mtask sets"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheAddTimeGroupMTasks(KHE_MTASK_FINDER mtf,                         */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_MTASK_SET mts)            */
/*                                                                           */
/*  Add to mts the mtasks of type rt running during tg.  Don't worry about   */
/*  uniqueifying, that will be done later.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheAddTimeGroupMTasks(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg, KHE_MTASK_SET mts)
{
  int i, j, k, meet_count, task_count;
  KHE_TIME t;  KHE_MEET meet;  KHE_TASK task;  KHE_MTASK mt;
  if( DEBUG2 )
    fprintf(stderr, "[ KheAddTimeGroupMTasks(mtf, %s, %s, mts)\n",
      rt == NULL ? "NULL" : KheResourceTypeId(rt), KheTimeGroupId(tg));
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    meet_count = KheEventTimetableMonitorTimeMeetCount(mtf->etm, t);
    for( j = 0;  j < meet_count;  j++ )
    {
      meet = KheEventTimetableMonitorTimeMeet(mtf->etm, t, j);
      task_count = KheMeetTaskCount(meet);
      if( DEBUG2 )
	fprintf(stderr, "  examining meet %s (with %d tasks)\n",
	  KheMeetId(meet), task_count);
      for( k = 0;  k < task_count;  k++ )
      {
	task = KheMeetTask(meet, k);
	if( DEBUG2 )
	  fprintf(stderr, "    examining task %s (type %s)\n",
	    KheTaskId(task), KheResourceTypeId(KheTaskResourceType(task)));
	if( KheTaskResourceType(task) == rt )
	{
	  mt = KheMTaskFinderTaskToMTask(mtf, task);
	  KheMTaskSetAddMTask(mts, mt);
	}
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheAddTimeGroupMTasks returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET KheTimeGroupMTaskSetMake(KHE_MTASK_FINDER mtf,  */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Make a new time group mtask set object for rt and tg, including          */
/*  building and uniqueifying its mtask set.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_MTASK_SET KheTimeGroupMTaskSetMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  KHE_TIME_GROUP_MTASK_SET res;
  HaMake(res, mtf->arena);
  res->rt = rt;
  res->tg = tg;
  res->mts = KheMTaskSetMake(mtf);
  KheAddTimeGroupMTasks(mtf, rt, tg, res->mts);
  KheMTaskSetUniqueify(res->mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME_GROUP_MTASK_SET_SET KheTimeGroupMTaskSetSetMake(                */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Make a new, empty time group mtask set set object.                       */
/*                                                                           */
/*****************************************************************************/

static KHE_TIME_GROUP_MTASK_SET_SET KheTimeGroupMTaskSetSetMake(
  KHE_MTASK_FINDER mtf)
{
  KHE_TIME_GROUP_MTASK_SET_SET res;
  HaMake(res, mtf->arena);
  HaArrayInit(res->mtask_sets, mtf->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,      */
/*    KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)                               */
/*                                                                           */
/*  Return an mtask set containing all mtasks of resource type rt running    */
/*  (wholly or partly) in time group tg.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskFinderMTasksInTimeGroup(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_TIME_GROUP tg)
{
  int i;  KHE_TIME_GROUP_MTASK_SET_SET tg_mtss;
  KHE_TIME_GROUP_MTASK_SET tg_mts;

  /* empty time group implies empty mtask set */
  if( KheTimeGroupTimeCount(tg) == 0 )
    return mtf->empty_mtask_set;

  /* make sure that there is a time group mtask set set at tg's first time */
  i = KheTimeIndex(KheTimeGroupTime(tg, 0));
  HaArrayFill(mtf->mtasks_in_tg, i + 1, NULL);
  tg_mtss = HaArray(mtf->mtasks_in_tg, i);
  if( tg_mtss == NULL )
  {
    tg_mtss = KheTimeGroupMTaskSetSetMake(mtf);
    HaArrayPut(mtf->mtasks_in_tg, i, tg_mtss);
  }

  /* retrieve and return the mtask set for tg if there is one */
  HaArrayForEach(tg_mtss->mtask_sets, tg_mts, i)
    if( tg_mts->rt == rt && tg_mts->tg == tg )
      return tg_mts->mts;

  /* make, add, and return a new mtask set for tg */
  tg_mts = KheTimeGroupMTaskSetMake(mtf, rt, tg);
  HaArrayAddLast(tg_mtss->mtask_sets, tg_mts);
  return tg_mts->mts;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "interval mtask sets"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET KheIntervalMTaskSetMake(KHE_MTASK_FINDER mtf,     */
/*    KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)                                 */
/*                                                                           */
/*  Make a new interval mtask set object for rt and in, including building   */
/*  and uniqueifying its mtask set.                                          */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_MTASK_SET KheIntervalMTaskSetMake(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  KHE_INTERVAL_MTASK_SET res;  int i;  KHE_TIME_GROUP tg;
  HaMake(res, mtf->arena);
  res->rt = rt;
  res->in = in;
  res->mts = KheMTaskSetMake(mtf);
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    tg = KheFrameTimeGroup(mtf->days_frame, i);
    KheAddTimeGroupMTasks(mtf, rt, tg, res->mts);
  }
  KheMTaskSetUniqueify(res->mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL_MTASK_SET_SET KheIntervalMTaskSetSetMake(                   */
/*    KHE_MTASK_FINDER mtf)                                                  */
/*                                                                           */
/*  Make a new, empty interval mtask set set object.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_INTERVAL_MTASK_SET_SET KheIntervalMTaskSetSetMake(
  KHE_MTASK_FINDER mtf)
{
  KHE_INTERVAL_MTASK_SET_SET res;
  HaMake(res, mtf->arena);
  HaArrayInit(res->mtask_sets, mtf->arena);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,       */
/*    KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)                                 */
/*                                                                           */
/*  Return an mtask set containing all mtasks of resource type rt running    */
/*  (wholly or partly) in interval in.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskFinderMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE_TYPE rt, KHE_INTERVAL in)
{
  int i;  KHE_INTERVAL_MTASK_SET_SET in_mtss;
  KHE_INTERVAL_MTASK_SET in_mts;

  /* empty interval implies empty mtask set */
  if( KheIntervalLength(in) == 0 )
    return mtf->empty_mtask_set;

  /* make sure that there is an interval mtask set set at in's first */
  i = KheIntervalFirst(in);
  HaArrayFill(mtf->mtasks_in_in, i + 1, NULL);
  in_mtss = HaArray(mtf->mtasks_in_in, i);
  if( in_mtss == NULL )
  {
    in_mtss = KheIntervalMTaskSetSetMake(mtf);
    HaArrayPut(mtf->mtasks_in_in, i, in_mtss);
  }

  /* retrieve and return the mtask set for in if there is one */
  HaArrayForEach(in_mtss->mtask_sets, in_mts, i)
    if( in_mts->rt == rt && KheIntervalEqual(in_mts->in, in) )
      return in_mts->mts;

  /* make, add, and return a new mtask set for in */
  in_mts = KheIntervalMTaskSetMake(mtf, rt, in);
  HaArrayAddLast(in_mtss->mtask_sets, in_mts);
  return in_mts->mts;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderAddResourceMTasksInInterval(KHE_MTASK_FINDER mtf,     */
/*    KHE_RESOURCE r, KHE_INTERVAL in, KHE_MTASK_SET mts)                    */
/*                                                                           */
/*  Add to mts the mtasks that r is assigned to in interval in, in order.    */
/*  The chosen mtasks lie completely (not partially) within in.              */
/*                                                                           */
/*****************************************************************************/

void KheMTaskFinderAddResourceMTasksInInterval(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE r, KHE_INTERVAL in, KHE_MTASK_SET mts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j, k;  KHE_TIME_GROUP tg;
  KHE_TIME t;  KHE_TASK task;  KHE_MTASK mt, prev_mt;  KHE_INTERVAL mt_in;
  rtm = KheResourceTimetableMonitor(mtf->soln, r);
  prev_mt = NULL;
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(mtf->days_frame, i);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	mt = KheMTaskFinderTaskToMTask(mtf, task);
	mt_in = KheMTaskInterval(mt);
	if( mt != prev_mt && KheIntervalSubset(mt_in, in) )
	{
	  KheMTaskSetAddMTask(mts, mt);
	  prev_mt = mt;
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddResourceProperRootTasksInInterval(KHE_RESOURCE r,             */
/*    KHE_INTERVAL in, KHE_SOLN soln, KHE_FRAME days_frame, KHE_TASK_SET ts) */
/*                                                                           */
/*  Add to mts the mtasks that r is assigned to in interval in, in order.    */
/*  The chosen mtasks lie completely (not partially) within in.              */
/*                                                                           */
/*****************************************************************************/

void KheAddResourceProperRootTasksInInterval(KHE_RESOURCE r,
  KHE_INTERVAL in, KHE_SOLN soln, KHE_FRAME days_frame, KHE_TASK_SET ts)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  int i, j, k;  KHE_TIME_GROUP tg;
  KHE_TIME t;  KHE_TASK task, prev_task;  KHE_INTERVAL task_in;
  rtm = KheResourceTimetableMonitor(soln, r);
  prev_task = NULL;
  for( i = in.first;  i <= in.last;  i++ )
  {
    tg = KheFrameTimeGroup(days_frame, i);
    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
    {
      t = KheTimeGroupTime(tg, j);
      for( k = 0;  k < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  k++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, k);
	task_in = KheTaskInterval(task, days_frame);
	if( task != prev_task && KheIntervalSubset(task_in, in) )
	{
	  KheTaskSetAddTask(ts, task);
	  prev_task = task;
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_AND_COST"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task,                      */
/*    KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)      */
/*                                                                           */
/*  Make and return a new task and cost object.                              */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_COST KheTaskAndCostMake(KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)
{
  KHE_TASK_AND_COST res;
  if( HaArrayCount(mtf->free_tasks_and_costs) > 0 )
    res = HaArrayLastAndDelete(mtf->free_tasks_and_costs);
  else
    HaMake(res, mtf->arena);
  res->task = task;
  res->non_asst_cost = non_asst_cost;
  res->asst_cost = asst_cost;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ATOMIC_TASK_SIG"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheEventResourceTasksHaveSameProperRoot(KHE_EVENT_RESOURCE er,      */
/*    KHE_SOLN soln, KHE_TASK *proper_root_task)                             */
/*                                                                           */
/*  If the tasks derived from er in soln have the same proper root task,     */
/*  return true with *proper_root_task set to that proper root task.         */
/*  Otherwise return false with *proper_root_task set to NULL.               */
/*                                                                           */
/*****************************************************************************/

static bool KheEventResourceTasksHaveSameProperRoot(KHE_EVENT_RESOURCE er,
  KHE_SOLN soln, KHE_TASK *proper_root_task)
{
  KHE_TASK task, prt;  int i;
  prt = KheTaskProperRoot(KheEventResourceTask(soln, er, 0));
  for( i = 1;  i < KheEventResourceTaskCount(soln, er);  i++ )
  {
    task = KheTaskProperRoot(KheEventResourceTask(soln, er, i));
    if( task != prt )
      return *proper_root_task = NULL, false;
  }
  return *proper_root_task = prt, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(             */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_SOLN soln)               */
/*                                                                           */
/*  Return true when the tasks monitored by asam have the same proper root.  */
/*                                                                           */
/*****************************************************************************/

static bool KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam, KHE_SOLN soln)
{
  KHE_TASK prt, task;  int i, eg_index, count;  KHE_EVENT_RESOURCE er;
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT asac;
  asac = KheAvoidSplitAssignmentsMonitorConstraint(asam);
  eg_index = KheAvoidSplitAssignmentsMonitorEventGroupIndex(asam);
  prt = NULL;
  count = KheAvoidSplitAssignmentsConstraintEventResourceCount(asac, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheAvoidSplitAssignmentsConstraintEventResource(asac, eg_index, i);
    if( !KheEventResourceTasksHaveSameProperRoot(er, soln, &task) )
      return false;
    if( prt == NULL )
      prt = task;
    else if( prt != task )
      return false;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorTasksHaveSameProperRoot(                    */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_SOLN soln)                        */
/*                                                                           */
/*  Return true when the tasks monitored by lrm have the same proper root.   */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheLimitResourcesMonitorTasksHaveSameProperRoot(
  KHE_LIMIT_RESOURCES_MONITOR lrm, KHE_SOLN soln)
{
  KHE_TASK prt, task;  int i, eg_index, count;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  prt = NULL;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    if( !KheEventResourceTasksHaveSameProperRoot(er, soln, &task) )
      return false;
    if( prt == NULL )
      prt = task;
    else if( prt != task )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesMonitorTaskCount(KHE_LIMIT_RESOURCES_MONITOR lrm,   */
/*    KHE_SOLN soln)                                                         */
/*                                                                           */
/*  Return the number of tasks monitored by lrm.                             */
/*                                                                           */
/*****************************************************************************/

static int KheLimitResourcesMonitorTaskCount(KHE_LIMIT_RESOURCES_MONITOR lrm,
  KHE_SOLN soln)
{
  int i, eg_index, count, res;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  res = 0;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    res += KheEventResourceTaskCount(soln, er);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheLimitResourcesMonitorTaskDuration(KHE_LIMIT_RESOURCES_MONITOR lrm)*/
/*                                                                           */
/*  Return the total duration of the tasks monitored by lrm.                 */
/*                                                                           */
/*****************************************************************************/

static int KheLimitResourcesMonitorTaskDuration(KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  int i, eg_index, count, res;  KHE_EVENT_RESOURCE er;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  eg_index = KheLimitResourcesMonitorEventGroupIndex(lrm);
  res = 0;
  count = KheLimitResourcesConstraintEventResourceCount(lrc, eg_index);
  for( i = 0;  i < count;  i++ )
  {
    er = KheLimitResourcesConstraintEventResource(lrc, eg_index, i);
    res += KheEventDuration(KheEventResourceEvent(er));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_COST KheFi ndCost(KHE_CONSTRAINT c, int dev)                         */
/*                                                                           */
/*  Find the cost of c when the deviation is dev.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_COST KheFin dCost(KHE_CONSTRAINT c, int dev)
{
  switch( KheConst raintCostFunction(c) )
  {
    case KHE_STEP_COST_FUNCTION:

      return dev > 0 ? KheConstr aintCombinedWeight(c) : 0;

    case KHE_LINEAR_COST_FUNCTION:

      return KheConstra intCombinedWeight(c) * dev;

    case KHE_QUADRATIC_COST_FUNCTION:

      return KheConstr aintCombinedWeight(c) * dev * dev;

    default:

      HnAbort("KheFindCost: unknown cost function (%d)",
	KheConstrain tCostFunction(c));
      return 0;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KheAssignResourceConstraintClassify(KHE_CONSTRAINT c,   */
/*    int task_count, int durn, KHE_COST *non_asst_cost, KHE_COST *asst_cost)*/
/*                                                                           */
/*  Carry out KheMonitorKind for an assign resource constraint with the      */
/*  cost function and weight of c which monitors task_count tasks.           */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KheAssignResourceMonitorClassify(KHE_MONITOR m,
  int task_count, int durn, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  if( task_count == 1 || KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
  {
    /* separable resource-independent with non-assignment cost */
    *non_asst_cost += KheMonitorDevToCost(m, durn);
    /* *non_asst_cost += KheFindC ost(c, durn); */
    return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
  }
  else
  {
    /* inseparable */
    return KHE_MONITOR_INSEPARABLE;
  }
}

/* ***
static KHE_MONITOR_KIND KheAssignResourceConstraintClassify(KHE_CONSTRAINT c,
  int task_count, int durn, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  if( task_count == 1 ||
      KheConstrain tCostFunction(c) == KHE_LINEAR_COST_FUNCTION )
  {
    ** separable resource-independent with non-assignment cost **
    *non_asst_cost += KheFindC ost(c, durn);
    return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
  }
  else
  {
    ** inseparable **
    return KHE_MONITOR_INSEPARABLE;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KhePreferResourcesConstraintClassify(KHE_CONSTRAINT c,  */
/*    KHE_RESOURCE_TYPE rt, int resource_count, int task_count, int durn,    */
/*    KHE_COST *non_asst_cost, KHE_COST *asst_cost)                          */
/*                                                                           */
/*  Carry out KheMonitorKind for a prefer resources constraint with the      */
/*  cost function and weight of c which prefers resource_count resources     */
/*  of type rt and monitors task_count tasks.                                */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KhePreferResourcesMonitorClassify(KHE_MONITOR m,
  KHE_RESOURCE_TYPE rt, int resource_count, int task_count, int durn,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  if( resource_count == KheResourceTypeResourceCount(rt) )
  {
    /* no cost is possible, ignore this monitor */
    return KHE_MONITOR_IGNORE;
  }
  else if( task_count == 1 ||
    KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION )
  {
    /* separable */
    if( resource_count == 0 )
    {
      /* separable resource-independent with assignment cost */
      *asst_cost += KheMonitorDevToCost(m, durn);
      return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
    }
    else
    {
      /* separable resource-dependent */
      return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
    }
  }
  else
  {
    /* inseparable */
    return KHE_MONITOR_INSEPARABLE;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorMimicsAssignResourceMonitor(                */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm)                                       */
/*                                                                           */
/*  Return true if lrm mimics an assign resource monitor:  if it monitors    */
/*  every resource of its type and its minimum limit is equal to the total   */
/*  duration of the tasks it monitors.                                       */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorMimicsAssignResourceMonitor(
  KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE_TYPE rt;  int rg_count, min_limit;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  rg = KheLimitResourcesConstraintDomain(lrc);
  rg_count = KheResourceGroupResourceCount(rg);
  rt = KheResourceGroupResourceType(rg);
  min_limit = KheLimitResourcesConstraintMinimum(lrc);
  return rg_count == KheResourceTypeResourceCount(rt) &&
    min_limit == KheLimitResourcesMonitorTaskDuration(lrm);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheLimitResourcesMonitorMimicsPreferResourcesMonitor(               */
/*    KHE_LIMIT_RESOURCES_MONITOR lrm)                                       */
/*                                                                           */
/*  Return true if lrm mimics a prefer resources monitor:  if its maximum    */
/*  limit is 0.  It will then mimic a prefer resources monitor whose set     */
/*  of preferred resources is the complement of lrm's set of resources.      */
/*                                                                           */
/*****************************************************************************/

static bool KheLimitResourcesMonitorMimicsPreferResourcesMonitor(
  KHE_LIMIT_RESOURCES_MONITOR lrm)
{
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  lrc = KheLimitResourcesMonitorConstraint(lrm);
  return KheLimitResourcesConstraintMaximum(lrc) == 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,                 */
/*    KHE_COST *non_asst_cost, KHE_COST *asst_cost)                          */
/*                                                                           */
/*  Return m's kind.  If m is separable resource-independent, also add to    */
/*  *non_asst_cost the cost of not assigning a resource to a task of the     */
/*  given durn monitored by m, and add to *asst_cost the cost of doing so.   */
/*                                                                           */
/*****************************************************************************/

static KHE_MONITOR_KIND KheMonitorKind(KHE_MONITOR m, int durn,
  KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_PREFER_RESOURCES_MONITOR prm;
  KHE_LIMIT_RESOURCES_MONITOR lrm;  KHE_LIMIT_RESOURCES_CONSTRAINT lrc;
  KHE_RESOURCE_GROUP rg;  int rg_count, task_count, max_limit, min_limit;
  KHE_RESOURCE_TYPE rt;  KHE_EVENT_RESOURCE er;
  KHE_ASSIGN_RESOURCE_MONITOR arm;  KHE_SOLN soln;
  KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR asam;
  soln = KheMonitorSoln(m);
  switch( KheMonitorTag(m) )
  {
    case KHE_ASSIGN_RESOURCE_MONITOR_TAG:

      arm = (KHE_ASSIGN_RESOURCE_MONITOR) m;
      er = KheAssignResourceMonitorEventResource(arm);
      return KheAssignResourceMonitorClassify(m,
        KheEventResourceTaskCount(soln, er), durn, non_asst_cost, asst_cost);

    case KHE_PREFER_RESOURCES_MONITOR_TAG:

      prm = (KHE_PREFER_RESOURCES_MONITOR) m;
      rg = KhePreferResourcesMonitorDomain(prm);
      rg_count = KheResourceGroupResourceCount(rg);
      rt = KheResourceGroupResourceType(rg);
      er = KhePreferResourcesMonitorEventResource(prm);
      return KhePreferResourcesMonitorClassify(m, rt, rg_count,
	KheEventResourceTaskCount(soln, er), durn, non_asst_cost, asst_cost);

    case KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG:

      asam = (KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR) m;
      if( KheAvoidSplitAssignmentsMonitorTasksHaveSameProperRoot(asam, soln) )
      {
	/* no cost is possible, ignore this monitor */
	return KHE_MONITOR_IGNORE;
      }
      else
      {
	/* inseparable */
	return KHE_MONITOR_INSEPARABLE;
      }

    case KHE_LIMIT_RESOURCES_MONITOR_TAG:

      lrm = (KHE_LIMIT_RESOURCES_MONITOR) m;
      lrc = KheLimitResourcesMonitorConstraint(lrm);
      rg = KheLimitResourcesConstraintDomain(lrc);
      rg_count = KheResourceGroupResourceCount(rg);
      rt = KheResourceGroupResourceType(rg);
      max_limit = KheLimitResourcesConstraintMaximum(lrc);
      min_limit = KheLimitResourcesConstraintMinimum(lrc);
      task_count = KheLimitResourcesMonitorTaskCount(lrm, soln);
      /* *** what this case asserts is not true!
      if( rg_count == 0 )
      {
	** no cost is possible, ignore this monitor **
	return KHE_MONITOR_IGNORE;
      }
      else
      *** */
      if( KheLimitResourcesMonitorMimicsAssignResourceMonitor(lrm) )
      {
	/* classify as an assign resource monitor would be classifed */
	return KheAssignResourceMonitorClassify(m, task_count, durn,
	  non_asst_cost, asst_cost);
      }
      else if( KheLimitResourcesMonitorMimicsPreferResourcesMonitor(lrm) )
      {
	/* classify as a prefer resources monitor would be classifed */
	return KhePreferResourcesMonitorClassify(m, rt,
	  KheResourceTypeResourceCount(rt) - rg_count,  /* size of complement */
          task_count, durn, non_asst_cost, asst_cost);
      }
      else if( KheMonitorCostFunction(m) == KHE_LINEAR_COST_FUNCTION
	  && max_limit == 0 )
      {
	/* separable */
	if( rg_count == KheResourceTypeResourceCount(rt) )
	{
	  /* separable resource-independent, with assignment cost */
	  *asst_cost += KheMonitorDevToCost(m, durn);
	  return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* separable resource-dependent */
	  return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
	}
      }
      else if( task_count == 1 )
      {
	/* separable */
	if( rg_count == KheResourceTypeResourceCount(rt) )
	{
	  /* separable resource-independent, with a messy cost calculation */
	  /* which reduces to the previous one when max_limit == 0 */
	  if( min_limit > 0 )
	    *non_asst_cost += KheMonitorDevToCost(m, min_limit);
	  if( durn < min_limit )
	    *asst_cost += KheMonitorDevToCost(m, min_limit - durn);
	  else if( durn > max_limit )
	    *asst_cost += KheMonitorDevToCost(m, durn - max_limit);
	  return KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT;
	}
	else
	{
	  /* separable resource-dependent */
	  return KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT;
	}
      }
      else
      {
	/* inseparable */
	return KHE_MONITOR_INSEPARABLE;
      }

    default:

      HnAbort("KheMonitorKind: unexpected monitor tag (%s)",
	KheMonitorTagShow(KheMonitorTag(m)));
      return KHE_MONITOR_INSEPARABLE;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig,                */
/*    KHE_MONITOR m, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Add m to asig, or add its costs to *non_asst_cost and *asst_cost.        */
/*                                                                           */
/*****************************************************************************/

static void KheAtomicTaskSigAddMonitor(KHE_ATOMIC_TASK_SIG asig,
  KHE_MONITOR m, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  switch( KheMonitorKind(m, asig->duration, non_asst_cost, asst_cost) )
  {
    case KHE_MONITOR_IGNORE:

      /* nothing to do; we're ignoring this monitor */
      break;

    case KHE_MONITOR_INSEPARABLE:

      HaArrayAddLast(asig->inseparable_monitors, m);
      break;

    case KHE_MONITOR_SEPARABLE_RESOURCE_INDEPENDENT:

      /* nothing to do; costs already added */
      break;

    case KHE_MONITOR_SEPARABLE_RESOURCE_DEPENDENT:

      HaArrayAddLast(asig->separable_rd_monitors, m);
      break;

    default:

      HnAbort("KheAtomicTaskSigAddMonitor: illegal monitor kind");
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInseparableMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)        */
/*                                                                           */
/*  Typed comparison function for sorting an array of monitors into the      */
/*  order that inseparable monitors need to be sorted into.                  */
/*                                                                           */
/*  Implementation note.  The inseparable monitors of two atomic task        */
/*  signatures need to be the same monitors for similarity, so it is         */
/*  sufficient here to sort them by their unique index numbers.              */
/*                                                                           */
/*****************************************************************************/

static int KheInseparableMonitorTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  return KheMonitorSolnIndex(m1) - KheMonitorSolnIndex(m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheInseparableMonitorUntypedCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Untyped comparison function for sorting an array of monitors into the    */
/*  order that inseparable monitors need to be sorted into.                  */
/*                                                                           */
/*****************************************************************************/

static int KheInseparableMonitorUntypedCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheInseparableMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSeparableResourceDependentMonitorTypedCmp(KHE_MONITOR m1,         */
/*    KHE_MONITOR m2)                                                        */
/*                                                                           */
/*  Typed comparison function for sorting an array of separable resource-    */
/*  dependent monitors into the order that they monitors need to be in.      */
/*                                                                           */
/*  Implementation note.  The resource-dependent separable monitors of       */
/*  two atomic task signatures need to have equal attributes for similarity, */
/*  so they have to be sorted by those attributes.                           */
/*                                                                           */
/*****************************************************************************/

static int KheSeparableResourceDependentMonitorTypedCmp(KHE_MONITOR m1,
  KHE_MONITOR m2)
{
  KHE_RESOURCE_GROUP rg1, rg2;  KHE_PREFER_RESOURCES_MONITOR prm1, prm2;
  KHE_LIMIT_RESOURCES_CONSTRAINT lrc1, lrc2;  int cmp;
  KHE_LIMIT_RESOURCES_MONITOR lrm1, lrm2;

  /* shortcut when m1 and m2 are the same monitor */
  if( m1 == m2 )
    return 0;

  /* monitor type attributes must be equal */
  cmp = KheMonitorTag(m1) - KheMonitorTag(m2);
  if( cmp != 0 ) return cmp;

  /* hardness, weight, and cost function attributes must be equal */
  /* c1 = KheMo nitorConstraint(m1); */
  /* c2 = KheM onitorConstraint(m2); */
  cmp = KheCostCmp(KheMonitorCombinedWeight(m1), KheMonitorCombinedWeight(m2));
  if( cmp != 0 ) return cmp;
  cmp = (int) KheMonitorCostFunction(m1) - (int) KheMonitorCostFunction(m2);
  if( cmp != 0 ) return cmp;

  switch( KheMonitorTag(m1) )
  {
    case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

      /* preferred resources must be equal */
      prm1 = (KHE_PREFER_RESOURCES_MONITOR) m1;
      prm2 = (KHE_PREFER_RESOURCES_MONITOR) m2;
      rg1 = KhePreferResourcesMonitorDomain(prm1);
      rg2 = KhePreferResourcesMonitorDomain(prm2);
      return KheResourceGroupTypedCmp(rg1, rg2);

    case KHE_LIMIT_RESOURCES_CONSTRAINT_TAG:

      /* min limit, max limit, and preferred resources must be equal */
      lrm1 = (KHE_LIMIT_RESOURCES_MONITOR) m1;
      lrm2 = (KHE_LIMIT_RESOURCES_MONITOR) m2;
      lrc1 = KheLimitResourcesMonitorConstraint(lrm1);
      lrc2 = KheLimitResourcesMonitorConstraint(lrm2);
      cmp = KheLimitResourcesConstraintMinimum(lrc1) -
	KheLimitResourcesConstraintMinimum(lrc2);
      if( cmp != 0 ) return cmp;
      cmp = KheLimitResourcesConstraintMaximum(lrc1) -
	KheLimitResourcesConstraintMaximum(lrc2);
      if( cmp != 0 ) return cmp;
      rg1 = KheLimitResourcesConstraintDomain(lrc1);
      rg2 = KheLimitResourcesConstraintDomain(lrc2);
      return KheResourceGroupTypedCmp(rg1, rg2);

    default:

      HnAbort("KheSeparableResourceDependentMonitorTypedCmp internal error: "
	"unexpected constraint tag (%d)", KheMonitorTag(m1));
      return 0;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSeparableResourceDependentMonitorUntypedCmp(const void *t1,       */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Untyped comparison function for sorting an array of separable resource-  */
/*  dependent monitors into the order that they need to be sorted into.      */
/*                                                                           */
/*****************************************************************************/

static int KheSeparableResourceDependentMonitorUntypedCmp(const void *t1,
  const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheSeparableResourceDependentMonitorTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_MTASK_FINDER mtf,           */
/*    KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Assuming that task lies in a meet, make and return a new atomic          */
/*  signature object for task.  Also add in task's costs.                    */
/*                                                                           */
/*  NB There is no KheAtomicTaskSigDelete, because KheRootTaskSigDelete      */
/*  uses HaArrayAppend to free all its atomic task sigs at once.             */
/*                                                                           */
/*****************************************************************************/

static KHE_ATOMIC_TASK_SIG KheAtomicTaskSigMake(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_ATOMIC_TASK_SIG res;  KHE_MEET meet;  KHE_EVENT_RESOURCE er;
  KHE_MONITOR m;  int i;  KHE_TIME first_time;

  /* get the object */
  if( HaArrayCount(mtf->free_atomic_task_sigs) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_atomic_task_sigs);
    HaArrayClear(res->separable_rd_monitors);
    HaArrayClear(res->inseparable_monitors);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->separable_rd_monitors, mtf->arena);
    HaArrayInit(res->inseparable_monitors, mtf->arena);
  }

  /* initialize first_time or meet, duration, and workload per time */
  meet = KheTaskMeet(task);
  first_time = KheMeetAsstTime(meet);
  if( mtf->fixed_times && first_time != NULL )
  {
    res->fixed_times = true;
    res->u.first_time = first_time;
  }
  else
  {
    res->fixed_times = false;
    res->u.encl_meet = meet;
  }
  res->duration = KheMeetDuration(meet);
  res->workload_per_time = KheTaskWorkloadPerTime(task);

  /* initialize monitors */
  er = KheTaskEventResource(task);
  for( i = 0;  i < KheSolnEventResourceMonitorCount(mtf->soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(mtf->soln, er, i);
    if( KheMonitorAttachedToSoln(m) && KheMonitorCombinedWeight(m) > 0 )
      KheAtomicTaskSigAddMonitor(res, m, non_asst_cost, asst_cost);
  }

  /* sort monitors */
  HaArraySort(res->separable_rd_monitors,
    &KheSeparableResourceDependentMonitorUntypedCmp);
  HaArraySort(res->inseparable_monitors, &KheInseparableMonitorUntypedCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,                  */
/*    KHE_ATOMIC_TASK_SIG asig2)                                             */
/*                                                                           */
/*  Typed comparison function for sorting an array of atomic task sigs by    */
/*  increasing start time or meet.  Sigs with a start time come first.       */
/*                                                                           */
/*****************************************************************************/

static int KheAtomicTaskSigTypedCmp(KHE_ATOMIC_TASK_SIG asig1,
  KHE_ATOMIC_TASK_SIG asig2)
{
  int cmp;
  if( asig1->fixed_times )
  {
    if( asig2->fixed_times )
    {
      cmp = KheTimeIndex(asig1->u.first_time)-KheTimeIndex(asig2->u.first_time);
      if( cmp != 0 )  return cmp;
      return asig1->duration - asig2->duration;
    }
    else
      return -1;
  }
  else
  {
    if( asig2->fixed_times )
      return 1;
    else
      return KheMeetSolnIndex(asig1->u.encl_meet) -
	KheMeetSolnIndex(asig2->u.encl_meet);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheAtomicTaskSigUntypedCmp(const void *t1, const void *t2)           */
/*                                                                           */
/*  Untyped comparison function for sorting an array of atomic task sigs     */
/*  by increasing start time, breaking ties by increasing duration.          */
/*                                                                           */
/*****************************************************************************/

static int KheAtomicTaskSigUntypedCmp(const void *t1, const void *t2)
{
  KHE_ATOMIC_TASK_SIG asig1 = * (KHE_ATOMIC_TASK_SIG *) t1;
  KHE_ATOMIC_TASK_SIG asig2 = * (KHE_ATOMIC_TASK_SIG *) t2;
  return KheAtomicTaskSigTypedCmp(asig1, asig2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheAtomicTaskSigSimilar(KHE_ATOMIC_TASK_SIG asig1,                  */
/*    KHE_ATOMIC_TASK_SIG asig2)                                             */
/*                                                                           */
/*  Return true if asig1 and asig2 are similar.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheAtomicTaskSigSimilar(KHE_ATOMIC_TASK_SIG asig1,
  KHE_ATOMIC_TASK_SIG asig2)
{
  int i, count;  KHE_MONITOR m1, m2;

  /* first_time (or meet), duration, and workload_per_time must be equal */
  if( asig1->fixed_times != asig2->fixed_times )
    return false;
  if( asig1->fixed_times )
  {
    if( asig1->u.first_time != asig2->u.first_time )
      return false;
  }
  else
  {
    if( asig1->u.encl_meet != asig2->u.encl_meet )
      return false;
  }
  if( asig1->duration != asig2->duration )
    return false;
  if( asig1->workload_per_time != asig2->workload_per_time )
    return false;

  /* corresponding separable rd monitors must have the same attributes */
  count = HaArrayCount(asig1->separable_rd_monitors);
  if( count != HaArrayCount(asig2->separable_rd_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->separable_rd_monitors, i);
    m2 = HaArray(asig2->separable_rd_monitors, i);
    if( KheSeparableResourceDependentMonitorTypedCmp(m1, m2) != 0 )
      return false;
  }

  /* corresponding inseparable monitors must be identical */
  count = HaArrayCount(asig1->inseparable_monitors);
  if( count != HaArrayCount(asig2->inseparable_monitors) )
    return false;
  for( i = 0;  i < count;  i++ )
  {
    m1 = HaArray(asig1->inseparable_monitors, i);
    m2 = HaArray(asig2->inseparable_monitors, i);
    if( m1 != m2 )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAtomicTaskSigDebug(KHE_ATOMIC_TASK_SIG asig, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of asig onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheAtomicTaskSigDebug(KHE_ATOMIC_TASK_SIG asig, int verbosity,
  int indent, FILE *fp)
{
  KHE_MONITOR m;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ AtomicTaskSig(%s, durn %d, wkld %.2f)\n", indent, "",
      asig->fixed_times ?  KheTimeId(asig->u.first_time) :
      KheMeetId(asig->u.encl_meet), asig->duration, asig->workload_per_time);
    if( HaArrayCount(asig->separable_rd_monitors) > 0 )
    {
      fprintf(fp, "%*s  separable rd monitors:\n", indent, "");
      HaArrayForEach(asig->separable_rd_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    if( HaArrayCount(asig->inseparable_monitors) > 0 )
    {
      fprintf(fp, "%*s  inseparable monitors:\n", indent, "");
      HaArrayForEach(asig->inseparable_monitors, m, i)
	fprintf(fp, "%*s    %s\n", indent, "", KheMonitorId(m));
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "AtomicTaskSig(%s, %d, %.2f, %d sep rd, %d insep)",
      asig->fixed_times ?  KheTimeId(asig->u.first_time) :
      KheMeetId(asig->u.encl_meet), asig->duration, asig->workload_per_time,
      HaArrayCount(asig->separable_rd_monitors),
      HaArrayCount(asig->inseparable_monitors));
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_ROOT_TASK_SIG"                                            */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigAddTaskAndDescendants(KHE_ROOT_TASK_SIG sig,          */
/*    KHE_TASK task, KHE_MTASK_FINDER mtf, KHE_COST *non_asst_cost,          */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Add atomic signatures for task and its descendants to sig.  Also add     */
/*  in the costs for task and its descendants.                               */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigAddTaskAndDescendants(KHE_ROOT_TASK_SIG sig,
  KHE_TASK task, KHE_MTASK_FINDER mtf, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  int i;  KHE_TASK child_task;  KHE_ATOMIC_TASK_SIG atsig;

  /* do it for task */
  if( KheTaskMeet(task) != NULL && KheTaskEventResource(task) != NULL )
  {
    atsig = KheAtomicTaskSigMake(mtf, task, non_asst_cost, asst_cost);
    HaArrayAddLast(sig->atomic_sigs, atsig);
    sig->total_duration += atsig->duration;
    sig->total_workload += atsig->duration * atsig->workload_per_time;
  }

  /* do it for task's descendants */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheRootTaskSigAddTaskAndDescendants(sig, child_task, mtf, non_asst_cost,
      asst_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_ROOT_TASK_SIG KheRootTaskSigMake(KHE_MTASK_FINDER mtf,               */
/*    KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)           */
/*                                                                           */
/*  Make and return the root task signature of task, a proper root task.     */
/*  Also set its costs.                                                      */
/*                                                                           */
/*  The result can be degenerate (no atomic signatures) if none of the       */
/*  tasks assigned to task directly or indirectly is derived from a meet.    */
/*                                                                           */
/*****************************************************************************/

static KHE_ROOT_TASK_SIG KheRootTaskSigMake(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_COST *non_asst_cost, KHE_COST *asst_cost)
{
  KHE_ROOT_TASK_SIG res;  KHE_RESOURCE r;

  /* get memory for the new object */
  if( HaArrayCount(mtf->free_root_task_sigs) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_root_task_sigs);
    HaArrayClear(res->atomic_sigs);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->atomic_sigs, mtf->arena);
  }

  /* set its domain, preasst, and is_fixed */
  res->root_domain = KheTaskDomain(task);
  res->root_preasst = (KheTaskIsPreassigned(task, &r) ? r : NULL);
  res->root_asst_is_fixed = KheTaskAssignIsFixed(task);

  /* get the atomic sigs (possibly none), and sort them */
  res->total_duration = 0;
  res->total_workload = 0.0;
  *non_asst_cost = *asst_cost = 0;
  KheRootTaskSigAddTaskAndDescendants(res, task, mtf, non_asst_cost, asst_cost);
  HaArraySort(res->atomic_sigs, &KheAtomicTaskSigUntypedCmp);

  /* all good */
  return res;
}

/* *** old version that rejected degenerate tasks
static bool KheRootTaskSigMake(KHE_MTASK_FINDER mtf, KHE_TASK task,
  KHE_ROOT_TASK_SIG *sig, KHE_COST *asst_cost, KHE_COST *n on_asst_cost)
{
  KHE_ROOT_TASK_SIG res;  KHE_RESOURCE r;

  ** get memory for the new object **
  if( HaArrayCount(mtf->free_root_task_sigs) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_root_task_sigs);
    HaArrayClear(res->atomic_sigs);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->atomic_sigs, mtf->arena);
  }

  ** set its domain, preasst, and is_fixed **
  res->root_domain = KheTaskDomain(task);
  if( KheTaskIsPreassigned(task, &r) )
    res->root_preasst = r;
  else
    res->root_preasst = NULL;
  res->root_asst_is_fixed = KheTaskAssignIsFixed(task);

  ** get the atomic sigs, and sort them **
  res->total_duration = 0;
  res->total_workload = 0.0;
  *asst_cost = *n on_asst_cost = 0;
  KheRootTaskSigAddTaskAndDescendants(res, task, mtf, asst_cost,n on_asst_cost);
  HaArraySort(res->atomic_sigs, &KheAtomicTaskSigUntypedCmp);

  return *sig = res, HaArrayCount(res->atomic_sigs) > 0;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDelete(KHE_ROOT_TASK_SIG sig, KHE_MTASK_FINDER mtf)   */
/*                                                                           */
/*  Delete sig, by adding it to the free list in mtf.                        */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDelete(KHE_ROOT_TASK_SIG sig, KHE_MTASK_FINDER mtf)
{
  int i;
  HaArrayAppend(mtf->free_atomic_task_sigs, sig->atomic_sigs, i);
  HaArrayAddLast(mtf->free_root_task_sigs, sig);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigIsNonDegenerate(KHE_ROOT_TASK_SIG sig)                */
/*                                                                           */
/*  Return true if sig is non-degenerate (if it has atomic sigs).            */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigIsNonDegenerate(KHE_ROOT_TASK_SIG sig)
{
  return HaArrayCount(sig->atomic_sigs) > 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigFixedTimes(KHE_ROOT_TASK_SIG sig, int *sort_index)    */
/*                                                                           */
/*  Assuming that sig is non-degenerate, return true if it has fixed times,  */
/*  and also return its sort index.                                          */
/*                                                                           */
/*  Implementation note.  A root task sig has fixed times if its first       */
/*  atomic signature has fixed times, which is equivalent to saying that     */
/*  all of its atomic signatures have fixed times.                           */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigFixedTimes(KHE_ROOT_TASK_SIG sig, int *sort_index)
{
  KHE_ATOMIC_TASK_SIG asig;
  HnAssert(KheRootTaskSigIsNonDegenerate(sig),
    "KheRootTaskSigFixedTimes internal error:  no atomic sigs");
  asig = HaArrayFirst(sig->atomic_sigs);
  if( asig->fixed_times )
    *sort_index = KheTimeIndex(asig->u.first_time);
  else
    *sort_index = KheMeetSolnIndex(asig->u.encl_meet);
  return asig->fixed_times;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRootTaskSigSimilar(KHE_ROOT_TASK_SIG sig1,                       */
/*    KHE_ROOT_TASK_SIG sig2)                                                */
/*                                                                           */
/*  Return true if sig1 and sig2 are similar.                                */
/*                                                                           */
/*****************************************************************************/

static bool KheRootTaskSigSimilar(KHE_ROOT_TASK_SIG sig1,
  KHE_ROOT_TASK_SIG sig2)
{
  int i;  KHE_ATOMIC_TASK_SIG asig1, asig2;

  /* domains and preassts must be equal */
  if( !KheResourceGroupEqual(sig1->root_domain, sig2->root_domain) )
    return false;
  if( sig1->root_preasst != sig2->root_preasst )
    return false;

  /* neither task may have a fixed asst */
  if( sig1->root_asst_is_fixed || sig2->root_asst_is_fixed )
    return false;

  /* same number of atomic sigs, and they must be similar */
  if( HaArrayCount(sig1->atomic_sigs) != HaArrayCount(sig2->atomic_sigs) )
    return false;
  for( i = 0;  i < HaArrayCount(sig1->atomic_sigs);  i++ )
  {
    asig1 = HaArray(sig1->atomic_sigs, i);
    asig2 = HaArray(sig2->atomic_sigs, i);
    if( !KheAtomicTaskSigSimilar(asig1, asig2) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheRootTaskSigStartTime(KHE_ROOT_TASK_SIG sig)                  */
/*                                                                           */
/*  Return the start time of sig, taken from its first atomic sig, or        */
/*  NULL if there is no assigned time.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static KHE_TIME KheRootTaskSigStartTime(KHE_ROOT_TASK_SIG sig)
{
  KHE_ATOMIC_TASK_SIG asig;
  asig = HaArrayFirst(sig->atomic_sigs);
  if( asig->sort_type == KHE_SORT_TYPE_FIRST_TIME )
    return asig->u.first_time;
  else
    return NULL;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDebugHeader(KHE_ROOT_TASK_SIG sig, FILE *fp)          */
/*                                                                           */
/*  Debug print of the header part of sig.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDebugHeader(KHE_ROOT_TASK_SIG sig, FILE *fp)
{
  fprintf(fp, "TaskSig(");
  KheResourceGroupDebug(sig->root_domain, 1, -1, fp);
  if( sig->root_preasst != NULL )
    fprintf(fp, ", preasst %s", KheResourceId(sig->root_preasst));
  if( sig->root_asst_is_fixed )
    fprintf(fp, ", fixed");
  fprintf(fp, ", %d atomic sigs)", HaArrayCount(sig->atomic_sigs));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRootTaskSigDebug(KHE_ROOT_TASK_SIG sig, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of the sig onto fp with the given verbosity and indent.      */
/*                                                                           */
/*****************************************************************************/

static void KheRootTaskSigDebug(KHE_ROOT_TASK_SIG sig, int verbosity,
  int indent, FILE *fp)
{
  KHE_ATOMIC_TASK_SIG asig;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheRootTaskSigDebugHeader(sig, fp);
    fprintf(fp, "\n");
    HaArrayForEach(sig->atomic_sigs, asig, i)
      KheAtomicTaskSigDebug(asig, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheRootTaskSigDebugHeader(sig, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_TIME"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_TIME KheMTaskTimeMake(KHE_TIME time,                           */
/*    float workload_per_time, KHE_MTASK_FINDER mtf)                         */
/*                                                                           */
/*  Make a new mtask time object with these attributes.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK_TIME KheMTaskTimeMake(KHE_TIME time,
  float workload_per_time, KHE_MTASK_FINDER mtf)
{
  KHE_MTASK_TIME res;
  /* ***
  if( HaArrayCount(mtf->free_mtask_times) > 0 )
    res = HaArrayLastAndDelete(mtf->free_mtask_times);
  else
  *** */
  HaMake(res, mtf->arena);
  res->time = time;
  res->workload_per_time = workload_per_time;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK"                                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskMake(KHE_MTASK_FINDER mtf, KHE_ROOT_TASK_SIG sig)      */
/*                                                                           */
/*  Make a new mtask object, suitable for holding tasks with signature       */
/*  sig, but initially empty.  This will always be followed by adding        */
/*  a first task to the result, so in fact mtasks are never empty.           */
/*                                                                           */
/*****************************************************************************/

static KHE_MTASK KheMTaskMake(KHE_MTASK_FINDER mtf, KHE_ROOT_TASK_SIG sig)
{
  KHE_MTASK res;  KHE_ATOMIC_TASK_SIG asig;  KHE_MTASK_TIME ct;
  int i, j, bi, sort_index, first_day_index;  KHE_TIME busy_time;

  /* get the basic object and initialize its arrays */
  /* ***
  if( HaArrayCount(mtf->free_mtasks) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_mtasks);
    HaArrayClear(res->tasks_and_costs);
    HaArrayClear(res->fixed_times_by_day);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->tasks_and_costs, mtf->arena);
    HaArrayInit(res->fixed_times_by_day, mtf->arena);
  }
  *** */

  /* get the basic object and initialize its arrays */
  HaMake(res, mtf->arena);
  HaArrayInit(res->tasks_and_costs, mtf->arena);
  HaArrayInit(res->fixed_times_by_day, mtf->arena);

  /* initialize basic fields */
  res->mtask_finder = mtf;
  res->sig = sig;
  res->asst_count = 0;

  /* set fixed_times (includes non-degeneracy) */
  res->fixed_times = KheRootTaskSigIsNonDegenerate(sig) &&
    KheRootTaskSigFixedTimes(sig, &sort_index);

  /* if fixed_times, set the other fields related to assigned times */
  if( res->fixed_times )
  {
    /* initialize fixed_times_first_day_index from first atomic sig */
    asig = HaArrayFirst(sig->atomic_sigs);
    first_day_index = KheFrameTimeIndex(mtf->days_frame, asig->u.first_time);
    /* ***
    res->fixed_times_first_day_index =
      KheFrameTimeIndex(mtf->days_frame, asig->u.first_time);
    *** */

    /* initialize fixed_times_by_day and fixed_times_no_overlap */
    res->fixed_times_no_overlap = true;
    HaArrayForEach(sig->atomic_sigs, asig, i)
    {
      for( j = 0;  j < asig->duration;  j++ )
      {
	busy_time = KheTimeNeighbour(asig->u.first_time, j);
	bi = KheFrameTimeIndex(mtf->days_frame, busy_time) - first_day_index;
	HnAssert(bi >= 0, "KheMTaskMake internal error (bi < 0)");
	HaArrayFill(res->fixed_times_by_day, bi + 1, NULL);
	if( HaArray(res->fixed_times_by_day, bi) == NULL )
	{
	  ct = KheMTaskTimeMake(busy_time, asig->workload_per_time, mtf);
	  HaArrayPut(res->fixed_times_by_day, bi, ct);
	}
	else
	  res->fixed_times_no_overlap = false;
      }
    }

    /* initialize fixed_times_interval from fixed_times_by_day */
    res->fixed_times_interval = KheIntervalMake(first_day_index,
      first_day_index + HaArrayCount(res->fixed_times_by_day) - 1);

    /* initialize fixed_times_no_gaps */
    res->fixed_times_no_gaps = true;
    HaArrayForEach(res->fixed_times_by_day, ct, bi)
      if( ct == NULL )
	res->fixed_times_no_gaps = false;
  }
  else
  {
    /* initialize no_overlap etc. when not res->fixed_times */
    res->fixed_times_no_overlap = false;
    res->fixed_times_no_gaps = false;
    res->fixed_times_interval = KheIntervalMake(0, -1);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  char *KheMTaskId(KHE_MTASK mt)                                           */
/*                                                                           */
/*  Return an Id for mt.                                                     */
/*                                                                           */
/*****************************************************************************/

char *KheMTaskId(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;
  tac = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskId(tac->task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TYPE KheMTaskResourceType(KHE_MTASK mt)                     */
/*                                                                           */
/*  Return the resource type of mt.                                          */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_TYPE KheMTaskResourceType(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;
  tac = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskResourceType(tac->task);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskIsPreassigned(KHE_MTASK mt, KHE_RESOURCE *r)                */
/*                                                                           */
/*  If mt is preassigned, return true with *r set to the resource.           */
/*  Otherwise return false with *r set to NULL.                              */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskIsPreassigned(KHE_MTASK mt, KHE_RESOURCE *r)
{
  *r = mt->sig->root_preasst;
  return (*r != NULL);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAssignIsFixed(KHE_MTASK mt)                                 */
/*                                                                           */
/*  If the assignments of the tasks of mt are fixed, return true, else false.*/
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAssignIsFixed(KHE_MTASK mt)
{
  return mt->sig->root_asst_is_fixed;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheMTaskDomain(KHE_MTASK mt)                          */
/*                                                                           */
/*  Return the domain shared by the tasks of mt.                             */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheMTaskDomain(KHE_MTASK mt)
{
  return mt->sig->root_domain;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTotalDuration(KHE_MTASK mt)                                  */
/*                                                                           */
/*  Return the total duration of mt.                                         */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTotalDuration(KHE_MTASK mt)
{
  return mt->sig->total_duration;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheMTaskTotalWorkload(KHE_MTASK mt)                                */
/*                                                                           */
/*  Return the total workload of mt.                                         */
/*                                                                           */
/*****************************************************************************/

float KheMTaskTotalWorkload(KHE_MTASK mt)
{
  return mt->sig->total_workload;
}


/*****************************************************************************/
/*                                                                           */
/*  float KheMTaskWorkloadPerTime(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the workload per time of the tasks of mt.  If the tasks of mt     */
/*  are grouped and the atomic tasks have different workloads per time,      */
/*  this will be a suitable averagle of those workloads per time, but that   */
/*  is probably not a problem.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** decided against doing it this way
float KheMTaskWorkloadPerTime(KHE_MTASK mt)
{
  return KheMTaskTotalWorkload(mt) / (float) KheMTaskTotalDuration(mt);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskHasSoleMeet(KHE_MTASK mt, KHE_MEET *meet)                   */
/*                                                                           */
/*  If the tasks of mt all come from the same meet, set *meet to that meet   */
/*  and return true.  Otherwise set *meet to NULL and return false.          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskHasSoleMeet(KHE_MTASK mt, KHE_MEET *meet)
{
  KHE_TASK_AND_COST tc;  int i;  KHE_MEET mt_meet;
  tc = HaArrayFirst(mt->tasks_and_costs);
  mt_meet = KheTaskMeet(tc->task);
  for( i = 1;  i < HaArrayCount(mt->tasks_and_costs);  i++ )
  {
    tc = HaArray(mt->tasks_and_costs, i);
    if( mt_meet != KheTaskMeet(tc->task) )
      return *meet = NULL, false;
  }
  return *meet = mt_meet, true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskHasFixedTimes(KHE_MTASK mt)                                 */
/*                                                                           */
/*  Return true if mt has fixed times, including non-degeneracy.             */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskHasFixedTimes(KHE_MTASK mt)
{
  return mt->fixed_times;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskInterval(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the interval covered by mt.                                       */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheMTaskInterval(KHE_MTASK mt)
{
  return mt->fixed_times_interval;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskFirstDayIndex(KHE_MTASK mt)                                  */
/*                                                                           */
/*  Return the day index of the first time of mt.                            */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
int KheMTaskFirstDayIndex(KHE_MTASK mt)
{
  return mt->fixed_times_first_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskLastDayIndex(KHE_MTASK mt)                                   */
/*                                                                           */
/*  Return the day index of the last time of mt.                             */
/*                                                                           */
/*****************************************************************************/

/* *** withdrawn
int KheMTaskLastDayIndex(KHE_MTASK mt)
{
  return mt->fixed_times_last_day_index;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TIME KheMTaskDayTime(KHE_MTASK mt, int day_index,                    */
/*    float *workload_per_time)                                              */
/*                                                                           */
/*  Return one time (the only one if KheMTaskNoOverlap returns true)         */
/*  when mt is busy on day day_index.  Also return the workload per time     */
/*  then.  If mt is not busy on day_index, return NULL (0 for workload).     */
/*                                                                           */
/*****************************************************************************/

KHE_TIME KheMTaskDayTime(KHE_MTASK mt, int day_index, float *workload_per_time)
{
  KHE_MTASK_TIME ct;
  /* ***
  if( day_index < mt->fixed_times_first_day_index ||
      day_index > mt->fixed_times_last_day_index )
  *** */
  if( !KheIntervalContains(mt->fixed_times_interval, day_index) )
    return *workload_per_time = 0, NULL;
  ct = HaArray(mt->fixed_times_by_day,
    day_index - KheIntervalFirst(mt->fixed_times_interval));
  if( ct == NULL )
    return *workload_per_time = 0, NULL;
  else
    return *workload_per_time = ct->workload_per_time, ct->time;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNoOverlap(KHE_MTASK mt)                                     */
/*                                                                           */
/*  Return true if there are no cases where two of the shared times of       */
/*  the tasks of mt overlap with a single day of days_frame.                 */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNoOverlap(KHE_MTASK mt)
{
  return mt->fixed_times_no_overlap;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNoGaps(KHE_MTASK mt)                                        */
/*                                                                           */
/*  Return true when there are no gaps in the days covered by the tasks      */
/*  of mt.                                                                   */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNoGaps(KHE_MTASK mt)
{
  return mt->fixed_times_no_gaps;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTaskCount(KHE_MTASK mt)                                      */
/*                                                                           */
/*  Return the number of tasks in mt.                                        */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTaskCount(KHE_MTASK mt)
{
  return HaArrayCount(mt->tasks_and_costs);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheMTaskTask(KHE_MTASK mt, int i, KHE_COST *non_asst_cost,      */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Return the i'th task of mt.                                              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheMTaskTask(KHE_MTASK mt, int i, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  KHE_TASK_AND_COST tac;
  tac = HaArray(mt->tasks_and_costs, i);
  *non_asst_cost = tac->non_asst_cost;
  *asst_cost = tac->asst_cost;
  return tac->task;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskDoNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost, */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Carry out the actual work of KheTaskNonAsstAndAsstCost.                  */
/*                                                                           */
/*  Implementation note.  This function calls KheMonitorKind, making it      */
/*  consistent with other calculations of non-assignment and assignment      */
/*  cost in this module.  However it does not use the KHE_MONITOR_KIND       */
/*  value returned by KheMonitorKind.                                        */
/*                                                                           */
/*****************************************************************************/

void KheTaskDoNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  KHE_TASK child_task;  int i, durn;  KHE_SOLN soln;  KHE_MONITOR m;
  KHE_EVENT_RESOURCE er;

  /* do it for task itself */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  durn = KheTaskDuration(task);
  for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
  {
    m = KheSolnEventResourceMonitor(soln, er, i);
    if( KheMonitorAttachedToSoln(m) && KheMonitorCombinedWeight(m) > 0 )
      KheMonitorKind(m, durn, non_asst_cost, asst_cost);
  }

  /* do it for the tasks assigned to task */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    KheTaskDoNonAsstAndAsstCost(child_task, non_asst_cost, asst_cost);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,   */
/*    KHE_COST *asst_cost)                                                   */
/*                                                                           */
/*  Find the non-assignment and assignment cost of task, including all       */
/*  tasks assigned directly or indirectly to task.                           */
/*                                                                           */
/*****************************************************************************/

void KheTaskNonAsstAndAsstCost(KHE_TASK task, KHE_COST *non_asst_cost,
  KHE_COST *asst_cost)
{
  *non_asst_cost = *asst_cost = 0;
  KheTaskDoNonAsstAndAsstCost(task, non_asst_cost, asst_cost);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskBringAsstCountUpToDate(KHE_MTASK mt)                        */
/*                                                                           */
/*  Bring the asst_count field of mt up to date.  It can get out of date     */
/*  when marks and paths are used to undo operations on mt's tasks.          */
/*                                                                           */
/*  As usual, mt->asst_count is the number of tasks with a resource          */
/*  assigned to them, or equivalently the index in mt->tasks_and_costs       */
/*  of the first unassigned task.  Even when mt->asst_count is out of        */
/*  date, we still assume that all the assigned tasks are at the front.      */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskBringAsstCountUpToDate(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tac;

  /* decrease mt->asst_count while it is too large */
  while( mt->asst_count > 0 )
  {
    tac = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
    if( KheTaskAsstResource(tac->task) != NULL )
      break;
    mt->asst_count--;
  }

  /* increase mt->asst_count while it is too small */
  while( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
  {
    tac = HaArray(mt->tasks_and_costs, mt->asst_count);
    if( KheTaskAsstResource(tac->task) == NULL )
      break;
    mt->asst_count++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFindTask(KHE_MTASK mt, KHE_RESOURCE r, KHE_TASK *task)      */
/*                                                                           */
/*  If mt contains a task assigned r (possibly NULL), return true with       */
/*  *task set to its first such task.  Otherwise return false with *task     */
/*  set to NULL.                                                             */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskFindTask(KHE_MTASK mt, KHE_RESOURCE r, KHE_TASK *task,
  int *pos)
{
  int i;  KHE_TASK_AND_COST tac;
  KheMTaskBringAsstCountUpToDate(mt);
  if( r != NULL )
  {
    /* non-NULL resource must be in first mt->asst_count entries */
    /* searching backwards is likely to be faster */
    for( i = mt->asst_count - 1;  i >= 0;  i-- )
    {
      tac = HaArray(mt->tasks_and_costs, i);
      if( KheTaskAsstResource(tac->task) == r )
	return *task = tac->task, *pos = i, true;
    }
  }
  else
  {
    /* NULL resource must be just after the first mt->asst_count entries */
    if( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
    {
      tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      HnAssert(KheTaskAsstResource(tac->task) == NULL,
	"KheMTaskFindTask internal error");
      return *task = tac->task, *pos = mt->asst_count, true;
    }
  }
  return *task = NULL, *pos = -1, false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceReassignCheck(KHE_MTASK mt, KHE_RESOURCE from_r,    */
/*    KHE_RESOURCE to_r, bool disallow_preassigned)                          */
/*                                                                           */
/*  Check whether the move of from_r to to_r within mt is possible.  If      */
/*  disallow_preassigned is true, always say no when mt is preassigned.      */
/*                                                                           */
/*  Here is proof that the various reasons for failure are tested here:      */
/*                                                                           */
/*    the move would change nothing, because from_r == to_r.  This is        */
/*    checked by KheTaskMoveCheck, called by KheTaskMoveResourceCheck.       */
/*                                                                           */
/*    mt contains only preassigned or fixed tasks; their assignments         */
/*    cannot change.  This is checked by KheTaskMoveCheck, directly          */
/*    for fixed tasks, and indirectly via domains for preassigned tasks.     */
/*    When disallow_preassigned is true, all changes to the assignments      */
/*    of preassigned tasks are disallowed; when it is false, some changes    */
/*    are allowed, including unassignments.                                  */
/*                                                                           */
/*    from_r != NULL and from_r is not one of the resources currently        */
/*    assigned to mt.  This is checked by KheMTaskFindTask.                  */
/*                                                                           */
/*    to_r != NULL and the domain of mt (the same for all its tasks) does    */
/*    not contain to_r.  This is checked by KheTaskMoveCheck.                */
/*                                                                           */
/*    from_r == NULL (and therefore to_r != NULL) and mt does not contain    */
/*    at least one currently unassigned task to assign to_r to.  This is     */
/*    checked by KheMTaskFindTask.                                           */
/*                                                                           */
/*  And this is everything checked by KheMTaskFindTask and KheTaskMoveCheck. */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskResourceReassignCheck(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned)
{
  KHE_TASK task;  int pos;  KHE_RESOURCE r2;
  return KheMTaskFindTask(mt, from_r, &task, &pos) &&
    !(disallow_preassigned && KheTaskIsPreassigned(task, &r2)) &&
    KheTaskMoveResourceCheck(task, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceReassign(KHE_MTASK mt, KHE_RESOURCE from_r,         */
/*    KHE_RESOURCE to_r, bool disallow_preassigned)                          */
/*                                                                           */
/*  Move from_r to to_r within mt.                                           */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskResourceReassign(KHE_MTASK mt, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool disallow_preassigned)
{
  KHE_TASK task;  int pos;  KHE_TASK_AND_COST pos_tac, other_tac;
  KHE_RESOURCE r, r2;
  if( KheMTaskFindTask(mt, from_r, &task, &pos) &&
    !(disallow_preassigned && KheTaskIsPreassigned(task, &r2)) &&
    KheTaskMoveResource(task, to_r) )
  {
    if( from_r == NULL )
    {
      /* we've added one assignment, which always goes to the right pos */
      mt->asst_count++;
    }
    else if( to_r == NULL )
    {
      /* we've removed one assignment, which might need fixing up */
      if( pos < mt->asst_count - 1 )
      {
	/* move the assignment at mt->asst_count - 1 to the gap at pos */
	pos_tac = HaArray(mt->tasks_and_costs, pos);
	other_tac = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
	r = KheTaskAsstResource(other_tac->task);
	HnAssert(r != NULL, "KheMTaskMoveResource internal error 1");
	if( !KheTaskUnAssignResource(other_tac->task) ||
	    !KheTaskAssignResource(pos_tac->task, r) )
	  HnAbort("KheMTaskMoveResource internal error 2");
      }
      mt->asst_count--;
    }
    else
    {
      /* we've changed one resource to another, which is always fine as is */
    }
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAssignResourceCheck(KHE_MTASK mt, KHE_RESOURCE to_r)        */
/*                                                                           */
/*  Check whether to_r can be assigned to mt.                                */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskAssignResourceCheck(KHE_MTASK mt, KHE_RESOURCE to_r)
{
  return KheMTaskRes ourceReassignCheck(mt, NULL, to_r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAssignResource(KHE_MTASK mt, KHE_RESOURCE to_r)             */
/*                                                                           */
/*  Assign to_r to mt.                                                       */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskAssignResource(KHE_MTASK mt, KHE_RESOURCE to_r)
{
  return KheMTaskResourceReassign(mt, NULL, to_r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskUnAssignResourceCheck(KHE_MTASK mt, KHE_RESOURCE from_r)    */
/*                                                                           */
/*  Check whether from_r can be unassigned from mt.                          */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskUnAssignResourceCheck(KHE_MTASK mt, KHE_RESOURCE from_r)
{
  return KheMTaskResou rceReassignCheck(mt, from_r, NULL);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskUnAssignResource(KHE_MTASK mt, KHE_RESOURCE from_r)         */
/*                                                                           */
/*  Unassigned from_r from mt.                                               */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskUnAssignResource(KHE_MTASK mt, KHE_RESOURCE from_r)
{
  return KheMTaskResourceReassign(mt, from_r, NULL);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceReassignTaskCheck(KHE_MTASK mt, KHE_TASK task,      */
/*    KHE_RESOURCE to_r)                                                       */
/*                                                                           */
/*  Check that task (an element of mt) can be moved to to_r.                   */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskResourceReassignTaskCheck(KHE_MTASK mt, KHE_TASK task,
  KHE_RESOURCE to_r)
{
  return KheTaskMoveResourceCheck(task, to_r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceReassignTask(KHE_MTASK mt, KHE_TASK task,           */
/*    KHE_RESOURCE to_r)                                                       */
/*                                                                           */
/*  Move task to to_r.                                                         */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskResourceReassignTask(KHE_MTASK mt, KHE_TASK task, KHE_RESOURCE to_r)
{
  return KheMTaskResourceReassign(mt, KheTaskAsstResource(task), to_r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSuggestion(KHE_TASK task, KHE_TASK root_task,                */
/*    KHE_RESOURCE *to_r)                                                    */
/*                                                                           */
/*  If task or its descendants generates a suggestion, set *to_r to that     */
/*  suggestion and return true.  Otherwise leave *to_r alone and return      */
/*  false.                                                                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskSuggestion(KHE_TASK task, KHE_TASK root_task,
  KHE_RESOURCE *to_r)
{
  int i;  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_RESOURCE r;

  /* see whether task itself produces a suggestion */
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheEventResourceTaskCount(soln, er);  i++ )
    {
      r = KheTaskAsstResource(KheEventResourceTask(soln, er, i));
      if( r != NULL && KheTaskAssignResourceCheck(root_task, r) )
	return *to_r = r, true;
    }

  /* see whether any tasks assigned to task produce a suggestion */
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    if( KheTaskSuggestion(KheTaskAssignedTo(task, i), root_task, to_r) )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskResourceAssignSuggestion(KHE_MTASK mt, KHE_RESOURCE *to_r)  */
/*                                                                           */
/*  If a suggestion for which resource to assign next to mt can be made,     */
/*  return true with *to_r set to that suggestion.  Else return false with   */
/*  *to_r set to NULL.                                                       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskResourceAssignSuggestion(KHE_MTASK mt, KHE_RESOURCE *to_r)
{
  KHE_TASK task;  int pos;
  *to_r = NULL;
  return KheMTaskFindTask(mt, NULL, &task, &pos) &&
    KheTaskSuggestion(task, task, to_r);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskAsstResourceCount(KHE_MTASK mt)                              */
/*                                                                           */
/*  Return the number of resources assigned to mt.                           */
/*                                                                           */
/*****************************************************************************/

int KheMTaskAsstResourceCount(KHE_MTASK mt)
{
  KheMTaskBringAsstCountUpToDate(mt);
  return mt->asst_count;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheMTaskAsstResource(KHE_MTASK mt, int i)                   */
/*                                                                           */
/*  Return the i'th resource assigned to mt.                                 */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE KheMTaskAsstResource(KHE_MTASK mt, int i)
{
  KHE_TASK_AND_COST tc;
  tc = HaArray(mt->tasks_and_costs, i);
  return KheTaskAsstResource(tc->task);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskUnassignedTaskCount(KHE_MTASK mt)                            */
/*                                                                           */
/*  Return the number of unassigned tasks in mt.                             */
/*                                                                           */
/*****************************************************************************/

int KheMTaskUnassignedTaskCount(KHE_MTASK mt)
{
  KheMTaskBringAsstCountUpToDate(mt);
  return HaArrayCount(mt->tasks_and_costs) - mt->asst_count;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskNeedsAssignment(KHE_MTASK mt)                               */
/*                                                                           */
/*  Return true if mt needs assignment, because it contains an unassigned    */
/*  task for which non_asst_cost - asst_cost > 0, so that assigning this     */
/*  task will reduce solution cost.                                          */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskNeedsAssignment(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  KheMTaskBringAsstCountUpToDate(mt);
  if( mt->asst_count < HaArrayCount(mt->tasks_and_costs) )
  {
    tc = HaArray(mt->tasks_and_costs, mt->asst_count);
    return tc->non_asst_cost - tc->asst_cost > 0;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskContainsNeedlessAssignment(KHE_MTASK mt)                    */
/*                                                                           */
/*  Return true if one of the assignments to mt could be removed without     */
/*  increasing solution cost.                                                */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskContainsNeedlessAssignment(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  KheMTaskBringAsstCountUpToDate(mt);
  if( mt->asst_count > 0 )
  {
    /* find the last assignment */
    tc = HaArray(mt->tasks_and_costs, mt->asst_count - 1);
    return tc->non_asst_cost - tc->asst_cost <= 0;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAddTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)          */
/*                                                                           */
/*  Check that tb can be added to mt.                                        */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAddTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskAddTaskBoundCheck(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskAddTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)               */
/*                                                                           */
/*  Try to add tb to mt, returning true if successful.                       */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskAddTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskAddTaskBound(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskDeleteTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)       */
/*                                                                           */
/*  Check that tb can be deleted from mt.                                    */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskDeleteTaskBoundCheck(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskDeleteTaskBoundCheck(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskDeleteTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)            */
/*                                                                           */
/*  Try to delete tb from mt, returning true if successful.                  */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskDeleteTaskBound(KHE_MTASK mt, KHE_TASK_BOUND tb)
{
  KHE_TASK_AND_COST tc;  int i;
  HaArrayForEach(mt->tasks_and_costs, tc, i)
    if( !KheTaskDeleteTaskBound(tc->task, tb) )
      return false;
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskTaskBoundCount(KHE_MTASK mt)                                 */
/*                                                                           */
/*  Return the number of task bounds in mt.                                  */
/*                                                                           */
/*****************************************************************************/

int KheMTaskTaskBoundCount(KHE_MTASK mt)
{
  KHE_TASK_AND_COST tc;
  tc = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskTaskBoundCount(tc->task);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_BOUND KheMTaskTaskBound(KHE_MTASK mt, int i)                    */
/*                                                                           */
/*  Return the i'th task bound of mt.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_BOUND KheMTaskTaskBound(KHE_MTASK mt, int i)
{
  KHE_TASK_AND_COST tc;
  tc = HaArrayFirst(mt->tasks_and_costs);
  return KheTaskTaskBound(tc->task, i);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskContainsTask(KHE_MTASK mt, KHE_TASK task)                   */
/*                                                                           */
/*  Return true if mt contains task.                                         */
/*                                                                           */
/*****************************************************************************/

static bool KheMTaskContainsTask(KHE_MTASK mt, KHE_TASK task)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEach(mt->tasks_and_costs, tac, i)
    if( tac->task == task )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFixAssignments(KHE_MTASK mt, int pos)                       */
/*                                                                           */
/*  Check whether there is a problem with the assignments at position        */
/*  pos of mt, which must exist.  If there is, fix it.                       */
/*                                                                           */
/*  There is a problem if the task at pos is assigned but the one            */
/*  before it is unassigned, or if the task at pos is unassigned but         */
/*  the one after it is assigned.  But we don't need to check these          */
/*  tasks, because we can use mt->asst_count to work out where we are:       */
/*                                                                           */
/*    * From 0 inclusive to mt->asst_count exclusive, all are assigned;      */
/*                                                                           */
/*    * From mt->asst_count inclusive to HaArrayCount(mt->tasks_and_costs)   */
/*      exclusive, all are unassigned.                                       */
/*                                                                           */
/*  KheMTaskFixAssignments is about preserving this invariant, given         */
/*  that it may be violated at pos.                                          */
/*                                                                           */
/*  There is no need to call KheMTaskBringAsstCountUpToDate(mt), because     */
/*  KheMTaskFixAssignments is called only from KheMTaskAddTask, which is     */
/*  called only from KheMTaskFinderAddTask, which is called only from        */
/*  KheMTaskFinderMake.  Indeed it would be wrong to call it here.           */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFixAssignments(KHE_MTASK mt, int pos)
{
  KHE_TASK_AND_COST pos_tac, other_tac;  KHE_RESOURCE r;
  pos_tac = HaArray(mt->tasks_and_costs, pos);
  if( KheTaskAsstResource(pos_tac->task) != NULL )
  {
    if( pos > mt->asst_count )
    {
      /* move the assignment at pos to the gap at mt->asst_count */
      other_tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      r = KheTaskAsstResource(pos_tac->task);
      if( !KheTaskUnAssignResource(pos_tac->task) ||
	  !KheTaskAssignResource(other_tac->task, r) )
	HnAbort("KheMTaskFixAssignments internal error 1");
    }

    /* one more assignment now */
    mt->asst_count++;
  }
  else
  {
    if( pos < mt->asst_count )
    {
      /* move the assignment at mt->asst_count to the gap at pos */
      /* NB the last assignment was initially at mt->asst_count - 1, but a */
      /* new entry has been inserted at pos and shifted it to mt->asst_count */
      other_tac = HaArray(mt->tasks_and_costs, mt->asst_count);
      r = KheTaskAsstResource(other_tac->task);
      HnAssert(r != NULL, "KheMTaskFixAssignments internal error 2");
      if( !KheTaskUnAssignResource(other_tac->task) ||
	  !KheTaskAssignResource(pos_tac->task, r) )
	HnAbort("KheMTaskFixAssignments internal error 3");
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskFinderLinkTaskToMTask(KHE_MTASK_FINDER mtf,                 */
/*    KHE_TASK task, KHE_MTASK mt)                                           */
/*                                                                           */
/*  Add a map from task (a proper root task) to mt in mtf.                   */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskFinderLinkTaskToMTask(KHE_MTASK_FINDER mtf,
  KHE_TASK task, KHE_MTASK mt)
{
  int index;
  HnAssert(KheTaskIsProperRoot(task),
    "KheMTaskFinderLinkTaskToMTask internal error 1");
  index = KheTaskSolnIndex(task);
  HaArrayFill(mtf->mtasks_by_task, index + 1, NULL);
  HnAssert(HaArray(mtf->mtasks_by_task, index) == NULL,
    "KheMTaskFinderLinkTaskToMTask internal error 2");
  HaArrayPut(mtf->mtasks_by_task, index, mt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskAddTask(KHE_MTASK mt, KHE_TASK task,                        */
/*    KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)      */
/*                                                                           */
/*  Add task (with the given costs) to mt.                                   */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskAddTask(KHE_MTASK mt, KHE_TASK task,
  KHE_COST non_asst_cost, KHE_COST asst_cost, KHE_MTASK_FINDER mtf)
{
  KHE_TASK_AND_COST tac;  int i;
  HaArrayForEachReverse(mt->tasks_and_costs, tac, i)
    if( tac->non_asst_cost - tac->asst_cost >= non_asst_cost - asst_cost )
      break;
  HaArrayAdd(mt->tasks_and_costs, i + 1,
    KheTaskAndCostMake(task, non_asst_cost, asst_cost, mtf));
  KheMTaskFixAssignments(mt, i + 1);
  KheMTaskFinderLinkTaskToMTask(mtf, task, mt);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDebugHeader(KHE_MTASK mt, FILE *fp)                         */
/*                                                                           */
/*  Print the header part of the debug of mt onto fp.                        */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskDebugDayRange(KHE_MTASK mt, FILE *fp)
{
  fprintf(fp, "%s", KheIntervalShow(mt->fixed_times_interval,
    mt->mtask_finder->days_frame));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskDebug(KHE_MTASK mt, int verbosity, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of mt onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskDebug(KHE_MTASK mt, int verbosity, int indent, FILE *fp)
{
  KHE_TASK_AND_COST tac;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ MTask ", indent, "");
    KheMTaskDebugDayRange(mt, fp);
    fprintf(fp, "\n");
    if( verbosity >= 3 )
      KheRootTaskSigDebug(mt->sig, verbosity, indent + 2, fp);
    HaArrayForEach(mt->tasks_and_costs, tac, i)
    {
      fprintf(fp, "%*s  n%.5f a%.5f ", indent, "",
	KheCostShow(tac->non_asst_cost), KheCostShow(tac->asst_cost));
      KheTaskDebug(tac->task, verbosity, 0, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
  else if( verbosity == 1 )
  {
    fprintf(fp, "%s", KheMTaskId(mt));
  }
  else
  {
    fprintf(fp, "MTask(");
    KheMTaskDebugDayRange(mt, fp);
    HaArrayForEach(mt->tasks_and_costs, tac, i)
    {
      fprintf(fp, ", ");
      KheTaskDebug(tac->task, verbosity, -1, fp);
    }
    fprintf(fp, ")");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MTASK_SET"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK_SET KheMTaskSetMake(KHE_MTASK_FINDER mtf)                      */
/*                                                                           */
/*  Make a new, empty mtask set.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK_SET KheMTaskSetMake(KHE_MTASK_FINDER mtf)
{
  KHE_MTASK_SET res;
  if( HaArrayCount(mtf->free_mtask_sets) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_mtask_sets);
    HaArrayClear(res->mtasks);
  }
  else
  {
    HaMake(res, mtf->arena);
    HaArrayInit(res->mtasks, mtf->arena);
  }
  res->interval = KheIntervalMake(0, -1);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDelete(KHE_MTASK_SET mts, KHE_MTASK_FINDER mtf)          */
/*                                                                           */
/*  Delete mtask set mts.  It goes on the free list in mtf.                  */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDelete(KHE_MTASK_SET mts, KHE_MTASK_FINDER mtf)
{
  HaArrayAddLast(mtf->free_mtask_sets, mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetClear(KHE_MTASK_SET mts)                                 */
/*                                                                           */
/*  Clear mts back to empty.                                                 */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetClear(KHE_MTASK_SET mts)
{
  HaArrayClear(mts->mtasks);
  mts->interval = KheIntervalMake(0, -1);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetResetInterval(KHE_MTASK_SET mts)                         */
/*                                                                           */
/*  Reset mts->interval to the correct value for mts->mtasks.  This is       */
/*  necessary after one or more mtasks are deleted.                          */
/*                                                                           */
/*****************************************************************************/

static void KheMTaskSetResetInterval(KHE_MTASK_SET mts)
{
  KHE_MTASK mt;  int i;
  mts->interval = KheIntervalMake(0, -1);
  HaArrayForEach(mts->mtasks, mt, i)
    mts->interval = KheIntervalUnion(mts->interval, KheMTaskInterval(mt));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetClearFromEnd(KHE_MTASK_SET mts, int count)               */
/*                                                                           */
/*  Clear mts from the end until count elements remain.                      */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetClearFromEnd(KHE_MTASK_SET mts, int count)
{
  while( HaArrayCount(mts->mtasks) > count )
    HaArrayDeleteLast(mts->mtasks);
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDropFromEnd(KHE_MTASK_SET mts, int n)                    */
/*                                                                           */
/*  Drop the last n elements of mts from the end.                            */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDropFromEnd(KHE_MTASK_SET mts, int n)
{
  while( n > 0 )
  {
    HaArrayDeleteLast(mts->mtasks);
    n--;
  }
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetAddMTask(KHE_MTASK_SET mts, KHE_MTASK mt)                */
/*                                                                           */
/*  Add mt to mts.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetAddMTask(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  HaArrayAddLast(mts->mtasks, mt);
  mts->interval = KheIntervalUnion(mts->interval, KheMTaskInterval(mt));
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAddMTaskDisjoint(KHE_MTASK_SET mts, KHE_MTASK mt)        */
/*                                                                           */
/*  If mt does not overlap mts's interval, add it to mts and return true.    */
/*  Otherwise change nothing and return false.                               */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskSetAddMTaskDisjoint(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  if( KheIntervalDisjoint(mts->interval, KheMTaskInterval(mt)) )
  {
    KheMTaskSetAddMTask(mts, mt);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetAddMTaskSet(KHE_MTASK_SET mts, KHE_MTASK_SET mts2)       */
/*                                                                           */
/*  Add the mtasks of mts2 to mts, without disturbing mts2.                  */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetAddMTaskSet(KHE_MTASK_SET mts, KHE_MTASK_SET mts2)
{
  int i;
  HaArrayAppend(mts->mtasks, mts2->mtasks, i);
  mts->interval = KheIntervalUnion(mts->interval, mts2->interval);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAddMTaskSetDisjoint(KHE_MTASK_SET mts,                   */
/*    KHE_MTASK_SET mts2)                                                    */
/*                                                                           */
/*  If mts2 does not overlap mts's interval, add it to mts and return true.  */
/*  Otherwise change nothing and return false.                               */
/*                                                                           */
/*****************************************************************************/

/* *** fine, but currently withdrawn
bool KheMTaskSetAddMTaskSetDisjoint(KHE_MTASK_SET mts, KHE_MTASK_SET mts2)
{
  if( KheIntervalDisjoint(mts->interval, mts2->interval) )
  {
    KheMTaskSetAddMTaskSet(mts, mts2);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDeleteMTask(KHE_MTASK_SET mts, KHE_MTASK mt)             */
/*                                                                           */
/*  Find and delete mt from mts.  It must be present.                        */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDeleteMTask(KHE_MTASK_SET mts, KHE_MTASK mt)
{
  int pos;
  if( HaArrayContains(mts->mtasks, mt, &pos) )
    HaArrayDeleteAndPlug(mts->mtasks, pos);
  else
    HnAbort("KheMTaskSetDeleteMTask mt not in mts");
  KheMTaskSetResetInterval(mts);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetLastAndDelete(KHE_MTASK_SET mts)                    */
/*                                                                           */
/*  Assuming mts is non-empty, delete and return the last element.           */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetLastAndDelete(KHE_MTASK_SET mts)
{
  KHE_MTASK res;
  res = HaArrayLastAndDelete(mts->mtasks);
  KheMTaskSetResetInterval(mts);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetContainsMTask(KHE_MTASK_SET mts, KHE_MTASK mt, int *pos) */
/*                                                                           */
/*  If mts contains mt, set *pos to its position and return true.  Otherwise */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetContainsMTask(KHE_MTASK_SET mts, KHE_MTASK mt, int *pos)
{
  return HaArrayContains(mts->mtasks, mt, pos);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskSetMTaskCount(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Return the number of mtasks in mts.                                      */
/*                                                                           */
/*****************************************************************************/

int KheMTaskSetMTaskCount(KHE_MTASK_SET mts)
{
  return HaArrayCount(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetMTask(KHE_MTASK_SET mts, int i)                     */
/*                                                                           */
/*  Return the i'th mtask of mts.                                            */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetMTask(KHE_MTASK_SET mts, int i)
{
  return HaArray(mts->mtasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetFirst(KHE_MTASK_SET mts)                            */
/*                                                                           */
/*  Assuming that mts is non-empty, return its first element.                */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetFirst(KHE_MTASK_SET mts)
{
  HnAssert(HaArrayCount(mts->mtasks) > 0, "KheMTaskSetFirst:  mts is empty");
  return HaArrayFirst(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_MTASK KheMTaskSetLast(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Assuming that mts is non-empty, return its last element.                 */
/*                                                                           */
/*****************************************************************************/

KHE_MTASK KheMTaskSetLast(KHE_MTASK_SET mts)
{
  HnAssert(HaArrayCount(mts->mtasks) > 0, "KheMTaskSetLast:  mts is empty");
  return HaArrayLast(mts->mtasks);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetSort(KHE_MTASK_SET mts,                                  */
/*    int(*compar)(const void *, const void *))                              */
/*                                                                           */
/*  Sort the mtasks of mts using comparison function mts.                    */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetSort(KHE_MTASK_SET mts,
  int(*compar)(const void *, const void *))
{
  HaArraySort(mts->mtasks, compar);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskSetIncreasingFirstTaskCmp(const void *x1, const void *x2)    */
/*                                                                           */
/*  Comparison function for sorting an array of mtasks so as to bring        */
/*  multiple occurrences of the same mtask together.                         */
/*                                                                           */
/*****************************************************************************/

static int KheMTaskSetIncreasingFirstTaskCmp(const void *x1, const void *x2)
{
  KHE_MTASK mt1, mt2;  KHE_TASK task1, task2;  KHE_COST junk1, junk2;
  KHE_MTASK_TIME mt_time1, mt_time2;  KHE_TIME time1, time2;
  mt1 = * (KHE_MTASK *) x1;
  mt2 = * (KHE_MTASK *) x2;

  /* sort by increasing starting time, if that works */
  if( mt1->fixed_times && mt2->fixed_times )
  {
    mt_time1 = HaArrayFirst(mt1->fixed_times_by_day);
    mt_time2 = HaArrayFirst(mt2->fixed_times_by_day);
    time1 = mt_time1->time;
    time2 = mt_time2->time;
    if( time1 != time2 )
      return KheTimeIndex(time1) - KheTimeIndex(time2);
  }

  /* otherwise sort by increasing index of first task */
  task1 = KheMTaskTask(mt1, 0, &junk1, &junk2);
  task2 = KheMTaskTask(mt2, 0, &junk1, &junk2);
  return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetUniqueify(KHE_MTASK_SET mts)                             */
/*                                                                           */
/*  Uniqueify mts.                                                           */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetUniqueify(KHE_MTASK_SET mts)
{
  HaArraySortUnique(mts->mtasks, &KheMTaskSetIncreasingFirstTaskCmp);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheMTaskSetInterval(KHE_MTASK_SET mts)                      */
/*                                                                           */
/*  Find the interval covered by mts, assuming that all its mtasks have      */
/*  fixed times.                                                             */
/*                                                                           */
/*****************************************************************************/

KHE_INTERVAL KheMTaskSetInterval(KHE_MTASK_SET mts)
{
  return mts->interval;
  /* ***
  KHE_INTERVAL res;  int i;  KHE_MTASK mt;
  res = KheIntervalMake(1, 0);
  HaArrayForEach(mts->mtasks, mt, i)
    res = KheIntervalUnion(res, mt->fixed_times_interval);
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetResourceReassignCheck(KHE_MTASK_SET mts,                 */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r)                                */
/*                                                                           */
/*  Return true if every mtask of mts can be moved from from_r to to_r.      */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetResourceReassignCheck(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded)
{
  return KheMTaskSetResourceReassignPartialCheck(mts, 0,
    HaArrayCount(mts->mtasks) - 1, from_r, to_r,
    disallow_preassigned, unassign_extreme_unneeded);
  /* ***
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
  {
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == 0 || i == HaArrayCount(mts->mtasks) - 1)
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      if( !KheMTaskResourceReassignCheck(mt, from_r,NULL,disallow_preassigned)
	&& !KheMTaskResourceReassignCheck(mt, from_r,to_r,disallow_preassigned))
	return false;
    }
    else
    {
      if( !KheMTaskResourceReassignCheck(mt, from_r,to_r,disallow_preassigned) )
	return false;
    }
  }
  return true;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetResourceReassign(KHE_MTASK_SET mts,                      */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r)                                */
/*                                                                           */
/*  Move the mtasks of mts from from_r to to_r, returning true if success.   */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetResourceReassign(KHE_MTASK_SET mts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool disallow_preassigned,
  bool unassign_extreme_unneeded)
{
  return KheMTaskSetResourceReassignPartial(mts, 0,
    HaArrayCount(mts->mtasks) - 1, from_r, to_r,
    disallow_preassigned, unassign_extreme_unneeded);
  /* ***
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
  {
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == 0 || i == HaArrayCount(mts->mtasks) - 1 )
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      ** extreme task that does not need assignment; unassign it **
      if( !KheMTaskResourceReassign(mt, from_r, NULL, disallow_preassigned) &&
	  !KheMTaskResourceReassign(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
    else
    {
      ** regular move **
      if( !KheMTaskResourceReassign(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
  }
  return true;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetResourceReassignPartialCheck(KHE_MTASK_SET mts,          */
/*    int first_index, int last_index, KHE_RESOURCE from_r,KHE_RESOURCE to_r,*/
/*    bool disallow_preassigned, bool unassign_extreme_unneeded)             */
/*                                                                           */
/*  Return true if every mtask of mts[first_index .. last_index] can be      */
/*  moved from from_r to to_r.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetResourceReassignPartialCheck(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded)
{
  KHE_MTASK mt;  int i;
  HnAssert(0 <= first_index,
    "KheMTaskSetResourceReassignPartialCheck internal error 1");
  HnAssert(first_index <= last_index + 1,
    "KheMTaskSetResourceReassignPartialCheck internal error 2");
  HnAssert(last_index < KheMTaskSetMTaskCount(mts),
    "KheMTaskSetResourceReassignPartialCheck internal error 3");
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == first_index || i == last_index)
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      if( !KheMTaskResourceReassignCheck(mt, from_r,NULL,disallow_preassigned)
	&& !KheMTaskResourceReassignCheck(mt, from_r,to_r,disallow_preassigned))
	return false;
    }
    else
    {
      if( !KheMTaskResourceReassignCheck(mt, from_r,to_r,disallow_preassigned) )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetResourceReassignPartial(KHE_MTASK_SET mts,               */
/*    int first_index, int last_index, KHE_RESOURCE from_r,KHE_RESOURCE to_r,*/
/*    bool disallow_preassigned, bool unassign_extreme_unneeded)             */
/*                                                                           */
/*  Move the mtasks of mts[first_index .. last_index] from from_r to to_r,   */
/*  returning true if success.                                               */
/*                                                                           */
/*****************************************************************************/

bool KheMTaskSetResourceReassignPartial(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  bool disallow_preassigned, bool unassign_extreme_unneeded)
{
  KHE_MTASK mt;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( unassign_extreme_unneeded && to_r != NULL &&
	(i == first_index || i == last_index )
	&& KheMTaskContainsNeedlessAssignment(mt) )
    {
      /* extreme task that does not need assignment; unassign it */
      if( !KheMTaskResourceReassign(mt, from_r, NULL, disallow_preassigned) &&
	  !KheMTaskResourceReassign(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
    else
    {
      /* regular move */
      if( !KheMTaskResourceReassign(mt, from_r, to_r, disallow_preassigned) )
	return false;
    }
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAssignResourceCheck(KHE_MTASK_SET mts, KHE_RESOURCE to_r)*/
/*                                                                           */
/*  Return true if every mtask of mts can be assigned to_r.                  */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetAssignResourceCheck(KHE_MTASK_SET mts, KHE_RESOURCE to_r)
{
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
    if( !KheMTaskAssignResourceCheck(mt, to_r) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetAssignResource(KHE_MTASK_SET mts, KHE_RESOURCE to_r)     */
/*                                                                           */
/*  Assign to_r to the mtasks of mts, returning true if successful.          */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetAssignResource(KHE_MTASK_SET mts, KHE_RESOURCE to_r)
{
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
    if( !KheMTaskAssignResource(mt, to_r) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetUnAssignResourceCheck(KHE_MTASK_SET mts,                 */
/*    KHE_RESOURCE from_r)                                                   */
/*                                                                           */
/*  Return true if from_r can be unassigned from every mtask of mts.         */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetUnAssignResourceCheck(KHE_MTASK_SET mts, KHE_RESOURCE from_r)
{
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
    if( !KheMTaskUnAssignResourceCheck(mt, from_r) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetUnAssignResource(KHE_MTASK_SET mts, KHE_RESOURCE from_r) */
/*                                                                           */
/*  Unassign from_r from every mtask of mts, returning true if successful.   */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetUnAssignResource(KHE_MTASK_SET mts, KHE_RESOURCE from_r)
{
  KHE_MTASK mt;  int i;
  HaArrayForEach(mts->mtasks, mt, i)
    if( !KheMTaskUnAssignResource(mt, from_r) )
      return false;
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetPartMoveResourceCheck(KHE_MTASK_SET mts,                 */
/*    int first_index, int last_index, KHE_RESOURCE from_r,                  */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Like KheMTaskSetResourceReassignCheck except that only the mtasks with   */
/*  indexes in mts between first_index and last_index inclusive are checked. */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetPartMoveResourceCheck(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  KHE_MTASK mt;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( !KheMTaskResour ceReassignCheck(mt, from_r, to_r) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetPartMoveResource(KHE_MTASK_SET mts,                      */
/*    int first_index, int last_index, KHE_RESOURCE from_r,                  */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Like KheMTaskSetMoveResource except that only the mtasks with indexes    */
/*  in mts between first_index and last_index inclusive are moved.           */
/*                                                                           */
/*****************************************************************************/

/* *** fine but currently withdrawn
bool KheMTaskSetPartMoveResource(KHE_MTASK_SET mts,
  int first_index, int last_index, KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  KHE_MTASK mt;  int i;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = HaArray(mts->mtasks, i);
    if( !KheMTaskResourceReassign(mt, from_r, to_r) )
      return false;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDebug(KHE_MTASK_SET mts, int verbosity, int indent,      */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of mts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDebug(KHE_MTASK_SET mts, int verbosity, int indent, FILE *fp)
{
  KHE_MTASK mt;  int i;  KHE_MTASK_FINDER mtf;
  if( indent >= 0 )
  {
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{ MTaskSet(");
    if( HaArrayCount(mts->mtasks) > 0 )
    {
      mt = HaArrayFirst(mts->mtasks);
      mtf = mt->mtask_finder;
      fprintf(fp, "%s", KheIntervalShow(mts->interval, mtf->days_frame));
    }
    fprintf(fp, ")\n");
    HaArrayForEach(mts->mtasks, mt, i)
      KheMTaskDebug(mt, verbosity, indent + 2, fp);
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "}\n");
  }
  else
  {
    fprintf(fp, "{");
    HaArrayForEach(mts->mtasks, mt, i)
    {
      if( i > 0 )
	fprintf(fp, ", ");
      KheMTaskDebug(mt, 1, -1, fp);
    }
    if( HaArrayCount(mts->mtasks) > 0 )
    {
      mt = HaArrayFirst(mts->mtasks);
      mtf = mt->mtask_finder;
      fprintf(fp, " [%s]", KheIntervalShow(mts->interval, mtf->days_frame));
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDebugPartial(KHE_MTASK_SET mts, int first_index,         */
/*    int last_index, int verbosity, int indent, FILE *fp)                   */
/*                                                                           */
/*  Debug print of mts onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

void KheMTaskSetDebugPartial(KHE_MTASK_SET mts, int first_index,
  int last_index, int verbosity, int indent, FILE *fp)
{
  KHE_MTASK mt;  int i;
  HnAssert(0 <= first_index,
    "KheMTaskSetDebugPartial internal error 1");
  HnAssert(first_index <= last_index + 1,
    "KheMTaskSetDebugPartial internal error 2");
  HnAssert(last_index < KheMTaskSetMTaskCount(mts),
    "KheMTaskSetDebugPartial internal error 3");
  if( indent >= 0 )
  {
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "{ MTaskSet(%d-%d):\n", first_index, last_index);
    for( i = first_index;  i <= last_index;  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      KheMTaskDebug(mt, verbosity, indent + 2, fp);
    }
    if( indent > 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "}\n");
  }
  else
  {
    fprintf(fp, "{%d-%d: ", first_index, last_index);
    for( i = first_index;  i <= last_index;  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      if( i > first_index )
	fprintf(fp, ", ");
      KheMTaskDebug(mt, 1, -1, fp);
    }
    fprintf(fp, "}");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_MPART_FROM" (private) - no longer used                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  char *KheResourceShow(KHE_RESOURCE r)                                    */
/*                                                                           */
/*  Return the Id of r, or "@" if NULL.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *KheResourceShow(KHE_RESOURCE r)
{
  return r == NULL ? "@" : KheResourceId(r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartFromInit(KHE_MPART_FROM pf, KHE_MTASK_SET mts)              */
/*                                                                           */
/*  Initialize pf to empty, using mts.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartFromInit(KHE_MPART_FROM pf, KHE_MTASK_SET mts)
{
  pf->mts = mts;
  KheMTaskSetClear(mts);
  pf->in = KheIntervalMake(1, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartFromClear(KHE_MPART_FROM pf)                                */
/*                                                                           */
/*  Clear pf back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartFromClear(KHE_MPART_FROM pf)
{
  KheMTaskSetClear(pf->mts);
  pf->in = KheIntervalMake(1, 0);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartFromAddMTask(KHE_MPART_FROM pf, KHE_MTASK mt)               */
/*                                                                           */
/*  Add mt to pf.                                                            */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartFromAddMTask(KHE_MPART_FROM pf, KHE_MTASK mt)
{
  KheMTaskSetAddMTask(pf->mts, mt);
  pf->in = KheIntervalUnion(pf->in, KheMTaskInterval(mt));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartFromAddMTasks(KHE_MPART_FROM pf, KHE_MTASK_SET mts,         */
/*    KHE_INTERVAL mts_in)                                                   */
/*                                                                           */
/*  Add the tasks of mts to pf.  Together they cover mts_in.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartFromAddMTasks(KHE_MPART_FROM pf, KHE_MTASK_SET mts,
  KHE_INTERVAL mts_in)
{
  int i;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
    KheMTaskSetAddMTask(pf->mts, KheMTaskSetMTask(mts, i));
  pf->in = KheIntervalUnion(pf->in, mts_in);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartFromUnion(KHE_MPART_FROM target_pf,                         */
/*    KHE_MPART_FROM source_pf)                                              */
/*                                                                           */
/*  Add source_pf's tasks to target_pf.                                      */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartFromUnion(KHE_MPART_FROM target_pf,
  KHE_MPART_FROM source_pf)
{
  KheMPartFromAddMTasks(target_pf, source_pf->mts, source_pf->in);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskCompatible(KHE_MTASK mt, KHE_RESOURCE_GROUP rg, int count)  */
/*                                                                           */
/*  Return true if mt's domain is compatible with rg and r.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMTaskCompatible(KHE_MTASK mt, KHE_RESOURCE_GROUP rg, int count)
{
  ** check r **
  ** ***
  if( r != NULL && !KheMTaskAssignResourceCheck(mt, r) )
    return false;
  *** **

  ** check rg **
  return (rg == NULL ? true :
    KheResourceGroupIntersectCount(rg, KheMTaskDomain(mt)) >= count);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt)                          */
/*                                                                           */
/*  Return the index in its days_frame time group of the first time that mt  */
/*  is running.                                                              */
/*                                                                           */
/*  This function assumes that mt has fixed times.                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static int KheMTaskIndexInFrameTimeGroup(KHE_MTASK mt)
{
  KHE_MTASK_TIME mt_time;  KHE_TIME_GROUP tg;  int pos;

  HnAssert(mt->fixed_times, "KheMTaskIndexInFrameTimeGroup internal error");
  mt_time = HaArrayFirst(mt->fixed_times_by_day);
  tg = KheFrame TimeTimeGroup(mt->mtask_finder->days_frame, mt_time->time);
  if( KheTimeGroupContains(tg, mt_time->time, &pos) )
    return pos;
  HnAbort("KheTaskIndexInFrameTimeGroup internal error");
  return 0;  ** keep compiler happy **

  ** *** old version
  task = KheMTaskT ask(mt, 0, &asst_cost, &non _asst_cost);
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    t = KheMeetAsstTime(meet);
    if( t != NULL )
    {
      tg = KheFrameTi meTimeGroup(days_frame, t);
      if( KheTimeGroupContains(tg, t, &pos) )
	return pos;
      HnAbort("KheTaskIndexInFrameTimeGroup internal error");
    }
  }
  return 0;
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMTask(KHE_MTASK_FINDER mtf, int day_index, KHE_INTERVAL *in, */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,              */
/*    KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,        */
/*    int *tg_start, KHE_MTASK *mt)                                          */
/*                                                                           */
/*  Set *mt to hold an mtask running on day_index but not during *in, and    */
/*  replace *in by its union with *mt's interval.  Or if no such mt can be   */
/*  found, set *mt to NULL and return false.                                 */
/*                                                                           */
/*  If allow_preassigned is true, preassigned mtasks are allowed, otherwise  */
/*  preassigned tasks are ignored.                                           */
/*                                                                           */
/*  In addition, if rtm != NULL:                                             */
/*                                                                           */
/*    * the mtask is to contain a task from rtm.                             */
/*                                                                           */
/*  If rtm == NULL:                                                          */
/*                                                                           */
/*    * The mtask is to contain an unassigned task of type rt that needs     */
/*      assignment.                                                          */
/*                                                                           */
/*    * If *rg != NULL, the mtask's domain must be compatible with *rg, and  */
/*      it should preferably run at index *tg_start in the times of its day. */
/*                                                                           */
/*    * If r != NULL the task must be assignable to r.                       */
/*                                                                           */
/*  If rtm == NULL && *rg == NULL, KheMTaskFindFrom resets *rg and           */
/*  *tg_start to the values appropriate for the mtask it finds.              */
/*                                                                           */
/*****************************************************************************/

/* *** replaced by KheGetMTask
static bool KheFindMTask(KHE_MTASK_FINDER mtf, int day_index, KHE_INTERVAL *in,
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_RESOURCE_TYPE rt,
  KHE_RESOURCE r, bool allow_preassigned, KHE_RESOURCE_GROUP *rg,
  int *tg_start, KHE_MTASK *mt)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, dc;
  KHE_RESOURCE r2;  KHE_MEET meet;  KHE_TASK task;  KHE_INTERVAL mt_in;
  if( day_index < 0 || day_index >= KheFrameTimeGroupCount(mtf->days_frame) )
    return false;
  tg = KheFrameTimeGroup(mtf->days_frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheFindMTask trying %s (rtm %s NULL)\n",
      KheTimeGroupId(tg), rtm != NULL ? "!=" : "==");
  if( rtm != NULL )
  {
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
    {
      t = KheTimeGroupTime(tg, i);
      count = KheResourceTimetableMonitorTimeTaskCount(rtm, t);
      for( j = 0;  j < count;  j++ )
      {
	task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
	*mt = KheMTaskFinderTaskToMTask(mtf, task);
	if( allow_preassigned || !KheMTaskIsPreassigned(*mt, &r2) )
	{
	  mt_in = KheMTaskInterval(*mt);
	  if( KheIntervalDisjoint(mt_in, *in) )
	  {
	    *in = KheIntervalUnion(*in, mt_in);
	    return true;
	  }
	}
      }
    }
  }
  else
  {
    dc = (*rg != NULL ? KheResourceGroupResourceCount(*rg) / 2 : 0);
    count = KheTimeGroupTimeCount(tg);
    for( i = 0;  i < count;  i++ )
    {
      t = KheTimeGroupTime(tg, (i + *tg_start) % count);
      for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(mtf->etm, t); j++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(mtf->etm, t, j);
	for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
	{
	  task = KheMeetTask(meet, k);
	  *mt = KheMTaskFinderTaskToMTask(mtf, task);
	  if( (allow_preassigned || !KheMTaskIsPreassigned(*mt, &r2)) &&
	      KheMTaskResourceType(*mt) == rt && KheMTaskNeedsAssignment(*mt) &&
	      KheMTaskCompatible(*mt, *rg, dc, r) )
	  {
	    mt_in = KheMTaskInterval(*mt);
	    if( KheIntervalDisjoint(mt_in, *in) )
	    {
	      *in = KheIntervalUnion(*in, mt_in);
	      if( *rg == NULL )
	      {
		*rg = KheMTaskDomain(*mt);
		*tg_start = KheMTaskIndexInFrameTimeGroup(*mt);
	      }
	      return true;
	    }
	  }
	}
      }
    }
  }
  *mt = NULL;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSatisfiesNeedsAssignment(KHE_TASK task, NEEDS_ASST_TYPE na)  */
/*                                                                           */
/*  Return true if task satisfies na.                                        */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused; there is a commented out call on it below
static bool KheTaskSatisfiesNeedsAssignment(KHE_TASK task, NEEDS_ASST_TYPE na)
{
  switch( na )
  {
    case NEEDS_ASST_FALSE:

      return !KheTaskNeedsAssignment(task);

    case NEEDS_ASST_TRUE:

      return KheTaskNeedsAssignment(task);

    case NEEDS_ASST_DONT_CARE:

      return true;

    default:

      HnAbort("KheTaskSatisfiesNeedsAssignment internal error");
      return false;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheGetMTask(KHE_MTASK_FINDER mtf, int day_index,                    */
/*    KHE_INTERVAL disjoint_in, KHE_INTERVAL subset_in,                      */
/*    bool allow_preassigned, KHE_RESOURCE from_r,                           */
/*    KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,                          */
/*    KHE_MTASK *mt, KHE_RESOURCE_GROUP *rg, int *tg_start)                  */
/*                                                                           */
/*  Search for an mtask satisfying the conditions given below.  If found,    */
/*  set *mt to the mtask, possibly replace *rg and *tg_start as explained    */
/*  below, and return true.  If not found, set *mt to NULL, leave *rg and    */
/*  *tg_start as they are, and return false.                                 */
/*                                                                           */
/*  These conditions always apply:                                           */
/*                                                                           */
/*    (1) The mtask must be running on the day with index day_index.         */
/*                                                                           */
/*    (2) The mtask's interval must be disjoint from disjoint_in.  If        */
/*        disjoint_in contains day_index, this condition is incompatible     */
/*        with condition (1) and false will be returned.                     */
/*                                                                           */
/*    (3) The mtask's interval must be a subset of subset_in.  If subset_in  */
/*        does not contain day_index, this condition is incompatible with    */
/*        condition (1) and false will be returned.                          */
/*                                                                           */
/*    (4) If allow_preassigned is true, the mtask may be preassigned,        */
/*        otherwise it must be unpreassigned.                                */
/*                                                                           */
/*    (5) If to_r != NULL, the mtask must be movable from from_r to to_r.    */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*    (6) The mtask must contain a task assigned from_r.                     */
/*                                                                           */
/*    (7) Parameters from_rt, *rg, and *tg_start are unused.                 */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*    (8) The mtask must contain an unassigned task of type from_rt that     */
/*        needs assignment.                                                  */
/*                                                                           */
/*    (9) If *rg != NULL, then (9a) mtask's domain must be compatible with   */
/*        *rg, in a sense that we do not define here; and (9b) the mtask     */
/*        should preferably run at index *tg_start in the times of its day.  */
/*                                                                           */
/*  If from_r == NULL && *rg == NULL, KheGetMTask resets *rg and *tg_start   */
/*  to values appropriate for the mtask it finds, if it finds an mtask.      */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Try to find a task which satisfies the following conditions.  If         */
/*  successful, set *task to one such task and *task_in to its interval      */
/*  and return true.  If unsuccessful, set *task to NULL and *task_in        */
/*  to an empty interval and return false.  The conditions are:              */
/*                                                                           */
/*    * The task must be a proper root task such that it, or a task          */
/*      assigned to it (directly or indirectly) is running at day_index.     */
/*                                                                           */
/*    * If rtm != NULL, rtm is r's timetable monitor and the task must       */
/*      be assigned r.                                                       */
/*                                                                           */
/*    * If rtm == NULL, so is r and the task must be an unassigned task of   */
/*      type rt which needs assignment.  If *rg != NULL, the task's domain   */
/*      must be compatible with *rg, and it should preferably run at index   */
/*      *tg_start in the times of its day.  If *rg == NULL, KheGetTask       */
/*      resets *rg and *tg_start for next time.  Also, if r != NULL the      */
/*      task must be assignable to r.                                        */
/*                                                                           */
/*    * If allow_preassigned is true, the task may be preassigned.           */
/*                                                                           */
/*    * (The task must need assignment or not, depending on na).             */
/*                                                                           */
/*    * The task's interval (including the intervals of all tasks assigned   */
/*      to it, directly or indirectly) must be disjoint from disjoint_in     */
/*      and a subset of subset_in; subset_in must be a legal interval.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheGetMTask(KHE_MTASK_FINDER mtf, int day_index,
  KHE_INTERVAL disjoint_in, KHE_INTERVAL subset_in,
  bool allow_preassigned, KHE_RESOURCE from_r,
  KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,
  KHE_MTASK *mt, KHE_RESOURCE_GROUP *rg, int *tg_start)
{
  KHE_TIME_GROUP tg;  KHE_TIME t;  int i, j, k, count, dc, t_index;
  KHE_RESOURCE r2;  KHE_MEET meet;  KHE_TASK task;  KHE_INTERVAL mt_in;
  KHE_RESOURCE_TIMETABLE_MONITOR from_rtm;
  ** *** these two conditions will be enforced without a special case
  if( !KheIntervalContains(subset_in, day_index) )
    return false;
  if( KheIntervalContains(disjoint_in, day_index) )
    return false;
  *** **
  tg = KheFrameTimeGroup(mtf->days_frame, day_index);
  if( DEBUG7 )
    fprintf(stderr, "  KheGetMTask trying %s (from_r %s)\n",
      KheTimeGroupId(tg), KheResourceShow(from_r));
  if( from_r != NULL )
  {
    from_rtm = KheResourceTimetableMonitor(mtf->soln, from_r);
    for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )  ** (1) **
    {
      t = KheTimeGroupTime(tg, i);
      count = KheResourceTimetableMonitorTimeTaskCount(from_rtm, t);
      for( j = 0;  j < count;  j++ )
      {
	task = KheResourceTimetableMonitorTimeTask(from_rtm, t, j);  ** (6) **
	*mt = KheMTaskFinderTaskToMTask(mtf, task);
	if( allow_preassigned || !KheMTaskIsPreassigned(*mt, &r2) ) ** (4) **
	{
	  mt_in = KheMTaskInterval(*mt);
	  if( KheIntervalDisjoint(mt_in, disjoint_in) &&  ** (2) **
	      KheIntervalSubset(mt_in, subset_in) &&  ** (3), and (5) below **
	      (to_r==NULL || KheMTaskR esourceReassignCheck(*mt, from_r, to_r)) )
	    return true;
	}
      }
    }
  }
  else
  {
    dc = (*rg != NULL ? KheResourceGroupResourceCount(*rg) / 2 : 0);
    count = KheTimeGroupTimeCount(tg);
    for( i = 0;  i < count;  i++ )  ** (1) **
    {
      t_index = (*rg == NULL ? i : (i + *tg_start) % count);
      t = KheTimeGroupTime(tg, t_index);  ** (9b) **
      for( j = 0;  j < KheEventTimetableMonitorTimeMeetCount(mtf->etm, t); j++ )
      {
	meet = KheEventTimetableMonitorTimeMeet(mtf->etm, t, j);
	for( k = 0;  k < KheMeetTaskCount(meet);  k++ )
	{
	  task = KheMeetTask(meet, k);
	  *mt = KheMTaskFinderTaskToMTask(mtf, task);
	  if( (allow_preassigned || !KheTaskIsPreassigned(task, &r2)) && **(4)**
	      KheMTaskResourceType(*mt) == from_rt &&   ** (8)  **
	      KheMTaskNeedsAssignment(*mt) &&           ** (8)  **
	      KheMTaskCompatible(*mt, *rg, dc) )        ** (9a) **
	  {
	    mt_in = KheMTaskInterval(*mt);
	    if( KheIntervalDisjoint(mt_in, disjoint_in) &&   ** (2) **
		KheIntervalSubset(mt_in, subset_in) &&  ** (3), and (5) below **
		(to_r == NULL || KheMTaskRes ourceReassignCheck(*mt, from_r, to_r)) )
	    {
	      if( *rg == NULL )
	      {
		** reset *rg and *tg_start **
		*rg = KheMTaskDomain(*mt);
		*tg_start = KheMTaskIndexInFrameTimeGroup(*mt);
	      }
	      return true;
	    }
	  }
	}
      }
    }
  }
  *mt = NULL;
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFindFrom(KHE_MTASK_FINDER mtf, int day_index,               */
/*    KHE_INTERVAL *disjoint_in, bool allow_preassigned,                     */
/*    KHE_RESOURCE from_r, KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,     */
/*    KHE_MPART_FROM pf, KHE_RESOURCE_GROUP *rg, int *tg_start)              */
/*                                                                           */
/*  Search for an mtask satisfying the conditions given below.  If found,    */
/*  set pf to a from part containing that mtask, set *disjoint_in to the     */
/*  union of its initial value with the mtask's interval, possibly replace   */
/*  *rg and *tg_start as explained below, and return true.  If not found,    */
/*  clear pf, leave *disjoint_in, *rg, and *tg_start as they are, and        */
/*  return false.                                                            */
/*                                                                           */
/*  These conditions always apply:                                           */
/*                                                                           */
/*    (1) The mtask must be running on the day with index day_index.         */
/*                                                                           */
/*    (2) The mtask's interval must be disjoint from the initial value of    */
/*        *disjoint_in.                                                      */
/*                                                                           */
/*    (3) There is no (3) like there is in KheGetMTask.                      */
/*                                                                           */
/*    (4) If allow_preassigned is true, the mtask may be preassigned,        */
/*        otherwise it must be unpreassigned.                                */
/*                                                                           */
/*    (5) If to_r != NULL, the mtask must be movable from from_r to to_r.    */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*    (6) The mtask must contain a task assigned from_r.                     */
/*                                                                           */
/*    (7) Parameters from_rt, *rg, and *tg_start are unused.                 */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*    (8) The mtask must contain an unassigned task of type from_rt that     */
/*        needs assignment.                                                  */
/*                                                                           */
/*    (9) If *rg != NULL, then (9a) mtask's domain must be compatible with   */
/*        *rg, in a sense that we do not define here; and (9b) the mtask     */
/*        should preferably run at index *tg_start in the times of its day.  */
/*                                                                           */
/*  These conditions are the same as the ones for KheGetMTask, except that   */
/*  disjoint_in is passed by reference and condition (3) is omitted.         */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Set pf to hold an mtask running on day_index but not during *in, and     */
/*  replace *in by its union with that mtask's interval.  Or if no such      */
/*  mtask can be found, clear pf and return false.                           */
/*                                                                           */
/*  If allow_preassigned is true, preassigned mtasks are allowed, otherwise  */
/*  only unpreassigned tasks are allowed.                                    */
/*                                                                           */
/*  In addition, if rtm != NULL:                                             */
/*                                                                           */
/*    * the mtask is to contain a task from rtm.                             */
/*                                                                           */
/*  If rtm == NULL:                                                          */
/*                                                                           */
/*    * The mtask is to contain an unassigned task of type rt that needs     */
/*      assignment.                                                          */
/*                                                                           */
/*    * If *rg != NULL, the mtask's domain must be compatible with *rg, and  */
/*      it should preferably run at index *tg_start in the times of its day. */
/*                                                                           */
/*    * If r != NULL the task must be assignable to r.                       */
/*                                                                           */
/*  If rtm == NULL && *rg == NULL, KheMTaskFindFrom resets *rg and           */
/*  *tg_start to the values appropriate for the mtask it finds.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMTaskFindFrom(KHE_MTASK_FINDER mtf, int day_index,
  KHE_INTERVAL *disjoint_in, bool allow_preassigned,
  KHE_RESOURCE from_r, KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r,
  KHE_MPART_FROM pf, KHE_RESOURCE_GROUP *rg, int *tg_start)
{
  KHE_MTASK mt;  KHE_INTERVAL subset_in;
  KheMPartFromClear(pf);
  subset_in = KheIntervalMake(0, KheMTaskFinderLastIndex(mtf));
  if( KheGetMTask(mtf, day_index, *disjoint_in, subset_in, allow_preassigned,
      from_r, from_rt, to_r, &mt, rg, tg_start) )
  {
    KheMPartFromAddMTask(pf, mt);
    *disjoint_in = KheIntervalUnion(*disjoint_in, KheMTaskInterval(mt));
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "to parts" (private)                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMPartToInit(KHE_MPART_TO pt, KHE_MTASK_SET mts)                  */
/*                                                                           */
/*  Initialize pt to empty, using mts.                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartToInit(KHE_MPART_TO pt, KHE_MTASK_SET mts)
{
  pt->mts = mts;
  KheMTaskSetClear(mts);
  pt->durn = 0;
  pt->effectively_free = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartToClear(KHE_MPART_TO pt)                                    */
/*                                                                           */
/*  Clear pt back to empty.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartToClear(KHE_MPART_TO pt)
{
  KheMTaskSetClear(pt->mts);
  pt->durn = 0;
  pt->effectively_free = true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartToAddMTask(KHE_MPART_TO pt, KHE_MTASK mt,                   */
/*    int mt_durn, bool mt_effectively_free)                                 */
/*                                                                           */
/*  Add mt, with the given duration and effectively_free value, to pt.       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartToAddMTask(KHE_MPART_TO pt, KHE_MTASK mt,
  bool mt_effectively_free)
{
  KheMTaskSetAddMTask(pt->mts, mt);
  pt->durn += KheIntervalLength(mt->fixed_times_interval);
  if( !mt_effectively_free )
    pt->effectively_free = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartToUnion(KHE_MPART_TO target_pt, KHE_MPART_TO source_pt)     */
/*                                                                           */
/*  Add source_pt to target_pt.                                              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartToUnion(KHE_MPART_TO target_pt, KHE_MPART_TO source_pt)
{
  int i;  KHE_MTASK mt;
  for( i = 0;  i < KheMTaskSetMTaskCount(source_pt->mts);  i++ )
  {
    mt = KheMTaskSetMTask(source_pt->mts, i);
    KheMTaskSetAddMTask(target_pt->mts, mt);
  }
  target_pt->durn += source_pt->durn;
  if( source_pt->effectively_free == false )
    target_pt->effectively_free = false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskFindTo(KHE_MTASK_FINDER mtf, KHE_INTERVAL in, bool subset,  */
/*    KHE_RESOURCE to_r, bool require_optional, KHE_MTASK_SET omit_mts,      */
/*    KHE_MPART_TO pt)                                                       */
/*                                                                           */
/*  The general aim here is to find the mtasks that to_r must be unassigned  */
/*  from before some other resource can be moved to to_r in interval in.     */
/*  If there will be a problem unassigning these mtasks, false is returned.  */
/*  Bearing this in mind helps to explain the somewhat odd specification.    */
/*                                                                           */
/*  Clear pt and reset it according to the following cases.                  */
/*                                                                           */
/*  If to_r == NULL, leave pt empty and return true.  We do this because     */
/*  in this case there are no mtasks that to_r needs to be unassigned from.  */
/*                                                                           */
/*  If to_r != NULL, find all mtasks mt satisfying these conditions:         */
/*                                                                           */
/*   (1) to_r is assigned to mt.                                             */
/*                                                                           */
/*   (2) mt is running, wholly or partly, within in.                         */
/*                                                                           */
/*   (3) If omit_mts != NULL, mt is not an element of omit_mts.              */
/*                                                                           */
/*  Then, if any of the following conditions hold for any of these mtasks    */
/*  mt, unassigning to_r from them is problematical, so false is returned    */
/*  and pt's contents are undefined:                                         */
/*                                                                           */
/*   (4) mt is preassigned.                                                  */
/*                                                                           */
/*   (5) subset is true and mt is running partly outside in.                 */
/*                                                                           */
/*   (6) require_optional is true and the assignment of to_r to mt is        */
/*       not optional, in the sense that unassigning to_r from mt would      */
/*       increase solution cost, because KheMTaskNeedsAssignment would       */
/*       be true after the unassignment.                                     */
/*                                                                           */
/*  Otherwise true is returned and pt contains the mtasks found.             */
/*                                                                           */
/*  Obsolete:                                                                */
/*  Find all tasks from rtm running within in, and set pt to them; although  */
/*  if omit_mts != NULL, ignore any tasks already present in omit_mts.       */
/*  Return false if any of them are preassigned, or (when subset is true)    */
/*  running outside in, or (when require_optional is true) not optional.     */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMTaskFindTo(KHE_MTASK_FINDER mtf, KHE_INTERVAL in, bool subset,
  KHE_RESOURCE to_r, bool require_optional, KHE_MTASK_SET omit_mts,
  KHE_MPART_TO pt)
{
  int index, i, j, count, pos;  KHE_TIME_GROUP tg;  KHE_RESOURCE r;
  KHE_TIME t;  KHE_TASK task;  bool optional;
  KHE_MTASK mt;  KHE_RESOURCE_TIMETABLE_MONITOR to_rtm;
  KheMPartToClear(pt);
  if( to_r != NULL )
  {
    to_rtm = KheResourceTimetableMonitor(mtf->soln, to_r);
    for( index = KheIntervalFirst(in);  index <= KheIntervalLast(in);  index++ )
    {
      tg = KheFrameTimeGroup(mtf->days_frame, index);
      for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
      {
	t = KheTimeGroupTime(tg, i);
	count = KheResourceTimetableMonitorTimeTaskCount(to_rtm, t);
	for( j = 0;  j < count;  j++ )
	{
	  task = KheResourceTimetableMonitorTimeTask(to_rtm, t, j);
	  mt = KheMTaskFinderTaskToMTask(mtf, task);  ** (1) and (2) **
	  if( !KheMTaskSetContainsMTask(pt->mts, mt, &pos) && ** (3) below **
	    (omit_mts == NULL || !KheMTaskSetContainsMTask(omit_mts,mt,&pos)) )
	  {
	    if( KheMTaskIsPreassigned(mt, &r) )  ** (4) **
	      return false;
	    if( subset && !KheIntervalSubset(KheMTaskInterval(mt), in) ) **(5)**
	      return false;
	    optional = KheMTaskContainsNeedlessAssignment(mt);
	    if( require_optional && !optional )  ** (6) **
	      return false;
	    KheMPartToAddMTask(pt, mt, optional);
	  }
	}
      }
    }
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "parts" (private)                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMPartInit(KHE_MPART p, KHE_MTASK_SET from_mts,                   */
/*    KHE_MTASK_SET to_mts)                                                  */
/*                                                                           */
/*  Initialize p to empty, using from_mts and to_mts.                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartInit(KHE_MPART p, KHE_MTASK_SET from_mts,
  KHE_MTASK_SET to_mts)
{
  KheMPartFromInit(&p->from, from_mts);
  KheMPartToInit(&p->to, to_mts);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMPartClear(KHE_MPART p)                                          */
/*                                                                           */
/*  Clear p back to empty.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMPartClear(KHE_MPART p)
{
  KheMPartFromClear(&p->from);
  KheMPartToClear(&p->to);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_MPART KheMPartMake(KHE_WIDENED_MTASK_SET wmts)                       */
/*                                                                           */
/*  Make a new, empty part.                                                  */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_MPART KheMPartMake(KHE_WIDENED_MTASK_SET wmts)
{
  KHE_MPART res;  KHE_MTASK_FINDER mtf;

  ** get a part object from wts's free list or arena **
  mtf = wmts->mtask_finder;
  if( HaArrayCount(mtf->free_mparts) > 0 )
  {
    res = HaArrayLastAndDelete(mtf->free_mparts);
    KheMPartClear(res);
  }
  else
  {
    HaMake(res, mtf->arena);
    KheMPartInit(res, KheMTaskSetMake(mtf), KheMTaskSetMake(mtf));
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartMoveCheck(KHE_MPART p, bool force, KHE_RESOURCE from_r,     */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Return true if p can move from from_r to to_r, assuming it has been      */
/*  prepared.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartMoveCheck(KHE_MPART p, bool force, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r)
{
  return (force || p->to.effectively_free) &&
    KheMTaskSetResourceReassignCheck(p->to.mts, NULL, to_r) &&
    KheMTaskSetResourceReassignCheck(p->from.mts, from_r, to_r);
    ** ***
    KheMTaskSetResourceReassignCheck(p->to.mts) &&
    KheMTaskSetResourceReassignCheck(p->from.mts, to_r);
    *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartMove(KHE_MPART p, KHE_RESOURCE from_r,                      */
/*    KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change)     */
/*                                                                           */
/*  Move p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartMove(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  if( KheMTaskSetResourceReassign(p->to.mts, NULL, to_r) &&
      KheMTaskSetResourceReassign(p->from.mts, from_r, to_r) )
  {
    len = KheIntervalLength(p->from.in);
    *from_r_durn_change -= len;
    *to_r_durn_change += (len - p->to.durn);
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartMovePart(KHE_MPART p, int first_index, int last_index,      */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r)                                */
/*                                                                           */
/*  Move p[first_index .. last_index] to to_r.                               */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartMovePart(KHE_MPART p, int first_index, int last_index,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r)
{
  return KheMTaskSetResourceReassign(p->to.mts, NULL, to_r) &&
    KheMTaskSetPartMoveResource(p->from.mts, first_index, last_index,
      from_r, to_r);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBlockedByTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)         */
/*                                                                           */
/*  Return true if moving task is blocked because it has a time from tg.     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer unused
static bool KheTaskBlockedByTimeGroup(KHE_TASK task, KHE_TIME_GROUP tg)
{
  KHE_MEET meet;  KHE_TIME t, t2;  int i, durn, pos;  KHE_TASK child_task;

  ** do the job for task itself **
  meet = KheTaskMeet(task);
  t = KheMeetAsstTime(meet);
  durn = KheMeetDuration(meet);
  for( i = 0;  i < durn;  i++ )
  {
    t2 = KheTimeNeighbour(t, i);
    if( KheTimeGroupContains(tg, t2, &pos) )
      return true;
  }

  ** do the job for task's children **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskBlockedByTimeGroup(child_task, tg) )
      return true;
  }

  ** no blockage anywhere **
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)    */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks have       */
/*  times from time group tg.  Here tg may be NULL, in which case false      */
/*  is returned.                                                             */
/*                                                                           */
/*****************************************************************************/

/* *** no longer unused
static bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)
{
  int i;  KHE_TASK task;
  if( tg != NULL )
  {
    for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( KheTaskBlockedByTimeGroup(task, tg) )
	return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskBlockedByTimeGroup(KHE_MTASK mt, KHE_TIME_GROUP tg)         */
/*                                                                           */
/*  Return true if mt's times intersect with tg's times.                     */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheMTaskBlockedByTimeGroup(KHE_MTASK mt, KHE_TIME_GROUP tg)
{
  KHE_INTERVAL in;  KHE_MTASK_TIME mt_time;  int i, pos;
  in = mt->fixed_times_interval;
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    mt_time = HaArray(mt->fixed_times_by_day, i);
    if( KheTimeGroupContains(tg, mt_time->time, &pos) )
      return true;
  }
  return false;
}

*** */

/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByTimeGroup(KHE_TASK_SET ts, KHE_TIME_GROUP tg)    */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks have       */
/*  times from time group tg.  Here tg may be NULL, in which case false      */
/*  is returned.                                                             */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheMTaskSetBlockedByTimeGroup(KHE_MTASK_SET mts, KHE_TIME_GROUP tg)
{
  int i;  KHE_MTASK mt;
  if( tg != NULL )
  {
    for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      if( KheMTaskBlockedByTimeGroup(mt, tg) )
	return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskBlockedByMonitor(KHE_TASK task, KHE_MONITOR m)               */
/*                                                                           */
/*  Return true if m monitors task.                                          */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheTaskBlockedByMonitor(KHE_TASK task, KHE_MONITOR m)
{
  KHE_EVENT_RESOURCE er;  int i;  KHE_SOLN soln;  KHE_TASK child_task;

  ** do the job for task itself **
  soln = KheTaskSoln(task);
  er = KheTaskEventResource(task);
  if( er != NULL )
    for( i = 0;  i < KheSolnEventResourceMonitorCount(soln, er);  i++ )
      if( KheSolnEventResourceMonitor(soln, er, i) == m )
	return true;

  ** do the job for the children of task **
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
  {
    child_task = KheTaskAssignedTo(task, i);
    if( KheTaskBlockedByMonitor(child_task, m) )
      return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskSetBlockedByMonitor(KHE_TASK_SET ts, KHE_MONITOR m)          */
/*                                                                           */
/*  Return true if moving ts is blocked because some of the tasks are        */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static bool KheTaskSetBlockedByMonitor(KHE_TASK_SET ts, KHE_MONITOR m)
{
  int i;  KHE_TASK task;
  if( m != NULL )
  {
    for( i = 0;  i < KheTaskSetTaskCount(ts);  i++ )
    {
      task = KheTaskSetTask(ts, i);
      if( KheTaskBlockedByMonitor(task, m) )
	return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskBlockedByMonitor(KHE_MTASK mt, KHE_MONITOR m)               */
/*                                                                           */
/*  Return true if moving mt is blocked because some of the tasks are        */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMTaskBlockedByMonitor(KHE_MTASK mt, KHE_MONITOR m)
{
  int i;  KHE_TASK_AND_COST task_and_cost;
  if( m != NULL )
  {
    HaArrayForEach(mt->tasks_and_costs, task_and_cost, i)
      if( KheTaskBlockedByMonitor(task_and_cost->task, m) )
	return true;
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskSetBlockedByMonitor(KHE_MTASK_SET mts, KHE_MONITOR m)       */
/*                                                                           */
/*  Return true if moving mts is blocked because some of the tasks are       */
/*  monitored by m.  Here m may be NULL, in which case false is returned.    */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
static bool KheMTaskSetBlockedByMonitor(KHE_MTASK_SET mts, KHE_MONITOR m)
{
  int i;  KHE_MTASK mt;
  if( m != NULL )
  {
    for( i = 0;  i < KheMTaskSetMTaskCount(mts);  i++ )
    {
      mt = KheMTaskSetMTask(mts, i);
      if( KheMTaskBlockedByMonitor(mt, m) )
	return true;
    }
  }
  return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartCoreSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,             */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m)                                                */
/*                                                                           */
/*  Return true if core part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartCoreSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m)
{
  ** check equivalent tasks, blocking_tg, and blocking_m **
  ** *** no longer needed
  if( KheTaskSetFirstTasksEquivalent(p->from.ts, p->to.ts) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (equiv tasks)\n");
    return false;
  }
  *** **

  if( KheMTaskSetBlockedByTimeGroup(p->to.mts, blocking_tg) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (time group)\n");
    return false;
  }

  if( KheMTaskSetBlockedByMonitor(p->to.mts, blocking_m) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (monitor)\n");
    return false;
  }

  ** check exact **
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck returning false (exact)\n");
    return false;
  }

  ** check move from from_r to to_r **
  if( !KheMTaskSetResourceReassignCheck(p->from.mts, from_r, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck ret false (from_r to to_r)\n");
    return false;
  }

  ** check move from to_r to from_r **
  if( !KheMTaskSetResourceReassignCheck(p->to.mts, to_r, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartCoreSwapCheck ret false (to_r to from_r)\n");
    return false;
  }

  ** all good **
  if( DEBUG1 )
    fprintf(stderr, "KheMPartCoreSwapCheck returning true\n");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartWingSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,             */
/*    KHE_RESOURCE to_r, bool exact)                                         */
/*                                                                           */
/*  Return true if wing part p can swap to to_r.                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartWingSwapCheck(KHE_MPART p, KHE_RESOURCE from_r,
  KHE_RESOURCE to_r, bool exact)
{
  ** check exact **
  if( exact && p->to.durn != KheIntervalLength(p->from.in))
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (exact)\n");
    return false;
  }

  ** check move from from_r to to_r **
  if( !KheMTaskSetResourceReassignCheck(p->from.mts, from_r, to_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (from_r to to_r)\n");
    return false;
  }

  ** check move from to_r to from_r **
  if( !KheMTaskSetResourceReassignCheck(p->to.mts, to_r, from_r) )
  {
    if( DEBUG1 )
      fprintf(stderr, "KheMPartWingSwapCheck ret false (to_r to from_r)\n");
    return false;
  }

  ** all good **
  if( DEBUG1 )
    fprintf(stderr, "KheMPartWingSwapCheck returning true\n");
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMPartSwap(KHE_MPART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,   */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Swap p to to_r, updating *from_r_durn_change and *to_r_durn_change       */
/*  if successful.                                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMPartSwap(KHE_MPART p, KHE_RESOURCE from_r, KHE_RESOURCE to_r,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int len;
  if( KheMTaskSetResourceReassign(p->from.mts, from_r, to_r) &&
      KheMTaskSetResourceReassign(p->to.mts, to_r, from_r) )
  {
    len = KheIntervalLength(p->from.in) - p->to.durn;
    *from_r_durn_change -= len;
    *to_r_durn_change += len;
    return true;
  }
  else
    return false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task finders"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskFinderTimeGroupInterval(KHE_TASK_FINDER tf,                  */
/*    KHE_TIME_GROUP tg, int *first_index, int *last_index)                  */
/*                                                                           */
/*  Return the interval covered by tg.                                       */
/*                                                                           */
/*****************************************************************************/

/* *** currently unused
void KheMTaskFinderTimeGroupInterval(KHE_MTASK_FINDER mtf,
  KHE_TIME_GROUP tg, int *first_index, int *last_index)
{
  KHE_TIME t;  int count;
  count = KheTimeGroupTimeCount(tg);
  if( count == 0 )
    *first_index = 1, *last_index = 0;
  else
  {
    t = KheTimeGroupTime(tg, 0);
    *first_index = KheFrameTimeIndex(mtf->days_frame, t);
    t = KheTimeGroupTime(tg, count - 1);
    *last_index = KheFrameTimeIndex(mtf->days_frame, t);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_TIMETABLE_MONITOR KheTimetableMonitor(KHE_SOLN soln,        */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return r's timetable monitor, or NULL if r is NULL.                      */
/*                                                                           */
/*****************************************************************************/

/* *** no longer used
static KHE_RESOURCE_TIMETABLE_MONITOR KheTimetableMonitor(KHE_SOLN soln,
  KHE_RESOURCE r)
{
  return (r == NULL ? NULL : KheResourceTimetableMonitor(soln, r));
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheFindMTasksInInterval(KHE_MTASK_FINDER mtf, KHE_INTERVAL in,      */
/*    bool allow_partial, bool allow_preassigned, KHE_RESOURCE from_r,       */
/*    KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r, KHE_MTASK_SET res_mts)   */
/*                                                                           */
/*  Clear res_mts then add some mtasks to it:  a maximal number              */
/*  satisfying the following conditions:                                     */
/*                                                                           */
/*    (1) Each mtask must run partly or wholly within in; see (3).           */
/*                                                                           */
/*    (2) No two mtasks may run on the same day.                             */
/*                                                                           */
/*    (3) If allow_partial is true, each mtask may run partly inside in and  */
/*        partly outside in.  If allow_partial is false, each mtask must     */
/*        run entirely within in.                                            */
/*                                                                           */
/*    (4) If allow_preassigned is true, each mtask may be preassigned,       */
/*        otherwise each mtask must be unpreassigned.                        */
/*                                                                           */
/*    (5) If to_r != NULL, each mtask must be movable from from_r to to_r.   */
/*                                                                           */
/*  In addition, if from_r != NULL:                                          */
/*                                                                           */
/*    (6) Each mtask must contain a task assigned from_r.                    */
/*                                                                           */
/*    (7) Parameter from_rt is unused.                                       */
/*                                                                           */
/*  If from_r == NULL:                                                       */
/*                                                                           */
/*    (8) Each mtask must contain an unassigned task of type from_rt that    */
/*        needs assignment.                                                  */
/*                                                                           */
/*    (9) The mtasks must have compatible domains, and should preferably     */
/*        run at the same times each day.                                    */
/*                                                                           */
/*   Obsolete:                                                               */
/*   * The mtasks must lie wholly or partially within in.  If allow_partial  */
/*     is true, mtasks that lie partly inside in and partly outside it are   */
/*     allowed, otherwise they aren't.                                       */
/*                                                                           */
/*   * No two of the mtasks added to res_mts may run on the same day.        */
/*                                                                           */
/*   * The mtasks must be assigned from_r, or if from_r == NULL they must    */
/*     have resource type rt and need assignment.                            */
/*                                                                           */
/*   * If allow_preassigned is true, preassigned mtasks are allowed,         */
/*     otherwise they aren't.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheFindMTasksInInterval(KHE_MTASK_FINDER mtf, KHE_INTERVAL in,
  bool allow_partial, bool allow_preassigned, KHE_RESOURCE from_r,
  KHE_RESOURCE_TYPE from_rt, KHE_RESOURCE to_r, KHE_MTASK_SET res_mts)
{
  int tg_start, i;  KHE_MTASK mt;
  KHE_INTERVAL disjoint_in, subset_in;  KHE_RESOURCE_GROUP rg;
  disjoint_in = KheIntervalMake(1, 0);
  subset_in = allow_partial ?
    KheIntervalMake(0, KheMTaskFinderLastIndex(mtf)) : in;
  rg = NULL;
  tg_start = 0;
  KheMTaskSetClear(res_mts);
  for( i = KheIntervalFirst(in);  i <= KheIntervalLast(in);  i++ )
  {
    if( KheGetMTask(mtf, i, disjoint_in, subset_in, allow_preassigned,
	  from_r, from_rt, to_r, &mt, &rg, &tg_start) )
    {
      KheMTaskSetAddMTask(res_mts, mt);
      disjoint_in = KheIntervalUnion(disjoint_in, KheMTaskInterval(mt));
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened multi-task sets"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftWing(KHE_WIDENED_TASK_SET wts,            */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_left_wing_count)                                               */
/*                                                                           */
/*  Build a left wing for wts containing up to max_left_wing_count parts.    */
/*  This searches only for unassigned tasks.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetBuildLeftWing(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_left_wing_count)
{
  KHE_INTERVAL in;  int i;  KHE_MPART p;  KHE_MPART_FROM pf;
  in = wmts->core.from.in;
  pf = &wmts->mtask_finder->scratch_mfrom;
  for( i = 0;  i < max_left_wing_count;  i++ )
  {
    if( !KheMTaskFindFrom(wmts->mtask_finder, KheIntervalFirst(in) - 1, &in,
	  false, NULL, rt, NULL, pf, rg, tg_start) )
      break;
    p = KheMPartMake(wmts);
    KheMPartFromUnion(&p->from, pf);
    HaArrayAddLast(wmts->left_wing, p);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildRightWing(KHE_WIDENED_TASK_SET wts,           */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,           */
/*    int max_right_wing_count)                                              */
/*                                                                           */
/*  Build a right wing for wts containing up to max_right_wing_count parts.  */
/*  This searches only for unassigned tasks.                                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetBuildRightWing(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start,
  int max_right_wing_count)
{
  KHE_INTERVAL in;  int i;  KHE_MPART p;  KHE_MPART_FROM pf;
  in = wmts->core.from.in;
  pf = &wmts->mtask_finder->scratch_mfrom;
  for( i = 0;  i < max_right_wing_count;  i++ )
  {
    if( !KheMTaskFindFrom(wmts->mtask_finder, KheIntervalLast(in) + 1, &in,
	  false, NULL, rt, NULL, pf, rg, tg_start) )
      break;
    p = KheMPartMake(wmts);
    KheMPartFromUnion(&p->from, pf);
    HaArrayAddLast(wmts->right_wing, p);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedTaskSetBuildLeftAndRightWings(KHE_WIDENED_TASK_SET wts,   */
/*    KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc)  */
/*                                                                           */
/*  Build left and right wings for wts containing up to mwc parts.           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetBuildLeftAndRightWings(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE_TYPE rt, KHE_RESOURCE_GROUP *rg, int *tg_start, int mwc)
{
  KHE_INTERVAL in;  KHE_MPART_FROM pf;  bool lopen, ropen, try_left;
  KHE_MPART p;  KHE_MTASK_FINDER mtf;
  mtf = wmts->mtask_finder;
  in = wmts->core.from.in;
  pf = &wmts->mtask_finder->scratch_mfrom;
  lopen = true;  ropen = true;
  while( HaArrayCount(wmts->left_wing) + HaArrayCount(wmts->right_wing) < mwc )
  {
    ** sort out which side to try, or break if neither side is open **
    if( lopen )
    {
      if( ropen )
      {
	** both sides are open, so try the smallest side **
	try_left =
	  HaArrayCount(wmts->left_wing) < HaArrayCount(wmts->right_wing);
      }
      else
      {
	** only the left side is open, so try that **
	try_left = true;
      }
    }
    else
    {
      if( ropen )
      {
	** only the right side is open, so try that **
	try_left = false;
      }
      else
      {
	** neither side is open, time to stop **
	break;
      }
    }

    ** try the left or the right side, depending on try_left **
    if( try_left )
    {
      if( KheMTaskFindFrom(mtf, KheIntervalFirst(in) - 1, &in, false,
	    NULL, rt, NULL, pf, rg, tg_start) )
      {
	p = KheMPartMake(wmts);
	KheMPartFromUnion(&p->from, pf);
	HaArrayAddLast(wmts->left_wing, p);
      }
      else
	lopen = false;
    }
    else
    {
      if( KheMTaskFindFrom(mtf, KheIntervalLast(in) + 1, &in, false,
	    NULL, rt, NULL, pf, rg, tg_start) )
      {
	p = KheMPartMake(wmts);
	KheMPartFromUnion(&p->from, pf);
	HaArrayAddLast(wmts->right_wing, p);
      }
      else
	ropen = false;
    }
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_WIDENED_MTASK_SET KheWidenedMTaskSetGet(KHE_MTASK_FINDER mtf,        */
/*    KHE_RESOURCE from_r, bool to_r_is_set, KHE_RESOURCE to_r)              */
/*                                                                           */
/*  Get a widened mtask set from mtf's arena or free list, with all its      */
/*  fields initialized to the values passed, or to null values.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static KHE_WIDENED_MTASK_SET KheWidenedMTaskSetGet(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE from_r, bool to_r_is_set, KHE_RESOURCE to_r)
{
  KHE_WIDENED_MTASK_SET res;  HA_ARENA a;
  *f( HaArrayCount(mtf->free_widened_mtask_sets) > 0 )
  {
    ** get res from tf's free list **
    res = HaArrayLastAndDelete(mtf->free_widened_mtask_sets);
    KheMPartClear(&res->core);
    HaArrayClear(res->left_wing);
    HaArrayClear(res->right_wing);
    HaArrayClear(res->swappable_wings);
  }
  else
  {
    ** get res from tf's arena **
    a = mtf->arena;
    HaMake(res, a);
    res->mtask_finder = mtf;
    KheMPartInit(&res->core, KheMTaskSetMake(mtf), KheMTaskSetMake(mtf));
    HaArrayInit(res->left_wing, a);
    HaArrayInit(res->right_wing, a);
    HaArrayInit(res->swappable_wings, a);
  }
  res->from_r = from_r;
  res->to_r_is_set = to_r_is_set;
  res->to_r = to_r;  ** undefined if !to_r_is_set **
  res->to_r_max_left_wing_count = 0;
  res->to_r_max_right_wing_count = 0;
  res->best_cost = KheCost(INT_MAX, INT_MAX);
  res->best_defects = INT_MAX;
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetDoMake(KHE_MTASK_FINDER mtf, KHE_RESOURCE from_r, */
/*    KHE_MTASK_SET from_r_mts, KHE_WIDENED_MTASK_SET *wmts,                 */
/*    KHE_RESOURCE_TYPE *rt, KHE_RESOURCE_GROUP *rg, int *tg_start)          */
/*                                                                           */
/*  Do most of the work of building a widened mtask set; all but the wings.  */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheWidenedMTaskSetDoMake(KHE_MTASK_FINDER mtf, KHE_RESOURCE from_r,
  KHE_MTASK_SET from_r_mts, KHE_WIDENED_MTASK_SET *wmts,
  KHE_RESOURCE_TYPE *rt, KHE_RESOURCE_GROUP *rg, int *tg_start)
{
  ** KHE_RESOURCE_TIMETABLE_MONITOR from_r_rtm; **
  KHE_INTERVAL mt_in;  int i;  KHE_RESOURCE r;  KHE_INTERVAL from_r_in;
  KHE_MTASK mt;

  ** check the basic conditions for from_r_mts and find its bounding interval **
  if( KheMTaskSetMTaskCount(from_r_mts) == 0 )
    return *wmts = NULL, false;
  from_r_in = KheIntervalMake(1, 0);
  HaArrayForEach(from_r_mts->mtasks, mt, i)
  {
    if( KheMTaskIsPreassigned(mt, &r) )
      return *wmts = NULL, false;
    mt_in = KheMTaskInterval(mt);
    if( !KheIntervalDisjoint(mt_in, from_r_in) )
      return *wmts = NULL, false;
    from_r_in = KheIntervalUnion(from_r_in, mt_in);
  }

  ** get the first mtask and its resource type **
  mt = KheMTaskSetFirst(from_r_mts);
  *rt = KheResourceGroupResourceType(KheMTaskDomain(mt));

  ** get res, the new widened mtask set object, and set rg and tg_start **
  if( from_r != NULL )
  {
    ** from_r_rtm = KheResourceTimetableMonitor(mtf->soln, from_r); **
    *rg = NULL;
    *tg_start = 0;
  }
  else
  {
    ** from_r_rtm = NULL; **
    *rg = KheMTaskDomain(mt);
    *tg_start = KheMTaskIndexInFrameTimeGroup(mt);
  }
  *wmts = KheWidenedMTaskSetGet(mtf, from_r, false, NULL);
  KheMPartFromAddMTasks(&(*wmts)->core.from, from_r_mts, from_r_in);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,      */
/*    KHE_TASK_SET from_r_ts, int max_left_wing_count,                       */
/*    int max_right_wing_count, KHE_WIDENED_TASK_SET *wts)                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedTaskSetMake(KHE_TASK_FINDER tf, KHE_RESOURCE from_r,
  KHE_TASK_SET from_r_ts, int max_left_wing_count,
  int max_right_wing_count, KHE_WIDENED_TASK_SET *wts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  ** make the widened task set object and set its core **
  if( !KheWidenedTaskSetDoMake(tf, from_r, from_r_ts, wts, &rt, &rg,&tg_start) )
    return false;

  ** set the wings **
  KheWidenedTaskSetBuildLeftWing(*wts, rt, &rg, &tg_start, max_left_wing_count);
  KheWidenedTaskSetBuildRightWing(*wts,rt, &rg, &tg_start,max_right_wing_count);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetMake(KHE_MTASK_FINDER mtf, KHE_RESOURCE from_r,   */
/*    KHE_MTASK_SET from_r_mts, int max_left_wing_count,                     */
/*    int max_right_wing_count, KHE_WIDENED_MTASK_SET *wmts)                 */
/*                                                                           */
/*  Check the basic conditions for from_r_mts and make a wmts for it.        */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetMake(KHE_MTASK_FINDER mtf, KHE_RESOURCE from_r,
  KHE_MTASK_SET from_r_mts, int max_left_wing_count,
  int max_right_wing_count, KHE_WIDENED_MTASK_SET *wmts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  ** make the widened mtask set object and set its core **
  if( !KheWidenedMTaskSetDoMake(mtf, from_r, from_r_mts, wmts,
	&rt, &rg, &tg_start) )
    return false;

  ** set the wings **
  KheWidenedMTaskSetBuildLeftWing(*wmts, rt, &rg, &tg_start,
    max_left_wing_count);
  KheWidenedMTaskSetBuildRightWing(*wmts,rt, &rg, &tg_start,
    max_right_wing_count);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetMakeFlexible(KHE_MTASK_FINDER mtf,                */
/*    KHE_RESOURCE from_r, KHE_MTASK_SET from_r_mts,                         */
/*    int max_wing_count, KHE_WIDENED_MTASK_SET *wmts)                       */
/*                                                                           */
/*  Like KheWidenedTaskSetMake except that max_wing_count constrains the     */
/*  total number of tasks in the two wings, taken together.                  */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetMakeFlexible(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE from_r, KHE_MTASK_SET from_r_mts,
  int max_wing_count, KHE_WIDENED_MTASK_SET *wmts)
{
  KHE_RESOURCE_GROUP rg;  int tg_start;  KHE_RESOURCE_TYPE rt;

  ** make the widened task set object and set its core **
  if( !KheWidenedMTaskSetDoMake(mtf, from_r, from_r_mts, wmts, &rt,
      &rg, &tg_start) )
    return false;

  ** set both wings **
  KheWidenedMTaskSetBuildLeftAndRightWings(*wmts, rt, &rg, &tg_start,
    max_wing_count);
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetDelete(KHE_WIDENED_MTASK_SET wmts)                */
/*                                                                           */
/*  Delete wmts by cycling it through a free list in tf.                     */
/*                                                                           */
/*  Implementation note.  The number of wings will change next time, so      */
/*  we move them to a free list too.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetDelete(KHE_WIDENED_MTASK_SET wmts)
{
  int i;  KHE_MTASK_FINDER mtf;
  mtf = wmts->mtask_finder;
  HaArrayAppend(mtf->free_mparts, wmts->left_wing, i);
  HaArrayAppend(mtf->free_mparts, wmts->right_wing, i);
  HaArrayAddLast(mtf->free_widened_mtask_sets, wmts);
  ** don't append swappable_wings, they lie in left_wing and right_wing **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheWidenedMTaskSetInterval(KHE_WIDENED_MTASK_SET wmts,      */
/*    int left_count, int right_count)                                       */
/*                                                                           */
/*  Return the interval covered by wmts's core, its first left_count left    */
/*  wings, and its first right_count right wings.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_INTERVAL KheWidenedMTaskSetInterval(KHE_WIDENED_MTASK_SET wmts,
  int left_count, int right_count)
{
  KHE_INTERVAL res;  KHE_MPART p;  int i;
  res = wmts->core.from.in;
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    res = KheIntervalUnion(res, p->from.in);
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    res = KheIntervalUnion(res, p->from.in);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_INTERVAL KheWidenedMTaskSetFullInterval(KHE_WIDENED_MTASK_SET wmts)  */
/*                                                                           */
/*  Return the interval covered by wmts's core and all its wings.            */
/*                                                                           */
/*****************************************************************************/

/* ***
KHE_INTERVAL KheWidenedMTaskSetFullInterval(KHE_WIDENED_MTASK_SET wmts)
{
  KHE_INTERVAL res;  KHE_MPART p;  int i;
  res = wmts->core.from.in;
  for( i = 0;  i < HaArrayCount(wmts->left_wing);  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    res = KheIntervalUnion(res, p->from.in);
  }
  for( i = 0;  i < HaArrayCount(wmts->right_wing);  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    res = KheIntervalUnion(res, p->from.in);
  }
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMovableWidenedMTaskSetRight(KHE_MTASK_FINDER mtf,            */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,          */
/*    KHE_WIDENED_MTASK_SET *res_wmts)                                       */
/*                                                                           */
/*  Find the first maximal widened task set at or to the right of            */
/*  days_first_index that can be moved from from_r to to_r.  Core only.      */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheFindMovableWidenedMTaskSetRight(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,
  KHE_WIDENED_MTASK_SET *res_wmts)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_MTASK_SET res;  KHE_MPART_FROM pf;  KHE_MPART_TO pt;
  KHE_RESOURCE_TYPE rt;

  ** boilerplate **
  if( DEBUG3 )
    fprintf(stderr, "[ KheFindMovableWidenedMTaskSetRight(tf, %s, %s, %d, -)\n",
      KheResourceShow(from_r), KheResourceShow(to_r), days_first_index);
  HnAssert(from_r != to_r,"KheFindMovableWidenedMTaskSetRight: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(mtf->days_frame);
  pf = &mtf->scratch_mfrom;
  pt = &mtf->scratch_mto;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  ** search for a suitable core **
  in = KheIntervalMake(0, days_first_index - 1);
  rg = NULL, tg_start = 0;
  for( i = days_first_index;  i < tg_count;  i++ )
  {
    if( KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r, pf, &rg,
	&tg_start) && KheMTaskFindTo(mtf, pf->in, false, to_r, true, NULL, pt) )
    {
      ** run begins at i; get res, add pf and pt to it, and skip to after it **
      res = KheWidenedMTaskSetGet(mtf, from_r, true, to_r);
      KheMPartFromUnion(&res->core.from, pf);
      KheMPartToUnion(&res->core.to, pt);
      i = KheIntervalLast(in) + 1;

      ** while run continues, add pf and pt to res and skip to after them **
      while( KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r, pf, &rg,
	  &tg_start) && KheMTaskFindTo(mtf, pf->in, false, to_r, true,
	    res->core.to.mts, pt) )
      {
	KheMPartFromUnion(&res->core.from, pf);
	KheMPartToUnion(&res->core.to, pt);
	i = KheIntervalLast(in) + 1;
      }

      ** run ends here **
      if( DEBUG3 )
	fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight ret. true\n");
      return *res_wmts = res, true;
    }
  }

  ** if we get here, no luck **
  if( DEBUG3 )
    fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight returning false\n");
  return *res_wmts = NULL, false;
}
*** */

/* *** old version, nothing actually wrong with it
bool KheFindMovableWidenedMTaskSetRight(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_first_index,
  KHE_WIDENED_MTASK_SET *res_wmts)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_MTASK_SET res;  KHE_MPART_FROM pf;  KHE_MPART_TO pt;
  KHE_RESOURCE_TYPE rt;  bool ff, ft;

  ** boilerplate **
  if( DEBUG3 )
    fprintf(stderr, "[ KheFindMovableWidenedMTaskSetRight(tf, %s, %s, %d, -)\n",
      KheResourceShow(from_r), KheResourceShow(to_r), days_first_index);
  HnAssert(from_r != to_r,"KheFindMovableWidenedMTaskSetRight: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(mtf->days_frame);
  pf = &mtf->scratch_mfrom;
  pt = &mtf->scratch_mto;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  ** search for a suitable core **
  in = KheIntervalMake(0, days_first_index - 1);
  rg = NULL, tg_start = 0;
  for( i = days_first_index;  i < tg_count;  i++ )
  {
    ff = KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r,
      pf, &rg, &tg_start);
    if( DEBUG3 )
      fprintf(stderr, "  at %d, ff = %s\n", i, bool_show(ff));
    if( ff )
    {
      ft = KheMTaskFindTo(mtf, pf->in, false, to_r, true, NULL, pt);
      if( DEBUG3 )
	fprintf(stderr, "  at %d, ft = %s\n", i, bool_show(ff));
      if( ft )
      {
	** run begins at i; get res **
	res = KheWidenedMTaskSetGet(mtf, from_r, to_r);

	** repeatedly add pf and pt to the run and skip to after it **
	do
	{
	  KheMPartFromUnion(&res->core.from, pf);
	  KheMPartToUnion(&res->core.to, pt);
	  i = in.last + 1;
	} while( KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r,
	    pf, &rg, &tg_start) &&
	    KheMTaskFindTo(mtf, pf->in, false, to_r, true,
	      res->core.to.mts, pt) );

	** run ends here **
	if( DEBUG3 )
	  fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight ret. true\n");
	return *res_wmts = res, true;
      }
    }
  }

  ** if we get here, no luck **
  if( DEBUG3 )
    fprintf(stderr, "] KheFindMovableWidenedMTaskSetRight returning false\n");
  return *res_wmts = NULL, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindMovableWidenedMTaskSetLeft(KHE_MTASK_FINDER mtf,             */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,           */
/*    KHE_WIDENED_MTASK_SET *res_wmts)                                       */
/*                                                                           */
/*  Find the first maximal widened task set at or to the left of             */
/*  days_last_index that can be moved from from_r to to_r.  Core only.       */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheFindMovableWidenedMTaskSetLeft(KHE_MTASK_FINDER mtf,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int days_last_index,
  KHE_WIDENED_MTASK_SET *res_wmts)
{
  int tg_count, tg_start, i;  KHE_RESOURCE_GROUP rg;  KHE_INTERVAL in;
  KHE_WIDENED_MTASK_SET res;  KHE_MPART_FROM pf;  KHE_MPART_TO pt;
  KHE_RESOURCE_TYPE rt;

  ** boilerplate **
  HnAssert(from_r != to_r, "KheFindMovableWidenedTaskSetLeft: from_r == to_r");
  tg_count = KheFrameTimeGroupCount(mtf->days_frame);
  pf = &mtf->scratch_mfrom;
  pt = &mtf->scratch_mto;
  rt = (from_r != NULL ? KheResourceResourceType(from_r) :
    KheResourceResourceType(to_r));

  ** search for a suitable core **
  in = KheIntervalMake(days_last_index + 1, tg_count - 1);
  rg = NULL, tg_start = 0;
  for( i = days_last_index;  i >= 0;  i-- )
  {
    if( KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r, pf,
	  &rg, &tg_start) &&
	KheMTaskFindTo(mtf, pf->in, false, to_r, true, NULL, pt) )
    {
      ** run end at i; get res, add pf and pt to it, and skip to before it **
      res = KheWidenedMTaskSetGet(mtf, from_r, true, to_r);
      KheMPartFromUnion(&res->core.from, pf);
      KheMPartToUnion(&res->core.to, pt);
      i = in.first - 1;

      ** while run continues, add pf and pt to res and skip to before them **
      while( KheMTaskFindFrom(mtf, i, &in, false, from_r, rt, to_r, pf,
	    &rg, &tg_start) &&
	  KheMTaskFindTo(mtf, pf->in, false, to_r, true,
	    res->core.to.mts, pt) );
      {
	KheMPartFromUnion(&res->core.from, pf);
	KheMPartToUnion(&res->core.to, pt);
        i = in.first - 1;
      }

      ** run ends here **
      return *res_wmts = res, true;
    }
  }

  ** if we get here, no luck **
  return *res_wmts = NULL, false;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskSetDoDebug(KHE_MTASK_SET mts, int first_index,              */
/*    int last_index, int verbosity, FILE *fp)                               */
/*                                                                           */
/*  Debug print of mts without enclosing braces, for local use.              */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMTaskSetDoDebug(KHE_MTASK_SET mts, int first_index,
  int last_index, int verbosity, FILE *fp)
{
  int i;  KHE_MTASK mt;
  if( last_index == -1 )
    last_index = KheMTaskSetMTaskCount(mts) - 1;
  for( i = first_index;  i <= last_index;  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    if( i > first_index )
      fprintf(fp, ", ");
    KheMTaskDebug(mt, verbosity, -1, fp);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetDoDebug(KHE_WIDENED_MTASK_SET wmts, bool opt,     */
/*    int left_count, int right_count, int first_index, int last_index,      */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of widened task set wmts (but only the first left_count      */
/*  tasks of the left wing, and only the first right_count tasks of the      */
/*  right wing) onto fp with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetDoDebug(KHE_WIDENED_MTASK_SET wmts, bool opt,
  int left_count, int right_count, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KHE_MPART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "{");
  if( left_count > HaArrayCount(wmts->left_wing) )
    left_count = HaArrayCount(wmts->left_wing);
  if( right_count > HaArrayCount(wmts->right_wing) )
    right_count = HaArrayCount(wmts->right_wing);
  for( i = left_count - 1;  i >= 0;  i-- )
  {
    p = HaArray(wmts->left_wing, i);
    if( i < left_count - 1 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheMTaskSetDoDebug(p->from.mts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  if( left_count > 0 || right_count > 0 )
  {
    if( left_count > 0 )
      fprintf(fp, " ");
    fprintf(fp, "[");
  }
  KheMTaskSetDoDebug(wmts->core.from.mts, first_index, last_index,
    verbosity, fp);
  if( left_count > 0 || right_count > 0 )
  {
    fprintf(fp, "]");
    if( right_count > 0 )
      fprintf(fp, " ");
  }
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( i > 0 )
      fprintf(fp, ", ");
    if( !opt || p->best_is_swap )
      KheMTaskSetDoDebug(p->from.mts, 0, -1, verbosity, fp);
    else
      fprintf(fp, "-");
  }
  fprintf(fp, "}");
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetDebug(KHE_WIDENED_MTASK_SET wmts,                 */
/*    int left_count, int right_count, int verbosity, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of widened mtask set wmts (but only the first left_count     */
/*  tasks of the left wing, and only the first right_count tasks of the      */
/*  right wing) onto fp with the given verbosity and indent.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetDebug(KHE_WIDENED_MTASK_SET wmts,
  int left_count, int right_count, int verbosity, int indent, FILE *fp)
{
  KheWidenedMTaskSetDoDebug(wmts, false, left_count, right_count, 0, -1,
    verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened mtask set moves"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheMTaskClearOptimal(KHE_WIDENED_MTASK_SET wmts)                    */
/*                                                                           */
/*  Make clear that any previously discovered optimal move is out of date.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMTaskClearOptimal(KHE_WIDENED_MTASK_SET wmts)
{
  wmts->best_defects = INT_MAX;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskHaveOptimal(KHE_WIDENED_MTASK_SET wmts)                     */
/*                                                                           */
/*  Return true if an optimal result is currently present.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMTaskHaveOptimal(KHE_WIDENED_MTASK_SET wmts)
{
  return wmts->best_defects < INT_MAX;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  char *KheMTaskFinderIntervalShow(KHE_MTASK_FINDER mtf, KHE_INTERVAL in)  */
/*                                                                           */
/*  Show in in static memory.                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static char *KheMTaskFinderIntervalShow(KHE_MTASK_FINDER mtf, KHE_INTERVAL in)
{
  static char buff[500];
  if( KheIntervalEmpty(in) )
    sprintf(buff, "-");
  else
    sprintf(buff, "%s-%s",
      KheTimeGroupId(KheFrameTimeGroup(mtf->days_frame, in.first)),
      KheTimeGroupId(KheFrameTimeGroup(mtf->days_frame, in.last)));
  return buff;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheMTaskPrepareResource(KHE_WIDENED_MTASK_SET wmts,                 */
/*    KHE_RESOURCE to_r)                                                     */
/*                                                                           */
/*  Get wmts ready for operations involving moves from from_r to to_r, or    */
/*  return false if that can't be done.  Do nothing if already set for to_r. */
/*                                                                           */
/*  Implementation note.  When to_r == NULL, KheMTaskFindTo will clear       */
/*  wmts->core.to and return true, which is the right thing.                 */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheMTaskPrepareResource(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r)
{
  int i;  KHE_MPART p;  KHE_MTASK_FINDER mtf;
  if( !wmts->to_r_is_set || to_r != wmts->to_r )
  {
    ** core - fail if it doesn't work **
    mtf = wmts->mtask_finder;
    if( !KheMTaskFindTo(mtf, wmts->core.from.in, true, to_r, false,
	  NULL, &wmts->core.to) )
      return false;
    wmts->to_r_is_set = true;
    wmts->to_r = to_r;
    if( DEBUG4 )
    {
      fprintf(stderr, "  PrepareResource(wmts, %s) core %s = ",
	KheResourceShow(to_r),
	KheMTaskFinderIntervalShow(mtf, wmts->core.from.in));
      KheMTaskSetDebug(wmts->core.to.mts, 2, 0, stderr);
    }

    ** left wing - stop at first that doesn't work **
    HaArrayForEach(wmts->left_wing, p, i)
      if( !KheMTaskFindTo(mtf, p->from.in, true, to_r, false, NULL, &p->to) )
	break;
    wmts->to_r_max_left_wing_count = i;

    ** right wing - stop at first that doesn't work **
    HaArrayForEach(wmts->right_wing, p, i)
      if( !KheMTaskFindTo(mtf, p->from.in, true, to_r, false, NULL, &p->to) )
	break;
    wmts->to_r_max_right_wing_count = i;
  }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetMoveCheck(KHE_WIDENED_MTASK_SET wmts,             */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool force,                    */
/*    int *max_left_count, int *max_right_count)                             */
/*                                                                           */
/*  Return true if the core of wmts can move to to_r; set *max_left_count    */
/*  and *max_right_count to the number of left and right wings that can too. */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetMoveCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, bool force,
  int *max_left_count, int *max_right_count)
{
  int i, j;  KHE_MPART p;

  ** give up now if to_r == from_r or can't prepare to_r **
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
    return *max_left_count = *max_right_count = -1, false;

  ** core must be movable, else fail **
  if( !KheMPartMoveCheck(&wmts->core, force, from_r, to_r) )
    return *max_left_count = *max_right_count = -1, false;

  ** find i, the number of left wing parts that are movable **
  for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartMoveCheck(p, force, from_r, to_r) )
      break;
  }

  ** find j, the number of right wing parts that are movable **
  for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wmts->right_wing, j);
    if( !KheMPartMoveCheck(p, force, from_r, to_r) )
      break;
  }

  ** all good, set *max_left_count and *max_right_count and return **
  return *max_left_count = i, *max_right_count = j, true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetMove(KHE_WIDENED_MTASK_SET wmts,                  */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int left_count,                */
/*    int right_count, int *from_r_durn_change, int *to_r_durn_change)       */
/*                                                                           */
/*  Move the core of wmts, its first left_count left wings, and its first    */
/*  right_count right wings, to to_r.  If successful return true and set     */
/*  *from_r_durn_change and *to_r_durn_change to the change in duration.     */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetMove(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int left_count,
  int right_count, int *from_r_durn_change, int *to_r_durn_change)
{
  int i;  KHE_MPART p;

  ** give up now if to_r == from_r or can't prepare to_r **
  KheMTaskClearOptimal(wmts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
    return false;

  ** check that left_count and right_count are in range **
  HnAssert(0 <= left_count && left_count <= wmts->to_r_max_left_wing_count,
    "KheWidenedMTaskSetMove: left_count (%d) out of range (0 .. %d)",
    left_count, wmts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wmts->to_r_max_right_wing_count,
    "KheWidenedMTaskSetMove: right_count (%d) out of range (0 .. %d)",
    right_count, wmts->to_r_max_right_wing_count);

  ** move the core **
  if( !KheMPartMove(&wmts->core, from_r, to_r, from_r_durn_change,
	to_r_durn_change) )
    return false;

  ** move the first left_count left wings **
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartMove(p, from_r, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  ** move the first right_count right wings **
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( !KheMPartMove(p, from_r, to_r, from_r_durn_change, to_r_durn_change) )
      return false;
  }

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetToResourceDebug(KHE_WIDENED_MTASK_SET wmts,       */
/*    bool opt, KHE_RESOURCE to_r, int left_count, int right_count,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of the to_r parts of widened mtask set wmts (but only the    */
/*  first left_count tasks of the left wing, and only the first right_count  */
/*  tasks of the right wing) onto fp with the given verbosity and indent.    */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetToResourceDebug(KHE_WIDENED_MTASK_SET wmts,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  KHE_MPART p;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  if( to_r == wmts->from_r )
    fprintf(fp, "failed (from_r == to_r)");
  else if ( to_r != wmts->to_r )
    fprintf(fp, "failed (not prepared for %s)", KheResourceShow(to_r));
  else
  {
    fprintf(fp, "{");
    if( left_count > HaArrayCount(wmts->left_wing) )
      left_count = HaArrayCount(wmts->left_wing);
    if( right_count > HaArrayCount(wmts->right_wing) )
      right_count = HaArrayCount(wmts->right_wing);
    for( i = left_count - 1;  i >= 0;  i-- )
    {
      p = HaArray(wmts->left_wing, i);
      if( i < left_count - 1 )
	fprintf(fp, ", ");
      if( KheMTaskSetMTaskCount(p->to.mts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheMTaskSetDoDebug(p->to.mts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    if( left_count > 0 || right_count > 0 )
    {
      if( left_count > 0 )
	fprintf(fp, " ");
      fprintf(fp, "[");
    }
    KheMTaskSetDoDebug(wmts->core.to.mts, 0, -1, verbosity, fp);
    if( left_count > 0 || right_count > 0 )
    {
      fprintf(fp, "]");
      if( right_count > 0 )
	fprintf(fp, " ");
    }
    for( i = 0;  i < right_count;  i++ )
    {
      p = HaArray(wmts->right_wing, i);
      if( i > 0 )
	fprintf(fp, ", ");
      if( KheMTaskSetMTaskCount(p->to.mts) == 0 )
	fprintf(fp, "-");
      else if( !opt || p->best_is_swap )
	KheMTaskSetDoDebug(p->to.mts, 0, -1, verbosity, fp);
      else
	fprintf(fp, "-");
    }
    fprintf(fp, "}");
  }
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetOpDebug(KHE_WIDENED_MTASK_SET wmts, char *op,     */
/*    bool opt, KHE_RESOURCE to_r, int left_count, int right_count,          */
/*    int first_index, int last_index, int verbosity, int indent, FILE *fp)  */
/*                                                                           */
/*  Debug print of an operation.                                             */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheWidenedMTaskSetOpDebug(KHE_WIDENED_MTASK_SET wmts, char *op,
  bool opt, KHE_RESOURCE to_r, int left_count, int right_count,
  int first_index, int last_index, int verbosity, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s ", KheResourceShow(wmts->from_r));
  KheWidenedMTaskSetDoDebug(wmts, opt, left_count, right_count, first_index,
    last_index, verbosity, indent, fp);
  fprintf(fp, " %s ", op);
  fprintf(fp, "%s ", KheResourceShow(to_r));
  KheWidenedMTaskSetToResourceDebug(wmts, opt, to_r, left_count, right_count,
    verbosity, indent, fp);
  if( indent >= 0 )
    fprintf(fp, "\n");
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetMoveDebug(KHE_WIDENED_MTASK_SET wmts,             */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a move.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetMoveDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "--->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetMovePartial(KHE_WIDENED_MTASK_SET wmts,           */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int first_index,               */
/*    int last_index)                                                        */
/*                                                                           */
/*  Move core[first_index .. last_index] to to_r.                            */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetMovePartial(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int first_index, int last_index)
{
  int count;

  ** give up now if to_r == from_r or can't prepare to_r **
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
    return false;

  ** check that first_index and last_index are in range **
  count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  HnAssert(0 <= first_index && first_index < count,
    "KheWidenedMTaskSetMovePartial: first_index (%d) out of range (0 .. %d)",
    first_index, count - 1);
  HnAssert(0 <= last_index && last_index < count,
    "KheWidenedMTaskSetMovePartial: last_index (%d) out of range (0 .. %d)",
    last_index, count - 1);
  HnAssert(first_index <= last_index,
    "KheWidenedMTaskSetMovePartial: first_index (%d) greater than last_index "
    "(%d)", first_index, last_index);

  ** move the core, in part **
  if( !KheMPartMovePart(&wmts->core, first_index, last_index, from_r, to_r) )
    return false;

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetMovePartialDebug(KHE_WIDENED_MTASK_SET wmts,      */
/*    KHE_RESOURCE to_r, int first_index, int last_index,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a partial move.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetMovePartialDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int first_index, int last_index,
  int verbosity, int indent, FILE *fp)
{
  KheWidenedMTaskSetOpDebug(wmts, "-:->", false, to_r, 0, 0,
    first_index, last_index, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetFindInitial(KHE_WIDENED_MTASK_SET wmts,           */
/*    int wanted_durn, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Seach for an initial sequence of wmts's core tasks whose total           */
/*  duration is wanted_durn.  Set *first_index and *last_index to the        */
/*  indexes of the first and last tasks in the sequence, and return true     */
/*  true when their total duration equals wanted_durn.                       */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetFindInitial(KHE_WIDENED_MTASK_SET wmts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i;  KHE_MTASK mt;  KHE_MTASK_SET mts;
  durn = 0;
  mts = wmts->core.from.mts;
  for( i = 0;  i < KheMTaskSetMTaskCount(mts) && durn < wanted_durn;  i++ )
  {
    mt = KheMTaskSetMTask(mts, i);
    durn += KheIntervalLength(KheMTaskInterval(mt));
  }
  *first_index = 0;
  *last_index = i - 1;
  return durn == wanted_durn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetFindFinal(KHE_WIDENED_MTASK_SET wmts,             */
/*    int wanted_durn, int *first_index, int *last_index)                    */
/*                                                                           */
/*  Like KheWidenedMTaskSetFindInitial only searching for final tasks.       */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetFindFinal(KHE_WIDENED_MTASK_SET wmts,
  int wanted_durn, int *first_index, int *last_index)
{
  int durn, i, count;  KHE_MTASK mt;  KHE_MTASK_SET mts;
  durn = 0;
  mts = wmts->core.from.mts;
  count = KheMTaskSetMTaskCount(mts);
  for( i = count - 1;  i >= 0 && durn < wanted_durn;  i-- )
  {
    mt = KheMTaskSetMTask(mts, i);
    durn += KheIntervalLength(KheMTaskInterval(mt));
  }
  *first_index = i + 1;
  *last_index = count - 1;
  return durn == wanted_durn;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened mtask set swaps"                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetSwapCheck(KHE_WIDENED_MTASK_SET wmts,             */
/*    KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,             */
/*    KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count)     */
/*                                                                           */
/*  Check whether wts is swappable with to_r.                                */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetSwapCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, bool exact, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m, int *max_left_count, int *max_right_count)
{
  int i, j;  KHE_MPART p;

  ** give up now if to_r == from_r or can't prepare to_r **
  KheMTaskClearOptimal(wmts);
  if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) ret false (prep)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  ** check core swappable **
  if( !KheMPartCoreSwapCheck(&wmts->core, wmts->from_r, to_r, exact,
	blocking_tg, blocking_m) )
  {
    if( DEBUG2 )
      fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) ret false (core)\n",
	KheResourceShow(to_r));
    return *max_left_count = *max_right_count = -1, false;
  }

  ** find swappable left wings **
  for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartWingSwapCheck(p, wmts->from_r, to_r, exact) )
      break;
  }

  ** find swappable right wings **
  for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
  {
    p = HaArray(wmts->right_wing, j);
    if( !KheMPartWingSwapCheck(p, wmts->from_r, to_r, exact) )
      break;
  }

  ** all good, set *max_left_count and *max_right_count and return **
  if( DEBUG2 )
    fprintf(stderr, "KheWidenedMTaskSetSwapCheck(%s) returning true (%d, %d)\n",
      KheResourceShow(to_r), i, j);
  return *max_left_count = i, *max_right_count = j, true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetSwap(KHE_WIDENED_MTASK_SET wmts,                  */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int *from_r_durn_change, int *to_r_durn_change)                        */
/*                                                                           */
/*  Swap wmts to to_r.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetSwap(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int *from_r_durn_change, int *to_r_durn_change)
{
  int i;  KHE_MPART p;

  ** give up now if to_r == from_r or can't prepare to_r **
  KheMTaskClearOptimal(wmts);
  *from_r_durn_change = *to_r_durn_change = 0;
  if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
    return false;

  ** check that left_count and right_count are in range **
  HnAssert(0 <= left_count && left_count <= wmts->to_r_max_left_wing_count,
    "KheWidenedMTaskSetSwap: left_count (%d) out of range (0 .. %d)",
    left_count, wmts->to_r_max_left_wing_count);
  HnAssert(0 <= right_count && right_count <= wmts->to_r_max_right_wing_count,
    "KheWidenedMTaskSetSwap: right_count (%d) out of range (0 .. %d)",
    right_count, wmts->to_r_max_right_wing_count);

  ** swap the core **
  if( !KheMPartSwap(&wmts->core, wmts->from_r, to_r,
	from_r_durn_change, to_r_durn_change) )
    return false;

  ** swap the first left_count left wings **
  for( i = 0;  i < left_count;  i++ )
  {
    p = HaArray(wmts->left_wing, i);
    if( !KheMPartSwap(p,wmts->from_r,to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  ** swap the first right_count right wings **
  for( i = 0;  i < right_count;  i++ )
  {
    p = HaArray(wmts->right_wing, i);
    if( !KheMPartSwap(p,wmts->from_r,to_r,from_r_durn_change,to_r_durn_change) )
      return false;
  }

  ** all good **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetSwapDebug(KHE_WIDENED_MTASK_SET wmts,             */
/*    KHE_RESOURCE to_r, int left_count, int right_count,                    */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of a swap.                                                   */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetSwapDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int left_count, int right_count,
  int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "<-->", false, to_r, left_count, right_count,
    0, count - 1, verbosity, indent, fp);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "widened mtask sets - optimal moves"                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool NewMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,        */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  Return true if curr_cost and curr_defects represent a new best.          */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool NewMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,
  int curr_defects)
{
  return curr_cost < wmts->best_cost ||
    (curr_cost == wmts->best_cost && curr_defects < wmts->best_defects);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void SetMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,        */
/*    int curr_defects)                                                      */
/*                                                                           */
/*  The current state is a new best.  Remember it.                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void SetMTaskBest(KHE_WIDENED_MTASK_SET wmts, KHE_COST curr_cost,
  int curr_defects)
{
  int i;  KHE_MPART p;

  ** new best wts attributes **
  wmts->best_cost = curr_cost;
  wmts->best_defects = curr_defects;

  ** new best core and wings attributes **
  wmts->core.best_is_swap = wmts->core.curr_is_swap;
  HaArrayForEach(wmts->swappable_wings, p, i)
    p->best_is_swap = p->curr_is_swap;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void RedoMTaskBest(KHE_WIDENED_MTASK_SET wmts, int *from_r_durn_change,  */
/*    int *to_r_durn_change)                                                 */
/*                                                                           */
/*  Redo the previously discovered best.                                     */
/*                                                                           */
/*****************************************************************************/

/* ***
static void RedoMTaskBest(KHE_WIDENED_MTASK_SET wmts, int *from_r_durn_change,
  int *to_r_durn_change)
{
  int i;  KHE_MPART p;

  ** redo the core **
  *from_r_durn_change = *to_r_durn_change = 0;
  if( wmts->core.best_is_swap )
    KheMPartSwap(&wmts->core, wmts->from_r, wmts->to_r, from_r_durn_change,
      to_r_durn_change);
  else
    KheMPartMove(&wmts->core, wmts->from_r ** ? still to do **,
      wmts->to_r, from_r_durn_change, to_r_durn_change);

  ** redo the wings **
  HaArrayForEach(wmts->swappable_wings, p, i)
    if( p->best_is_swap )
      KheMPartSwap(p, wmts->from_r, wmts->to_r, from_r_durn_change,
	  to_r_durn_change);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void DoMTaskWingSwaps(KHE_WIDENED_MTASK_SET wmts, int index)             */
/*                                                                           */
/*  Do wing swaps from index onwards.  Leave the solution as it was found.   */
/*                                                                           */
/*****************************************************************************/

/* ***
static void DoMTaskWingSwaps(KHE_WIDENED_MTASK_SET wmts, int index)
{
  KHE_COST curr_cost;  int curr_defects, junk1, junk2;
  KHE_MARK mark;  KHE_MPART p;  KHE_SOLN soln;
  soln = wmts->mtask_finder->soln;
  if( index >= HaArrayCount(wmts->swappable_wings) )
  {
    ** base of recursion, check for new best and record if found **
    curr_cost = KheSolnCost(soln);
    curr_defects = KheGroupMonitorDefectCount((KHE_GROUP_MONITOR) soln);
    if( NewMTaskBest(wmts, curr_cost, curr_defects) )
      SetMTaskBest(wmts, curr_cost, curr_defects);
  }
  else
  {
    ** try without swapping at index **
    p = HaArray(wmts->swappable_wings, index);
    p->curr_is_swap = false;
    DoMTaskWingSwaps(wmts, index + 1);

    ** try with swapping at index, if p swaps (which it should) **
    p->curr_is_swap = true;
    mark = KheMarkBegin(soln);
    junk1 = junk2 = 0;
    if( KheMPartSwap(p, wmts->from_r, wmts->to_r, &junk1, &junk2) )
      DoMTaskWingSwaps(wmts, index + 1);
    KheMarkEnd(mark, true);
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetOptimalMoveCheck(KHE_WIDENED_MTASK_SET wmts,      */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg,    */
/*    KHE_MONITOR blocking_m)                                                */
/*                                                                           */
/*  Check that an optimal move of wts to to_r is possible.                   */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetOptimalMoveCheck(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, KHE_TIME_GROUP blocking_tg,
  KHE_MONITOR blocking_m)
{
  ** solve from scratch if to_r has changed or no optimal result present **
  if( wmts->to_r != to_r || !KheMTaskHaveOptimal(wmts) )
  {
    ** prepare for to_r; return false if it doesn't work **
    if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
      return false;

    ** try one of the two possibilities for the core **
    if( wmts->core.to.effectively_free )
    {
      ** check moving the core; it must be movable, else fail **
      if( !KheMPartMoveCheck(&wmts->core, false, from_r, to_r) )
	return false;
    }
    else
    {
      ** check swapping the core; it must be swappable, else fail **
      if( !KheMPartCoreSwapCheck(&wmts->core, wmts->from_r, to_r, false,
	    blocking_tg, blocking_m) )
	return false;
    }
  }

  ** no problems, optimal move will work **
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWidenedMTaskSetOptimalMove(KHE_WIDENED_MTASK_SET wmts,           */
/*    KHE_RESOURCE from_r, KHE_RESOURCE to_r, int *from_r_durn_change,       */
/*    int *to_r_durn_change)                                                 */
/*                                                                           */
/*  Make an optimal move of wmts to to_r.                                    */
/*                                                                           */
/*****************************************************************************/

/* ***
bool KheWidenedMTaskSetOptimalMove(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE from_r, KHE_RESOURCE to_r, int *from_r_durn_change,
  int *to_r_durn_change)
{
  int i, j, junk1, junk2;  KHE_MPART p;

  ** solve from scratch if to_r has changed or no optimal result present **
  if( wmts->to_r != to_r || !KheMTaskHaveOptimal(wmts) )
  {
    ** initialize solve fields **
    wmts->best_cost = KheCost(INT_MAX, INT_MAX);
    wmts->best_defects = INT_MAX;

    ** prepare for to_r; return false if it doesn't work **
    *from_r_durn_change = *to_r_durn_change = 0;
    if( to_r == wmts->from_r || !KheMTaskPrepareResource(wmts, to_r) )
      return false;

    ** Set wmts->swappable_wings to all swappable left and right wings **
    HaArrayClear(wmts->swappable_wings);
    for( i = 0;  i < wmts->to_r_max_left_wing_count;  i++ )
    {
      p = HaArray(wmts->left_wing, i);
      if( KheMPartWingSwapCheck(p, wmts->from_r, to_r, false) )
	HaArrayAddLast(wmts->swappable_wings, p);
    }
    for( j = 0;  j < wmts->to_r_max_right_wing_count;  j++ )
    {
      p = HaArray(wmts->right_wing, j);
      if( KheMPartWingSwapCheck(p, wmts->from_r, to_r, false) )
	HaArrayAddLast(wmts->swappable_wings, p);
    }

    ** try one of the two possibilities for the core **
    if( wmts->core.to.effectively_free )
    {
      ** more the core; it must be movable, else fail **
      if( !KheMPartMove(&wmts->core, from_r, to_r, &junk1, &junk2) )
	return false;
      wmts->core.curr_is_swap = false;
    }
    else
    {
      ** swap the core; it must be swappable, else fail **
      if( !KheMPartSwap(&wmts->core, wmts->from_r, to_r, &junk1, &junk2) )
	return false;
      wmts->core.curr_is_swap = true;
    }

    ** recursively try all swaps and non-swaps in the wings **
    DoMTaskWingSwaps(wmts, 0);
  }

  ** apply best solution, or do nothing if none **
  if( KheMTaskHaveOptimal(wmts) )
  {
    ** there is a best solution; apply it and return **
    RedoMTaskBest(wmts, from_r_durn_change, to_r_durn_change);
    return true;
  }
  else
  {
    ** no best solution, false **
    return false;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheWidenedMTaskSetOptimalMoveDebug(KHE_WIDENED_MTASK_SET wmts,      */
/*    KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp)                */
/*                                                                           */
/*  Debug print of an optimal move.                                          */
/*                                                                           */
/*****************************************************************************/

/* ***
void KheWidenedMTaskSetOptimalMoveDebug(KHE_WIDENED_MTASK_SET wmts,
  KHE_RESOURCE to_r, int verbosity, int indent, FILE *fp)
{
  int count = KheMTaskSetMTaskCount(wmts->core.from.mts);
  KheWidenedMTaskSetOpDebug(wmts, "<-opt->", true, to_r,
    wmts->to_r_max_left_wing_count, wmts->to_r_max_right_wing_count,
    0, count - 1, verbosity, indent, fp);
}
*** */


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
/*  FILE:         khe_sm_parallel_solve.c                                    */
/*  DESCRIPTION:  KheParallelSolve().  If you can't compile this file,       */
/*                see the makefile for a workaround.                         */
/*                                                                           */
/*  If you are having trouble compiling this file, try replacing the 1       */
/*  by 0 in line "#define KHE_USE_PTHREAD 1" in khe_solvers.h.                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include "howard_a.h"
#include "howard_n.h"
#include <limits.h>
#include <math.h>
#include <setjmp.h>

#define bool_show(x) ((x) ? "true" : "false")

#if KHE_USE_PTHREAD
#include <pthread.h>
#endif

#if USE_SYSINFO
#include <sys/sysinfo.h>
#endif

/* ***
#if KHE_USE_TI MING
#include <time.h>
#include <sys/time.h>
#endif
*** */

#define DEBUG1 0
#define DEBUG2 1
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0


/*****************************************************************************/
/*                                                                           */
/*  PINST - an instance to be solved in parallel                             */
/*                                                                           */
/*  These variables are used to keep track of where we are up to:            */
/*                                                                           */
/*    wanted_solves                                                          */
/*      The number of solves of pinst->instance wanted.  Initially this is   */
/*      ps->ps_make; if time runs out it is reduced to initiated_solves.     */
/*                                                                           */
/*    initiated_solves                                                       */
/*      The number of solves of pinst->instance that have been initiated.    */
/*                                                                           */
/*  The number of completed solves is HaArrayCount(pinst->all_costs).        */
/*                                                                           */
/*****************************************************************************/

typedef struct psolver_rec *PSOLVER;
typedef HA_ARRAY(KHE_SOLN) ARRAY_KHE_SOLN;
typedef HA_ARRAY(KHE_COST) ARRAY_KHE_COST;

typedef struct pinst_rec {
  PSOLVER		psolver;		/* enclosing solver          */
  KHE_INSTANCE		instance;		/* the instance to solve     */
  KHE_TIMER		ins_timer;		/* the time the solve began  */
  float			running_time;		/* the final running time    */
  KHE_SOLN		first_soln;		/* first solve (optional)    */
  KHE_SOLN		recombined_soln;	/* recombined soln (optional)*/
  ARRAY_KHE_SOLN	best_solns;		/* best solns so far         */
  int			wanted_solves;		/* wanted solves (ps_make)   */
  int			initiated_solves;	/* initiated solves          */
  ARRAY_KHE_COST	all_costs;		/* costs of all solutions    */
  ARRAY_KHE_COST	unique_costs;		/* uniqueified all_costs     */
  bool			have_optimal_soln;	/* true if have optimal soln */
} *PINST;

typedef HA_ARRAY(PINST) ARRAY_PINST;


/*****************************************************************************/
/*                                                                           */
/*  PTHREAD - one thread                                                     */
/*                                                                           */
/*****************************************************************************/

#if KHE_USE_PTHREAD
#define PTHREAD_FIELD pthread_t thread;
#else
#define PTHREAD_FIELD
#endif

#define INHERIT_PTHREAD							\
  HA_ARENA		arena;						\
  PSOLVER		psolver;					\
  KHE_OPTIONS		options;					\
  HA_ARENA_SET		arena_set;					\
  PTHREAD_FIELD

typedef struct pthread_rec {
  INHERIT_PTHREAD
} *PTHREAD;

typedef HA_ARRAY(PTHREAD) ARRAY_PTHREAD;


/*****************************************************************************/
/*                                                                           */
/*  PSOLVER - a parallel solver                                              */
/*                                                                           */
/*  Implementation note.  PSOLVER is a subtype of PTHREAD; as well as being  */
/*  a solver object, it represents the leader thread (the thread that is     */
/*  already running to begin with).                                          */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_PS_TIME_OMIT,
  KHE_PS_TIME_SHARED
} KHE_PS_TIME;

struct psolver_rec {
  INHERIT_PTHREAD
  KHE_ARCHIVE		archive;		/* archive, if any           */
  KHE_GENERAL_SOLVER	solver;			/* solver parameter          */
  KHE_SOLN_TYPE		soln_type;		/* save solns write-only     */
  KHE_TIMER		global_timer;		/* times whole run           */
  KHE_SOLN_GROUP	first_soln_group;	/* holds first solutions     */
  KHE_SOLN_GROUP	soln_group;		/* holds all kept solutions  */
  int			ps_make;		/* ps_make option            */
  bool			ps_no_diversify;	/* ps_no_diversify option    */
  int			ps_keep;		/* ps_keep option            */
  bool			ps_recombine;		/* ps_recombine option       */
  KHE_PS_TIME		ps_time_measure;	/* ps_time_measure option    */
  float			ps_time_limit;		/* time limit, or -1 if none */
  /* bool		ps_reverse_order; */	/* solve in reverse order    */
  size_t		ps_avail_mem;		/* total available memory    */
  char			*cache;			/* intermediate results here */
  int			curr_instance;		/* currently on this         */
  ARRAY_PINST		instances;		/* instances to be solved    */
#if KHE_USE_PTHREAD
  int			ps_threads;		/* ps_threads option         */
  ARRAY_PTHREAD		other_threads;		/* non-leader threads        */
  pthread_mutex_t	mutex;			/* locks everything          */
#endif
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PInst"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PINST PInstMake(PSOLVER ps, KHE_INSTANCE ins, int wanted_solves,         */
/*    float ps_time_limit, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new pinst object, for solving ins.                                */
/*                                                                           */
/*****************************************************************************/

static PINST PInstMake(PSOLVER ps, KHE_INSTANCE ins, int wanted_solves,
  float ps_time_limit, HA_ARENA a)
{
  PINST res;
  HaMake(res, a);
  res->psolver = ps;
  res->instance = ins;
  res->ins_timer = KheTimerMake("instance", ps_time_limit, a);
  res->running_time = 0.0;
  res->first_soln = NULL;
  res->recombined_soln = NULL;
  HaArrayInit(res->best_solns, a);
  res->wanted_solves = wanted_solves;
  res->initiated_solves = 0;
  HaArrayInit(res->all_costs, a);
  HaArrayInit(res->unique_costs, a);
  res->have_optimal_soln = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool PInstSolveWanted(PINST pinst, int *diversifier, bool *first)        */
/*                                                                           */
/*  If pinst wants a solve, that is, if it has not yet initiated ps_make     */
/*  solves, we don't yet have an obviously optimal solution, and time is     */
/*  available, return true, otherwise false.                                 */
/*                                                                           */
/*  If true is returned, pinst understands that a new solve will actually    */
/*  be initiated, and it updates itself accordingly, and returns the         */
/*  appropriate diversifier to use for the new solve.  This diversifier      */
/*  will be ignored by the caller if the ps_no_diversify option is set.      */
/*                                                                           */
/*  Also, if this is the first solve of this instance, *first is set to      */
/*  true, otherwise false.                                                   */
/*                                                                           */
/*  This function must be run while the enclosing psolver is locked.         */
/*  Accordingly we do as little as possible; in particular we do not         */
/*  create a new solution.                                                   */
/*                                                                           */
/*****************************************************************************/
static bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins);

static bool PInstSolveWanted(PINST pinst, int *diversifier, bool *first)
{
  if( pinst->initiated_solves == 0 )
    KheTimerResetStartTime(pinst->ins_timer);
  if( pinst->initiated_solves >= pinst->wanted_solves )
  {
    /* we've initiated all the solves of pinst that we want */
    return *diversifier = -1, *first = false, false;
  }
  else if( pinst->have_optimal_soln )
  {
    /* we already have an optimal solution */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: only %d of %d solves "
	"started (have optimal solution)\n", KheInstanceId(pinst->instance),
	pinst->initiated_solves, pinst->wanted_solves);
    pinst->wanted_solves = pinst->initiated_solves;
    return *diversifier = -1, *first = false, false;
  }
  else if( KheTimerTimeLimitReached(pinst->ins_timer) )
  {
    /* we're out of time */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: only %d of %d solves "
	"started (time limit reached)\n", KheInstanceId(pinst->instance),
	pinst->initiated_solves, pinst->wanted_solves);
    pinst->wanted_solves = pinst->initiated_solves;
    return *diversifier = -1, *first = false, false;
  }
  else if( pinst->initiated_solves == 0 &&
    PSolverHasCachedSolns(pinst->psolver, pinst->instance) )
  {
    /* we've already got cached solutions for inst */
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: using cached solutions from"
	" %s\n", KheInstanceId(pinst->instance), pinst->psolver->cache);
    pinst->wanted_solves = pinst->initiated_solves;
    return *diversifier = -1, *first = false, false;
  }
  else
  {
    /* OK to start off another solve of pinst */
    *diversifier = pinst->initiated_solves++;
    *first = (pinst->initiated_solves == 1);
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: starting %ssolve %d%s\n",
	KheInstanceId(pinst->instance),
	pinst->psolver->soln_group == NULL ? "NOT SAVING " : "",
	pinst->initiated_solves,
	pinst->initiated_solves == pinst->wanted_solves ? " (last)" : "");
    return true;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstSolveEnded(PINST pinst, KHE_SOLN soln, bool first,             */
/*    KHE_SOLN *unwanted_soln)                                               */
/*                                                                           */
/*  Add soln to pinst.  If first is true, this is the first solution.  If    */
/*  *unwanted_soln is non-NULL on return, it is unwanted (it is neither      */
/*  the first solution nor a best solution) and should be deleted.           */
/*                                                                           */
/*  This function must be run while the enclosing psolver is locked.         */
/*  Accordingly we do as little as possible; in particular we do not         */
/*  delete any unwanted solution.                                            */
/*                                                                           */
/*****************************************************************************/
static void PInstDebug(PINST pinst, int indent, FILE *fp);

static void PInstSolveEnded(PINST pinst, KHE_SOLN soln, bool first,
  KHE_SOLN *unwanted_soln)
{
  int i;  KHE_SOLN best_soln;  KHE_COST soln_cost;

  /* set have_optimal_soln if soln is optimal */
  if( KheSolnCost(soln) <= KheMonitorLowerBound((KHE_MONITOR) soln) )
    pinst->have_optimal_soln = true;

  /* add soln_cost to all_costs and unique_costs */
  soln_cost = KheSolnCost(soln);
  HaArrayAddLast(pinst->all_costs, soln_cost);
  HaArrayAddLast(pinst->unique_costs, soln_cost);

  /* save soln as the first solution, if appropriate */
  if( first && pinst->psolver->first_soln_group != NULL )
  {
    HnAssert(pinst->first_soln == NULL, "PInstSolveEnded internal error");
    pinst->first_soln = soln;
  }

  /* insert soln into the list of best solutions */
  for( i = 0;  i < HaArrayCount(pinst->best_solns);  i++ )
  {
    best_soln = HaArray(pinst->best_solns, i);
    if( soln_cost < KheSolnCost(best_soln) )
      break;
  }
  HaArrayAdd(pinst->best_solns, i, soln);  /* works even at end */

  /* optionally remove the worst solution and make it unwanted */
  if( HaArrayCount(pinst->best_solns) <= pinst->psolver->ps_keep )
  {
    /* still accumulating solutions here, none are unwanted */
    *unwanted_soln = NULL;
  }
  else
  {
    /* remove the worst solution and make it unwanted if not kept as first */
    *unwanted_soln = HaArrayLastAndDelete(pinst->best_solns);
    if( *unwanted_soln == pinst->first_soln )
      *unwanted_soln = NULL;
  }
  pinst->running_time = KheTimerElapsedTime(pinst->ins_timer);
  if( DEBUG5 )
    fprintf(stderr, "  PInstSolveEnded updating %s running time to %.1f\n",
      KheInstanceId(pinst->instance), pinst->running_time);
  if( DEBUG2 && HaArrayCount(pinst->all_costs) == pinst->wanted_solves )
    PInstDebug(pinst, 2, stderr);
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstEndRun(PINST pinst, bool set_time)                             */
/*                                                                           */
/*  Solving of pinst has ended.  Don't actually delete pinst (it gets        */
/*  deleted when the solver's arena is deleted), but do what is needed       */
/*  when pinst will not be used any more.  If ps->soln_group is NULL,        */
/*  delete all the saved solutions.  Otherwise, optionally set their         */
/*  running times, and add them to ps->soln_group.                           */
/*                                                                           */
/*  Implementation note.  Because it may add solutions to ps->soln_group,    */
/*  PInstEndRun must be called single-threaded.                              */
/*                                                                           */
/*****************************************************************************/

static void PInstEndRun(PINST pinst, bool set_time)
{
  int i;  KHE_SOLN soln;  PSOLVER ps;  bool soln_added;  FILE *fp;

  /* if wanted, save a copy of pinst->first_soln; then delete it */
  soln_added = false;
  ps = pinst->psolver;
  if( pinst->first_soln != NULL )
  {
    /* if wanted, save a copy of pinst->first_soln in ps->first_soln_group */
    if( ps->first_soln_group != NULL )
    {
      KheSolnGroupAddSoln(ps->first_soln_group,
	KheSolnCopy(pinst->first_soln, ps->arena_set));
      soln_added = true;
    }

    /* delete pinst->first_soln unless it is also a best soln */
    if( !HaArrayContains(pinst->best_solns, pinst->first_soln, &i) )
      KheSolnDelete(pinst->first_soln);
    pinst->first_soln = NULL;
  }

  /* if wanted, save copies of the best solns; then delete them */
  HaArrayForEach(pinst->best_solns, soln, i)
  {
    /* if wanted, save a copy of soln in ps->soln_group */
    if( ps->soln_group != NULL )
    {
      if( set_time )
      {
	if( DEBUG5 )
	  fprintf(stderr, "  PInstEndRun set soln %p running time to %.1f\n",
            (void *) soln, pinst->running_time);
	KheSolnSetRunningTime(soln, pinst->running_time);
      }
      KheSolnGroupAddSoln(ps->soln_group, KheSolnCopy(soln, ps->arena_set));
      soln_added = true;
      if( DEBUG5 )
	fprintf(stderr, "   KheSolnGroupAddSoln(%p, soln %p)\n",
	  (void *) ps->soln_group, (void *) soln);
    }

    /* delete soln */
    KheSolnDelete(soln);
  }
  HaArrayClear(pinst->best_solns);

  /* write the archive if we've added a solution and we're caching */
  if( soln_added && ps->cache != NULL )
  {
    HnAssert(ps->archive != NULL, "PInstEndRun internal error");
    if( DEBUG2 )
      fprintf(stderr, "  parallel solve of %s: writing cache file %s "
	"after last solve\n", KheInstanceId(pinst->instance), ps->cache);
    fp = fopen(ps->cache, "w");
    HnAssert(fp != NULL, "KheArchiveParallelSolve: cannot open cache "
      "file \"%s\" for writing", ps->cache);
    KheArchiveWrite(ps->archive, false, fp);
    fclose(fp);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN PInstEndRunSingle(PINST pinst, bool set_time)                   */
/*                                                                           */
/*  Similar to PInstEndRun, except there is supposed to be exactly one       */
/*  soln, which is returned.                                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN PInstEndRunSingle(PINST pinst, bool set_time)
{
  KHE_SOLN soln, res;
  HnAssert(HaArrayCount(pinst->best_solns) == 1,
    "PInstEndRunSingle internal error");
  soln = HaArrayFirst(pinst->best_solns);
  if( set_time )
    KheSolnSetRunningTime(soln, pinst->running_time);
  /* KheSolnSetArenaSet(soln, pinst->psolver->arena_set); */
  res = KheSolnCopy(soln, pinst->psolver->arena_set);
  KheSolnDelete(soln);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheCostIncreasingCmp(const void *t1, const void *t2)                 */
/*                                                                           */
/*  Comparison function for sorting an array of costs into increasing order. */
/*                                                                           */
/*****************************************************************************/

static int KheCostIncreasingCmp(const void *t1, const void *t2)
{
  KHE_COST cost1 = * (KHE_COST *) t1;
  KHE_COST cost2 = * (KHE_COST *) t2;
  return KheCostCmp(cost1, cost2);
}


/*****************************************************************************/
/*                                                                           */
/*  void PInstDebug(PINST pinst, int indent, FILE *fp)                       */
/*                                                                           */
/*  Debug print of pinst.                                                    */
/*                                                                           */
/*****************************************************************************/
static int PSolverThreadCount(PSOLVER ps);

static void PInstDebug(PINST pinst, int indent, FILE *fp)
{
  KHE_COST cost;  int i;  char buff[20];  KHE_SOLN best_soln;
  fprintf(fp, "%*s[ \"%s\", ", indent, "", KheInstanceId(pinst->instance));
  if( pinst->wanted_solves <= 0 )
    fprintf(fp, "0 solutions requested ]\n");
  else if( pinst->wanted_solves == 1 )
  {
    if( pinst->wanted_solves == HaArrayCount(pinst->all_costs) )
      fprintf(fp, "1 solution, in %s: cost %.5f ]\n",
        KheTimeShow(pinst->running_time, buff),
        KheCostShow(HaArrayFirst(pinst->all_costs)));
    else
      fprintf(fp, "1 solution requested, 0 completed ]\n");
  }
  else
  {
    HaArraySort(pinst->all_costs, &KheCostIncreasingCmp);
    HaArraySortUnique(pinst->unique_costs, &KheCostIncreasingCmp);
    fprintf(fp, "%d thread%s, ", PSolverThreadCount(pinst->psolver),
      PSolverThreadCount(pinst->psolver) == 1 ? "" : "s");
    if( pinst->wanted_solves == HaArrayCount(pinst->all_costs) )
      fprintf(fp, "%d solves, ", HaArrayCount(pinst->all_costs));
    else
      fprintf(fp, "%d solves (%d completed), ", pinst->wanted_solves,
	HaArrayCount(pinst->all_costs));
    if( pinst->first_soln != NULL )
      fprintf(fp, "first %.5f, ", KheCostShow(KheSolnCost(pinst->first_soln)));
    fprintf(fp, "%s:", KheTimeShow(pinst->running_time, buff));
    HaArrayForEach(pinst->all_costs, cost, i)
    {
      if( i % 8 == 0 )
	fprintf(fp, "\n%*s  ", indent, "");
      else
	fprintf(fp, " ");
      fprintf(fp, "%.5f", KheCostShow(cost));
    }
    fprintf(fp, "\n%*s]", indent, "");
    fprintf(fp, " %d distinct cost%s",
      HaArrayCount(pinst->unique_costs),
      HaArrayCount(pinst->unique_costs) == 1 ? "" : "s");
    if( HaArrayCount(pinst->best_solns) > 0 )
    {
      best_soln = HaArrayFirst(pinst->best_solns);
      fprintf(fp, ", best soln (cost %.5f) has diversifier %d",
	KheCostShow(KheSolnCost(best_soln)), KheSolnDiversifier(best_soln));
    }
    fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "PSolver"                                                      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  PTHREAD PSolverThreadMake(PSOLVER ps)                                    */
/*                                                                           */
/*  Make a new thread object for ps.  This thread object does not            */
/*  represent the leader thread, because ps itself (upcast to PTHREAD)       */
/*  represents the leader thread.                                            */
/*                                                                           */
/*  Implementation note 1.  It would be good to initialize the thread field  */
/*  here to a NULL value, but the pthread module does not seem to supply     */
/*  such a value.  It even says explicitly that you can't apply `==' to the  */
/*  thread type.  Setting the thread running immediately would solve this    */
/*  problem, but that leads to contention on ps->arena_set.  So we leave     */
/*  the thread field uninitialized.                                          */
/*                                                                           */
/*  Implementation note 2.  Care is needed to avoid contention for memory    */
/*  when running threads.  This is not a problem for PSolverThreadMake,      */
/*  because it is run single-threaded.  It gives its thread its own arena    */
/*  and its own arena set lying in that arena, and copies ps->options, so    */
/*  that there can be no contention when the thread runs.                    */
/*                                                                           */
/*****************************************************************************/

static PTHREAD PSolverThreadMake(PSOLVER ps)
{
  PTHREAD res;  HA_ARENA a;  HA_ARENA_SET as;
  as = HaArenaSetMake();
  a = HaArenaMake(as);
  HaMake(res, a);
  res->arena = a;
  res->psolver = ps;
  res->options = (ps->options == NULL ? NULL : KheOptionsCopy(ps->options, a));
  res->arena_set = as;
  return res;
  /* ***
  PTHREAD res;  HA_ARENA a;
  a = HaArenaSetArenaBegin(ps->arena_set, false);
  HaMake(res, a);
  res->arena = a;
  res->psolver = ps;
  res->options = (ps->options == NULL ? NULL : KheOptionsCopy(ps->options, a));
  res->arena_set = (ps->arena_set == NULL ? NULL : HaArenaSetMake(a));
  ** res->thread is left uninitialized, as explained above **
  return res;
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverThreadDelete(PSOLVER ps, PTHREAD pt)                         */
/*                                                                           */
/*  Delete pt, including merging its arena set's arenas back to ps.          */
/*                                                                           */
/*****************************************************************************/

static void PSolverThreadDelete(PSOLVER ps, PTHREAD pt)
{
  HA_ARENA_SET as;
  as = pt->arena_set;
  HaArenaDelete(pt->arena);
  HaArenaSetMerge(ps->arena_set, as);
  /* ***
  if( ps->arena_set != NULL )
    HaArenaSetMerge(ps->arena_set, pt->arena_set);
  HaArenaSetArenaEnd(ps->arena_set, pt->arena);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverThreadLimitMemory(PTHREAD pt, size_t thread_memory_limit)    */
/*                                                                           */
/*  Limit the memory used by thread pt to thread_memory_limit bytes.         */
/*                                                                           */
/*****************************************************************************/

static void PSolverThreadLimitMemory(PTHREAD pt, size_t thread_memory_limit)
{
  HaArenaSetLimitMemory(pt->arena_set, thread_memory_limit);
}


/*****************************************************************************/
/*                                                                           */
/*  void SolnTypeCheck(KHE_SOLN_TYPE soln_type, char *fn_name)               */
/*                                                                           */
/*  Check that soln_type is OK.                                              */
/*                                                                           */
/*****************************************************************************/

static void SolnTypeCheck(KHE_SOLN_TYPE soln_type, char *fn_name)
{
  switch( soln_type )
  {
    case KHE_SOLN_INVALID_PLACEHOLDER:

      HnAbort("%s: soln_type may not be KHE_SOLN_INVALID_PLACEHOLDER", fn_name);
      break;

    case KHE_SOLN_BASIC_PLACEHOLDER:
    case KHE_SOLN_WRITABLE_PLACEHOLDER:
    case KHE_SOLN_ORDINARY:

      /* these are the acceptable values, so do nothing */
      break;

    default:

      HnAbort("%s: invalid soln_type (value is %d)", fn_name, soln_type);
      break;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  PSOLVER PSolverMake(KHE_ARCHIVE archive, KHE_GENERAL_SOLVER solver,      */
/*    KHE_OPTIONS options, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,         */
/*    KHE_SOLN_GROUP first_soln_group, KHE_SOLN_GROUP soln_group,            */
/*    int ps_threads, int ps_make, bool ps_no_diversify, int ps_keep,        */
/*    KHE_PS_TIME ps_time_measure, float ps_time_limit,                      */
/*    bool ps_reverse_order, size_t ps_avail_mem, char *cache, HA_ARENA a)   */
/*                                                                           */
/*  Make a new psolver object with these attributes, and also with the       */
/*  appropriate number of thread objects, but initially with no instances.   */
/*                                                                           */
/*****************************************************************************/

static PSOLVER PSolverMake(KHE_ARCHIVE archive, KHE_GENERAL_SOLVER solver,
  KHE_OPTIONS options, KHE_SOLN_TYPE soln_type, HA_ARENA_SET as,
  KHE_SOLN_GROUP first_soln_group, KHE_SOLN_GROUP soln_group,
  int ps_threads, int ps_make, bool ps_no_diversify, int ps_keep,
  bool ps_recombine, KHE_PS_TIME ps_time_measure, float ps_time_limit,
  /* bool ps_reverse_order, */ size_t ps_avail_mem, char *cache, HA_ARENA a)
{
  PSOLVER res;  int i;  PTHREAD pt;

  /* fields inherited from PTHREAD */
  HaMake(res, a);
  res->arena = a;
  res->psolver = res;
  res->options = options;
  res->arena_set = as;
  /* res->thread is left uninitialized, as explained for PSolverThreadMake */

  /* other fields */
  res->archive = archive;
  res->solver = solver;
  res->soln_type = soln_type;
  res->global_timer = KheTimerMake("global", KHE_NO_TIME, a);
  res->first_soln_group = first_soln_group;
  res->soln_group = soln_group;
  if( DEBUG5 )
    fprintf(stderr, "  psolver->soln_group = %p\n", (void *) res->soln_group);
  res->ps_make = ps_make;
  res->ps_no_diversify = ps_no_diversify;
  res->ps_keep = ps_keep;
  res->ps_recombine = ps_recombine;
  res->ps_time_measure = ps_time_measure;
  res->ps_time_limit = ps_time_limit;
  /* res->ps_reverse_order = ps_reverse_order; */
  res->ps_avail_mem = ps_avail_mem;
  if( ps_avail_mem > 0 )
    PSolverThreadLimitMemory((PTHREAD) res, ps_avail_mem / ps_threads);
  res->cache = cache;
  res->curr_instance = 0;
  HaArrayInit(res->instances, a);
#if KHE_USE_PTHREAD
  res->ps_threads = ps_threads;
  HaArrayInit(res->other_threads, a);
  for( i = 1;  i < res->ps_threads;  i++ )
  {
    pt = PSolverThreadMake(res);
    if( ps_avail_mem > 0 )
      PSolverThreadLimitMemory(pt, ps_avail_mem / ps_threads);
    HaArrayAddLast(res->other_threads, pt);
    if( DEBUG6 )
      fprintf(stderr, "  adding thread %d (pt = %p)\n", i, (void *) pt);
  }
  pthread_mutex_init(&res->mutex, NULL);
#endif
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverDelete(PSOLVER ps)                                           */
/*                                                                           */
/*  Delete ps and its thread objects, including tidying up the arena sets.   */
/*                                                                           */
/*****************************************************************************/

static void PSolverDelete(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  PTHREAD pt;  int i;
  HaArrayForEach(ps->other_threads, pt, i)
    PSolverThreadDelete(ps, pt);
#endif
  HaArenaDelete(ps->arena);
  /* HaArenaSetArenaEnd(ps->arena_set, ps->arena); */
}


/*****************************************************************************/
/*                                                                           */
/*  int PSolverThreadCount(PSOLVER ps)                                       */
/*                                                                           */
/*  Return the number of threads requested, or 1 if not using threads.       */
/*                                                                           */
/*****************************************************************************/

static int PSolverThreadCount(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  return ps->ps_threads;
#else
  return 1;
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverLock(PSOLVER ps)                                             */
/*                                                                           */
/*  Lock ps, or do nothing if we are not running multi-threaded.             */
/*                                                                           */
/*****************************************************************************/

static void PSolverLock(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  pthread_mutex_lock(&ps->mutex);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverUnLock(PSOLVER ps)                                           */
/*                                                                           */
/*  Unlock ps, or do nothing if we are not running multi-threaded.           */
/*                                                                           */
/*****************************************************************************/

static void PSolverUnLock(PSOLVER ps)
{
#if KHE_USE_PTHREAD
  pthread_mutex_unlock(&ps->mutex);
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins)                 */
/*                                                                           */
/*  Return true if ps has cached solutions for ins in ps->soln_group or      */
/*  ps->first_soln_group.                                                    */
/*                                                                           */
/*****************************************************************************/

static bool PSolverHasCachedSolns(PSOLVER ps, KHE_INSTANCE ins)
{
  if( ps->cache == NULL )
    return false;
  else if( ps->soln_group != NULL &&
    KheSolnSetSolnCount(KheSolnGroupInstanceSolnSet(ps->soln_group, ins)) > 0 )
    return true;
  else if( ps->first_soln_group != NULL && KheSolnSetSolnCount(
    KheSolnGroupInstanceSolnSet(ps->first_soln_group, ins)) > 0 )
    return true;
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool PSolverSolveWanted(PSOLVER ps, PINST *res, int *diversifier)        */
/*                                                                           */
/*  If there is still work to do, set *res to the pinst needing solving,     */
/*  *diversifier to a suitable diversifier, and *first to true when this     */
/*  is the first solve for this instance, and return true.  Otherwise        */
/*  return false.  Also update ps.                                           */
/*                                                                           */
/*  Implementation note.  Because ps is locked during this operation,        */
/*  we prefer to do as little as possible here, i.e. we do not create a      */
/*  new solution.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool PSolverSolveWanted(PSOLVER ps, PINST *res, int *diversifier,
  bool *first)
{
  PINST pinst;
  PSolverLock(ps);
  *res = NULL;
  *diversifier = -1;
  *first = false;
  while( ps->curr_instance < HaArrayCount(ps->instances) )
  {
    pinst = HaArray(ps->instances, ps->curr_instance);
    if( PInstSolveWanted(pinst, diversifier, first) )
    {
      *res = pinst;
      break;
    }
    ps->curr_instance++;
  }
  PSolverUnLock(ps);
  return *res != NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverSolveEnded(PSOLVER ps, PINST pinst, KHE_SOLN soln,           */
/*    bool first)                                                            */
/*                                                                           */
/*  Add soln to pinst.  If first is true, this is the solution made by the   */
/*  first solve of its instance, so save it in first_soln_group if any.      */
/*                                                                           */
/*****************************************************************************/

static void PSolverSolveEnded(PSOLVER ps, PINST pinst, KHE_SOLN soln,
  bool first)
{
  KHE_SOLN unwanted_soln;
  PSolverLock(ps);
  PInstSolveEnded(pinst, soln, first, &unwanted_soln);
  PSolverUnLock(ps);
  if( unwanted_soln != NULL )
    KheSolnDelete(unwanted_soln);
}


/*****************************************************************************/
/*                                                                           */
/*  void *PSolverRunOneThread(void *arg)                                     */
/*                                                                           */
/*  Thread execution function.                                               */
/*                                                                           */
/*  Implementation note.  All memory allocated by this function comes from   */
/*  the thread's own arena and arena set.                                    */
/*                                                                           */
/*****************************************************************************/

static void *PSolverRunOneThread(void *arg)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;  bool first;  jmp_buf env;
  int diversifier;  KHE_OPTIONS options;  PTHREAD pt;  char description[40];
  pt = (PTHREAD) arg;
  ps = pt->psolver;
  while( PSolverSolveWanted(ps, &pinst, &diversifier, &first) )
  {
    if( DEBUG3 )
      fprintf(stderr, "  PSolverRunOneThread creating new soln\n");
      /* HaArenaSetDebug(pt->arena_set, 1, 4, stderr); */
    soln = KheSolnMake(pinst->instance, pt->arena_set);
    if( !ps->ps_no_diversify )
      KheSolnSetDiversifier(soln, diversifier);
    options = (pt->options == NULL ? NULL :
      KheOptionsCopy(pt->options, pt->arena));
    if( setjmp(env) == 0 )
    {
      KheSolnJmpEnvBegin(soln, &env);
      soln = ps->solver(soln, options);
      KheSolnJmpEnvEnd(soln);
      sprintf(description, "diversifier %d", KheSolnDiversifier(soln));
    }
    else
    {
      if( DEBUG7 )
	fprintf(stderr, "  long jump (diversifier %d ran out of memory)\n",
	  KheSolnDiversifier(soln));
      KheSolnJmpEnvEnd(soln);
      sprintf(description, "diversifier %d (ran out of memory)",
	KheSolnDiversifier(soln));
    }
    KheSolnTypeReduce(soln, ps->soln_type, NULL);
    KheSolnSetDescription(soln, description);  /* will copy description */
    PSolverSolveEnded(ps, pinst, soln, first);
  }
  return NULL;
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverEvenOutArenas(PSOLVER ps)                                    */
/*                                                                           */
/*  It's likely that there will be more arenas in ps's arena set than in     */
/*  the other threads' arena sets.  While that is the case, even them out.   */
/*                                                                           */
/*****************************************************************************/

/* *** obsolete, free chunks are held in the arena set now
#if KHE_USE_PTHREAD
static void PSolverEvenOutArenas(PSOLVER ps)
{
  bool progressing;  int i, pt_count;  HA_ARENA a;  PTHREAD pt;
  do
  {
    progressing = false;
    HaArrayForEach(ps->other_threads, pt, i)
    {
      pt_count = HaArenaSetArenaCount(pt->arena_set);
      if( pt_count < HaArenaSetArenaCount(ps->arena_set) - 1 )
      {
	a = HaArenaSetLastAndDelete(ps->arena_set);
	HaArenaSetAddArena(pt->arena_set, a);
	progressing = true;
      }
    }
  } while( progressing );
}
#endif
*** */


/*****************************************************************************/
/*                                                                           */
/*  void PSolverRun(PSOLVER ps)                                              */
/*                                                                           */
/*  Run ps (called by the leader thread).                                    */
/*                                                                           */
/*****************************************************************************/

static void PSolverRun(PSOLVER ps)
{
  if( DEBUG4 )
    fprintf(stderr, "[ PSolverRun(ps); before evening out\n");
      /* HaArenaSetArenaCount(ps->arena_set)); */
#if KHE_USE_PTHREAD
  {
    int i;  PTHREAD pt;

    /* even out the number of arenas in each thread */
    /* *** PSolverEvenOutArenas(ps);
    if( DEBUG4 )
    {
      fprintf(stderr, " after evening out: %d arenas\n",
	HaArenaSetArenaCount(ps->arena_set));
      HaArrayForEach(ps->other_threads, pt, i)
	fprintf(stderr, "  other_thread %d: %d arenas\n", i,
	  HaArenaSetArenaCount(pt->arena_set));
    }
    *** */

    /* set the other threads running from the first instance */
    ps->curr_instance = 0;
    HaArrayForEach(ps->other_threads, pt, i)
    {
      if( DEBUG6 )
	fprintf(stderr, "  calling pthread_create %d (pt = %p)\n",
	  i, (void *) pt);
      pthread_create(&pt->thread, NULL, &PSolverRunOneThread, (void *) pt);
    }

    /* run this thread */
    ps->thread = pthread_self();  /* actually unused */
    PSolverRunOneThread((void *) (PTHREAD) ps);

    /* wait for the other threads to terminate */
    HaArrayForEach(ps->other_threads, pt, i)
      pthread_join(pt->thread, NULL);
  }
#else
  PSolverRunOneThread((void *) (PTHREAD) ps);
#endif
  if( DEBUG4 )
    fprintf(stderr, "] PSolverRun\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void PSolverLoadInstance(PSOLVER ps, KHE_INSTANCE ins)                   */
/*                                                                           */
/*  Load ins into ps.                                                        */
/*                                                                           */
/*****************************************************************************/

static PINST PSolverLoadInstance(PSOLVER ps, KHE_INSTANCE ins)
{
  PINST res;
  res = PInstMake(ps, ins, ps->ps_make, ps->ps_time_limit, ps->arena);
  HaArrayAddLast(ps->instances, res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "public functions"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN_GROUP KheGetSolnGroup(KHE_ARCHIVE archive,                      */
/*    char *sg_name, bool allow_existing)                                    */
/*                                                                           */
/*  Get a soln group from archive whose name is sg_name.  If allow_existing  */
/*  is true, this solution group may be an existing soln group if there      */
/*  is one with a suitable name, otherwise it must be new.                   */
/*                                                                           */
/*  If sg_name is NULL, return NULL.                                         */
/*                                                                           */
/*****************************************************************************/

static KHE_SOLN_GROUP KheGetSolnGroup(KHE_ARCHIVE archive,
  char *sg_name, bool allow_existing)
{
  static char version_str[200];  KHE_SOLN_GROUP res;

  /* return NULL if sg_name is NULL */
  if( sg_name == NULL )
    return NULL;

  if( !KheArchiveRetrieveSolnGroup(archive, sg_name, &res) )
  {
    /* no existing soln group with this name, so make one */
    sprintf(version_str, "Produced by KHE Version %s", KheVersionNumber());
    if( !KheSolnGroupMake(archive, sg_name, &res) )
      HnAbort("KheGetSolnGroup internal error");
    KheSolnGroupSetMetaData(res, "Jeffrey H. Kingston",
      KheDateToday(), version_str, NULL, NULL);
    return res;
  }
  else if( allow_existing )
  {
    /* existing soln group with this name; it's allowed, so return it */
    return res;
  }
  else
  {
    /* existing soln group with this name; it's not allowed, so abort */
    HnAbort("KheArchiveParallelSolve: soln group \"%s\" already present",
      sg_name);
    return NULL;  /* keep compiler happy */
  }
}


/* *** old version that always makes a new soln group
static KHE_SOLN_GROUP KheMakeSolnGroup(KHE_ARCHIVE archive,
  KHE_OPTIONS options, char *option_name)
{
  static char id[200], version_str[200];
  int i;  char *sg_name;  KHE_SOLN_GROUP res;

  ** consult options for solution group name; return NULL if nothing there **
  sg_name = KheOptionsGet(options, option_name, NULL);
  if( sg_name == NULL )
    return NULL;

  ** extend sg_name into a name that is not already in use **
  sprintf(id, "%s", sg_name);
  for( i = 1;  KheArchiveRetrieveSolnGroup(archive, id, &res);  i++ )
    sprintf(id, "%s_%d", sg_name, i);

  ** construct version_str **
  sprintf(version_str, "Produced by KHE Version %s", KheVersionNumber());

  ** make and return a new soln group **
  if( !KheSolnGroupMake(archive, id, &res) )
    HnAbort("KheMakeSolnGroup internal error");
  KheSolnGroupSetMetaData(res, "Jeffrey H. Kingston",
    KheDateToday(), version_str, NULL, NULL);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  size_t GetAvailMemFromSysInfo(void)                                      */
/*                                                                           */
/*  Get available memory from the Linux sysinfo system call.                 */
/*                                                                           */
/*****************************************************************************/

#if USE_SYSINFO
static size_t GetAvailMemFromSysInfo(void)
{
  struct sysinfo info;
  return (sysinfo(&info) == 0 ? (size_t) info.freeram : 0);
}
#endif


/*****************************************************************************/
/*                                                                           */
/*  void KheGetPSOptions(KHE_OPTIONS options, bool keep_is_one,              */
/*    int *ps_threads, int *ps_make, bool *ps_no_diversify, int *ps_keep,    */
/*    char **ps_soln_group, char **ps_first_soln_group,                      */
/*    KHE_PS_TIME *ps_time_measure, float *ps_time_limit,                    */
/*    size_t *ps_avail_mem, bool *ps_use_cache)                              */
/*                                                                           */
/*  Get the parallel solve options from options.  If keep_is_one, set        */
/*  ps_keep to 1 regardless of what options says.                            */
/*                                                                           */
/*****************************************************************************/

static void KheGetPSOptions(KHE_OPTIONS options, bool keep_is_one,
  int *ps_threads, int *ps_make, bool *ps_no_diversify, int *ps_keep,
  bool *ps_recombine, char **ps_soln_group, char **ps_first_soln_group,
  KHE_PS_TIME *ps_time_measure, float *ps_time_limit,
  size_t *ps_avail_mem, bool *ps_use_cache)
{
  char *str;

  /* ps_threads */
  *ps_threads = KheOptionsGetInt(options, "ps_threads", 1);
  HnAssert(*ps_threads >= 1, "KheParallelSolve: ps_threads < 1");

  /* ps_make */
  *ps_make = KheOptionsGetInt(options, "ps_make", 1);
  HnAssert(*ps_make >= 1, "KheParallelSolve: ps_make < 1");

  /* reduce ps_threads to ps_make if it is larger */
  /* *** no, not necessarily - we may want to solve instances in parallel
  if( *ps_threads > *ps_make )
    *ps_threads = *ps_make;
  *** */

  /* ps_no_diversify */
  *ps_no_diversify = KheOptionsGetBool(options, "ps_no_diversify", false);

  /* ps_keep */
  *ps_keep = (keep_is_one ? 1 : KheOptionsGetInt(options, "ps_keep", 1));
  HnAssert(*ps_keep >= 0, "KheParallelSolve: ps_keep < 0");

  /* ps_recombine */
  *ps_recombine = KheOptionsGetBool(options, "ps_recombine", true);

  /* ps_time_measure */
  str = KheOptionsGet(options, "ps_time_measure", "auto");
  if( strcmp(str, "auto") == 0 )
    *ps_time_measure = (*ps_keep < *ps_make ?
      KHE_PS_TIME_SHARED : KHE_PS_TIME_OMIT);
  else if( strcmp(str, "omit") == 0 )
    *ps_time_measure = KHE_PS_TIME_OMIT;
  else if( strcmp(str, "shared") == 0 )
    *ps_time_measure = KHE_PS_TIME_SHARED;
  else
  {
    HnAbort("KheParallelSolve: ps_time_measure option has unknown value \"%s\"",
      str);
    *ps_time_measure = KHE_PS_TIME_OMIT;  /* keep compiler happy */
  }

  /* ps_time_limit */
  *ps_time_limit =
    KheTimeFromString(KheOptionsGet(options, "ps_time_limit", "-"));

  /* ps_soln_group */
  *ps_soln_group = KheOptionsGet(options, "ps_soln_group", NULL);

  /* ps_first_soln_group */
  *ps_first_soln_group = KheOptionsGet(options, "ps_first_soln_group", NULL);

  /* ps_reverse_order */
  /* *** withdrawn
  *ps_reverse_order = KheOptionsGetBool(options, "ps_reverse_order", false);
  *** */

  /* ps_avail_mem */
#if USE_SYSINFO
  str = KheOptionsGet(options, "ps_avail_mem", "sysinfo");
  if( strcmp(str, "sysinfo") == 0 )
    *ps_avail_mem = GetAvailMemFromSysInfo();
  else if( sscanf(str, "%lu", ps_avail_mem) != 1 )
    HnAbort("KheParallelSolve: invalid ps_avail_mem value \"%s\"", str);
#else
  str = KheOptionsGet(options, "ps_avail_mem", "0");
  if( strcmp(str, "sysinfo") == 0 )
    HnAbort("KheParallelSolve: invalid ps_avail_mem value \"sysinfo\" "
      "(USE_SYSINFO not set)");
  else if( sscanf(str, "%lu", ps_avail_mem) != 1 )
    HnAbort("KheParallelSolve: invalid ps_avail_mem value \"%s\"", str);
#endif

  /* ps_use_cache */
  *ps_use_cache = KheOptionsGetBool(options, "ps_use_cache", false);
}


/*****************************************************************************/
/*                                                                           */
/*  char *ElapsedTime(float secs)                                            */
/*                                                                           */
/*  Return a string display of the elapsed time, in static memory.           */
/*                                                                           */
/*****************************************************************************/

/* *** moved to KheTimeShow in khe_sm_timer.c
static char *ElapsedTime(float secs)
{
  static char res[50];
  if( secs > 300.0 )
    sprintf(res, "%.1f mins", secs / 60.0);
  else
    sprintf(res, "%.1f secs", secs);
  return res;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheArchiveParallelSolve(KHE_ARCHIVE archive,                        */
/*    KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,                        */
/*    KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)                              */
/*                                                                           */
/*  Solve the instances of archive in parallel.                              */
/*                                                                           */
/*****************************************************************************/

void KheArchiveParallelSolve(KHE_ARCHIVE archive,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;  bool resumed;
  KHE_SOLN_GROUP first_soln_group, soln_group;  HA_ARENA a;
  KHE_INSTANCE ins;  KHE_PS_TIME ps_time_measure;  int i;
  float ps_time_limit; int ps_threads, ps_make, ps_keep;
  bool ps_recombine, ps_no_diversify /* , ps_reverse_order */;
  char buff[20];  size_t ps_avail_mem;  bool ps_use_cache;  FILE *fp;
  char *ps_soln_group, *ps_first_soln_group, *cache;
  KML_ERROR ke;

  /* get options */
  KheGetPSOptions(options, false, &ps_threads, &ps_make, &ps_no_diversify,
    &ps_keep, &ps_recombine, &ps_soln_group, &ps_first_soln_group,
    &ps_time_measure, &ps_time_limit, /* &ps_reverse_order, */
    &ps_avail_mem, &ps_use_cache);

  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheArchiveParallelSolve(%s) soln_group %s\n",
      KheArchiveId(archive) == NULL ? "unnamed archive" : KheArchiveId(archive),
      ps_soln_group == NULL ? "NOT SAVING" : ps_soln_group);
    fprintf(stderr, "  threads %d, make %d, keep %d, recombine %s, time %s, "
      "limit %s, use_cache %s)\n", ps_threads, ps_make, ps_keep,
      bool_show(ps_recombine),
      ps_time_measure == KHE_PS_TIME_OMIT ? "omit" : "shared",
      KheTimeShow(ps_time_limit, buff), bool_show(ps_use_cache));
  }

  /* optionally get the cache file name and replace archive with its contents */
  a = HaArenaMake(as);
  cache = (ps_use_cache && (ps_soln_group!=NULL || ps_first_soln_group!=NULL) ?
    HnStringMake(a, "tmp_%s.xml", KheArchiveId(archive)) : NULL);
  resumed = false;
  if( cache != NULL && (fp = fopen(cache, "r")) != NULL )
  {
    if( DEBUG2 )
      fprintf(stderr, "  resuming solving using cache file %s\n", cache);
    resumed = true;
    KheArchiveClear(archive);
    if( !KheArchiveLoad(archive, fp, &ke, false, true /* choice still to do */,
      true, false, KHE_SOLN_WRITABLE_PLACEHOLDER, NULL) )
      HnFatal("%s:%d:%d: %s\n", cache, KmlErrorLineNum(ke),
	KmlErrorColNum(ke), KmlErrorString(ke));
    fclose(fp);
  }

  /* get soln groups */
  first_soln_group = KheGetSolnGroup(archive, ps_first_soln_group, resumed);
  soln_group = KheGetSolnGroup(archive, ps_soln_group, resumed);

  /* create a psolver with all these attributes */
  SolnTypeCheck(soln_type, "KheArchiveParallelSolve");
  ps = PSolverMake(archive, solver, options, soln_type, as, first_soln_group,
    soln_group, ps_threads, ps_make, ps_no_diversify, ps_keep, ps_recombine,
    ps_time_measure, ps_time_limit, /* ps_reverse_order, */ ps_avail_mem,
    cache, a);

  /* solve the archive, either all together or instance by instance */
  switch( ps_time_measure )
  {
    case KHE_PS_TIME_OMIT:

      /* load and solve the whole archive */
      /* ***
      if( ps_reverse_order )
      {
	for( i = KheArchiveInstanceCount(archive) - 1;  i >= 0;  i-- )
	{
	  ins = KheArchiveInstance(archive, i);
	  PSolverLoadInstance(ps, ins);
	}
      }
      else
      {
      *** */
	for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
	{
	  ins = KheArchiveInstance(archive, i);
	  PSolverLoadInstance(ps, ins);
	}
      /* } */
      PSolverRun(ps);

      /* add the surviving solutions to soln_group, if any */
      HaArrayForEach(ps->instances, pinst, i)
	PInstEndRun(pinst, false);
      HaArrayClear(ps->instances);
      break;

    case KHE_PS_TIME_SHARED:

      /* ***
      if( ps_reverse_order )
      {
	for( i = KheArchiveInstanceCount(archive) - 1;  i >= 0;  i-- )
	{
	  ** load and solve the i'th instance of archive **
	  ins = KheArchiveInstance(archive, i);
	  pinst = PSolverLoadInstance(ps, ins);
	  PSolverRun(ps);

	  ** add the surviving solutions to soln_group, if any; set run times **
	  PInstEndRun(pinst, true);
	  HaArrayClear(ps->instances);
	}
      }
      else
      {
      *** */
	for( i = 0;  i < KheArchiveInstanceCount(archive);  i++ )
	{
	  /* load and solve the i'th instance of archive */
	  ins = KheArchiveInstance(archive, i);
	  pinst = PSolverLoadInstance(ps, ins);
	  PSolverRun(ps);

	  /* add the surviving solutions to soln_group, if any; set run times */
	  PInstEndRun(pinst, true);
	  HaArrayClear(ps->instances);
	}
      /* } */
      break;

    default:

      HnAbort("KheArchiveParallelSolve: internal error 1");
      break;
  }

  /* debug prints */
  if( DEBUG1 )
  {
    if( soln_group != NULL && KheSolnGroupSolnCount(soln_group) != 0 )
    {
      fprintf(stderr, "  KheArchiveParallelSolve best solution%s:\n",
	KheSolnGroupSolnCount(soln_group) != 1 ? "s" : "");
      for( i = 0;  i < KheSolnGroupSolnCount(soln_group);  i++ )
      {
	soln = KheSolnGroupSoln(soln_group, i);
	KheSolnDebug(soln, 2, 2, stderr);
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheArchiveParallelSolve returning (%s elapsed)\n",
      KheTimeShow(KheTimerElapsedTime(ps->global_timer), buff));

  /* delete the solver, including tidying up arena sets */
  PSolverDelete(ps);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheInstanceParallelSolve(KHE_INSTANCE ins,                      */
/*    KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,                        */
/*    KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)                              */
/*                                                                           */
/*  Like KheArchiveParallelSolve except that it solves only one instance     */
/*  and saves (and returns) any one best solution.                           */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheInstanceParallelSolve(KHE_INSTANCE ins,
  KHE_GENERAL_SOLVER solver, KHE_OPTIONS options,
  KHE_SOLN_TYPE soln_type, HA_ARENA_SET as)
{
  PSOLVER ps;  PINST pinst;  KHE_SOLN soln;
  int ps_threads, ps_make, ps_keep;
  bool ps_recombine, ps_no_diversify /*, ps_reverse_order*/;
  KHE_PS_TIME ps_time_measure;  float ps_time_limit;  size_t ps_avail_mem;
  bool ps_use_cache;  HA_ARENA a;  char *ps_soln_group, *ps_first_soln_group;

  /* get options */
  KheGetPSOptions(options, true, &ps_threads, &ps_make, &ps_no_diversify,
    &ps_keep, &ps_recombine, &ps_soln_group, &ps_first_soln_group,
    &ps_time_measure, &ps_time_limit, /* &ps_reverse_order, */
    &ps_avail_mem, &ps_use_cache);
  if( DEBUG2 )
    fprintf(stderr, "[ KheInstanceParallelSolve(threads %d, make %d)\n",
      ps_threads, ps_make);

  /* create a psolver object and load and solve ins */
  SolnTypeCheck(soln_type, "KheInstanceParallelSolve");
  a = HaArenaMake(as);
  ps = PSolverMake(NULL, solver, options, soln_type, as, NULL, NULL,
    ps_threads, ps_make, ps_no_diversify, 1, ps_recombine, ps_time_measure,
    ps_time_limit, /* ps_reverse_order, */ ps_avail_mem, NULL, a);
  pinst = PSolverLoadInstance(ps, ins);
  PSolverRun(ps);

  /* extract the sole surviving solution */
  soln = PInstEndRunSingle(pinst, ps_time_measure == KHE_PS_TIME_SHARED);
  if( DEBUG2 )
  {
    fprintf(stderr, "  final solution:\n");
    KheSolnDebug(soln, 2, 2, stderr);
    fprintf(stderr, "] KheInstanceParallelSolve returning\n");
  }
  return soln;
}

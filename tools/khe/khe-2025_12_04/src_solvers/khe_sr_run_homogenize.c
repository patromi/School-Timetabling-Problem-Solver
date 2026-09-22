
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
/*  FILE:         khe_sr_run_homogenize.c                                    */
/*  DESCRIPTION:  Run homogenization                                         */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_HOMO_RUN - a homogeneous run                                    */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_homo_run_rec {
  KHE_RESOURCE		resource;		/* the owner of the run      */
  KHE_INTERVAL		interval;		/* the days of the run       */
  KHE_TASK_SET		task_set;		/* the tasks of the run      */
  int			offset;			/* the offset of the tasks   */
  int			offset_to_left;		/* the offset to the left    */
  int			offset_to_right;	/* the offset to the right   */
} *KHE_HOMO_RUN;

typedef HA_ARRAY(KHE_HOMO_RUN) ARRAY_KHE_HOMO_RUN;


/*****************************************************************************/
/*                                                                           */
/*  KHE_HOMO_SOLVER - a homogeneous run solver                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_homo_solver_rec {
  HA_ARENA		arena;
  KHE_SOLN		soln;
  KHE_RESOURCE_TYPE	resource_type;
  KHE_OPTIONS		options;
  KHE_FRAME		days_frame;
  ARRAY_KHE_HOMO_RUN	runs;
} *KHE_HOMO_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HOMO_RUN"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_HOMO_RUN KheHomoRunMake(KHE_RESOURCE r, KHE_INTERVAL in,             */
/*    KHE_TASK_SET ts, int offset, int offset_to_left, int offset_to_right,  */
/*    KHE_HOMO_SOLVER hs)                                                    */
/*                                                                           */
/*  Make a new homo run object with these attributes.                        */
/*                                                                           */
/*****************************************************************************/

static KHE_HOMO_RUN KheHomoRunMake(KHE_RESOURCE r, KHE_INTERVAL in,
  KHE_TASK_SET ts, int offset, int offset_to_left, int offset_to_right,
  KHE_HOMO_SOLVER hs)
{
  KHE_HOMO_RUN res;
  HaMake(res, hs->arena);
  res->resource = r;
  res->interval = in;
  res->task_set = ts;
  res->offset = offset;
  res->offset_to_left = offset_to_left;
  res->offset_to_right = offset_to_right;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheHomoRunTypedCmp(KHE_HOMO_RUN hr1, KHE_HOMO_RUN hr2)               */
/*                                                                           */
/*  Typed comparison function to bring runs with equal intervals together.   */
/*                                                                           */
/*****************************************************************************/

static int KheHomoRunTypedCmp(KHE_HOMO_RUN hr1, KHE_HOMO_RUN hr2)
{
  if( hr1->interval.first != hr2->interval.first )
    return hr1->interval.first - hr2->interval.first;
  else
    return hr1->interval.last - hr2->interval.last;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheHomoRunCmp(const void *t1, const void *t2)                        */
/*                                                                           */
/*  Untyped comparison function to bring runs with equal intervals together. */
/*                                                                           */
/*****************************************************************************/

static int KheHomoRunCmp(const void *t1, const void *t2)
{
  KHE_HOMO_RUN hr1 = * (KHE_HOMO_RUN *) t1;
  KHE_HOMO_RUN hr2 = * (KHE_HOMO_RUN *) t2;
  return KheHomoRunTypedCmp(hr1, hr2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHomoDebugHeader(KHE_HOMO_RUN hr, FILE *fp)                       */
/*                                                                           */
/*  Debug print of the header of hr onto fp.                                 */
/*                                                                           */
/*****************************************************************************/

static void KheHomoDebugHeader(KHE_HOMO_RUN hr, KHE_FRAME frame, FILE *fp)
{
  fprintf(fp, "%s", KheResourceId(hr->resource));
  if( hr->offset_to_left != -1 )
    fprintf(fp, "|%d", hr->offset_to_left);
  fprintf(fp, "|%s:%d|", KheIntervalShow(hr->interval, frame), hr->offset);
  if( hr->offset_to_right != -1 )
    fprintf(fp, "%d|", hr->offset_to_right);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHomoRunDebug(KHE_HOMO_RUN hr, KHE_FRAME frame,                   */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of hr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHomoRunDebug(KHE_HOMO_RUN hr, KHE_FRAME frame,
  int verbosity, int indent, FILE *fp)
{
  int i;  KHE_TASK task;
  if( indent < 0 )
    KheHomoDebugHeader(hr, frame, fp);
  else
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheHomoDebugHeader(hr, frame, fp);
    fprintf(fp, "\n");
    for( i = 0;  i < KheTaskSetTaskCount(hr->task_set);  i++ )
    {
      task = KheTaskSetTask(hr->task_set, i);
      KheTaskDebug(task, verbosity, indent + 2, fp);
    }
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_HOMO_SOLVER"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_HOMO_SOLVER KheHomoSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,   */
/*    KHE_OPTIONS options, HA_ARENA a)                                       */
/*                                                                           */
/*  Make a new homo solver with these attributes, and initially no runs.     */
/*                                                                           */
/*****************************************************************************/

static KHE_HOMO_SOLVER KheHomoSolverMake(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options, HA_ARENA a)
{
  KHE_HOMO_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  res->options = options;
  res->days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  HaArrayInit(res->runs, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheHomoSolverDebug(KHE_HOMO_SOLVER hs,                              */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of hs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheHomoSolverDebug(KHE_HOMO_SOLVER hs,
  int verbosity, int indent, FILE *fp)
{
  KHE_HOMO_RUN hr;   int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ HomoSolver(%s, %s)\n", indent, "",
      KheInstanceId(KheSolnInstance(hs->soln)),
      KheResourceTypeId(hs->resource_type));
    HaArrayForEach(hs->runs, hr, i)
      KheHomoRunDebug(hr, hs->days_frame, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "HomoSolver");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KheRunHomogenize"                                             */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTimeGroupSoleUnpreassignedTask(                              */
/*    KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, int *offset)    */
/*                                                                           */
/*  If rtm has a single unpreassigned task running during tg, return that    */
/*  task and set *offset to its offset in tg.  Otherwise return NULL with    */
/*  *offset set to -1.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK KheTimeGroupSoleUnpreassignedTask(
  KHE_RESOURCE_TIMETABLE_MONITOR rtm, KHE_TIME_GROUP tg, int *offset)
{
  KHE_TIME t;  int i, j;  KHE_TASK task, res;  KHE_RESOURCE r2;
  res = NULL;
  for( i = 0;  i < KheTimeGroupTimeCount(tg);  i++ )
  {
    t = KheTimeGroupTime(tg, i);
    for( j = 0;  j < KheResourceTimetableMonitorTimeTaskCount(rtm, t);  j++ )
    {
      task = KheResourceTimetableMonitorTimeTask(rtm, t, j);
      task = KheTaskProperRoot(task);
      if( !KheTaskIsPreassigned(task, &r2) )
      {
	if( res == NULL )
	  *offset = i, res = task;
	else if( res != task )
	  return *offset = -1, NULL;
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheBuildRunsForResource(KHE_HOMO_SOLVER hs, KHE_RESOURCE r)         */
/*                                                                           */
/*  Build runs for r and add them to hs.                                     */
/*                                                                           */
/*  Loop invariant:                                                          */
/*                                                                           */
/*    If prev_ts != NULL, then there is a run ending at i-1 and:             */
/*                                                                           */
/*      prev_ts is the set of tasks of that run.                             */
/*      prev_ts_offset is the offset of the tasks of prev_ts.                */
/*      prev_run_offset_on_left is the offset of the tasks of the run        */
/*      immediately adjacent to prev_ts on the left, or -1 if there is       */
/*      no such run.                                                         */
/*                                                                           */
/*    If prev_ts == NULL, there is no run ending at i-1 and prev_ts_offset   */
/*    and prev_run_offset_on_left are undefined.                             */
/*                                                                           */
/*****************************************************************************/

static void KheBuildRunsForResource(KHE_HOMO_SOLVER hs, KHE_RESOURCE r)
{
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;  KHE_HOMO_RUN hr;
  int i, task_offset, prev_ts_offset, prev_run_offset_on_left, pos;
  KHE_TIME_GROUP tg;  KHE_TASK_SET prev_ts;  KHE_TASK task;
  rtm = KheResourceTimetableMonitor(hs->soln, r);
  prev_ts = NULL;  prev_ts_offset = prev_run_offset_on_left = -1;
  for( i = 0;  i < KheFrameTimeGroupCount(hs->days_frame);  i++ )
  {
    tg = KheFrameTimeGroup(hs->days_frame, i);
    task = KheTimeGroupSoleUnpreassignedTask(rtm, tg, &task_offset);

    if( prev_ts != NULL )  /* there is a run ending at i-1 */
    {
      if( task != NULL && task_offset == prev_ts_offset )
      {
	/* carry on with current run */
	if( !KheTaskSetContainsTask(prev_ts, task, &pos) )
	  KheTaskSetAddTask(prev_ts, task);
      }
      else
      {
	/* finish off the previous run */
	hr = KheHomoRunMake(r, KheTaskSetInterval(prev_ts, hs->days_frame),
	  prev_ts, prev_ts_offset, prev_run_offset_on_left,
	  task != NULL ? task_offset : -1, hs);
	HaArrayAddLast(hs->runs, hr);

	/* start new run if any */
	if( task != NULL )
	{
	  prev_run_offset_on_left = prev_ts_offset;
	  prev_ts_offset = task_offset;
	  prev_ts = KheTaskSetMake(hs->soln);
	  KheTaskSetAddTask(prev_ts, task);
	}
	else
	  prev_ts = NULL;
      }
    }
    else /* there is no run ending at i-1 */
    {
      if( task != NULL )
      {
	/* start new run */
	prev_run_offset_on_left = -1;
	prev_ts_offset = task_offset;
	prev_ts = KheTaskSetMake(hs->soln);
	KheTaskSetAddTask(prev_ts, task);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryRun(KHE_HOMO_SOLVER hs, KHE_HOMO_RUN hr1, KHE_HOMO_RUN hr2)   */
/*                                                                           */
/*  Try swapping hr1 and hr2 if it will make the runs more homogeneous.      */
/*                                                                           */
/*****************************************************************************/

static bool KheTryRun(KHE_HOMO_SOLVER hs, KHE_HOMO_RUN hr1, KHE_HOMO_RUN hr2)
{
  KHE_MARK mark;  KHE_COST init_cost;  bool success;
  if( hr1->offset == hr2->offset_to_left ||
      hr1->offset == hr2->offset_to_right ||
      hr2->offset == hr1->offset_to_left ||
      hr2->offset == hr1->offset_to_right )
  {
    mark = KheMarkBegin(hs->soln);
    init_cost = KheSolnCost(hs->soln);
    success = KheTaskSetUnAssignResource(hr1->task_set) &&
      KheTaskSetUnAssignResource(hr2->task_set) &&
      KheTaskSetAssignResource(hr1->task_set, hr2->resource) &&
      KheTaskSetAssignResource(hr2->task_set, hr1->resource) &&
      KheSolnCost(hs->soln) <= init_cost;
    if( DEBUG2 && success )
      fprintf(stderr, "  KheRunHomogenize swapping %s and %s during %s\n",
	KheResourceId(hr1->resource), KheResourceId(hr2->resource),
	KheIntervalShow(hr1->interval, hs->days_frame));
    KheMarkEnd(mark, !success);
    return success;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTryRuns(KHE_HOMO_SOLVER hs, int first, int last)                 */
/*                                                                           */
/*****************************************************************************/

static bool KheTryRuns(KHE_HOMO_SOLVER hs, int first, int last)
{
  int i, j;  KHE_HOMO_RUN hr1, hr2;
  for( i = first;  i <= last;  i++ )
  {
    hr1 = HaArray(hs->runs, i);
    for( j = i + 1;  j <= last;  j++ )
    {
      hr2 = HaArray(hs->runs, j);
      if( KheTryRun(hs, hr1, hr2) )
	return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRunHomogenize(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,               */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Carry out run homogenization for the resources of type rt in soln.       */
/*                                                                           */
/*****************************************************************************/

void KheRunHomogenize(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  KHE_HOMO_SOLVER hs;  HA_ARENA a;  int i, j;  KHE_RESOURCE r;
  KHE_HOMO_RUN hr1, hr2;
  if( DEBUG1 )
    fprintf(stderr, "[ KheRunHomogenize(%s, %s, options)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  RESTART:

  /* build a homo solver and add all runs to it */
  a = KheSolnArenaBegin(soln);
  hs = KheHomoSolverMake(soln, rt, options, a);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    KheBuildRunsForResource(hs, r);
  }

  /* sort runs to bring those with equal intervals together */
  HaArraySort(hs->runs, &KheHomoRunCmp);
  if( DEBUG1 )
    KheHomoSolverDebug(hs, 2, 2, stderr);

  /* try equal intervals */
  for( i = 0;  i < HaArrayCount(hs->runs);  i += j )
  {
    hr1 = HaArray(hs->runs, i);
    for( j = i + 1;  j < HaArrayCount(hs->runs);  j++ )
    {
      hr2 = HaArray(hs->runs, j);
      if( !KheIntervalEqual(hr1->interval, hr2->interval) )
	break;
    }
    if( KheTryRuns(hs, i, j - 1) )
    {
      KheSolnArenaEnd(soln, a);
      goto RESTART;
    }
  }

  /* finish off and end */
  KheSolnArenaEnd(soln, a);
  if( DEBUG1 )
    fprintf(stderr, "] KheRunHomogenize returning\n");
}

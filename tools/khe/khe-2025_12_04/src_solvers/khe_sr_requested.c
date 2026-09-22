
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
/*  FILE:         khe_sr_requested.c                                         */
/*  DESCRIPTION:  Satisfying requested task assignments                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG4_ID "A10"
#define DEBUG5 0	/* clash checking */

#define DEBUG6 0	/* trial for a specific nurse */
#define DEBUG6_ID "Nurse3"
#define DEBUG6_ON(r) (DEBUG6 && strcmp(KheResourceId(r), DEBUG6_ID) == 0)
#define DEBUG7 0


/*****************************************************************************/
/*                                                                           */
/*  KHE_REQUEST_SOLVER                                                       */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_request_solver_rec {
  KHE_RESOURCE		resource;		/* the resource              */
  KHE_RESOURCE_TIMETABLE_MONITOR rtm;		/* resource tt monitor       */
  KHE_SOLN		soln;			/* the solution              */
  KHE_GROUP_MONITOR	group_monitor;		/* minimize its cost         */
  KHE_FRAME		days_frame;		/* days frame                */
  KHE_EVENT_TIMETABLE_MONITOR etm;		/* event timetable monitor   */
  KHE_TASK		best_task;		/* best task so far          */
  KHE_COST		best_soln_cost;		/* if best task              */
  KHE_COST		best_gm_cost;		/* if best task              */
  int			attempts;		/* attempts                  */
} *KHE_REQUEST_SOLVER;

typedef HA_ARRAY(KHE_MONITOR) ARRAY_KHE_MONITOR;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "request solver"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheRequestSolverBegin(KHE_REQUEST_SOLVER rs, KHE_RESOURCE r,        */
/*    KHE_SOLN soln, KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,             */
/*    KHE_EVENT_TIMETABLE_MONITOR etm)                                       */
/*                                                                           */
/*  Initialize rs with these attributes.                                     */
/*                                                                           */
/*****************************************************************************/

static void KheRequestSolverBegin(KHE_REQUEST_SOLVER rs, KHE_RESOURCE r,
  KHE_SOLN soln, KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  if( DEBUG6_ON(r) )
    fprintf(stderr, "  KheRequestSolverBegin(soln cost %.5f, %s)\n",
      KheCostShow(KheSolnCost(soln)), KheResourceId(r));
  rs->resource = r;
  rs->rtm = KheResourceTimetableMonitor(soln, r);
  rs->soln = soln;
  rs->group_monitor = gm;
  rs->days_frame = days_frame;
  rs->etm = etm;
  rs->best_task = NULL;
  rs->best_soln_cost = 0;  /* actually undefined while best_task == NULL */
  rs->best_gm_cost = 0;  /* actually undefined while best_task == NULL */
  rs->attempts = 0;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRequestSolverCompareTask(KHE_REQUEST_SOLVER rs, KHE_TASK task)   */
/*                                                                           */
/*  Compare task with what we have now.  If it's better, make it the         */
/*  best task and return true.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheRequestSolverCompareTask(KHE_REQUEST_SOLVER rs, KHE_TASK task)
{
  KHE_COST soln_cost, gm_cost;
  soln_cost = KheSolnCost(rs->soln);
  if( rs->group_monitor != NULL )
  {
    gm_cost = KheMonitorCost((KHE_MONITOR) rs->group_monitor);
    if( rs->best_task == NULL ||
	gm_cost < rs->best_gm_cost ||
	(gm_cost == rs->best_gm_cost && soln_cost < rs->best_soln_cost) )
    {
      /* new best */
      rs->best_task = task;
      rs->best_gm_cost = gm_cost;
      rs->best_soln_cost = soln_cost;
      return true;
    }
  }
  else
  {
    if( rs->best_task == NULL || soln_cost < rs->best_soln_cost )
    {
      /* new best */
      rs->best_task = task;
      rs->best_soln_cost = soln_cost;
      return true;
    }
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheRequestSolverTryTime(KHE_REQUEST_SOLVER rs, KHE_TIME t)          */
/*                                                                           */
/*  Try the tasks that cover t.                                              */
/*                                                                           */
/*****************************************************************************/

static void KheRequestSolverTryTime(KHE_REQUEST_SOLVER rs, KHE_TIME t)
{
  int i, j, extra, index, inc;  KHE_MEET meet;  KHE_TIME_GROUP tg;
  KHE_TASK task;

  /* if r is already busy at time t, return immediately */
  index = KheFrameTimeIndex(rs->days_frame, t);
  tg = KheFrameTimeGroup(rs->days_frame, index);
  if( !KheResourceTimetableMonitorFreeForTimeGroup(rs->rtm,tg,NULL,NULL,false) )
  /* ***
  if( !KheResourceTimetableMonitorTimeGroupAvailable(rs->rtm, tg, false, true) )
  *** */
    return;

  /* try a selection of tasks running at time t */
  extra = KheResourceInstanceIndex(rs->resource);
  for( i = 0;  i < KheEventTimetableMonitorTimeMeetCount(rs->etm, t);  i++ )
  {
    meet = KheEventTimetableMonitorTimeMeet(rs->etm, t, i);
    inc = (KheMeetTaskCount(meet) >= 10 ? 5 : 1);
    for( j = 0;  j < KheMeetTaskCount(meet);  j += inc )
    {
      /* attempt to assign task to r */
      index = (j + extra) % KheMeetTaskCount(meet);
      task = KheTaskProperRoot(KheMeetTask(meet, index));
      rs->attempts++;
      if( KheTaskAsst(task) != NULL )
      {
	/* task already assigned */
	if( DEBUG6_ON(rs->resource) )
	{
	  fprintf(stderr, "    tried ");
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, " (already assigned %s)\n",
	    KheResourceId(KheTaskAsstResource(task)));
	}
      }
      else if( !KheResourceTimetableMonitorFreeForTask(rs->rtm, task,
	    rs->days_frame, NULL, false) )
      /* ***
      else if( !KheResourceTimetableMonitorTaskAvailableInFrame(rs->rtm, task,
	    rs->days_frame, NULL) )
      *** */
      {
	/* resource not available at frame time groups covered by task */
	if( DEBUG6_ON(rs->resource) )
	{
	  fprintf(stderr, "    tried ");
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, " (times unavailable in frame)\n");
	}
      }
      else if( !KheTaskAssignResource(task, rs->resource) )
      {
	/* task does not accept resource */
	if( DEBUG6_ON(rs->resource) )
	{
	  fprintf(stderr, "    tried ");
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, " (did not assign)\n");
	}
      }
      else if( !KheRequestSolverCompareTask(rs, task) )
      {
	/* task accepts resource, not new best */
	if( DEBUG6_ON(rs->resource) )
	{
	  fprintf(stderr, "    tried ");
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, " (cost %.5f, not new best)\n",
	    KheCostShow(KheSolnCost(rs->soln)));
	}
	KheTaskUnAssignResource(task);
      }
      else
      {
	/* task accepts resource, new best */
	if( DEBUG6_ON(rs->resource) )
	{
	  fprintf(stderr, "    tried ");
	  KheTaskDebug(task, 2, -1, stderr);
	  fprintf(stderr, " (cost %.5f, new best)\n",
	    KheCostShow(KheSolnCost(rs->soln)));
	}
	KheTaskUnAssignResource(task);
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheRequestSolverEnd(KHE_REQUEST_SOLVER rs)                          */
/*                                                                           */
/*  End a trial, including making the best assignment, if there is one.      */
/*  Return true if an assignment is made.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheRequestSolverEnd(KHE_REQUEST_SOLVER rs)
{
  if( rs->best_task != NULL )
  {
    KheTaskAssignResource(rs->best_task, rs->resource);
    if( DEBUG7 || DEBUG6_ON(rs->resource) )
    {
      fprintf(stderr, "  KheRequestSolverEnd ");
      KheTaskDebug(rs->best_task, 2, -1, stderr);
      fprintf(stderr, " := %s (soln cost now %.5f)\n",
	KheResourceId(rs->resource), KheCostShow(KheSolnCost(rs->soln)));
    }
    return true;
  }
  else
  {
    if( DEBUG7 || DEBUG6_ON(rs->resource) )
      fprintf(stderr, "  KheRequestSolverEnd (unsuccessful)\n");
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main function"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE KheMonitorResource(KHE_MONITOR m)                           */
/*                                                                           */
/*  Return the resource monitored by m.                                      */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE KheMonitorResource(KHE_MONITOR m)
{
  if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
    return KheLimitBusyTimesMonitorResource((KHE_LIMIT_BUSY_TIMES_MONITOR) m);
  else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
    return KheClusterBusyTimesMonitorResource(
      (KHE_CLUSTER_BUSY_TIMES_MONITOR) m);
  else
  {
    HnAbort("KheMonitorResource: unexpected tag %d", KheMonitorTag(m));
    return NULL;  /* keep compiler happy */
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDecreasingWeightTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)   */
/*                                                                           */
/*  Comparison function for sorting an array of monitors by decreasing       */
/*  weight, with resource index as a secondary key.                          */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorDecreasingWeightTypedCmp(KHE_MONITOR m1, KHE_MONITOR m2)
{
  KHE_COST weight1, weight2;  int cmp;  KHE_RESOURCE r1, r2;
  /* ***
  KHE_CONSTRAINT c1, c2;
  c1 = KheMonitor Constraint(m1);
  c2 = KheMonitorC onstraint(m2);
  *** */
  weight1 = KheMonitorCombinedWeight(m1);
  weight2 = KheMonitorCombinedWeight(m2);
  cmp = KheCostCmp(weight2, weight1);  /* reverse order, decreasing weight */
  if( cmp != 0 )  return cmp;
  r1 = KheMonitorResource(m1);
  r2 = KheMonitorResource(m2);
  return KheResourceInstanceIndex(r1) - KheResourceInstanceIndex(r2);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheMonitorDecreasingWeightCmp(const void *t1, const void *t2)        */
/*                                                                           */
/*  Untyped version of KheMonitorDecreasingWeightTypedCmp.                   */
/*                                                                           */
/*****************************************************************************/

static int KheMonitorDecreasingWeightCmp(const void *t1, const void *t2)
{
  KHE_MONITOR m1 = * (KHE_MONITOR *) t1;
  KHE_MONITOR m2 = * (KHE_MONITOR *) t2;
  return KheMonitorDecreasingWeightTypedCmp(m1, m2);
}


/*****************************************************************************/
/*                                                                           */
/*  bool MakeBestAsstAtTime(KHE_RESOURCE r, KHE_SOLN soln,                   */
/*    KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,                            */
/*    KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME t)                           */
/*                                                                           */
/*  Make the best possible assignment of a task running at time t to r,      */
/*  that is, if r is not already busy then.  Obtain the events running       */
/*  at time t from etm.  If an assignment is actually made, return true.     */
/*                                                                           */
/*  Actually, if the meet has more than 10 tasks, we only try every fifth    */
/*  task, cyclically starting from a random point.  This is because most     */
/*  tasks are symmetrical and it takes too long to try them all.             */
/*                                                                           */
/*****************************************************************************/

static bool MakeBestAsstAtTime(KHE_RESOURCE r, KHE_SOLN soln,
  KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm, KHE_TIME t)
{
  struct khe_request_solver_rec rsr;
  if( DEBUG2 )
    fprintf(stderr, "      MakeBestAsstAtTime(soln, %s, %s)", KheResourceId(r),
      KheTimeId(t));
  KheRequestSolverBegin(&rsr, r, soln, gm, days_frame, etm);
  KheRequestSolverTryTime(&rsr, t);
  return KheRequestSolverEnd(&rsr);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHandleClusterForced(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,         */
/*    KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)  */
/*                                                                           */
/*  Try to correct the defects of cbtm in places where the choices of times  */
/*  are forced, and return true if any changes were made.                    */
/*                                                                           */
/*  A time is forced when it is the only time in a positive time group, and  */
/*  every time group has to be active; or when it is the only time in a      */
/*  negative time group, and every time group has to be inactive.            */
/*                                                                           */
/*****************************************************************************/

static bool KheHandleClusterForced(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  KHE_SOLN soln, KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int active_group_count, open_group_count, min_limit, max_limit, i;
  int busy_count, count;  KHE_RESOURCE r;  KHE_TIME t;
  bool allow_zero, active, res;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
  r = KheClusterBusyTimesMonitorResource(cbtm);
  count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
  KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &active_group_count,
    &open_group_count, &min_limit, &max_limit, &allow_zero);
  res = false;
  if( min_limit >= count && active_group_count + open_group_count < min_limit )
  {
    /* every time group has to be active, and not all of them are */
    if( !KheClusterBusyTimesConstraintAllNegative(cbtc) )
    {
      for( i = 0;  i < count;  i++ )
      {
	active = KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i,
	  &tg, &po, &busy_count);
	if( !active && po == KHE_POSITIVE && KheTimeGroupTimeCount(tg) == 1 )
	{
	  /* try to make tg busy */
	  t = KheTimeGroupTime(tg, 0);
	  if( MakeBestAsstAtTime(r, soln, gm, days_frame, etm, t) )
	    res = true;
	}
      }
    }
  }
  else if( max_limit == 0 && active_group_count > max_limit )
  {
    /* every time group has to be inactive, and not all of them are */
    if( !KheClusterBusyTimesConstraintAllPositive(cbtc) )
    {
      for( i = 0;  i < count;  i++ )
      {
	active = KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, i,
	  &tg, &po, &busy_count);
	if( active && po == KHE_NEGATIVE && KheTimeGroupTimeCount(tg) == 1 )
	{
	  /* try to make tg busy */
	  t = KheTimeGroupTime(tg, 0);
	  if( MakeBestAsstAtTime(r, soln, gm, days_frame, etm, t) )
	    res = true;
	}
      }
    }
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHandleClusterNotForced(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,      */
/*    KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)  */
/*                                                                           */
/*  Try to correct the defects of cbtm in places where the choices of times  */
/*  are not forced, and return true if any changes were made.                */
/*                                                                           */
/*****************************************************************************/

static bool KheHandleClusterNotForced(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm,
  KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int active_group_count, open_group_count, min_limit, max_limit, i, j;
  int count, extra, index, busy_count;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  bool allow_zero, active, progressing, res;  KHE_RESOURCE r;  KHE_TIME t;
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;  struct khe_request_solver_rec rsr;
  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
  r = KheClusterBusyTimesMonitorResource(cbtm);
  res = false;
  do
  {
    progressing = false;
    KheClusterBusyTimesMonitorActiveTimeGroupCount(cbtm, &active_group_count,
      &open_group_count, &min_limit, &max_limit, &allow_zero);
    if( active_group_count + open_group_count < min_limit )
    {
      if( !KheClusterBusyTimesConstraintAllNegative(cbtc) )
      {
	/* need to increase the number of active time groups */
	/* by assigning r to tasks in inactive positive time groups */
	KheRequestSolverBegin(&rsr, r, soln, NULL, days_frame, etm);
	count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
	extra = KheMonitorSolnIndex((KHE_MONITOR) cbtm) * 13 +
	  KheSolnDiversifier(soln) * 7;
	for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
	{
	  index = (i + extra) % count;
	  active = KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, index,
	    &tg, &po, &busy_count);
	  if( !active && po == KHE_POSITIVE )
	  {
	    /* try to make tg busy */
	    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	    {
	      t = KheTimeGroupTime(tg, j);
	      KheRequestSolverTryTime(&rsr, t);
	    }
	  }
	}
	if( KheRequestSolverEnd(&rsr) )
	  progressing = res = true;
      }
    }
    else if( active_group_count > max_limit )
    {
      /* need to decrease the number of active time groups */
      /* by assigning r to tasks in active negative time groups */
      if( !KheClusterBusyTimesConstraintAllPositive(cbtc) )
      {
	KheRequestSolverBegin(&rsr, r, soln, NULL, days_frame, etm);
	count = KheClusterBusyTimesMonitorTimeGroupCount(cbtm);
	extra = KheMonitorSolnIndex((KHE_MONITOR) cbtm) * 13 +
	  KheSolnDiversifier(soln) * 7;
	for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
	{
	  index = (i + extra) % count;
	  active = KheClusterBusyTimesMonitorTimeGroupIsActive(cbtm, index,
	    &tg, &po, &busy_count);
	  if( active && po == KHE_NEGATIVE )
	  {
	    /* try to make tg busy */
	    for( j = 0;  j < KheTimeGroupTimeCount(tg);  j++ )
	    {
	      t = KheTimeGroupTime(tg, j);
	      KheRequestSolverTryTime(&rsr, t);
	    }
	  }
	}
	if( KheRequestSolverEnd(&rsr) )
	  progressing = res = true;
      }
    }
  } while( progressing );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHandleLimitForced(KHE_LIMIT_BUSY_TIMES_MONITOR lbtm,             */
/*    KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)  */
/*                                                                           */
/*  Try to correct the defects of lbtm in places where the choices of times  */
/*  are forced, and return true if any changes were made.                    */
/*                                                                           */
/*****************************************************************************/

static bool KheHandleLimitForced(KHE_LIMIT_BUSY_TIMES_MONITOR lbtm,
  KHE_SOLN soln, KHE_GROUP_MONITOR gm, KHE_FRAME days_frame,
  KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int min_limit, max_limit, i, k, busy_count, open_count;  KHE_RESOURCE r;
  bool allow_zero, res, progressing;  KHE_TIME_GROUP tg;  KHE_TIME t;
  r = KheLimitBusyTimesMonitorResource(lbtm);
  res = false;
  do
  {
    progressing = false;
    for( i = 0; i < KheLimitBusyTimesMonitorDefectiveTimeGroupCount(lbtm); i++ )
    {
      KheLimitBusyTimesMonitorDefectiveTimeGroup(lbtm, i, &tg,
	&busy_count, &open_count, &min_limit, &max_limit, &allow_zero);
      if( busy_count + open_count < min_limit &&
	  KheTimeGroupTimeCount(tg) <= min_limit )
      {
	/* every time of tg is requested to be busy */
	for( k = 0;  k < KheTimeGroupTimeCount(tg);  k++ )
	{
	  t = KheTimeGroupTime(tg, k);
	  if( MakeBestAsstAtTime(r, soln, gm, days_frame, etm, t) )
	    res = progressing = true;
	}
      }
    }
  } while( progressing );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheHandleLimitNotForced(KHE_LIMIT_BUSY_TIMES_MONITOR lbtm,          */
/*    KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)  */
/*                                                                           */
/*  Try to correct the defects of cbtm in places where the choices of times  */
/*  are not forced, and return true if any changes were made.                */
/*                                                                           */
/*****************************************************************************/

static bool KheHandleLimitNotForced(KHE_LIMIT_BUSY_TIMES_MONITOR lbtm,
  KHE_SOLN soln, KHE_FRAME days_frame, KHE_EVENT_TIMETABLE_MONITOR etm)
{
  int min_limit, max_limit, i, k, busy_count, open_count;
  KHE_RESOURCE r;  struct khe_request_solver_rec rsr;
  bool allow_zero, res, progressing;  KHE_TIME_GROUP tg;  KHE_TIME t;
  r = KheLimitBusyTimesMonitorResource(lbtm);
  res = false;
  do
  {
    progressing = false;
    for( i = 0; i < KheLimitBusyTimesMonitorDefectiveTimeGroupCount(lbtm); i++ )
    {
      KheLimitBusyTimesMonitorDefectiveTimeGroup(lbtm, i, &tg,
	&busy_count, &open_count, &min_limit, &max_limit, &allow_zero);
      if( busy_count + open_count < min_limit &&
	  KheTimeGroupTimeCount(tg) > min_limit )
      {
	/* not every time of tg is requested to be busy */
	KheRequestSolverBegin(&rsr, r, soln, NULL, days_frame, etm);
	for( k = 0;  k < KheTimeGroupTimeCount(tg);  k++ )
	{
	  t = KheTimeGroupTime(tg, k);
	  KheRequestSolverTryTime(&rsr, t);
	}
	if( KheRequestSolverEnd(&rsr) )
	  res = progressing = true;
      }
    }
  } while( progressing );
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSolnAssignRequestedResources(KHE_SOLN soln, KHE_OPTIONS options) */
/*                                                                           */
/*  Assign resources in soln as requested by its instance's limit busy       */
/*  times constraints.  Return true if any assignments were made.            */
/*                                                                           */
/*  This function uses option gs_event_timetable_monitor.                    */
/*                                                                           */
/*****************************************************************************/

bool KheSolnAssignRequestedResources(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_OPTIONS options)
{
  ARRAY_KHE_MONITOR monitors;  HA_ARENA a;  KHE_EVENT_TIMETABLE_MONITOR etm;
  KHE_MONITOR m, m2;  bool soln_changed;  KHE_RESOURCE r;  int i, j, k;
  KHE_FRAME days_frame;  KHE_COST init_cost;  KHE_GROUP_MONITOR gm;
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;

  /* boilerplate */
  if( KheOptionsGetBool(options, "rs_requested_off", false) )
    return false;
  if( DEBUG1 )
  {
    fprintf(stderr, "[ KheSolnAssignRequestedResources(soln, %s, options)\n",
      rt == NULL ? "-" : KheResourceTypeId(rt));
    KheSolnCostByTypeDebug(soln, 1, 2, stderr);
  }
  days_frame = KheOptionsFrame(options, "gs_common_frame", soln);
  if( DEBUG5 )
    KheFrameAssertNoClashes(days_frame, soln);
  etm = (KHE_EVENT_TIMETABLE_MONITOR)
    KheOptionsGetObject(options, "gs_event_timetable_monitor", NULL);
  HnAssert(etm != NULL, "KheSolnAssignRequestedResources: "
    "no gs_event_timetable_monitor option in options");
  HnAssert(rt != NULL, "KheSolnAssignRequestedResources: rt == NULL");

  /* build and sort the relevant monitors:  for all resources of type */
  /* rt, those limit busy times and cluster busy times monitors with  */
  /* non-zero cost whose allow_zero attributes are false              */
  init_cost = KheSolnCost(soln);
  a = KheSolnArenaBegin(soln);
  HaArrayInit(monitors, a);
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorCost(m) > 0 )
      {
	if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
	{
	  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	  lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
	  if( KheLimitBusyTimesConstraintMinimum(lbtc) > 0 &&
	      !KheLimitBusyTimesConstraintAllowZero(lbtc) )
	    HaArrayAddLast(monitors, m);
	}
	else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
	{
	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
	  if( (KheClusterBusyTimesConstraintMinimum(cbtc) > 0 &&
	      !KheClusterBusyTimesConstraintAllowZero(cbtc) &&
	      !KheClusterBusyTimesConstraintAllNegative(cbtc)) ||
	      (KheClusterBusyTimesConstraintMaximum(cbtc) <
	      KheClusterBusyTimesConstraintTimeGroupCount(cbtc) &&
	      !KheClusterBusyTimesConstraintAllPositive(cbtc)) )
	    HaArrayAddLast(monitors, m);
	}
      }
    }
  }
  HaArraySort(monitors, &KheMonitorDecreasingWeightCmp);

  /* handle each monitor, for cases where the choices of times are forced */
  soln_changed = false;
  if( DEBUG1 )
    fprintf(stderr, "  %d monitors (forced pass):\n", HaArrayCount(monitors));
  for( i = 0;  i < HaArrayCount(monitors);  i = j )
  {
    /* find the range of monitors with the same weight and resource as m */
    /* and build a group monitor from them */
    m = HaArray(monitors, i);
    r = KheMonitorResource(m);
    gm = KheGroupMonitorMake(soln, 8, "requested");
    KheGroupMonitorAddChildMonitor(gm, m);
    for( j = i + 1;  j < HaArrayCount(monitors);  j++ )
    {
      m2 = HaArray(monitors, j);
      if( KheMonitorDecreasingWeightTypedCmp(m, m2) != 0 )
	break;
      KheGroupMonitorAddChildMonitor(gm, m2);
    }

    /* handle each of these monitors */
    if( DEBUG3 )
    {
      fprintf(stderr, "  monitors %d .. %d (%s, gm %.5f):\n", i, j - 1,
	KheResourceId(r), KheCostShow(KheMonitorCost((KHE_MONITOR) gm)));
      /*KheCostShow(KheConst raintCombinedWeight(KheMonitor Constraint(m))));*/
      for( k = i;  k < j;  k++ )
      {
	m = HaArray(monitors, k);
	KheMonitorDebug(m, 2, 4, stderr);
      }
    }
    for( k = i;  k < j;  k++ )
    {
      m = HaArray(monitors, k);
      if( DEBUG3 )
	KheMonitorDebug(m, 2, 2, stderr);
      if( DEBUG5 )
	KheFrameAssertNoClashes(days_frame, soln);
      if( KheMonitorCost(m) > 0 )  /* not redundant, cost may have changed */
      {
	if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
	{
	  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	  if( KheHandleLimitForced(lbtm, soln, gm, days_frame, etm) )
	    soln_changed = true;
	}
	else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
	{
	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  if( KheHandleClusterForced(cbtm, soln, gm, days_frame, etm) )
	    soln_changed = true;
	}
	else
	  HnAbort("KheSolnAssignRequestedResources internal error");
      }
    }

    /* finished with gm */
    KheGroupMonitorDelete(gm);
  }

  /* handle each monitor, for cases where the choices of times are not forced */
  if( KheOptionsGetBool(options, "rs_requested_nonforced_on", false) )
  {
    if( DEBUG1 )
      fprintf(stderr, "  %d monitors (non-forced pass):\n",
	HaArrayCount(monitors));
    HaArrayForEach(monitors, m, i)
    {
      if( DEBUG3 )
	KheMonitorDebug(m, 2, 4, stderr);
      if( DEBUG5 )
	KheFrameAssertNoClashes(days_frame, soln);
      if( KheMonitorCost(m) > 0 )
      {
	if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
	{
	  lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
	  if( KheHandleLimitNotForced(lbtm, soln, days_frame, etm) )
	    soln_changed = true;
	}
	else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
	{
	  cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
	  if( KheHandleClusterNotForced(cbtm, soln, days_frame, etm) )
	    soln_changed = true;
	}
	else
	  HnAbort("KheSolnAssignRequestedResources internal error");
      }
    }
  }

  /* and return */
  KheSolnArenaEnd(soln, a);
  if( DEBUG5 )
    KheFrameAssertNoClashes(days_frame, soln);
  if( DEBUG1 )
  {
    if( soln_changed )
      fprintf(stderr,
	"] KheSolnAssignRequestedResources returning true (%.5f -> %.5f)\n",
	KheCostShow(init_cost), KheCostShow(KheSolnCost(soln)));
    else
      fprintf(stderr, "] KheSolnAssignRequestedResources returning false\n");
  }
  return soln_changed;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorHasSingletonTimeGroup(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)*/
/*                                                                           */
/*  Return true if cbtm contains a singleton time group.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheMonitorHasSingletonTimeGroup(KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm)
{
  int i;  KHE_TIME_GROUP tg;  KHE_POLARITY po;
  for( i = 0;  i < KheClusterBusyTimesMonitorTimeGroupCount(cbtm);  i++ )
  {
    tg = KheClusterBusyTimesMonitorTimeGroup(cbtm, i, &po);
    if( KheTimeGroupTimeCount(tg) == 1 )
      return true;
  }
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheMonitorRequestsSpecificBusyTimes(KHE_MONITOR m)                  */
/*  bool KheMonitorIsRequestedForced(KHE_MONITOR m)                          */
/*                                                                           */
/*  Return true if m embodies a request for a resource to be busy at a       */
/*  specific time, or several such requests.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheMonitorRequestsSpecificBusyTimes(KHE_MONITOR m)
{
  KHE_CLUSTER_BUSY_TIMES_MONITOR cbtm;  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  KHE_LIMIT_BUSY_TIMES_MONITOR lbtm;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
  int i, min_limit, junk;  KHE_TIME_GROUP tg;
  if( KheMonitorTag(m) == KHE_LIMIT_BUSY_TIMES_MONITOR_TAG )
  {
    lbtm = (KHE_LIMIT_BUSY_TIMES_MONITOR) m;
    lbtc = KheLimitBusyTimesMonitorConstraint(lbtm);
    min_limit = KheLimitBusyTimesConstraintMinimum(lbtc);
    if( KheLimitBusyTimesConstraintMinimum(lbtc) > 0 &&
	!KheLimitBusyTimesConstraintAllowZero(lbtc) )
    {
      for( i = 0;  i < KheLimitBusyTimesMonitorTimeGroupCount(lbtm);  i++ )
      {
	tg = KheLimitBusyTimesMonitorTimeGroup(lbtm, i, &junk);
	if( KheTimeGroupTimeCount(tg) <= min_limit )
	  return true;
      }
    }
  }
  else if( KheMonitorTag(m) == KHE_CLUSTER_BUSY_TIMES_MONITOR_TAG )
  {
    cbtm = (KHE_CLUSTER_BUSY_TIMES_MONITOR) m;
    cbtc = KheClusterBusyTimesMonitorConstraint(cbtm);
    if( KheClusterBusyTimesConstraintMinimum(cbtc) >=
	KheClusterBusyTimesConstraintTimeGroupCount(cbtc) &&
	!KheClusterBusyTimesConstraintAllowZero(cbtc) &&
        KheClusterBusyTimesConstraintAllPositive(cbtc) &&
        KheMonitorHasSingletonTimeGroup(cbtm) )
    {
      /* minimum limit forces assignments if any time groups are singletons */
      return true;
    }
    if( KheClusterBusyTimesConstraintMaximum(cbtc) == 0 &&
	KheClusterBusyTimesConstraintAllNegative(cbtc) &&
        KheMonitorHasSingletonTimeGroup(cbtm) )
    {
      /* maximum limit forces assignments if any time groups are singletons */
      return true;
    }
  }

  /* no luck */
  return false;
}

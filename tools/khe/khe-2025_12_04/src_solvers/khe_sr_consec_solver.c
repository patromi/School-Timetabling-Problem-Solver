
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
/*  FILE:         khe_sr_consec_solver.c                                     */
/*  DESCRIPTION:  Limits on consecutive days                                 */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"
#include <limits.h>
#define bool_show(x) ((x) ? "true" : "false")
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define	DEBUG1	0

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_LIMITS - a set of limits of one kind for one resource         */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_consec_limits_rec {
  int		history;
  int		min_limit;
  int		max_limit;
} KHE_CONSEC_LIMITS;

typedef HA_ARRAY(KHE_CONSEC_LIMITS) ARRAY_KHE_CONSEC_LIMITS;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_RESOURCE - everything about one resource                      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_consec_resource_rec {
  KHE_RESOURCE					resource;	/* resource */
  KHE_CONSEC_LIMITS				free_days;
  KHE_CONSEC_LIMITS				busy_days;
  ARRAY_KHE_CONSEC_LIMITS			busy_times;
  int						non_rigidity;
} *KHE_CONSEC_RESOURCE;

typedef HA_ARRAY(KHE_CONSEC_RESOURCE) ARRAY_KHE_CONSEC_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_SOLVER - the solver                                           */
/*                                                                           */
/*****************************************************************************/

struct khe_consec_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_FRAME			frame;
  int				max_offset;
  ARRAY_KHE_CONSEC_RESOURCE	resources;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSEC_LIMITS objects (private)"                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_LIMITS KheConsecLimitsMake(int history, int min_limit,         */
/*    int max_limit)                                                         */
/*                                                                           */
/*  Return a new consec limits object with these attributes.                 */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSEC_LIMITS KheConsecLimitsMake(int history, int min_limit,
  int max_limit)
{
  KHE_CONSEC_LIMITS res;
  res.history = history;
  res.min_limit = min_limit;
  res.max_limit = max_limit;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_LIMITS KheConsecLimitsMerge(KHE_CONSEC_LIMITS cl,             */
/*    int history, int min_limit, int max_limit)                             */
/*                                                                           */
/*  Return a consec limits object that merges cl with the new values.        */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSEC_LIMITS KheConsecLimitsMerge(KHE_CONSEC_LIMITS cl,
  int history, int min_limit, int max_limit)
{
  return KheConsecLimitsMake(max(cl.history, history),
    max(cl.min_limit, min_limit), min(cl.max_limit, max_limit));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecLimitsDebug(KHE_CONSEC_LIMITS cl, int indent, FILE *fp)    */
/*                                                                           */
/*  Debug print of cl onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void KheConsecLimitsDebug(KHE_CONSEC_LIMITS cl, int indent, FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%d|%d-%d", cl.history, cl.min_limit, cl.max_limit);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSEC_RESOURCE"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_RESOURCE KheConsecResourceMake(KHE_RESOURCE r,                */
/*    int busy_times_count, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a new consec resource object for r.                                 */
/*                                                                           */
/*****************************************************************************/

static KHE_CONSEC_RESOURCE KheConsecResourceMake(KHE_RESOURCE r,
  int max_offset, int frame_len, HA_ARENA a)
{
  KHE_CONSEC_RESOURCE res;  int i;  KHE_CONSEC_LIMITS init_cl;
  init_cl = KheConsecLimitsMake(0, 1, frame_len);
  HaMake(res, a);
  res->resource = r;
  res->free_days = init_cl;
  res->busy_days = init_cl;
  HaArrayInit(res->busy_times, a);
  for( i = 0;  i <= max_offset;  i++ )
    HaArrayAddLast(res->busy_times, init_cl);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecResourceDebug(KHE_CONSEC_RESOURCE cr, int indent, FILE *fp)*/
/*                                                                           */
/*  Debug print of cr onto fp with the given indent.                         */
/*                                                                           */
/*****************************************************************************/

static void KheConsecResourceDebug(KHE_CONSEC_RESOURCE cr, int indent, FILE *fp)
{
  KHE_CONSEC_LIMITS cl;  int i;
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "[ %5s: free days ", KheResourceId(cr->resource));
  KheConsecLimitsDebug(cr->free_days, -1, fp);
  fprintf(fp, "; busy days ");
  KheConsecLimitsDebug(cr->busy_days, -1, fp);
  fprintf(fp, "; busy times ");
  HaArrayForEach(cr->busy_times, cl, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%d:", i);
    KheConsecLimitsDebug(cl, -1, fp);
  }
  fprintf(fp, "; non_rigidity %d", cr->non_rigidity);
  fprintf(fp, " ]");
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraint handling"                                          */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintCoversFrame(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,   */
/*    KHE_FRAME frame)                                                       */
/*                                                                           */
/*  Return true if c's time groups equal frame's time groups.                */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintCoversFrame(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  KHE_FRAME frame)
{
  int count, i;  KHE_TIME_GROUP c_tg, f_tg;  KHE_POLARITY po;

  /* the number of time groups must be equal */
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  if( count != KheFrameTimeGroupCount(frame) )
    return false;

  /* the individual time groups must be equal */
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheLimitActiveIntervalsConstraintTimeGroup(c, i, 0, &po);
    f_tg = KheFrameTimeGroup(frame, i);
    if( !KheTimeGroupEqual(c_tg, f_tg) )
      return false;
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheConstraintCoversOffset(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,  */
/*    KHE_FRAME frame, int *offset)                                          */
/*                                                                           */
/*  Return true if c's time groups are singleton time groups that each       */
/*  contain one time from the corresponding frame's time group, with all     */
/*  times being at the same offset, returned in *offset.                     */
/*                                                                           */
/*****************************************************************************/

static bool KheConstraintCoversOffset(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c,
  KHE_FRAME frame, int *offset)
{
  int count, i, pos;  KHE_TIME_GROUP c_tg, f_tg;  KHE_POLARITY po;

  /* the number of time groups must be equal */
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  if( count != KheFrameTimeGroupCount(frame) )
    return *offset = -1, false;

  /* each time group of the constraint must be a singleton time group */
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheLimitActiveIntervalsConstraintTimeGroup(c, i, 0, &po);
    if( KheTimeGroupTimeCount(c_tg) != 1 )
      return *offset = -1, false;
  }

  /* find the offset of the first singleton time */
  c_tg = KheLimitActiveIntervalsConstraintTimeGroup(c, 0, 0, &po);
  f_tg = KheFrameTimeGroup(frame, 0);
  if( !KheTimeGroupContains(f_tg, KheTimeGroupTime(c_tg, 0), &pos) )
    return *offset = -1, false;

  /* each subsequent time group must have the same pos */
  for( i = 0;  i < count;  i++ )
  {
    c_tg = KheLimitActiveIntervalsConstraintTimeGroup(c, i, 0, &po);
    f_tg = KheFrameTimeGroup(frame, i);
    if( pos >= KheTimeGroupTimeCount(f_tg) ||
	KheTimeGroupTime(f_tg, pos) != KheTimeGroupTime(c_tg, 0) )
      return *offset = -1, false;
  }

  /* all good */
  return *offset = pos, true;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_RESOURCE KheConsecSolverGetResource(KHE_CONSEC_SOLVER cs,     */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Get the KHE_CONSEC_RESOURCE object of cs corresponding to r.             */
/*                                                                           */
/*****************************************************************************/

KHE_CONSEC_RESOURCE KheConsecSolverGetResource(KHE_CONSEC_SOLVER cs,
  KHE_RESOURCE r)
{
  return HaArray(cs->resources, KheResourceInstanceIndex(r));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddFreeDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,          */
/*    int history, int min_limit, int max_limit)                             */
/*                                                                           */
/*  Add the given free days limits for r.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheAddFreeDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int history, int min_limit, int max_limit)
{
  KHE_CONSEC_RESOURCE cr;
  cr = KheConsecSolverGetResource(cs, r);
  cr->free_days = KheConsecLimitsMerge(cr->free_days,
    history, min_limit, max_limit);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddBusyDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,          */
/*    int history, int min_limit, int max_limit)                             */
/*                                                                           */
/*  Add the given busy days limits for r.                                    */
/*                                                                           */
/*****************************************************************************/

static void KheAddBusyDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int history, int min_limit, int max_limit)
{
  KHE_CONSEC_RESOURCE cr;
  cr = KheConsecSolverGetResource(cs, r);
  cr->busy_days = KheConsecLimitsMerge(cr->busy_days,
    history, min_limit, max_limit);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddBusyTimesLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,         */
/*    int offset, int history, int min_limit, int max_limit)                 */
/*                                                                           */
/*  Add the given busy times limits for r at offset.                         */
/*                                                                           */
/*****************************************************************************/

static void KheAddBusyTimesLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int offset, int history, int min_limit, int max_limit)
{
  KHE_CONSEC_RESOURCE cr;  KHE_CONSEC_LIMITS cl;
  cr = KheConsecSolverGetResource(cs, r);
  HnAssert(offset >= 0 && offset < HaArrayCount(cr->busy_times),
    "KheAddBusyTimesLimits internal error");
  cl = KheConsecLimitsMerge(HaArray(cr->busy_times, offset),
    history, min_limit, max_limit);
  HaArrayPut(cr->busy_times, offset, cl);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverAddConstraint(KHE_CONSEC_SOLVER cs,                  */
/*    KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)                               */
/*                                                                           */
/*  Add c's limits to cs.                                                    */
/*                                                                           */
/*****************************************************************************/

static void KheConsecSolverAddConstraint(KHE_CONSEC_SOLVER cs,
  KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT c)
{
  int count, c_min, c_max, c_hist, i, j, offset;  bool all_positive;
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE r;

  /* number of time groups must be non-zero and equal number in frame */
  count = KheLimitActiveIntervalsConstraintTimeGroupCount(c);
  if( count == 0 || count != KheFrameTimeGroupCount(cs->frame) )
      return;

  /* time groups must be all positive or all negative */
  if( KheLimitActiveIntervalsConstraintAllPositive(c) )
    all_positive = true;
  else if( KheLimitActiveIntervalsConstraintAllNegative(c) )
    all_positive = false;
  else
    return;

  /* get min and max */
  c_min = KheLimitActiveIntervalsConstraintMinimum(c);
  c_max = KheLimitActiveIntervalsConstraintMaximum(c);

  /* if constraint time groups equal frame time groups, do busy/free days */
  if( KheConstraintCoversFrame(c, cs->frame) )
  {
    /* handle resources lying in resource groups */
    for( i=0; i < KheLimitActiveIntervalsConstraintResourceGroupCount(c); i++ )
    {
      rg = KheLimitActiveIntervalsConstraintResourceGroup(c, i);
      for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      {
	r = KheResourceGroupResource(rg, j);
	c_hist = KheLimitActiveIntervalsConstraintHistory(c, r);
	if( all_positive )
	  KheAddBusyDaysLimits(cs, r, c_hist, c_min, c_max);
	else
	  KheAddFreeDaysLimits(cs, r, c_hist, c_min, c_max);
      }
    }

    /* handle separate resources */
    for( i = 0;  i < KheLimitActiveIntervalsConstraintResourceCount(c);  i++ )
    {
      r = KheLimitActiveIntervalsConstraintResource(c, i);
      c_hist = KheLimitActiveIntervalsConstraintHistory(c, r);
      if( all_positive )
	KheAddBusyDaysLimits(cs, r, c_hist, c_min, c_max);
      else
	KheAddFreeDaysLimits(cs, r, c_hist, c_min, c_max);
    }
  }

  /* if constraint covers one offset in the frame time groups, do busy times */
  if( all_positive && KheConstraintCoversOffset(c, cs->frame, &offset) )
  {
    /* handle resources lying in resource groups */
    for( i=0; i < KheLimitActiveIntervalsConstraintResourceGroupCount(c); i++ )
    {
      rg = KheLimitActiveIntervalsConstraintResourceGroup(c, i);
      for( j = 0;  j < KheResourceGroupResourceCount(rg);  j++ )
      {
	r = KheResourceGroupResource(rg, j);
	c_hist = KheLimitActiveIntervalsConstraintHistory(c, r);
	KheAddBusyTimesLimits(cs, r, offset, c_hist, c_min, c_max);
      }
    }

    /* handle separate resources */
    for( i = 0;  i < KheLimitActiveIntervalsConstraintResourceCount(c);  i++ )
    {
      r = KheLimitActiveIntervalsConstraintResource(c, i);
      c_hist = KheLimitActiveIntervalsConstraintHistory(c, r);
      KheAddBusyTimesLimits(cs, r, offset, c_hist, c_min, c_max);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "non_rigidity"                                                 */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int ceil_div(int a, int b)                                               */
/*                                                                           */
/*  Return ceiling(a/b).                                                     */
/*                                                                           */
/*****************************************************************************/

int ceil_div(int a, int b)
{
  return (a + b - 1) / b;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheFindBusyRuns(int frame_len, int total_busy, int busy_run_len,    */
/*    int free_run_len, int *busy_runs)                                      */
/*                                                                           */
/*  If the parameters lead to a valid timetable, return true and set         */
/*  *busy_runs to the number of busy runs in that timetable.  If not,        */
/*  return false with *busy_runs undefined.  The parameters are:             */
/*                                                                           */
/*  frame_len                                                                */
/*    The number of days in the cycle.                                       */
/*                                                                           */
/*  total_busy                                                               */
/*    The total number of times that we want the resource to be busy.        */
/*                                                                           */
/*  busy_run_len                                                             */
/*    The length of one busy run.  One busy run may be smaller.              */
/*                                                                           */
/*  free_run_len                                                             */
/*    The length of one free run.                                            */
/*                                                                           */
/*****************************************************************************/

static bool KheFindBusyRuns(int frame_len, int total_busy, int busy_run_len,
  int free_run_len, int *busy_runs)
{
  int min_total_free, max_total_free;

  /* find the number of busy runs */
  *busy_runs = ceil_div(total_busy, busy_run_len);

  /* min free times when all free runs lie between busy runs */
  min_total_free = free_run_len * (*busy_runs - 1);

  /* max free times when free runs lie before, between and after busy runs */
  max_total_free = free_run_len * (*busy_runs + 1);

  /* success if free times plus total_busy can add up to frame_len */
  return min_total_free + total_busy <= frame_len &&
    max_total_free + total_busy >= frame_len;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConsecSolverNonRigidity1(KHE_CONSEC_RESOURCE cr,                  */
/*    int frame_len, KHE_SOLN soln)                                          */
/*                                                                           */
/*  Return the value of the first measure of the non-rigidity of cr.  If     */
/*  there are any problems, return 0.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheConsecSolverNonRigidity1(KHE_CONSEC_RESOURCE cr,
  int frame_len, KHE_SOLN soln)
{
  int x, y, min_busy_runs, max_busy_runs, busy_runs, total_busy, res;
  if( DEBUG1 )
    fprintf(stderr, "[ KheConsecSolverSetRigidity(%s, %d, soln)\n",
      KheResourceId(cr->resource), frame_len);
  res = 0;
  if( KheResourceMaxBusyTimes(soln, cr->resource, &total_busy) )
  {
    min_busy_runs = INT_MAX;  max_busy_runs = 0;
    for( x = cr->busy_days.min_limit;  x <= cr->busy_days.max_limit;  x++ )
      for( y = cr->free_days.min_limit;  y <= cr->free_days.max_limit;  y++ )
      {
	if( KheFindBusyRuns(frame_len, total_busy, x, y, &busy_runs) )
	{
	  if( busy_runs < min_busy_runs )
	    min_busy_runs = busy_runs;
	  if( busy_runs > max_busy_runs )
	    max_busy_runs = busy_runs;
	}
      }
    if( min_busy_runs < INT_MAX )
    {
      res = max_busy_runs - min_busy_runs;
      if( DEBUG1 )
	fprintf(stderr, "  max_busy_runs %d, min_busy_runs %d\n",
          max_busy_runs, min_busy_runs);
    }
  }
  if( DEBUG1 )
    fprintf(stderr, "] KheConsecSolverSetRigidity returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConsecSolverNonRigidity2(KHE_CONSEC_RESOURCE cr, KHE_SOLN soln)   */
/*                                                                           */
/*  Return the value of the second measure of the non-rigidity of cr.        */
/*                                                                           */
/*****************************************************************************/

static int KheConsecSolverNonRigidity2(KHE_CONSEC_RESOURCE cr, KHE_SOLN soln)
{
  int b_min, b_max, f_min, f_max, b, f;

  /* get b_min, b_max, and b, and make sure they have consistent values */
  b_min = cr->busy_days.min_limit;
  b_max = cr->busy_days.max_limit;
  b     = cr->busy_days.history;
  if( b_min < 1 ) b_min = 1;
  if( b_max < b_min ) b_max = b_min;
  if( b < 0 ) b = 0;

  /* get f_min, f_max, and f, and make sure they have consistent values */
  f_min = cr->free_days.min_limit;
  f_max = cr->free_days.max_limit;
  f     = cr->free_days.history;
  if( f_min < 1 ) f_min = 1;
  if( f_max < f_min ) f_max = f_min;
  if( f < 0 ) f = 0;

  /* apply the formula from the Guide */
  if( b == 0 )
  {
    if( f == 0 )
    {
      /* b == 0 and f == 0 */
      return f_max - f_min + 2;
    }
    else
    {
      /* b == 0 and f > 0 */
      if( f < f_min )
	return f_max - f_min + 1;
      else if( f < f_max )
	return 1 + f_max - f;
      else
	return 1;
    }
  }
  else
  {
    if( f == 0 )
    {
      /* b > 0 and f == 0 */
      if( b < b_min )
	return 1;
      else if( b < b_max )
	return f_max - f_min + 2;
      else
	return f_max - f_min + 1;
    }
    else
    {
      /* b > 0 and f > 0 */
      /* should never happen, but we are at the mercy of the history */
      /* values stored in the instance, and we don't want to crash */
      return 0;
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverSetNonRigidity(KHE_CONSEC_RESOURCE cr,               */
/*    int frame_len, KHE_SOLN soln)                                          */
/*                                                                           */
/*  Set cr->non_rigidity following the formula given in the Guide.           */
/*                                                                           */
/*****************************************************************************/

static void KheConsecSolverSetNonRigidity(KHE_CONSEC_RESOURCE cr,
  int frame_len, KHE_SOLN soln)
{
  cr->non_rigidity = 10 * KheConsecSolverNonRigidity1(cr, frame_len, soln) +
    1 * KheConsecSolverNonRigidity2(cr, soln);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "main functions"                                               */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_CONSEC_SOLVER KheConsecSolverMake(KHE_SOLN soln, KHE_FRAME frame)    */
/*                                                                           */
/*  Make a new consec solver object with these attributes.  Grab an arena    */
/*  from soln to store it in.                                                */
/*                                                                           */
/*****************************************************************************/

KHE_CONSEC_SOLVER KheConsecSolverMake(KHE_SOLN soln, KHE_FRAME frame)
{
  HA_ARENA a;  KHE_CONSEC_SOLVER res;  int i, max_offset, frame_len;
  KHE_RESOURCE r;  KHE_TIME_GROUP tg;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;
  KHE_CONSEC_RESOURCE cr;

  /* frame must be non-empty */
  HnAssert(KheFrameTimeGroupCount(frame) > 0,
    "KheConsecSolverMake: frame has no time groups");

  /* make the initial object */
  a = KheSolnArenaBegin(soln);
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->frame = frame;
  HaArrayInit(res->resources, a);

  /* find res->max_offset, one less than the smallest time group size */
  tg = KheFrameTimeGroup(frame, 0);
  res->max_offset = KheTimeGroupTimeCount(tg) - 1;
  for( i = 1;  i < KheFrameTimeGroupCount(frame);  i++ )
  {
    tg = KheFrameTimeGroup(frame, i);
    max_offset = KheTimeGroupTimeCount(tg) - 1;
    if( max_offset < res->max_offset )
      res->max_offset = max_offset;
  }

  /* add one KHE_CONSEC_RESOURCE for each resource of the instance */
  frame_len = KheFrameTimeGroupCount(frame);
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    HaArrayAddLast(res->resources,
      KheConsecResourceMake(r, res->max_offset, frame_len, a));
  }

  /* find all relevant constraints and add them in */
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintTag(c) == KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT_TAG &&
	KheConstraintWeight(c) > 0 )
      KheConsecSolverAddConstraint(res,
	(KHE_LIMIT_ACTIVE_INTERVALS_CONSTRAINT) c);
  }

  /* set the rigidities */
  HaArrayForEach(res->resources, cr, i)
    KheConsecSolverSetNonRigidity(cr, frame_len, soln);

  /* all done */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_SOLN KheConsecSolverSoln(KHE_CONSEC_SOLVER cs)                       */
/*                                                                           */
/*  Return the soln attribute of cs.                                         */
/*                                                                           */
/*****************************************************************************/

KHE_SOLN KheConsecSolverSoln(KHE_CONSEC_SOLVER cs)
{
  return cs->soln;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_FRAME KheConsecSolverFrame(KHE_CONSEC_SOLVER cs)                     */
/*                                                                           */
/*  Return the frame attribute of cs.                                        */
/*                                                                           */
/*****************************************************************************/

KHE_FRAME KheConsecSolverFrame(KHE_CONSEC_SOLVER cs)
{
  return cs->frame;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverDelete(KHE_CONSEC_SOLVER cs)                         */
/*                                                                           */
/*  Delete cs.                                                               */
/*                                                                           */
/*****************************************************************************/

void KheConsecSolverDelete(KHE_CONSEC_SOLVER cs)
{
  KheSolnArenaEnd(cs->soln, cs->arena);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverFreeDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r, */
/*    int *history, int *min_limit, int *max_limit)                          */
/*                                                                           */
/*  Return the free days history, min_limit, and max_limit values for r.     */
/*                                                                           */
/*****************************************************************************/

void KheConsecSolverFreeDayLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int *history, int *min_limit, int *max_limit)
{
  KHE_CONSEC_RESOURCE cr;  KHE_CONSEC_LIMITS cl;
  cr = KheConsecSolverGetResource(cs, r);
  cl = cr->free_days;
  *history = cl.history;
  *min_limit = cl.min_limit;
  *max_limit = cl.max_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverBusyDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r, */
/*    int *history, int *min_limit, int *max_limit)                          */
/*                                                                           */
/*  Return the busy days history, min_limit, and max_limit values for r.     */
/*                                                                           */
/*****************************************************************************/

void KheConsecSolverBusyDaysLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int *history, int *min_limit, int *max_limit)
{
  KHE_CONSEC_RESOURCE cr;  KHE_CONSEC_LIMITS cl;
  cr = KheConsecSolverGetResource(cs, r);
  cl = cr->busy_days;
  *history = cl.history;
  *min_limit = cl.min_limit;
  *max_limit = cl.max_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverBusyTimesLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,*/
/*    int offset, int *history, int *min_limit, int *max_limit)              */
/*                                                                           */
/*  Return the busy times history, min_limit, and max_limit values for r.    */
/*                                                                           */
/*****************************************************************************/

void KheConsecSolverBusyTimesLimits(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r,
  int offset, int *history, int *min_limit, int *max_limit)
{
  KHE_CONSEC_RESOURCE cr;  KHE_CONSEC_LIMITS cl;
  cr = KheConsecSolverGetResource(cs, r);
  HnAssert(offset >= 0 && offset < HaArrayCount(cr->busy_times),
    "KheConsecSolverBusyTimeLimits: offset (%d) out of range (0 .. %d)",
    offset, HaArrayCount(cr->busy_times) - 1);
  cl = HaArray(cr->busy_times, offset);
  *history = cl.history;
  *min_limit = cl.min_limit;
  *max_limit = cl.max_limit;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConsecSolverMaxOffset(KHE_CONSEC_SOLVER cs)                       */
/*                                                                           */
/*  Return the max_offset attribute of cs.                                   */
/*                                                                           */
/*****************************************************************************/

int KheConsecSolverMaxOffset(KHE_CONSEC_SOLVER cs)
{
  return cs->max_offset;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConsecSolverNonRigidity(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r)     */
/*                                                                           */
/*  Return the non_rigidity of r.                                            */
/*                                                                           */
/*****************************************************************************/

int KheConsecSolverNonRigidity(KHE_CONSEC_SOLVER cs, KHE_RESOURCE r)
{
  KHE_CONSEC_RESOURCE cr;
  cr = KheConsecSolverGetResource(cs, r);
  return cr->non_rigidity;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheConsecSolverDebug(KHE_CONSEC_SOLVER cs, int verbosity,           */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of cs onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

void KheConsecSolverDebug(KHE_CONSEC_SOLVER cs, int verbosity,
  int indent, FILE *fp)
{
  KHE_CONSEC_RESOURCE cr;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ Consec Solver(max_offset %d)\n", indent, "",
      cs->max_offset);
    if( verbosity >= 2 )
      HaArrayForEach(cs->resources, cr, i)
	KheConsecResourceDebug(cr, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    fprintf(fp, "Consec Solver(max_offset %d)", cs->max_offset);
}

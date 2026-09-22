
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

#define DEBUG1	0


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
  KHE_CONSEC_LIMITS				free_days_limits;
  KHE_CONSEC_LIMITS				busy_days_limits;
  ARRAY_KHE_CONSEC_LIMITS			busy_times_limits;
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
  /* ***
  if( cl.history == 0 )
    fprintf(fp, "%d-%d", cl.min_limit, cl.max_limit);
  else
  *** */
    fprintf(fp, "%d|%d-%d", cl.history, cl.min_limit, cl.max_limit);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_CONSEC_RESOURCE objects (private)"                        */
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
  int max_offset, HA_ARENA a)
{
  KHE_CONSEC_RESOURCE res;  int i;  KHE_CONSEC_LIMITS init_cl;
  init_cl = KheConsecLimitsMake(0, 0, INT_MAX);
  HaMake(res, a);
  res->resource = r;
  res->free_days_limits = init_cl;
  res->busy_days_limits = init_cl;
  HaArrayInit(res->busy_times_limits, a);
  for( i = 0;  i <= max_offset;  i++ )
    HaArrayAddLast(res->busy_times_limits, init_cl);
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
  KheConsecLimitsDebug(cr->free_days_limits, -1, fp);
  fprintf(fp, "; busy days ");
  KheConsecLimitsDebug(cr->busy_days_limits, -1, fp);
  fprintf(fp, "; busy times ");
  HaArrayForEach(cr->busy_times_limits, cl, i)
  {
    if( i > 0 )
      fprintf(fp, ", ");
    fprintf(fp, "%d:", i);
    KheConsecLimitsDebug(cl, -1, fp);
  }
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
  cr->free_days_limits = KheConsecLimitsMerge(cr->free_days_limits,
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
  cr->busy_days_limits = KheConsecLimitsMerge(cr->busy_days_limits,
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
  HnAssert(offset >= 0 && offset < HaArrayCount(cr->busy_times_limits),
    "KheAddBusyTimesLimits internal error");
  cl = KheConsecLimitsMerge(HaArray(cr->busy_times_limits, offset),
    history, min_limit, max_limit);
  HaArrayPut(cr->busy_times_limits, offset, cl);
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
  HA_ARENA a;  KHE_CONSEC_SOLVER res;  int i, max_offset;
  KHE_RESOURCE r;  KHE_TIME_GROUP tg;  KHE_INSTANCE ins;  KHE_CONSTRAINT c;

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
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceResourceCount(ins);  i++ )
  {
    r = KheInstanceResource(ins, i);
    HaArrayAddLast(res->resources,
      KheConsecResourceMake(r, res->max_offset, a));
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
  cl = cr->free_days_limits;
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
  cl = cr->busy_days_limits;
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
  HnAssert(offset >= 0 && offset < HaArrayCount(cr->busy_times_limits),
    "KheConsecSolverBusyTimeLimits: offset (%d) out of range (0 .. %d)",
    offset, HaArrayCount(cr->busy_times_limits) - 1);
  cl = HaArray(cr->busy_times_limits, offset);
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

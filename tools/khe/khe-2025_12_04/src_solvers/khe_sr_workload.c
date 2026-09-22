
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
/*  FILE:         khe_sr_workload.c                                          */
/*  DESCRIPTION:  Workload packing                                           */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1	0
#define DEBUG2	0
#define DEBUG3	0

/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_RESOURCE - a resource with associated attributes                  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_wp_resource_rec {
  KHE_RESOURCE			resource;		/* the resource      */
  float				max_workload;		/* its max workload  */
  int				matches[2];		/* one for each wers */
} *KHE_WP_RESOURCE;

typedef HA_ARRAY(KHE_WP_RESOURCE) ARRAY_KHE_WP_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_EVENT_RESOURCE_SET - a set of event resources of equal workload   */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_EVENT_RESOURCE) ARRAY_KHE_EVENT_RESOURCE;

typedef struct khe_wp_event_resource_set_rec {
  int				index;			/* 0 or 1            */
  int				workload;		/* per event resource*/
  int				max_resources;		/* called c_i in doc */
  ARRAY_KHE_EVENT_RESOURCE	event_resources;	/* event resources   */
  ARRAY_KHE_WP_RESOURCE		resources;		/* domain            */
} *KHE_WP_EVENT_RESOURCE_SET;

typedef HA_ARRAY(KHE_WP_EVENT_RESOURCE_SET) ARRAY_KHE_WP_EVENT_RESOURCE_SET;


/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_SOLVER                                                            */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_wp_solver_rec {
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_OPTIONS			options;
  KHE_RESOURCE_TYPE		resource_type;
  ARRAY_KHE_WP_RESOURCE		resources;
  ARRAY_KHE_WP_EVENT_RESOURCE_SET event_resource_sets;
  float				total_supply;
  int				total_demand;
  /* KHE_TASK_BOUND_GROUP	task_bound_group; */
} *KHE_WP_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WP_RESOURCE"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_RESOURCE KheWpResourceMake(KHE_RESOURCE r, float max_workload,    */
/*    HA_ARENA a)                                                            */
/*                                                                           */
/*  Return a new resource object with no matches initially.                  */
/*                                                                           */
/*****************************************************************************/

static KHE_WP_RESOURCE KheWpResourceMake(KHE_RESOURCE r, float max_workload,
  HA_ARENA a)
{
  KHE_WP_RESOURCE res;
  HaMake(res, a);
  res->resource = r;
  res->max_workload = max_workload;
  res->matches[0] = res->matches[1] = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpResourceSuitsDistinctWorkload(KHE_WP_RESOURCE wr,              */
/*    KHE_WP_EVENT_RESOURCE_SET wers)                                        */
/*                                                                           */
/*  Return true if wr is suited to wers.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheWpResourceSuitsDistinctWorkload(KHE_WP_RESOURCE wr,
  KHE_WP_EVENT_RESOURCE_SET wers)
{
  return wers->workload * wr->matches[wers->index] >= wr->max_workload;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpResourceDebug(KHE_WP_RESOURCE wr, int verbosity, int indent,   */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of wr onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheWpResourceDebug(KHE_WP_RESOURCE wr, int verbosity, int indent,
  FILE *fp)
{
  if( indent > 0 )
    fprintf(fp, "%*s", indent, "");
  fprintf(fp, "%s:%.1f:%d:%d", KheResourceId(wr->resource),
    wr->max_workload, wr->matches[0], wr->matches[1]);
  if( indent >= 0 )
    fprintf(fp, "\n");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WP_EVENT_RESOURCE_SET"                                    */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_EVENT_RESOURCE_SET KheWpEventResourceSetMake(int index,           */
/*    int workload, HA_ARENA a)                                              */
/*                                                                           */
/*  Make an event resource set object, initially with no event resources.    */
/*                                                                           */
/*****************************************************************************/

static KHE_WP_EVENT_RESOURCE_SET KheWpEventResourceSetMake(int index,
  int workload, HA_ARENA a)
{
  KHE_WP_EVENT_RESOURCE_SET res;
  HaMake(res, a);
  res->index = index;
  res->workload = workload;
  HaArrayInit(res->event_resources, a);
  HaArrayInit(res->resources, a);
  res->max_resources = -1;   /* undefined until set later */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpEventResourceSetAddEventResource(                              */
/*    KHE_WP_EVENT_RESOURCE_SET wers, KHE_EVENT_RESOURCE er,                 */
/*    KHE_WP_SOLVER wp)                                                      */
/*                                                                           */
/*  Add er to wers.  Also update the matches[] field of er's resources.      */
/*                                                                           */
/*****************************************************************************/

static void KheWpEventResourceSetAddEventResource(
  KHE_WP_EVENT_RESOURCE_SET wers, KHE_EVENT_RESOURCE er, KHE_WP_SOLVER wp)
{
  KHE_RESOURCE_GROUP domain;  int i;  KHE_RESOURCE r;  KHE_WP_RESOURCE wr;
  HaArrayAddLast(wers->event_resources, er);
  domain = KheEventResourceHardAndSoftDomain(er);
  for( i = 0;  i < KheResourceGroupResourceCount(domain);  i++ )
  {
    r = KheResourceGroupResource(domain, i);
    wr = HaArray(wp->resources, KheResourceResourceTypeIndex(r));
    wr->matches[wers->index]++;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpEventResourceSetInitMaxResources(                              */
/*    KHE_WP_EVENT_RESOURCE_SET wers, KHE_WP_EVENT_RESOURCE_SET other_wers)  */
/*                                                                           */
/*  Initialize the max_workload field of wers.  This can only be done        */
/*  after all event resources are added.                                     */
/*                                                                           */
/*  This field is called c_i in the documentation.  See the documentation    */
/*  for why it has this peculiar-looking value.                              */
/*                                                                           */
/*****************************************************************************/

static void KheWpEventResourceSetInitMaxResources(
  KHE_WP_EVENT_RESOURCE_SET wers, KHE_WP_EVENT_RESOURCE_SET other_wers)
{
  wers->max_resources = HaArrayCount(wers->event_resources) /
    other_wers->workload;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpEventResourceSetAddResource(KHE_WP_EVENT_RESOURCE_SET wers,    */
/*    KHE_WP_RESOURCE wr)                                                    */
/*                                                                           */
/*  Add wr to wers, returning false if can't.  This is assuming that         */
/*  KheWpEventResourceSetInitMaxResources has been called by now.            */
/*                                                                           */
/*****************************************************************************/

static bool KheWpEventResourceSetAddResource(KHE_WP_EVENT_RESOURCE_SET wers,
  KHE_WP_RESOURCE wr)
{
  if( HaArrayCount(wers->resources) >= wers->max_resources )
    return false;
  HaArrayAddLast(wers->resources, wr);
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpEventResourceSetAddTaskBounds(KHE_WP_EVENT_RESOURCE_SET wers,  */
/*    KHE_WP_SOLVER wp, KHE_TASK_BOUND_GROUP tbg)                            */
/*                                                                           */
/*  Assuming that all is in order, build a task bound object and add it      */
/*  to the event resources of wers, and also to tbg.                         */
/*                                                                           */
/*  Some task bounds may be declined by some tasks.  We ignore that.         */
/*                                                                           */
/*****************************************************************************/

static void KheWpEventResourceSetAddTaskBounds(KHE_WP_EVENT_RESOURCE_SET wers,
  KHE_WP_SOLVER wp, KHE_SOLN_ADJUSTER sa /* KHE_TASK_BOUND_GROUP tbg */)
{
  KHE_WP_RESOURCE wr;  int i, j;  KHE_RESOURCE_GROUP rg;  KHE_TASK_BOUND tb;
  KHE_EVENT_RESOURCE er;  KHE_TASK task;

  /* build the resource group that will go into the task bound */
  KheSolnResourceGroupBegin(wp->soln, wp->resource_type);
  HaArrayForEach(wers->resources, wr, i)
    KheSolnResourceGroupAddResource(wp->soln, wr->resource);
  rg = KheSolnResourceGroupEnd(wp->soln);

  /* build the task bound and add it to tbg */
  tb = KheSolnAdjusterTaskBoundMake(sa, wp->soln, rg);
  /* ***
  tb = KheTaskBoundMake(wp->soln, rg);
  KheTaskBoundGroupAddTaskBound(tbg, tb);
  *** */

  /* add the task bound to the tasks of the event resources of wers */
  HaArrayForEach(wers->event_resources, er, i)
    for( j = 0;  j < KheEventResourceTaskCount(wp->soln, er);  j++ )
    {
      task = KheEventResourceTask(wp->soln, er, j);
      KheTaskAddTaskBound(task, tb);   /* ignore any failures */
    }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpEventResourceSetDebugHeader(KHE_WP_EVENT_RESOURCE_SET wers,    */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of the header part of wers onto fp with no indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheWpEventResourceSetDebugHeader(KHE_WP_EVENT_RESOURCE_SET wers,
  FILE *fp)
{
  fprintf(fp, "EventResourceSet %d(workload %d, event resources %d,"
    " max_workload %d)", wers->index, wers->workload,
    HaArrayCount(wers->event_resources), wers->max_resources);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpEventResourceSetDebug(KHE_WP_EVENT_RESOURCE_SET wers,          */
/*    int verbosity, int indent, FILE *fp)                                   */
/*                                                                           */
/*  Debug print of wers onto fp with the given verbosity and indent.         */
/*                                                                           */
/*****************************************************************************/

static void KheWpEventResourceSetDebug(KHE_WP_EVENT_RESOURCE_SET wers,
  int verbosity, int indent, FILE *fp)
{
  KHE_WP_RESOURCE wr;  int i;
  if( indent >= 0 )
  {
    fprintf(fp, "%*s[ ", indent, "");
    KheWpEventResourceSetDebugHeader(wers, fp);
    fprintf(fp, "\n");
    HaArrayForEach(wers->resources, wr, i)
      KheWpResourceDebug(wr, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
  else
    KheWpEventResourceSetDebugHeader(wers, fp);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_WP_SOLVER"                                                */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_WP_SOLVER KheWpSolverMake(KHE_SOLN soln, KHE_OPTIONS options,        */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a workload packer with these attributes.                            */
/*                                                                           */
/*****************************************************************************/

static KHE_WP_SOLVER KheWpSolverMake(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE_TYPE rt, /* KHE_TASK_BOUND_GROUP tbg, */ HA_ARENA a)
{
  KHE_WP_SOLVER res;
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->options = options;
  res->resource_type = rt;
  HaArrayInit(res->resources, a);
  HaArrayInit(res->event_resource_sets, a);
  res->total_supply = 0.0;
  res->total_demand = 0;
  /* res->task_bound_group = tbg; */
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpSolverAddResource(KHE_WP_SOLVER wp, KHE_RESOURCE r)            */
/*                                                                           */
/*  Add r to wp, returning true if successful.                               */
/*                                                                           */
/*****************************************************************************/

static bool KheWpSolverAddResource(KHE_WP_SOLVER wp, KHE_RESOURCE r)
{
  float max_workload;  KHE_WP_RESOURCE wr;
  if( KheResourceMaxWorkload(wp->soln, r, &max_workload) )
  {
    wp->total_supply += max_workload;
    wr = KheWpResourceMake(r, max_workload, wp->arena);
    HaArrayAddLast(wp->resources, wr);
    if( DEBUG2 )
      KheWpResourceDebug(wr, 2, 2, stderr);
    return true;
  }
  else
  {
    if( DEBUG2 )
      fprintf(stderr, "  %s: no max_workload)\n", KheResourceId(r));
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpSolverAddEventResource(KHE_WP_SOLVER wp, KHE_EVENT_RESOURCE er)*/
/*                                                                           */
/*  Record the existence of event resource er.  Return true if the number    */
/*  of distinct workloads does not change, or increases to at most 2.        */
/*                                                                           */
/*  Event resources that do not need assignment or whose workload is 0       */
/*  can be ignored; this function ignores them.                              */
/*                                                                           */
/*****************************************************************************/

static bool KheWpSolverAddEventResource(KHE_WP_SOLVER wp, KHE_EVENT_RESOURCE er)
{
  KHE_WP_EVENT_RESOURCE_SET wers;  int i, workload, wers_count;

  workload = KheEventResourceWorkload(er);
  if( KheEventResourceNeedsAssignment(er) == KHE_YES && workload > 0 )
  {
    /* find an existing entry and update that */
    HaArrayForEach(wp->event_resource_sets, wers, i)
      if( wers->workload == workload )
      {
	KheWpEventResourceSetAddEventResource(wers, er, wp);
	wp->total_demand += workload;
	return true;
      }

    /* make sure a new distinct workload would not put us over the limit */
    wers_count = HaArrayCount(wp->event_resource_sets);
    if( wers_count == 2 )
      return false;

    /* add a new distinct workload */
    wers = KheWpEventResourceSetMake(wers_count, workload, wp->arena);
    HaArrayAddLast(wp->event_resource_sets, wers);
    KheWpEventResourceSetAddEventResource(wers, er, wp);
    wp->total_demand += workload;
  }
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheGreatestCommonDivisor(int v1, int v2)                             */
/*                                                                           */
/*  Return gcd(v1, v2).                                                      */
/*                                                                           */
/*****************************************************************************/
#define swap(a, b, tmp) (tmp = a, a = b, b = tmp)

static int KheGreatestCommonDivisor(int v1, int v2)
{
  int tmp;
  if( v1 < v2 )
    swap(v1, v2, tmp);
  while( v2 > 0 )
  {
    v1 = v1 % v2;
    swap(v1, v2, tmp);
  }
  return v1;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpSolverOpenToSolving(KHE_WP_SOLVER wp)                          */
/*                                                                           */
/*  Return true if wp is open to solving.                                    */
/*                                                                           */
/*****************************************************************************/

static bool KheWpSolverOpenToSolving(KHE_WP_SOLVER wp)
{
  KHE_WP_EVENT_RESOURCE_SET wers1, wers2;  int workload, i;  KHE_WP_RESOURCE wr;

  /* there must be exactly two distinct non-zero event resource workloads */
  if( HaArrayCount(wp->event_resource_sets) != 2 )
    return false;

  /* the two distinct workloads must be relatively prime */
  wers1 = HaArrayFirst(wp->event_resource_sets);
  wers2 = HaArrayLast(wp->event_resource_sets);
  if( KheGreatestCommonDivisor(wers1->workload, wers2->workload) != 1 )
    return false;

  /* all resources must have maximum workload wers1 * wers2 */
  workload = wers1->workload * wers2->workload;
  HaArrayForEach(wp->resources, wr, i)
    if( wr->max_workload != (float) workload )
      return false;

  /* total demand must equal or exceed total supply */
  if( wp->total_supply > (float) wp->total_demand )
    return false;

  /* good to go */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpSolverFindDistributeleChoiceResources(KHE_WP_SOLVER wp,        */
/*    KHE_WP_EVENT_RESOURCE_SET wers, KHE_WP_EVENT_RESOURCE_SET other_wers)  */
/*                                                                           */
/*  Add to wers the resources for which wers is the only option.  Return     */
/*  true if nothing gets overloaded.                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static bool KheWpSolverDistributeSingleChoiceResources(KHE_WP_SOLVER wp,
  KHE_WP_EVENT_RESOURCE_SET wers, KHE_WP_EVENT_RESOURCE_SET other_wers)
{
  KHE_WP_RESOURCE wr;  int i;
  HaArrayForEach(wp->resources, wr, i)
    if( KheWpResourceSuitsDistinctWorkload(wr, wers) &&
        !KheWpResourceSuitsDistinctWorkload(wr, other_wers) )
    {
      if( HaArrayCount(wers->resources) >= wers->max_resources )
	return false;
      HaArrayAddLast(wers->resources, wr);
    }
  return true;
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KheWpSolverDistributeResources(KHE_WP_SOLVER wp,                    */
/*    KHE_WP_EVENT_RESOURCE_SET wers1, KHE_WP_EVENT_RESOURCE_SET wers2)      */
/*                                                                           */
/*  Distribute the resources between wers1 and wers2.                        */
/*                                                                           */
/*  Also make some checks to ensure that every resource gets distributed     */
/*  and no event resource set gets overloaded.  Return false if these        */
/*  checks do not all succeed.                                               */
/*                                                                           */
/*****************************************************************************/

static bool KheWpSolverDistributeResources(KHE_WP_SOLVER wp,
  KHE_WP_EVENT_RESOURCE_SET wers1, KHE_WP_EVENT_RESOURCE_SET wers2)
{
  KHE_WP_RESOURCE wr;  int i;  bool suits1, suits2;

  /* first pass:  distribute the fixed choice resources */
  HaArrayForEach(wp->resources, wr, i)
  {
    suits1 = KheWpResourceSuitsDistinctWorkload(wr, wers1);
    suits2 = KheWpResourceSuitsDistinctWorkload(wr, wers2);
    if( suits1 )
    {
      if( suits2 )
      {
	/* suits both wers1 and wers2; postpone to second pass */
      }
      else
      {
	/* suits wers1 but not wers2; try to add to wers1 */
        if( !KheWpEventResourceSetAddResource(wers1, wr) )
	  return false;
      }
    }
    else
    {
      if( suits2 )
      {
	/* suits wers2 but not wers1; try to add to wers2 */
        if( !KheWpEventResourceSetAddResource(wers2, wr) )
	  return false;
      }
      else
      {
	/* suits neither wers1 nor wers2; return false now */
	return false;
      }
    }
  }

  /* second pass:  distribute the free choice resources */
  HaArrayForEach(wp->resources, wr, i)
  {
    suits1 = KheWpResourceSuitsDistinctWorkload(wr, wers1);
    suits2 = KheWpResourceSuitsDistinctWorkload(wr, wers2);
    if( suits1 && suits2 )
    {
      /* wr is free choice, give it to any wers that can hold it */
      if( !KheWpEventResourceSetAddResource(wers1, wr) &&
          !KheWpEventResourceSetAddResource(wers2, wr) )
	return false;
    }
  }

  /* all good */
  return true;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheWpSolverDebug(KHE_WP_SOLVER wp, int verbosity, int indent,       */
/*    FILE *fp)                                                              */
/*                                                                           */
/*  Debug print of wp.                                                       */
/*                                                                           */
/*****************************************************************************/

static void KheWpSolverDebug(KHE_WP_SOLVER wp, int verbosity, int indent,
  FILE *fp)
{
  KHE_WP_EVENT_RESOURCE_SET wers;  int i;  KHE_WP_RESOURCE wr;
  fprintf(fp, "%*s[ WorkloadPacker(%s)\n", indent, "",
    KheInstanceId(KheSolnInstance(wp->soln)));
  HaArrayForEach(wp->event_resource_sets, wers, i)
    KheWpEventResourceSetDebug(wers, verbosity, indent + 2, fp);
  HaArrayForEach(wp->resources, wr, i)
    KheWpResourceDebug(wr, verbosity, indent + 2, fp);
  fprintf(fp, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "Public function"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KheWorkloadPack(KHE_SOLN soln, KHE_OPTIONS options,                 */
/*    KHE_RESOURCE_TYPE rt, KHE_TASK_BOUND_GROUP *tbg)                       */
/*                                                                           */
/*  Carry out workload packing for type rt.  If successful, set *tbg to      */
/*  a task bound group containing the task bounds that have been added       */
/*  to tasks of soln and return true.  Otherwise set *tbg to NULL and        */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

bool KheWorkloadPack(KHE_SOLN soln, KHE_OPTIONS options,
  KHE_RESOURCE_TYPE rt, KHE_SOLN_ADJUSTER sa /* KHE_TASK_BOUND_GROUP *tbg */)
{
  KHE_RESOURCE r;  int i;  HA_ARENA a;  KHE_WP_SOLVER wp;  KHE_INSTANCE ins;
  KHE_EVENT_RESOURCE er;   KHE_WP_EVENT_RESOURCE_SET wers1, wers2;
  if( DEBUG1 )
    fprintf(stderr, "[ KheWorkloadPack(soln of %s, options, %s, tbg)\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  /* make a workload packer object */
  a = KheSolnArenaBegin(soln);
  wp = KheWpSolverMake(soln, options, rt, /* *tbg, */ a);

  /* add resources to wp */
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    if( !KheWpSolverAddResource(wp, r) )
    {
      if( DEBUG1 )
	fprintf(stderr, "] KheWorkloadPack returning false (cannot add "
	  "resource %s)\n", KheResourceId(r));
      KheSolnArenaEnd(soln, a);
      return /* *tbg = NULL, */ false;
    }
  }

  /* add event resources to wp */
  ins = KheSolnInstance(soln);
  for( i = 0;  i < KheInstanceEventResourceCount(ins);  i++ )
  {
    er = KheInstanceEventResource(ins, i);
    if( KheEventResourceNeedsAssignment(er) == KHE_YES )
    {
      if( !KheWpSolverAddEventResource(wp, er) )
      {
	if( DEBUG1 )
	  fprintf(stderr, "] KheWorkloadPack returning false (cannot add event"
	    " resource %s.%d)\n", KheEventId(KheEventResourceEvent(er)),
	    KheEventResourceEventIndex(er));
	KheSolnArenaEnd(soln, a);
	return /* *tbg = NULL, */ false;
      }
    }
  }

  /* check whether open to solving */
  if( !KheWpSolverOpenToSolving(wp) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheWorkloadPack returning false (not open to "
	"solving)\n");
    KheSolnArenaEnd(soln, a);
    return /* *tbg = NULL, */ false;
  }

  /* open to solving, so do it */
  wers1 = HaArrayFirst(wp->event_resource_sets);
  wers2 = HaArrayLast(wp->event_resource_sets);
  KheWpEventResourceSetInitMaxResources(wers1, wers2);
  KheWpEventResourceSetInitMaxResources(wers2, wers1);
  if( DEBUG2 )
    fprintf(stderr, "  starting packing (%d resources, c1 = %d, c2 = %d)\n",
      KheResourceTypeResourceCount(rt), wers1->max_resources,
      wers2->max_resources);
  if( !KheWpSolverDistributeResources(wp, wers1, wers2) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheWorkloadPack returning false (cannot "
	"distribute resources)\n");
    KheSolnArenaEnd(soln, a);
    return /* *tbg = NULL, */ false;
  }
  /* *tbg = KheTaskBoundGroupMake(soln); */
  KheWpEventResourceSetAddTaskBounds(wers1, wp, sa /* *tbg */);
  KheWpEventResourceSetAddTaskBounds(wers2, wp, sa /* *tbg */);
  if( DEBUG3 )
    fprintf(stderr, "  KheWorkloadPack(%s, %s) returning true\n",
      KheInstanceId(KheSolnInstance(soln)), KheResourceTypeId(rt));

  /* wrapup */
  if( DEBUG2 )
    KheWpSolverDebug(wp, 2, 2, stderr);
  if( DEBUG1 )
    fprintf(stderr, "] KheWorkloadPack returning true (%d + %d resources)\n",
      wers1->max_resources, wers2->max_resources);
  KheSolnArenaEnd(soln, a);
  return true;
}


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
/*  FILE:         khe_sr_task_tree.c                                         */
/*  DESCRIPTION:  Task trees                                                 */
/*                                                                           */
/*****************************************************************************/
#include <limits.h>
#include "khe_solvers.h"

#define bool_show(x) ((x) ? "true" : "false")

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0
#define DEBUG9 0	/* limit busy times jobs */
#define DEBUG10 0

typedef HA_ARRAY(KHE_RESOURCE_TYPE) ARRAY_KHE_RESOURCE_TYPE;
typedef HA_ARRAY(KHE_CONSTRAINT) ARRAY_KHE_CONSTRAINT;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "constraint jobs"                                              */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingDoPreferResourcesConstraintJob(KHE_TASKING tasking,       */
/*    KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_TASK_BOUND_GROUP tbg,           */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Apply prefer resources constraint c to tasking.                          */
/*                                                                           */
/*****************************************************************************/

static void KheTaskingDoPreferResourcesConstraintJob(KHE_TASKING tasking,
  KHE_PREFER_RESOURCES_CONSTRAINT c, KHE_SOLN_ADJUSTER sa,
  /* KHE_TASK_BOUND_GROUP tbg, */ bool resource_invariant)
{
  KHE_TASK task;  KHE_RESOURCE_GROUP domain;  int i, j, init_count;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_MARK mark;  bool success;
  KHE_TASK_BOUND tb;
  domain = KhePreferResourcesConstraintDomain(c);  /* NB c != NULL */
  soln = KheTaskingSoln(tasking);
  /* t = KheTransactionMake(soln); */
  for( i = 0;  i < KhePreferResourcesConstraintEventResourceCount(c);  i++ )
  {
    er = KhePreferResourcesConstraintEventResource(c, i);
    for( j = 0;  j < KheEventResourceTaskCount(soln, er);  j++ )
    {
      task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, j));
      if( task != NULL && KheTaskTasking(task) == tasking &&
	  !KheResourceGroupDisjoint(KheTaskDomain(task), domain) )
      {
	KheAtomicOperationBegin(soln, &mark, &init_count, resource_invariant);
	/* success = KheTaskTightenDomain(task, domain, true); */
	/* success = KheTaskBoundMake(tbg, task, domain, &tb); */
	tb = KheSolnAdjusterTaskBoundMake(sa, soln, domain);
	/* tb = KheTaskBoundMake(soln, domain); */
	success = KheTaskAddTaskBound(task, tb);
	KheAtomicOperationEnd(soln, &mark, &init_count, resource_invariant,
	  success);
	/* *** already added to sa now
	if( success && tbg != NULL )
	  KheTaskBoundGroupAddTaskBound(tbg, tb);
	*** */
      }
    }
  }
  /* KheTransactionDelete(t); */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskIncreasingDomainSizeCmp(const void *t1, const void *t2)       */
/*                                                                           */
/*  Comparison function for sorting an array of tasks by increasing          */
/*  domain size.                                                             */
/*                                                                           */
/*****************************************************************************/

static int KheTaskIncreasingDomainSizeCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  KHE_RESOURCE_GROUP rg1 = KheTaskDomain(task1);
  KHE_RESOURCE_GROUP rg2 = KheTaskDomain(task2);
  if( KheResourceGroupResourceCount(rg1) != KheResourceGroupResourceCount(rg2) )
   return KheResourceGroupResourceCount(rg1)-KheResourceGroupResourceCount(rg2);
  else
    return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingDoAvoidSplitAssignmentsConstraintJob(KHE_TASKING tasking, */
/*    KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, KHE_TASK_BOUND_GROUP tbg,    */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Apply avoid split assignments constraint c to tasking.                   */
/*                                                                           */
/*  Implementation note.  After gathering the set of all tasks that need     */
/*  to be merged, the tasks are sorted into increasing domain size order.    */
/*  The point of doing this is that we need then only try merging each       */
/*  task into its preceding tasks, and only then if it is not assigned.      */
/*                                                                           */
/*****************************************************************************/

static void KheTaskingDoAvoidSplitAssignmentsConstraintJob(KHE_TASKING tasking,
  KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT c, /* KHE_TASK_BOUND_GROUP tbg, */
  KHE_SOLN_ADJUSTER sa, bool resource_invariant, HA_ARENA a)
{
  ARRAY_KHE_TASK tasks;  KHE_TASK task, task2;  int i, j, k, pos, init_count;
  KHE_EVENT_RESOURCE er;  KHE_RESOURCE_GROUP domain;  KHE_SOLN soln;
  KHE_MARK mark;  bool success;  KHE_TASK_BOUND tb;
  if( DEBUG8 )
    fprintf(stderr, "[ KheTaskingDoAvoidSplitAssignmentsConstraintJob()\n");

  HaArrayInit(tasks, a);
  soln = KheTaskingSoln(tasking);
  /* t = KheTransactionMake(soln); */
  for( i = 0;  i < KheAvoidSplitAssignmentsConstraintEventGroupCount(c);  i++ )
  {
    /* gather the distinct root tasks for event group i that lie in tasking */
    HaArrayClear(tasks);
    for( j=0; j<KheAvoidSplitAssignmentsConstraintEventResourceCount(c,i); j++ )
    {
      er = KheAvoidSplitAssignmentsConstraintEventResource(c, i, j);
      for( k = 0;  k < KheEventResourceTaskCount(soln, er);  k++ )
      {
	task = KheTaskFirstUnFixed(KheEventResourceTask(soln, er, k));
	if( task != NULL && KheTaskTasking(task) == tasking &&
	    !HaArrayContains(tasks, task, &pos) )
	  HaArrayAddLast(tasks, task);
      }
    }

    /* sort the tasks into increasing domain size order */
    HaArraySort(tasks, &KheTaskIncreasingDomainSizeCmp);
    if( DEBUG8 )
    {
      fprintf(stderr, "  trying to merge tasks:\n");
      HaArrayForEach(tasks, task, j)
	KheTaskDebug(task, 2, 4, stderr);
    }

    /* try to merge each unassigned task into a preceding task */
    HaArrayForEach(tasks, task, j)
      if( KheTaskAsst(task) == NULL )
      {
	/* look for a merge that involves no change to task's domain */
	KheAtomicOperationBegin(soln, &mark, &init_count, resource_invariant);
	success = false;
	for( k = j - 1;  !success && k >= 0;  k-- )
	{
          task2 = HaArray(tasks, k);
	  if( KheTaskAsst(task2) != NULL )
	    task2 = KheTaskAsst(task);
	  if( KheTaskAssign(task, task2) )
	  {
	    KheTaskAssignFix(task);
	    KheTaskingDeleteTask(tasking, task);
	    HaArrayDeleteAndShift(tasks, j);
	    j--;
	    success = true;
	  }
	}

	/* look for a merge that involves tightening task's domain */
	domain = KheTaskDomain(task);
	for( k = j - 1;  !success && k >= 0;  k-- )
	{
          task2 = HaArray(tasks, k);
	  if( KheTaskAsst(task2) == NULL &&
	      !KheResourceGroupDisjoint(domain, KheTaskDomain(task2)) )
	  {
	    /* reduce task's domain and assign task to task2 */
	    /* if( !KheTaskTightenDomain(task, KheTaskDomain(task2), true) ) */
	    /* ***
	    if( !KheTaskBoundMake(tbg, task, KheTaskDomain(task2), &tb) )
	      HnAbort("KheTaskingDoAvoidSplitAssignmentsConstraintJob"
		"internal error 1");
	    *** */
	    tb = KheSolnAdjusterTaskBoundMake(sa, soln, KheTaskDomain(task2));
	    /* tb = KheTaskBoundMake(soln, KheTaskDomain(task2)); */
	    /* *** already added to sa now
	    if( tbg != NULL )
	      KheTaskBoundGroupAddTaskBound(tbg, tb);
	    *** */
	    if( !KheTaskAddTaskBound(task, tb) )
	      HnAbort("KheTaskingDoAvoidSplitAssignmentsConstraintJob"
		"internal error 1");
	    if( !KheTaskAssign(task, task2) )
	      HnAbort("KheTaskingDoAvoidSplitAssignmentsConstraintJob"
		"internal error 2");
	    KheTaskAssignFix(task);
	    KheTaskingDeleteTask(tasking, task);
	    HaArrayDeleteAndShift(tasks, j);
	    j--;
	    success = true;
	  }
	}
	KheAtomicOperationEnd(soln, &mark, &init_count,
	  resource_invariant, success);
      }
  }
  /* KheTransactionDelete(t); */
  /* MArrayFree(tasks); */
  if( DEBUG8 )
    fprintf(stderr, "] KheTaskingDoAvoidSplitAssignmentsConstraintJob()\n");
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheLimitBusyTimesConstraintComplementResources(       */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                      */
/*                                                                           */
/*  Return a resource group containing the complement of the resources       */
/*  that c applies to.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheLimitBusyTimesConstraintComplementResources(
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;  int i;

  /* get a resource type and return if there are no resources */
  if( KheLimitBusyTimesConstraintResourceGroupCount(c) > 0 )
  {
    rg = KheLimitBusyTimesConstraintResourceGroup(c, 0);
    rt = KheResourceGroupResourceType(rg);
  }
  else if( KheLimitBusyTimesConstraintResourceCount(c) > 0 )
  {
    r = KheLimitBusyTimesConstraintResource(c, 0);
    rt = KheResourceResourceType(r);
  }
  else
    return false;

  /* build the complement of the union of the resource groups and resources */
  KheSolnResourceGroupBegin(soln, rt);
  KheSolnResourceGroupUnion(soln, KheResourceTypeFullResourceGroup(rt));
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheLimitBusyTimesConstraintResourceGroup(c, i);
    if( KheResourceGroupResourceType(rg) == rt )
      KheSolnResourceGroupDifference(soln, rg);
  }
  for( i = 0;  i < KheLimitBusyTimesConstraintResourceCount(c);  i++ )
  {
    r = KheLimitBusyTimesConstraintResource(c, i);
    if( KheResourceResourceType(r) == rt )
      KheSolnResourceGroupSubResource(soln, r);
  }
  return KheSolnResourceGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheClusterBusyTimesConstraintComplementResources(     */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)                    */
/*                                                                           */
/*  Return a resource group containing the complement of the resources       */
/*  that c applies to.                                                       */
/*                                                                           */
/*****************************************************************************/

static KHE_RESOURCE_GROUP KheClusterBusyTimesConstraintComplementResources(
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_SOLN soln)
{
  KHE_RESOURCE_GROUP rg;  KHE_RESOURCE_TYPE rt;  KHE_RESOURCE r;  int i;

  /* get a resource type and return if there are no resources */
  if( KheClusterBusyTimesConstraintResourceGroupCount(c) > 0 )
  {
    rg = KheClusterBusyTimesConstraintResourceGroup(c, 0);
    rt = KheResourceGroupResourceType(rg);
  }
  else if( KheClusterBusyTimesConstraintResourceCount(c) > 0 )
  {
    r = KheClusterBusyTimesConstraintResource(c, 0);
    rt = KheResourceResourceType(r);
  }
  else
    return false;

  /* build the complement of the union of the resource groups and resources */
  KheSolnResourceGroupBegin(soln, rt);
  KheSolnResourceGroupUnion(soln, KheResourceTypeFullResourceGroup(rt));
  for( i = 0;  i < KheClusterBusyTimesConstraintResourceGroupCount(c);  i++ )
  {
    rg = KheClusterBusyTimesConstraintResourceGroup(c, i);
    if( KheResourceGroupResourceType(rg) == rt )
      KheSolnResourceGroupDifference(soln, rg);
  }
  for( i = 0;  i < KheClusterBusyTimesConstraintResourceCount(c);  i++ )
  {
    r = KheClusterBusyTimesConstraintResource(c, i);
    if( KheResourceResourceType(r) == rt )
      KheSolnResourceGroupSubResource(soln, r);
  }
  return KheSolnResourceGroupEnd(soln);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheDoAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb,                 */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Add tb to task, returning true if successful.                            */
/*                                                                           */
/*****************************************************************************/

static bool KheDoAddTaskBound(KHE_TASK task, KHE_TASK_BOUND tb,
  bool resource_invariant)
{
  KHE_MARK mark;  int init_count;  bool success;  KHE_SOLN soln;
  soln = KheTaskSoln(task);
  KheAtomicOperationBegin(soln, &mark, &init_count, resource_invariant);
  success = KheTaskAddTaskBound(task, tb) &&
    KheResourceGroupResourceCount(KheTaskDomain(task)) > 1;
  KheAtomicOperationEnd(soln, &mark, &init_count, resource_invariant, success);
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingDoLimitBusyTimesConstraintJob(KHE_TASKING tasking,        */
/*    KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, KHE_TASK_BOUND_GROUP tbg,           */
/*    bool resource_invariant, HA_ARENA a)                                   */
/*                                                                           */
/*  Apply limit busy times constraint c to tasking.  The constraint is       */
/*  already known to have non-zero weight and maximum limit 0.               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskingDoLimitBusyTimesConstraintJob(KHE_TASKING tasking,
  KHE_LIMIT_BUSY_TIMES_CONSTRAINT c, /* KHE_TASK_BOUND_GROUP tbg, */
  KHE_SOLN_ADJUSTER sa, bool resource_invariant, HA_ARENA a)
{
  KHE_TASK task;  KHE_RESOURCE_GROUP domain;  int i, j, k, m, n, p, offset;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TIME t;  bool success;
  KHE_TASK_BOUND tb;  KHE_EVENT e;  KHE_RESOURCE_TYPE rt;  KHE_TIME_GROUP tg;
  if( DEBUG9 )
    fprintf(stderr, "[ KheTaskingDoLimitBusyTimesConstraintJob(-, %s, -, %s)\n",
      KheConstraintId((KHE_CONSTRAINT) c), resource_invariant ? "true":"false");
  soln = KheTaskingSoln(tasking);
  domain = KheLimitBusyTimesConstraintComplementResources(c, soln);
  if( domain != NULL )
  {
    if( DEBUG9 )
      KheResourceGroupDebug(domain, 2, 2, stderr);
    rt = KheResourceGroupResourceType(domain);
    /* tb = KheTaskBoundMake(soln, domain); */
    tb = KheSolnAdjusterTaskBoundMake(sa, soln, domain);
    /* ***
    if( tbg != NULL )
      KheTaskBoundGroupAddTaskBound(tbg, tb);
    *** */
    for( i = 0;  i < KheLimitBusyTimesConstraintAppliesToOffsetCount(c);  i++ )
    {
      offset = KheLimitBusyTimesConstraintAppliesToOffset(c, i);
      for( j = 0;  j < KheLimitBusyTimesConstraintTimeGroupCount(c);  j++ )
      {
	tg = KheLimitBusyTimesConstraintTimeGroup(c, j, offset);
	for( k = 0;  k < KheTimeGroupTimeCount(tg);  k++ )
	{
	  t = KheTimeGroupTime(tg, k);
	  for( m = 0;  m < KheTimePreassignedEventCount(t);  m++ )
	  {
	    e = KheTimePreassignedEvent(t, m);
	    for( n = 0;  n < KheEventResourceCount(e);  n++ )
	    {
	      er = KheEventResource(e, n);
	      if( KheEventResourceResourceType(er) == rt )
		for( p = 0;  p < KheEventResourceTaskCount(soln, er);  p++ )
		{
                  task = KheEventResourceTask(soln, er, p);
		  task = KheTaskProperRoot(task);
                  success = KheDoAddTaskBound(task, tb, resource_invariant);
		  if( DEBUG9 )
		  {
		    fprintf(stderr, "  at time %s, constraining task ",
		      KheTimeId(t));
		    KheTaskDebug(task, 2, -1, stderr);
		    if( !success )
		      fprintf(stderr, " (unsuccessful)");
		    fprintf(stderr, "\n");
		  }
		}
	    }
	  }
	}
      }
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheTaskingDoLimitBusyTimesConstraintJob returning\n");
}

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingDoClusterBusyTimesConstraintJob(KHE_TASKING tasking,      */
/*    KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, KHE_TASK_BOUND_GROUP tbg,         */
/*    bool resource_invariant, HA_ARENA a)                                   */
/*                                                                           */
/*  Apply cluster busy times constraint c to tasking.  The constraint is     */
/*  already known to have non-zero weight and maximum limit 0.  This         */
/*  means that we can treat its positive time groups like we treat the       */
/*  time groups of a limit busy times constraint.                            */
/*                                                                           */
/*****************************************************************************/

static void KheTaskingDoClusterBusyTimesConstraintJob(KHE_TASKING tasking,
  KHE_CLUSTER_BUSY_TIMES_CONSTRAINT c, /* KHE_TASK_BOUND_GROUP tbg, */
  KHE_SOLN_ADJUSTER sa, bool resource_invariant, HA_ARENA a)
{
  KHE_TASK task;  KHE_RESOURCE_GROUP domain;  int i, j, k, m, n, p, offset;
  KHE_EVENT_RESOURCE er;  KHE_SOLN soln;  KHE_TIME t;  bool success;
  KHE_TASK_BOUND tb;  KHE_EVENT e;  KHE_RESOURCE_TYPE rt;  KHE_TIME_GROUP tg;
  KHE_POLARITY po;
  if( DEBUG9 )
    fprintf(stderr, "[ KheTaskingDoClusterBusyTimesConstraintJob(%s, -, %s)\n",
      KheConstraintId((KHE_CONSTRAINT) c), resource_invariant ? "true":"false");
  soln = KheTaskingSoln(tasking);
  domain = KheClusterBusyTimesConstraintComplementResources(c, soln);
  if( domain != NULL )
  {
    if( DEBUG9 )
      KheResourceGroupDebug(domain, 2, 2, stderr);
    rt = KheResourceGroupResourceType(domain);
    tb = KheSolnAdjusterTaskBoundMake(sa, soln, domain);
    /* ***
    tb = KheTaskBoundMake(soln, domain);
    if( tbg != NULL )
      KheTaskBoundGroupAddTaskBound(tbg, tb);
    *** */
    for( i = 0;  i < KheClusterBusyTimesConstraintAppliesToOffsetCount(c); i++ )
    {
      offset = KheClusterBusyTimesConstraintAppliesToOffset(c, i);
      for( j = 0;  j < KheClusterBusyTimesConstraintTimeGroupCount(c);  j++ )
      {
	tg = KheClusterBusyTimesConstraintTimeGroup(c, j, offset, &po);
	if( po == KHE_POSITIVE )
	{
	  for( k = 0;  k < KheTimeGroupTimeCount(tg);  k++ )
	  {
	    t = KheTimeGroupTime(tg, k);
	    for( m = 0;  m < KheTimePreassignedEventCount(t);  m++ )
	    {
	      e = KheTimePreassignedEvent(t, m);
	      for( n = 0;  n < KheEventResourceCount(e);  n++ )
	      {
		er = KheEventResource(e, n);
		if( KheEventResourceResourceType(er) == rt )
		  for( p = 0;  p < KheEventResourceTaskCount(soln, er);  p++ )
		  {
		    task = KheEventResourceTask(soln, er, p);
		    task = KheTaskProperRoot(task);
		    success = KheDoAddTaskBound(task, tb, resource_invariant);
		    if( DEBUG9 )
		    {
		      fprintf(stderr, "  cluster at %s, constraining task ",
			KheTimeId(t));
		      KheTaskDebug(task, 2, -1, stderr);
		      if( !success )
			fprintf(stderr, " (unsuccessful)");
		      fprintf(stderr, "\n");
		    }
		  }
	      }
	    }
	  }
	}
      }
    }
  }
  if( DEBUG9 )
    fprintf(stderr, "] KheTaskingDoClusterBusyTimesConstraintJob returning\n");
}


/*****************************************************************************/
/*                                                                           */
/*  int KheConstraintDecreasingCombinedWeightCmp(const void *t1,             */
/*    const void *t2)                                                        */
/*                                                                           */
/*  Comparison function for sorting an array of constraints by decreasing    */
/*  combined weight.                                                         */
/*                                                                           */
/*****************************************************************************/

static int KheConstraintDecreasingCombinedWeightCmp(const void *t1,
  const void *t2)
{
  KHE_CONSTRAINT c1 = * (KHE_CONSTRAINT *) t1;
  KHE_CONSTRAINT c2 = * (KHE_CONSTRAINT *) t2;
  return KheCostCmp(KheConstraintCombinedWeight(c2),
    KheConstraintCombinedWeight(c1));
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskingDoConstraintJobs(KHE_TASKING tasking,                     */
/*    bool prefer_hard, bool prefer_soft, bool split_hard, bool split_soft,  */
/*    bool limit_busy_hard, bool limit_busy_soft, KHE_TASK_BOUND_GROUP tbg,  */
/*    bool resource_invariant, HA_ARENA a)                                   */
/*                                                                           */
/*  Apply to the tasks of tasking the constraint jobs requested.             */
/*  Any time bounds created are to be added to tbg, if non-NULL.             */
/*                                                                           */
/*****************************************************************************/

/* *** now inlined at its call point
static void KheTaskingDoConstraintJobs(KHE_TASKING tasking,
  bool prefer_hard, bool prefer_soft, bool split_hard, bool split_soft,
  bool limit_busy_hard, bool limit_busy_soft, KHE_TASK_BOUND_GROUP tbg,
  bool resource_invariant, HA_ARENA a)
{
  KHE_INSTANCE ins;  ARRAY_KHE_CONSTRAINT constraints;  KHE_CONSTRAINT c;
  KHE_RESOURCE_TYPE rt;  KHE_PREFER_RESOURCES_CONSTRAINT prc;  int i;
  KHE_RESOURCE_GROUP domain;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;

  ** gather the constraints required **
  ins = KheSolnInstance(KheTaskingSoln(tasking));
  rt = KheTaskingResourceType(tasking);
  HaArrayInit(constraints, a);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstr aintCombinedWeight(c) > 0 ) switch( KheConstraintTag(c) )
    {
      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	prc = (KHE_PREFER_RESOURCES_CONSTRAINT) c;
	domain = KhePreferResourc esConstraintDomain(prc);
	if( (rt == NULL || rt == KheResourceGroupResourceType(domain)) &&
	    (KheConstraintRequired(c) ? prefer_hard : prefer_soft) )
	  HaArrayAddLast(constraints, c);
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	if( KheConstraintRequired(c) ? split_hard : split_soft )
	  HaArrayAddLast(constraints, c);
	break;

      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

	if( KheConstraintRequired(c) ? limit_busy_hard : limit_busy_soft )
	{
          lbtc = (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
	  if( KheLimitBusyTimesConstraintMaximum(lbtc) == 0 )
	    HaArrayAddLast(constraints, c);
	}
	break;

      default:

	** constraint of other types are not relevant **
	break;
    }
  }

  ** handle each constraint in turn, in decreasing combined weight order **
  HaArraySort(constraints, &KheConstraintDecreasingCombinedWeightCmp);
  HaArrayForEach(constraints, c, i)
    switch( KheConstraintTag(c) )
    {
      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

        KheTaskingDoPreferResourcesConstraintJob(tasking,
          (KHE_PREFER_RESOURCES_CONSTRAINT) c, tbg, resource_invariant);
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

        KheTaskingDoAvoidSplitAssignmentsConstraintJob(tasking,
          (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, tbg,resource_invariant,a);
	break;

      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

        KheTaskingDoLimitBusyTimesConstraintJob(tasking,
          (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, tbg, resource_invariant, a);
	break;

      default:

	HnAbort("KheTaskingDoConstraintJobs internal error");
	break;
    }

  ** clean up and quit **
  ** MArrayFree(constraints); **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task tree construction"                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceTypeCmp(const void *t1, const void *t2)                   */
/*                                                                           */
/*  Comparison function for sorting an array of resource types.  Resource    */
/*  types whose event resources are all preassigned come first, followed     */
/*  by the others in decreasing order of number of points of application     */
/*  of avoid split assignments constraints.                                  */
/*                                                                           */
/*****************************************************************************/

static int KheResourceTypeCmp(const void *t1, const void *t2)
{
  KHE_RESOURCE_TYPE rt1 = * (KHE_RESOURCE_TYPE *) t1;
  KHE_RESOURCE_TYPE rt2 = * (KHE_RESOURCE_TYPE *) t2;
  bool all_preassigned1 = KheResourceTypeDemandIsAllPreassigned(rt1);
  bool all_preassigned2 = KheResourceTypeDemandIsAllPreassigned(rt2);
  int asa_count1 = KheResourceTypeAvoidSplitAssignmentsCount(rt1);
  int asa_count2 = KheResourceTypeAvoidSplitAssignmentsCount(rt2);
  if( all_preassigned1 != all_preassigned2 )
    return all_preassigned2 - all_preassigned1;
  else if( asa_count1 != asa_count2 )
    return asa_count2 - asa_count1;
  else
    return KheResourceTypeIndex(rt1) - KheResourceTypeIndex(rt2);
}


/*****************************************************************************/
/*                                                                           */
/*  void CheckResourceTypes(ARRAY_KHE_RESOURCE_TYPE *resource_types)         */
/*                                                                           */
/*  For tracking down a bug.                                                 */
/*                                                                           */
/*****************************************************************************/

/* *** bug fixed now
static void CheckResourceTypes(ARRAY_KHE_RESOURCE_TYPE *resource_types,
  char *mess, int i)
{
  KHE_RESOURCE_TYPE rt;  int j;
  HaArrayForEach(*resource_types, rt, j)
    HnAssert(rt != NULL, "KheTaskTreeMake internal error %s(i %d, j %d)",
      mess, i, j);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskTreeMake(KHE_SOLN soln, KHE_TASK_JOB_TYPE tjt,               */
/*    KHE_OPTIONS options)                                                   */
/*                                                                           */
/*  Make a task tree from the tasks of soln.                                 */
/*                                                                           */
/*****************************************************************************/

bool KheTaskTreeMake(KHE_SOLN soln, KHE_OPTIONS options)
{
  ARRAY_KHE_RESOURCE_TYPE resource_types;  KHE_RESOURCE_TYPE rt;
  KHE_INSTANCE ins;  int i;  KHE_TASKING tasking;  bool res;  HA_ARENA a;

  /* get the resource types of soln's instance, in the required order */
  ins = KheSolnInstance(soln);
  if( DEBUG10 )
    fprintf(stderr, "[ KheTaskTreeMake(%s %d, options)\n",
      KheInstanceId(ins), KheSolnDiversifier(soln));
  a = KheSolnArenaBegin(soln);
  /* a = HaAre naMake(); */
  HaArrayInit(resource_types, a);
  for( i = 0;  i < KheInstanceResourceTypeCount(ins);  i++ )
  {
    rt = KheInstanceResourceType(ins, i);
    HnAssert(rt != NULL, "KheTaskTreeMake internal error (i == %d)", i);
    HaArrayAddLast(resource_types, rt);
  }
  HaArraySort(resource_types, &KheResourceTypeCmp);

  /* build one tasking for each type, and apply KheTaskingMakeTaskTree to it */
  HnAssert(KheSolnTaskingCount(soln) == 0, "KheTaskTreeMake: soln has tasking");
  res = false;
  HaArrayForEach(resource_types, rt, i)
  {
    tasking = KheTaskingMakeFromResourceType(soln, rt);
    res = true;
    KheTaskingMakeTaskTree(tasking, NULL, options);
  }

  /* clean up and exit */
  KheSolnArenaEnd(soln, a);
  /* HaArena Delete(a); */
  /* MArrayFree(resource_types); */
  if( DEBUG10 )
    fprintf(stderr, "] KheTaskTreeMake returning %s\n", bool_show(res));
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKING KheTaskingMakeFromResourceType(KHE_SOLN soln,                */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Make a tasking holding the leader tasks of soln of resource type rt.     */
/*                                                                           */
/*****************************************************************************/

KHE_TASKING KheTaskingMakeFromResourceType(KHE_SOLN soln, KHE_RESOURCE_TYPE rt)
{
  int i;  KHE_TASK task;  KHE_TASKING res;
  res = KheTaskingMake(soln, rt);
  for( i = 0;  i < KheSolnTaskCount(soln);  i++ )
  {
    task = KheSolnTask(soln, i);
    if( !KheTaskAssignIsFixed(task) &&
	(rt == NULL || KheTaskResourceType(task) == rt) )
      KheTaskingAddTask(res, task);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingMakeTaskTree(KHE_TASKING tasking, KHE_TASK_JOB_TYPE tjt,  */
/*    KHE_TASK_BOUND_GROUP tbg, KHE_OPTIONS options)                         */
/*                                                                           */
/*  Make a task tree from the tasks of tasking.                              */
/*                                                                           */
/*****************************************************************************/

bool KheTaskingMakeTaskTree(KHE_TASKING tasking, KHE_SOLN_ADJUSTER sa,
  /* KHE_TASK_BOUND_GROUP tbg, */ KHE_OPTIONS options)
{
  int i;  KHE_TASK task;  KHE_RESOURCE r;  bool resource_invariant;
  bool prefer_hard, prefer_soft, split_hard, split_soft;  HA_ARENA a;
  bool limit_busy_hard, limit_busy_soft;  KHE_SOLN soln;
  KHE_INSTANCE ins;  ARRAY_KHE_CONSTRAINT constraints;  KHE_CONSTRAINT c;
  KHE_RESOURCE_TYPE rt;  KHE_PREFER_RESOURCES_CONSTRAINT prc;
  KHE_RESOURCE_GROUP domain;  KHE_LIMIT_BUSY_TIMES_CONSTRAINT lbtc;
   KHE_CLUSTER_BUSY_TIMES_CONSTRAINT cbtc;
  /* KHE_COST min_weight, weight; */

  if( DEBUG3 )
    fprintf(stderr, "[ KheTaskingMakeTaskTree(%s, tbg, options)\n",
      KheTaskingResourceType(tasking) == NULL ? "~" :
      KheResourceTypeId(KheTaskingResourceType(tasking)) == NULL ? "-" :
      KheResourceTypeId(KheTaskingResourceType(tasking)));

  /* jobs to install existing domains and assignments */
  /* nothing to do here */

  /* job to assign preassigned tasks (some may fail, ignore that) */
  for( i = 0;  i < KheTaskingTaskCount(tasking);  i++ )
  {
    task = KheTaskingTask(tasking, i);
    if( KheTaskIsPreassigned(task, &r) && KheTaskAsst(task) == NULL )
      KheTaskAssignResource(task, r);
  }

  /* jobs for various constraints */
  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  prefer_hard = !KheOptionsGetBool(options,
    "rs_task_tree_prefer_hard_off", false);
  prefer_soft = /* ! deliberately omitted */ KheOptionsGetBool(options,
    "rs_task_tree_prefer_soft", false);
  split_hard = !KheOptionsGetBool(options,
    "rs_task_tree_split_hard_off", false);
  split_soft = !KheOptionsGetBool(options,
    "rs_task_tree_split_soft_off", false);
  limit_busy_hard = !KheOptionsGetBool(options,
    "rs_task_tree_limit_busy_hard_off", false);
  limit_busy_soft = !KheOptionsGetBool(options,
    "rs_task_tree_split_limit_busy_soft_off", false);
  soln = KheTaskingSoln(tasking);
  a = KheSolnArenaBegin(soln);
  /* a = HaAren aMake(); */

  /* find the minimum weight of all constraints */
  ins = KheSolnInstance(KheTaskingSoln(tasking));
  /* ***
  min_weight = KheCost(INT_MAX, INT_MAX);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    weight = KheConstr aintCombinedWeight(c);
    if( weight > 0 && min_weight < weight )
      min_weight = weight;
  }
  *** */

  /* gather all constraints whose weight is greater than the minimum */
  rt = KheTaskingResourceType(tasking);
  HaArrayInit(constraints, a);
  for( i = 0;  i < KheInstanceConstraintCount(ins);  i++ )
  {
    c = KheInstanceConstraint(ins, i);
    if( KheConstraintCombinedWeight(c) > 0 )
      switch( KheConstraintTag(c) )
      {
	case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

	  prc = (KHE_PREFER_RESOURCES_CONSTRAINT) c;
	  domain = KhePreferResourcesConstraintDomain(prc);  /* NB c != NULL */
	  if( (rt == NULL || rt == KheResourceGroupResourceType(domain)) &&
	      (KheConstraintRequired(c) ? prefer_hard : prefer_soft) )
	    HaArrayAddLast(constraints, c);
	  break;

	case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

	  if( KheConstraintRequired(c) ? split_hard : split_soft )
	    HaArrayAddLast(constraints, c);
	  break;

	case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

	  if( KheConstraintRequired(c) ? limit_busy_hard : limit_busy_soft )
	  {
	    lbtc = (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c;
	    if( KheLimitBusyTimesConstraintMaximum(lbtc) == 0 )
	      HaArrayAddLast(constraints, c);
	  }
	  break;

	case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

	  if( KheConstraintRequired(c) ? limit_busy_hard : limit_busy_soft )
	  {
	    cbtc = (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c;
	    if( KheClusterBusyTimesConstraintMaximum(cbtc) == 0 )
	      HaArrayAddLast(constraints, c);
	  }
	  break;

	default:

	  /* constraint of other types are not relevant */
	  break;
      }
  }

  /* handle each constraint in turn, in decreasing combined weight order */
  HaArraySort(constraints, &KheConstraintDecreasingCombinedWeightCmp);
  HaArrayForEach(constraints, c, i)
    switch( KheConstraintTag(c) )
    {
      case KHE_PREFER_RESOURCES_CONSTRAINT_TAG:

        KheTaskingDoPreferResourcesConstraintJob(tasking,
          (KHE_PREFER_RESOURCES_CONSTRAINT) c, sa, /*tbg,*/ resource_invariant);
	break;

      case KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT_TAG:

        KheTaskingDoAvoidSplitAssignmentsConstraintJob(tasking,
          (KHE_AVOID_SPLIT_ASSIGNMENTS_CONSTRAINT) c, sa, /* tbg, */
	  resource_invariant, a);
	break;

      case KHE_LIMIT_BUSY_TIMES_CONSTRAINT_TAG:

        KheTaskingDoLimitBusyTimesConstraintJob(tasking,
          (KHE_LIMIT_BUSY_TIMES_CONSTRAINT) c, sa, /* tbg, */
	  resource_invariant, a);
	break;

      case KHE_CLUSTER_BUSY_TIMES_CONSTRAINT_TAG:

        KheTaskingDoClusterBusyTimesConstraintJob(tasking,
          (KHE_CLUSTER_BUSY_TIMES_CONSTRAINT) c, sa, /* tbg, */
	  resource_invariant, a);
	break;

      default:

	HnAbort("KheTaskingDoConstraintJobs internal error");
	break;
    }

  /* ***
  KheTaskingDoConstraintJobs(tasking, prefer_hard, prefer_soft, split_hard,
    split_soft, limit_busy_hard, limit_busy_soft, tbg, resource_invariant, a);
  *** */

  /* HaArena Delete(a); */

  /* partition jobs */
  /* ***
  if( tjt & KHE_TASK_JOB_PARTITION )
    KheDoPartitionJob(tasking, tbg, resource_invariant);
  *** */

  /* fix domains */
  /* ***
  for( i = 0;  i < KheTaskingTaskCount(tasking);  i++ )
  {
    task = KheTaskingTask(tasking, i);
    KheTaskDomainFix(task);
  }
  *** */

  /* check prefer resources monitors */
  /* ***
  if( check_prefer_resources_monitors )
    KheTaskingMonitorAttach Check(tasking, KHE_PREFER_RESOURCES_MONITOR_TAG,
      false);
  *** */

  /* check avoid split assignments monitors */
  /* ***
  if( check_avoid_split_assignments_monitors )
    KheTaskingMonitorAttach Check(tasking,
      KHE_AVOID_SPLIT_ASSIGNMENTS_MONITOR_TAG, false);
  *** */

  KheSolnArenaEnd(soln, a);
  if( DEBUG3 )
    fprintf(stderr, "] KheTaskingMakeTaskTree returning\n");
  return true;
}

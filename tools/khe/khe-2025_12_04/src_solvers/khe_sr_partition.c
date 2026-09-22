
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
/*  FILE:         khe_sr_partition.c                                         */
/*  DESCRIPTION:  Tighten to partition                                       */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define DEBUG5 0
#define DEBUG6 0
#define DEBUG7 0
#define DEBUG8 0

typedef HA_ARRAY(KHE_RESOURCE_TYPE) ARRAY_KHE_RESOURCE_TYPE;
typedef HA_ARRAY(KHE_CONSTRAINT) ARRAY_KHE_CONSTRAINT;
typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION - one resource partition                                   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_partition_rec {
  KHE_RESOURCE_GROUP		resource_group;		/* the resource grp  */
  int				duration_supply;	/* duration supply   */
  int				duration_demand;	/* duration demand   */
} *KHE_PARTITION;

typedef HA_ARRAY(KHE_PARTITION) ARRAY_KHE_PARTITION;


/*****************************************************************************/
/*                                                                           */
/*  KHE_MULTI_TASK - task assignable resources from more than one partition  */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_multi_task_rec {
  KHE_TASK			task;			/* the task          */
  ARRAY_KHE_PARTITION		partitions;		/* its partitions    */
  KHE_PARTITION			dominant_partition;	/* dominates         */
} *KHE_MULTI_TASK;

typedef HA_ARRAY(KHE_MULTI_TASK) ARRAY_KHE_MULTI_TASK;


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION_SOLVER - a partition solver.                               */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_partition_solver_rec {
  HA_ARENA			arena;			/* arena             */
  KHE_SOLN			soln;			/* solution          */
  KHE_RESOURCE_TYPE		resource_type;		/* resource type     */
  /* KHE_TASKING		tasking; */		/* the tasking       */
  bool				resource_invariant;	/* flag from caller  */
  ARRAY_KHE_PARTITION		partitions;		/* the partitions    */
  ARRAY_KHE_MULTI_TASK		multi_tasks;		/* multi-partn tasks */
} *KHE_PARTITION_SOLVER;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "partitions" (private)                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupDurationSupply(KHE_RESOURCE_GROUP rg, KHE_SOLN soln) */
/*                                                                           */
/*  Return the duration supply of rg.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheResourceGroupDurationSupply(KHE_RESOURCE_GROUP rg, KHE_SOLN soln)
{
  int i, j, cycle_length, r_supply, res;  KHE_RESOURCE r;  KHE_MONITOR m;
  if( DEBUG2 )
  {
    fprintf(stderr, "[ KheResourceGroupDurationSupply(");
    KheResourceGroupDebug(rg, 1, -1, stderr);
    fprintf(stderr, ", soln)\n");
  }

  cycle_length = KheInstanceTimeCount(KheSolnInstance(soln));
  if( DEBUG2 )
    fprintf(stderr, "  cycle_length %d\n", cycle_length);
  res = 0;
  for( i = 0;  i < KheResourceGroupResourceCount(rg);  i++ )
  {
    r = KheResourceGroupResource(rg, i);
    r_supply = cycle_length;
    for( j = 0;  j < KheSolnResourceMonitorCount(soln, r);  j++ )
    {
      m = KheSolnResourceMonitor(soln, r, j);
      if( KheMonitorTag(m) == KHE_WORKLOAD_DEMAND_MONITOR_TAG )
	r_supply--;
    }
    if( DEBUG2 )
      fprintf(stderr, "  supply %2d from %s\n", r_supply,
	KheResourceId(r) == NULL ? "-" : KheResourceId(r));
    res += r_supply;
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheResourceGroupDurationSupply returning %d\n", res);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION KhePartitionMake(KHE_RESOURCE_GROUP rg, KHE_SOLN soln)     */
/*                                                                           */
/*  Make a partition object for this resource group, initially with the      */
/*  correct duration supply, but with 0.0 for the duration demand.           */
/*                                                                           */
/*****************************************************************************/

static KHE_PARTITION KhePartitionMake(KHE_RESOURCE_GROUP rg, KHE_SOLN soln,
  HA_ARENA a)
{
  KHE_PARTITION res;
  HaMake(res, a);
  res->resource_group = rg;
  res->duration_supply = KheResourceGroupDurationSupply(rg, soln);
  res->duration_demand = 0;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  float KhePartitionAverageAvailableDuration(KHE_PARTITION p)              */
/*                                                                           */
/*  Return the average_available duration of p.                              */
/*                                                                           */
/*****************************************************************************/

static float KhePartitionAverageAvailableDuration(KHE_PARTITION p)
{
  int num = KheResourceGroupResourceCount(p->resource_group);
  return num == 0 ? 0.0 :
    (float) (p->duration_supply - p->duration_demand) / num;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDecreasingAvailCmp(const void *t1, const void *t2)      */
/*                                                                           */
/*  Comparison function for sorting an array of partition objects by         */
/*  decreasing average availability.                                         */
/*                                                                           */
/*****************************************************************************/

static int KhePartitionDecreasingAvailCmp(const void *t1, const void *t2)
{
  KHE_PARTITION p1 = * (KHE_PARTITION *) t1;
  KHE_PARTITION p2 = * (KHE_PARTITION *) t2;
  int count1 = KheResourceGroupResourceCount(p1->resource_group);
  int count2 = KheResourceGroupResourceCount(p2->resource_group);
  float cmp;
  cmp = KhePartitionAverageAvailableDuration(p2) -
    KhePartitionAverageAvailableDuration(p1);
  if( cmp < 0 )
    return -1;
  else if( cmp > 0 )
    return 1;
  else
    return count1 - count2;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDelete(KHE_PARTITION p)                                 */
/*                                                                           */
/*  Delete p.                                                                */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePartitionDelete(KHE_PARTITION p)
{
  MFree(p);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionAcceptsTask(KHE_PARTITION p, KHE_TASK task,             */
/*    KHE_TASK_BOUND_GROUP tbg, bool resource_invariant)                     */
/*                                                                           */
/*  If p accepts task (and, if resource_invariant is true, does so without   */
/*  increasing the number of unmatchable demand nodes), then reduce the      */
/*  domain of task to p and return true, otherwise change nothing and        */
/*  return false.                                                            */
/*                                                                           */
/*****************************************************************************/

static bool KhePartitionAcceptsTask(KHE_PARTITION p, KHE_TASK task,
  /* KHE_TASK_BOUND_GROUP tbg, */ KHE_SOLN_ADJUSTER sa, bool resource_invariant)
{
  int init_count;  bool success;  KHE_MARK mark;  KHE_SOLN soln;
  KHE_TASK_BOUND tb;
  if( DEBUG6 )
    fprintf(stderr, "[ KhePartitionAcceptsTask(p, task, %s, t)\n",
      resource_invariant ? "true" : "false");
  soln = KheTaskSoln(task);
  KheAtomicOperationBegin(soln, &mark, &init_count, resource_invariant);
  /* success = KheTaskBoundMake(tbg, task, p->resource_group, &tb); */
  tb = KheSolnAdjusterTaskBoundMake(sa, soln, p->resource_group);
  /* tb = KheTaskBoundMake(soln, p->resource_group); */
  success = KheTaskAddTaskBound(task, tb);
  success = KheAtomicOperationEnd(soln, &mark, &init_count,
    resource_invariant, success);
  /* *** already added to sa now
  if( success && tbg != NULL )
    KheTaskBoundGroupAddTaskBound(tbg, tb);
  *** */
  if( DEBUG6 )
    fprintf(stderr, "] KhePartitionAcceptsTask returning %s\n",
      success ? "true" : "false");
  return success;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionDebug(KHE_PARTITION p, int verbosity,                   */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of p onto fp.                                                */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionDebug(KHE_PARTITION p, int verbosity,
  int indent, FILE *fp)
{
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Partition %5.2f ", KhePartitionAverageAvailableDuration(p));
    KheResourceGroupDebug(p->resource_group, 1, -1, fp);
    fprintf(fp, " (supply %d, demand %d, resources %d) ]",
      p->duration_supply, p->duration_demand,
      KheResourceGroupResourceCount(p->resource_group));
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "multi-tasks" (private)                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_MULTI_TASK KheMultiTaskMake(KHE_TASK task)                           */
/*                                                                           */
/*  Make a multi-task, initially with no partitions.                         */
/*                                                                           */
/*****************************************************************************/

static KHE_MULTI_TASK KheMultiTaskMake(KHE_TASK task, HA_ARENA a)
{
  KHE_MULTI_TASK res;
  HaMake(res, a);
  res->task = task;
  HaArrayInit(res->partitions, a);
  res->dominant_partition = NULL;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultiTaskDelete(KHE_MULTI_TASK mt)                               */
/*                                                                           */
/*  Delete mt, freeing its memory.                                           */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheMultiTaskDelete(KHE_MULTI_TASK mt)
{
  MArrayFree(mt->partitions);
  MFree(mt);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheMultiTaskAddPartition(KHE_MULTI_TASK mt, KHE_PARTITION p)        */
/*                                                                           */
/*  Add p to mt, and if mt does not yet have a dominant partition, see       */
/*  whether p is it.                                                         */
/*                                                                           */
/*****************************************************************************/

static void KheMultiTaskAddPartition(KHE_MULTI_TASK mt, KHE_PARTITION p)
{
  int i, count;  KHE_RESOURCE r;  KHE_RESOURCE_GROUP rg;
  HaArrayAddLast(mt->partitions, p);
  if( mt->dominant_partition == NULL )
  {
    count = 0;
    rg = KheTaskDomain(mt->task);
    for( i = 0;  i < KheResourceGroupResourceCount(p->resource_group);  i++ )
    {
      r = KheResourceGroupResource(p->resource_group, i);
      if( KheResourceGroupContains(rg, r) )
	count++;
    }
    if( 4 * count >= 3 * KheResourceGroupResourceCount(rg) )
      mt->dominant_partition = p;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultiTaskCmp(const void *t1, const void *t2)                     */
/*                                                                           */
/*  Comparison function for sorting an array of multi-tasks into decreasing  */
/*  total workload order, except tasks with dominant partitions come first.  */
/*                                                                           */
/*****************************************************************************/

static int KheMultiTaskCmp(const void *t1, const void *t2)
{
  KHE_MULTI_TASK mt1 = * (KHE_MULTI_TASK *) t1;
  KHE_MULTI_TASK mt2 = * (KHE_MULTI_TASK *) t2;
  float workload1 = KheTaskTotalWorkload(mt1->task);
  float workload2 = KheTaskTotalWorkload(mt2->task);
  if( (mt1->dominant_partition != NULL) != (mt2->dominant_partition != NULL) )
    return (mt2->dominant_partition!=NULL) - (mt1->dominant_partition!=NULL);
  else if( workload1 < workload2 )
    return 1;
  else if( workload1 > workload2 )
    return -1;
  else
    return KheTaskSolnIndex(mt1->task) - KheTaskSolnIndex(mt2->task);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheMultiTaskDebug(KHE_MULTI_TASK mt, int verbosity,                 */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of mt onto fp with the given verbosity and indent.           */
/*                                                                           */
/*****************************************************************************/

static void KheMultiTaskDebug(KHE_MULTI_TASK mt, int verbosity,
  int indent, FILE *fp)
{
  KHE_PARTITION p;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ MultiTask ");
    KheTaskDebug(mt->task, 1, -1, fp);
    fprintf(fp, " (%d partitions", HaArrayCount(mt->partitions));
    if( mt->dominant_partition != NULL )
    {
      fprintf(fp, ", dominated by ");
      KheResourceGroupDebug(mt->dominant_partition->resource_group, 1, -1, fp);
    }
    fprintf(fp, ")");
    if( verbosity >= 2 )
    {
      fprintf(fp, "\n%*s  ", indent, "");
      HaArrayForEach(mt->partitions, p, i)
      {
	if( i > 0 )
	  fprintf(fp, ", ");
	KheResourceGroupDebug(p->resource_group, 1, -1, fp);
      }
      fprintf(fp, "\n%*s]", indent, "");
    }
    else
      fprintf(fp, " ]");
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "partition solvers" (private)                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  bool KhePartitionSolverRetrieve(KHE_PARTITION_SOLVER ps,                 */
/*    KHE_RESOURCE_GROUP rg, KHE_PARTITION *p)                               */
/*                                                                           */
/*  If ps contains a partition for resource group rg, set *p to that         */
/*  partition and return true.  Otherwise return false.                      */
/*                                                                           */
/*****************************************************************************/

static bool KhePartitionSolverRetrieve(KHE_PARTITION_SOLVER ps,
  KHE_RESOURCE_GROUP rg, KHE_PARTITION *p)
{
  int i;
  HaArrayForEach(ps->partitions, *p, i)
    if( (*p)->resource_group == rg )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_PARTITION_SOLVER KhePartitionSolverMake(KHE_TASKING tasking,         */
/*    bool resource_invariant)                                               */
/*                                                                           */
/*  Make a partition solver for with these attributes.  The tasking must     */
/*  have a resource type.                                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_PARTITION_SOLVER KhePartitionSolverMake(KHE_SOLN soln,
  KHE_RESOURCE_TYPE rt, bool resource_invariant, HA_ARENA a)
{
  int i;  KHE_RESOURCE_GROUP rg;  KHE_PARTITION_SOLVER res;

  /* do the basic initialization */
  HnAssert(rt != NULL, "KhePartitionSolverMake internal error");
  HaMake(res, a);
  res->arena = a;
  res->soln = soln;
  res->resource_type = rt;
  /* res->tasking = tasking; */
  res->resource_invariant = resource_invariant;
  HaArrayInit(res->partitions, a);
  HaArrayInit(res->multi_tasks, a);

  /* add one partition for each partition of rt */
  for( i = 0;  i < KheResourceTypePartitionCount(rt);  i++ )
  {
    rg = KheResourceTypePartition(rt, i);
    HaArrayAddLast(res->partitions, KhePartitionMake(rg, soln, a));
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSolverClassifyTask(KHE_PARTITION_SOLVER ps,             */
/*    KHE_TASK task, KHE_PARTITION *p, KHE_MULTI_TASK *mt)                   */
/*                                                                           */
/*  Classify task into one of three categories:                              */
/*                                                                           */
/*    * Task is assignable to resources of no partitions.  Return NULL       */
/*      in both *p and *mt.                                                  */
/*                                                                           */
/*    * Task is assignable to resources of one partition (possibly because   */
/*      it is assigned).  Return that partition in *p and NULL in *mt.       */
/*                                                                           */
/*    * Task is assignable to resources of two or more partitions.  Return   */
/*      NULL in *p and a new multi-task *mt holding the task and partitions. */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionSolverClassifyTask(KHE_PARTITION_SOLVER ps,
  KHE_TASK task, KHE_PARTITION *p, KHE_MULTI_TASK *mt)
{
  KHE_PARTITION p2;  int i;  KHE_RESOURCE r;

  /* do the classifying */
  *p = NULL;  *mt = NULL;
  r = KheTaskAsstResource(task);
  if( r != NULL )
  {
    if( !KhePartitionSolverRetrieve(ps, KheResourcePartition(r), p) )
      HnAbort("KhePartitionSolverClassifyTask internal error");
  }
  else
  {
    HaArrayForEach(ps->partitions, p2, i)
      if( !KheResourceGroupDisjoint(KheTaskDomain(task), p2->resource_group) )
      {
	if( *mt != NULL )
          KheMultiTaskAddPartition(*mt, p2);
	else if( *p != NULL )
	{
	  *mt = KheMultiTaskMake(task, ps->arena);
          KheMultiTaskAddPartition(*mt, *p);
          KheMultiTaskAddPartition(*mt, p2);
	  *p = NULL;
	}
	else
	  *p = p2;
      }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSolverSetDemandsAndTasks(KHE_PARTITION_SOLVER ps)       */
/*                                                                           */
/*  Work out the demands and tasks for ps.                                   */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionSolverSetDemandsAndTasks(KHE_PARTITION_SOLVER ps)
{
  int i;  KHE_TASK task;  KHE_PARTITION p;  KHE_MULTI_TASK mt;
  for( i = 0;  i < KheSolnTaskCount(ps->soln);  i++ )
  {
    task = KheSolnTask(ps->soln, i);
    if( KheTaskResourceType(task) == ps->resource_type )
    {
      HnAssert(!KheTaskAssignIsFixed(task),
	"KhePartitionSolverSetDemandsAndTasks internal error");
      KhePartitionSolverClassifyTask(ps, task, &p, &mt);
      if( mt != NULL )
	HaArrayAddLast(ps->multi_tasks, mt);
      else if( p != NULL )
	p->duration_demand += KheTaskTotalDuration(task);
      else
      {
	/* task is unassignable; ignore it */
	if( DEBUG1 )
	{
	  fprintf(stderr, "  unclassified task ");
	  KheTaskDebug(task, 1, 0, stderr);
	}
      }
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSolverDelete(KHE_PARTITION_SOLVER ps)                   */
/*                                                                           */
/*  Delete ps, reclaiming its memory.                                        */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KhePartitionSolverDelete(KHE_PARTITION_SOLVER ps)
{
  HaArenaD elete(ps->arena);
  ** ***
  while( HaArrayCount(ps->partitions) > 0 )
    KhePartitionDelete(MArrayRemoveLast(ps->partitions));
  MArrayFree(ps->partitions);
  while( HaArrayCount(ps->multi_tasks) > 0 )
    KheMultiTaskDelete(MArrayRemoveLast(ps->multi_tasks));
  MArrayFree(ps->multi_tasks);
  MFree(ps);
  *** **
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KhePartitionSolverDebug(KHE_PARTITION_SOLVER ps, int verbosity,     */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug printf of ps onto fp with the given verbosity and indent.          */
/*                                                                           */
/*****************************************************************************/

static void KhePartitionSolverDebug(KHE_PARTITION_SOLVER ps, int verbosity,
  int indent, FILE *fp)
{
  KHE_PARTITION p;  KHE_MULTI_TASK mt;  int i;  KHE_RESOURCE_TYPE rt;
  if( verbosity >= 1 && indent >= 0 )
  {
    rt = ps->resource_type;
    fprintf(fp, "%*s[ PartitionSolver (type %s)\n", indent, "",
      KheResourceTypeId(rt) == NULL ? "-" : KheResourceTypeId(rt));
    HaArrayForEach(ps->partitions, p, i)
      KhePartitionDebug(p, verbosity, indent + 2, fp);
    HaArrayForEach(ps->multi_tasks, mt, i)
      KheMultiTaskDebug(mt, verbosity, indent + 2, fp);
    fprintf(fp, "%*s]\n", indent, "");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskingTightenToPartition(KHE_TASKING tasking,                   */
/*    KHE_TASK_BOUND_GROUP tbg, KHE_OPTIONS options)                         */
/*                                                                           */
/*  Tighten the domains of the tasks of tasking so that they lie entirely    */
/*  within individual partitions.  If tbg != NULL, the task bounds created   */
/*  to carry this out will be added to tbg.                                  */
/*                                                                           */
/*****************************************************************************/

bool KheTightenToPartition(KHE_SOLN soln, KHE_RESOURCE_TYPE rt,
  KHE_SOLN_ADJUSTER sa, KHE_OPTIONS options)
{
  bool resource_invariant;  HA_ARENA a;  int i, j;
  KHE_PARTITION_SOLVER ps;  KHE_PARTITION p;  KHE_MULTI_TASK mt;

  resource_invariant = KheOptionsGetBool(options, "rs_invariant", false);
  if( DEBUG1 )
    fprintf(stderr, "[ KheDoPartitionJob(%s, %s)\n",
      rt == NULL ? "NULL" : KheResourceTypeId(rt) == NULL ? "?" :
      KheResourceTypeId(rt), resource_invariant ? "true" : "false");

  /* quit if tasking has no resource type, rt */
  if( rt == NULL )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheDoPartitionJob returning (no resource type)\n");
    return false;
  }

  /* quit if rt is all preassigned */
  if( KheResourceTypeDemandIsAllPreassigned(rt) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheDoPartitionJob returning (all preassigned)\n");
    return false;
  }

  /* quit if rt does not have partitions */
  if( !KheResourceTypeHasPartitions(rt) )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheDoPartitionJob returning (no partitions)\n");
    return false;
  }

  /* quit if rt has too few partitions */
  if( KheResourceTypePartitionCount(rt) < 4 )
  {
    if( DEBUG1 )
      fprintf(stderr, "] KheDoPartitionJob returning (too few partitions)\n");
    return false;
  }

  /* quit if rt has too many partitions */
  if( 3*KheResourceTypePartitionCount(rt) > KheResourceTypeResourceCount(rt) )
  {
    if( DEBUG1 )
      fprintf(stderr,"] KheDoPartitionJob returning (too many partitions)\n");
    return false;
  }

  /* initialize a partition solver */
  a = KheSolnArenaBegin(soln);
  ps = KhePartitionSolverMake(soln, rt, resource_invariant, a);

  /* set duration demand and accumulate general tasks */
  KhePartitionSolverSetDemandsAndTasks(ps);

  /* sort and debug */
  HaArraySort(ps->multi_tasks, &KheMultiTaskCmp);
  if( DEBUG4 )
    KhePartitionSolverDebug(ps, 2, 2, stderr);

  /* try to reduce the domain of each multi-task in turn */
  /* t = KheTransactionMake(KheTaskingSoln(tasking)); */
  HaArrayForEach(ps->multi_tasks, mt, i)
  {
    if( mt->dominant_partition != NULL )
    {
      /* if there is a dominant partition, try it only */
      p = mt->dominant_partition;
      if( KhePartitionAcceptsTask(p, mt->task, sa, /* tbg, */
	    ps->resource_invariant) )
      {
	p->duration_demand += KheTaskTotalDuration(mt->task);
	if( DEBUG1 )
	{
	  fprintf(stderr, "  reduced dominated task ");
	  KheTaskDebug(mt->task, 2, 0, stderr);
	}
      }
      else
      {
	if( DEBUG1 )
	{
	  fprintf(stderr, "  unreduced dominated task ");
	  KheTaskDebug(mt->task, 2, 0, stderr);
	}
      }
    }
    else
    {
      /* if there is no dominant partition, try them in avail duration order */
      HaArraySort(mt->partitions, &KhePartitionDecreasingAvailCmp);
      if( DEBUG5 )
      {
	fprintf(stderr, "  about to choose from:\n");
	HaArrayForEach(mt->partitions, p, j)
	  KhePartitionDebug(p, 2, 4, stderr);
      }
      HaArrayForEach(mt->partitions, p, j)
      {
	if( DEBUG5 )
	{
	  fprintf(stderr, "  trying:\n");
	  KhePartitionDebug(p, 2, 4, stderr);
	}
	if( KhePartitionAcceptsTask(p, mt->task, sa, /* tbg, */
	      ps->resource_invariant) )
	{
	  p->duration_demand += KheTaskTotalDuration(mt->task);
	  if( DEBUG1 )
	  {
	    fprintf(stderr, "  reduced undominated task ");
	    KheTaskDebug(mt->task, 2, 0, stderr);
	  }
	  break;
	}
      }
      if( DEBUG1 && j >= HaArrayCount(mt->partitions) )
      {
	fprintf(stderr, "  unreduced undominated task ");
	KheTaskDebug(mt->task, 2, 0, stderr);
      }
    }
  }
  /* KheTransactionDelete(t); */

  /* reclaim memory and quit */
  KheSolnArenaEnd(soln, a);
  /* KhePartitionSolverDelete(ps); */
  if( DEBUG1 )
    fprintf(stderr, "] KheDoPartitionJob returning (all done)\n");
  return true;
}


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
/*  FILE:         khe_sr_task_resource_grouper.c                             */
/*  DESCRIPTION:  Task resource grouper                                      */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_RESOURCE_GROUPER                                           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(KHE_RESOURCE) ARRAY_KHE_RESOURCE;

typedef struct khe_group_rec {
  int			leader_task_bound_count;
  ARRAY_KHE_TASK	tasks;
} *KHE_GROUP;

typedef HA_ARRAY(KHE_GROUP) ARRAY_KHE_GROUP;

struct khe_task_resource_grouper_rec {
  HA_ARENA		arena;			/* arena                     */
  KHE_RESOURCE_TYPE	type;			/* type of all tasks         */
  HA_ARRAY_INT		resource_indexes;	/* in any order              */
  ARRAY_KHE_GROUP	groups;			/* indexed by resource index */
  ARRAY_KHE_GROUP	free_groups;		/* free groups               */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_RESOURCE_GROUPER KheTaskResourceGrouperMake(                    */
/*    KHE_RESOURCE_TYPE rt, HA_ARENA a)                                      */
/*                                                                           */
/*  Return a new, empty task grouper object for resource type rt.            */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_RESOURCE_GROUPER KheTaskResourceGrouperMake(
  KHE_RESOURCE_TYPE rt, HA_ARENA a)
{
  KHE_TASK_RESOURCE_GROUPER res;
  HaMake(res, a);
  res->arena = a;
  res->type = rt;
  HaArrayInit(res->resource_indexes, a);
  HaArrayInit(res->groups, a);
  HaArrayInit(res->free_groups, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskResourceGrouperClear(KHE_TASK_RESOURCE_GROUPER trg)          */
/*                                                                           */
/*  Clear trg, ready for a fresh start, but with the same arena and type.    */
/*                                                                           */
/*****************************************************************************/

void KheTaskResourceGrouperClear(KHE_TASK_RESOURCE_GROUPER trg)
{
  int i, index;  KHE_GROUP group;

  /* move the groups to the free list */
  HaArrayForEach(trg->resource_indexes, index, i)
  {
    group = HaArray(trg->groups, index);
    HaArrayAddLast(trg->free_groups, group);
  }

  /* clear out the resources and groups arrays */
  HaArrayClear(trg->resource_indexes);
  HaArrayClear(trg->groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_GROUP KheGroupGet(KHE_TASK_RESOURCE_GROUPER trg)                     */
/*                                                                           */
/*  Get a new, empty group, either from trg's free list or from its arena.   */
/*                                                                           */
/*****************************************************************************/

static KHE_GROUP KheGroupGet(KHE_TASK_RESOURCE_GROUPER trg)
{
  KHE_GROUP res;
  if( HaArrayCount(trg->free_groups) > 0 )
  {
    res = HaArrayLastAndDelete(trg->free_groups);
    HaArrayClear(res->tasks);
  }
  else
  {
    HaMake(res, trg->arena);
    HaArrayInit(res->tasks, trg->arena);
  }
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskResourceGrouperAddTask(KHE_TASK_RESOURCE_GROUPER trg,        */
/*    KHE_TASK t)                                                            */
/*                                                                           */
/*  Add t to trg.  Return true if t is then a leader task.                   */
/*                                                                           */
/*****************************************************************************/

bool KheTaskResourceGrouperAddTask(KHE_TASK_RESOURCE_GROUPER trg, KHE_TASK t)
{
  KHE_TASK cycle_task;  KHE_RESOURCE r;  int index;  KHE_GROUP group;
  cycle_task = KheTaskAsst(t);
  HnAssert(cycle_task != NULL && KheTaskIsCycleTask(cycle_task),
    "KheTaskResourceGrouperAddTask:  task not assigned directly to cycle task");
  r = KheTaskAsstResource(t);
  HnAssert(KheResourceResourceType(r) == trg->type,
    "KheTaskResourceGrouperAddTask:  task has wrong type");
  index = KheResourceResourceTypeIndex(r);
  HaArrayFill(trg->groups, index + 1, NULL);
  group = HaArray(trg->groups, index);
  if( group == NULL )
  {
    /* add a new resource */
    HaArrayAddLast(trg->resource_indexes, index);

    /* get a new, empty group and add t to it */
    group = KheGroupGet(trg);
    HaArrayAddLast(group->tasks, t);
    group->leader_task_bound_count = 0;  /* actually underfined here */
    HaArrayPut(trg->groups, index, group);
    return true;
  }
  else
  {
    /* add t to existing group */
    HaArrayAddLast(group->tasks, t);
    return false;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskResourceGrouperLeaderTask(KHE_TASK_RESOURCE_GROUPER trg, */
/*    KHE_RESOURCE r)                                                        */
/*                                                                           */
/*  Return the leader task of r's group, or NULL if none.                    */
/*                                                                           */
/*****************************************************************************/

/* *** correct but not needed and encourages wrong thinking
KHE_TASK KheTaskResourceGrouperLeaderTask(KHE_TASK_RESOURCE_GROUPER trg,
  KHE_RESOURCE r)
{
  int index;  KHE_GROUP group;
  HnAssert(KheResourceResourceType(r) == trg->type,
    "KheTaskResourceGrouperLeaderTask:  r has wrong type");
  index = KheResourceResourceTypeIndex(r);
  if( HaArrayCount(trg->groups) <= index )
    return NULL;
  else
  {
    group = HaArray(trg->groups, index);
    return (group == NULL ? NULL : HaArrayFirst(group->tasks));
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheGroupDebug(KHE_GROUP group, int verbosity, int indent, FILE *fp) */
/*                                                                           */
/*  Debug print of group onto fp with the given verbosity and indent.        */
/*                                                                           */
/*****************************************************************************/

static void KheGroupDebug(KHE_GROUP group, int verbosity, int indent, FILE *fp)
{
  KHE_TASK task;  int i;
  fprintf(stderr, "%*s[ Group(%d bounds)\n", indent, "",
    group->leader_task_bound_count);
  HaArrayForEach(group->tasks, task, i)
    KheTaskDebug(task, verbosity, indent + 2, fp);
  fprintf(stderr, "%*s]\n", indent, "");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskResourceGrouperGroup(KHE_TASK_RESOURCE_GROUPER trg)          */
/*                                                                           */
/*  Group the tasks of trg.                                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskResourceGrouperGroup(KHE_TASK_RESOURCE_GROUPER trg)
{
  int i, j, k, index;  KHE_GROUP group;  KHE_TASK task, leader_task;
  KHE_TASK_BOUND tb;
  if( DEBUG2 )
    fprintf(stderr, "[ KheTaskResourceGrouperGroup(trg)\n");
  HaArrayForEach(trg->resource_indexes, index, i)
  {
    group = HaArray(trg->groups, index);
    if( DEBUG2 )
      KheGroupDebug(group, 3, 2, stderr);

    /* make sure the leader task is assigned to the cycle task */
    /* *** we're now not changing the assignment of the leader task
    leader_task = HaArrayFirst(group->tasks);
    r2 = KheTaskAsstResource(leader_task);
    cycle_task = KheTaskAsst(leader_task);
    if( cycle_task == NULL || !KheTaskIsCycleTask(cycle_task) || r2 != r )
    {
      ** leader_task has to be moved to r's cycle task **
      if( !KheTaskMoveResource(leader_task, r) )
	HnAbort("KheTaskResourceGrouperGroup: cannot move leader task to "
	  "resource %s", KheResourceId(r));
    }
    *** */

    /* make sure the non-leader tasks are assigned to the leader task */
    /* NB we have not prevented the leader task from being one of them */
    HnAssert(HaArrayCount(group->tasks) > 0,
      "KheTaskResourceGrouperGroup internal error");
    leader_task = HaArrayFirst(group->tasks);
    group->leader_task_bound_count = KheTaskTaskBoundCount(leader_task);
    for( j = 1;  j < HaArrayCount(group->tasks);  j++ )
    {
      task = HaArray(group->tasks, j);
      if( task != leader_task && KheTaskAsst(task) != leader_task )
      {
	/* task has to be moved to leader_task */
	if( !KheTaskMove(task, leader_task) )
	{
	  /* try adding task's bounds to leader_task */
	  for( k = 0;  k < KheTaskTaskBoundCount(task);  k++ )
	  {
	    tb = KheTaskTaskBound(task, k);
	    if( !KheTaskAddTaskBound(leader_task, tb) )
	      HnAbort("KheTaskResourceGrouperGroup: cannot add bound to "
		"leader task");
	  }
	  if( !KheTaskMove(task, leader_task) )
	    HnAbort("KheTaskResourceGrouperGroup: cannot move task to "
	      "leader task");
	}
      }
    }
  }
  if( DEBUG2 )
    fprintf(stderr, "] KheTaskResourceGrouperGroup returning)\n");
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskResourceGrouperUnGroup(KHE_TASK_RESOURCE_GROUPER trg)        */
/*                                                                           */
/*  Ungroup the tasks of trg.                                                */
/*                                                                           */
/*****************************************************************************/

void KheTaskResourceGrouperUnGroup(KHE_TASK_RESOURCE_GROUPER trg)
{
  int i, j, index;  KHE_GROUP group;  KHE_TASK task, leader_task, asst_task;
  KHE_TASK_BOUND tb;
  HaArrayForEach(trg->resource_indexes, index, i)
  {
    group = HaArray(trg->groups, index);

    /* make sure all tasks other than the leader task are assigned   */
    /* to whatever the leader task is assigned to (possibly nothing) */
    leader_task = HaArrayFirst(group->tasks);
    asst_task = KheTaskAsst(leader_task);
    for( j = 1;  j < HaArrayCount(group->tasks);  j++ )
    {
      task = HaArray(group->tasks, j);
      if( task != leader_task && KheTaskAsst(task) != asst_task )
      {
	if( !KheTaskMove(task, asst_task) )
	  HnAbort("KheTaskResourceGrouperUnGroup: cannot move task to cycle task");
      }
    }

    /* remove any bounds added to the leader task */
    while( KheTaskTaskBoundCount(leader_task) > group->leader_task_bound_count )
    {
      tb = KheTaskTaskBound(leader_task, KheTaskTaskBoundCount(leader_task)-1);
      if( !KheTaskDeleteTaskBound(leader_task, tb) )
      {
	if( DEBUG1 )
	{
	  KHE_TASK_BOUND tb2;
	  fprintf(stderr, "  KheTaskResourceGrouperUnGroup not removing task "
	    "bound:\n");
	  fprintf(stderr, "    leader task ");
	  KheTaskDebug(leader_task, 2, 0, stderr);
	  fprintf(stderr, "    asst ");
	  KheTaskDebug(KheTaskAsst(leader_task), 2, 0, stderr);
	  fprintf(stderr, "    domain ");
	  KheResourceGroupDebug(KheTaskDomain(leader_task), 2, 0, stderr);
	  fprintf(stderr, "    task bounds:\n");
	  for( i = 0;  i < KheTaskTaskBoundCount(leader_task);  i++ )
	  {
	    tb2 = KheTaskTaskBound(leader_task, i);
	    KheResourceGroupDebug(KheTaskBoundResourceGroup(tb2), 4, 0, stderr);
	  }
	  fprintf(stderr, "    problem task bound:\n");
	  KheResourceGroupDebug(KheTaskBoundResourceGroup(tb), 4, 0, stderr);
	}
	/* HnAbort("KheTaskResourceGrouperUnGroup: cannot delete task bound");*/
      }
    }
  }
}

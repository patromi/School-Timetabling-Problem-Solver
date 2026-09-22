
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
/*  FILE:         khe_sr_simple_grouping.c                                   */
/*  DESCRIPTION:  Simple grouping                                            */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"

#define DEBUG1 0
#define DEBUG2 0

/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASK_AND_INTERVAL - one task and the interval of days running   */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_and_interval_rec {
  KHE_TASK			task;
  KHE_INTERVAL			interval;
} *KHE_TASK_AND_INTERVAL;

typedef HA_ARRAY(KHE_TASK_AND_INTERVAL) ARRAY_KHE_TASK_AND_INTERVAL;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_TASKS_OF_RESOURCE - the tasks assigned a given resource         */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;

typedef struct khe_tasks_of_resource_rec {
  KHE_RESOURCE			resource;
  ARRAY_KHE_TASK_AND_INTERVAL	tasks_and_intervals;
} *KHE_TASKS_OF_RESOURCE;

typedef HA_ARRAY(KHE_TASKS_OF_RESOURCE) ARRAY_KHE_TASKS_OF_RESOURCE;


/*****************************************************************************/
/*                                                                           */
/*  Type KHE_SIMPLE_GROUPER                                                  */
/*                                                                           */
/*****************************************************************************/

typedef enum {
  KHE_SIMPLE_GROUPER_ADDING_TASKS,
  KHE_SIMPLE_GROUPER_GROUPING_DONE
} KHE_SIMPLE_GROUPER_STATE;

struct khe_simple_grouper_rec {

  /* free lists */
  ARRAY_KHE_TASK_AND_INTERVAL	task_and_interval_free_list;
  ARRAY_KHE_TASKS_OF_RESOURCE	tasks_of_resource_free_list;

  /* other fields */
  HA_ARENA			arena;
  KHE_SOLN			soln;
  KHE_FRAME			days_frame;
  KHE_TASK_GROUPER		task_grouper;
  KHE_SIMPLE_GROUPER_STATE	state;
  ARRAY_KHE_TASK		tasks;
  ARRAY_KHE_TASK		leader_tasks;
  ARRAY_KHE_TASKS_OF_RESOURCE	tasks_of_resource;
};


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASK_AND_INTERVAL"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_AND_INTERVAL KheTaskAndIntervalMake(KHE_TASK task,              */
/*    KHE_INTERVAL in, KHE_SIMPLE_GROUPER sg)                                */
/*                                                                           */
/*  Make a new task and interval object with these attributes.               */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_AND_INTERVAL KheTaskAndIntervalMake(KHE_TASK task,
  KHE_INTERVAL in, KHE_SIMPLE_GROUPER sg)
{
  KHE_TASK_AND_INTERVAL res;
  if( HaArrayCount(sg->task_and_interval_free_list) > 0 )
    res = HaArrayLastAndDelete(sg->task_and_interval_free_list);
  else
    HaMake(res, sg->arena);
  res->task = task;
  res->interval = in;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAndIntervalTypedCmp(KHE_TASK_AND_INTERVAL ti1,                */
/*    KHE_TASK_AND_INTERVAL ti2)                                             */
/*                                                                           */
/*  Typed comparison function for sorting an array of tasks and              */
/*  intervals by increasing interval.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheTaskAndIntervalTypedCmp(KHE_TASK_AND_INTERVAL ti1,
  KHE_TASK_AND_INTERVAL ti2)
{
  return KheIntervalTypedCmp(ti1->interval, ti2->interval);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAndIntervalCmp(const void *t1, const void *t2)                */
/*                                                                           */
/*  Untyped comparison function for sorting an array of tasks and            */
/*  intervals by increasing interval.                                        */
/*                                                                           */
/*****************************************************************************/

static int KheTaskAndIntervalCmp(const void *t1, const void *t2)
{
  KHE_TASK_AND_INTERVAL ti1 = * (KHE_TASK_AND_INTERVAL *) t1;
  KHE_TASK_AND_INTERVAL ti2 = * (KHE_TASK_AND_INTERVAL *) t2;
  return KheTaskAndIntervalTypedCmp(ti1, ti2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_TASKS_OF_RESOURCE"                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASKS_OF_RESOURCE KheTasksOfResourceMake(KHE_RESOURCE r,             */
/*    KHE_SIMPLE_GROUPER sg)                                                 */
/*                                                                           */
/*  Make a new tasks of resource object for resource r.                      */
/*                                                                           */
/*****************************************************************************/

static KHE_TASKS_OF_RESOURCE KheTasksOfResourceMake(KHE_RESOURCE r,
  KHE_SIMPLE_GROUPER sg)
{
  KHE_TASKS_OF_RESOURCE res;
  if( HaArrayCount(sg->tasks_of_resource_free_list) > 0 )
  {
    res = HaArrayLastAndDelete(sg->tasks_of_resource_free_list);
    HaArrayClear(res->tasks_and_intervals);
  }
  else
  {
    HaMake(res, sg->arena);
    HaArrayInit(res->tasks_and_intervals, sg->arena);
  }
  res->resource = r;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTasksOfResourceFree(KHE_TASKS_OF_RESOURCE tr,                    */
/*    KHE_SIMPLE_GROUPER sg)                                                 */
/*                                                                           */
/*  Free tr.                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheTasksOfResourceFree(KHE_TASKS_OF_RESOURCE tr,
  KHE_SIMPLE_GROUPER sg)
{
  int i;
  HaArrayAppend(sg->task_and_interval_free_list, tr->tasks_and_intervals, i);
  HaArrayAddLast(sg->tasks_of_resource_free_list, tr);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTasksOfResourceGroupAllTasks(KHE_TASKS_OF_RESOURCE tr,           */
/*    KHE_SIMPLE_GROUPER sg, KHE_SOLN_ADJUSTER sa)                           */
/*                                                                           */
/*  Group all tasks in tr.                                                   */
/*                                                                           */
/*****************************************************************************/

static void KheTasksOfResourceGroupAllTasks(KHE_TASKS_OF_RESOURCE tr,
  KHE_SIMPLE_GROUPER sg, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK leader_task;  int i;  KHE_TASK_AND_INTERVAL ti;
  if( HaArrayCount(tr->tasks_and_intervals) > 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "[ KheTasksOfResourceGroupAllTasks:\n");
    KheTaskGrouperClear(sg->task_grouper);
    HaArrayForEach(tr->tasks_and_intervals, ti, i)
    {
      if( !KheTaskGrouperAddTask(sg->task_grouper, ti->task) )
      {
	/* no luck here, start a new group beginning with ti->task */
	leader_task = KheTaskGrouperMakeGroup(sg->task_grouper, sa);
	HaArrayAddLast(sg->leader_tasks, leader_task);
	if( DEBUG1 )
	  fprintf(stderr, "  KheTasksOfResourceGroupAllTasks(%s) adding leader"
	    " task %s\n", KheResourceId(tr->resource), KheTaskId(leader_task));
	KheTaskGrouperClear(sg->task_grouper);
        if( !KheTaskGrouperAddTask(sg->task_grouper, ti->task) )
	  HnAbort("KheTasksOfResourceGroupAllTasks internal error");
      }
    }

    /* at end, finish off the last group */
    leader_task = KheTaskGrouperMakeGroup(sg->task_grouper, sa);
    HaArrayAddLast(sg->leader_tasks, leader_task);
    if( DEBUG1 )
      fprintf(stderr, "  KheTasksOfResourceGroupAllTasks(%s) adding leader"
	" task %s\n", KheResourceId(tr->resource), KheTaskId(leader_task));
    KheTaskGrouperClear(sg->task_grouper);
    if( DEBUG2 )
      fprintf(stderr, "] KheTasksOfResourceGroupAllTasks\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskAndIntervalAdjacent(KHE_TASK_AND_INTERVAL prev_ti,           */
/*    KHE_TASK_AND_INTERVAL ti)                                              */
/*                                                                           */
/*  Return true if the intervals of these two objects are consecutive.       */
/*  As a special case, also return true if prev_ti = NULL.                   */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskAndIntervalAdjacent(KHE_TASK_AND_INTERVAL prev_ti,
  KHE_TASK_AND_INTERVAL ti)
{
  return prev_ti == NULL || prev_ti->interval.last == ti->interval.first - 1;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTasksOfResourceGroupConsecutiveTasks(KHE_TASKS_OF_RESOURCE tr,   */
/*    KHE_SIMPLE_GROUPER sg, KHE_SOLN_ADJUSTER sa)                           */
/*                                                                           */
/*  Group consecutive tasks in tr.                                           */
/*                                                                           */
/*****************************************************************************/

static void KheTasksOfResourceGroupConsecutiveTasks(KHE_TASKS_OF_RESOURCE tr,
  KHE_SIMPLE_GROUPER sg, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK leader_task;  int i;  KHE_TASK_AND_INTERVAL ti, prev_ti;
  HaArraySort(tr->tasks_and_intervals, &KheTaskAndIntervalCmp);
  if( HaArrayCount(tr->tasks_and_intervals) > 0 )
  {
    if( DEBUG2 )
      fprintf(stderr, "[ KheTasksOfResourceGroupConsecutiveTasks:\n");
    KheTaskGrouperClear(sg->task_grouper);
    prev_ti = NULL;
    HaArrayForEach(tr->tasks_and_intervals, ti, i)
    {
      if( !KheTaskAndIntervalAdjacent(prev_ti, ti) ||
	  !KheTaskGrouperAddTask(sg->task_grouper, ti->task) )
      {
	/* no luck here, start a new group */
	leader_task = KheTaskGrouperMakeGroup(sg->task_grouper, sa);
	HaArrayAddLast(sg->leader_tasks, leader_task);
	KheTaskGrouperClear(sg->task_grouper);
        if( !KheTaskGrouperAddTask(sg->task_grouper, ti->task) )
	  HnAbort("KheTasksOfResourceGroupConsecutiveTasks internal error");
      }
    }

    /* at end, finish off the last group */
    leader_task = KheTaskGrouperMakeGroup(sg->task_grouper, sa);
    HaArrayAddLast(sg->leader_tasks, leader_task);
    KheTaskGrouperClear(sg->task_grouper);
    if( DEBUG2 )
      fprintf(stderr, "] KheTasksOfResourceGroupConsecutiveTasks\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "KHE_SIMPLE_GROUPER"                                           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_SIMPLE_GROUPER KheSimpleGrouperMake(KHE_SOLN soln,                   */
/*    KHE_FRAME days_frame, HA_ARENA a)                                      */
/*                                                                           */
/*  Make a task multi-grouper with these attributes.                         */
/*                                                                           */
/*****************************************************************************/

KHE_SIMPLE_GROUPER KheSimpleGrouperMake(KHE_SOLN soln,
  KHE_FRAME days_frame, KHE_TASK_GROUP_DOMAIN_FINDER tgdf, HA_ARENA a)
{
  KHE_SIMPLE_GROUPER res;
  HnAssert(days_frame != NULL, "KheSimpleGrouperMake: days_frame == NULL");
  HaMake(res, a);
  HaArrayInit(res->task_and_interval_free_list, a);
  HaArrayInit(res->tasks_of_resource_free_list, a);
  res->arena = a;
  res->soln = soln;
  res->days_frame = days_frame;
  res->task_grouper = KheTaskGrouperMake(soln, days_frame, tgdf, a);
  res->state = KHE_SIMPLE_GROUPER_ADDING_TASKS;
  HaArrayInit(res->tasks, a);
  HaArrayInit(res->leader_tasks, a);
  HaArrayInit(res->tasks_of_resource, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSimpleGrouperClear(KHE_SIMPLE_GROUPER sg)                        */
/*                                                                           */
/*  Clear sg.                                                                */
/*                                                                           */
/*****************************************************************************/

void KheSimpleGrouperClear(KHE_SIMPLE_GROUPER sg)
{
  int i;  KHE_TASKS_OF_RESOURCE tr;
  sg->state = KHE_SIMPLE_GROUPER_ADDING_TASKS;
  HaArrayClear(sg->tasks);
  HaArrayClear(sg->leader_tasks);
  HaArrayForEach(sg->tasks_of_resource, tr, i)
    if( tr != NULL )
      KheTasksOfResourceFree(tr, sg);
  HaArrayClear(sg->tasks_of_resource);
  sg->state = KHE_SIMPLE_GROUPER_ADDING_TASKS;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheSimpleGrouperAddTask(KHE_SIMPLE_GROUPER sg, KHE_TASK task)       */
/*                                                                           */
/*  Add task to sg and return true, or if task is not a proper root task,    */
/*  do nothing and return false.                                             */
/*                                                                           */
/*****************************************************************************/

bool KheSimpleGrouperAddTask(KHE_SIMPLE_GROUPER sg, KHE_TASK task)
{
  HnAssert(sg->state == KHE_SIMPLE_GROUPER_ADDING_TASKS,
    "KheSimpleGrouperAddTask internal error (called after groups made)");
  if( KheTaskIsProperRoot(task) )
  {
    HaArrayAddLast(sg->tasks, task);
    return true;
  }
  else
    return false;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSimpleGrouperAddResourceTypeTasks(KHE_SIMPLE_GROUPER sg,         */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Add all proper root tasks of type rt to sg.                              */
/*                                                                           */
/*****************************************************************************/

void KheSimpleGrouperAddResourceTypeTasks(KHE_SIMPLE_GROUPER sg,
  KHE_RESOURCE_TYPE rt)
{
  KHE_TASK task;  int i;
  for( i = 0;  i < KheSolnTaskCount(sg->soln);  i++ )
  {
    task = KheSolnTask(sg->soln, i);
    if( KheTaskResourceType(task) == rt )
      KheSimpleGrouperAddTask(sg, task);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSimpleGrouperAddAssignedResourceTypeTasks(KHE_SIMPLE_GROUPER sg, */
/*    KHE_RESOURCE_TYPE rt)                                                  */
/*                                                                           */
/*  Add all assigned proper root tasks of type rt to sg.                     */
/*                                                                           */
/*****************************************************************************/

void KheSimpleGrouperAddAssignedResourceTypeTasks(KHE_SIMPLE_GROUPER sg,
  KHE_RESOURCE_TYPE rt)
{
  KHE_TASK task;  int i, j;  KHE_RESOURCE r;
  for( i = 0;  i < KheResourceTypeResourceCount(rt);  i++ )
  {
    r = KheResourceTypeResource(rt, i);
    for( j = 0;  j < KheResourceAssignedTaskCount(sg->soln, r);  j++ )
    {
      task = KheResourceAssignedTask(sg->soln, r, j);
      KheSimpleGrouperAddTask(sg, task);
    }
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddTaskToTasksOfResource(KHE_TASK task, KHE_RESOURCE r,          */
/*    KHE_SIMPLE_GROUPER sg)                                                 */
/*                                                                           */
/*  Add task (assumed to be assigned r) to sg's task of resource structure.  */
/*                                                                           */
/*****************************************************************************/

static void KheAddTaskToTasksOfResource(KHE_TASK task, KHE_RESOURCE r,
  KHE_SIMPLE_GROUPER sg)
{
  int index;  KHE_TASKS_OF_RESOURCE tr;  KHE_INTERVAL in;
  KHE_TASK_AND_INTERVAL ti;

  /* get tr, the required tasks of resource object */
  index = KheResourceInstanceIndex(r);
  HaArrayFill(sg->tasks_of_resource, index + 1, NULL);
  tr = HaArray(sg->tasks_of_resource, index);
  if( tr == NULL )
  {
    tr = KheTasksOfResourceMake(r, sg);
    HaArrayPut(sg->tasks_of_resource, index, tr);
  }

  /* add task and its interval to tr */
  if( sg->days_frame != NULL )
    in = KheTaskInterval(task, sg->days_frame);
  else
    in = KheIntervalMake(1, 0);
  ti = KheTaskAndIntervalMake(task, in, sg);
  HaArrayAddLast(tr->tasks_and_intervals, ti);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheSimpleGrouperMakeGroups(KHE_SIMPLE_GROUPER sg,                   */
/*    KHE_SIMPLE_GROUPER_GROUP_TYPE group_type, KHE_SOLN_ADJUSTER sa)        */
/*                                                                           */
/*  Make groups out of the tasks that have been added.                       */
/*                                                                           */
/*****************************************************************************/

void KheSimpleGrouperMakeGroups(KHE_SIMPLE_GROUPER sg,
  KHE_SIMPLE_GROUPER_GROUP_TYPE group_type, KHE_SOLN_ADJUSTER sa)
{
  KHE_TASK task;  int i;  KHE_RESOURCE r;  KHE_TASKS_OF_RESOURCE tr;

  /* distribute the tasks to the leader tasks and tasks_of_resource */
  HnAssert(sg->state == KHE_SIMPLE_GROUPER_ADDING_TASKS,
    "KheSimpleGrouperMakeGroups internal error (groups already made)");
  HnAssert(HaArrayCount(sg->leader_tasks) == 0,
    "KheSimpleGrouperMakeGroups internal error 1");
  HaArrayForEach(sg->tasks, task, i)
  {
    r = KheTaskAsstResource(task);
    if( r == NULL )
    {
      HaArrayAddLast(sg->leader_tasks, task);
      if( DEBUG1 )
	fprintf(stderr, "  KheSimpleGrouperMakeGroups adding unassigned "
	  "leader task %s\n", KheTaskId(task));
    }
    else
    {
      KheAddTaskToTasksOfResource(task, r, sg);
      if( DEBUG1 )
	fprintf(stderr, "  KheSimpleGrouperMakeGroups adding assigned "
	  "(%s) leader task %s\n", KheResourceId(r), KheTaskId(task));
    }
  }

  switch( group_type )
  {
    case KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE:

      HaArrayForEach(sg->tasks_of_resource, tr, i)
	if( tr != NULL )
	  KheTasksOfResourceGroupAllTasks(tr, sg, sa);
      break;

    case KHE_SIMPLE_GROUPER_GROUP_SAME_RESOURCE_CONSECUTIVE:

      HaArrayForEach(sg->tasks_of_resource, tr, i)
	if( tr != NULL )
	  KheTasksOfResourceGroupConsecutiveTasks(tr, sg, sa);
      break;

    default:

      HnAbort("KheSimpleGrouperMakeGroups internal error 2");
      break;
  }


  /* remainder still to do */
  sg->state = KHE_SIMPLE_GROUPER_GROUPING_DONE;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheSimpleGrouperGroupCount(KHE_SIMPLE_GROUPER sg)                    */
/*                                                                           */
/*  Return the number of leader tasks.                                       */
/*                                                                           */
/*****************************************************************************/

int KheSimpleGrouperGroupCount(KHE_SIMPLE_GROUPER sg)
{
  HnAssert(sg->state == KHE_SIMPLE_GROUPER_GROUPING_DONE,
    "KheSimpleGrouperGroupCount called out of order (groups not yet made)");
  return HaArrayCount(sg->leader_tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheSimpleGrouperGroup(KHE_SIMPLE_GROUPER sg, int i)             */
/*                                                                           */
/*  Return the i'th leader task.                                             */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheSimpleGrouperGroup(KHE_SIMPLE_GROUPER sg, int i)
{
  HnAssert(sg->state == KHE_SIMPLE_GROUPER_GROUPING_DONE,
    "KheSimpleGrouperGroup called out of order (groups not yet made)");
  return HaArray(sg->leader_tasks, i);
}

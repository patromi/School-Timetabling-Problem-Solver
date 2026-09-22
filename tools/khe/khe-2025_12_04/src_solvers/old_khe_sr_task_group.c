
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
/*  FILE:         khe_sr_task_group.c                                        */
/*  DESCRIPTION:  Task groups                                                */
/*                                                                           */
/*****************************************************************************/
#include "khe_solvers.h"


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP                                                           */
/*                                                                           */
/*****************************************************************************/

typedef HA_ARRAY(KHE_TASK) ARRAY_KHE_TASK;
typedef HA_ARRAY(KHE_TASK_GROUP) ARRAY_KHE_TASK_GROUP;
typedef HA_ARRAY(int64_t) ARRAY_INT64;

struct khe_task_group_rec {
  KHE_TASK_GROUPS		task_groups;		/* encl task_groups  */
  ARRAY_KHE_TASK		tasks;			/* the tasks         */
  bool				tasks_sorted;		/* true after asst   */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPS                                                          */
/*                                                                           */
/*****************************************************************************/

struct khe_task_groups_rec {
  HA_ARENA			arena;			/* arena             */
  KHE_TASKING			tasking;		/* orig. tasking     */
  ARRAY_KHE_TASK_GROUP		task_groups;		/* task groups       */
};


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_OBJ - an object representing one task, in detail (private)      */
/*                                                                           */
/*****************************************************************************/

typedef struct khe_task_obj_rec {
  KHE_TASK			task;		/* the task of interest      */
  ARRAY_KHE_TASK		full_task_set;	/* all its descendants       */
  bool				time_complete;	/* true if all have times    */
  int				total_duration;	/* of all descendants        */
  float				total_workload;	/* of all descendants        */
} *KHE_TASK_OBJ;

typedef HA_ARRAY(KHE_TASK_OBJ) ARRAY_KHE_TASK_OBJ;


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task objects" (private)                                       */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskObjBuildFullTaskSet(KHE_TASK_OBJ task_obj, KHE_TASK task)    */
/*                                                                           */
/*  Build the full task set of task (the set of tasks assigned to task       */
/*  directly or indirectly, and including task itself, that lie in soln      */
/*  events).                                                                 */
/*                                                                           */
/*****************************************************************************/

static void KheTaskObjBuildFullTaskSet(KHE_TASK_OBJ task_obj, KHE_TASK task)
{
  int i;  KHE_MEET meet;
  meet = KheTaskMeet(task);
  if( meet != NULL )
  {
    HaArrayAddLast(task_obj->full_task_set, task);
    task_obj->total_duration += KheTaskDuration(task);
    task_obj->total_workload += KheTaskWorkload(task);
    if( KheMeetAsstTime(meet) == NULL )
      task_obj->time_complete = false;
  }
  for( i = 0;  i < KheTaskAssignedToCount(task);  i++ )
    KheTaskObjBuildFullTaskSet(task_obj, KheTaskAssignedTo(task, i));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskAssignedTimeCmp(const void *t1, const void *t2)               */
/*                                                                           */
/*  Comparison function for sorting an array of tasks into increasing        */
/*  starting time then decreasing duration and workload order, assuming      */
/*  that they all lie in meets that have an assigned time, as guaranteed     */
/*  at this function's point of call in KheTaskObjMake below.                */
/*                                                                           */
/*****************************************************************************/

static int KheTaskAssignedTimeCmp(const void *t1, const void *t2)
{
  KHE_TASK task1 = * (KHE_TASK *) t1;
  KHE_TASK task2 = * (KHE_TASK *) t2;
  KHE_TIME time1 = KheMeetAsstTime(KheTaskMeet(task1));
  KHE_TIME time2 = KheMeetAsstTime(KheTaskMeet(task2));
  if( time1 != time2 )
    return KheTimeIndex(time1) - KheTimeIndex(time2);
  else if( KheTaskDuration(task1) != KheTaskDuration(task2) )
    return KheTaskDuration(task2) - KheTaskDuration(task1);
  else if( KheTaskWorkload(task1) != KheTaskWorkload(task2) )
    return KheTaskWorkload(task2) > KheTaskWorkload(task1) ? 1 : -1;
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_OBJ KheTaskObjMake(KHE_TASK task)                               */
/*                                                                           */
/*  Make a task object representing task.                                    */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_OBJ KheTaskObjMake(KHE_TASK task, HA_ARENA a)
{
  KHE_TASK_OBJ res;
  HaMake(res, a);
  res->task = task;
  HaArrayInit(res->full_task_set, a);
  res->time_complete = true;
  res->total_duration = 0;
  res->total_workload = 0.0;
  KheTaskObjBuildFullTaskSet(res, task);
  if( res->time_complete )
    HaArraySort(res->full_task_set, &KheTaskAssignedTimeCmp);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskObjDelete(KHE_TASK_OBJ task_obj)                             */
/*                                                                           */
/*  Delete task_obj.                                                         */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskObjDelete(KHE_TASK_OBJ task_obj)
{
  MArrayFree(task_obj->full_task_set);
  MFree(task_obj);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheResourceGroupCmp(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)  */
/*                                                                           */
/*  Comparison function for comparing resource groups.                       */
/*                                                                           */
/*****************************************************************************/

/* *** moved to KheResourceGroupTypedCmp in the platform
static int KheResourceGroupCmp(KHE_RESOURCE_GROUP rg1, KHE_RESOURCE_GROUP rg2)
{
  KHE_RESOURCE r1, r2;  int i;
  if( KheResourceGroupEqual(rg1, rg2) )
    return 0;
  else if( KheResourceGroupResourceCount(rg1) !=
      KheResourceGroupResourceCount(rg2) )
    return KheResourceGroupResourceCount(rg1) -
      KheResourceGroupResourceCount(rg2);
  else
  {
    for( i = 0;  i < KheResourceGroupResourceCount(rg1);  i++ )
    {
      r1 = KheResourceGroupResource(rg1, i);
      r2 = KheResourceGroupResource(rg2, i);
      if( r1 != r2 )
	return KheResourceInstanceIndex(r1) - KheResourceInstanceIndex(r2);
    }
    HnAbort("KheResourceGroupCmp internal error");
    return 0;  ** keep compiler happy **
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskObjInterchangeable(KHE_TASK_OBJ to1, KHE_TASK_OBJ to2)        */
/*                                                                           */
/*  Comparison function for bringing interchangeable task objects together.  */
/*                                                                           */
/*****************************************************************************/

static int KheTaskObjInterchangeable(KHE_TASK_OBJ to1, KHE_TASK_OBJ to2)
{
  int c, i;  KHE_TASK task1, task2;  KHE_TIME time1, time2;
  if( !to1->time_complete )
    return -1;
  else if( !to2->time_complete )
    return 1;
  else if( to1->total_duration != to2->total_duration )
    return to2->total_duration - to1->total_duration;
  else if( to1->total_workload != to2->total_workload )
    return to2->total_workload > to1->total_workload ? 1 : -1;
  else if( HaArrayCount(to1->full_task_set) != HaArrayCount(to2->full_task_set) )
    return HaArrayCount(to1->full_task_set) - HaArrayCount(to2->full_task_set);
  else
  {
    c = KheResourceGroupTypedCmp(KheTaskDomain(to1->task),
      KheTaskDomain(to2->task));
    if( c != 0 )
      return c;
    for( i = 0;  i < HaArrayCount(to1->full_task_set);  i++ )
    {
      task1 = HaArray(to1->full_task_set, i);
      task2 = HaArray(to2->full_task_set, i);
      time1 = KheMeetAsstTime(KheTaskMeet(task1));
      time2 = KheMeetAsstTime(KheTaskMeet(task2));
      if( time1 != time2 )
	return KheTimeIndex(time1) - KheTimeIndex(time2);
    }
    return 0;
  }
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskObjInterchangeableCmp(const void *t1, const void *t2)         */
/*                                                                           */
/*  Comparison function for sorting task objects to bring interchangeable    */
/*  ones together.                                                           */
/*                                                                           */
/*****************************************************************************/

static int KheTaskObjInterchangeableCmp(const void *t1, const void *t2)
{
  KHE_TASK_OBJ to1 = * (KHE_TASK_OBJ *) t1;
  KHE_TASK_OBJ to2 = * (KHE_TASK_OBJ *) t2;
  return KheTaskObjInterchangeable(to1, to2);
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "task groups"                                                  */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP KheTaskGroupMake(KHE_TASK_GROUPS task_groups)             */
/*                                                                           */
/*  Make a new, empty task group object with these attributes.               */
/*                                                                           */
/*****************************************************************************/

static KHE_TASK_GROUP KheTaskGroupMake(KHE_TASK_GROUPS task_groups, HA_ARENA a)
{
  KHE_TASK_GROUP res;
  HaMake(res, a);
  res->task_groups = task_groups;
  HaArrayInit(res->tasks, a);
  res->tasks_sorted = false;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheAddTaskGroups(KHE_TASK_GROUPS task_groups)                       */
/*                                                                           */
/*  Add the task groups to task_groups.                                      */
/*                                                                           */
/*****************************************************************************/

static void KheAddTaskGroups(KHE_TASK_GROUPS task_groups, HA_ARENA a)
{
  ARRAY_KHE_TASK_OBJ task_objs;  int i;  KHE_TASK task;
  KHE_TASK_OBJ to, prev_to;  KHE_TASK_GROUP task_group;

  /* build the task objects */
  HaArrayInit(task_objs, a);
  for( i = 0;  i < KheTaskingTaskCount(task_groups->tasking);  i++ )
  {
    task = KheTaskingTask(task_groups->tasking, i);
    HaArrayAddLast(task_objs, KheTaskObjMake(task, a));
  }

  /* sort them to bring interchangeable task objects together */
  HaArraySort(task_objs, &KheTaskObjInterchangeableCmp);

  /* make one task group for each run of adjacent interchangeable tasks */
  prev_to = NULL;
  task_group = NULL;  /* keep compiler happy */
  for( i = 0;  i < HaArrayCount(task_objs);  i++ )
  {
    to = HaArray(task_objs, i);
    if( prev_to == NULL || KheTaskObjInterchangeable(prev_to, to) != 0 )
    {
      /* at start, or not interchangeable, so need a new task group */
      task_group = KheTaskGroupMake(task_groups, a);
      HaArrayAddLast(task_groups->task_groups, task_group);
    }
    HaArrayAddLast(task_group->tasks, to->task);
    prev_to = to;
  }

  /* free the task objects and their array */
  /* ***
  while( HaArrayCount(task_objs) > 0 )
    KheTaskObjDelete(MArrayRemoveLast(task_objs));
  HaArrayFree(task_objs);
  *** */
}

/* *** old version
static void KheAddTaskGroups(KHE_TASK_GROUPS task_groups)
{
  KHE_TASK task, prev_task;  int i;  KHE_TASK_GROUP task_group;
  HnAssert(HaArrayCount(task_groups->task_groups) == 0,
    "KheAddTaskGroups: task groups already present");

  ** make sure each separate task is sorted **
  KheTaskingSortForTaskGroups(task_groups->tasking);

  ** make one task set for each run of interchangeable tasks **
  prev_task = NULL;
  for( i = 0;  i < KheTaskingTaskCount(task_groups->tasking);  i++ )
  {
    task = KheTaskingTask(task_groups->tasking, i);
    if( prev_task == NULL || KheTasksInterchangeable(task, prev_task) != 0 )
    {
      ** at start, or not interchangeable, so need a new task set **
      task_group = KheTaskGroupMake(task_groups);
      HaArrayAddLast(task_groups->task_groups, task_group);
    }
    HaArrayAddLast(task_group->tasks, task);
    prev_task = task;
  }
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUPS KheTaskGroupsMakeFromTasking(KHE_TASKING tasking)        */
/*                                                                           */
/*  Make a new task groups object holding the tasks of tasking.              */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUPS KheTaskGroupsMakeFromTasking(KHE_TASKING tasking)
{
  KHE_TASK_GROUPS res;  HA_ARENA a;  KHE_SOLN soln;
  soln = KheTaskingSoln(tasking);
  a = KheSolnArenaBegin(soln, false);
  /* a = HaArena Make(); */
  HaMake(res, a);
  res->arena = a;
  res->tasking = tasking;
  HaArrayInit(res->task_groups, a);
  KheAddTaskGroups(res, a);
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDelete(KHE_TASK_GROUP task_group)                       */
/*                                                                           */
/*  Delete task_group.                                                       */
/*                                                                           */
/*****************************************************************************/

/* ***
static void KheTaskGroupDelete(KHE_TASK_GROUP task_group)
{
  MArrayFree(task_group->tasks);
  MFree(task_group);
}
*** */


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupsDelete(KHE_TASK_GROUPS task_groups)                    */
/*                                                                           */
/*  Remove task_groups and free its memory.                                  */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupsDelete(KHE_TASK_GROUPS task_groups)
{
  KheSolnArenaEnd(KheTaskingSoln(task_groups->tasking), task_groups->arena);
  /* HaArenaDel ete(task_groups->arena); */
  /* ***
  while( HaArrayCount(task_groups->task_groups) > 0 )
    KheTaskGroupDelete(MArrayRemoveLast(task_groups->task_groups));
  MArrayFree(task_groups->task_groups);
  MFree(task_groups);
  *** */
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupsTaskGroupCount(KHE_TASK_GROUPS task_groups)             */
/*                                                                           */
/*  Return the number of task groups in task_groups.                         */
/*                                                                           */
/*****************************************************************************/

int KheTaskGroupsTaskGroupCount(KHE_TASK_GROUPS task_groups)
{
  return HaArrayCount(task_groups->task_groups);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK_GROUP KheTaskGroupsTaskGroup(KHE_TASK_GROUPS task_groups, int i)*/
/*                                                                           */
/*  Return the i'th task group of task_groups.                               */
/*                                                                           */
/*****************************************************************************/

KHE_TASK_GROUP KheTaskGroupsTaskGroup(KHE_TASK_GROUPS task_groups, int i)
{
  return HaArray(task_groups->task_groups, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupTaskCount(KHE_TASK_GROUP task_group)                     */
/*                                                                           */
/*  Return the number of tasks in task_group.                                */
/*                                                                           */
/*****************************************************************************/

int KheTaskGroupTaskCount(KHE_TASK_GROUP task_group)
{
  return HaArrayCount(task_group->tasks);
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_TASK KheTaskGroupTask(KHE_TASK_GROUP task_group, int i)              */
/*                                                                           */
/*  Return the i'th tasks of task_group.                                     */
/*                                                                           */
/*****************************************************************************/

KHE_TASK KheTaskGroupTask(KHE_TASK_GROUP task_group, int i)
{
  return HaArray(task_group->tasks, i);
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupTotalDuration(KHE_TASK_GROUP task_group)                 */
/*                                                                           */
/*  Return the common total duration of the tasks of task_group.             */
/*                                                                           */
/*****************************************************************************/

int KheTaskGroupTotalDuration(KHE_TASK_GROUP task_group)
{
  HnAssert(HaArrayCount(task_group->tasks) > 0,
    "KheTaskGroupTotalDuration internal error");
  return KheTaskTotalDuration(HaArrayFirst(task_group->tasks));
}


/*****************************************************************************/
/*                                                                           */
/*  float KheTaskGroupTotalWorkload(KHE_TASK_GROUP task_group)               */
/*                                                                           */
/*  Return the common total workload of the tasks of task_group.             */
/*                                                                           */
/*****************************************************************************/

float KheTaskGroupTotalWorkload(KHE_TASK_GROUP task_group)
{
  HnAssert(HaArrayCount(task_group->tasks) > 0,
    "KheTaskGroupTotalWorkload internal error");
  return KheTaskTotalWorkload(HaArrayFirst(task_group->tasks));
}


/*****************************************************************************/
/*                                                                           */
/*  KHE_RESOURCE_GROUP KheTaskGroupDomain(KHE_TASK_GROUP task_group)         */
/*                                                                           */
/*  Return the common domain of the tasks of task_group.                     */
/*                                                                           */
/*****************************************************************************/

KHE_RESOURCE_GROUP KheTaskGroupDomain(KHE_TASK_GROUP task_group)
{
  HnAssert(HaArrayCount(task_group->tasks) > 0,
    "KheTaskGroupDomain internal error");
  return KheTaskDomain(HaArrayFirst(task_group->tasks));
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupDecreasingDurationCmp(KHE_TASK_GROUP tg1,                */
/*    KHE_TASK_GROUP tg2)                                                    */
/*                                                                           */
/*  Comparison function for sorting task groups by decreasing duration.      */
/*                                                                           */
/*****************************************************************************/

int KheTaskGroupDecreasingDurationCmp(KHE_TASK_GROUP tg1, KHE_TASK_GROUP tg2)
{
  if( KheTaskGroupTotalDuration(tg1) != KheTaskGroupTotalDuration(tg2) )
    return KheTaskGroupTotalDuration(tg2) - KheTaskGroupTotalDuration(tg1);
  else if( HaArrayCount(tg1->tasks) != HaArrayCount(tg2->tasks) )
    return HaArrayCount(tg2->tasks) - HaArrayCount(tg1->tasks);
  else if( HaArrayCount(tg1->tasks) > 0 )
  {
    KHE_TASK task1 = HaArrayFirst(tg1->tasks);
    KHE_TASK task2 = HaArrayFirst(tg2->tasks);
    return KheTaskSolnIndex(task1) - KheTaskSolnIndex(task2);
  }
  else
    return 0;
}


/*****************************************************************************/
/*                                                                           */
/*  int KheTaskGroupUnassignedTaskCount(KHE_TASK_GROUP task_group)           */
/*                                                                           */
/*  Return the number of unassigned tasks of task_group.                     */
/*                                                                           */
/*****************************************************************************/

int KheTaskGroupUnassignedTaskCount(KHE_TASK_GROUP task_group)
{
  int res, i;  KHE_TASK task;
  res = 0;
  HaArrayForEach(task_group->tasks, task, i)
    if( KheTaskAsst(task) == NULL )
      res++;
  return res;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupHasUnassignedTask(KHE_TASK_GROUP task_group,            */
/*    KHE_TASK *task)                                                        */
/*                                                                           */
/*  If task_group has an unassigned task, set *task to the first one and     */
/*  return true, otherwise return false.                                     */
/*                                                                           */
/*****************************************************************************/

static bool KheTaskGroupHasUnassignedTask(KHE_TASK_GROUP task_group,
  KHE_TASK *task)
{
  int i;
  HaArrayForEach(task_group->tasks, *task, i)
    if( KheTaskAsst(*task) == NULL )
      return true;
  return false;
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupAssignCheck(KHE_TASK_GROUP task_group, KHE_RESOURCE r)  */
/*                                                                           */
/*  Check whether r can be assigned to a task of task_group.                 */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGroupAssignCheck(KHE_TASK_GROUP task_group, KHE_RESOURCE r)
{
  KHE_TASK task;
  return KheTaskGroupHasUnassignedTask(task_group, &task) &&
    KheTaskAssignResourceCheck(task, r);
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupSortTasks(KHE_TASK_GROUP task_group, KHE_RESOURCE r)    */
/*                                                                           */
/*  Sort the tasks of task_group by assigning r to each in turn and          */
/*  observing the cost afterwards.   It is known that all of the tasks       */
/*  are unassigned and r is suitable for assignment to each.                 */
/*                                                                           */
/*  Implementation note.  A simple insertion sort is used.  It is not        */
/*  convenient to use qsort here.  The array size is usually small, and      */
/*  insertion sort will be faster anyway in the usual case where all costs   */
/*  are equal.                                                               */
/*                                                                           */
/*****************************************************************************/

static void KheTaskGroupSortTasks(KHE_TASK_GROUP task_group, KHE_RESOURCE r)
{
  KHE_TASK task;  int i, j;  KHE_COST cost;  ARRAY_INT64 costs;  HA_ARENA a;
  a = task_group->task_groups->arena;
  HaArrayInit(costs, a);
  HaArrayForEach(task_group->tasks, task, i)
  {
    if( KheTaskAsst(task) != NULL )
      cost = -1;
    else
    {
      if( !KheTaskAssignResource(task, r) )
	HnAbort("KheTaskGroupSortTasks internal error");
      cost = KheSolnCost(KheTaskSoln(task));
    }
    HaArrayAddLast(costs, cost);
    for( j = i - 1;  j >= 0 && HaArray(costs, j) > cost;  j-- )
    {
      HaArrayPut(task_group->tasks, j + 1, HaArray(task_group->tasks, j));
      HaArrayPut(costs, j + 1, HaArray(costs, j));
    }
    HaArrayPut(task_group->tasks, j + 1, task);
    HaArrayPut(costs, j + 1, cost);
    if( cost != -1 )
      KheTaskUnAssign(task);
  }
  HaArrayFree(costs);
}


/*****************************************************************************/
/*                                                                           */
/*  bool KheTaskGroupAssign(KHE_TASK_GROUP task_group, KHE_RESOURCE r)       */
/*                                                                           */
/*  Assign r to a task of task_group, if possible.                           */
/*                                                                           */
/*****************************************************************************/

bool KheTaskGroupAssign(KHE_TASK_GROUP task_group, KHE_RESOURCE r)
{
  KHE_TASK task;
  if( !KheTaskGroupHasUnassignedTask(task_group, &task) )
    return false;
  else if( task_group->tasks_sorted || HaArrayCount(task_group->tasks) <= 1 )
  {
    /* simple algorithm which assigns the first unassigned task, if any */
    return KheTaskAssignResource(task, r);
  }
  else
  {
    /* longer algorithm which includes sorting the tasks */
    if( !KheTaskAssignResourceCheck(task, r) )
      return false;
    KheTaskGroupSortTasks(task_group, r);
    task_group->tasks_sorted = true;
    return KheTaskGroupAssign(task_group, r);
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupUnAssign(KHE_TASK_GROUP task_group, KHE_RESOURCE r)     */
/*                                                                           */
/*  Unassign r from a task of task_group.                                    */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupUnAssign(KHE_TASK_GROUP task_group, KHE_RESOURCE r)
{
  int i;  KHE_TASK task;
  task = NULL;  /* keep compiler happy */
  HaArrayForEach(task_group->tasks, task, i)
    if( KheTaskAsstResource(task) == r )
      break;
  HnAssert(i < HaArrayCount(task_group->tasks),
    "KheTaskGroupUnAssign: r not assigned to any task of task_group");
  if( !KheTaskUnAssign(task) )
    HnAbort("KheTaskGroupUnAssign: cannot unassign");
}


/*****************************************************************************/
/*                                                                           */
/*  Submodule "debug"                                                        */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupDebug(KHE_TASK_GROUP task_group, int verbosity,         */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of task_group onto fp with the given indent.                 */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupDebug(KHE_TASK_GROUP task_group, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK task;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Task Group (%d tasks)", HaArrayCount(task_group->tasks));
    if( indent >= 0 && verbosity >= 2 )
    {
      fprintf(fp, "\n");
      HaArrayForEach(task_group->tasks, task, i)
	KheTaskDebug(task, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]", indent, "");
    }
    else
    {
      if( HaArrayCount(task_group->tasks) > 0 )
      {
	fprintf(fp, " ");
	KheTaskDebug(HaArrayFirst(task_group->tasks), 1, -1, fp);
	if( HaArrayCount(task_group->tasks) > 1 )
	  fprintf(fp, " etc.");
      }
      fprintf(fp, " ]");
    }
    if( indent >= 0 )
      fprintf(fp, "\n");
  }
}


/*****************************************************************************/
/*                                                                           */
/*  void KheTaskGroupsDebug(KHE_TASK_GROUPS task_groups, int verbosity,      */
/*    int indent, FILE *fp)                                                  */
/*                                                                           */
/*  Debug print of task_groups onto fp with the given indent.                */
/*                                                                           */
/*****************************************************************************/

void KheTaskGroupsDebug(KHE_TASK_GROUPS task_groups, int verbosity,
  int indent, FILE *fp)
{
  KHE_TASK_GROUP task_group;  int i;
  if( verbosity >= 1 )
  {
    if( indent >= 0 )
      fprintf(fp, "%*s", indent, "");
    fprintf(fp, "[ Task Groups (%d task groups)",
      HaArrayCount(task_groups->task_groups));
    if( indent >= 0 )
    {
      fprintf(fp, "\n");
      if( verbosity >= 2 )
	HaArrayForEach(task_groups->task_groups, task_group, i)
	  KheTaskGroupDebug(task_group, verbosity, indent + 2, fp);
      fprintf(fp, "%*s]\n", indent, "");
    }
    else
      fprintf(fp, " ]");
  }
}
